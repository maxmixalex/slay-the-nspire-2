"""Public RGR3 conversion through concern split, NSL3 and release readback."""
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import re
import struct
import sys
import tempfile
import unittest
from unittest import mock
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import compact_retained_colors as colors
import compact_retained_planes as planes
from content_version import MAXIMUM_CONTENT_VERSION
import content_tiles as t
import package_release
import repack_retained as retained
import split_content as split
import storage_parts as storage
from test_repack_retained import fixture, stored


class RetainedPlanesPipelineTest(unittest.TestCase):
    def test_runtime_and_host_agree_on_current_maximum(self):
        header = (ROOT/'include/index_prediction.hpp').read_text()
        maximum = re.search(r'\bMaximumContentVersion\s*=\s*(\d+)', header)
        self.assertIsNotNone(maximum)
        self.assertEqual(int(maximum.group(1)), MAXIMUM_CONTENT_VERSION)

    def test_convert_partition_merge_and_package_current_rgr3(self):
        self.check_pipeline(42,43)

    def test_later_version_is_not_downgraded_by_retained_conversion(self):
        self.check_pipeline(MAXIMUM_CONTENT_VERSION,MAXIMUM_CONTENT_VERSION)

    def check_pipeline(self,input_version,output_version):
        # Legal deliberately uncompressed RGR1 gives this small fixture a
        # guaranteed RGR3 winner. Actual pixels, hidden RGB and alpha exercise
        # the production converter; there is no private-game input here.
        original = {}
        for code in ('eng', 'zhs'):
            source = retained.view(fixture(code))
            batches = retained.plan_images(source, fast_images='none')
            for batch in batches:
                batch.encoded = stored(b''.join(record.raw for record in batch.records))
            original[code] = retained.write_pack(source, batches, output_version=input_version)[0]
        common, locales, _ = split.split(original, compress_directory=True)
        index, parts, _ = storage.partition(common, original['eng'])
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder); source = root/'input'; source.mkdir()
            output, work = root/'converted', root/'work'
            (source/'nSpire2.dat.tns').write_bytes(index)
            for name, data in parts.items():(source/name).write_bytes(data)
            for code, data in locales.items():
                (source/f'nSpire2.{code}.dat.tns').write_bytes(storage.compress_blocks(data)[0])
            argv = ['compact_retained_planes', '--input', str(source), '--output', str(output),
                    '--work', str(work), '--workers', '1', '--locales', 'eng', 'zhs']
            with mock.patch.object(sys, 'argv', argv), redirect_stdout(io.StringIO()):planes.main()
            receipt = json.loads((output/'retained-planes-receipt.json').read_text())
            self.assertTrue(receipt['complete'])
            self.assertGreater(receipt['changed_groups'], 0)
            common = storage.join_parts((output/'nSpire2.dat.tns').read_bytes(),
                                        {n:(output/n).read_bytes() for n in storage.PART_FILES})
            for code in ('eng', 'zhs'):
                locale = (output/f'nSpire2.{code}.dat.tns').read_bytes()
                self.assertEqual(locale[:4], b'NSL3')
                logical = split.reconstruct(common, locale, code)
                self.assertEqual(logical, (work/f'{code}-planes{output_version}.nsp').read_bytes())
                self.assertEqual(split.language_resource_version(locale), output_version)
                before, after = colors.view(original[code]), colors.view(logical)
                self.assertTrue(any(after.data[g.offset:g.offset+4] == b'RGR3'
                                    for g in colors.groups(after)))
                old_reader, new_reader = t.TileReader(before), t.TileReader(after)
                for image in range(len(after.images)):
                    a, b = (old_reader, new_reader) if before.images[image][3] == 6 else (before, after)
                    self.assertEqual(a.image_raw(image), b.image_raw(image))
            launcher = root/'synthetic.tns'; launcher.write_bytes(b'synthetic launcher fixture')
            argv = ['package_release', '--launcher', str(launcher), '--common', str(output/'nSpire2.dat.tns'),
                    '--eng', str(output/'nSpire2.eng.dat.tns'), '--zhs', str(output/'nSpire2.zhs.dat.tns'),
                    '--output', str(root/'release'), '--calculator-only', '--local-checkpoints',
                    '--runtime-checkpoint', 'a'*40, '--resource-checkpoint', 'b'*40, '--stage', 'synthetic43']
            with mock.patch.object(sys, 'argv', argv), redirect_stdout(io.StringIO()):package_release.main()
            archives = list((root/'release').glob('*.zip')); self.assertEqual(len(archives), 1)
            with zipfile.ZipFile(archives[0]) as archive:
                info = json.loads(archive.read('data/BUILD-INFO.json'))
                self.assertEqual(info['resource_format'], output_version)
                self.assertTrue(info['local_checkpoints'])
                for name in ('nSpire2.dat.tns', *storage.PART_FILES,
                             'nSpire2.eng.dat.tns', 'nSpire2.zhs.dat.tns'):
                    self.assertEqual(archive.read('data/'+name), (output/name).read_bytes())
            # A repeated content-addressed-cache run goes through the same full
            # equality proof and emits identical physical installation files.
            argv = ['compact_retained_planes', '--input', str(source), '--output', str(root/'again'),
                    '--work', str(work), '--workers', '1', '--locales', 'eng', 'zhs']
            with mock.patch.object(sys, 'argv', argv), redirect_stdout(io.StringIO()):planes.main()
            for path in output.glob('*.tns'):
                self.assertEqual(path.read_bytes(), (root/'again'/path.name).read_bytes())


if __name__ == '__main__':unittest.main()
