import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from nspire_converter import phobia_swarm_resources as swarm


class SwarmCurveValidationTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.header = self.root / "cpp/include/detail/data/phobia_swarm_curves.hpp"
        self.header.parent.mkdir(parents=True)
        self.metadata = {
            "tables": {
                "Scale": list(range(129)),
                "Orbit": [i * 3 for i in range(129)],
                "Directional": [i - 64 for i in range(129)],
            }
        }

    def check_header(self, text):
        self.header.write_text(text)
        with (
            patch.object(swarm, "ROOT", self.root),
            patch.object(swarm, "Pack"),
            patch.object(swarm, "source_metadata", return_value=self.metadata),
            patch.object(sys, "argv", ["swarm", "--pck", str(self.root / "original.pck")]),
            contextlib.redirect_stdout(io.StringIO()),
        ):
            swarm.main()

    def test_generated_and_formatted_headers_match(self):
        generated = swarm.curve_header(self.metadata)
        self.check_header(generated)
        self.check_header("#pragma once\n" + generated)
        self.check_header(
            "#pragma once\n/* Formatter comment. */\n"
            + generated.replace("={", " = {\n").replace(",", ", ").replace("};", "\n};")
        )

    def test_changed_declarations_and_samples_fail(self):
        generated = swarm.curve_header(self.metadata)
        changes = (
            generated.replace("Scale[", "Other[", 1),
            generated.replace("int16_t", "int32_t", 1),
            generated.replace("[129]", "[128]", 1),
            generated.replace("0,1,2,", "0,2,2,", 1),
            generated.replace("-64", "64", 1),
            generated.replace(" 128,\n", "", 1),
            generated + "static constexpr int16_t Extra = 0;\n",
        )
        for index, changed in enumerate(changes):
            self.assertNotEqual(generated, changed)
            with (
                self.subTest(case=index),
                self.assertRaisesRegex(ValueError, "Checked-in swarm curves differ"),
            ):
                self.check_header(changed)


if __name__ == "__main__":
    unittest.main()
