"""Recovery and ownership gates for the production workspace collector."""

import contextlib
import io
import json
import subprocess
import tempfile
import time
import unittest
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from unittest.mock import patch

from nspire_converter import converter as c
from nspire_converter.converter_workspace import WorkLock, Workspace


class WorkspaceTests(unittest.TestCase):
    def make_chain(self, root):
        stages = []
        for name in ("one", "two", "three"):
            output = root / name / "pack.nsp"
            arguments = ("--output", str(output))
            if stages:
                arguments = ("--input", str(stages[-1].outputs[0]), *arguments)
            stages.append(c.Stage(name, "fixture", arguments, (output,)))
        return stages

    def test_released_ancestors_resume_and_changed_output_rebuilds(self):
        with tempfile.TemporaryDirectory() as directory, contextlib.redirect_stdout(io.StringIO()):
            root = Path(directory)
            stages = self.make_chain(root)
            calls = []

            def convert(command, **kwargs):
                output = Path(command[command.index("--output") + 1])
                calls.append(output.parent.name)
                if "--input" in command:
                    data = Path(command[command.index("--input") + 1]).read_bytes() + b" next"
                else:
                    data = b"original"
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(data)

            with (
                patch.object(c.subprocess, "run", side_effect=convert),
                patch.object(Workspace, "KEEP", frozenset(("three",))),
            ):
                c.run_stages(stages, root, "source", compact=True)
                self.assertEqual(calls, ["one", "two", "three"])
                self.assertFalse(stages[0].outputs[0].exists())
                self.assertFalse(stages[1].outputs[0].exists())
                expected = stages[2].outputs[0].read_bytes()
                c.run_stages(stages, root, "source", compact=True)
                self.assertEqual(len(calls), 3)
                stages[2].outputs[0].write_bytes(b"changed")
                c.run_stages(stages, root, "source", compact=True)
                self.assertEqual(calls, ["one", "two", "three"] * 2)
                self.assertEqual(stages[2].outputs[0].read_bytes(), expected)

    def test_failure_retains_required_inputs_then_recovers(self):
        with tempfile.TemporaryDirectory() as directory, contextlib.redirect_stdout(io.StringIO()):
            root = Path(directory)
            stages = self.make_chain(root)
            fail = [True]

            def convert(command, **kwargs):
                output = Path(command[command.index("--output") + 1])
                if output.parent.name == "two" and fail[0]:
                    raise subprocess.CalledProcessError(1, command)
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(b"valid")

            with (
                patch.object(c.subprocess, "run", side_effect=convert),
                patch.object(Workspace, "KEEP", frozenset(("three",))),
            ):
                with self.assertRaises(subprocess.CalledProcessError):
                    c.run_stages(stages, root, "source", compact=True)
                self.assertTrue(stages[0].outputs[0].is_file())
                self.assertFalse(
                    json.loads((root / "receipts/pipeline.json").read_text())["complete"]
                )
                fail[0] = False
                c.run_stages(stages, root, "source", compact=True)
                self.assertTrue(stages[-1].outputs[0].is_file())

    def test_unowned_directory_and_external_input_survive(self):
        with tempfile.TemporaryDirectory() as directory, contextlib.redirect_stdout(io.StringIO()):
            root = Path(directory)
            stages = self.make_chain(root)
            first = root / "one"
            first.mkdir()
            (first / "owner.txt").write_text("keep")

            def convert(command, **kwargs):
                output = Path(command[command.index("--output") + 1])
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(b"valid")

            with patch.object(c.subprocess, "run", side_effect=convert):
                c.run_stages(stages, root, "source", compact=True)
            self.assertEqual((first / "owner.txt").read_text(), "keep")
            self.assertTrue(stages[0].outputs[0].is_file())

    def test_language_request_reaches_every_locale_writer(self):
        recipe = c.Recipe(
            "/game", "/work", "/runtime", "dotnet", "/noise", "-", languages=("eng", "zhs")
        )
        recipe.source_stages()
        names = ("editions", "ui12", "startup", "swarm", "ui13", "ui14", "ui15")
        for stage in recipe.stages:
            if stage.name in names:
                self.assertEqual(
                    stage.arguments[stage.arguments.index("--languages") + 1], "eng,zhs"
                )
                self.assertFalse(any("jpn.dat.tns" in str(path) for path in stage.outputs))
        editions = next(stage for stage in recipe.stages if stage.name == "editions")
        self.assertNotIn("--keep-logical", editions.arguments)

    def test_shared_scratch_lives_until_its_last_consumer(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            cache = root / "cache/sample"
            stages = [
                c.Stage("sample", "sample", ("--work", str(cache)), (cache / "sample.json",)),
                c.Stage(
                    "first",
                    "reader",
                    ("--input", str(cache / "sample.json"), "--output", str(root / "first/out")),
                    (root / "first/out",),
                ),
                c.Stage(
                    "last",
                    "reader",
                    ("--input", str(cache / "sample.json"), "--output", str(root / "last/out")),
                    (root / "last/out",),
                ),
            ]
            workspace = Workspace(stages, root, "source", c.digest, c.atomic_json)
            cache.mkdir(parents=True)
            (cache / "sample.json").write_text("{}")
            (cache / "pixels.bin").write_bytes(b"pixels")
            workspace.reclaim(1)
            self.assertTrue((cache / "pixels.bin").is_file())
            workspace.reclaim(2)
            self.assertFalse(cache.exists())

    def test_external_published_data_is_required_before_final_validation(self):
        with tempfile.TemporaryDirectory() as directory, contextlib.redirect_stdout(io.StringIO()):
            root = Path(directory)
            work = root / "work"
            output = root / "output"
            calls = []
            stages = [
                c.Stage(
                    "publish-locales",
                    "fixture",
                    ("--output", str(output / "data/pack")),
                    (output / "data/pack",),
                ),
                c.Stage(
                    "validate-install",
                    "fixture",
                    ("--output", str(output / "receipt.json")),
                    (output / "receipt.json",),
                ),
            ]

            def convert(command, **kwargs):
                path = Path(command[command.index("--output") + 1])
                calls.append(path.name)
                if path.name == "receipt.json":
                    self.assertTrue((output / "data/pack").is_file())
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"valid")

            with patch.object(c.subprocess, "run", side_effect=convert):
                c.run_stages(stages, work, "source", compact=True)
                (output / "receipt.json").unlink()
                c.run_stages(stages, work, "source", compact=True)
            self.assertEqual(calls, ["pack", "receipt.json", "receipt.json"])

    def test_work_lock_rejects_overlap_and_releases_after_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            with WorkLock(directory):
                with self.assertRaisesRegex(ValueError, "Another conversion"):
                    with WorkLock(directory):
                        pass
            with WorkLock(directory):
                pass

    def test_parallel_cache_initializers_wait_then_release_the_shared_lock(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "count"

            def increment(_):
                with WorkLock(directory, blocking=True):
                    old = int(path.read_text()) if path.exists() else 0
                    time.sleep(0.005)
                    path.write_text(str(old + 1))

            with ThreadPoolExecutor(max_workers=4) as pool:
                list(pool.map(increment, range(12)))
            self.assertEqual(path.read_text(), "12")
            with WorkLock(directory):
                pass

    def test_immutable_alias_replacement_does_not_overwrite_previous_input(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source"
            target = root / "target"
            other = root / "other"
            source.write_bytes(b"first")
            other.write_bytes(b"next")
            c.link_or_copy(source, target)
            self.assertEqual(target.read_bytes(), b"first")
            c.link_or_copy(other, target)
            self.assertEqual(source.read_bytes(), b"first")
            self.assertEqual(target.read_bytes(), b"next")


if __name__ == "__main__":
    unittest.main()
