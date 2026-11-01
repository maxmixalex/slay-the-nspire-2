import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import package_variants_ui23 as package


class LocaleEditionGate(unittest.TestCase):
    def inspect(self, versions=None, reduced=(), strings=None, require_timeline=False):
        versions = versions or {}
        strings = strings or {}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'nSpire2.dat.tns').write_bytes(b'common fixture')
            for code in package.LANGUAGES:
                (root / f'nSpire2.{code}.dat.tns').write_bytes(code.encode())

            def reconstructed(common, localized, code):
                # Container reconstruction has its own round-trip tests.
                # Exercise the delivery gate with independently valid headers
                # that came from mismatched conversion outputs.
                header = bytearray(40)
                header[:4] = b'NSP2'
                header[4:8] = versions.get(code, 46).to_bytes(4, 'little')
                header[24:28] = strings.get(code, 8961).to_bytes(4, 'little')
                if code in reduced:
                    header[36:40] = b'NAF1'
                return bytes(header)

            with patch.object(package, 'reconstruct', side_effect=reconstructed):
                return package.inspect_resources(root, 'animated', require_timeline=require_timeline)

    def test_matching_all_sixteen_locales_are_reported(self):
        result = self.inspect()
        self.assertEqual(result['resource_format'], 46)
        self.assertEqual(len(result['locales']), 16)
        self.assertEqual(len(set(result['locales']) - {'eng', 'zhs'}), 14)
        self.assertTrue(all(row['resource_format'] == 46 and row['animation_profile'] == 'animated'
                            for row in result['locales'].values()))

    def test_optional_locale_cannot_use_an_older_format(self):
        with self.assertRaisesRegex(ValueError, 'resource format.*fra'):
            self.inspect({'fra': 45})

    def test_optional_locale_cannot_use_the_other_animation_profile(self):
        with self.assertRaisesRegex(ValueError, 'animation profile.*fra'):
            self.inspect(reduced=('fra',))

    def test_ui24_checks_timeline_catalog_in_optional_locales(self):
        result = self.inspect(require_timeline=True)
        self.assertTrue(all(row['string_count'] == 8961 for row in result['locales'].values()))
        with self.assertRaisesRegex(ValueError, '8961-string Timeline catalog: fra'):
            self.inspect(strings={'fra': 8704}, require_timeline=True)


if __name__ == '__main__':
    unittest.main()
