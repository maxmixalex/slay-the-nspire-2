"""Carry the actual installed game's version into either resource format."""

from __future__ import annotations

import json
import re
from pathlib import Path

FILE_NAME = "nSpire2.version.tns"
_VERSION = re.compile(r"[0-9]{1,6}(?:\.[0-9]{1,6}){1,3}")


def normalize_version(value: object) -> str:
    if not isinstance(value, str):
        raise ValueError("release_info.json has no valid version string")
    version = value.removeprefix("v")
    if _VERSION.fullmatch(version) is None:
        raise ValueError("release_info.json version must contain numeric dot-separated components")
    return version


def installed_version(game: Path) -> str:
    """Read an explicit owner-supplied installation; never infer a release label."""
    info = json.loads((game / "release_info.json").read_text(encoding="utf-8-sig"))
    if not isinstance(info, dict):
        raise ValueError("release_info.json must be an object")
    return normalize_version(info.get("version"))


def encode_version(version: str) -> bytes:
    return f"NSVR1\n{normalize_version(version)}\n".encode("ascii")


def decode_version(data: bytes) -> str:
    try:
        text = data.decode("ascii")
    except UnicodeDecodeError as error:
        raise ValueError("Source version metadata is not ASCII") from error
    if not text.startswith("NSVR1\n") or not text.endswith("\n"):
        raise ValueError("Invalid source version metadata")
    value = text[6:-1]
    if _VERSION.fullmatch(value) is None:
        raise ValueError("Invalid source version metadata")
    return value


def write_version(directory: Path, version: str) -> Path:
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / FILE_NAME
    temporary = path.with_suffix(".tmp")
    temporary.write_bytes(encode_version(version))
    temporary.replace(path)
    return path
