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

MAX_IMAGE_BYTES = 320 * 240 * 8 + 2 * 240
CONTENT_VERSION = 36
PLANES_CONTENT_VERSION = 43
GROUP_LIMIT = 512 * 1024
ENCODED_LIMIT = MAX_IMAGE_BYTES + MAX_IMAGE_BYTES // 255 + 32
GROUP = struct.Struct('<4s7I')
SLICE = struct.Struct('<II')


def _require(condition, message):
    if not condition:
        raise ValueError(message)


def group_layout(payload, version):
    """Return the bounded retained stream start and optional RGB slice table.

    Header inspection performs no decoding. Both the offline reader and the
    shared-art splitter use it, matching the runtime's requested-group checks.
    Legacy RGR1 reserved fields deliberately remain uninterpreted.
    """
    _require(GROUP.size < len(payload) <= ENCODED_LIMIT, 'Retained group outside encoded bound')
    tag, raw_size, encoded, prop, count, mode, table_bytes, reserved = GROUP.unpack_from(payload)
    _require(0 < raw_size <= GROUP_LIMIT and prop == 14, 'Invalid retained group size/property')
    if tag == b'RGR1':
        _require(encoded == len(payload) - GROUP.size, 'Invalid retained group stream length')
        return raw_size, GROUP.size, 0, ()
    planar = tag == b'RGR3'
    _require((planar and version >= PLANES_CONTENT_VERSION) or
             (tag == b'RGR2' and version >= CONTENT_VERSION), 'Unsupported retained group version')
    _require(mode in ((0, 1, 2) if planar else (1, 2)) and reserved == 0 and 0 < count <= raw_size // 2 and
             count <= (len(payload) - GROUP.size) // SLICE.size and table_bytes == count * SLICE.size,
             'Invalid retained color table header')
    start = GROUP.size + table_bytes
    _require(0 < encoded == len(payload) - start, 'Invalid retained color stream length')
    slices, end = [], 0
    for index in range(count):
        at, shape = SLICE.unpack_from(payload, GROUP.size + index * SLICE.size)
        length, fmt = shape & 0xffffff, shape >> 24
        _require(at == end and fmt <= 4 and 0 < length <= raw_size - end,
                 'Invalid retained color slice coverage')
        slices.append((at, shape))
        end += length
    _require(end == raw_size, 'Incomplete retained color slice coverage')
    return raw_size, start, mode + (8 if planar else 0), tuple(slices)


def transform_group(raw, slices, mode, inverse=False):
    """Transform each complete image slice, preserving its original offset."""
    _require(0 < len(raw) <= GROUP_LIMIT and slices, 'Invalid retained group payload')
    output, end = bytearray(raw), 0
    for at, shape in slices:
        length, fmt = shape & 0xffffff, shape >> 24
        _require(at == end and fmt <= 4 and 0 < length <= len(raw) - end,
                 'Invalid retained color slice coverage')
        data = raw[at:at + length]
        if mode in (8, 9, 10):
            from retained_planes import transform as planes
            if fmt == 4:
                data = planes(data, inverse, mode - 8)
            elif mode != 8:
                data = transform(data, fmt, mode - 8, inverse)
        else:
            data = transform(data, fmt, mode, inverse)
        output[at:at + length] = data
        end += length
    _require(end == len(raw), 'Incomplete retained color slice coverage')
    return bytes(output)


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
        # Dense RGB+alpha stores two contiguous planes. Sparse spans below
        # are the only retained representation with interleaved alpha.
        return [(0, 256, 2)] if prefix else [(0, len(raw) // stride, 2)]

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
    # Header-only split/pack inspection remains usable with the standard
    # library. NumPy is needed only by the offline pixel transform itself.
    import numpy as np
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
