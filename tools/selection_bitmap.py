"""Exact opaque selection coverage inside an existing NSP2/46 geometry slot.

The fixed-size bitmaps never change tile IDs, record offsets, decoded image
costs or index working sets. A short original geometry stays explicit. The
inverse must reproduce every original five-byte span, not merely its pixels.
"""
from __future__ import annotations

import struct

SENTINEL = 0xFFFC
WIDTH, HEIGHT = 320, 240
TILES = (WIDTH // 16) * (HEIGHT // 16)
ACTIVE_BYTES = (TILES + 7) // 8
ROW_BYTES = WIDTH // 8
COVERAGE_BYTES = ROW_BYTES * HEIGHT
MINIMUM_BYTES = 2 + ACTIVE_BYTES + COVERAGE_BYTES


def require(ok, message):
    if not ok:
        raise ValueError(message)


def marked(geometry):
    return len(geometry) >= 2 and geometry[:2] == b'\xfc\xff'


def _shape(width, height):
    require((width, height) == (WIDTH, HEIGHT),
            'Selection bitmap requires a complete 320x240 selection frame')


def _decode(width, height, geometry):
    _shape(width, height)
    require(marked(geometry) and len(geometry) >= MINIMUM_BYTES,
            'Truncated selection bitmap geometry')
    active = bytes(geometry[2:2 + ACTIVE_BYTES])
    require(not (active[-1] & 0xf0), 'Selection active mask has unused bits')
    require(not any(geometry[MINIMUM_BYTES:]), 'Selection bitmap padding is nonzero')
    output, used = bytearray(), 0
    for y in range(HEIGHT):
        at = 2 + ACTIVE_BYTES + y * ROW_BYTES
        bits = int.from_bytes(geometry[at:at + ROW_BYTES], 'little')
        for x in range(WIDTH // 16):
            if (bits >> (x * 16)) & 65535:
                used |= 1 << ((y // 16) * (WIDTH // 16) + x)
        row = bytearray(); count = 0
        while bits:
            first = (bits & -bits).bit_length() - 1
            shifted = bits >> first
            length = (shifted ^ (shifted + 1)).bit_length() - 1
            require(length > 0 and first + length <= WIDTH, 'Invalid selection copy span')
            row.extend(struct.pack('<HHB', first, length, 0)); count += 1
            bits &= ~(((1 << length) - 1) << first)
        output.extend(struct.pack('<H', count)); output.extend(row)
    require(used.to_bytes(ACTIVE_BYTES, 'little') == active,
            'Selection active mask differs from its copy coverage')
    require(len(output) == len(geometry), 'Selection bitmap changes original geometry size')
    return bytes(output), active


def active_mask(width, height, geometry):
    """Return the checked 300-tile, least-significant-bit-first active mask."""
    return _decode(width, height, geometry)[1]


def restore_geometry(width, height, geometry):
    """Recover the exact original row counts and opaque copy spans."""
    return _decode(width, height, geometry)[0]


def encode_geometry(width, height, geometry):
    """Return an exact same-length bitmap, or None for an unsuitable record."""
    _shape(width, height)
    if marked(geometry):
        restore_geometry(width, height, geometry)
        return None
    if len(geometry) < MINIMUM_BYTES:
        return None
    output, cursor, used = bytearray(), 0, 0
    for y in range(HEIGHT):
        require(cursor + 2 <= len(geometry), 'Truncated selection geometry row')
        count, = struct.unpack_from('<H', geometry, cursor); cursor += 2
        require(count <= WIDTH, 'Excessive selection span count')
        bits, previous = 0, 0
        for _ in range(count):
            require(cursor + 5 <= len(geometry), 'Truncated selection copy span')
            x, length, mode = struct.unpack_from('<HHB', geometry, cursor); cursor += 5
            require(mode in (0, 1) and length > 0 and previous <= x and
                    x + length <= WIDTH, 'Invalid selection copy geometry')
            if mode != 0:
                return None
            previous = x + length
            bits |= ((1 << length) - 1) << x
        output.extend(bits.to_bytes(ROW_BYTES, 'little'))
        for x in range(WIDTH // 16):
            if (bits >> (x * 16)) & 65535:
                used |= 1 << ((y // 16) * (WIDTH // 16) + x)
    require(cursor == len(geometry), 'Trailing selection geometry bytes')
    encoded = (struct.pack('<H', SENTINEL) + used.to_bytes(ACTIVE_BYTES, 'little') +
               output + bytes(len(geometry) - MINIMUM_BYTES))
    try:
        restored = restore_geometry(width, height, encoded)
    except ValueError:
        # Adjacent source spans can cover the same pixels as one maximal run.
        # Their exact original representation must remain explicit.
        return None
    return bytes(encoded) if restored == geometry else None
