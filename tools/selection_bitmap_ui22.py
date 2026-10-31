#!/usr/bin/env python3
"""Encode smaller exact selection copy masks in NSP2/46 index records.

Every original record offset/length, image decoded cost, tile stream, tile ID,
clock and locale payload is preserved. Only complete opaque spans with an
exact inverse are eligible. The original compressed group is always the
fallback; final release use additionally requires the complete renderer gate.
"""
from __future__ import annotations

import argparse
import json
import lzma
from pathlib import Path
import struct
import time

import coalesce_tile_resources as coalesce
import content_tiles as t
from selection_bitmap import encode_geometry, marked, restore_geometry
from tile_prediction import index_transform

FIRST_CLIP, CHARACTERS = 1705, 5


def choose_group(original, reader, group, selected):
    raw, starts = reader.group(group)
    changed = bytearray(raw); converted = []
    for at, size in starts.items():
        if at not in selected:
            continue
        _decoded, ids, geometry = t.FRAME.unpack_from(raw, at)
        begin = at + t.FRAME.size + ids
        t.require(begin + geometry == at + size, 'Selection record boundary differs')
        encoded = encode_geometry(320, 240, raw[begin:begin + geometry])
        if encoded is not None:
            t.require(len(encoded) == geometry, 'Selection bitmap changed a record offset')
            changed[begin:begin + geometry] = encoded; converted.append(at)
    if not converted:
        return original, dict(eligible_frames=0, accepted=False, saved_bytes=0)
    for at in converted:
        _decoded, ids, geometry = t.FRAME.unpack_from(raw, at)
        begin = at + t.FRAME.size + ids
        t.require(restore_geometry(320, 240, changed[begin:begin + geometry]) ==
                  raw[begin:begin + geometry], 'Selection bitmap inverse changed a span')
    header = list(t.GROUP.unpack_from(original))
    profile = coalesce.source_lzma_profile(original[32:])
    transformed = (index_transform(bytes(changed), header[6], 3, tile_count=reader.tile_count)
                   if reader.predictors[group] else bytes(changed))
    stream = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
        id=lzma.FILTER_LZMA2, preset=9, dict_size=reader.dictionary,
        nice_len=273, depth=512, lc=profile[0], lp=profile[1], pb=profile[2])])
    decoded = t.decode_lzma2(stream, len(raw), reader.dictionary)
    if reader.predictors[group]:
        decoded = index_transform(decoded, header[6], 3, True, reader.tile_count)
    t.require(decoded == changed, 'Selection bitmap compressed roundtrip differs')
    header[4] = len(stream); candidate = t.GROUP.pack(*header) + stream
    accepted = len(candidate) < len(original)
    return (candidate if accepted else original), dict(
        eligible_frames=len(converted), accepted=accepted, profile=profile,
        before_bytes=len(original), candidate_bytes=len(candidate),
        saved_bytes=max(0, len(original) - len(candidate)),
        raw_sha256=t.sha256(raw), original_group_sha256=t.sha256(original),
        candidate_group_sha256=t.sha256(candidate),
        record_offsets_lengths_and_decoded_costs_unchanged=True,
        exact_original_span_roundtrip=True)


def convert(data):
    source = coalesce.view(data)
    t.require(source.version == 46 and len(source.aux) >= 7212 and len(source.clips) >= 1710,
              'Selection bitmaps require the complete NSP2/46 UI22 profile')
    reader = t.TileReader(source); selected = {}
    for first, count, *_ in source.clips[FIRST_CLIP:FIRST_CLIP + CHARACTERS]:
        for image in range(first, first + count):
            offset, width, height, fmt, length = source.images[image]
            t.require((width, height, fmt, length) == (320, 240, 6, 16),
                      'Selection frame geometry changed')
            global_at, local, at, _size = t.DESCRIPTOR.unpack_from(data, offset)
            t.require(global_at == source.base_end and local < reader.index_groups,
                      'Invalid selection frame index reference')
            selected.setdefault(reader.tile_groups + local, set()).add(at)
    originals = {g: bytes(data[at - 32:at + size])
                 for g, (at, size, *_) in enumerate(reader.groups)}
    plan = {g: (payload, payload, payload) for g, payload in originals.items()}
    rows = []
    for group, offsets in sorted(selected.items()):
        payload, row = choose_group(originals[group], reader, group, offsets)
        physical = payload + bytes((len(originals[group]) - len(payload)) & 1)
        t.require(len(physical) <= len(originals[group]), 'Selection bitmap increased storage')
        plan[group] = originals[group], payload, physical
        rows.append(dict(group=group, **row))
    changed = [g for g, (a, b, _c) in sorted(plan.items()) if a != b]
    padding = 0
    if changed:
        padding = sum(len(a) - len(c) for a, _b, c in plan.values()) % 4096
        last = changed[-1]; a, b, c = plan[last]; plan[last] = a, b, c + bytes(padding)
    output, relocate = coalesce.rewrite(data, plan, version=source.version)
    target = coalesce.view(output); after = t.TileReader(target)
    t.require(source.counts == target.counts and source.clips == target.clips and
              source.aux == target.aux and source.cards == target.cards,
              'Selection bitmap changed a catalog or clock')
    for image, (old, new) in enumerate(zip(source.images, target.images)):
        t.require(old[1:] == new[1:], 'Selection bitmap changed image geometry/cost shape')
        if old[3] == 6:
            a = t.DESCRIPTOR.unpack_from(data, old[0])
            b = t.DESCRIPTOR.unpack_from(output, new[0])
            t.require(a == b, 'Selection bitmap changed a frame descriptor')
    for group, (original, payload, _physical) in plan.items():
        at, size, *_ = after.groups[group]
        t.require(output[at - 32:at + size] == payload, 'Published selection group differs')
        if group not in changed:
            continue
        raw, starts = reader.group(group); rebuilt, new_starts = after.group(group)
        t.require(starts == new_starts and len(raw) == len(rebuilt), 'Selection index pages changed')
        rebuilt = bytearray(rebuilt)
        for at, size in new_starts.items():
            _decoded, ids, geometry = t.FRAME.unpack_from(rebuilt, at)
            begin = at + t.FRAME.size + ids
            if marked(rebuilt[begin:begin + geometry]):
                rebuilt[begin:begin + geometry] = restore_geometry(320, 240, rebuilt[begin:begin + geometry])
        t.require(bytes(rebuilt) == raw, 'Published bitmap changed a tile ID, cost or original span')
    tail = reader.groups[-1][0] + reader.groups[-1][1]
    t.require(output[relocate(tail):] == data[tail:], 'Retained/font/text tail changed')
    return output, dict(complete=True, version=46, source_sha256=t.sha256(data),
        output_sha256=t.sha256(output), source_bytes=len(data), output_bytes=len(output),
        saved_bytes=len(data) - len(output), source_groups=len(selected), changed_groups=changed,
        converted_frames=sum(r['eligible_frames'] for r in rows if r['accepted']),
        original_stream_is_fallback=True, suffix_alignment_padding=padding, groups=rows,
        exact_original_geometry_and_active_tiles=True, all_pixels_clocks_and_locales_unchanged=True,
        image_and_index_cache_pages_unchanged=True, additional_resident_bytes=0,
        release_whole_frame_cpu_gate_required=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('input', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    t.private_path(args.input, 'Private original selection resources')
    t.private_path(args.output, 'Private bitmap candidate resources')
    t.require(args.input.resolve() != args.output.resolve(), 'Never overwrite original resources')
    started = time.monotonic(); output, report = convert(args.input.read_bytes())
    report['seconds'] = time.monotonic() - started
    t.atomic_bytes(args.output, output)
    t.atomic_bytes(args.output.with_suffix('.json'), json.dumps(report, indent=2).encode() + b'\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'groups'}), flush=True)


if __name__ == '__main__':
    main()
