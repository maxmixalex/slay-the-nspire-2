#!/usr/bin/env python3
"""Check journal state transitions without a compiler or private assets."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

RUNNER = Path(__file__).resolve().parents[1] / 'tools' / 'run_logged.py'
CHECKPOINT = '0' * 40


class JournalTests(unittest.TestCase):
    def invoke(self, child: str, timeout: float = 10):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'src').mkdir()
            (root / 'src' / 'sample.cpp').write_text('int main() { return 0; }\n')
            (root / 'VERSION').write_text('test\n')
            cmd = [sys.executable, str(RUNNER), '--root', tmp, '--label', 'test',
                   '--checkpoint', CHECKPOINT, '--timeout', str(timeout), '--', sys.executable, '-c', child]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
            journals = list((root / 'build' / 'journal').glob('*.json'))
            self.assertEqual(len(journals), 1, result.stderr)
            report = json.loads(journals[0].read_text())
            log = (journals[0].parent / report['log']).read_text()
            return result.returncode, report, log

    def test_success_sees_running_journal_before_exit(self):
        code, report, log = self.invoke("import pathlib,json; p=next(pathlib.Path('build/journal').glob('*.json')); assert json.loads(p.read_text())['status']=='running'; print('ok')")
        self.assertEqual(code, 0)
        self.assertEqual(report['status'], 'passed')
        self.assertFalse(report['source_changed'])
        self.assertEqual(log.strip(), 'ok')
        self.assertIn('src/sample.cpp', report['source_files'])

    def test_failure_preserves_exit_code(self):
        code, report, _ = self.invoke('raise SystemExit(7)')
        self.assertEqual(code, 7)
        self.assertEqual(report['status'], 'failed')
        self.assertEqual(report['returncode'], 7)

    def test_timeout_is_not_success(self):
        code, report, _ = self.invoke('import time; time.sleep(5)', timeout=0.15)
        self.assertEqual(code, 124)
        self.assertEqual(report['status'], 'timed_out')

    def test_source_change_invalidates_success(self):
        code, report, _ = self.invoke("from pathlib import Path; Path('src/sample.cpp').write_text('changed')")
        self.assertEqual(code, 1)
        self.assertEqual(report['status'], 'source_changed')
        self.assertTrue(report['source_changed'])


if __name__ == '__main__':
    unittest.main()
