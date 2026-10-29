#!/usr/bin/env python3
"""Run unchanged CTest programs through their declared ELF interpreter.

Some workspace transports drop execute bits from generated files after a
successful build. The system ELF interpreter remains executable. Prefixing
the generated test commands avoids that metadata race without changing the
programs, arguments, CTest properties or production files. The generated
CTest file is restored even when a test fails.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import subprocess


def interpreter(path: Path) -> str | None:
    if not path.is_file():
        return None
    with path.open("rb") as stream:
        head = stream.read(64)
        if len(head) < 64 or head[:4] != b"\x7fELF" or head[5] != 1:
            return None
        if head[4] == 2:
            offset = struct.unpack_from("<Q", head, 32)[0]
            size, count = struct.unpack_from("<HH", head, 54)
            fields, offset_index, length_index = "<IIQQQQQQ", 2, 5
        elif head[4] == 1:
            offset = struct.unpack_from("<I", head, 28)[0]
            size, count = struct.unpack_from("<HH", head, 42)
            fields, offset_index, length_index = "<8I", 1, 4
        else:
            return None
        for index in range(count):
            stream.seek(offset + index * size)
            row = struct.unpack(fields, stream.read(struct.calcsize(fields)))
            if row[0] == 3:  # PT_INTERP
                stream.seek(row[offset_index])
                value = stream.read(row[length_index]).rstrip(b"\0").decode()
                if not Path(value).is_absolute() or not Path(value).is_file():
                    raise ValueError(f"Unavailable declared ELF interpreter: {value}")
                return value
    return None


def quoted(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--parallel", type=int, default=3)
    args = parser.parse_args()
    if args.parallel < 1:
        parser.error("--parallel must be positive")
    build = args.build.resolve()
    listing = json.loads(subprocess.check_output(
        ["ctest", "--test-dir", str(build), "--show-only=json-v1"], text=True))
    replacements = {}
    for test in listing["tests"]:
        command = test.get("command", [])
        if command and (loader := interpreter(Path(command[0]))):
            replacements[quoted(command[0])] = quoted(loader) + " " + quoted(command[0])
    test_file = build / "CTestTestfile.cmake"
    original = test_file.read_bytes()
    changed = []
    wrapped = 0
    seen = set()
    expected_wrapped = sum(
        bool(test.get("command")) and quoted(test["command"][0]) in replacements
        for test in listing["tests"])
    for line in original.decode().splitlines(keepends=True):
        if line.startswith("add_test("):
            for old, new in replacements.items():
                if old in line:
                    line = line.replace(old, new, 1)
                    wrapped += 1
                    seen.add(old)
                    break
        changed.append(line)
    if seen != set(replacements) or wrapped != expected_wrapped:
        raise ValueError("CTest commands were not all found in the generated test file")
    print(f"Running {len(listing['tests'])} CTest tests; {wrapped} wrapped commands; "
          f"{len(replacements)} distinct declared ELF launchers", flush=True)
    try:
        test_file.write_text("".join(changed))
        return subprocess.run(["ctest", "--test-dir", str(build), "--parallel",
                               str(args.parallel), "--output-on-failure"]).returncode
    finally:
        test_file.write_bytes(original)


if __name__ == "__main__":
    raise SystemExit(main())
