"""Exact independent 8x8 dictionaries inside unchanged 16x16 tile groups.

This experiment preserves the existing group/frame boundaries and restores
every original 770-byte record, including alpha and unused edge pixels. It
therefore measures the four local index words as part of each candidate and
does not assume a smaller frame index or new cross-group cache behavior.
"""
import struct
import numpy as np

TILE_BYTES = 770
SUBTILE_BYTES = 192


def require(value, message):
    if not value:
        raise ValueError(message)


def encode(raw, mode):
    require(raw and len(raw) % TILE_BYTES == 0 and len(raw)//TILE_BYTES <= 680,
            'Invalid source tile shape')
    require(mode in (0, 1, 2), 'Invalid source color mode')
    records = np.frombuffer(raw, dtype='u1').reshape(-1, TILE_BYTES)
    count = len(records)
    require(np.all((records[:, :2] >= 1) & (records[:, :2] <= 16)), 'Invalid tile dimensions')
    colors = records[:, 2:514].copy().view('<u2').reshape(count, 2, 8, 2, 8)
    alpha = records[:, 514:].reshape(count, 2, 8, 2, 8)
    blocks = np.empty((count*4, SUBTILE_BYTES), dtype='u1')
    blocks[:, :128] = colors.transpose(0, 1, 3, 2, 4).reshape(count*4, 64).copy().view('u1')
    blocks[:, 128:] = alpha.transpose(0, 1, 3, 2, 4).reshape(count*4, 64)
    _, first, inverse = np.unique(blocks.view('V192').reshape(-1), return_index=True, return_inverse=True)
    order = np.argsort(first)
    remap = np.empty(len(order), dtype='<u2')
    remap[order] = np.arange(len(order), dtype='<u2')
    ids = remap[inverse]
    unique = blocks[first[order]]
    if mode:
        rgb = unique[:, :128].copy().view('<u2')
        green = (rgb >> 5) & 63
        red = ((rgb >> 11) - (green >> 1)) & 31
        blue = ((rgb & 31) - (green >> 1)) & 31
        if mode == 2:
            red = np.where(red < 16, red*2, (32-red)*2-1)
            blue = np.where(blue < 16, blue*2, (32-blue)*2-1)
        payload = (green | (red << 6) | (blue << 11)).astype('<u2').tobytes() + unique[:, 128:].tobytes()
    else:
        payload = unique.tobytes()
    return (struct.pack('<I', len(unique)) + records[:, :2].tobytes() +
            ids.tobytes() + payload)


def decode(data, count, mode):
    require(1 <= count <= 680 and mode in (0, 1, 2) and len(data) >= 4+count*10,
            'Invalid subtile dictionary header')
    unique_count, = struct.unpack_from('<I', data)
    require(1 <= unique_count <= count*4 and len(data) == 4+count*10+unique_count*SUBTILE_BYTES,
            'Invalid subtile dictionary length')
    dims = np.frombuffer(data, dtype='u1', count=count*2, offset=4).reshape(count, 2)
    require(np.all((dims >= 1) & (dims <= 16)), 'Invalid subtile dimensions')
    ids = np.frombuffer(data, dtype='<u2', count=count*4, offset=4+count*2)
    require(np.all(ids < unique_count), 'Subtile index exceeds local dictionary')
    offset = 4+count*10
    if mode:
        packed = np.frombuffer(data, dtype='<u2', count=unique_count*64, offset=offset).reshape(unique_count, 64)
        green = packed & 63
        red, blue = (packed >> 6) & 31, packed >> 11
        if mode == 2:
            red = (red >> 1) ^ -(red & 1)
            blue = (blue >> 1) ^ -(blue & 1)
        colors = ((((red+(green >> 1)) & 31) << 11) | (green << 5) | ((blue+(green >> 1)) & 31)).astype('<u2')
        alpha = np.frombuffer(data, dtype='u1', count=unique_count*64, offset=offset+unique_count*128).reshape(unique_count, 64)
    else:
        unique = np.frombuffer(data, dtype='u1', offset=offset).reshape(unique_count, SUBTILE_BYTES)
        colors = unique[:, :128].copy().view('<u2')
        alpha = unique[:, 128:]
    output = np.empty((count, TILE_BYTES), dtype='u1')
    output[:, :2] = dims
    output[:, 2:514] = colors[ids].reshape(count, 2, 2, 8, 8).transpose(0, 1, 3, 2, 4).reshape(count, 256).copy().view('u1')
    output[:, 514:] = alpha[ids].reshape(count, 2, 2, 8, 8).transpose(0, 1, 3, 2, 4).reshape(count, 256)
    return output.tobytes()
