"""Exercise reviewed source bindings with actual Windows path semantics."""
from pathlib import Path, PureWindowsPath
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import locale_profile


class WindowsSourcePath:
    """Read real fixture files while returning Windows relative path objects."""
    def __init__(self, path):
        self.path = path
        self.name, self.suffix = path.name, path.suffix

    def __lt__(self, other):
        return self.path < other.path

    def read_text(self, *args, **kwargs):
        return self.path.read_text(*args, **kwargs)

    def relative_to(self, root):
        return PureWindowsPath(self.path.relative_to(root).as_posix())


class LocaleSourcePaths(unittest.TestCase):
    def test_windows_paths_validate_the_real_reviewed_catalog(self):
        expected = locale_profile.source_literals()
        original_rglob = Path.rglob

        def windows_rglob(folder, pattern):
            return (WindowsSourcePath(path) for path in original_rglob(folder, pattern))

        # The adapter must reproduce the failure mode, not return POSIX strings.
        calendar = WindowsSourcePath(locale_profile.ROOT / 'include/ui_calendar_locale_data.inc')
        relative = calendar.relative_to(locale_profile.ROOT)
        self.assertIsInstance(relative, PureWindowsPath)
        self.assertEqual(str(relative), 'include\\ui_calendar_locale_data.inc')
        with patch.object(Path, 'rglob', windows_rglob):
            locale_profile.validate_generated()
            self.assertEqual(locale_profile.source_literals(), expected)
            self.assertEqual(locale_profile.source_literals()['午前'],
                             ['include/ui_calendar_locale_data.inc'])


if __name__ == '__main__':
    unittest.main()
