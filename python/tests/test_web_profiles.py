"""The actual embedded storage helper, without a browser or private assets."""

import shutil
import subprocess
import unittest

from nspire_converter.paths import ROOT


class WebProfiles(unittest.TestCase):
    @unittest.skipUnless(shutil.which("node"), "Node is needed for the browser storage helper")
    def test_isolation_and_failed_deletion(self):
        subprocess.run(
            ["node", str(ROOT / "cpp/platform/web/tests/web_profiles_storage_test.js")], check=True
        )

    @unittest.skipUnless(shutil.which("node"), "Node is needed for the browser command controls")
    def test_command_press_release_and_cancellation(self):
        subprocess.run(
            ["node", str(ROOT / "cpp/platform/web/tests/web_command_buttons_test.js")], check=True
        )


if __name__ == "__main__":
    unittest.main()
