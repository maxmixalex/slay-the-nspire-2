"""Release packaging preserves resource bytes and installation metadata."""

import json
import tempfile
import unittest
import zipfile
from pathlib import Path
from unittest.mock import patch

import package_release
from nspire_converter.source_version import decode_version, encode_version, installed_version


class ReleasePackagingTests(unittest.TestCase):
    def test_version_comes_from_the_supplied_installation(self):
        with tempfile.TemporaryDirectory() as directory:
            game = Path(directory)
            (game / "release_info.json").write_text(json.dumps({"version": "v9.88.7"}))
            version = installed_version(game)
            self.assertEqual(version, "9.88.7")
            self.assertEqual(decode_version(encode_version(version)), version)
            self.assertIn(b"StS II v9.88.7", package_release.instructions("full", version))

    def test_archive_budget_includes_launcher_and_instructions(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            launcher = root / "launcher.tns"
            launcher.write_bytes(b"launcher")
            archive = root / "release.zip"
            with self.assertRaisesRegex(ValueError, "above the 10-byte limit"):
                package_release.write_archive(
                    archive,
                    {"nSpire2.tns": launcher},
                    {"README.txt": b"instructions"},
                    maximum_bytes=10,
                )
            self.assertFalse(archive.exists())

    def test_all_three_archives_keep_mode_and_language_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            game = root / "game"
            game.mkdir()
            (game / "release_info.json").write_text('{"version": "v9.88.7"}')
            launcher = root / "launcher.tns"
            launcher.write_bytes(b"launcher")
            for profile in ("full", "reduced"):
                target = root / profile
                target.mkdir()
                for name in (
                    "nSpire2.dat.tns",
                    *(f"nSpire2.{code}.dat.tns" for code in package_release.LANGUAGES),
                ):
                    (target / name).write_bytes((profile + "/" + name).encode())

            def edition_files(directory, profile):
                self.assertEqual(directory.name, profile)
                return {
                    "data/" + name: directory / name
                    for name in ("nSpire2.dat.tns", "nSpire2.eng.dat.tns", "nSpire2.zhs.dat.tns")
                }

            with (
                patch.object(package_release, "verify_release"),
                patch.object(package_release, "edition_files", edition_files),
            ):
                result = package_release.package(
                    game, root / "full", root / "reduced", launcher, root / "release"
                )
            self.assertEqual(result["port_version"], "0.1.0")
            self.assertEqual(result["source_game_version"], "9.88.7")
            for profile in ("full", "reduced"):
                with zipfile.ZipFile(result[profile]["path"]) as archive:
                    self.assertIn(b"SIL OPEN FONT LICENSE", archive.read("data/Credits.txt"))
                    self.assertEqual(archive.read("nSpire2.tns"), launcher.read_bytes())
                    self.assertEqual(
                        archive.read("data/nSpire2.version.tns"), encode_version("9.88.7")
                    )
                    self.assertEqual(
                        archive.read("data/nSpire2.eng.dat.tns"),
                        (root / profile / "nSpire2.eng.dat.tns").read_bytes(),
                    )
                    self.assertFalse(any(name.startswith("state/") for name in archive.namelist()))
            with zipfile.ZipFile(result["languages"]["path"]) as archive:
                names = [name for name in archive.namelist() if name.endswith(".tns")]
                self.assertEqual(len(names), 28)
                for profile in ("full", "reduced"):
                    for code in set(package_release.LANGUAGES) - {"eng", "zhs"}:
                        name = f"nSpire2.{code}.dat.tns"
                        self.assertEqual(
                            archive.read(f"{profile}/data/{name}"),
                            (root / profile / name).read_bytes(),
                        )


if __name__ == "__main__":
    unittest.main()
