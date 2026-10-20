"""Batch the reference rasterizer's small NumPy solves without dropping frames.

The original ``animation.raster`` remains the pixel oracle. Batched output was
compared byte-for-byte against it on synthetic cases and original full clips.
Triangle order, pixel centers, interpolation and blend arithmetic are retained.
Temporary batches are bounded by both triangle count and sampled pixel count.
"""
from __future__ import annotations

import numpy as np
from PIL import Image

MAX_TRIANGLES = 32
MAX_BATCH_PIXELS = 65536


def _paint_layer(layer, xy, uv, triangles, texture, tint):
    height, width = layer.shape[:2]
    if not len(triangles):
        return
    dst = xy[triangles]
    src = uv[triangles] * [texture.shape[1] - 1, texture.shape[0] - 1]
    lower = np.maximum(0, np.floor(dst.min(axis=1)).astype(int))
    upper = np.minimum([width - 1, height - 1], np.ceil(dst.max(axis=1)).astype(int))
    mats = np.concatenate((dst.transpose(0, 2, 1), np.ones((len(dst), 1, 3))), axis=1)
    usable = np.all(upper >= lower, axis=1) & (np.abs(np.linalg.det(mats)) >= 1e-6)
    lower, upper, mats, src = lower[usable], upper[usable], mats[usable], src[usable]
    counts = np.prod(upper - lower + 1, axis=1)
    begin = 0
    while begin < len(counts):
        end, stride = begin, 0
        while end < len(counts) and end - begin < MAX_TRIANGLES:
            proposed = max(stride, int(counts[end]))
            if end > begin and proposed * (end - begin + 1) > MAX_BATCH_PIXELS:
                break
            stride = proposed
            end += 1
        # Exactly the same row-major pixel centers as meshgrid/arange in the
        # reference, with ignored padding to permit one batched LAPACK solve.
        pos = np.arange(stride)[None, :]
        lo, hi = lower[begin:end], upper[begin:end]
        row_width = (hi[:, 0] - lo[:, 0] + 1)[:, None]
        xx = lo[:, 0, None] + pos % row_width
        yy = lo[:, 1, None] + pos // row_width
        rhs = np.stack((xx + 0.5, yy + 0.5, np.ones_like(xx)), axis=1)
        weights = np.linalg.solve(mats[begin:end], rhs)
        inside = ((weights.min(axis=1) >= -1e-5) &
                  (weights.max(axis=1) <= 1.00001) &
                  (pos < counts[begin:end, None]))
        coords = src[begin:end].transpose(0, 2, 1) @ weights
        sx = np.clip(coords[:, 0], 0, texture.shape[1] - 1)
        sy = np.clip(coords[:, 1], 0, texture.shape[0] - 1)
        ix, iy = np.floor(sx).astype(int), np.floor(sy).astype(int)
        jx = np.minimum(ix + 1, texture.shape[1] - 1)
        jy = np.minimum(iy + 1, texture.shape[0] - 1)
        fx, fy = (sx - ix)[:, :, None], (sy - iy)[:, :, None]
        sample = ((texture[iy, ix] * (1 - fx) + texture[iy, jx] * fx) * (1 - fy) +
                  (texture[jy, ix] * (1 - fx) + texture[jy, jx] * fx) * fy) * tint
        # Assign each triangle in its original order. Shared-edge/overlapping
        # fragments must not rely on unspecified duplicate advanced indexes.
        for row in range(end - begin):
            mask = inside[row]
            layer[yy[row, mask], xx[row, mask]] = sample[row, mask]
        begin = end


def raster(draw, scale, origin, canvas):
    width, height = canvas
    out = np.zeros((height, width, 4))
    for vertices, uv, triangles, texture, tint, blend in draw:
        xy = vertices * scale + origin
        xy[:, 1] = origin[1] - vertices[:, 1] * scale
        layer = np.zeros_like(out)
        _paint_layer(layer, xy, uv, triangles, texture, tint)
        # Keep the reference expressions, including full-canvas operations:
        # changing alpha-zero arithmetic can move a final byte at a threshold.
        alpha, old = layer[:, :, 3:4], out[:, :, 3:4]
        new = alpha + old * (1 - alpha)
        if blend == 1:
            out[:, :, :3] = np.minimum(1, out[:, :, :3] + layer[:, :, :3] * alpha)
            out[:, :, 3:4] = new
        elif blend in (2, 3):
            src, dst = layer[:, :, :3], out[:, :, :3]
            mix = src * dst if blend == 2 else src + dst - src * dst
            premul = src * alpha * (1 - old) + dst * old * (1 - alpha) + mix * alpha * old
            out[:, :, :3] = np.divide(premul, new, out=np.zeros_like(premul), where=new > 0)
            out[:, :, 3:4] = new
        else:
            out[:, :, :3] = np.divide(layer[:, :, :3] * alpha + out[:, :, :3] * old * (1 - alpha),
                                      new, out=np.zeros_like(out[:, :, :3]), where=new > 0)
            out[:, :, 3:4] = new
    return Image.fromarray(np.uint8(np.clip(out * 255, 0, 255)))
