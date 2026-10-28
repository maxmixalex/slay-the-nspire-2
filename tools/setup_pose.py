"""Bounded binary input helpers for the offline animation converter.

The legacy module name is kept for existing converter imports. Static-pose
rendering was superseded by animation.bake; no duplicate renderer is retained.
"""
from __future__ import annotations
import io
import math
import struct
from PIL import Image


class Reader:
    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0
        self.strings: list[str] = []

    def take(self, length: int) -> bytes:
        if length < 0 or self.pos < 0 or length > len(self.data) - self.pos:
            raise ValueError(f'Truncated binary input at {self.pos}, requested {length}')
        result = self.data[self.pos:self.pos + length]
        self.pos += length
        return result

    def byte(self) -> int:
        return self.take(1)[0]

    def vi(self) -> int:
        value = 0
        for shift in range(0, 35, 7):
            part = self.byte()
            if shift == 28 and part > 15:
                raise ValueError('Unsigned varint exceeds 32 bits')
            value |= (part & 127) << shift
            if part < 128:
                return value
        raise ValueError('Unterminated varint')

    def uint(self) -> int:
        return struct.unpack('>I', self.take(4))[0]

    def fl(self) -> float:
        value = struct.unpack('>f', self.take(4))[0]
        if not math.isfinite(value):
            raise ValueError('Non-finite animation coordinate')
        return value

    def floats(self, count: int) -> list[float]:
        if count < 0 or count > (len(self.data) - self.pos) // 4:
            raise ValueError('Float array exceeds input')
        return [self.fl() for _ in range(count)]

    def st(self) -> str:
        length = self.vi()
        return self.take(length - 1).decode('utf-8') if length > 1 else ''

    def ref(self) -> str:
        index = self.vi()
        if index > len(self.strings):
            raise ValueError('String reference outside table')
        return self.strings[index - 1] if index else ''

    def verts(self, weighted: bool):
        count = self.vi()
        if count > 20000:
            raise ValueError('Vertex count exceeds conversion limit')
        result = []
        for _ in range(count):
            if not weighted:
                result.append([(-1, self.fl(), self.fl(), 1.0)])
                continue
            influences = self.vi()
            if influences > 256:
                raise ValueError('Too many bone influences')
            result.append([(self.vi(), self.fl(), self.fl(), self.fl())
                           for _ in range(influences)])
        return result

    def sequence(self) -> list[int]:
        return [self.vi() for _ in range(4)]


def decode_texture(data: bytes) -> Image.Image:
    if data[:4] != b'GST2':
        raise ValueError('Expected a Godot texture header')
    offset = data.find(b'RIFF', 36, 100)
    if offset < 0 or data[offset + 8:offset + 12] != b'WEBP':
        raise ValueError('Expected an embedded WebP image')
    size = struct.unpack_from('<I', data, offset + 4)[0] + 8
    if size < 12 or size > len(data) - offset:
        raise ValueError('WebP payload exceeds texture bounds')
    with Image.open(io.BytesIO(data[offset:offset + size])) as image:
        return image.convert('RGBA')
