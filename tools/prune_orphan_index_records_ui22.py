#!/usr/bin/env python3
"""Remove only index records with no image descriptor, behind an ARM gate.

All live record bytes, tile IDs, image IDs, pixels, geometry and clocks stay
exact. Group IDs and descriptor counts stay fixed; index raw sizes may only
shrink. Each affected group keeps its original stream unless both encoded
size and complete linked LZMA2 plus index inverse work do not regress.
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
from compact_tile_resources import grouped_slices
from select_index_streams_ui22 import aligned_plan
from select_pixel_streams_ui22 import ArmTiles
from tile_prediction import index_transform


class ArmIndices(ArmTiles):
    """Measure the release ELF on both sides, including its index inverse."""
    def __init__(self, path):
        super().__init__(path)
        from arm_division_regression import linked_helper
        self.symbols['index'] = linked_helper(self.runtime.data,
            b'_ZN5spire24restore_index_predictionEPhjjj')[1][0]

    def index(self, payload, raw, count, predicted, dictionary, tile_count):
        e, state = self.emulator, self.runtime
        size = len(raw); target = state.destination + 32
        transformed = index_transform(raw, count, 3, tile_count=tile_count) if predicted else raw
        t.require(dictionary in (512*1024, 1024*1024), 'Unsupported ARM dictionary')
        e.mem_write(state.destination, b'\xb7'*(size+64)); e.mem_write(self.input, payload)
        work = self.call('lzma', (target, size, size, self.input, len(payload),
                                 16 if dictionary == 1024*1024 else 14))
        t.require(bytes(e.mem_read(target, size)) == transformed, 'Linked index LZMA2 differs')
        inverse = self.call('index', (target, size, count, tile_count, 0, 0)) if predicted else 0
        t.require(bytes(e.mem_read(target, size)) == raw and
                  bytes(e.mem_read(state.destination, 32)) == b'\xb7'*32 and
                  bytes(e.mem_read(target+size, 32)) == b'\xb7'*32,
                  'Linked index decoder changed raw bytes or guards')
        return dict(lzma_instructions=work, inverse_instructions=inverse,
                    total_instructions=work+inverse, stored_bytes=len(payload),
                    decoded_bytes=size, restored_sha256=t.sha256(raw),
                    exact_records_guards_aapcs=True)


def references(source, reader):
    """Every image, including aliases and unused game routes, is a root."""
    groups = {i: {} for i in range(reader.index_groups)}; descriptors = set()
    for offset, width, height, fmt, _length in source.images:
        if fmt != 6:
            continue
        t.require(offset >= reader.descriptors and (offset-reader.descriptors) % 16 == 0 and
                  (offset-reader.descriptors)//16 < reader.frame_count,
                  'Image descriptor is outside the directory')
        global_at, local, at, length = t.DESCRIPTOR.unpack_from(source.data, offset)
        t.require(global_at == source.base_end and local < reader.index_groups,
                  'Image descriptor refers outside the index directory')
        shape = length, width, height
        t.require(at not in groups[local] or groups[local][at] == shape,
                  'Aliases disagree about live record dimensions')
        groups[local][at] = shape; descriptors.add(offset)
    t.require(len(descriptors) == reader.frame_count and all(groups.values()),
              'Prune whole dead descriptors/groups before removing partial records')
    return groups


def compact_records(raw, count, tile_count, live):
    starts = t.index_records(raw, count, tile_count)
    t.require(set(live) <= set(starts), 'A descriptor points inside/misses an index record')
    output = bytearray(); mapping = {}
    for at, length in starts.items():
        if at not in live:
            continue
        expected, width, height = live[at]
        _, indices, _ = t.FRAME.unpack_from(raw, at)
        t.require(expected == length and indices//4 == ((width+15)//16)*((height+15)//16),
                  'A live record has a different length or tile grid')
        mapping[at] = len(output); output.extend(raw[at:at+length])
    t.require(bool(output), 'A whole unused index group requires directory pruning')
    return bytes(output), mapping, len(starts)-len(mapping)


def encode(raw, count, original, reader, group):
    header = list(t.GROUP.unpack_from(original)); predicted = reader.predictors[group]
    transformed = index_transform(raw, count, 3, tile_count=reader.tile_count) if predicted else raw
    lc, lp, pb = coalesce.source_lzma_profile(original[32:])
    payload = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
        id=lzma.FILTER_LZMA2, preset=9, dict_size=reader.dictionary,
        nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
    decoded = t.decode_lzma2(payload, len(raw), reader.dictionary)
    restored = index_transform(decoded, count, 3, True, reader.tile_count) if predicted else decoded
    t.require(restored == raw, 'Re-encoding changed a live index record')
    header[3], header[4], header[5], header[6] = len(raw), len(payload), 0, count
    return t.GROUP.pack(*header)+payload, (lc, lp, pb)


def checked_measurement(row, payload, raw):
    t.require(isinstance(row, dict) and row.get('exact_records_guards_aapcs') is True and
              row.get('restored_sha256') == t.sha256(raw) and
              row.get('stored_bytes') == len(payload) and row.get('decoded_bytes') == len(raw),
              'ARM result is not bound to the complete group bytes')
    t.require(all(type(row.get(k)) is int and row[k] >= 0 for k in
                  ('lzma_instructions', 'inverse_instructions', 'total_instructions')) and
              row['lzma_instructions'] > 0 and row['total_instructions'] ==
              row['lzma_instructions']+row['inverse_instructions'], 'Invalid ARM instruction totals')
    return row


def verify(data, output, plan, mappings):
    source, target = coalesce.view(data), coalesce.view(output)
    old, new = t.TileReader(source), t.TileReader(target)
    t.require(source.version == target.version and source.counts == target.counts and
              source.clips == target.clips and source.cards == target.cards and source.aux == target.aux,
              'Index pruning changed the content catalog or clocks')
    t.require((old.tile_count, old.tile_groups, old.index_groups, old.frame_count) ==
              (new.tile_count, new.tile_groups, new.index_groups, new.frame_count),
              'Index pruning changed a group ID or descriptor count')
    live = references(source, old); compared = records = 0
    for group, (before, after) in enumerate(zip(old.groups, new.groups)):
        a, n, *_ = before; b, m, *_ = after
        t.require(output[b-32:b+m] == plan[group][1], 'Output stream differs from accepted plan')
        if group not in mappings:
            t.require(data[a-32:a+n] == output[b-32:b+m], 'An unchanged stream differs')
            continue
        local = group-old.tile_groups
        original = old.group(group)[0]; actual = new.group(group)[0]
        expected, mapping, removed = compact_records(original, before[5], old.tile_count, live[local])
        t.require(removed > 0 and actual == expected and mapping == mappings[group] and
                  new.groups[group][5] == len(mapping), 'Removed record or raw offset proof differs')
        compared += len(actual); records += len(mapping)
    for a, b in zip(source.images, target.images):
        t.require(a[1:] == b[1:], 'Image dimensions or format changed')
        if a[3] == 6:
            before = t.DESCRIPTOR.unpack_from(data, a[0]); after = t.DESCRIPTOR.unpack_from(output, b[0])
            mapping = mappings.get(old.tile_groups+before[1])
            t.require(after == (target.base_end, before[1],
                      mapping[before[2]] if mapping is not None else before[2], before[3]),
                      'Live image points to a different index record')
        elif a[3] != 7:
            t.require(source.block(a[0], a[4]) == target.block(b[0], b[4]), 'A retained image changed')
    # Coalesce's relocation updates all slices, including image/font/text aliases.
    source_slices, target_slices = grouped_slices(source), grouped_slices(target)
    t.require(len(source_slices) == len(target_slices), 'Grouped slice count changed')
    for (a, av), (b, bv) in zip(sorted(source_slices.items()), sorted(target_slices.items())):
        ar, size, raw_at, shape = av; br, nsize, nraw, nshape = bv
        t.require((size, raw_at, shape) == (nsize, nraw, nshape) and
                  source.block(a+ar, size) == target.block(b+br, nsize), 'A grouped retained/font/text stream changed')
    for a, b in zip(source.glyphs, target.glyphs):
        t.require(a[:1]+a[2:] == b[:1]+b[2:], 'Glyph metrics or encoding changed')
        if not a[6] & t.GLYPH_GROUP_FLAG:
            size = a[6] or a[3]*a[4]
            t.require(source.block(a[1], size) == target.block(b[1], size), 'Literal glyph changed')
    for a, b in zip(source.strings, target.strings):
        t.require(a[1] == b[1], 'Text length/encoding changed')
        if not a[1] & t.STRING_GROUP_FLAG:
            t.require(source.block(a[0], a[1]+1) == target.block(b[0], b[1]+1), 'Literal text changed')
    old_cost = t.GLOBAL.unpack_from(data, source.base_end)[14]
    new_cost = t.GLOBAL.unpack_from(output, target.base_end)[14]
    t.require(source.block(old_cost, len(source.images)) == target.block(new_cost, len(target.images)),
              'Per-image cache costs changed')
    return dict(live_changed_records_exact=records, live_changed_raw_bytes_exact=compared,
                image_descriptors_exact=len(source.images), retained_font_text_streams_exact=True,
                image_and_index_group_ids_unchanged=True, pixel_streams_exact=old.tile_groups,
                image_pages_unchanged=True, index_raw_pages_never_increase=True)


def convert_bytes(data, arm, *, align_suffix=True, progress=None):
    t.require(arm is not None, 'A complete linked ARM measurement is required')
    source = coalesce.view(data); reader = t.TileReader(source); live = references(source, reader)
    plan = {}; mappings = {}; rows = []; orphan_count = 0
    for group, (offset, size, _raw_size, _crc, _kind, count) in enumerate(reader.groups):
        original = bytes(data[offset-32:offset+size]); plan[group] = (original, original, original)
        if group < reader.tile_groups:
            continue
        old_raw = reader.group(group)[0]
        raw, mapping, removed = compact_records(old_raw, count, reader.tile_count, live[group-reader.tile_groups])
        if not removed:
            continue
        orphan_count += removed; candidate, profile = encode(raw, len(mapping), original, reader, group)
        physical = candidate + bytes((len(original)-len(candidate)) & 1)
        row = dict(group=group, orphan_records=removed, live_records=len(mapping),
                   old_raw_bytes=len(old_raw), new_raw_bytes=len(raw), profile=profile,
                   original_sha256=t.sha256(original), candidate_sha256=t.sha256(candidate),
                   original_raw_sha256=t.sha256(old_raw), candidate_raw_sha256=t.sha256(raw),
                   original_bytes=len(original), candidate_bytes=len(candidate), accepted=False)
        if len(physical) < len(original):
            predicted = reader.predictors[group]
            a = checked_measurement(arm.index(original[32:], old_raw, count, predicted,
                        reader.dictionary, reader.tile_count), original[32:], old_raw)
            b = checked_measurement(arm.index(candidate[32:], raw, len(mapping), predicted,
                        reader.dictionary, reader.tile_count), candidate[32:], raw)
            row.update(before=a, after=b, accepted=b['total_instructions'] <= a['total_instructions'])
            if row['accepted']:
                plan[group] = original, candidate, physical; mappings[group] = mapping
            else:
                row['rejection'] = 'complete_linked_decoder_instructions_increased'
        else:
            row['rejection'] = 'encoded_size_did_not_shrink'
        rows.append(row)
        if progress:
            progress(row)
    plan, changed, padding = aligned_plan(plan, align_suffix)
    if changed:
        output, relocate = coalesce.rewrite(data, plan, version=source.version)
        output = bytearray(output)
        for index in range(reader.frame_count):
            at = reader.descriptors+index*16
            global_at, local, raw_at, size = t.DESCRIPTOR.unpack_from(data, at)
            mapping = mappings.get(reader.tile_groups+local)
            if mapping is not None:
                t.DESCRIPTOR.pack_into(output, relocate(at), relocate(global_at), local, mapping[raw_at], size)
        output = bytes(output)
    else:
        output = data
    proof = verify(data, output, plan, mappings)
    t.require(len(output) <= len(data), 'Accepted compaction grew the pack')
    return output, dict(schema=1, complete=True, input_sha256=t.sha256(data), output_sha256=t.sha256(output),
        input_bytes=len(data), output_bytes=len(output), saved_bytes=len(data)-len(output),
        measured_elf_sha256=arm.elf_sha256, orphan_records_found=orphan_count,
        orphan_records_removed=sum(r['orphan_records'] for r in rows if r['accepted']),
        groups_considered=len(rows), groups_accepted=len(changed), suffix_padding=padding,
        groups=rows, verification=proof, maximum_instruction_regression_percent=0,
        limitation='Complete linked decoder instructions, not physical calculator FPS.')


def validate_output_paths(input_path, output_path):
    receipt_path = output_path.with_suffix('.json')
    paths = (input_path, output_path, receipt_path)
    for path in paths:
        t.private_path(path, 'Private index compaction path')
    t.require(len({path.resolve() for path in paths}) == len(paths),
              'Input, output and receipt paths must be distinct')
    t.require(not output_path.exists(), 'Preserve the original pack')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', type=Path, required=True); p.add_argument('--output', type=Path, required=True)
    p.add_argument('--elf', type=Path, required=True); args = p.parse_args()
    validate_output_paths(args.input, args.output)
    started = time.monotonic(); t.HOST_CACHE_GROUPS = 16
    output, proof = convert_bytes(args.input.read_bytes(), ArmIndices(args.elf),
        progress=lambda row: print(json.dumps(dict(stage='orphan_index_group', **row)), flush=True))
    proof['seconds'] = time.monotonic()-started
    t.atomic_bytes(args.output, output)
    t.atomic_bytes(args.output.with_suffix('.json'), json.dumps(proof, indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in proof.items() if k not in ('groups', 'verification')}), flush=True)


if __name__ == '__main__': main()
