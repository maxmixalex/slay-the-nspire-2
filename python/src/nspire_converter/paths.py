"""Source and compiled-payload paths shared by converter stages."""

import sys
from pathlib import Path

PACKAGE_ROOT = Path(__file__).resolve().parent
COMPILED = "__compiled__" in globals()
ROOT = PACKAGE_ROOT.parents[2] if PACKAGE_ROOT.parent.name == "src" else PACKAGE_ROOT.parent
DATA_ROOT = PACKAGE_ROOT / "data"
HELPER_ROOT = ROOT / "converter_helpers"


def compiled() -> bool:
    return COMPILED or bool(getattr(sys, "frozen", False))
