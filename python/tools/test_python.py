#!/usr/bin/env python3
"""Run all public Python regression suites without original game resources."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    environment = os.environ.copy()
    environment.setdefault("CXX", "c++")
    environment["PYTHONPATH"] = (
        os.pathsep.join(
            map(str, (ROOT / "python/src", ROOT / "python/tools", ROOT / "python/tests"))
        )
        + os.pathsep
        + environment.get("PYTHONPATH", "")
    )
    for command in (
        ["-m", "unittest", "discover", "-s", "python/tests", "-p", "test_*.py"],
        [
            "-m",
            "unittest",
            "animation_regression",
            "logged_command_regression",
            "character_pack_regression",
            "all_character_pack_regression",
            "event_pack_regression",
            "world_pack_regression",
        ],
    ):
        result = subprocess.run([sys.executable, *command], cwd=ROOT, env=environment)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
