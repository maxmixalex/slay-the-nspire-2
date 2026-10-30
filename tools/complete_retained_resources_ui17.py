#!/usr/bin/env python3
"""Apply the existing exact RGR2 codec to later unfiltered image groups.

Already-filtered groups and incomplete/overlapping retained slices stay
unchanged. A complete group is replaced only when its full aligned footprint
shrinks. Existing TGI2 indices, every tile, frame, glyph and string survive.
No new runtime decoder or memory is required by this conversion stage.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import struct
import time

import compact_retained_colors as colors
import content_tiles as t
from retained_filter import group_layout, transform_group
from repack_retained import DESCRIPTOR
import split_content as split
import storage_parts as storage


def decoded_group(payload, version):
    size, start, mode, slices = group_layout(payload, version)
    raw = t.decode_lzma2(payload[start:], size)
    return transform_group(raw, slices, mode, True) if mode else raw


def verify(source, converted, choices, *, decode_groups, expected_version=None):
    target = colors.view(converted)
    t.require(source.counts == target.counts and source.clips == target.clips and
              source.cards == target.cards and source.aux == target.aux and
              target.version == (expected_version if expected_version is not None else max(source.version, colors.CONTENT_VERSION)), 'Retained completion changed catalog or format version')
    old_tiles, new_tiles = t.TileReader(source), t.TileReader(target)
    t.require((old_tiles.tile_count, old_tiles.frame_count, old_tiles.per_group, len(old_tiles.groups)) ==
              (new_tiles.tile_count, new_tiles.frame_count, new_tiles.per_group, len(new_tiles.groups)),
              'Retained completion changed animation grouping')
    for old, new in zip(old_tiles.groups, new_tiles.groups):
        at, size = old[:2]; nat, nsize = new[:2]
        t.require(old[1:] == new[1:] and source.data[at-32:at+size] == converted[nat-32:nat+nsize],
                  'Compressed tile/index group changed')
    old_groups, new_groups = colors.groups(source), colors.groups(target)
    t.require(len(old_groups) == len(new_groups) == len(choices), 'Retained group count changed')
    mapping, raw_bytes, changed = {}, 0, 0
    for old, new, choice in zip(old_groups, new_groups, choices):
        before = bytes(source.block(old.offset, old.size)); after = bytes(target.block(new.offset, new.size))
        t.require(old.slices == new.slices and old.first_image == new.first_image and
                  t.sha256(before) == choice.original_sha256 and
                  after == (choice.payload if choice.payload is not None else before),
                  'Selected retained bytes or image slices changed')
        if decode_groups:
            a, b = decoded_group(before, source.version), decoded_group(after, target.version)
            t.require(a == b, 'Complete decoded retained group changed')
            raw_bytes += len(a)
        mapping[old.offset] = (new.offset, new.size)
        changed += choice.payload is not None
    for old, new in zip(source.images, target.images):
        at, _w, _h, fmt, size = old; nat = new[0]
        t.require(old[1:] == new[1:], 'Image dimensions/format/length changed')
        if fmt == 6:
            t.require(source.block(at, size) == target.block(nat, size), 'Frame descriptor changed')
        elif fmt == 7:
            relative, length, raw_at, shape = DESCRIPTOR.unpack_from(source.data, at)
            nrelative, nlength, nraw_at, nshape = DESCRIPTOR.unpack_from(converted, nat)
            t.require((nat+nrelative, nlength) == mapping[at+relative] and
                      (raw_at, shape) == (nraw_at, nshape), 'Retained image points at a different slice')
        else:t.require(source.block(at, size) == target.block(nat, size), 'Literal picture changed')
    old_cost = t.GLOBAL.unpack_from(source.data, source.base_end)[14]
    new_cost = t.GLOBAL.unpack_from(converted, target.base_end)[14]
    t.require(source.data[old_cost:old_cost+len(source.images)] == converted[new_cost:new_cost+len(target.images)],
              'Decoded image preparation costs changed')
    for i, (old, new) in enumerate(zip(source.glyphs, target.glyphs)):
        t.require(old[:1]+old[2:] == new[:1]+new[2:] and source.glyph_payload(i) == target.glyph_payload(i),
                  'Glyph metrics or pixels changed')
    for i, (old, new) in enumerate(zip(source.strings, target.strings)):
        t.require(old[1] == new[1] and source.string_payload(i) == target.string_payload(i), 'String changed')
    return dict(images=len(source.images), glyphs=len(source.glyphs), strings=len(source.strings),
                clips=len(source.clips), unchanged_tile_and_index_groups=len(old_tiles.groups),
                retained_groups=len(old_groups), changed_retained_groups=changed,
                complete_raw_retained_groups_compared=len(old_groups) if decode_groups else 0,
                complete_raw_retained_bytes_compared=raw_bytes,
                all_image_slices_and_literal_pixels_equal=True,
                all_glyphs_strings_clocks_and_costs_equal=True,
                original_version=source.version, output_version=target.version,
                input_bytes=len(source.data), output_bytes=len(converted),
                saved_bytes=len(source.data)-len(converted),
                input_sha256=t.sha256(source.data), output_sha256=t.sha256(converted))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--workers', type=int, default=2)
    args = p.parse_args()
    source = t.private_path(args.input, 'Private source'); target = t.private_path(args.output, 'Private output')
    work = t.private_path(args.work, 'Private work')
    t.require(source != target and source not in target.parents and target not in source.parents,
              'Never overwrite the source resource set')
    t.require(1 <= args.workers <= 4, 'Use one through four bounded workers')
    t.require(all((source/f'nSpire2.{code}.dat.tns').is_file() for code in ('eng','zhs')),
              'Both default locale files are required')
    target.mkdir(parents=True, exist_ok=True); work.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    old_common = storage.join_parts((source/'nSpire2.dat.tns').read_bytes(),
                                    {name:(source/name).read_bytes() for name in storage.PART_FILES})
    english = split.reconstruct(old_common, (source/'nSpire2.eng.dat.tns').read_bytes(), 'eng')
    initial = colors.view(english)
    choices = colors.plan(initial, work/'encoded', jobs=args.workers)
    eligibility = Counter()
    for choice in choices:
        reason = choice.receipt.get('preserved_reason', 'encoded_candidate')
        eligibility[reason] += 1
    report = dict(schema=1, reference='v0.111.0 / 41cef1ea', version=initial.version,
        independent_groups_preserved=True, runtime_source_changed=False, resident_bytes_added=0,
        eligibility=dict(eligibility), results=[choice.receipt for choice in choices], locales={})
    common = None
    for code in ('eng', *(c for c in split.LANGUAGES if c != 'eng')):
        path = source/f'nSpire2.{code}.dat.tns'
        if not path.is_file():continue
        before = english if code == 'eng' else split.reconstruct(old_common, path.read_bytes(), code)
        old = initial if code == 'eng' else colors.view(before)
        converted = colors.rewrite(old, choices)
        proof = verify(old, converted, choices, decode_groups=code == 'eng')
        row = split.layout(converted, code); artwork = converted[row['prefix']:row['prefix']+row['artwork']]
        if common is None:
            common = split.common_file(artwork)
            index, parts, partition = storage.partition(common, converted)
            t.atomic_bytes(target/'nSpire2.dat.tns', index)
            for name, payload in parts.items():t.atomic_bytes(target/name, payload)
            report['artwork'] = partition
        else:t.require(common[64:] == artwork, 'Locale artwork differs from fully decoded English proof')
        proof['shared_artwork_equal_to_proven_english'] = True
        locale = split.language_file(converted, row, code, bytes(32))
        locale = split.compress_language_directory(locale)
        locale, storage_receipt = storage.compress_blocks(locale)
        t.require(split.reconstruct(common, locale, code) == converted, 'Retained locale roundtrip differs')
        t.atomic_bytes(target/path.name, locale)
        if code in ('eng', 'zhs'):t.atomic_bytes(work/(code+'-retained41.nsp'), converted)
        report['locales'][code] = dict(proof=proof, storage=storage_receipt,
                                      previous_locale_bytes=path.stat().st_size)
        print(json.dumps(dict(stage='retained_locale_complete', locale=code,
                             stored_locale_bytes=len(locale), **proof)), flush=True)
    report.update(complete=True, elapsed_seconds=time.monotonic()-started,
        installed_artwork_bytes=sum((target/name).stat().st_size for name in ('nSpire2.dat.tns', *storage.PART_FILES)),
        installed_locale_bytes=sum((target/f'nSpire2.{code}.dat.tns').stat().st_size for code in report['locales']))
    t.atomic_bytes(target/'retained-completion-receipt.json', json.dumps(report, indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('locales','artwork','results')}), flush=True)


if __name__ == '__main__':main()
