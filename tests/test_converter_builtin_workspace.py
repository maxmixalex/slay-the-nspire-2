"""Actual filesystem/runner lifetimes for positional builtin scratch.

Lightweight deterministic workers replace private resource conversion; the
production recipe, workspace, receipts, verification and reclamation execute.
"""
import contextlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import converter as c
from converter_finalize import add_final_stages
from converter_workspace import SpaceMeter, Workspace


BUILTINS = (
    ('fresh-actor-dicing', '@fresh-actor-dicing'),
    ('codec45', '@fresh-codec45'),
    ('quality46', '@final-quality'),
    ('sparse46', '@final-sparse'),
    ('retuned46', '@final-retune'),
)


class BuiltinWorkspaceTests(unittest.TestCase):
    def recipe(self, root, reduced=False):
        recipe = c.Recipe(root / 'original', root / 'work', '/runtime',
                          'dotnet', '/noise', '-', languages=('eng', 'zhs'))
        add_final_stages(recipe, recipe.source_stages(), root / 'published',
                         ('eng', 'zhs'), reduced)
        return recipe

    def chain(self, root):
        work = root / 'work'
        source = root / 'original.nsp'
        source.write_bytes(b'original bytes')
        stages = []
        for name, module in BUILTINS:
            output = work / name / 'eng.nsp'
            scratch = work / 'cache' / name
            stages.append(c.Stage(name, module,
                (str(source), str(output), str(scratch), '1'),
                (output, output.with_suffix('.json'))))
            source = output
        output = root / 'published' / 'final.nsp'
        stages.append(c.Stage('publish-fixture', 'publish-fixture',
                              (str(source), str(output)), (output,)))
        return work, stages

    def write_fixture(self, command, **kwargs):
        at = command.index('--internal-stage')
        module, arguments = command[at + 1], command[at + 2:]
        source, target = map(Path, arguments[:2])
        data = source.read_bytes()
        if module.startswith('@'):
            scratch = Path(arguments[2])
            scratch.mkdir(parents=True, exist_ok=True)
            (scratch / 'pixels.bin').write_bytes(b'p' * 4096)
            (scratch / 'diagnostic.json').write_text(json.dumps({'module': module}))
            data += b'/' + module.encode()
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        target.with_suffix('.json').write_text(json.dumps({'sha256': c.digest(target)}))

    def test_production_full_and_reduced_recipe_register_only_scratch_writes(self):
        for reduced in (False, True):
            with self.subTest(reduced=reduced), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                recipe = self.recipe(root, reduced)
                workspace = Workspace(recipe.stages, recipe.work, 'source', c.digest, c.atomic_json)
                found = set()
                for index, stage in enumerate(recipe.stages):
                    if stage.module in dict(BUILTINS).values():
                        scope = 'cache/' + stage.name
                        self.assertEqual(Path(stage.arguments[2]), recipe.work / scope)
                        self.assertIn(scope, workspace.writes[index])
                        self.assertIn(scope, workspace.owned)
                        found.add(stage.name)
                self.assertEqual(found, {name for name, _ in BUILTINS[:3 if reduced else 5]})
                self.assertTrue(Workspace.KEEP <= workspace.owned)
                # This shared source cache is read much later by Neow's fire.
                last = next(i for i, stage in enumerate(recipe.stages) if stage.name == 'neow-fire46')
                self.assertEqual(workspace.last_use['cache/ancient-scenes45'], last)
                shared = recipe.work / 'cache/ancient-scenes45'
                kept = recipe.work / 'cache/paired44'
                for path in (shared, kept):
                    path.mkdir(parents=True)
                    (path / 'pixels.bin').write_bytes(b'reused')
                workspace.reclaim(last - 1)
                self.assertEqual((shared / 'pixels.bin').read_bytes(), b'reused')
                workspace.reclaim(last)
                self.assertFalse(shared.exists())
                workspace.reclaim(len(recipe.stages) - 1)
                self.assertEqual((kept / 'pixels.bin').read_bytes(), b'reused')

    def test_success_reclaims_each_builtin_before_next_stage_and_keeps_reports(self):
        output_log = io.StringIO()
        with tempfile.TemporaryDirectory() as directory, contextlib.redirect_stdout(output_log):
            root = Path(directory)
            work, stages = self.chain(root)
            calls = []
            def execute(command, **kwargs):
                if calls:
                    self.assertFalse((work / 'cache' / calls[-1]).exists())
                self.write_fixture(command, **kwargs)
                at = command.index('--internal-stage')
                calls.append(Path(command[at + 3]).parent.name)
            with patch.object(c.subprocess, 'run', side_effect=execute):
                self.assertTrue(c.run_stages(stages, work, 'source', compact=True))
            self.assertEqual(len(calls), 6)
            self.assertEqual(stages[-1].outputs[0].read_bytes(),
                b'original bytes' + b''.join(b'/' + module.encode() for _, module in BUILTINS))
            self.assertEqual((root / 'original.nsp').read_bytes(), b'original bytes')
            for name, module in BUILTINS:
                self.assertFalse((work / 'cache' / name).exists())
                report = work / 'receipts/details/cache' / name / 'diagnostic.json'
                self.assertEqual(json.loads(report.read_text())['module'], module)
            summary = json.loads((work / 'receipts/pipeline.json').read_text())
            self.assertTrue(summary['complete'])
            self.assertGreaterEqual(summary['reclaimed_bytes'], 5 * 4096)
            self.assertEqual(summary['work'], str(work.resolve()))
            label = 'estimated allocated bytes' if summary['allocation_bytes_estimated'] else 'allocated bytes'
            self.assertIn(f"Workspace: {work.resolve()}; retained {summary['retained_work_bytes']:,} logical bytes / "
                          f"{summary['retained_work_allocated_bytes']:,} {label}.", output_log.getvalue())

    def test_keep_then_boundary_only_resume_reclaims_without_rerunning_workers(self):
        with tempfile.TemporaryDirectory() as directory, contextlib.redirect_stdout(io.StringIO()):
            work, stages = self.chain(Path(directory))
            with patch.object(c.subprocess, 'run', side_effect=self.write_fixture) as execute:
                self.assertTrue(c.run_stages(stages, work, 'source', compact=True, keep_intermediates=True))
                self.assertEqual(execute.call_count, 6)
                for name, _ in BUILTINS:
                    self.assertTrue((work / 'cache' / name / 'pixels.bin').is_file())
                expected = stages[-1].outputs[0].read_bytes()
                self.assertTrue(c.run_stages(stages, work, 'source', compact=True))
                self.assertEqual(execute.call_count, 6)
            self.assertEqual(stages[-1].outputs[0].read_bytes(), expected)
            self.assertTrue(all(not (work / 'cache' / name).exists() for name, _ in BUILTINS))
            summary = json.loads((work / 'receipts/pipeline.json').read_text())
            self.assertEqual([row['action'] for row in summary['stages']], ['boundary'] * 6)

    def test_skipped_boundary_keeps_shared_scratch_until_needed_later_consumer(self):
        with tempfile.TemporaryDirectory() as directory, contextlib.redirect_stdout(io.StringIO()):
            root = Path(directory)
            work, stages = self.chain(root)
            producer = stages[0]
            scratch = Path(producer.arguments[2])
            output = root / 'published/final.nsp'
            consumer = c.Stage('consume', 'fixture-consumer',
                (str(producer.outputs[0]), str(output), str(scratch / 'pixels.bin')), (output,))
            calls = []
            def execute(command, **kwargs):
                at = command.index('--internal-stage')
                module = command[at + 1]
                calls.append(module)
                if module == 'fixture-consumer':
                    self.assertEqual(Path(command[at + 4]).read_bytes(), b'p' * 4096)
                self.write_fixture(command, **kwargs)
            with patch.object(c.subprocess, 'run', side_effect=execute):
                c.run_stages([producer, consumer], work, 'source', compact=True, keep_intermediates=True)
                expected = output.read_bytes()
                output.write_bytes(b'changed output')
                c.run_stages([producer, consumer], work, 'source', compact=True)
            self.assertEqual(calls, ['@fresh-actor-dicing', 'fixture-consumer', 'fixture-consumer'])
            self.assertEqual(output.read_bytes(), expected)
            self.assertFalse(scratch.exists())

    def test_failure_keeps_partial_scratch_and_required_input_then_recovers(self):
        with tempfile.TemporaryDirectory() as directory, contextlib.redirect_stdout(io.StringIO()):
            root = Path(directory)
            work, stages = self.chain(root)
            failed_scratch = Path(stages[1].arguments[2])
            fail = [True]
            def execute(command, **kwargs):
                at = command.index('--internal-stage')
                if command[at + 1] == '@fresh-codec45':
                    if fail[0]:
                        failed_scratch.mkdir(parents=True)
                        (failed_scratch / 'partial.bin').write_bytes(b'recover me')
                        raise subprocess.CalledProcessError(1, command)
                    self.assertEqual((failed_scratch / 'partial.bin').read_bytes(), b'recover me')
                self.write_fixture(command, **kwargs)
            with patch.object(c.subprocess, 'run', side_effect=execute):
                with self.assertRaises(subprocess.CalledProcessError):
                    c.run_stages(stages, work, 'source', compact=True)
                self.assertTrue(stages[0].outputs[0].is_file())
                self.assertEqual(json.loads((work / 'receipts/codec45.json').read_text())['status'], 'failed')
                self.assertIn('cache/codec45', json.loads((work / 'receipts/workspace.json').read_text())['owned_roots'])
                fail[0] = False
                self.assertTrue(c.run_stages(stages, work, 'source', compact=True))
            self.assertFalse(failed_scratch.exists())
            self.assertTrue(stages[-1].outputs[0].is_file())

    def test_preexisting_external_and_helper_inputs_are_not_owned(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            work = root / 'work'
            existing = work / 'cache/existing'
            outside = root / 'external'
            helper = work / 'cache/helper'
            for path in (existing, outside):
                path.mkdir(parents=True)
                (path / 'owner.bin').write_bytes(b'owner input')
            stages = [c.Stage(name, module, ('input', str(work / name / 'out'), str(path), '1'),
                              (work / name / 'out',)) for name, module, path in (
                ('existing', '@fresh-actor-dicing', existing),
                ('outside', '@final-sparse', outside),
                ('helper', '@mandatory-orphan-prune', helper),
            )]
            workspace = Workspace(stages, work, 'source', c.digest, c.atomic_json)
            self.assertNotIn('cache/existing', workspace.owned)
            self.assertNotIn('cache/helper', workspace.writes[2])
            helper.mkdir(parents=True)
            (helper / 'owner.bin').write_bytes(b'owner input')
            workspace.reclaim(len(stages) - 1)
            for path in (existing, outside, helper):
                self.assertEqual((path / 'owner.bin').read_bytes(), b'owner input')

    def test_owned_scratch_refuses_symlink_descendants(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            work, stages = self.chain(root)
            workspace = Workspace(stages, work, 'source', c.digest, c.atomic_json)
            scratch = Path(stages[0].arguments[2])
            scratch.mkdir(parents=True)
            outside = root / 'owner.bin'
            outside.write_bytes(b'keep')
            try:
                (scratch / 'linked.bin').symlink_to(outside)
            except OSError as exc:
                self.skipTest('Symlinks unavailable: ' + str(exc))
            with self.assertRaisesRegex(ValueError, 'refuses symbolic links'):
                workspace.reclaim(0)
            self.assertEqual(outside.read_bytes(), b'keep')
            self.assertTrue(scratch.is_dir())

    def test_space_meter_marks_missing_block_counts_as_estimated(self):
        for blocks in (None, 6):
            with self.subTest(st_blocks=blocks), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                payload = root / 'sample.bin'
                payload.write_bytes(b'x' * 513)
                original_lstat = Path.lstat
                class StatSample:
                    def __init__(self, source):
                        self.source = source
                    def __getattr__(self, name):
                        if name == 'st_blocks':
                            if blocks is None:
                                raise AttributeError(name)
                            return blocks
                        return getattr(self.source, name)
                def lstat(path, *args, **kwargs):
                    value = original_lstat(path, *args, **kwargs)
                    return StatSample(value) if path == payload else value
                with patch.object(Path, 'lstat', lstat):
                    meter = SpaceMeter(root)
                    meter.start()
                    report = meter.stop()
                self.assertEqual(report['retained_work_bytes'], 513)
                self.assertEqual(report['retained_work_allocated_bytes'], 1024 if blocks is None else 3072)
                self.assertEqual(report['allocation_bytes_estimated'], blocks is None)
                self.assertIn('st_blocks', report['allocation_measurement'])
                if blocks is None:
                    self.assertIn('allocation granularity not measured', report['allocation_measurement'])


if __name__ == '__main__':
    unittest.main()
