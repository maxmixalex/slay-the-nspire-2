"""Strict, read-only Godot 4 PCK v3 reader for STS2 v0.111.0."""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
import hashlib
import struct

@dataclass(frozen=True)
class Entry:
    name: str
    offset: int
    size: int
    md5: bytes

class Pack:
    def __init__(self, path: str | Path):
        self.path = Path(path)
        self.file = self.path.open('rb')
        self.length = self.path.stat().st_size
        header = self.file.read(40)
        if len(header) != 40:
            raise ValueError('Truncated PCK header')
        magic, version, major, minor, patch, flags, base, directory = struct.unpack('<6I2Q', header)
        if magic != 0x43504447 or version != 3 or flags & 1:
            raise ValueError('Expected an unencrypted Godot PCK v3')
        if directory < 112 or directory >= self.length:
            raise ValueError('Invalid PCK directory offset')
        self.file.seek(directory)
        count = self._uint()
        if count > 100000:
            raise ValueError('Unreasonable PCK directory size')
        self.entries: dict[str, Entry] = {}
        for _ in range(count):
            n = self._uint()
            if not 0 < n <= 4096:
                raise ValueError('Invalid PCK path length')
            name = self.file.read(n).rstrip(b'\0').decode('utf-8')
            off, size = struct.unpack('<2Q', self.file.read(16))
            digest = self.file.read(16)
            entry_flags = self._uint()
            if entry_flags & 1:
                raise ValueError('Encrypted PCK entry is unsupported')
            off += base if flags & 2 else 0
            if off + size > self.length or name in self.entries:
                raise ValueError('Invalid or duplicate PCK entry')
            self.entries[name] = Entry(name, off, size, digest)

    def _uint(self) -> int:
        data = self.file.read(4)
        if len(data) != 4:
            raise ValueError('Truncated PCK directory')
        return struct.unpack('<I', data)[0]

    def read(self, name: str) -> bytes:
        entry = self.entries[name.removeprefix('res://')]
        self.file.seek(entry.offset)
        data = self.file.read(entry.size)
        if len(data) != entry.size or hashlib.md5(data).digest() != entry.md5:
            raise ValueError(f'PCK integrity check failed: {entry.name}')
        return data

    def imported(self, filename: str, extension: str) -> str:
        matches = [n for n in self.entries if n.startswith('.godot/imported/' + filename + '-') and n.endswith(extension)]
        if len(matches) != 1:
            raise ValueError(f'Expected one imported {filename}, found {len(matches)}')
        return matches[0]

    def close(self) -> None:
        self.file.close()
