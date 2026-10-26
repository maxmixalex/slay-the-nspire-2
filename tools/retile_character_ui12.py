#!/usr/bin/env python3
"""Replace one private character loop without retaining its obsolete tile data.

Every non-replaced tiled image is included in the liveness scan, including
static auxiliary pictures and aliased frames. Live tile IDs remain unchanged.
New tiles first use dead slots; only changed groups are recompressed. Existing
index groups, all other pictures, fonts, strings and clip clocks are preserved.
The existing UI11 character precision (454a6) also applies to repaired pixels.
No runtime codec, buffers, frame dropping or extra image cache is introduced.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import time

import numpy as np
from PIL import Image

import content_tiles as t
from quantize_animation_resources import quantize_image
from retile_appended_resources import view, emit, compress_plan, rewrite
from sparse_image import encode as sparse_encode


def image_ids(source, clip=1709, auxiliary=1504):
    first, count, *_ = source.clips[clip]
    t.require(count == 96, 'Pinned Necrobinder loop must retain all 96 frames')
    return sorted(set(range(first, first+count)) | {source.aux[auxiliary]})


def live_tiles(source, old, selected):
    used = np.zeros(old.tile_count, dtype=bool)
    visited = set()
    for image, (offset, _w, _h, fmt, _length) in enumerate(source.images):
        if fmt != 6 or image in selected or offset in visited:
            continue
        visited.add(offset)
        global_at, group, at, size = t.DESCRIPTOR.unpack_from(source.data, offset)
        t.require(global_at == source.base_end, 'Tiled image refers to another global catalog')
        raw, starts = old.group(old.tile_groups+group)
        t.require(starts.get(at) == size, 'Tiled image does not name a complete index record')
        _size, indices, geometry = t.FRAME.unpack_from(raw, at)
        t.require(size == t.FRAME.size+indices+geometry, 'Incomplete live frame record')
        ids = np.frombuffer(raw, dtype='<u4', count=indices//4, offset=at+t.FRAME.size)
        used[ids] = True
    emit('character_live_tiles', live=int(np.count_nonzero(used)), total=old.tile_count,
         protected_image_descriptors=len(visited))
    return used


class RecycledCatalog:
    def __init__(self, old, used, path):
        self.file = path.open('w+b', buffering=0)
        self.count, self.per_group = old.tile_count, old.per_group
        self.free = np.flatnonzero(~used).tolist()
        self.next_free = self.reused = 0
        self.changed, self.keys = set(), {}
        blank = bytes((t.SIDE, t.SIDE))+bytes(t.TILE_BYTES-2)
        for group in range(old.tile_groups):
            raw = old.group(group)[0]
            result = bytearray(raw)
            first = group*old.per_group
            for local, at in enumerate(range(0, len(raw), t.TILE_BYTES)):
                tile = first+local
                if used[tile]:
                    key = hashlib.sha256(raw[at:at+t.TILE_BYTES]).digest()
                    self.keys.setdefault(key, []).append(tile)
                else:
                    result[at:at+t.TILE_BYTES] = blank
            if result != raw:
                self.changed.add(group)
            self.file.write(result)
        old.cache.clear()

    def add(self, record):
        key = hashlib.sha256(record).digest()
        for tile in self.keys.get(key, ()):
            self.file.seek(tile*t.TILE_BYTES)
            if self.file.read(t.TILE_BYTES) == record:
                self.reused += 1
                return tile
        if self.next_free < len(self.free):
            tile = self.free[self.next_free]
            self.next_free += 1
        else:
            tile = self.count
            self.count += 1
        t.require(self.count*t.TILE_BYTES <= t.SPOOL_LIMIT, 'Recycled tile spool exceeds bound')
        self.file.seek(tile*t.TILE_BYTES)
        self.file.write(record)
        self.changed.add(tile//self.per_group)
        self.keys.setdefault(key, []).append(tile)
        return tile

    def close(self):
        self.file.close()


def replacements_from_manifest(source, path):
    result = {}
    if path is None:
        return result
    for row in json.loads(Path(path).read_text()):
        image = row.get('image')
        if image is None:
            auxiliary = row.get('auxiliary', row.get('aux'))
            image = source.aux[auxiliary] if auxiliary is not None else source.clips[row['clip']][0]+row['frame']
        t.require(image not in result, 'Duplicate replacement image')
        result[image] = Path(row['png'])
    return result


def make_plan(source, work, selected, replacements=None, precision=2, *,
              allow_geometry=False, clip_overrides=None):
    replacements = replacements or {}
    selected = set(selected)
    t.require(set(replacements) <= selected, 'Manifest contains an unselected image')
    clip_overrides = clip_overrides or {}
    for directory, clip in clip_overrides.items():
        original = source.clips[directory]
        t.require(allow_geometry and tuple(clip[:3]) == original[:3],
                  'A replacement may not change frame IDs, counts or clocks')
        t.require(set(range(clip[0], clip[0]+clip[1])) <= selected,
                  'A geometry override must replace every frame in its clip')
        t.require(-640 <= clip[3] <= 640 and -480 <= clip[4] <= 480 and
                  0 < clip[5] < 1000000, 'Replacement registration is unbounded')
    old = t.TileReader(source)
    used = live_tiles(source, old, selected)
    np.save(work/'protected-tiles.npy', used)
    catalog = RecycledCatalog(old, used, work/'tiles.raw')
    frames, mapping, seen, ranges = [], {}, {}, []
    group = group_bytes = group_frames = total = original_bytes = 0
    try:
        with (work/'indices.raw').open('wb') as indices:
            for image in sorted(selected):
                if image in replacements:
                    picture = Image.open(replacements[image]).convert('RGBA')
                    width, height, fmt, raw = picture.width, picture.height, 4, sparse_encode(picture)
                elif source.images[image][3] == 6:
                    width, height, fmt, raw = old.image_raw(image)
                else:
                    width, height, fmt, raw = source.image_raw(image)
                t.require((allow_geometry or (width, height) == source.images[image][1:3]) and
                          0 < width <= 320 and 0 < height <= 240 and fmt == 4,
                          'Replacement geometry or sparse format differs')
                raw = quantize_image(raw, width, height, fmt, precision)
                original_bytes += len(raw)
                key = width, height, len(raw), t.sha256(raw)
                if key in seen:
                    mapping[image] = seen[key]
                    continue
                geometry, colors, alpha = t.validate_sparse(raw, width, height, pixels=True)
                ids = []
                rgb_tile = np.zeros((t.SIDE, t.SIDE), dtype='<u2')
                alpha_tile = np.zeros((t.SIDE, t.SIDE), dtype='u1')
                for y in range(0, height, t.SIDE):
                    h = min(t.SIDE, height-y)
                    for x in range(0, width, t.SIDE):
                        w = min(t.SIDE, width-x)
                        rgb_tile.fill(0); alpha_tile.fill(0)
                        rgb_tile[:h, :w] = colors[y:y+h, x:x+w]
                        alpha_tile[:h, :w] = alpha[y:y+h, x:x+w]
                        ids.append(catalog.add(bytes((w, h))+rgb_tile.tobytes()+alpha_tile.tobytes()))
                raw_ids = np.asarray(ids, dtype='<u4').tobytes()
                record = t.FRAME.pack(len(raw), len(raw_ids), len(geometry))+raw_ids+geometry
                t.require(len(record) <= t.GROUP_LIMIT, 'Character frame record exceeds group bound')
                if group_frames and (group_frames == t.INDEX_FRAMES or group_bytes+len(record) > t.GROUP_LIMIT):
                    ranges.append((total-group_bytes, group_bytes, group_frames))
                    group += 1; group_bytes = group_frames = 0
                mapping[image] = seen[key] = len(frames)
                frames.append(dict(image=image, width=width, height=height, raw_bytes=len(raw),
                                   raw_sha256=key[3], index_group=group, record_offset=group_bytes,
                                   record_bytes=len(record)))
                indices.write(record)
                group_bytes += len(record); group_frames += 1; total += len(record)
            if group_frames:
                ranges.append((total-group_bytes, group_bytes, group_frames))
        changed = sorted(catalog.changed)
        tile_ranges = [(i*old.per_group*t.TILE_BYTES,
                        min(old.per_group, catalog.count-i*old.per_group)*t.TILE_BYTES,
                        min(old.per_group, catalog.count-i*old.per_group)) for i in changed]
        plan = dict(selected=sorted(selected), mapping=mapping, frames=frames, tiles=catalog.count,
                    old_tiles=old.tile_count, per_group=old.per_group, changed_groups=changed,
                    tile_ranges=tile_ranges, index_ranges=ranges, require_smaller=False,
                    precision='454a6' if precision == 2 else 'exact', original_sparse_bytes=original_bytes,
                    dead_slots=len(catalog.free), recycled_slots=catalog.next_free,
                    reused_tile_references=catalog.reused, protected_tiles=int(np.count_nonzero(used)),
                    allow_geometry=allow_geometry, clip_overrides=clip_overrides)
        emit('character_retile_plan', images=len(selected), changed_groups=len(changed),
             new_tiles=catalog.count-old.tile_count, recycled_slots=catalog.next_free,
             reused_references=catalog.reused)
        return plan
    finally:
        catalog.close()


def verify(source, output, plan, work):
    target = view(output)
    old, new = t.TileReader(source), t.TileReader(target)
    selected = set(plan['selected'])
    expected_clips = list(source.clips)
    for directory, clip in plan.get('clip_overrides', {}).items():
        expected_clips[int(directory)] = tuple(clip)
    t.require(source.counts == target.counts and expected_clips == target.clips and
              source.cards == target.cards and source.aux == target.aux, 'Stable catalog or clip clock changed')
    used = np.load(work/'protected-tiles.npy')
    checked_tiles = 0
    for group in range(old.tile_groups):
        original = old.group(group)[0]
        rewritten = new.group(group)[0]
        first = group*old.per_group
        count = len(original)//t.TILE_BYTES
        live = used[first:first+count]
        a = np.frombuffer(original, dtype='u1').reshape(-1, t.TILE_BYTES)
        b = np.frombuffer(rewritten, dtype='u1').reshape(-1, t.TILE_BYTES)[:count]
        t.require(np.array_equal(a[live], b[live]), 'A protected static or animation tile changed')
        checked_tiles += int(np.count_nonzero(live))
    for group in range(old.index_groups):
        t.require(old.group(old.tile_groups+group)[0] == new.group(new.tile_groups+group)[0],
                  'A pre-existing index group changed')
    for image, entry in enumerate(source.images):
        if image in selected:
            frame = plan['frames'][plan['mapping'][image]]
            t.require(target.images[image][1:3] == (frame['width'], frame['height']),
                      'Replacement image dimensions differ')
            t.require(t.sha256(new.image_raw(image)[3]) == frame['raw_sha256'], 'Repaired character pixels differ')
        else:
            t.require(entry[1:3] == target.images[image][1:3], 'Protected image dimensions changed')
        if image not in selected and entry[3] == 6:
            a = t.DESCRIPTOR.unpack_from(source.data, entry[0])
            b = t.DESCRIPTOR.unpack_from(output, target.images[image][0])
            t.require(a == b, 'Protected tiled image registration changed')
        elif image not in selected:
            t.require(source.image_raw(image) == target.image_raw(image), 'Unselected retained image changed')
    for i in range(len(source.glyphs)):
        t.require(source.glyphs[i][:1]+source.glyphs[i][2:] == target.glyphs[i][:1]+target.glyphs[i][2:] and
                  source.glyph_payload(i) == target.glyph_payload(i), 'Glyph metadata or pixels changed')
    for i in range(len(source.strings)):
        t.require(source.string_payload(i) == target.string_payload(i), 'Localized text changed')
    return dict(repaired_images=len(selected), exact_protected_tiles=checked_tiles,
                existing_index_groups_exact=old.index_groups, glyphs_unchanged=len(source.glyphs),
                strings_unchanged=len(source.strings), all_frames_clocks_preserved=True,
                all_frames_dimensions_clocks_preserved=not plan.get('allow_geometry', False),
                registered_clip_layouts=len(plan.get('clip_overrides', {})))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--replacements', type=Path)
    parser.add_argument('--workers', type=int, choices=range(1, 5), default=3)
    args = parser.parse_args()
    for path in [*args.input, args.output, args.work]:t.private_path(path, 'Private character retile path')
    args.output.mkdir(parents=True, exist_ok=True);args.work.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    source = view(args.input[0].read_bytes())
    replacements = replacements_from_manifest(source, args.replacements)
    plan = make_plan(source, args.work, image_ids(source), replacements)
    encoded = compress_plan(plan, args.work, args.work/'encoded', args.workers)
    t.atomic_bytes(args.work/'plan.json', (json.dumps(plan, indent=2)+'\n').encode())
    receipts = {}
    for index, path in enumerate(args.input):
        current = source if not index else view(path.read_bytes())
        output, receipt = rewrite(current, plan, encoded)
        receipt['verification'] = verify(current, output, plan, args.work)
        receipt['input_sha256'], receipt['output_sha256'] = t.sha256(current.data), t.sha256(output)
        t.atomic_bytes(args.output/path.name, output)
        receipts[path.name] = receipt
        emit('character_retile_written', file=path.name, **receipt)
    result = dict(schema=1, complete=True, precision=plan['precision'], images=len(plan['selected']),
                  reclaimed_old_slots=plan['recycled_slots'], dead_slots=plan['dead_slots'],
                  changed_groups=len(plan['changed_groups']), new_tiles=plan['tiles']-plan['old_tiles'],
                  receipts=receipts, seconds=time.monotonic()-started)
    t.atomic_bytes(args.output/'character-retile-receipt.json', (json.dumps(result, indent=2)+'\n').encode())
    print(json.dumps(result), flush=True)


if __name__ == '__main__':
    main()
