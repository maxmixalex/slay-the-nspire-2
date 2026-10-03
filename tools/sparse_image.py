"""Lossless RGB565 scanline runs with minimum-cost opaque/alpha segmentation."""
from __future__ import annotations
import struct
import numpy as np
from PIL import Image


def encode(image: Image.Image) -> bytes:
    rgba = np.asarray(image.convert('RGBA'), dtype=np.uint16)
    rgb = ((rgba[:, :, 0] >> 3) << 11) | ((rgba[:, :, 1] >> 2) << 5) | (rgba[:, :, 2] >> 3)
    output = bytearray()
    for y, row in enumerate(rgba):
        spans = []
        x = 0
        while x < image.width:
            if row[x, 3] == 0:
                x += 1
                continue
            start = x
            while x < image.width and row[x, 3] != 0:
                x += 1
            # Each run has a five-byte header. Keeping a few opaque pixels in
            # an alpha run can cost less than introducing two extra headers.
            # Two-state dynamic programming finds the exact cheapest encoding.
            costs = [5, 5]
            parents = []
            for at in range(start, x):
                previous = [0 if costs[0] <= costs[1] + 5 else 1,
                            1 if costs[1] <= costs[0] + 5 else 0]
                next_cost = [costs[previous[0]] + (5 if previous[0] else 0) + 2,
                             costs[previous[1]] + (5 if previous[1] == 0 else 0) + 3]
                if row[at, 3] != 255:
                    next_cost[0] = 1 << 30
                parents.append(previous)
                costs = next_cost
            mode = 0 if costs[0] <= costs[1] else 1
            modes = [0] * (x - start)
            for i in range(x - start - 1, -1, -1):
                modes[i] = mode
                mode = parents[i][mode]
            at = start
            while at < x:
                end = at + 1
                mode = modes[at - start]
                while end < x and modes[end - start] == mode:
                    end += 1
                pixels = rgb[y, at:end].astype('<u2')
                if mode == 0:
                    payload = pixels.tobytes()
                else:
                    data = np.empty((end - at, 3), dtype='u1')
                    data[:, 0], data[:, 1], data[:, 2] = pixels & 255, pixels >> 8, row[at:end, 3]
                    payload = data.tobytes()
                spans.append(struct.pack('<HHB', at, end - at, mode) + payload)
                at = end
        output.extend(struct.pack('<H', len(spans)))
        output.extend(b''.join(spans))
    return bytes(output)
