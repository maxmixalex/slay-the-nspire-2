"""The actual embedded storage helper, without a browser or private assets."""
from pathlib import Path
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]


class WebProfiles(unittest.TestCase):
    @unittest.skipUnless(shutil.which('node'), 'Node is needed for the browser storage helper')
    def test_isolation_and_failed_deletion(self):
        subprocess.run(['node', str(ROOT / 'tests/web_profiles_storage_test.js')], check=True)


if __name__ == '__main__':
    unittest.main()
