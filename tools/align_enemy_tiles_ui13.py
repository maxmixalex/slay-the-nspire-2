#!/usr/bin/env python3
"""Losslessly align independently registered enemy canvases to sparse tiles.

Only transparent padding and an equal pivot translation are permitted. Every
source RGB565/alpha sample, inverse density, frame id/count and clock is kept.
The existing codec and runtime remain unchanged. Integer screen rounding at
non-native scales may differ by a boundary pixel; world-sample identity is
independently verified for every frame. Private original packs stay private.
"""
from __future__ import annotations

import argparse
from collections import defaultdict, OrderedDict
import hashlib
import json
from pathlib import Path
import struct
import time

import numpy as np

import content_tiles as t
from retile_appended_resources import view, emit, compress_plan, rewrite
from retile_character_ui12 import make_plan, verify


def padding_layouts(source, samples):
    """Share one tile phase per model/density, minimizing transparent area."""
    groups = defaultdict(list)
    for sample in samples:
        directory = sample['clip'];clip = source.clips[directory]
        first, count, _, px, py, units = clip
        width, height = source.images[first][1:3]
        t.require(units > 0, 'Only explicitly registered source clips may be aligned')
        t.require(all(source.images[i][1:3] == (width, height) for i in range(first, first+count)),
                  'An aligned clip must share one canvas')
        groups[(sample['key'].split('.')[0], units)].append(
            dict(clip=directory, key=sample['key'], frames=count, width=width, height=height, px=px, py=py))
    result = {}
    for (model, units), rows in groups.items():
        def phase(axis):
            pivot, edge, limit, cross = ('px', 'width', 320, 'height') if axis == 0 else ('py', 'height', 240, 'width')
            def score(value):
                possible = [row for row in rows if row[edge]+(value-row[pivot]) % 16 <= limit]
                return (sum(row['frames'] for row in possible),
                        -sum(row['frames']*((value-row[pivot]) % 16)*row[cross] for row in possible), -value)
            return max(range(16), key=score)
        phase_x, phase_y = phase(0), phase(1)
        for row in rows:
            dx, dy = (phase_x-row['px']) % 16, (phase_y-row['py']) % 16
            if row['width']+dx > 320:dx = 0
            if row['height']+dy > 240:dy = 0
            width = ((row['width']+dx+15)//16)*16
            height = ((row['height']+dy+15)//16)*16
            t.require(width <= 320 and height <= 240 and
                      -640 <= row['px']+dx <= 640 and -480 <= row['py']+dy <= 480,
                      'Aligned canvas/pivot exceeds existing runtime bounds')
            result[row['clip']] = dict(**row, model=model, sprite_units=units,
                phase=[phase_x, phase_y], offset=[dx, dy], size=[width, height])
    return result


def padded_sparse(raw, width, height, dx, dy, new_width, new_height):
    """Translate sparse run headers only; never decode/requantize the pixels."""
    t.require(0 <= dx and 0 <= dy and width+dx <= new_width <= 320 and
              height+dy <= new_height <= 240, 'Padding may not crop or rescale a frame')
    result = bytearray(2*dy);at = 0
    for _ in range(height):
        count, = struct.unpack_from('<H', raw, at);result.extend(raw[at:at+2]);at += 2
        for _ in range(count):
            x, length, mode = struct.unpack_from('<HHB', raw, at);at += 5
            t.require(x+length <= width and mode in (0, 1), 'Invalid source sparse span')
            result.extend(struct.pack('<HHB', x+dx, length, mode))
            size = length*(2+mode);result.extend(raw[at:at+size]);at += size
    t.require(at == len(raw), 'Sparse padding left unparsed data')
    result.extend(bytes(2*(new_height-height-dy)))
    return new_width, new_height, 4, bytes(result)


def world_pixel_signature(raw, height, pivot_x, pivot_y, units):
    """Hash source-world run coordinates and exact already-decoded samples."""
    digest = hashlib.sha256(struct.pack('<I', units));at = 0
    for y in range(height):
        count, = struct.unpack_from('<H', raw, at);at += 2
        for _ in range(count):
            x, length, mode = struct.unpack_from('<HHB', raw, at);at += 5
            digest.update(struct.pack('<iiIB', x-pivot_x, pivot_y-y, length, mode))
            size = length*(2+mode);digest.update(raw[at:at+size]);at += size
    t.require(at == len(raw), 'World comparison left unparsed data')
    return digest.hexdigest()


def image_raw(source, reader, image):
    return reader.image_raw(image) if source.images[image][3] == 6 else source.image_raw(image)


def verify_world_samples(before, after, layouts):
    old, new = t.TileReader(before), t.TileReader(after);checked = 0;digest = hashlib.sha256()
    for directory, layout in layouts.items():
        a, b = before.clips[directory], after.clips[directory]
        t.require(a[:3] == b[:3] and a[5] == b[5], 'Alignment changed animation or inverse density')
        dx, dy = layout['offset']
        t.require((b[3]-a[3], b[4]-a[4]) == (dx, dy), 'Pivot differs from transparent padding')
        for image in range(a[0], a[0]+a[1]):
            wa, ha, fa, ra = image_raw(before, old, image)
            wb, hb, fb, rb = image_raw(after, new, image)
            t.require(fa == fb == 4 and [wb, hb] == layout['size'], 'Aligned frame format/size differs')
            t.require(rb == padded_sparse(ra, wa, ha, dx, dy, wb, hb)[3],
                      'A source sample or sparse span was altered')
            identity = world_pixel_signature(ra, ha, a[3], a[4], a[5])
            t.require(identity == world_pixel_signature(rb, hb, b[3], b[4], b[5]),
                      'A frame moved or changed pixels in source-world coordinates')
            digest.update(struct.pack('<I', image)+bytes.fromhex(identity));checked += 1
    return dict(world_frames_exact=checked, world_pixels_sha256=digest.hexdigest(),
                source_density_unchanged=True, original_sample_dimensions_unchanged=True)


def clip_working_sets(source, layouts):
    """Offline catalog costs, not elapsed-time or physical-device estimates."""
    reader = t.TileReader(source);result = []
    for directory, layout in layouts.items():
        clip = source.clips[directory];rows = [];union = set();raw_sizes = [];frame_keys = {}
        for image in range(clip[0], clip[0]+clip[1]):
            offset, width, height, fmt, length = source.images[image]
            t.require(fmt == 6 and length == 16, 'Expected a tiled aligned frame')
            _, index, at, size = t.DESCRIPTOR.unpack_from(source.data, offset)
            group = reader.tile_groups+index;raw = reader.group(group)[0]
            sparse_size, index_bytes, _ = t.FRAME.unpack_from(raw, at)
            ids = np.frombuffer(raw, dtype='<u4', count=index_bytes//4, offset=at+t.FRAME.size)
            needed = [group]+sorted(set(int(tile)//reader.per_group for tile in ids))
            rows.append(needed);union.update(needed);raw_sizes.append(sparse_size)
            frame_keys[source.images[image]] = (sparse_size+4095)//4096
        def lru_cost(budget):
            cache = OrderedDict();resident = 0;passes = []
            for repeat in range(3):
                reads = total = 0
                for groups in rows:
                    # Runtime revisits cached groups before evicting any for
                    # new ones. Model that order and its bounded LRU budget.
                    order = [g for g in groups if g in cache]+[g for g in groups if g not in cache]
                    for g in order:
                        if g in cache:cache.move_to_end(g);continue
                        stored, raw_bytes = reader.groups[g][1:3]
                        while cache and resident+raw_bytes > budget:
                            _, evicted = cache.popitem(last=False);resident -= evicted
                        if raw_bytes <= budget:cache[g] = raw_bytes;resident += raw_bytes
                        reads += 1;total += stored
                passes.append(dict(group_reads=reads, encoded_bytes=total))
            return passes
        result.append(dict(clip=directory, key=layout['key'], frames=clip[1], frame_ms=clip[2],
            raw_frame_bytes=dict(min=min(raw_sizes), max=max(raw_sizes), mean=round(sum(raw_sizes)/len(raw_sizes), 2),
                                 total=sum(raw_sizes)),
            unique_decoded_frames=len(frame_keys), decoded_frame_pages=sum(frame_keys.values()),
            loop_fits_5mib_image_pool=len(frame_keys) <= 480 and sum(frame_keys.values()) <= 1280,
            unique_groups=len(union), raw_group_working_set=sum(reader.groups[g][2] for g in union),
            encoded_group_working_set=sum(reader.groups[g][1] for g in union),
            groups_per_frame=dict(min=min(map(len, rows)), max=max(map(len, rows))),
            group_only_lru_6mib=lru_cost(6*1024*1024),
            group_only_lru_1_5mib=lru_cost(1536*1024)))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, action='append', required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=range(1, 5), default=3)
    args = parser.parse_args()
    for path in [*args.input, args.receipt, args.output, args.work]:t.private_path(path, 'Private enemy tile alignment path')
    t.require(all(path.resolve().parent != args.output.resolve() for path in args.input), 'Keep original complete packs')
    args.output.mkdir(parents=True, exist_ok=True);args.work.mkdir(parents=True, exist_ok=True)
    started = time.monotonic();source = view(args.input[0].read_bytes())
    samples = json.loads(args.receipt.read_text())['source_samples']
    layouts = padding_layouts(source, samples);translations = {};clips = {}
    for directory, layout in layouts.items():
        clip = source.clips[directory];dx, dy = layout['offset']
        clips[directory] = (*clip[:3], clip[3]+dx, clip[4]+dy, clip[5])
        for image in range(clip[0], clip[0]+clip[1]):translations[image] = layout
    def transform(image, row):
        width, height, fmt, raw = row;layout = translations[image]
        t.require(fmt == 4 and (width, height) == (layout['width'], layout['height']), 'Source frame differs from clip canvas')
        return padded_sparse(raw, width, height, *layout['offset'], *layout['size'])
    t.atomic_bytes(args.work/'alignment-layouts.json', json.dumps(layouts, indent=2).encode()+b'\n')
    plan = make_plan(source, args.work, translations, precision=8, allow_geometry=True,
                     clip_overrides=clips, raw_transform=transform)
    t.atomic_bytes(args.work/'plan.json', json.dumps(plan, indent=2).encode()+b'\n')
    encoded = compress_plan(plan, args.work, args.work/'encoded', args.workers)
    reports = [];before_costs = clip_working_sets(source, layouts);after_costs = None
    for path in args.input:
        original = source if path == args.input[0] else view(path.read_bytes())
        output, sizes = rewrite(original, plan, encoded)
        checked = verify(original, output, plan, args.work);target = view(output)
        exact = verify_world_samples(original, target, layouts)
        if after_costs is None:after_costs = clip_working_sets(target, layouts)
        destination = args.output/path.name;t.atomic_bytes(destination, output)
        reports.append(dict(path=str(destination), sha256=t.sha256(output), **sizes, **checked, **exact))
        emit('aligned_enemy_pack_verified', **reports[-1])
    result = dict(clips=len(layouts), frames=len(translations), elapsed_seconds=round(time.monotonic()-started, 3),
        codec_unchanged=True, geometry='transparent padding with equal pivot translation; no resampling',
        screen_rounding='non-native nearest-neighbor boundaries can differ by one pixel',
        outputs=reports, before_working_sets=before_costs, after_working_sets=after_costs)
    t.atomic_bytes(args.output/'enemy-alignment-receipt.json', json.dumps(result, indent=2).encode()+b'\n')
    emit('enemy_alignment_complete', frames=len(translations), elapsed_seconds=result['elapsed_seconds'])


if __name__ == '__main__':main()
