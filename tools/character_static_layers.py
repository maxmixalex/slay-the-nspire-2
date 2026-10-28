"""Exact static/dynamic planes for sampled character-selection loops.

Only a complete 16x16 region whose RGB565 and alpha match in every frame is
moved into the static plane. The two resulting masks are disjoint; drawing
the base followed by a delta therefore applies each original pixel once.
This uses existing sparse/LZ4/retained image formats and adds no runtime
allocation, frame interpolation, visual approximation or integrity scan.
"""
from __future__ import annotations

from pathlib import Path
import numpy as np
from PIL import Image

import content_tiles as tiles
from group_ui7_hands import RowImages
from patch_feedback_assets import write_image


def decoded(row, cache):
    payload = (Path(cache)/row['file']).read_bytes()
    tiles.require(len(payload) == row['bytes'] and tiles.sha256(payload) == row['sha256'],
                  'Changed character image before static-layer extraction')
    width, height, fmt, raw = RowImages([row], [payload]).image_raw(0)
    tiles.require(fmt == 4 and tiles.sha256(raw) == row['raw_sha256'],
                  'Character layer extraction requires exact sparse input')
    _, colors, alpha = tiles.validate_sparse(raw, width, height, pixels=True)
    return colors, alpha


def pixels_image(colors, alpha):
    """Expand at the low end of each quantization interval, preserving bits."""
    height, width = colors.shape
    rgba = np.empty((height, width, 4), dtype='u1')
    rgba[:, :, 0] = (colors >> 11) << 3
    rgba[:, :, 1] = ((colors >> 5) & 63) << 2
    rgba[:, :, 2] = (colors & 31) << 3
    rgba[:, :, 3] = alpha
    return Image.fromarray(rgba, 'RGBA')


def split_loop(rows, cache, key, *, side=16):
    """Return one base, ordered deltas, and an executed exact-pixel receipt."""
    tiles.require(rows and 1 <= side <= 32, 'Invalid character loop or region size')
    cache = Path(cache)
    first_colors, first_alpha = decoded(rows[0], cache)
    shape = first_colors.shape
    constant = np.ones(shape, dtype=bool)
    for row in rows[1:]:
        colors, alpha = decoded(row, cache)
        tiles.require(colors.shape == shape, 'Character loop dimensions changed')
        constant &= (colors == first_colors) & (alpha == first_alpha)
    # Whole regions avoid isolated quantization coincidences creating large
    # numbers of one-pixel spans in the dynamic plane.
    height, width = shape
    for y in range(0, height, side):
        for x in range(0, width, side):
            region = constant[y:y+side, x:x+side]
            region[:] = bool(np.all(region))
    static_alpha = np.where(constant, first_alpha, 0).astype('u1')
    base = write_image(cache, key+'-static', pixels_image(first_colors, static_alpha), sparse=True)
    base.update(key=key+'.static', source=rows[0].get('source'))
    base_colors, base_alpha = decoded(base, cache)
    deltas = []
    for index, row in enumerate(rows):
        colors, alpha = decoded(row, cache)
        dynamic_alpha = np.where(constant, 0, alpha).astype('u1')
        delta = write_image(cache, key+f'-delta-{index:03d}', pixels_image(colors, dynamic_alpha), sparse=True)
        delta.update(key=key+f'.delta.{index:03d}', source=row.get('source'), frame=index,
                     sample_seconds=row.get('sample_seconds'), original_raw_sha256=row['raw_sha256'])
        delta_colors, delta_alpha = decoded(delta, cache)
        tiles.require(not np.any((base_alpha != 0) & (delta_alpha != 0)),
                      'Character static/dynamic planes overlap')
        restored_colors = np.where(base_alpha != 0, base_colors, delta_colors)
        restored_alpha = np.where(base_alpha != 0, base_alpha, delta_alpha)
        tiles.require(np.array_equal(restored_colors, colors) and np.array_equal(restored_alpha, alpha),
                      'Character static/dynamic RGB565 or alpha changed')
        deltas.append(delta)
    original_bytes = sum((row['bytes']+1) & ~1 for row in rows)
    layered_bytes = sum((row['bytes']+1) & ~1 for row in [base, *deltas])
    report = dict(key=key, frames=len(rows), region_size=side,
                  static_regions=int(sum(bool(np.all(constant[y:y+side, x:x+side]))
                      for y in range(0, height, side) for x in range(0, width, side))),
                  static_visible_pixels=int(np.count_nonzero(static_alpha)),
                  original_independent_bytes=original_bytes, layered_independent_bytes=layered_bytes,
                  independent_bytes_saved=original_bytes-layered_bytes,
                  disjoint_masks=True, all_composited_rgb565_alpha_equal=True,
                  frame_count_unchanged=True, runtime_buffer_growth=0)
    return base, deltas, report
