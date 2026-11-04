"""Standalone seed-code commands do not require game data or native helpers."""

import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from nspire_converter import converter
from nspire_converter.seed_codec import encode_seed, encode_seed_file


class SeedEntryTests(unittest.TestCase):
    def test_actual_original_dll_vector_runs_before_game_discovery(self):
        with (
            patch.object(
                converter, "locate_game", side_effect=AssertionError("Game lookup is unnecessary")
            ),
            patch.object(
                converter,
                "configure_helpers",
                side_effect=AssertionError("Native helpers are unnecessary"),
            ),
        ):
            for seed, expected in (
                ("oldA", "SEED64-EC8011CFFF0DFCC3"),
                (" \t\r\n\v\f", "SEED64-EF46DB3751D8E999"),
            ):
                out = io.StringIO()
                with contextlib.redirect_stdout(out):
                    self.assertEqual(converter.main(["--encode-seed", seed]), 0)
                self.assertEqual(out.getvalue(), expected + "\n")

    def test_file_entry_matches_whole_text_across_chunk_and_trim_boundaries(self):
        text = " \t" + ("OIoldA raw:10 SEED64:10 \x1c" * 5000) + "\r\n" + (" " * 70000)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "seed.txt"
            path.write_bytes(text.encode("ascii"))
            out = io.StringIO()
            with (
                patch.object(
                    converter,
                    "locate_game",
                    side_effect=AssertionError("Game lookup is unnecessary"),
                ),
                contextlib.redirect_stdout(out),
            ):
                self.assertEqual(converter.main(["--encode-seed-file", str(path)]), 0)
            self.assertEqual(out.getvalue().strip(), encode_seed(text))
            # ASCII controls outside the original Trim set remain significant.
            path.write_bytes(b"oldA\x1c")
            self.assertNotEqual(encode_seed_file(path), encode_seed("oldA"))

    def test_prefixes_are_ordinary_text_and_non_ascii_is_explicitly_rejected(self):
        self.assertNotEqual(encode_seed("raw:10"), encode_seed("10"))
        self.assertNotEqual(encode_seed("SEED64:10"), encode_seed("10"))
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(converter.main(["--encode-seed", "中文"]), 1)


if __name__ == "__main__":
    unittest.main()
