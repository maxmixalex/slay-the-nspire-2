"""Exact NSP2/33 TGR2 field and RGB565 transforms (no resampling)."""
import numpy as np

TILE_BYTES = 770
MAX_ITEMS = 680


def shape(raw, mode):
    if mode not in (1, 2) or not raw or len(raw) % TILE_BYTES or len(raw) // TILE_BYTES > MAX_ITEMS:
        raise ValueError('Invalid TGR2 transform shape or mode')
    return len(raw) // TILE_BYTES


def filter_tiles(raw, mode):
    count = shape(raw, mode)
    records = np.frombuffer(raw, dtype='u1').reshape(count, TILE_BYTES)
    if np.any(records[:, :2] < 1) or np.any(records[:, :2] > 16):
        raise ValueError('Invalid source tile dimensions')
    rgb = records[:, 2:514].copy().view('<u2')
    green = (rgb >> 5) & 63
    red = ((rgb >> 11) - (green >> 1)) & 31
    blue = ((rgb & 31) - (green >> 1)) & 31
    if mode == 2:
        red = np.where(red < 16, red * 2, (32 - red) * 2 - 1)
        blue = np.where(blue < 16, blue * 2, (32 - blue) * 2 - 1)
    packed = (green | (red << 6) | (blue << 11)).astype('<u2')
    return records[:, :2].tobytes() + packed.tobytes() + records[:, 514:].tobytes()


def restore_tiles(raw, mode):
    count = shape(raw, mode)
    headers = np.frombuffer(raw[:count * 2], dtype='u1').reshape(count, 2)
    if np.any(headers < 1) or np.any(headers > 16):
        raise ValueError('Invalid transformed tile dimensions')
    packed = np.frombuffer(raw[count * 2:count * 514], dtype='<u2').reshape(count, 256)
    green = packed & 63
    red, blue = (packed >> 6) & 31, packed >> 11
    if mode == 2:
        red = (red >> 1) ^ (-(red & 1))
        blue = (blue >> 1) ^ (-(blue & 1))
    rgb = ((((red + (green >> 1)) & 31) << 11) | (green << 5) |
           ((blue + (green >> 1)) & 31)).astype('<u2')
    output = np.empty((count, TILE_BYTES), dtype='u1')
    output[:, :2] = headers
    output[:, 2:514] = rgb.view('u1').reshape(count, 512)
    output[:, 514:] = np.frombuffer(raw[count * 514:], dtype='u1').reshape(count, 256)
    return output.tobytes()
