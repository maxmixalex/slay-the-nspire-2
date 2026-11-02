import tempfile
import unittest
import argparse
import json
import zipfile
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


class ThreeArchiveDelivery(unittest.TestCase):
    def assemble(self, root, maximum=70_000_000):
        launcher = root / 'launcher.tns'
        launcher.write_bytes(b'launcher fixture')
        for profile in package.PROFILES:
            directory = root / profile
            directory.mkdir()
            (directory / 'nSpire2.dat.tns').write_bytes(b'common fixture')
            for code in package.LANGUAGES:
                (directory / f'nSpire2.{code}.dat.tns').write_bytes((profile + ':' + code).encode())
        args = argparse.Namespace(
            output=root / 'output', stage='ui24', launcher=launcher,
            animated=root / 'animated', no_animations=root / 'no-animations',
            runtime_checkpoint='a' * 40, resource_checkpoint='b' * 40,
            no_animations_resource_checkpoint=None,
            maximum_animated_bytes=maximum, allow_over_budget_candidate=False)
        invocations = []

        def package_one(command, check):
            self.assertTrue(check)
            stage = command[command.index('--stage') + 1]
            invocations.append(stage)
            destination = Path(command[command.index('--output') + 1])
            destination.mkdir()
            with zipfile.ZipFile(destination / 'fixture-calculator.zip', 'w') as archive:
                archive.writestr('nSpire2.tns', b'launcher fixture')
                archive.writestr('README.txt', b'documentation counts against installation budget')
            (destination / 'fixture-validation.json').write_text('{}\n')

        def resource_info(directory, expected_profile, require_timeline):
            self.assertTrue(require_timeline)
            self.assertEqual(directory.name, expected_profile)
            return {'resource_format': 46, 'required_bytes': 1, 'locales': {}}

        with patch.object(package, 'inspect_resources', side_effect=resource_info), \
             patch.object(package.subprocess, 'run', side_effect=package_one):
            result = package.build(args)
        return result, invocations

    def test_stage_identity_survives_three_archive_assembly(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            result, stages = self.assemble(root)
            self.assertEqual(stages, ['ui24-animated', 'ui24-no-animations'])
            self.assertEqual(result['stage'], 'ui24')
            self.assertEqual(json.loads((root / 'output/UI24-delivery-manifest.json').read_text()), result)
            self.assertEqual(len(list((root / 'output').glob('*.zip'))), 3)
            optional = root / 'output' / result['archives']['optional-languages']['file']
            with zipfile.ZipFile(optional) as archive:
                self.assertEqual(len(archive.namelist()), 29)
                self.assertIn(b'UI24', archive.read('README-languages.txt'))
                for profile in package.PROFILES:
                    for code in package.LANGUAGES:
                        if code not in ('eng', 'zhs'):
                            self.assertEqual(archive.read(f'{profile}/data/nSpire2.{code}.dat.tns'),
                                             (profile + ':' + code).encode())

    def test_complete_archive_budget_counts_documentation(self):
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(ValueError, 'Complete animated archive expands'):
                self.assemble(Path(temporary), maximum=20)


if __name__ == '__main__':
    unittest.main()
