"""Exact bounded TGR3 occupancy masks followed by compact color/alpha planes.

A pixel is omitted only when both its RGB565 word and alpha byte are zero.
Hidden color is therefore retained. Each group restores the same 770-byte
records and uses no more temporary plane bytes than the previous TGR2 codec.
"""
import numpy as np

from tile_filter import TILE_BYTES, MAX_ITEMS, filter_tiles, restore_tiles

CONTENT_VERSION = 45


def pixel_count(items, size):
    if not 0 < items <= MAX_ITEMS or size < items*34 or (size-items*34) % 3:
        raise ValueError('Invalid sparse tile shape')
    count = (size-items*34)//3
    if count > items*224:
        raise ValueError('Sparse tile planes exceed the existing scratch bound')
    return count


def transform(raw, mode):
    filtered = filter_tiles(raw, mode)
    items = len(raw)//TILE_BYTES
    colors = np.frombuffer(filtered, dtype='<u2', count=items*256, offset=items*2)
    alpha = np.frombuffer(filtered, dtype='u1', count=items*256, offset=items*514)
    occupied = (colors != 0) | (alpha != 0)
    masks = np.packbits(occupied, bitorder='little').tobytes()
    result = filtered[:items*2]+masks+colors[occupied].tobytes()+alpha[occupied].tobytes()
    pixel_count(items, len(result))
    return result


def restore(raw, items, mode):
    count = pixel_count(items, len(raw))
    headers = raw[:items*2]
    masks = np.frombuffer(raw, dtype='u1', count=items*32, offset=items*2)
    occupied = np.unpackbits(masks, bitorder='little').astype(np.bool_)
    if int(occupied.sum()) != count:
        raise ValueError('Sparse tile mask population disagrees with stream length')
    colors = np.zeros(items*256, dtype='<u2')
    alpha = np.zeros(items*256, dtype='u1')
    colors[occupied] = np.frombuffer(raw, dtype='<u2', count=count, offset=items*34)
    alpha[occupied] = np.frombuffer(raw, dtype='u1', count=count, offset=items*34+count*2)
    return restore_tiles(headers+colors.tobytes()+alpha.tobytes(), mode)
