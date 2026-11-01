"""Source-revision baseline ownership without accepting changed cached data."""
import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import ui13_resources as ui13


def manifest(signature, *, retained=None, extra=None):
    endpoint=ui13.retained_endpoint() if retained is not None else ui13.AUXILIARY
    value=dict(schema=1,reference='v0.111.0 / 41cef1ea',signature=signature,
               images=[dict(auxiliary=i) for i in range(ui13.BASE_AUX,endpoint)])
    if retained is not None:value['retained_extension']=dict(signature=retained)
    if extra is not None:value['extra']=extra
    return json.dumps(value,sort_keys=True).encode()


class ExtensionBaselineTests(unittest.TestCase):
    def test_new_source_identity_archives_exact_previous_bytes(self):
        for retained in (False,True):
            with self.subTest(retained=retained), tempfile.TemporaryDirectory() as folder, \
                 contextlib.redirect_stdout(io.StringIO()):
                path=Path(folder)/'baseline.json'
                # The retained identity can change independently (e.g. noise
                # input changes), even if the base source signature is equal.
                old=manifest('a'*64,retained='b'*64 if retained else None)
                new=manifest('a'*64 if retained else 'c'*64,
                             retained='c'*64 if retained else None)
                endpoint=ui13.retained_endpoint() if retained else ui13.AUXILIARY
                ui13.extension_baseline(path,old,endpoint,'mismatch',retained=retained)
                ui13.extension_baseline(path,new,endpoint,'mismatch',retained=retained)
                archive=path.parent/'superseded-baselines'/('baseline-'+ui13.tiles.sha256(old)+'.json')
                self.assertEqual(archive.read_bytes(),old)
                self.assertEqual(path.read_bytes(),new)
                # Exact repeats and an already archived prior revision are
                # idempotent; history never gets rewritten or discarded.
                ui13.extension_baseline(path,new,endpoint,'mismatch',retained=retained)
                path.write_bytes(old)
                ui13.extension_baseline(path,new,endpoint,'mismatch',retained=retained)
                self.assertEqual(archive.read_bytes(),old)
                self.assertEqual(path.read_bytes(),new)

    def test_same_identity_or_invalid_catalog_never_replaces_baseline(self):
        old=manifest('a'*64)
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'baseline.json';path.write_bytes(old)
            for changed in (manifest('a'*64,extra='changed under the same identity'),
                            manifest('invalid'),manifest('b'*64,retained='c'*64)):
                with self.subTest(changed=changed[:100]), self.assertRaises(ValueError):
                    ui13.extension_baseline(path,changed,ui13.AUXILIARY,'mismatch')
                self.assertEqual(path.read_bytes(),old)
            self.assertFalse((path.parent/'superseded-baselines').exists())
            # A damaged previous manifest cannot be discarded as an upgrade.
            path.write_bytes(manifest('invalid'))
            with self.assertRaises(ValueError):
                ui13.extension_baseline(path,manifest('b'*64),ui13.AUXILIARY,'mismatch')
            self.assertEqual(path.read_bytes(),manifest('invalid'))

    def test_changed_archive_is_rejected_before_installing_new_baseline(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'baseline.json';old=manifest('a'*64);path.write_bytes(old)
            archive=path.parent/'superseded-baselines'/('baseline-'+ui13.tiles.sha256(old)+'.json')
            archive.parent.mkdir();archive.write_bytes(b'changed history')
            with self.assertRaisesRegex(ValueError,'Changed archived'):
                ui13.extension_baseline(path,manifest('b'*64),ui13.AUXILIARY,'mismatch')
            self.assertEqual(path.read_bytes(),old)
            self.assertEqual(archive.read_bytes(),b'changed history')


if __name__=='__main__':unittest.main()
