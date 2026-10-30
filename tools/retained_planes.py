"""Byte-exact sparse-image field separation for NSP2/43 RGR3 groups.

Geometry comes first, followed by contiguous RGB565 words and mixed-span alpha.
Every original byte survives, including hidden RGB and empty scanlines. Dense
images retain their existing representation. A geometry-only scan determines
all three ranges without a new per-image header or a larger decoded group.
"""
from __future__ import annotations

import struct


def _require(ok, message):
    if not ok:
        raise ValueError(message)


def transform(raw, inverse=False, color_mode=0):
    _require(0 < len(raw) <= 512 * 1024, 'Sparse planes exceed the group bound')
    _require(color_mode in (0, 1, 2), 'Unsupported sparse plane color mode')
    source = memoryview(raw)
    cursor = total = pixels = alphas = rows = 0
    geometry = bytearray()
    spans = []
    while total < len(raw):
        _require(rows < 240 and cursor + 2 <= len(raw), 'Truncated sparse planes row')
        count, = struct.unpack_from('<H', source, cursor)
        _require(count <= 320, 'Excessive sparse planes spans')
        geometry.extend(source[cursor:cursor + 2]); cursor += 2; total += 2; rows += 1
        previous = 0
        for _ in range(count):
            _require(cursor + 5 <= len(raw), 'Truncated sparse planes span')
            x, size, alpha = struct.unpack_from('<HHB', source, cursor)
            _require(size and alpha <= 1 and previous <= x <= 320 and size <= 320 - x,
                     'Invalid sparse planes geometry')
            geometry.extend(source[cursor:cursor + 5]); cursor += 5
            length = size * (2 + alpha); total += 5 + length
            _require(total <= len(raw), 'Sparse planes pixels exceed the slice')
            spans.append((cursor, size, alpha))
            pixels += size; alphas += size * alpha; previous = x + size
            if not inverse:
                _require(cursor + length <= len(raw), 'Truncated sparse planes pixels')
                cursor += length
    _require(total == len(raw) and len(geometry) + pixels * 2 + alphas == len(raw),
             'Sparse planes coverage differs')
    if inverse:
        _require(cursor == len(geometry), 'Sparse planes geometry extent differs')
        # Field separation makes all RGB565 words contiguous. Applying the
        # same per-word inverse once avoids a NumPy setup per sparse span.
        if color_mode and pixels:
            from retained_filter import transform as color_transform
            restored = bytearray(raw)
            restored[cursor:cursor + pixels * 2] = color_transform(
                raw[cursor:cursor + pixels * 2], 0, color_mode, True)
            source = memoryview(restored)
        output = bytearray()
        geom_at = 0; color_at = cursor; alpha_at = cursor + pixels * 2
        for _ in range(rows):
            count, = struct.unpack_from('<H', source, geom_at)
            output.extend(source[geom_at:geom_at + 2]); geom_at += 2
            for _ in range(count):
                _x, size, alpha = struct.unpack_from('<HHB', source, geom_at)
                output.extend(source[geom_at:geom_at + 5]); geom_at += 5
                if alpha:
                    # Slice assignment performs the exact interleave in C;
                    # no pixel conversion or host-only NumPy dependency.
                    span = bytearray(size * 3)
                    span[0::3] = source[color_at:color_at + size * 2:2]
                    span[1::3] = source[color_at + 1:color_at + size * 2:2]
                    span[2::3] = source[alpha_at:alpha_at + size]
                    output.extend(span); color_at += size * 2; alpha_at += size
                else:
                    output.extend(source[color_at:color_at + size * 2]); color_at += size * 2
        _require(geom_at == cursor and color_at == cursor + pixels * 2 and
                 alpha_at == len(raw) and len(output) == len(raw), 'Sparse planes restore extent differs')
        return bytes(output)
    colors, opacity = bytearray(), bytearray()
    for at, size, alpha in spans:
        if alpha:
            span = bytearray(size * 2)
            span[0::2] = source[at:at + size * 3:3]
            span[1::2] = source[at + 1:at + size * 3:3]
            colors.extend(span); opacity.extend(bytes(source[at + 2:at + size * 3:3]))
        else:
            colors.extend(source[at:at + size * 2])
    if color_mode and pixels:
        from retained_filter import transform as color_transform
        colors = color_transform(bytes(colors), 0, color_mode)
    return bytes(geometry + colors + opacity)
