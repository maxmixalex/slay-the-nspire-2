"""Small public delivery fixtures for multipart resources and source receipts."""
import base64
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest import mock
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import build_web
import package_release
import repack_retained as retained
import split_content as split
import storage_parts as storage
import verify_offline_release as offline
from test_repack_retained import fixture


def resources(root):
    original = {}
    for code in ('eng','zhs'):
        source=retained.view(fixture(code))
        batches=retained.plan_images(source,fast_images='none')
        original[code]=retained.write_pack(source,batches,output_version=43)[0]
    common,locales,_ = split.split(original,compress_directory=True)
    index,parts,_ = storage.partition(common,original['eng'])
    (root/'nSpire2.dat.tns').write_bytes(index)
    for name,data in parts.items():(root/name).write_bytes(data)
    for code,data in locales.items():
        (root/f'nSpire2.{code}.dat.tns').write_bytes(storage.compress_blocks(data)[0])
    return original,common


class WebReleaseTest(unittest.TestCase):
    def test_multipart_is_joined_in_memory_and_bad_parts_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);original,common=resources(root)
            names=set(p.name for p in root.iterdir())
            result=build_web.resource_package(common_path=root/'nSpire2.dat.tns',
                eng_path=root/'nSpire2.eng.dat.tns',zhs_path=root/'nSpire2.zhs.dat.tns')
            self.assertEqual(result['mode'],'split')
            self.assertEqual(base64.b64decode(result['common']),common)
            self.assertEqual(set(p.name for p in root.iterdir()),names)
            for code,data in result['languages'].items():
                self.assertEqual(split.reconstruct(common,base64.b64decode(data),code),original[code])
            part=root/storage.PART_FILES[0]
            data=part.read_bytes();part.unlink()
            with self.assertRaises(FileNotFoundError):
                build_web.resource_package(common_path=root/'nSpire2.dat.tns',eng_path=root/'nSpire2.eng.dat.tns')
            part.write_bytes(b'BAD!'+data[4:])
            with self.assertRaises(ValueError):
                build_web.resource_package(common_path=root/'nSpire2.dat.tns',eng_path=root/'nSpire2.eng.dat.tns')

    def test_complete_release_embeds_multipart_and_honest_local_source_receipt(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);original,common=resources(root)
            (root/'launcher.tns').write_bytes(b'public synthetic launcher')
            wasm=root/'runtime.wasm';wasm.write_bytes(b'\0asm\1\0\0\0')
            argv=['package_release','--launcher',str(root/'launcher.tns'),
                '--common',str(root/'nSpire2.dat.tns'),'--eng',str(root/'nSpire2.eng.dat.tns'),
                '--zhs',str(root/'nSpire2.zhs.dat.tns'),'--wasm',str(wasm),
                '--output',str(root/'release'),'--local-checkpoints',
                '--runtime-checkpoint','a'*40,'--resource-checkpoint','b'*40,'--stage','public-web']
            with mock.patch.object(sys,'argv',argv),redirect_stdout(io.StringIO()):package_release.main()
            html=next((root/'release').glob('*.html')).read_text()
            embedded=json.loads(re.search(r'const packages=(.*);',html).group(1))
            self.assertEqual(base64.b64decode(embedded['common']),common)
            for code,data in embedded['languages'].items():
                self.assertEqual(split.reconstruct(common,base64.b64decode(data),code),original[code])
            self.assertIn('Corresponding runtime source checkpoint (local):\n'+'a'*40,html)
            self.assertIn('has not been published to GitHub',html)
            self.assertNotIn('/tree/'+'a'*40,html)
            actual_html=next((root/'release').glob('*.html'))
            receipt=offline.verify(actual_html,wasm,root/'nSpire2.dat.tns',
                {code:root/f'nSpire2.{code}.dat.tns' for code in ('eng','zhs')},
                'a'*40,local_checkpoints=True)
            self.assertTrue(receipt['complete'])
            self.assertEqual(receipt['locales']['eng']['version'],43)
            with self.assertRaisesRegex(ValueError,'Published source receipt is missing'):
                offline.verify(actual_html,wasm,root/'nSpire2.dat.tns',
                    {code:root/f'nSpire2.{code}.dat.tns' for code in ('eng','zhs')},'a'*40)
            wrong_wasm=root/'wrong.wasm';wrong_wasm.write_bytes(wasm.read_bytes()+b'changed')
            with self.assertRaisesRegex(ValueError,'Embedded WebAssembly differs'):
                offline.verify(actual_html,wrong_wasm,root/'nSpire2.dat.tns',
                    {code:root/f'nSpire2.{code}.dat.tns' for code in ('eng','zhs')},
                    'a'*40,local_checkpoints=True)
            with zipfile.ZipFile(next((root/'release').glob('*.zip'))) as archive:
                info=json.loads(archive.read('data/BUILD-INFO.json'))
                self.assertTrue(info['local_checkpoints']);self.assertTrue(info['multipart_artwork'])
                self.assertFalse(info['calculator_only'])
                readme=archive.read('data/README.txt').decode()
                self.assertIn('Exit the game before updating; keep existing files in state.',readme)
                self.assertIn('Replace the entire old data folder',readme)
                self.assertIn("Do not overwrite the existing state folder",readme)
                self.assertIn('older multipart installation',readme)
                self.assertNotIn('/tree/'+'a'*40,readme)
                for name in ('nSpire2.dat.tns',*storage.PART_FILES):
                    self.assertEqual(archive.read('data/'+name),(root/name).read_bytes())

            # The existing default still records an explicitly published ref.
            with redirect_stdout(io.StringIO()):
                build_web.package_runtime(destination=root/'published.html',
                    common_path=root/'nSpire2.dat.tns',eng_path=root/'nSpire2.eng.dat.tns',
                    wasm_path=wasm,source_checkpoint='c'*40)
            self.assertIn('https://github.com/maxmixalex/slay-the-nspire-2/tree/'+'c'*40,
                          (root/'published.html').read_text())


if __name__=='__main__':unittest.main()
