from pathlib import Path
import json
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import bind_source_image_oracle as binding
import export_source_image_oracle as export
import implicit_tile_geometry_ui22 as convert
from test_implicit_tile_geometry_ui22 import fixture


class SourceOracleBindingTest(unittest.TestCase):
    def prepare(self, folder):
        root = Path(folder)
        before = fixture()
        after, _ = convert.convert_bytes(before, root / 'cache', workers=1, align_suffix=False)
        reference, target, oracle, output = [root / name for name in ('old.nsp', 'new.nsp', 'old.bin', 'bound.bin')]
        reference.write_bytes(before); target.write_bytes(after)
        export.export(before, oracle)
        return reference, target, oracle, output

    def test_old_reader_body_stays_exact_when_new_target_format_and_size_differ(self):
        with tempfile.TemporaryDirectory() as folder:
            reference, target, oracle, output = self.prepare(folder)
            result = binding.bind(reference, target, oracle, output)
            self.assertTrue(result['image_body_bytes_identical'])
            self.assertEqual(result['reference_pack_version'], 45)
            self.assertEqual(result['target_pack_version'], 46)
            old, bound = oracle.read_bytes(), output.read_bytes()
            self.assertEqual(old[28:], bound[28:])
            self.assertNotEqual(old[:28], bound[:28])
            self.assertEqual(struct.unpack_from('<I', bound, 20)[0], target.stat().st_size)
            self.assertEqual(result, json.loads(output.with_suffix('.bin.json').read_text()))

    def test_foreign_or_partial_reference_and_corrupt_spool_fail_closed(self):
        with tempfile.TemporaryDirectory() as folder:
            reference, target, oracle, output = self.prepare(folder)
            receipt = oracle.with_suffix('.bin.json'); good = receipt.read_bytes()
            for key, value in (('complete', False), ('first_image', 1), ('images', 1),
                               ('input_sha256', '0' * 64), ('oracle_sha256', '0' * 64)):
                invalid = json.loads(good); invalid[key] = value; receipt.write_text(json.dumps(invalid))
                with self.assertRaises(ValueError): binding.bind(reference, target, oracle, output)
                self.assertFalse(output.exists())
            receipt.write_bytes(good)
            original = oracle.read_bytes()
            for invalid in (original[:-1], original + b'extra', original[:48] + bytes((original[48] ^ 1,)) + original[49:]):
                oracle.write_bytes(invalid)
                with self.assertRaises(ValueError): binding.bind(reference, target, oracle, output)
                self.assertFalse(output.exists())
            self.assertEqual(list(Path(folder).glob('bound.bin.*')), [])

    def test_target_shape_change_or_output_overwrite_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            reference, target, oracle, output = self.prepare(folder)
            with self.assertRaises(ValueError): binding.bind(reference, target, oracle, oracle)
            changed = bytearray(target.read_bytes()); struct.pack_into('<H', changed, 44, 31)
            target.write_bytes(changed)
            with self.assertRaisesRegex(ValueError, 'identities'):
                binding.bind(reference, target, oracle, output)


if __name__ == '__main__':
    unittest.main()
