"""NSP2/46 exact sparse geometry reconstructed from original alpha bytes.

The marker replaces only the existing geometry area. Record offsets, lengths,
tile IDs, decoded sparse bytes and cache-page costs do not change. The bitmap
retains precisely the tiles the explicit geometry used, including empty gaps
which must not cause extra cold pixel-group reads.
"""
from __future__ import annotations

import struct
import numpy as np

VERSION = 46
MARKER = 0xfffd
SIDE = 16


def require(condition, message):
    if not condition:
        raise ValueError(message)


def marked(geometry):
    return len(geometry) >= 2 and struct.unpack_from('<H', geometry)[0] == MARKER


def cells(width, height):
    require(0 < width <= 320 and 0 < height <= 240, 'Invalid implicit image bounds')
    return ((width + 15) // 16) * ((height + 15) // 16)


def active_mask(geometry, width, height):
    """Read one canonical LSB-first mask; reject truncated or nonzero padding."""
    count = cells(width, height)
    size = (count + 7) // 8
    require(marked(geometry) and len(geometry) >= 2 + size,
            'Truncated implicit geometry marker or bitmap')
    bits = geometry[2:2 + size]
    require(not any(geometry[2 + size:]), 'Nonzero implicit geometry padding')
    require(count % 8 == 0 or not (bits[-1] >> (count % 8)),
            'Implicit bitmap has bits outside the image')
    return np.unpackbits(np.frombuffer(bits, dtype='u1'), bitorder='little')[:count].astype(np.bool_)


def encode_mask(active, width, height, geometry_size):
    count = cells(width, height)
    active = np.asarray(active, dtype=np.bool_)
    require(active.ndim == 1 and len(active) == count, 'Implicit active bitmap shape differs')
    bits = np.packbits(active, bitorder='little').tobytes()
    require(2 + len(bits) <= geometry_size, 'Geometry area cannot hold the implicit bitmap')
    return struct.pack('<H', MARKER) + bits + bytes(geometry_size - 2 - len(bits))


def alpha_geometry(alpha):
    """Exact minimum-cost segmentation, including the historical DP ties.

    Each nonzero-alpha component is independent. An opaque island pays five
    bytes per new span, and saves one byte per opaque pixel. Leading islands
    therefore split above five pixels, interior islands above ten; the DP's
    final opaque-first tie splits a trailing island at exactly five as well.
    The independent tests compare complete output with sparse_image.encode's
    original two-state, per-pixel dynamic program.
    """
    alpha = np.asarray(alpha)
    require(alpha.ndim == 2 and alpha.dtype == np.uint8, 'Expected a byte alpha plane')
    height, width = alpha.shape
    cells(width, height)
    output = bytearray()
    for row in alpha:
        # np transitions avoid a Python iteration for every opaque pixel.
        kinds = np.where(row == 0, 0, np.where(row == 255, 2, 1))
        changes = np.flatnonzero(kinds[1:] != kinds[:-1]) + 1
        edges = np.concatenate(([0], changes, [width]))
        runs = [(int(a), int(b), int(kinds[a])) for a, b in zip(edges[:-1], edges[1:])]
        spans = []
        i = 0
        while i < len(runs):
            if not runs[i][2]:
                i += 1
                continue
            first = i
            while i < len(runs) and runs[i][2]:
                i += 1
            last = i - 1
            for j in range(first, i):
                start, end, kind = runs[j]
                opaque = kind == 2 and (first == last or
                    (j == first and end - start > 5) or
                    (j == last and end - start >= 5) or
                    (first < j < last and end - start > 10))
                mode = 0 if opaque else 1
                if spans and spans[-1][0] + spans[-1][1] == start and spans[-1][2] == mode:
                    x, length, _ = spans[-1]
                    spans[-1] = x, length + end - start, mode
                else:
                    spans.append((start, end - start, mode))
        output.extend(struct.pack('<H', len(spans)))
        for span in spans:
            output.extend(struct.pack('<HHB', *span))
    return bytes(output)


def sparse_bytes(geometry, colors, alpha, raw_size):
    """Serialize exact existing sparse bytes from explicit geometry and planes."""
    height, width = alpha.shape
    require(colors.shape == alpha.shape and colors.dtype == np.dtype('<u2') and
            alpha.dtype == np.uint8, 'Sparse pixel planes differ')
    result = bytearray()
    at = 0
    for y in range(height):
        require(at + 2 <= len(geometry), 'Truncated geometry row')
        count, = struct.unpack_from('<H', geometry, at)
        require(count <= width, 'Excessive geometry spans')
        result.extend(geometry[at:at + 2]); at += 2
        previous = 0
        for _ in range(count):
            require(at + 5 <= len(geometry), 'Truncated geometry span')
            x, size, mode = struct.unpack_from('<HHB', geometry, at)
            require(mode in (0, 1) and size > 0 and previous <= x and x + size <= width,
                    'Invalid geometry span')
            previous = x + size
            result.extend(geometry[at:at + 5]); at += 5
            if mode:
                span = np.empty((size, 3), dtype='u1')
                span[:, 0] = colors[y, x:previous] & 255
                span[:, 1] = colors[y, x:previous] >> 8
                span[:, 2] = alpha[y, x:previous]
                result.extend(span.tobytes())
            else:
                require(np.all(alpha[y, x:previous] == 255), 'Opaque span has non-opaque tile alpha')
                result.extend(colors[y, x:previous].tobytes())
            require(len(result) <= raw_size, 'Geometry output exceeds declared size')
    require(at == len(geometry) and len(result) == raw_size, 'Geometry output byte count mismatch')
    return bytes(result)
