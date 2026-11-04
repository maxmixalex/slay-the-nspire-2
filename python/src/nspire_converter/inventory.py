"""Stable input inventory for resumable conversions."""

from __future__ import annotations

import hashlib
import json
from functools import cache
from pathlib import Path

from nspire_converter.paths import ROOT


@cache
def inventory(root: Path = ROOT) -> dict[str, str]:
    manifest = root / "converter_helpers/source-inventory.json"
    if manifest.is_file():
        bundled = json.loads(manifest.read_text(encoding="utf-8"))
        if not isinstance(bundled, dict) or not all(
            isinstance(k, str) and isinstance(v, str) for k, v in bundled.items()
        ):
            raise ValueError("Invalid bundled source inventory")
        return {str(k): str(v) for k, v in bundled.items()}
    result: dict[str, str] = {}
    for directory in ("python/src/nspire_converter", "python/dotnet", "cpp/src", "cpp/include"):
        for path in (root / directory).rglob("*"):
            if path.is_file() and "__pycache__" not in path.parts:
                result[path.relative_to(root).as_posix()] = hashlib.sha256(
                    path.read_bytes()
                ).hexdigest()
    lock = root / "poetry.lock"
    if lock.is_file():
        result["poetry.lock"] = hashlib.sha256(lock.read_bytes()).hexdigest()
    return result


def source_identity(path: Path) -> bytes:
    """A cache input for source modules, also available after Nuitka compilation."""
    names = inventory()
    candidates = [key for key in names if key.rsplit("/", 1)[-1] == path.name]
    if len(candidates) != 1:
        raise ValueError(f"Source identity is missing or ambiguous: {path.name}")
    return bytes.fromhex(names[candidates[0]])
