#!/usr/bin/env python3
"""Factor the complete existing regular merchant clips without changing pixels.

The private UI13 logical pack supplies every RGB565/alpha frame. Pixels which
are visible and identical in every frame become one fixed plane; the moving
frames retain all other pixels. Both planes preserve the original dimensions,
and runtime placement uses the unchanged clip pivot and clock. Independent
LZ4/sparse frames avoid the old per-frame multi-group LZMA tile reads.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image

import content_tiles as tiles
from group_ui7_hands import RowImages
from patch_feedback_assets import write_image
from ui12_resources import view

BASE_AUXILIARY, AUXILIARY_COUNT = 5265, 5275
FIRST_CLIP = 1695
CLIP_KEYS = ('bottom', 'merchant', 'ironclad', 'silent', 'defect', 'regent',
             'necrobinder', 'merchant-focused', 'flames-behind', 'flames-front')


def pixel_planes(decoded):
    """Recover exact color/alpha, independently of PNG's expanded RGB channels."""
    width, height, fmt, raw = decoded
    if fmt == 4:
        _geometry, colors, alpha = tiles.validate_sparse(raw, width, height, pixels=True)
    else:
        tiles.validate_raw(raw, width, height, fmt)
        area = width*height
        if fmt <= 1:
            colors = np.frombuffer(raw, dtype='<u2', count=area).reshape(height, width)
            alpha = (np.frombuffer(raw, dtype='u1', count=area, offset=area*2).reshape(height, width)
                     if fmt == 1 else np.full((height, width), 255, np.uint8))
        else:
            palette = np.frombuffer(raw, dtype='<u2', count=256)
            indexes = np.frombuffer(raw, dtype='u1', count=area, offset=512).reshape(height, width)
            colors = palette[indexes]
            alpha = (np.frombuffer(raw, dtype='u1', count=area, offset=512+area).reshape(height, width)
                     if fmt == 3 else np.full((height, width), 255, np.uint8))
    colors = np.array(colors, dtype='<u2', copy=True)
    alpha = np.array(alpha, dtype=np.uint8, copy=True)
    colors[alpha == 0] = 0
    return colors, alpha


def rgba_image(colors, alpha):
    """Expand RGB565 exactly; re-quantizing the PNG recovers every source bit."""
    pixels = np.empty((*colors.shape, 4), np.uint8)
    red, green, blue = colors >> 11, (colors >> 5) & 63, colors & 31
    pixels[..., 0] = (red << 3) | (red >> 2)
    pixels[..., 1] = (green << 2) | (green >> 4)
    pixels[..., 2] = (blue << 3) | (blue >> 2)
    pixels[..., 3] = alpha
    return Image.fromarray(pixels)


def invariant_mask(frames):
    tiles.require(bool(frames), 'Merchant clip has no frames')
    colors, alpha = frames[0]
    fixed = alpha != 0
    for next_colors, next_alpha in frames[1:]:
        tiles.require(next_colors.shape == colors.shape and next_alpha.shape == alpha.shape,
                      'Merchant clip changes its registered frame dimensions')
        fixed &= (next_colors == colors) & (next_alpha == alpha)
    return fixed


def separated_planes(frames):
    """Keep partial-alpha invariants too: static and moving never overlap."""
    fixed = invariant_mask(frames)
    colors, alpha = frames[0]
    static = (np.where(fixed, colors, 0).astype('<u2'), np.where(fixed, alpha, 0).astype(np.uint8))
    moving = [(np.where(fixed, 0, color).astype('<u2'), np.where(fixed, 0, opacity).astype(np.uint8))
              for color, opacity in frames]
    return static, moving, int(np.count_nonzero(fixed))


def verify_recomposition(original, static, moving):
    """Disjoint coverage proves exact source-over for every destination color."""
    color, alpha = original
    static_color, static_alpha = static
    moving_color, moving_alpha = moving
    tiles.require(color.shape == static_color.shape == moving_color.shape and
                  alpha.shape == static_alpha.shape == moving_alpha.shape,
                  'Merchant split changed a frame shape')
    tiles.require(not np.any((static_alpha != 0) & (moving_alpha != 0)),
                  'Merchant fixed/moving coverage overlaps')
    joined_alpha = np.maximum(static_alpha, moving_alpha)
    joined_color = np.where(static_alpha != 0, static_color, moving_color)
    tiles.require(np.array_equal(joined_alpha, alpha) and
                  np.array_equal(np.where(alpha != 0, joined_color, 0), np.where(alpha != 0, color, 0)),
                  'Merchant split changed visible RGB565 or alpha')


def plane_digest(planes):
    colors, alpha = planes
    return tiles.sha256(colors.astype('<u2').tobytes()+alpha.tobytes())


def decoded_row(row, cache):
    payload = (cache/row['file']).read_bytes()
    tiles.require(len(payload) == row['bytes'] and tiles.sha256(payload) == row['sha256'],
                  'Merchant encoded plane changed')
    decoded = RowImages([row], [payload]).image_raw(0)
    tiles.require(decoded[2] == 4 and tiles.sha256(decoded[3]) == row['raw_sha256'],
                  'Merchant independent sparse/LZ4 decode differs')
    return pixel_planes(decoded), len(decoded[3])


def fingerprint():
    root = Path(__file__).resolve().parent
    return tiles.sha256(b''.join((root/name).read_bytes() for name in (
        'merchant_ui14_assets.py', 'content_tiles.py', 'sparse_image.py',
        'patch_feedback_assets.py', 'group_ui7_hands.py', 'lz4_block.py')))


def source_assets(source_pack, cache, *, progress=None):
    """Return ten appended image rows and original-image-ID -> PNG paths.

    The accompanying private manifest records every independently decoded
    moving plane. Cached output is accepted only for identical source and
    converter bytes, and after checking all encoded and PNG file identities.
    """
    cache = tiles.private_path(cache, 'Private merchant split cache')
    cache.mkdir(parents=True, exist_ok=True)
    if isinstance(source_pack, (str, Path)):
        data = tiles.private_path(source_pack, 'Private merchant source pack').read_bytes()
    else:
        data = bytes(source_pack)
    source_sha, converter_sha = tiles.sha256(data), fingerprint()
    receipt = cache/'merchant-split.json'
    if receipt.exists():
        manifest = json.loads(receipt.read_text())
        if manifest.get('source_sha256') == source_sha and manifest.get('converter_sha256') == converter_sha:
            rows = manifest['images'];frames = manifest['moving_images']
            tiles.require([row['auxiliary'] for row in rows] == list(range(BASE_AUXILIARY, AUXILIARY_COUNT)),
                          'Merchant fixed-plane catalog changed')
            for row in rows+frames:
                decoded_row(row, cache)
                tiles.require(tiles.sha256((cache/row['png']).read_bytes()) == row['png_sha256'],
                              'Merchant cached PNG changed')
            tiles.emit(progress, 'merchant_split_reused', frames=len(frames), fixed_planes=len(rows))
            return rows, {row['image']: cache/row['png'] for row in frames}
    source = view(data)
    tiles.require(len(source.aux) == 5156 and len(source.clips) >= FIRST_CLIP+len(CLIP_KEYS),
                  'Merchant factoring requires the complete UI13 source pack')
    tiled = tiles.TileReader(source)
    selected = set()
    for first, count, *_ in source.clips[FIRST_CLIP:FIRST_CLIP+len(CLIP_KEYS)]:
        ids = set(range(first, first+count))
        tiles.require(not selected.intersection(ids), 'Merchant clips share image IDs')
        selected.update(ids)
    protected = set(source.aux) | {card[0] for card in source.cards}
    for index, (first, count, *_) in enumerate(source.clips):
        if not FIRST_CLIP <= index < FIRST_CLIP+len(CLIP_KEYS):
            protected.update(range(first, first+count))
    tiles.require(not selected.intersection(protected),
                  'A merchant frame also belongs to an unsplit scene')
    rows, moving_rows, reports = [], [], []
    totals = dict(original_decoded_bytes=0, moving_decoded_bytes=0, fixed_decoded_bytes=0,
                  original_visible_pixels=0, moving_visible_pixels=0, invariant_pixels=0)
    for index, key in enumerate(CLIP_KEYS):
        clip = FIRST_CLIP+index
        first, count, frame_ms, pivot_x, pivot_y, reserved = source.clips[clip]
        frames, decoded_bytes = [], 0
        for image in range(first, first+count):
            decoded = tiled.image_raw(image) if source.images[image][3] == 6 else source.image_raw(image)
            decoded_bytes += len(decoded[3]);frames.append(pixel_planes(decoded))
        static, moving, fixed_pixels = separated_planes(frames)
        prefix = 'merchant-ui14-'+key
        static_image = rgba_image(*static)
        row = write_image(cache, prefix+'-fixed', static_image, sparse=True)
        static_image.save(cache/(prefix+'-fixed.png'))
        row.update(key=prefix+'-fixed', auxiliary=BASE_AUXILIARY+index, clip=clip,
                   png=prefix+'-fixed.png', png_sha256=tiles.sha256((cache/(prefix+'-fixed.png')).read_bytes()),
                   invariant_pixels=fixed_pixels, source_frames=count, pivot=[pivot_x, pivot_y])
        decoded_static, fixed_bytes = decoded_row(row, cache);rows.append(row)
        original_digest, recomposed_digest = hashlib.sha256(), hashlib.sha256()
        source_visible = moving_visible = moving_bytes = 0
        for frame, (original, plane) in enumerate(zip(frames, moving)):
            image_id = first+frame;image = rgba_image(*plane)
            name = f'{prefix}-{frame:03d}'
            moving_row = write_image(cache, name, image, sparse=True)
            image.save(cache/(name+'.png'))
            moving_row.update(image=image_id, clip=clip, frame=frame, png=name+'.png',
                              png_sha256=tiles.sha256((cache/(name+'.png')).read_bytes()))
            decoded_moving, raw_bytes = decoded_row(moving_row, cache)
            verify_recomposition(original, decoded_static, decoded_moving)
            original_digest.update(bytes.fromhex(plane_digest(original)))
            joined = (np.where(decoded_static[1] != 0, decoded_static[0], decoded_moving[0]),
                      np.maximum(decoded_static[1], decoded_moving[1]))
            recomposed_digest.update(bytes.fromhex(plane_digest(joined)))
            moving_row['visible_pixels'] = int(np.count_nonzero(decoded_moving[1]))
            moving_row['decoded_bytes'] = raw_bytes
            source_visible += int(np.count_nonzero(original[1]));moving_visible += moving_row['visible_pixels']
            moving_bytes += raw_bytes;moving_rows.append(moving_row)
        tiles.require(original_digest.digest() == recomposed_digest.digest(), 'Merchant sequence pixel proof differs')
        report = dict(key=key, clip=clip, first=first, count=count, frame_ms=frame_ms,
                      pivot=[pivot_x, pivot_y], reserved=reserved, size=list(static_image.size),
                      invariant_pixels=fixed_pixels, original_visible_pixels=source_visible,
                      moving_visible_pixels=moving_visible, original_decoded_bytes=decoded_bytes,
                      moving_decoded_bytes=moving_bytes, fixed_decoded_bytes=fixed_bytes,
                      source_pixels_sha256=original_digest.hexdigest(),
                      recomposed_pixels_sha256=recomposed_digest.hexdigest(),
                      disjoint_coverage=True, all_encoded_frames_recomposed_exactly=True)
        reports.append(report)
        for field in totals:totals[field] += report[field]
        tiles.emit(progress, 'merchant_clip_factored', **report)
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea', source_sha256=source_sha,
                    converter_sha256=converter_sha, base_auxiliary=BASE_AUXILIARY,
                    auxiliary_count=AUXILIARY_COUNT, images=rows, moving_images=moving_rows,
                    clips=reports, totals=totals, all_frames=len(moving_rows),
                    all_encoded_frames_recomposed_exactly=True, frame_order_timing_pivots_unchanged=True,
                    runtime_buffers_added=0, moving_codec='independent lossless sparse/LZ4')
    tiles.atomic_bytes(receipt, json.dumps(manifest, indent=2).encode()+b'\n')
    return rows, {row['image']: cache/row['png'] for row in moving_rows}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-pack', type=Path, required=True)
    parser.add_argument('--cache', type=Path, required=True)
    args = parser.parse_args()
    progress = lambda value: print(json.dumps(value), flush=True)
    rows, overrides = source_assets(args.source_pack, args.cache, progress=progress)
    progress(dict(stage='merchant_split_complete', fixed_planes=len(rows), moving_frames=len(overrides),
                  manifest=str(args.cache/'merchant-split.json')))


if __name__ == '__main__':main()
