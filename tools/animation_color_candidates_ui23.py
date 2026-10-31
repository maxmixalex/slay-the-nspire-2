#!/usr/bin/env python3
"""Build private, unapproved color-only candidates for LCD visual comparison.

No image is resized, no alpha value changes, and no animation frame is removed.
The output uses the input's existing tile/retained codecs, bounds and grouping.
This is an experiment generator, not a claim that a color reduction is invisible.
Typed opaque parameter images, fonts, text, geometry and clocks remain exact.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from functools import lru_cache
import json
import lzma
import mmap
from pathlib import Path
import struct
import time

import numpy as np

import content_tiles as t
from coalesce_tile_resources import source_lzma_profile
from compact_retained_colors import groups as retained_groups
from quantize_animation_resources import rewrite
from retained_filter import _color_ranges, group_layout, transform_group
from retile_appended_resources import emit, view
from tile_filter import filter_tiles


LEVELS = {'555a8': (31, 31, 31), '454a8': (15, 31, 15),
          '444a8': (15, 15, 15)}


@lru_cache(None)
def color_lut(policy):
    """One time-independent color map; opacity is deliberately not an input."""
    t.require(policy in LEVELS, 'Unknown color policy')
    colors = np.arange(65536, dtype=np.int32)
    result = np.zeros(65536, dtype=np.int32)
    for shift, previous, levels in zip((11, 5, 0), (31, 63, 31), LEVELS[policy]):
        channel = (colors >> shift) & previous
        reduced = (channel * levels + previous // 2) // previous
        restored = (reduced * previous + levels // 2) // levels
        result |= restored << shift
    return result.astype('<u2')


def color_image(raw, fmt, policy, width=None, height=None):
    """Only replace color words; preserve palette indices, alpha and geometry."""
    result = bytearray(raw)
    for at, count, stride in _color_ranges(raw, fmt, width, height):
        lo = np.ndarray((count,), dtype='u1', buffer=result, offset=at, strides=(stride,))
        hi = np.ndarray((count,), dtype='u1', buffer=result, offset=at + 1, strides=(stride,))
        values = color_lut(policy)[lo.astype('u2') | (hi.astype('u2') << 8)]
        lo[:] = values & 255
        hi[:] = values >> 8
    return bytes(result)


def color_tiles(raw, policy):
    t.require(len(raw) % t.TILE_BYTES == 0, 'Incomplete tile record')
    original = np.frombuffer(raw, dtype='u1').reshape(-1, t.TILE_BYTES)
    result = original.copy()
    colors = original[:, 2:514].copy().view('<u2').reshape(-1, 256)
    result[:, 2:514] = color_lut(policy)[colors].view('u1').reshape(-1, 512)
    t.require(np.array_equal(result[:, :2], original[:, :2]) and
              np.array_equal(result[:, 514:], original[:, 514:]),
              'Tile dimensions or alpha changed')
    return result.tobytes()


def encode_like(raw, original, dictionary):
    """Retain the existing decoder's LZMA settings and dictionary allocation."""
    lc, lp, pb = source_lzma_profile(original)
    encoded = lzma.compress(raw, format=lzma.FORMAT_RAW, filters=[dict(
        id=lzma.FILTER_LZMA2, preset=9, dict_size=dictionary, nice_len=273,
        depth=512, lc=lc, lp=lp, pb=pb)])
    t.require(t.decode_lzma2(encoded, len(raw), dictionary) == raw,
              'Candidate LZMA roundtrip failed')
    return encoded


def jobs(source, policy, *, retained=True):
    """Yield bounded independent groups; no full decoded animation spool."""
    reader = t.TileReader(source)
    t.require(not any(reader.sparse), 'Candidate requires ordinary TGR1/TGR2 pixel groups')
    for group in range(reader.tile_groups):
        offset, length, *_ = reader.groups[group]
        original = bytes(source.data[offset - 32:offset + length])
        raw = color_tiles(reader.group(group)[0], policy)
        mode = reader.filters[group]
        filtered = filter_tiles(raw, mode) if mode else raw
        yield dict(kind='tile', index=group, original=original, raw=raw,
                   filtered=filtered, start=32, dictionary=reader.dictionary)
    if not retained:
        return
    # Selection's fixed layers must use the same map as their moving layers.
    # Every other opaque fmt0 image stays exact: this includes typed actor,
    # mesh and shader parameter words which must never be treated as colors.
    planes = set(source.aux[1500:1505]) if len(source.aux) >= 1505 else set()
    opaque_slices = set()
    for image in planes:
        offset, _w, _h, fmt, _length = source.images[image]
        if fmt == 7:
            relative, _size, at, _shape = struct.unpack_from('<iIII', source.data, offset)
            opaque_slices.add((offset + relative, at))
    for group in retained_groups(source):
        original = bytes(source.block(group.offset, group.size))
        size, start, mode, slices = group_layout(original, source.version)
        raw = t.decode_lzma2(original[start:], size)
        if mode:
            raw = transform_group(raw, slices, mode, True)
        out = bytearray(raw)
        selected = []
        for at, shape in group.slices:
            length, fmt = shape & 0xffffff, shape >> 24
            if fmt in (1, 3, 4) or (group.offset, at) in opaque_slices:
                out[at:at+length] = color_image(raw[at:at+length], fmt, policy)
                selected.append(at)
        if out == raw:
            continue
        updated = bytes(out)
        filtered = transform_group(updated, slices, mode) if mode else updated
        yield dict(kind='plane', first_image=group.first_image, original=original,
                   raw=updated, filtered=filtered, start=start,
                   dictionary=t.GROUP_LIMIT, selected_slices=selected)


def convert_job(job):
    original = job['original']
    encoded = encode_like(job['filtered'], original[job['start']:], job['dictionary'])
    header = list(t.GROUP.unpack_from(original))
    header[4 if job['kind'] == 'tile' else 2] = len(encoded)
    payload = t.GROUP.pack(*header) + original[32:job['start']] + encoded
    t.require(len(payload) <= t.ENCODED_LIMIT, 'Encoded candidate exceeds existing group bound')
    result = dict(kind=job['kind'], original_sha256=t.sha256(original), payload=payload,
                  raw_sha256=t.sha256(job['raw']), original_bytes=len(original),
                  candidate_bytes=len(payload))
    for key in ('index', 'first_image', 'selected_slices'):
        if key in job:
            result[key] = job[key]
    return result


def convert(source, policy, *, workers=2, retained=True):
    """Bound submissions: at most one raw group per worker plus its successor."""
    results = []
    with ThreadPoolExecutor(max_workers=workers) as pool:
        pending = []
        for job in jobs(source, policy, retained=retained):
            pending.append(pool.submit(convert_job, job))
            if len(pending) >= workers * 2:
                results.append(pending.pop(0).result())
                if len(results) % 64 == 0:
                    emit('color_candidates', complete=len(results), policy=policy,
                         saved_bytes=sum(x['original_bytes']-x['candidate_bytes'] for x in results))
        results.extend(future.result() for future in pending)
    return results


def verify(source, output, results):
    target = view(output)
    before, after = t.TileReader(source), t.TileReader(target)
    t.require(source.counts == target.counts and source.clips == target.clips and
              source.cards == target.cards and source.aux == target.aux,
              'Catalog identities or animation clocks changed')
    changed_tiles = {row['index']: row for row in results if row['kind'] == 'tile'}
    changed_retained = {row['first_image']: row for row in results if row['kind'] == 'plane'}
    for group in range(before.tile_groups):
        raw = after.group(group)[0]
        t.require(t.sha256(raw) == changed_tiles[group]['raw_sha256'], 'Candidate tile pixels differ')
        a = np.frombuffer(before.group(group)[0], dtype='u1').reshape(-1, t.TILE_BYTES)
        b = np.frombuffer(raw, dtype='u1').reshape(-1, t.TILE_BYTES)
        t.require(np.array_equal(a[:, :2], b[:, :2]) and np.array_equal(a[:, 514:], b[:, 514:]),
                  'Tile dimensions or source alpha changed')
    for group in range(before.tile_groups, len(before.groups)):
        a, b = before.groups[group], after.groups[group]
        t.require(source.data[a[0]-32:a[0]+a[1]] == output[b[0]-32:b[0]+b[1]],
                  'Animation index or implicit geometry changed')
    for a, b in zip(retained_groups(source), retained_groups(target)):
        old, new = bytes(source.block(a.offset, a.size)), bytes(target.block(b.offset, b.size))
        row = changed_retained.get(a.first_image)
        if row is None:
            t.require(old == new, 'Unselected retained group changed')
            continue
        size, start, mode, slices = group_layout(new, target.version)
        raw = t.decode_lzma2(new[start:], size)
        if mode:
            raw = transform_group(raw, slices, mode, True)
        t.require(t.sha256(raw) == row['raw_sha256'], 'Retained candidate pixels differ')
    for image, (a, b) in enumerate(zip(source.images, target.images)):
        t.require(a[1:] == b[1:], 'Image geometry or format changed')
        if a[3] == 6:
            t.require(t.DESCRIPTOR.unpack_from(source.data, a[0])[1:] ==
                      t.DESCRIPTOR.unpack_from(output, b[0])[1:], 'Frame descriptor changed')
        elif a[3] != 7:
            t.require(source.block(a[0], a[4]) == target.block(b[0], b[4]),
                      'Ungrouped image or typed parameters changed')
    for i in range(len(source.glyphs)):
        t.require(source.glyphs[i][:1]+source.glyphs[i][2:] == target.glyphs[i][:1]+target.glyphs[i][2:] and
                  source.glyph_payload(i) == target.glyph_payload(i), 'Glyph data changed')
    for i in range(len(source.strings)):
        t.require(source.string_payload(i) == target.string_payload(i), 'Localized text changed')
    old_cost = t.GLOBAL.unpack_from(source.data, source.base_end)[14]
    new_cost = t.GLOBAL.unpack_from(output, target.base_end)[14]
    t.require(source.data[old_cost:old_cost+len(source.images)] == output[new_cost:new_cost+len(target.images)],
              'Decoded cache-page costs changed')
    return dict(tile_groups=before.tile_groups, exact_index_groups=before.index_groups,
                alpha_dimensions_geometry_clocks_cache_costs_exact=True,
                glyphs=len(source.glyphs), strings=len(source.strings),
                retained_changed_groups=len(changed_retained))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--policy', choices=tuple(LEVELS), required=True)
    parser.add_argument('--workers', type=int, choices=range(1, 5), default=2)
    parser.add_argument('--tiles-only', action='store_true')
    parser.add_argument('--experimental-candidate-only', action='store_true')
    args = parser.parse_args(argv)
    if not args.experimental_candidate_only:
        parser.error('Requires --experimental-candidate-only; visual and performance approval are separate')
    t.require(args.input.resolve() != args.output.resolve(), 'Never overwrite the original input')
    for path in (args.input, args.output):
        t.private_path(path, 'Private original-derived candidate')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    with args.input.open('rb') as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
        source = view(data)
        results = convert(source, args.policy, workers=args.workers, retained=not args.tiles_only)
        output = rewrite(source, {}, results)
        checked = verify(source, output, results)
        t.atomic_bytes(args.output, output)
        receipt = dict(schema=1, complete=True, not_release_approved=True, policy=args.policy,
            input_sha256=t.sha256(data), output_sha256=t.sha256(output),
            input_bytes=len(data), output_bytes=len(output), saved_bytes=len(data)-len(output),
            visual_comparison_required=True, complete_decoder_performance_required=True,
            no_runtime_codec_or_buffer_added=True, no_frames_removed_or_resampled=True,
            alpha_error=0, verification=checked, seconds=time.monotonic()-started,
            groups=[{k:v for k,v in row.items() if k != 'payload'} for row in results])
        t.atomic_bytes(args.output.with_suffix('.json'), (json.dumps(receipt, indent=2)+'\n').encode())
        emit('color_candidate_complete', **{k:v for k,v in receipt.items() if k != 'groups'})


if __name__ == '__main__':
    main()
