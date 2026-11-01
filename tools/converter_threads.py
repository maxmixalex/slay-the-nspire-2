"""Keep tiny raster solves serial inside each bounded converter process.

Set this before importing NumPy, including frozen multiprocessing bootstrap.
Parallelism belongs to complete independent conversion jobs: a 3x3 solve must
not start another machine-sized BLAS team in every worker.
"""
from __future__ import annotations
import os

VARIABLES = ('OPENBLAS_NUM_THREADS', 'OMP_NUM_THREADS', 'MKL_NUM_THREADS',
             'VECLIB_MAXIMUM_THREADS', 'NUMEXPR_NUM_THREADS', 'BLIS_NUM_THREADS')


def configure():
    for name in VARIABLES:
        os.environ[name] = '1'
    return {name: os.environ[name] for name in VARIABLES}


def worker_probe(directory=None, number=None):
    """Exercise the actual spawned numerical interpreter in bundle self-test."""
    import numpy as np
    matrix = np.array([[4., 1., 0.], [1., 4., 1.], [0., 1., 4.]])
    expected = np.arange(192, dtype=np.float64).reshape(3, 64)
    solved = np.linalg.solve(matrix, matrix @ expected)
    result = dict(environment={name: os.environ.get(name) for name in VARIABLES},
                  numerical_solve=bool(np.allclose(solved, expected, rtol=0, atol=1e-12)))
    if directory is not None:
        from pathlib import Path
        import time
        from converter_workspace import WorkLock
        directory = Path(directory)
        (directory / ('ready-' + str(number))).write_text(str(os.getpid()), encoding='ascii')
        deadline = time.monotonic() + 25
        while len(list(directory.glob('ready-*'))) != 2:
            if time.monotonic() > deadline:
                raise ValueError('The frozen pool did not start two independent workers')
            time.sleep(.01)
        with WorkLock(directory / 'shared', blocking=True):
            counter = directory / 'counter'
            value = int(counter.read_text()) if counter.exists() else 0
            time.sleep(.05)
            counter.write_text(str(value + 1), encoding='ascii')
        result['pid'] = os.getpid()
    return result
