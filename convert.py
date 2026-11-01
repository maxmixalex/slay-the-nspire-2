#!/usr/bin/env python3
"""Run the original-install converter (also the frozen executable entry point)."""
from pathlib import Path
import multiprocessing
import sys

# Frozen multiprocessing workers unpickle recipe functions during
# freeze_support(), before the ordinary CLI entry runs.
sys.path.insert(0, str(Path(__file__).resolve().parent/'tools'))
if __name__ == '__main__':
    multiprocessing.freeze_support()
    from converter import main
    raise SystemExit(main())
