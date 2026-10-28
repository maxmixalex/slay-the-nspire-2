#!/usr/bin/env python3
"""Apply existing lossless dicing to animations appended after NSP2/15.

The original tiled catalog is reused verbatim. New sparse pictures reuse its
global tiles before adding any records. Image IDs, every decoded sparse byte,
clip timings, geometry, fonts and strings remain unchanged. This produces the
same existing NSP2/39 format and needs no new runtime codec or startup pass.
"""
from __future__ import annotations

import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import lzma
from pathlib import Path
import struct
import time

import numpy as np

import content_tiles as t
from repack_retained import auxiliary_count
from tile_filter import filter_tiles
import split_content


def view(data):
    return t.PackView(data, auxiliary_count(data), versions=(39,40))


def emit(stage, **values):
    print(json.dumps(dict(stage=stage, **values)), flush=True)


class TileCatalog:
    """Disk-backed exact comparisons; only hashes and IDs stay in host RAM."""
    def __init__(self, path):
        self.path = path
        self.writer = path.open('wb', buffering=0)
        self.reader = path.open('rb', buffering=0)
        self.keys = {}
        self.count = self.reused = 0

    def add(self, record):
        key = hashlib.sha256(record).digest()
        for previous in self.keys.get(key, ()):
            self.reader.seek(previous * t.TILE_BYTES)
            if self.reader.read(t.TILE_BYTES) == record:
                self.reused += 1
                return previous
        index = self.count
        self.writer.write(record)
        self.keys.setdefault(key, []).append(index)
        self.count += 1
        t.require(self.count * t.TILE_BYTES <= t.SPOOL_LIMIT, 'Tile spool exceeds bound')
        return index

    def close(self):
        self.writer.close()
        self.reader.close()


def make_plan(source, work):
    old = t.TileReader(source)
    catalog = TileCatalog(work / 'tiles.raw')
    selected, skipped, mapping, frames, seen = [], Counter(), {}, [], {}
    ranges, group, group_bytes, group_frames, total = [], 0, 0, 0, 0
    animated = {i for first, count, *_ in source.clips for i in range(first, first + count)}
    try:
        for index in range(old.tile_groups):
            raw = old.group(index)[0]
            for at in range(0, len(raw), t.TILE_BYTES):
                expected = index * old.per_group + at // t.TILE_BYTES
                t.require(catalog.add(raw[at:at + t.TILE_BYTES]) == expected,
                          'Existing tile catalog unexpectedly contains duplicate IDs')
            if index % 128 == 0:
                emit('seed_tiles', groups=index, total=old.tile_groups)
        old.cache.clear()
        t.require(catalog.count == old.tile_count, 'Original tile count differs')
        with (work / 'indices.raw').open('wb') as indices:
            for image, entry in enumerate(source.images):
                if entry[3] == 6 or image not in animated:
                    continue
                width, height, fmt, raw = source.image_raw(image)
                if fmt != 4:
                    skipped[str(fmt)] += 1
                    continue
                selected.append(image)
                key = (width, height, len(raw), t.sha256(raw))
                if key in seen:
                    mapping[image] = seen[key]
                    continue
                geometry, colors, alpha = t.validate_sparse(raw, width, height, pixels=True)
                ids = []
                rgb_tile = np.zeros((t.SIDE, t.SIDE), dtype='<u2')
                alpha_tile = np.zeros((t.SIDE, t.SIDE), dtype='u1')
                for y in range(0, height, t.SIDE):
                    h = min(t.SIDE, height - y)
                    for x in range(0, width, t.SIDE):
                        w = min(t.SIDE, width - x)
                        rgb_tile.fill(0)
                        alpha_tile.fill(0)
                        rgb_tile[:h, :w] = colors[y:y+h, x:x+w]
                        alpha_tile[:h, :w] = alpha[y:y+h, x:x+w]
                        ids.append(catalog.add(bytes((w, h)) + rgb_tile.tobytes() + alpha_tile.tobytes()))
                raw_ids = np.asarray(ids, dtype='<u4').tobytes()
                record = t.FRAME.pack(len(raw), len(raw_ids), len(geometry)) + raw_ids + geometry
                t.require(len(record) <= t.GROUP_LIMIT, 'Frame record exceeds group bound')
                if group_frames and (group_frames == t.INDEX_FRAMES or group_bytes + len(record) > t.GROUP_LIMIT):
                    ranges.append((total - group_bytes, group_bytes, group_frames))
                    group += 1
                    group_bytes = group_frames = 0
                mapping[image] = seen[key] = len(frames)
                frames.append(dict(image=image, width=width, height=height, raw_bytes=len(raw),
                                   raw_sha256=key[3], index_group=group, record_offset=group_bytes,
                                   record_bytes=len(record)))
                indices.write(record)
                group_bytes += len(record)
                group_frames += 1
                total += len(record)
                if len(frames) % 500 == 0:
                    emit('append_tiles', images=len(selected), frames=len(frames),
                         added_tiles=catalog.count - old.tile_count)
            if group_frames:
                ranges.append((total - group_bytes, group_bytes, group_frames))
        first_changed = old.tile_count // old.per_group
        if catalog.count == old.tile_count:
            first_changed = old.tile_groups
        tile_ranges = [(first * t.TILE_BYTES,
                        min(old.per_group, catalog.count - first) * t.TILE_BYTES,
                        min(old.per_group, catalog.count - first))
                       for first in range(first_changed * old.per_group, catalog.count, old.per_group)]
        return dict(selected=selected, mapping=mapping, frames=frames,
                    tiles=catalog.count, old_tiles=old.tile_count,
                    first_changed=first_changed, tile_ranges=tile_ranges, index_ranges=ranges,
                    per_group=old.per_group, skipped=dict(skipped))
    finally:
        catalog.close()


def encode(raw, kind, cache):
    identity = b'ui11-retile-lzma2-v1' + bytes((kind,)) + raw
    path = cache / (t.sha256(identity) + '.bin')
    if path.is_file():
        result = path.read_bytes()
        mode, size = struct.unpack_from('<II', result)
        candidate = t.decode_lzma2(result[8:], size)
        if mode:
            candidate = t.restore_tiles(candidate, mode)
        t.require(candidate == raw, 'Cached retile bytes differ')
        return mode, result[8:]
    best = None
    for mode in ((0, 1, 2) if kind == 1 else (0,)):
        transformed = filter_tiles(raw, mode) if mode else raw
        for lc, lp, pb in ((3, 0, 2), (4, 0, 1), (3, 1, 1)):
            compressed = lzma.compress(transformed, format=lzma.FORMAT_RAW,
                filters=[dict(id=lzma.FILTER_LZMA2, preset=9, dict_size=t.GROUP_LIMIT,
                              nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
            if best is None or len(compressed) < len(best[1]):
                best = mode, compressed
    mode, result = best
    decoded = t.decode_lzma2(result, len(raw))
    t.require((t.restore_tiles(decoded, mode) if mode else decoded) == raw,
              'Retile compressed bytes differ')
    t.atomic_bytes(path, struct.pack('<II', mode, len(raw)) + result)
    return best


def compress_plan(plan, work, cache, workers):
    cache.mkdir(parents=True, exist_ok=True)
    jobs = [(1, work / 'tiles.raw', row) for row in plan['tile_ranges']]
    jobs += [(2, work / 'indices.raw', row) for row in plan['index_ranges']]

    def one(job):
        kind, path, (offset, size, count) = job
        with path.open('rb') as handle:
            handle.seek(offset)
            raw = handle.read(size)
        t.require(len(raw) == size, 'Truncated retile spool')
        mode, compressed = encode(raw, kind, cache)
        return dict(kind=kind, size=size, count=count, mode=mode, compressed=compressed)

    result = []
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for row in pool.map(one, jobs):
            result.append(row)
            if len(result) % 32 == 0:
                emit('compress_appended', complete=len(result), total=len(jobs),
                     stored_bytes=sum(len(r['compressed']) + 32 for r in result))
    return result


def rewrite(source, plan, encoded):
    old = t.TileReader(source)
    t.require(old.tile_count == plan['old_tiles'] and old.per_group == plan['per_group'],
              'Locale original tile catalogs differ')
    tile_count = (plan['tiles'] + old.per_group - 1) // old.per_group
    index_count = old.index_groups + len(plan['index_ranges'])
    frame_count = old.frame_count + len(plan['frames'])
    table = source.base_end + 64
    cost_at = table + (tile_count + index_count) * 8
    descriptor_at = cost_at + ((len(source.images) + 1) & ~1)
    t.require(descriptor_at <= t.DIRECTORY_LIMIT, 'Retiled directory exceeds resident bound')
    out = bytearray(source.data[:source.base_end])
    for directory, clip in plan.get('clip_overrides', {}).items():
        directory = int(directory)
        t.require(tuple(clip[:3]) == source.clips[directory][:3],
                  'A retile registration override changed a clip clock or frame range')
        t.CLIP.pack_into(out, source.clip_at + directory * t.CLIP.size, *clip)
    out.extend(t.GLOBAL.pack(b'TIL1', 1, 16, 14, plan['tiles'], tile_count, index_count,
                            frame_count, descriptor_at, table, t.TILE_BYTES, old.per_group,
                            t.GROUP_LIMIT, t.ENCODED_LIMIT, cost_at, len(source.images)))
    out.extend(bytes((tile_count + index_count) * 8))
    previous_cost_at = t.GLOBAL.unpack_from(source.data, source.base_end)[14]
    out.extend(source.data[previous_cost_at:previous_cost_at + ((len(source.images) + 1) & ~1)])
    out.extend(source.data[old.descriptors:old.descriptors + old.frame_count * 16])
    for frame in plan['frames']:
        out.extend(t.DESCRIPTOR.pack(source.base_end, old.index_groups + frame['index_group'],
                                    frame['record_offset'], frame['record_bytes']))
    t.require(len(out) == descriptor_at + frame_count * 16, 'Retile descriptor layout differs')

    def add(payload, alignment=2):
        out.extend(bytes((-len(out)) % alignment))
        offset = len(out)
        out.extend(payload)
        return offset

    def group(payload, index):
        payload = bytearray(payload)
        struct.pack_into('<I', payload, 8, index)
        offset = add(payload)
        struct.pack_into('<II', out, table + index * 8, offset, len(payload))

    def compressed_group(row, index):
        magic = b'TGR2' if row['mode'] else b'TGR1'
        payload = t.GROUP.pack(magic, row['kind'], index, row['size'], len(row['compressed']),
                               0, row['count'], row['mode']) + row['compressed']
        group(payload, index)

    new_tile_rows = len(plan['tile_ranges'])
    if 'changed_groups' in plan:
        changed = dict(zip(plan['changed_groups'], encoded[:new_tile_rows]))
        for index in range(tile_count):
            if index in changed:
                compressed_group(changed[index], index)
            else:
                t.require(index < old.tile_groups, 'Missing new tile group')
                offset, size, *_ = old.groups[index]
                group(source.data[offset - 32:offset + size], index)
    else:
        for index in range(plan['first_changed']):
            offset, size, *_ = old.groups[index]
            group(source.data[offset - 32:offset + size], index)
        for index, row in enumerate(encoded[:new_tile_rows], plan['first_changed']):
            compressed_group(row, index)
    for index in range(old.index_groups):
        offset, size, *_ = old.groups[old.tile_groups + index]
        group(source.data[offset - 32:offset + size], tile_count + index)
    for index, row in enumerate(encoded[new_tile_rows:], tile_count + old.index_groups):
        compressed_group(row, index)

    copied, slices = {}, {}

    def copy(offset, length):
        key = offset, length
        if key not in copied:
            copied[key] = add(source.block(offset, length))
        return copied[key]

    def slice_copy(offset):
        if offset not in slices:
            relative, count, raw_at, shape = struct.unpack_from('<iIII', source.data, offset)
            target = copy(offset + relative, count)
            new_at = add(bytes(16))
            struct.pack_into('<iIII', out, new_at, target - new_at, count, raw_at, shape)
            slices[offset] = new_at
        return slices[offset]

    for image, (offset, width, height, fmt, length) in enumerate(source.images):
        if image in plan['mapping']:
            frame = plan['frames'][plan['mapping'][image]]
            new_at = descriptor_at + (old.frame_count + plan['mapping'][image]) * 16
            width, height = frame['width'], frame['height']
            fmt, length = 6, 16
            out[cost_at + image] = (frame['raw_bytes'] + 4095) // 4096
        elif fmt == 6:
            new_at = descriptor_at + offset - old.descriptors
        else:
            new_at = slice_copy(offset) if fmt == 7 else copy(offset, length)
        t.IMAGE.pack_into(out, 40 + image * 16, new_at, width, height, fmt, length)
    artwork_end = len(out)
    for index, (cp, offset, size, width, height, advance, encoded_size) in enumerate(source.glyphs):
        grouped = encoded_size & t.GLYPH_GROUP_FLAG
        count = (encoded_size & ~t.GLYPH_GROUP_FLAG) or width * height
        new_at = slice_copy(offset) if grouped else copy(offset, count)
        t.GLYPH.pack_into(out, source.glyph_at + index * 16, cp, new_at, size,
                         width, height, advance, encoded_size)
    for index, (offset, length) in enumerate(source.strings):
        new_at = slice_copy(offset) if length & t.STRING_GROUP_FLAG else copy(offset, length + 1)
        struct.pack_into('<II', out, source.string_at + index * 8, new_at, length)
    struct.pack_into('<I', out, 12, len(out))
    struct.pack_into('<I', out, 36, 0)
    if plan.get('require_smaller', True):
        t.require(len(out) < len(source.data), 'Retiling does not reduce this pack')
    return bytes(out), dict(input_bytes=len(source.data), output_bytes=len(out),
                           saved_bytes=len(source.data)-len(out), artwork_end=artwork_end,
                           old_tile_groups=old.tile_groups, tile_groups=tile_count,
                           old_frame_count=old.frame_count, frame_count=frame_count)


def verify(source, output, plan):
    target = view(output)
    reader = t.TileReader(target)
    t.require(source.counts == target.counts and source.clips == target.clips and
              source.cards == target.cards and source.aux == target.aux, 'Model/animation catalog changed')
    for index, frame in enumerate(plan['frames']):
        decoded = reader.image_raw(frame['image'])
        t.require(t.sha256(decoded[3]) == frame['raw_sha256'], 'Retiled frame bytes differ')
        if index % 1000 == 0:
            emit('verify_appended_frames', complete=index, total=len(plan['frames']))
    selected = set(plan['selected'])
    retained_checked = 0
    for index, old in enumerate(source.images):
        new = target.images[index]
        t.require(old[1:3] == new[1:3], 'Image geometry changed')
        if index in selected or old[3] == 6:
            continue
        t.require(source.image_raw(index) == target.image_raw(index), 'Retained image bytes differ')
        retained_checked += 1
    for i in range(len(source.glyphs)):
        t.require(source.glyphs[i][:1]+source.glyphs[i][2:] == target.glyphs[i][:1]+target.glyphs[i][2:] and
                  source.glyph_payload(i) == target.glyph_payload(i), 'Glyph metadata or pixels changed')
    for i in range(len(source.strings)):
        t.require(source.string_payload(i) == target.string_payload(i), 'Localized text changed')
    return dict(exact_appended_frames=len(plan['frames']), exact_retained_images=retained_checked,
                existing_tiled_images_preserved=sum(row[3] == 6 for row in source.images),
                exact_glyphs=len(source.glyphs), exact_strings=len(source.strings),
                no_frame_removed_resampled_or_quantized=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--eng', type=Path, required=True)
    parser.add_argument('--zhs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=range(1, 5), default=3)
    args = parser.parse_args()
    for path in (args.eng, args.zhs, args.output, args.work):
        t.private_path(path, 'Private retile input/output')
    args.output.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    source = view(args.eng.read_bytes())
    plan = make_plan(source, args.work)
    emit('plan_complete', selected_images=len(plan['selected']), canonical_frames=len(plan['frames']),
         added_tiles=plan['tiles']-plan['old_tiles'], tile_groups=len(plan['tile_ranges']),
         index_groups=len(plan['index_ranges']))
    encoded = compress_plan(plan, args.work, args.work / 'encoded', args.workers)
    receipts = {}
    for language, path in (('eng', args.eng), ('zhs', args.zhs)):
        source = source if language == 'eng' else view(path.read_bytes())
        output, receipt = rewrite(source, plan, encoded)
        receipt['verification'] = verify(source, output, plan)
        receipt['input_sha256'] = t.sha256(source.data)
        receipt['output_sha256'] = t.sha256(output)
        t.atomic_bytes(args.output / ('ui11-' + language + '.nsp'), output)
        receipts[language] = receipt
    a, b = (args.output / 'ui11-eng.nsp').read_bytes(), (args.output / 'ui11-zhs.nsp').read_bytes()
    la, lb = split_content.layout(a, 'eng'), split_content.layout(b, 'zhs')
    t.require(a[la['prefix']:la['prefix']+la['artwork']] == b[lb['prefix']:lb['prefix']+lb['artwork']],
              'Retiled locales do not share exact artwork')
    result = dict(schema=1, complete=True, format='existing NSP2/39', locales=receipts,
                  selected_images=len(plan['selected']), new_tiles=plan['tiles']-plan['old_tiles'],
                  original_tiles=plan['old_tiles'], shared_artwork_bytes=la['artwork'],
                  seconds=time.monotonic()-started)
    t.atomic_bytes(args.output / 'retile-receipt.json', (json.dumps(result, indent=2)+'\n').encode())
    print(json.dumps(result), flush=True)


if __name__ == '__main__':
    main()
