"""The actual executable bootstrap limits nested numerical worker teams."""

import json
import os
import subprocess
import sys
import unittest
from unittest.mock import patch

from nspire_converter import converter_threads as threads
from nspire_converter.paths import ROOT


class ConverterThreadsTest(unittest.TestCase):
    def test_machine_wide_numeric_defaults_are_bounded_without_changing_path(self):
        with patch.dict(os.environ, {name: "32" for name in threads.VARIABLES}):
            before = os.environ.get("PATH")
            self.assertEqual(threads.configure(), {name: "1" for name in threads.VARIABLES})
            self.assertEqual(os.environ.get("PATH"), before)

    def test_bootstrap_precedes_numpy_in_both_actual_spawned_interpreters(self):
        program = """import runpy,json
import nspire_converter.__main__
from concurrent.futures import ProcessPoolExecutor
import multiprocessing,tempfile
from nspire_converter.converter_threads import worker_probe
with ProcessPoolExecutor(max_workers=2,mp_context=multiprocessing.get_context('spawn')) as pool:
    with tempfile.TemporaryDirectory() as directory:
        jobs=[pool.submit(worker_probe,directory,number) for number in range(2)]
        results=[job.result(timeout=30) for job in jobs]
        assert len({value.pop('pid') for value in results})==2
        print(json.dumps(results))
"""
        environment = dict(os.environ, **{name: "9" for name in threads.VARIABLES})
        result = subprocess.run(
            [sys.executable, "-c", program],
            cwd=ROOT,
            env=environment,
            text=True,
            capture_output=True,
            timeout=60,
            check=True,
        )
        self.assertEqual(
            json.loads(result.stdout),
            [dict(environment={name: "1" for name in threads.VARIABLES}, numerical_solve=True)] * 2,
        )


if __name__ == "__main__":
    unittest.main()
