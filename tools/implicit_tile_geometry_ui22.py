#!/usr/bin/env python3
"""Replace reproducible sparse layout with exact alpha-derived NSP2/46 geometry.

Pixels, palette precision, frame clocks, descriptor offsets, index raw lengths
and decoded cache costs remain unchanged. Only a geometry region whose complete
original bytes are reproduced by the historical sparse segmentation is eligible.
Selection patches and unknown/noncanonical records retain literal geometry.
The resulting runtime must pass the independent pre-conversion byte oracle and
whole-frame cold/warm release gates before packaging.
"""
from __future__ import annotations

import argparse
from collections import deque
from concurrent.futures import ThreadPoolExecutor
import json
import lzma
from pathlib import Path
import struct
import time

import numpy as np

import coalesce_tile_resources as coalesce
import content_tiles as t
import implicit_geometry as implicit
import selection_bitmap
from compact_tile_resources import grouped_slices, SLICE
from tile_prediction import index_transform

ENCODER = b'ui22-implicit-alpha-v1-same-records-same-profile-nice273-depth512'
SELECTION_CLIPS = range(1705, 1710)


def record_shapes(source, reader, extra_protected=()):
    protected = set(extra_protected)
    t.require(all(type(i) is int and 0 <= i < len(source.images) for i in protected),
              'Protected image is outside the catalog')
    for clip in SELECTION_CLIPS:
        if clip < len(source.clips):
            first, count, *_ = source.clips[clip]
            protected.update(range(first, first + count))
    shapes = {}
    for image, (off, width, height, fmt, _length) in enumerate(source.images):
        if fmt != 6:
            continue
        base, local, at, length = t.DESCRIPTOR.unpack_from(source.data, off)
        t.require(base == source.base_end and local < reader.index_groups,
                  'Image descriptor names a foreign tile directory')
        key = reader.tile_groups + local, at
        if key not in shapes:
            shapes[key] = dict(width=width, height=height, length=length, images=[], protected=False)
        row = shapes[key]
        t.require((row['width'], row['height'], row['length']) == (width, height, length),
                  'Aliased frame changes dimensions or record length')
        row['images'].append(image)
        row['protected'] |= image in protected
    return shapes, protected


def encode_group(original, raw, dictionary, tile_count, predicted, cache):
    """Preserve the current mode/profile and verify cached bytes before use."""
    header = list(t.GROUP.unpack_from(original))
    transformed = index_transform(raw, header[6], 3, tile_count=tile_count) if predicted else raw
    if predicted:
        t.require(index_transform(transformed, header[6], 3, True, tile_count) == raw,
                  'Index prediction changed implicit record bytes')
    profile = coalesce.source_lzma_profile(original[32:])
    key = t.sha256(ENCODER + struct.pack('<I', dictionary) + bytes(profile) + transformed)
    path = cache / (key + '.lzma2')
    if path.is_file():
        encoded = path.read_bytes()
    else:
        lc, lp, pb = profile
        encoded = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
            id=lzma.FILTER_LZMA2, preset=9, dict_size=dictionary, nice_len=273,
            depth=512, lc=lc, lp=lp, pb=pb)])
    t.require(t.decode_lzma2(encoded, len(transformed), dictionary) == transformed,
              'Implicit geometry compressed cache changed exact record bytes')
    if not path.is_file():
        t.atomic_bytes(path, encoded)
    header[4] = len(encoded)
    payload = t.GROUP.pack(*header) + encoded
    physical = payload + bytes((len(original) - len(payload)) & 1)
    if len(physical) >= len(original):
        payload = physical = original
    return payload, physical, profile


def preserved_directory(source, target, old, new, relocate, plan):
    """Verify untouched payloads and all relocated fields without full raw RAM."""
    a, b = source.data, target.data
    t.require(source.counts == target.counts and source.clips == target.clips and
              source.cards == target.cards and source.aux == target.aux,
              'Implicit geometry changed a catalog entry or animation clock')
    t.require((old.tile_count, old.tile_groups, old.index_groups, old.frame_count, old.per_group) ==
              (new.tile_count, new.tile_groups, new.index_groups, new.frame_count, new.per_group),
              'Implicit geometry changed tile groups or descriptor count')
    for group, (before, after) in enumerate(zip(old.groups, new.groups)):
        at, size, raw, crc, kind, count = before
        bt, bs, br, bc, bk, bn = after
        t.require((raw, crc, kind, count) == (br, bc, bk, bn) and
                  bt == relocate(at - 32) + 32 and b[bt - 32:bt + bs] == plan[group][1],
                  'Published group differs from its exact encoded plan')
        if group < old.tile_groups:
            t.require(a[at - 32:at + size] == b[bt - 32:bt + bs], 'Pixel stream changed')
    for before, after in zip(source.images, target.images):
        off, _width, _height, fmt, length = before
        t.require(after == (relocate(off), *before[1:]), 'Image identity, format or dimensions changed')
        if fmt < 6:
            t.require(a[off:off + length] == b[after[0]:after[0] + length], 'Literal image changed')
    for frame in range(old.frame_count):
        off = old.descriptors + frame * 16
        base, local, at, length = t.DESCRIPTOR.unpack_from(a, off)
        t.require(t.DESCRIPTOR.unpack_from(b, relocate(off)) == (relocate(base), local, at, length),
                  'Descriptor changed index offset, group or raw record length')
    checked = set()
    for off, (relative, length, raw_at, shape) in grouped_slices(source).items():
        at = off + relative
        t.require(SLICE.unpack_from(b, relocate(off)) ==
                  (relocate(at) - relocate(off), length, raw_at, shape), 'Retained descriptor changed')
        if at not in checked:
            t.require(a[at:at + length] == b[relocate(at):relocate(at) + length],
                      'Retained image/font/text compressed bytes changed')
            checked.add(at)
    for i, (before, after) in enumerate(zip(source.glyphs, target.glyphs)):
        t.require(after == (before[0], relocate(before[1]), *before[2:]) and
                  source.glyph_payload(i) == target.glyph_payload(i), 'Glyph metrics or mask changed')
    for i, (before, after) in enumerate(zip(source.strings, target.strings)):
        t.require(after == (relocate(before[0]), before[1]) and
                  source.string_payload(i) == target.string_payload(i), 'Text changed')
    ac = t.GLOBAL.unpack_from(a, source.base_end)[14]
    bc = t.GLOBAL.unpack_from(b, target.base_end)[14]
    t.require(a[ac:ac + len(source.images)] == b[bc:bc + len(target.images)], 'Image cache-page costs changed')


def convert_bytes(data, cache, *, workers=2, groups=None, protected_images=(), align_suffix=True):
    t.require(1 <= workers <= 2, 'Use at most two bounded encoding workers')
    source = coalesce.view(data)
    t.require(source.version in (45, implicit.VERSION), 'Implicit geometry expects NSP2/45 or NSP2/46')
    old = t.TileReader(source)
    cache = Path(cache); cache.mkdir(parents=True, exist_ok=True)
    shapes, protected = record_shapes(source, old, protected_images)
    wanted = set(range(old.tile_groups, len(old.groups))) if groups is None else set(groups)
    t.require(all(old.tile_groups <= g < len(old.groups) for g in wanted), 'Expected existing index-group IDs')
    originals = {g: bytes(data[at - 32:at + size]) for g, (at, size, *_) in enumerate(old.groups)}
    plan = {g: (raw, raw, raw) for g, raw in originals.items()}
    reports = []
    pending = deque()

    def drain():
        group, records, summary, future = pending.popleft()
        payload, physical, profile = future.result()
        accepted = payload != originals[group]
        plan[group] = originals[group], payload, physical
        summary.update(accepted=accepted, source_profile=profile, source_bytes=len(originals[group]),
            output_bytes=len(payload), physical_bytes=len(physical), saved_bytes=len(originals[group]) - len(physical),
            source_group_sha256=t.sha256(originals[group]), output_group_sha256=t.sha256(payload),
            converted_records=records if accepted else [])
        reports.append(summary)

    with ThreadPoolExecutor(max_workers=workers) as pool:
        for ordinal, group in enumerate(sorted(wanted)):
            original, starts = old.group(group)
            candidate = bytearray(original)
            records = []
            summary = dict(group=group, physical_frames=len(starts), protected_frames=0,
                unknown_frames=0, insufficient_area_frames=0, noncanonical_frames=0, already_implicit_frames=0,
                replaced_geometry_bytes=0, bitmap_bytes=0, original_raw_sha256=t.sha256(original))
            for at, length in starts.items():
                shape = shapes.get((group, at))
                if shape is None:
                    summary['unknown_frames'] += 1; continue
                t.require(shape['length'] == length, 'Image descriptor does not name a full index record')
                size, id_bytes, geometry_size = t.FRAME.unpack_from(original, at)
                width, height = shape['width'], shape['height']
                t.require(id_bytes == implicit.cells(width, height) * 4, 'Image tile grid differs')
                geometry_at = at + t.FRAME.size + id_bytes
                geometry = original[geometry_at:at + length]
                if shape['protected']:
                    summary['protected_frames'] += 1; continue
                if implicit.marked(geometry):
                    t.require(source.version >= implicit.VERSION, 'Implicit marker in an old-version pack')
                    implicit.active_mask(geometry, width, height)
                    summary['already_implicit_frames'] += 1; continue
                if selection_bitmap.marked(geometry):
                    t.require(source.version >= implicit.VERSION, 'Selection bitmap in an old-version pack')
                    selection_bitmap.restore_geometry(width, height, geometry)
                    summary['already_implicit_frames'] += 1; continue
                mask_bytes = (id_bytes // 4 + 7) // 8
                if geometry_size < 2 + mask_bytes:
                    summary['insufficient_area_frames'] += 1; continue
                active = coalesce.active_tiles(geometry, width, height)
                ids = np.frombuffer(original, dtype='<u4', count=id_bytes // 4, offset=at + t.FRAME.size)
                colors, alpha = old.tile_planes(ids, width, height, active)
                restored = implicit.alpha_geometry(alpha)
                if restored != geometry:
                    summary['noncanonical_frames'] += 1; continue
                # Validate the original decoded-byte bound as well as geometry;
                # hidden RGB in any explicit alpha-zero span is not canonical
                # and was rejected by the byte comparison above.
                sparse = implicit.sparse_bytes(geometry, colors, alpha, size)
                replacement = implicit.encode_mask(active, width, height, geometry_size)
                t.require(np.array_equal(implicit.active_mask(replacement, width, height), active),
                          'Stored mask changes the original active tile set')
                candidate[geometry_at:at + length] = replacement
                records.append(dict(offset=at, length=length, images=shape['images'],
                    decoded_bytes=size, sparse_sha256=t.sha256(sparse), geometry_sha256=t.sha256(geometry),
                    active_tiles=int(np.count_nonzero(active))))
                summary['replaced_geometry_bytes'] += geometry_size
                summary['bitmap_bytes'] += 2 + mask_bytes
            t.require(len(candidate) == len(original) and
                      t.index_records(candidate, len(starts), old.tile_count) == starts,
                      'Implicit conversion changed index record boundaries')
            summary['candidate_raw_sha256'] = t.sha256(candidate)
            if records:
                future = pool.submit(encode_group, originals[group], bytes(candidate), old.dictionary,
                                     old.tile_count, old.predictors[group], cache)
                pending.append((group, records, summary, future))
            else:
                summary.update(accepted=False, source_bytes=len(originals[group]), output_bytes=len(originals[group]),
                    physical_bytes=len(originals[group]), saved_bytes=0, converted_records=[])
                reports.append(summary)
            if len(pending) >= workers:
                drain()
            if (ordinal + 1) % 32 == 0 or ordinal + 1 == len(wanted):
                print(json.dumps(dict(stage='implicit_geometry', groups=ordinal + 1, total=len(wanted),
                    converted_records=sum(len(r['converted_records']) for r in reports),
                    saved_bytes=sum(r['saved_bytes'] for r in reports))), flush=True)
        while pending:
            drain()
    reports.sort(key=lambda row: row['group'])
    changed = [g for g, (a, b, _c) in sorted(plan.items()) if a != b]
    padding = 0
    if changed and align_suffix:
        padding = sum(len(a) - len(c) for a, _b, c in plan.values()) % 4096
        a, b, c = plan[changed[-1]]; plan[changed[-1]] = a, b, c + bytes(padding)
    output, relocate = coalesce.rewrite(data, plan, version=implicit.VERSION)
    target = coalesce.view(output); new = t.TileReader(target)
    preserved_directory(source, target, old, new, relocate, plan)
    for row in reports:
        if row['accepted']:
            t.require(t.sha256(new.group(row['group'])[0]) == row['candidate_raw_sha256'],
                      'Published index differs from verified decoded candidate')
    proof = dict(schema=1, complete=True, full_catalog=groups is None, format='NSP2/46 alpha-derived sparse geometry',
        input_sha256=t.sha256(data), output_sha256=t.sha256(output), input_bytes=len(data), output_bytes=len(output),
        saved_bytes=len(data) - len(output), images=len(source.images), auxiliary_count=source.counts[-1],
        protected_images=len(protected), index_groups_considered=len(wanted), index_groups_changed=len(changed),
        converted_records=sum(len(r['converted_records']) for r in reports), suffix_alignment_padding=padding,
        exact_original_geometry_before_conversion=True, exact_original_active_tile_masks=True,
        all_pixel_streams_byte_identical=True, all_other_payloads_byte_identical=True,
        index_record_offsets_and_lengths_unchanged=True, decoded_shapes_pages_and_clocks_unchanged=True,
        source_profiles_and_group_modes_preserved=True, resident_dense_scratch_bytes=320 * 240 * 3,
        independent_old_format_byte_oracle_required=True, runtime_cold_warm_gate_required=True, groups=reports)
    return output, proof


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--groups', nargs='+', type=int)
    args = parser.parse_args()
    source = t.private_path(args.input, 'Private source pack')
    target = t.private_path(args.output, 'Private converted pack')
    work = t.private_path(args.work, 'Private encoded cache')
    t.protect_paths(source, target, work, target.with_suffix('.json'))
    t.HOST_CACHE_GROUPS = 32
    started = time.monotonic()
    output, proof = convert_bytes(source.read_bytes(), work / 'encoded', workers=args.workers, groups=args.groups)
    t.atomic_bytes(target, output)
    proof.update(input=str(source), output=str(target), elapsed_seconds=time.monotonic() - started,
                 cache_groups=32, workers=args.workers)
    t.atomic_bytes(target.with_suffix('.json'), json.dumps(proof, indent=2).encode() + b'\n')
    print(json.dumps({k: v for k, v in proof.items() if k != 'groups'}), flush=True)


if __name__ == '__main__':
    main()
