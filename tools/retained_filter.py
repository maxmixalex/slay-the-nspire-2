"""Reversible RGB565 decorrelation for retained image payloads.

The byte layout never changes. Mode 1 stores green and modulo-32 red/blue
differences; mode 2 zigzags the signed differences in [-16, 15]. Pixel alpha,
palette indices, sparse row counts and span geometry stay byte-identical.
This module neither compresses data nor changes a pack/resource contract.
Callers must compare complete encoded costs and retain the old stream when a
candidate is not strictly smaller.
"""
from __future__ import annotations

import struct

import numpy as np


MAX_IMAGE_BYTES = 320 * 240 * 8 + 2 * 240


def _require(condition, message):
    if not condition:
        raise ValueError(message)


def _color_ranges(raw, fmt, width, height):
    """Validate the complete payload before returning color byte ranges."""
    _require(isinstance(fmt, int) and fmt in (0, 1, 2, 3, 4), 'Unsupported retained image format')
    _require(0 < len(raw) <= MAX_IMAGE_BYTES, 'Retained image outside byte limit')
    _require((width is None) == (height is None), 'Both image dimensions are required')
    if width is not None:
        _require(isinstance(width, int) and isinstance(height, int) and
                 0 < width <= 320 and 0 < height <= 240, 'Invalid retained image dimensions')
    if fmt < 4:
        stride = (2, 3, 1, 2)[fmt]
        prefix = 512 if fmt >= 2 else 0
        _require(len(raw) > prefix and (len(raw) - prefix) % stride == 0,
                 'Invalid retained image byte count')
        if width is not None:
            _require(len(raw) == prefix + width * height * stride,
                     'Retained image dimensions disagree with its payload')
        return [(0, 256, 2)] if prefix else [(0, len(raw) // stride, stride)]

    at, rows, ranges = 0, 0, []
    maximum_width = width if width is not None else 320
    maximum_height = height if height is not None else 240
    while at < len(raw) and rows < maximum_height:
        _require(at + 2 <= len(raw), 'Truncated retained sparse row')
        spans, = struct.unpack_from('<H', raw, at)
        _require(spans <= maximum_width, 'Excessive retained sparse span count')
        at += 2
        previous = 0
        for _ in range(spans):
            _require(at + 5 <= len(raw), 'Truncated retained sparse span')
            x, count, alpha = struct.unpack_from('<HHB', raw, at)
            _require(alpha in (0, 1) and count and previous <= x and
                     x + count <= maximum_width, 'Invalid retained sparse geometry')
            at += 5
            stride = 2 + alpha
            _require(at + count * stride <= len(raw), 'Truncated retained sparse colors')
            ranges.append((at, count, stride))
            at += count * stride
            previous = x + count
        rows += 1
    _require(at == len(raw), 'Trailing retained sparse bytes')
    if height is not None:
        _require(rows == height, 'Missing retained sparse rows')
    return ranges


def transform(raw, fmt, mode, inverse=False, *, width=None, height=None):
    """Return an exact-length transformed copy, leaving ``raw`` untouched.

    Pass width and height for strict image geometry, including detection of
    additional empty sparse rows. Omitting them retains the measured probe's
    payload-only API; sparse coordinates still obey the 320x240 image bounds.
    Both forward and inverse paths validate identical unchanged geometry.
    """
    _require(isinstance(mode, int) and mode in (1, 2), 'Unsupported retained color transform')
    ranges = _color_ranges(raw, fmt, width, height)
    out = bytearray(raw)
    for at, count, stride in ranges:
        low = np.ndarray((count,), dtype='u1', buffer=out, offset=at, strides=(stride,))
        high = np.ndarray((count,), dtype='u1', buffer=out, offset=at + 1, strides=(stride,))
        value = low.astype(np.int32) | (high.astype(np.int32) << 8)
        if inverse:
            green, red, blue = value & 63, (value >> 6) & 31, value >> 11
            if mode == 2:
                red = (red >> 1) ^ (-(red & 1))
                blue = (blue >> 1) ^ (-(blue & 1))
            value = (((red + (green >> 1)) & 31) << 11) | (green << 5) | \
                    ((blue + (green >> 1)) & 31)
        else:
            green = (value >> 5) & 63
            red, blue = ((value >> 11) - (green >> 1)) & 31, ((value & 31) - (green >> 1)) & 31
            if mode == 2:
                red = np.where(red < 16, red * 2, (32 - red) * 2 - 1)
                blue = np.where(blue < 16, blue * 2, (32 - blue) * 2 - 1)
            value = green | (red << 6) | (blue << 11)
        low[:] = value & 255
        high[:] = (value >> 8) & 255
    return bytes(out)
