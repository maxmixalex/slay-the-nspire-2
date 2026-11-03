"""Source locale failures must stop a bundle before PyInstaller or game work."""
import argparse
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import build_converter_bundle as bundle
import locale_profile


class ConverterBundlePreflight(unittest.TestCase):
    def arguments(self, folder):
        root = Path(folder)
        # Deliberately absent helpers and no original game assets.
        return argparse.Namespace(helpers=root/'helpers', output=root/'output', work=root/'work')

    def test_missing_reviewed_binding_fails_before_pyinstaller(self):
        target = locale_profile.ROOT/'config/locale-ui-inline.json'
        original_read = Path.read_text
        config = json.loads(original_read(target))
        removed = next(row for row in config['direct_bindings'] if row['source'] == '鼠标速度')
        config['direct_bindings'].remove(removed)

        def read(path, *args, **kwargs):
            return json.dumps(config, ensure_ascii=False) if path == target else original_read(path, *args, **kwargs)

        with tempfile.TemporaryDirectory() as folder, patch.object(Path, 'read_text', read), \
                patch.object(bundle.importlib.util, 'find_spec') as dependency, \
                patch.object(bundle.subprocess, 'run') as process:
            with self.assertRaisesRegex(ValueError, 'New UI literals need reviewed locale bindings:.*鼠标速度'):
                bundle.build(self.arguments(folder))
            dependency.assert_not_called()
            process.assert_not_called()
            self.assertEqual(list(Path(folder).iterdir()), [])

    def test_valid_source_reaches_dependency_gate_without_game_assets(self):
        # Exercise the actual catalog and generated source, then deliberately
        # stop at the next build dependency rather than building a fake bundle.
        with tempfile.TemporaryDirectory() as folder, \
                patch.object(bundle.importlib.util, 'find_spec', return_value=None) as dependency, \
                patch.object(bundle.subprocess, 'run') as process:
            with self.assertRaisesRegex(ValueError, 'Install PyInstaller'):
                bundle.build(self.arguments(folder))
            dependency.assert_called_once_with('PyInstaller')
            process.assert_not_called()
            self.assertEqual(list(Path(folder).iterdir()), [])

    def test_generated_index_drift_also_fails_before_freezing(self):
        target = locale_profile.ROOT/'src/locale_ui_index.inc'
        original_read = Path.read_text

        def read(path, *args, **kwargs):
            result = original_read(path, *args, **kwargs)
            return result + '\n' if path == target else result

        with tempfile.TemporaryDirectory() as folder, patch.object(Path, 'read_text', read), \
                patch.object(bundle.importlib.util, 'find_spec') as dependency, \
                patch.object(bundle.subprocess, 'run') as process:
            with self.assertRaisesRegex(ValueError, 'Locale generated index drift: src/locale_ui_index.inc'):
                bundle.build(self.arguments(folder))
            dependency.assert_not_called()
            process.assert_not_called()


if __name__ == '__main__':
    unittest.main()
