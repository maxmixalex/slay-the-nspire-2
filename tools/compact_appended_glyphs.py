#!/usr/bin/env python3
"""Compact only the new literal glyph tail of a private NSP2/35-38 font append.

The supplied immutable baseline identifies every old glyph. Existing artwork,
glyph/string groups and their descriptors stay byte-exact at their current
locations. Only strictly smaller new tails use the already supported RGR1
glyph format; no runtime/version/pool change or font resampling is involved.
Run after scene_ui9_fonts, preserving its current resource version.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass, field
import json
import lzma
from pathlib import Path
import struct

import card_library_fonts as library
import content_tiles as tiles
from group_glyphs import DESCRIPTOR, DEFAULT_GROUP_BYTES, FLAG
from repack_retained import auxiliary_count

PROFILES = ((3, 0, 2), (3, 0, 0), (4, 0, 1))
CUTOFFS = (0, 24, 32, 48)


@dataclass
class Record:
    payload: bytes
    band: int
    codepoint: int
    ids: list = field(default_factory=list)


def view(data):
    return tiles.PackView(data, auxiliary_count(data), versions=(34, 35, 36, 37, 38))


def write_tail(source, start, records, aliases, cutoff):
    out = bytearray(source.data[:start])
    locations = dict(aliases)
    stats = dict(groups=0, grouped_entries=0, grouped_unique_payloads=0,
                 group_payload_bytes=0, descriptor_bytes=0, literal_payload_bytes=0,
                 maximum_group_raw_bytes=0, minimum_grouped_payload_bytes=cutoff + 1)

    def append(payload, align=False):
        if align:
            out.extend(bytes(len(out) & 1))
        at = len(out)
        out.extend(payload)
        return at

    def literal(record):
        at = append(record.payload)
        for index in record.ids:
            locations[index] = (at, False)
        stats['literal_payload_bytes'] += len(record.payload)

    pending, raw_size = [], 0

    def finish():
        nonlocal pending, raw_size
        if not pending:
            return
        raw = b''.join(record.payload for record in pending)
        candidates = [lzma.compress(raw, format=lzma.FORMAT_RAW,
            filters=[dict(id=lzma.FILTER_LZMA2, preset=9, dict_size=tiles.GROUP_LIMIT,
                          nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
            for lc, lp, pb in PROFILES]
        encoded = min(candidates, key=len)
        tiles.require(tiles.decode_lzma2(encoded, len(raw)) == raw,
                      'Appended glyph compression roundtrip differs')
        group = tiles.GROUP.pack(b'RGR1', len(raw), len(encoded), 14, len(pending), 0, 0, 0) + encoded
        stored = (len(out) & 1) + ((len(group) + 1) & ~1) + len(pending) * DESCRIPTOR.size
        if stored < len(raw):
            at = append(group, True)
            raw_at = 0
            for record in pending:
                descriptor_at = (len(out) + 1) & ~1
                actual = append(DESCRIPTOR.pack(at - descriptor_at, len(group), raw_at, len(record.payload)), True)
                tiles.require(actual == descriptor_at, 'Appended glyph descriptor alignment differs')
                for index in record.ids:
                    locations[index] = (actual, True)
                raw_at += len(record.payload)
            stats['groups'] += 1
            stats['grouped_entries'] += sum(len(record.ids) for record in pending)
            stats['grouped_unique_payloads'] += len(pending)
            stats['group_payload_bytes'] += len(group)
            stats['descriptor_bytes'] += len(pending) * DESCRIPTOR.size
            stats['maximum_group_raw_bytes'] = max(stats['maximum_group_raw_bytes'], len(raw))
        else:
            for record in pending:
                literal(record)
        pending, raw_size = [], 0

    # Group newly added faces/sizes together for demand locality. Tiny glyphs
    # stay literal when their 16-byte descriptor would erase the saving.
    for record in sorted(records, key=lambda row: (row.band, row.codepoint)):
        if len(record.payload) <= cutoff:
            continue
        if raw_size + len(record.payload) > DEFAULT_GROUP_BYTES:
            finish()
        pending.append(record)
        raw_size += len(record.payload)
    finish()
    for record in records:
        if len(record.payload) <= cutoff:
            literal(record)
    for index, (at, grouped) in locations.items():
        cp, _old, px, width, height, advance, encoded = source.glyphs[index]
        tiles.GLYPH.pack_into(out, source.glyph_at + index * 16,
            cp, at, px, width, height, advance, encoded | (FLAG if grouped else 0))
    struct.pack_into('<I', out, 12, len(out))
    return bytes(out), stats


def verify(source, data, new_ids, start):
    target = view(data)
    tiles.require(source.version == target.version and source.counts == target.counts and
                  source.images == target.images and source.clips == target.clips and
                  source.cards == target.cards and source.aux == target.aux and
                  source.strings == target.strings, 'Appended glyph compaction changed a catalog')
    expected = bytearray(source.data[:start])
    struct.pack_into('<I', expected, 12, len(data))
    added = set(new_ids)
    for index, (before, after) in enumerate(zip(source.glyphs, target.glyphs)):
        if index in added:
            tiles.require(before[:1] + before[2:6] == after[:1] + after[2:6] and
                          before[6] == (after[6] & ~FLAG), 'New glyph key, metrics or encoding changed')
            tiles.GLYPH.pack_into(expected, source.glyph_at + index * 16, *after)
        else:
            tiles.require(before == after, 'An old glyph descriptor changed')
        tiles.require(source.glyph_payload(index) == target.glyph_payload(index) and
                      source.glyph_alpha(index) == target.glyph_alpha(index), 'Glyph pixels changed')
    tiles.require(data[:start] == expected, 'Existing artwork, groups, strings or payload bytes changed')
    return dict(glyphs_verified=len(source.glyphs), new_glyphs_verified=len(added),
        unchanged_old_glyph_records=len(source.glyphs) - len(added),
        exact_all_glyph_payloads_and_alpha=True, exact_keys_sizes_advances=True,
        exact_complete_old_payload=True, exact_artwork_and_clip_clocks=True,
        exact_string_records_and_bytes=True, resident_pool_growth_bytes=0)


def compact(data, baseline):
    source, old = view(data), view(baseline)
    tiles.require(source.version in (35, 36, 37, 38), 'Only an NSP2/35-38 scene-font append is supported')
    tiles.require(source.version == old.version or source.version == 35 and old.version == 34,
                  'Compaction cannot upgrade or downgrade a scene-font resource version')
    tiles.require(source.counts[5] == old.counts[5], 'Baseline auxiliary catalog differs')
    library.verify_preserved(baseline, data, source.counts[5], set())
    known = {(row[0], row[2]) for row in old.glyphs}
    new_ids = [index for index, row in enumerate(source.glyphs) if (row[0], row[2]) not in known]
    start = len(baseline) + (len(source.glyphs) - len(old.glyphs)) * tiles.GLYPH.size
    report = dict(input_bytes=len(data), input_sha256=tiles.sha256(data),
        baseline_sha256=tiles.sha256(baseline), version=source.version, added_glyphs=len(new_ids),
        group_target_bytes=DEFAULT_GROUP_BYTES, new_runtime_pools=0)
    if not new_ids or any(source.glyphs[index][6] & FLAG for index in new_ids):
        report.update(output_bytes=len(data), output_sha256=tiles.sha256(data), saved_bytes=0,
                      preserved_reason='no_new_literal_tail' if not new_ids else 'existing_appended_groups')
        return data, report
    cursor = start
    for index in sorted(new_ids, key=lambda i: source.glyphs[i][1]):
        row = source.glyphs[index]
        tiles.require(row[1] == cursor, 'New glyphs must form the complete contiguous literal tail')
        cursor += row[6] or row[3] * row[4]
    tiles.require(cursor == len(data), 'Unknown bytes follow the appended glyph tail')
    old_payloads = {}
    added = set(new_ids)
    for index, row in enumerate(source.glyphs):
        if index not in added:
            old_payloads.setdefault(bytes(source.glyph_payload(index)), (row[1], bool(row[6] & FLAG)))
    unique, records, aliases = {}, [], {}
    for index in new_ids:
        row = source.glyphs[index]
        payload = bytes(source.glyph_payload(index))
        if payload in old_payloads:
            aliases[index] = old_payloads[payload]
            continue
        record = unique.get(payload)
        if record is None:
            record = Record(payload, row[2], row[0])
            unique[payload] = record
            records.append(record)
        record.ids.append(index)
    candidates = [write_tail(source, start, records, aliases, cutoff) for cutoff in CUTOFFS]
    result, statistics = min(candidates, key=lambda candidate: len(candidate[0]))
    if len(result) >= len(data):
        result, statistics = data, dict(groups=0, preserved_reason='no_strict_net_saving')
    proof = verify(source, result, new_ids, start)
    report.update(output_bytes=len(result), output_sha256=tiles.sha256(result), saved_bytes=len(data) - len(result),
        old_payload_aliases=len(aliases), unique_new_payloads=len(records),
        original_literal_tail_bytes=len(data) - start, **statistics, verification=proof)
    return result, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('input', 'baseline', 'output', 'receipt'):
        parser.add_argument('--' + option, type=Path, required=True)
    args = parser.parse_args()
    paths = {name: tiles.private_path(getattr(args, name), name) for name in ('input', 'baseline', 'output', 'receipt')}
    tiles.require(len(set(paths.values())) == len(paths), 'All paths must differ; inputs are immutable')
    result, receipt = compact(paths['input'].read_bytes(), paths['baseline'].read_bytes())
    tiles.atomic_bytes(paths['output'], result)
    tiles.atomic_bytes(paths['receipt'], json.dumps(receipt, indent=2).encode() + b'\n')
    print(json.dumps(receipt), flush=True)


if __name__ == '__main__':
    main()
