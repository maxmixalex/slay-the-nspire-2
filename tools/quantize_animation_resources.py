#!/usr/bin/env python3
"""Bound animation color precision for the 320x240 calculator display.

This is an explicitly lossy, offline platform adaptation, not a new codec.
Every animation frame, dimension, sparse span, index and clock is retained.
Static UI/card/icon resources and every glyph/string remain exact. Character
selection's disjoint static and dynamic planes use the same color mapping.
The existing TGR2/RGR2 readers restore ordinary RGB565 + 8-bit alpha, so this
adds no runtime work or resident buffers.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from functools import lru_cache
import json
from pathlib import Path
import struct
import time

import numpy as np

import content_tiles as t
from compact_retained_colors import Relocation, complete_slices, groups as retained_groups
from compact_tile_resources import grouped_slices
from retained_filter import _color_ranges, group_layout, transform_group, SLICE
from retile_appended_resources import encode, emit, view
import split_content


PROFILES = {1: '444a6', 2: '454a6', 4: '555a6', 8: 'exact'}
LEVELS = {1: (15, 15, 15), 2: (15, 31, 15), 4: (31, 31, 31)}
CHARACTER_CLIPS = range(1705, 1710)
CHARACTER_AUXILIARIES = slice(1500, 1505)


def rounded_levels(values, previous, levels):
    values = np.asarray(values, dtype=np.int32)
    return (((values * levels + previous // 2) // previous) * previous + levels // 2) // levels


@lru_cache(None)
def lookup(profile):
    """A single deterministic mapping shared by every frame and static plane."""
    colors = np.arange(65536, dtype=np.int32)
    alpha = np.arange(256, dtype=np.int32)
    if profile == 8:
        return colors.astype('<u2'), alpha.astype('u1')
    red, green, blue = LEVELS[profile]
    result = (rounded_levels(colors >> 11, 31, red) << 11) | \
             (rounded_levels((colors >> 5) & 63, 63, green) << 5) | \
             rounded_levels(colors & 31, 31, blue)
    return result.astype('<u2'), rounded_levels(alpha, 255, 63).astype('u1')


def image_indices(source, reader, image):
    offset, _w, _h, fmt, length = source.images[image]
    t.require(fmt == 6 and length == 16, 'Expected a tiled animation frame')
    _global, group, at, size = t.DESCRIPTOR.unpack_from(source.data, offset)
    raw = reader.group(reader.tile_groups + group)[0]
    _raw_size, index_bytes, geometry_bytes = t.FRAME.unpack_from(raw, at)
    t.require(size == t.FRAME.size + index_bytes + geometry_bytes, 'Frame record size differs')
    return np.frombuffer(raw, dtype='<u4', count=index_bytes // 4, offset=at + t.FRAME.size)


def selection(source, reader, policy):
    """Protect static references even if a future pack aliases them to frames."""
    t.require(policy in ('mixed', '444a6', '454a6', '555a6'), 'Unknown precision policy')
    animated = {image for first, count, *_ in source.clips for image in range(first, first + count)}
    has_character_planes = len(source.clips) >= CHARACTER_CLIPS.stop and len(source.aux) >= 1505
    planes = set(source.aux[CHARACTER_AUXILIARIES]) if has_character_planes else set()
    protected = (set(range(len(source.images))) - animated - planes) | \
                ({row[0] for row in source.cards} | set(source.aux)) - planes
    # A whole loop remains exact when its frame ID is also a static design
    # resource. This prevents one exact frame flashing among quantized frames.
    protected_clips = []
    for clip, (first, count, *_) in enumerate(source.clips):
        if any(image in protected for image in range(first, first + count)):
            protected_clips.append(clip)
            protected.update(range(first, first + count))
    default = next((key for key, name in PROFILES.items() if name == policy), 1)
    image_profiles = {}
    for clip, (first, count, *_) in enumerate(source.clips):
        profile = (4 if clip < 38 else 2 if clip in CHARACTER_CLIPS else 1) if policy == 'mixed' else default
        for image in range(first, first + count):
            image_profiles[image] = max(image_profiles.get(image, 0), 8 if image in protected else profile)
    plane_profile = 2 if policy == 'mixed' else default
    image_profiles.update((image, plane_profile) for image in planes)
    uses = np.zeros(reader.tile_count, dtype='u1')
    for image, entry in enumerate(source.images):
        if entry[3] == 6:
            uses[image_indices(source, reader, image)] |= image_profiles.get(image, 8)
    t.require(np.all(uses), 'Unreferenced tiled pixels require an explicit policy')
    return image_profiles, uses, planes, protected_clips


def quantize_tiles(raw, uses):
    """Keep exact protected tiles; reject inconsistent visible shared pixels."""
    original = np.frombuffer(raw, dtype='u1').reshape(-1, t.TILE_BYTES)
    t.require(len(original) == len(uses), 'Tile policy count differs')
    result = original.copy()
    colors = original[:, 2:514].copy().view('<u2').reshape(-1, 256)
    alpha = original[:, 514:]
    selected = np.zeros(len(uses), dtype='u1')
    for profile in PROFILES:
        selected[(uses & profile) != 0] = profile
    for profile in PROFILES:
        mask = selected == profile
        if not np.any(mask):
            continue
        color_lut, alpha_lut = lookup(profile)
        result[mask, 2:514] = color_lut[colors[mask]].view('u1').reshape(-1, 512)
        result[mask, 514:] = alpha_lut[alpha[mask]]
    output_colors = result[:, 2:514].copy().view('<u2').reshape(-1, 256)
    output_alpha = result[:, 514:]
    for profile in PROFILES:
        shared = ((uses & profile) != 0) & (selected != profile)
        if not np.any(shared):
            continue
        color_lut, alpha_lut = lookup(profile)
        target_alpha = alpha_lut[alpha[shared]]
        t.require(np.array_equal(output_alpha[shared], target_alpha) and
                  np.all((output_colors[shared] == color_lut[colors[shared]]) | (target_alpha == 0)),
                  'Shared visible tile requires conflicting color mappings; keep its loops at one precision')
    return result.tobytes()


def quantize_image(raw, width, height, fmt, profile):
    """Preserve byte layout and sparse geometry, including all opacity masks."""
    colors, alpha = lookup(profile)
    result = bytearray(raw)
    ranges = _color_ranges(raw, fmt, width, height)
    for at, count, stride in ranges:
        lo = np.ndarray((count,), dtype='u1', buffer=result, offset=at, strides=(stride,))
        hi = np.ndarray((count,), dtype='u1', buffer=result, offset=at + 1, strides=(stride,))
        values = colors[lo.astype('u2') | (hi.astype('u2') << 8)]
        lo[:] = values & 255
        hi[:] = values >> 8
        if fmt == 4 and stride == 3:
            a = np.ndarray((count,), dtype='u1', buffer=result, offset=at + 2, strides=(stride,))
            a[:] = alpha[a]
    if fmt == 1:
        start = width * height * 2
        result[start:] = alpha[np.frombuffer(raw, dtype='u1', offset=start)].tobytes()
    elif fmt == 3:
        # Palette indices and alpha are separate contiguous planes.
        a = np.ndarray((width * height,), dtype='u1', buffer=result, offset=512 + width * height)
        a[:] = alpha[a]
    t.validate_raw(result, width, height, fmt)
    return bytes(result)


def make_plan(source, work, policy):
    reader = t.TileReader(source)
    image_profiles, uses, planes, protected_clips = selection(source, reader, policy)
    rows, raw_total = [], 0
    with (work / 'quantized-tiles.raw').open('wb') as handle:
        for group in range(reader.tile_groups):
            raw = reader.group(group)[0]
            first = group * reader.per_group
            quantized = quantize_tiles(raw, uses[first:first + len(raw) // t.TILE_BYTES])
            handle.write(quantized)
            rows.append(dict(index=group, raw_offset=raw_total, raw_size=len(raw),
                             raw_sha256=t.sha256(quantized)))
            raw_total += len(raw)
            if group % 128 == 0:
                emit('quantize_animation', complete=group, total=reader.tile_groups)
    plane_groups = []
    replacements = {image: quantize_image(raw, width, height, fmt, image_profiles[image])
                    for image in planes for width, height, fmt, raw in [source.image_raw(image)]}
    for group in retained_groups(source):
        relevant = [image for image in planes if source.images[image][0] +
                    struct.unpack_from('<i', source.data, source.images[image][0])[0] == group.offset]
        if not relevant:
            continue
        payload = source.block(group.offset, group.size)
        size, start, mode, slices = group_layout(payload, source.version)
        raw = t.decode_lzma2(payload[start:], size)
        if mode:
            raw = transform_group(raw, slices, mode, True)
        slices = slices or group.slices
        t.require(complete_slices(slices, size), 'Character plane group is not completely described')
        updated = bytearray(raw)
        for image in relevant:
            _rel, _size, at, shape = struct.unpack_from('<iIII', source.data, source.images[image][0])
            t.require(len(replacements[image]) == shape & 0xffffff, 'Plane raw size changed')
            updated[at:at + len(replacements[image])] = replacements[image]
        plane_groups.append(dict(first_image=group.first_image, slices=slices,
                                 original_sha256=t.sha256(payload), raw=bytes(updated)))
    t.require(all(source.images[image][3] == 7 for image in planes), 'Character plane format changed')
    return dict(rows=rows, planes=plane_groups, image_profiles=image_profiles,
                protected_clips=protected_clips, policy=policy,
                tile_profile_uses={str(k): int(v) for k, v in zip(*np.unique(uses, return_counts=True))},
                static_plane_images=sorted(planes), raw_bytes=raw_total)


def compress_plan(source, plan, work, cache, workers):
    reader = t.TileReader(source)
    cache.mkdir(parents=True, exist_ok=True)

    def one(row):
        with (work / 'quantized-tiles.raw').open('rb') as handle:
            handle.seek(row['raw_offset'])
            raw = handle.read(row['raw_size'])
        t.require(t.sha256(raw) == row['raw_sha256'], 'Quantized spool changed')
        mode, encoded = encode(raw, 1, cache)
        group = row['index']
        payload = t.GROUP.pack(b'TGR2' if mode else b'TGR1', 1, group, len(raw), len(encoded),
                               0, len(raw) // t.TILE_BYTES, mode) + encoded
        return dict(kind='tile', index=group, original_sha256=t.sha256(source.data[
            reader.groups[group][0] - 32:reader.groups[group][0] + reader.groups[group][1]]), payload=payload)

    converted = []
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for row in pool.map(one, plan['rows']):
            converted.append(row)
            if len(converted) % 32 == 0:
                stored = sum(len(item['payload']) + (len(item['payload']) & 1) for item in converted)
                original = sum(item[1] + 32 for item in reader.groups[:len(converted)])
                emit('encode_animation_precision', complete=len(converted), total=len(plan['rows']),
                     original_bytes=original, stored_bytes=stored, saved_bytes=original-stored)
    for row in plan['planes']:
        best = None
        for mode in (0, 1, 2):
            transformed = transform_group(row['raw'], row['slices'], mode) if mode else row['raw']
            _none, encoded = encode(transformed, 2, cache)
            table = b''.join(SLICE.pack(*item) for item in row['slices']) if mode else b''
            payload = t.GROUP.pack(b'RGR2' if mode else b'RGR1', len(row['raw']), len(encoded), 14,
                len(row['slices']), mode, len(table), 0) + table + encoded
            if best is None or len(payload) < len(best):
                best = payload
        converted.append(dict(kind='plane', first_image=row['first_image'],
                              original_sha256=row['original_sha256'], payload=best))
    return converted


def rewrite(source, plan, converted):
    reader = t.TileReader(source)
    planes = {row.first_image: row for row in retained_groups(source)}
    replacements, new_sizes = [], {}
    for row in converted:
        if row['kind'] == 'tile':
            offset, length, *_ = reader.groups[row['index']]
            offset, length = offset - 32, length + 32
        else:
            group = planes[row['first_image']]
            offset, length = group.offset, group.size
        t.require(t.sha256(source.block(offset, length)) == row['original_sha256'],
                  'Locales do not share identical original artwork')
        end = offset + length
        if end & 1:
            t.require(source.data[end] == 0, 'Missing group alignment byte')
            end += 1
        payload = row['payload']
        replacements.append((offset, end, payload + bytes(len(payload) & 1)))
        new_sizes[offset] = len(payload)
    replacements.sort()
    t.require(all(a[1] <= b[0] for a, b in zip(replacements, replacements[1:])), 'Overlapping replacements')
    relocate = Relocation(replacements)
    out, previous = bytearray(), 0
    for begin, end, payload in replacements:
        out.extend(source.data[previous:begin])
        out.extend(payload)
        previous = end
    out.extend(source.data[previous:])
    for image, (offset, width, height, fmt, length) in enumerate(source.images):
        t.IMAGE.pack_into(out, 40 + image * 16, relocate(offset), width, height, fmt, length)
    for glyph, (cp, offset, size, width, height, advance, encoded) in enumerate(source.glyphs):
        t.GLYPH.pack_into(out, source.glyph_at + glyph * 16, cp, relocate(offset), size,
                         width, height, advance, encoded)
    for string, (offset, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at + string * 8, relocate(offset), length)
    header = list(t.GLOBAL.unpack_from(source.data, source.base_end))
    table = header[9]
    for field in (8, 9, 14):
        header[field] = relocate(header[field])
    t.GLOBAL.pack_into(out, source.base_end, *header)
    for group in range(reader.tile_groups + reader.index_groups):
        offset, length = struct.unpack_from('<II', source.data, table + group * 8)
        struct.pack_into('<II', out, relocate(table + group * 8), relocate(offset), new_sizes.get(offset, length))
    for frame in range(reader.frame_count):
        offset = reader.descriptors + frame * 16
        global_at, group, raw_at, length = t.DESCRIPTOR.unpack_from(source.data, offset)
        t.DESCRIPTOR.pack_into(out, relocate(offset), relocate(global_at), group, raw_at, length)
    for offset, (relative, length, raw_at, shape) in grouped_slices(source).items():
        group = offset + relative
        struct.pack_into('<iIII', out, relocate(offset), relocate(group)-relocate(offset),
                         new_sizes.get(group, length), raw_at, shape)
    struct.pack_into('<I', out, 12, len(out))
    struct.pack_into('<I', out, 36, 0)
    return bytes(out)


def verify(source, output, plan, work):
    target = view(output)
    before, after = t.TileReader(source), t.TileReader(target)
    t.require(source.counts == target.counts and source.clips == target.clips and
              source.cards == target.cards and source.aux == target.aux, 'Model, frames or clocks changed')
    with (work / 'quantized-tiles.raw').open('rb') as handle:
        for group in range(before.tile_groups):
            expected = handle.read(plan['rows'][group]['raw_size'])
            t.require(after.group(group)[0] == expected, 'Decoded quantized tile bytes differ')
    for group in range(before.tile_groups, len(before.groups)):
        a, b = before.groups[group], after.groups[group]
        t.require(source.data[a[0]-32:a[0]+a[1]] == output[b[0]-32:b[0]+b[1]],
                  'Animation index stream or geometry changed')
    exact, planes = 0, set(plan['static_plane_images'])
    for image, entry in enumerate(source.images):
        t.require(entry[1:] == target.images[image][1:], 'Image geometry or format changed')
        if entry[3] == 6:
            a = t.DESCRIPTOR.unpack_from(source.data, entry[0])
            b = t.DESCRIPTOR.unpack_from(output, target.images[image][0])
            t.require(a[1:] == b[1:], 'Frame descriptor changed')
            continue
        width, height, fmt, raw = source.image_raw(image)
        expected = quantize_image(raw, width, height, fmt, plan['image_profiles'][image]) if image in planes else raw
        t.require(target.image_raw(image) == (width, height, fmt, expected), 'Static image pixels differ')
        exact += image not in planes
    for glyph in range(len(source.glyphs)):
        a, b = source.glyphs[glyph], target.glyphs[glyph]
        t.require(a[:1]+a[2:] == b[:1]+b[2:] and source.glyph_payload(glyph) == target.glyph_payload(glyph),
                  'Glyph metadata or pixels changed')
    for string in range(len(source.strings)):
        t.require(source.string_payload(string) == target.string_payload(string), 'Localized text changed')
    old_cost = t.GLOBAL.unpack_from(source.data, source.base_end)[14]
    new_cost = t.GLOBAL.unpack_from(output, target.base_end)[14]
    t.require(source.data[old_cost:old_cost+len(source.images)] == output[new_cost:new_cost+len(target.images)],
              'Decoded image cache costs changed')
    return dict(exact_static_images=exact, matching_character_planes=len(planes),
                tile_groups_verified=before.tile_groups, exact_index_groups=before.index_groups,
                exact_glyphs=len(source.glyphs), exact_strings=len(source.strings),
                frames_dimensions_geometry_clocks_and_cache_costs_exact=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--eng', type=Path, required=True)
    parser.add_argument('--zhs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=range(1, 5), default=3)
    parser.add_argument('--policy', choices=('mixed', '444a6', '454a6', '555a6'), default='mixed')
    args = parser.parse_args()
    for path in (args.eng, args.zhs, args.output, args.work):
        t.private_path(path, 'Private animation precision input/output')
    t.require(args.eng.resolve() != (args.output/'ui11-eng.nsp').resolve() and
              args.zhs.resolve() != (args.output/'ui11-zhs.nsp').resolve(), 'Never overwrite input packs')
    args.work.mkdir(parents=True, exist_ok=True)
    args.output.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    source = view(args.eng.read_bytes())
    plan = make_plan(source, args.work, args.policy)
    emit('precision_plan', policy=args.policy, tile_groups=len(plan['rows']),
         protected_clips=plan['protected_clips'], tile_profile_uses=plan['tile_profile_uses'])
    converted = compress_plan(source, plan, args.work, args.work/'encoded', args.workers)
    receipts = {}
    for language, path in (('eng', args.eng), ('zhs', args.zhs)):
        original = source if language == 'eng' else view(path.read_bytes())
        output = rewrite(original, plan, converted)
        verification = verify(original, output, plan, args.work)
        layout = split_content.layout(output, language)
        receipts[language] = dict(input_bytes=len(original.data), output_bytes=len(output),
            saved_bytes=len(original.data)-len(output), input_sha256=t.sha256(original.data),
            output_sha256=t.sha256(output), shared_artwork_bytes=layout['artwork'], verification=verification)
        t.atomic_bytes(args.output/('ui11-'+language+'.nsp'), output)
        emit('precision_locale_complete', language=language, **receipts[language])
    english, chinese = ((args.output/('ui11-'+language+'.nsp')).read_bytes() for language in ('eng', 'zhs'))
    a, b = split_content.layout(english, 'eng'), split_content.layout(chinese, 'zhs')
    t.require(english[a['prefix']:a['prefix']+a['artwork']] == chinese[b['prefix']:b['prefix']+b['artwork']],
              'Output locales do not share identical artwork')
    report = dict(schema=1, complete=True, policy=args.policy, lossy_animation_color=True,
        format='existing NSP2/39', levels={PROFILES[key]: list(value)+[63] for key, value in LEVELS.items()},
        maximum_rgb565_code_error={'555a6': [0, 1, 0], '454a6': [1, 1, 1], '444a6': [1, 2, 1]},
        maximum_alpha_error=2, profiles=plan['tile_profile_uses'], locales=receipts,
        no_frames_removed_or_resampled=True, no_runtime_codec_or_buffer_added=True,
        seconds=time.monotonic()-started)
    t.atomic_bytes(args.output/'animation-precision-receipt.json', (json.dumps(report, indent=2)+'\n').encode())
    print(json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
