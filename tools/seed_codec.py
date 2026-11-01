"""Portable import codes for the pinned game's ordinary ASCII custom seeds.

The code records the original canonical seed's xxHash64 value. It is only an
import code when the calculator's explicit Import seed code mode is selected;
ordinary text beginning with SEED64-, raw:, or old is never treated specially.
This module needs no game assets or third-party Python packages.
"""
from pathlib import Path

_MASK = (1 << 64) - 1
_P1, _P2, _P3, _P4, _P5 = (11400714785074694791, 14029467366897019727,
                           1609587929392839161, 9650029242287828579,
                           2870177450012600261)
_SPACE = b" \t\r\n\v\f"
_TRANSLATE = bytes.maketrans(b"OI", b"01")


def _rot(value, count):
    return ((value << count) | (value >> (64 - count))) & _MASK


def _round(value, lane):
    return (_rot((value + lane * _P2) & _MASK, 31) * _P1) & _MASK


class _Hash64:
    def __init__(self):
        self.length = 0
        self.words = [(_P1 + _P2) & _MASK, _P2, 0, (-_P1) & _MASK]
        self.tail = b""

    def copy(self):
        other = _Hash64()
        other.length, other.words, other.tail = self.length, self.words[:], self.tail
        return other

    def update(self, block):
        self.length += len(block)
        block = self.tail + block
        end = len(block) & ~31
        for at in range(0, end, 32):
            for i in range(4):
                self.words[i] = _round(self.words[i], int.from_bytes(block[at+i*8:at+i*8+8], "little"))
        self.tail = block[end:]

    def digest(self):
        if self.length >= 32:
            value = sum(_rot(word, shift) for word, shift in zip(self.words, (1, 7, 12, 18))) & _MASK
            for word in self.words:
                value = ((value ^ _round(0, word)) * _P1 + _P4) & _MASK
        else:
            value = _P5
        value = (value + self.length) & _MASK
        tail, at = self.tail, 0
        while at + 8 <= len(tail):
            value ^= _round(0, int.from_bytes(tail[at:at+8], "little"))
            value = (_rot(value, 27) * _P1 + _P4) & _MASK
            at += 8
        if at + 4 <= len(tail):
            value ^= (int.from_bytes(tail[at:at+4], "little") * _P1) & _MASK
            value = (_rot(value, 23) * _P2 + _P3) & _MASK
            at += 4
        for byte in tail[at:]:
            value = _rot(value ^ ((byte * _P5) & _MASK), 11) * _P1 & _MASK
        value ^= value >> 33
        value = value * _P2 & _MASK
        value ^= value >> 29
        value = value * _P3 & _MASK
        return value ^ (value >> 32)


def _encode(chunks):
    current = _Hash64()
    trimmed = current.copy()
    started = False
    for block in chunks:
        if not block.isascii():
            raise ValueError("Seed import currently supports ASCII text only; use ordinary seed entry for short Unicode seeds.")
        block = block.upper().translate(_TRANSLATE)
        if not started:
            block = block.lstrip(_SPACE)
            started = bool(block)
        if not block:
            continue
        content = block.rstrip(_SPACE)
        if content:
            current.update(content)
            trimmed = current.copy()
        current.update(block[len(content):])
    return f"SEED64-{trimmed.digest():016X}"


def encode_seed(text: str) -> str:
    """Encode complete ordinary ASCII seed text, with original casing/trim."""
    try:
        data = text.encode("ascii")
    except UnicodeEncodeError as error:
        raise ValueError("Seed import currently supports ASCII text only.") from error
    return _encode((data,))


def encode_seed_file(path) -> str:
    """Stream an ASCII/UTF-8 file without retaining long text or trailing space."""
    with Path(path).open("rb") as stream:
        return _encode(iter(lambda: stream.read(65536), b""))
