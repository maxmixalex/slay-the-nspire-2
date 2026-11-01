"""Exact sparse source-channel fields for continuous card materials.

This is storage, not a texture-size or color approximation. All retained source
texels are byte-identical; an omitted tile must be proved irrelevant by the
material's alpha/support analysis. Bilinear neighbours remain in that support.
The runtime reads these fields from existing bounded Content image pages.
"""
from __future__ import annotations
import struct
import numpy as np

TILE = 32
HEADER = struct.Struct('<4s6H4I')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def support(alpha, horizontal=1, vertical=1):
    """Conservative texel support including every bilinear neighbour.

    ``horizontal`` also includes the greatest possible authored UV distortion.
    Clamp-to-edge consumers use this helper; repeating noise is never culled.
    """
    a = np.asarray(alpha) != 0
    require(a.ndim == 2 and 0 <= horizontal <= 64 and 0 <= vertical <= 64,
            'Invalid source support')
    h, w = a.shape
    out = np.zeros_like(a)
    # Prefix sums keep the exact rectangular dilation inexpensive, including
    # all boundary texels. No floating-point threshold is involved.
    x = np.pad(a.astype('i4'), ((0, 0), (1, 0))).cumsum(axis=1)
    lo = np.maximum(0, np.arange(w)-horizontal)
    hi = np.minimum(w, np.arange(w)+horizontal+1)
    row = x[:, hi]-x[:, lo] != 0
    y = np.pad(row.astype('i4'), ((1, 0), (0, 0))).cumsum(axis=0)
    lo = np.maximum(0, np.arange(h)-vertical)
    hi = np.minimum(h, np.arange(h)+vertical+1)
    out[:] = y[hi]-y[lo] != 0
    return out


def encode(array, used=None):
    a = np.asarray(array)
    if a.ndim == 2:
        a = a[:, :, None]
    require(a.dtype == np.uint8 and a.ndim == 3, 'Field must contain source bytes')
    h, w, channels = a.shape
    require(0 < w <= 1024 and 0 < h <= 1024 and 1 <= channels <= 4,
            'Field shape outside source limits')
    if used is None:
        used = np.ones((h, w), dtype=bool)
    used = np.asarray(used)
    require(used.dtype == bool and used.shape == (h, w), 'Invalid field support')
    cols, rows = (w+TILE-1)//TILE, (h+TILE-1)//TILE
    blocks, lookup, ids = [], {}, []
    for ty in range(rows):
        for tx in range(cols):
            x, y = tx*TILE, ty*TILE
            if not np.any(used[y:y+TILE, x:x+TILE]):
                ids.append(0)
                continue
            block = np.zeros((TILE, TILE, channels), dtype='u1')
            part = a[y:y+TILE, x:x+TILE]
            block[:part.shape[0], :part.shape[1]] = part
            raw = block.tobytes()
            if not any(raw):
                ids.append(0)
                continue
            slot = lookup.get(raw)
            if slot is None:
                slot = len(blocks)+1
                require(slot < 65536, 'Too many source tiles')
                lookup[raw] = slot
                blocks.append(raw)
            ids.append(slot)
    directory = HEADER.size
    data = (directory+len(ids)*2+3)&~3
    size = data+len(blocks)*TILE*TILE*channels
    header = HEADER.pack(b'ACF1', 2, w, h, channels, cols, rows,
                         directory, data, len(blocks), size)
    result = header+struct.pack('<%dH'%len(ids), *ids)
    # Channel-major storage is lossless. Similar masks/noise bytes remain
    # adjacent for the existing group codec; runtime addressing adds only the
    # fixed channel-plane offset and needs no reconstruction buffer.
    planes=(np.frombuffer(b''.join(blocks),'u1').reshape(len(blocks),32,32,channels)
            .transpose(3,0,1,2).tobytes()) if blocks else b''
    result += bytes(data-len(result))+planes
    validate(result)
    restored = decode(result)
    require(np.array_equal(restored[used], a[used]), 'Source field changed retained bytes')
    return result, dict(width=w, height=h, channels=channels, tiles=len(blocks),
                        directory_tiles=len(ids), bytes=size,
                        source_bytes=a.nbytes, supported_texels=int(used.sum()))


def validate(raw):
    require(len(raw) >= HEADER.size, 'Truncated source field')
    magic, version, w, h, c, cols, rows, directory, data, count, size = HEADER.unpack_from(raw)
    require(magic == b'ACF1' and version == 2 and 0 < w <= 1024 and 0 < h <= 1024 and
            1 <= c <= 4 and (cols, rows) == ((w+31)//32, (h+31)//32), 'Invalid source field header')
    require(directory == HEADER.size and data == (directory+cols*rows*2+3)&~3 and
            count <= cols*rows and size == len(raw) == data+count*1024*c,
            'Invalid source field ranges')
    ids = struct.unpack_from('<%dH'%(cols*rows), raw, directory)
    require(all(i <= count for i in ids) and set(ids)-{0} == set(range(1, count+1)),
            'Source field directory has invalid or unreachable tiles')
    require(not any(raw[directory+len(ids)*2:data]), 'Nonzero source field alignment')
    return w, h, c, cols, rows, data, ids


def decode(raw):
    w, h, c, cols, rows, data, ids = validate(raw)
    out = np.zeros((h, w, c), dtype='u1')
    for i, slot in enumerate(ids):
        if not slot:
            continue
        count=(len(raw)-data)//(1024*c)
        block=np.stack([np.frombuffer(raw,'u1',1024,data+(channel*count+slot-1)*1024)
                        .reshape(32,32) for channel in range(c)],axis=-1)
        x, y = (i % cols)*32, (i//cols)*32
        out[y:min(y+32, h), x:min(x+32, w)] = block[:min(32, h-y), :min(32, w-x)]
    return out
