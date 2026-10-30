"""Reversible predictors contained entirely inside one existing artwork group.

Index predictors preserve the original record lengths. Tile prediction stores
one packed dimension byte and a backward-only uint16 reference per tile. Its
771-byte filtered records fit the same 128 KiB cache pages as 770-byte originals
(170 records occupy 131070 bytes). No other group or previous frame is needed.
"""
from __future__ import annotations

import struct
import numpy as np

TILE_BYTES = 770
PREDICTED_TILE_BYTES = 771


def require(ok, message):
    if not ok:
        raise ValueError(message)


def zigzag(values, mask):
    values = np.asarray(values, dtype=np.int32) & mask
    return np.where(values <= mask // 2, values * 2, (mask + 1 - values) * 2 - 1)


def unzigzag(values):
    return (values >> 1) ^ -(values & 1)


def index_transform(raw, count, mode, inverse=False):
    require(1 <= count <= 64 and mode in (1, 2, 3, 5, 6, 7), 'Invalid index predictor')
    source = memoryview(raw)
    output = bytearray(raw)
    cursor, previous, previous_geometry = 0, None, None
    predictor = mode & 3
    for _ in range(count):
        require(cursor + 12 <= len(raw), 'Truncated predictive index header')
        size, ids_bytes, geometry = struct.unpack_from('<III', raw, cursor)
        end = cursor + 12 + ids_bytes + geometry
        require(0 < size <= 614880 and 0 < ids_bytes <= 1200 and ids_bytes % 4 == 0 and
                0 < geometry <= size and end <= len(raw), 'Invalid predictive index record')
        ids = np.frombuffer(source, dtype='<u4', count=ids_bytes // 4, offset=cursor + 12)
        current = np.empty(len(ids), dtype='<u4')
        same = previous is not None and len(previous) == len(ids)
        for i, value in enumerate(ids):
            spatial = int(current[i - 1]) if inverse and i else int(ids[i - 1]) if i else 0
            temporal = int(previous[i]) if same else 0
            if inverse:
                code = int(value)
                if predictor == 3:
                    base = temporal if code & 1 else spatial
                    code >>= 1
                else:
                    base = spatial if predictor == 1 else temporal
                difference = (code >> 1) ^ -(code & 1)
                current[i] = (base + difference) & 0xffffffff
            else:
                def encode(base):
                    difference = ((int(value) - base + 0x80000000) & 0xffffffff) - 0x80000000
                    return ((difference << 1) ^ (difference >> 31)) & 0xffffffff
                if predictor == 3:
                    a, b = encode(spatial), encode(temporal)
                    code = (min(a, b) << 1) | int(b < a)
                    require(code <= 0xffffffff, 'Hybrid predictive tile ID exceeds its bound')
                else:
                    code = encode(spatial if predictor == 1 else temporal)
                current[i] = code
        output[cursor + 12:cursor + 12 + ids_bytes] = current.tobytes()
        here_geometry = bytes(source[cursor + 12 + ids_bytes:end])
        if mode & 4 and previous_geometry is not None and len(previous_geometry) == geometry:
            a = np.frombuffer(here_geometry, dtype='u1').astype(np.int16)
            b = np.frombuffer(previous_geometry, dtype='u1').astype(np.int16)
            transformed = ((a + b) if inverse else (a - b)).astype('u1').tobytes()
            output[cursor + 12 + ids_bytes:end] = transformed
        previous = current if inverse else ids.copy()
        previous_geometry = bytes(output[cursor + 12 + ids_bytes:end]) if inverse else here_geometry
        cursor = end
    require(cursor == len(raw), 'Predictive index count/trailing bytes differ')
    return bytes(output)


def tile_fields(raw):
    require(raw and len(raw) % TILE_BYTES == 0 and len(raw) // TILE_BYTES <= 680,
            'Invalid predictive tile record size')
    records = np.frombuffer(raw, dtype='u1').reshape(-1, TILE_BYTES)
    require(np.all((records[:, :2] >= 1) & (records[:, :2] <= 16)), 'Invalid tile dimensions')
    rgb = records[:, 2:514].copy().view('<u2').astype(np.int32)
    green = (rgb >> 5) & 63
    red = ((rgb >> 11) - (green >> 1)) & 31
    blue = ((rgb & 31) - (green >> 1)) & 31
    alpha = records[:, 514:].astype(np.int32)
    return records, rgb, green, red, blue, alpha


def predict_tiles(raw):
    records, rgb, green, red, blue, alpha = tile_fields(raw)
    count = len(records)
    # Nearest candidates use the actual RGB/alpha signal at a fixed 4x4 grid.
    # The final choice compares all 256 pixels. Only earlier local records are
    # eligible, so the inverse is a single bounded forward pass.
    selected = np.array([y * 16 + x for y in (0, 5, 10, 15) for x in (0, 5, 10, 15)])
    features = np.concatenate(((rgb >> 11)[:, selected], (green >> 1)[:, selected],
                               (rgb & 31)[:, selected], (alpha >> 3)[:, selected]), axis=1).astype(np.float32)
    norms = np.einsum('ij,ij->i', features, features)
    distance = norms[:, None] + norms[None, :] - 2 * features @ features.T
    references = np.full(count, 65535, dtype='<u2')
    packed = np.empty((count, 256), dtype='<u2')
    opacity = np.empty((count, 256), dtype='u1')
    cost = np.log2(np.arange(256, dtype=np.float64) + 1)
    for tile in range(count):
        g, r, b, a = green[tile], zigzag(red[tile], 31), zigzag(blue[tile], 31), alpha[tile]
        literal_cost = float(np.sum(cost[g] + cost[r] + cost[b] + cost[a]))
        if tile:
            n = min(12, tile)
            candidates = np.argpartition(distance[tile, :tile], n - 1)[:n]
            dg = zigzag(green[tile] - green[candidates], 63)
            dr = zigzag(red[tile] - red[candidates], 31)
            db = zigzag(blue[tile] - blue[candidates], 31)
            da = zigzag(alpha[tile] - alpha[candidates], 255)
            scores = np.sum(cost[dg] + cost[dr] + cost[db] + cost[da], axis=1)
            best = int(np.argmin(scores))
            if float(scores[best]) + 24 < literal_cost * .85:
                references[tile] = candidates[best]
                g, r, b, a = dg[best], dr[best], db[best], da[best]
        packed[tile] = g | (r << 6) | (b << 11)
        opacity[tile] = a
    dimensions = ((records[:, 0] - 1) | ((records[:, 1] - 1) << 4)).astype('u1')
    return dimensions.tobytes() + references.tobytes() + packed.tobytes() + opacity.tobytes()


def restore_tiles(raw, count):
    require(0 < count <= 680 and len(raw) == count * PREDICTED_TILE_BYTES,
            'Invalid predicted tile group length')
    dimensions = np.frombuffer(raw, dtype='u1', count=count)
    references = np.frombuffer(raw, dtype='<u2', count=count, offset=count)
    colors = np.frombuffer(raw, dtype='<u2', count=count * 256, offset=count * 3).reshape(count, 256).astype(np.int32)
    alphas = np.frombuffer(raw, dtype='u1', count=count * 256, offset=count * 515).reshape(count, 256)
    output = np.empty((count, TILE_BYTES), dtype='u1')
    greens = np.empty((count, 256), dtype=np.int32)
    reds, blues, opacity = greens.copy(), greens.copy(), greens.copy()
    for tile in range(count):
        ref = int(references[tile])
        require(ref == 65535 or ref < tile, 'Predictive tile reference is not backward')
        p = colors[tile]
        g, r, b = p & 63, unzigzag((p >> 6) & 31), unzigzag(p >> 11)
        a = alphas[tile].astype(np.int32)
        if ref != 65535:
            g = (greens[ref] + unzigzag(g)) & 63
            r = (reds[ref] + r) & 31
            b = (blues[ref] + b) & 31
            a = (opacity[ref] + unzigzag(a)) & 255
        greens[tile], reds[tile], blues[tile], opacity[tile] = g, r, b, a
        rgb = ((((r + (g >> 1)) & 31) << 11) | (g << 5) | ((b + (g >> 1)) & 31)).astype('<u2')
        output[tile, :2] = (int(dimensions[tile] & 15) + 1, int(dimensions[tile] >> 4) + 1)
        output[tile, 2:514] = rgb.view('u1')
        output[tile, 514:] = a
    return output.tobytes()
