#!/usr/bin/env python3
"""NSP2/43 RGR3: losslessly separate retained sparse image fields before LZMA2.

Every image, slice, original alpha/RGB565 byte, glyph, string, frame and group
identity survives. Only smaller complete groups are selected. Current shared
concern boundaries are preserved by semantic object relocation. The runtime
uses its existing group and transfer pools, including incremental preparation.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import lzma
from pathlib import Path
import struct
import time

import compact_retained_colors as colors
from compact_tile_slots import partition_like
from complete_retained_resources_ui17 import decoded_group, verify
import content_tiles as t
from retained_filter import group_layout, transform_group
import split_content as split
import storage_parts as storage

VERSION = 43
ENCODER = b'RGR3-exact-sparse-planes-v1-lzma2-9-273-512'
PROFILES = ((3, 0, 2), (3, 0, 0), (4, 0, 1))


def candidate(source, group, cache):
    old = bytes(source.block(group.offset, group.size))
    raw_size, _start, _mode, _slices = group_layout(old, source.version)
    receipt = dict(first_image=group.first_image, records=len(group.slices), raw_bytes=raw_size,
                   original_bytes=len(old), output_bytes=len(old), saved_bytes=0, mode=None)
    result = colors.Choice(t.sha256(old), group.slices, None, receipt)
    if not colors.complete_slices(group.slices, raw_size):
        receipt['preserved_reason'] = 'unreferenced_or_overlapping_slices'; return result
    sparse = [shape & 0xffffff for _at, shape in group.slices if shape >> 24 == 4]
    if not sparse:
        receipt['preserved_reason'] = 'no_sparse_image'; return result
    table = b''.join(struct.pack('<II', *row) for row in group.slices)
    if 32 + len(table) + max(sparse) > t.ENCODED_LIMIT:
        receipt['preserved_reason'] = 'existing_transfer_bound'; return result
    raw = decoded_group(old, source.version)
    key = t.sha256(ENCODER + old + table)
    path = cache / (key + '.bin')
    best, chosen = old, None
    if path.is_file():
        proposed = path.read_bytes()
        t.require(decoded_group(proposed, VERSION) == raw, 'Cached retained planes differ')
        if colors.aligned(len(proposed)) < colors.aligned(len(best)):
            best = proposed; chosen = t.GROUP.unpack_from(best)[5]
    else:
        for mode in (0, 1, 2):
            separated = transform_group(raw, group.slices, 8 + mode)
            t.require(transform_group(separated, group.slices, 8 + mode, True) == raw,
                      'Retained planes failed the exact roundtrip')
            for lc, lp, pb in PROFILES:
                stream = lzma.compress(separated, format=lzma.FORMAT_RAW, filters=[dict(
                    id=lzma.FILTER_LZMA2, preset=9, dict_size=t.GROUP_LIMIT,
                    nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
                length = 32 + len(table) + len(stream)
                if length > t.ENCODED_LIMIT or colors.aligned(length) >= colors.aligned(len(best)):
                    continue
                t.require(t.decode_lzma2(stream, raw_size) == separated, 'Retained planes LZMA2 differs')
                best = t.GROUP.pack(b'RGR3', raw_size, len(stream), 14,
                    len(group.slices), mode, len(table), 0) + table + stream
                chosen = mode
        t.atomic_bytes(path, best)
    if chosen is not None:
        result.payload = best
        receipt.update(mode=chosen, output_bytes=len(best),
                       saved_bytes=colors.aligned(len(old))-colors.aligned(len(best)))
    return result


def make_plan(source, cache, workers=2):
    cache = Path(cache); cache.mkdir(parents=True, exist_ok=True)
    catalog = colors.groups(source); choices = []
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for result in pool.map(lambda group:candidate(source, group, cache), catalog):
            choices.append(result)
            if len(choices) % 40 == 0 or len(choices) == len(catalog):
                print(json.dumps(dict(stage='retained_planes', complete=len(choices), total=len(catalog),
                    changed=sum(c.payload is not None for c in choices),
                    saved_bytes=sum(c.receipt['saved_bytes'] for c in choices))), flush=True)
    return choices


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--locales', nargs='+', choices=split.LANGUAGES)
    args = parser.parse_args()
    source = t.private_path(args.input, 'Private source')
    target = t.private_path(args.output, 'Private output'); work = t.private_path(args.work, 'Private work')
    t.require(source != target and source not in target.parents and target not in source.parents,
              'Never overwrite source resources')
    t.require(1 <= args.workers <= 4, 'Use one through four bounded workers')
    locales = args.locales or list(split.LANGUAGES)
    t.require('eng' in locales and len(set(locales)) == len(locales) and
              all((source/f'nSpire2.{c}.dat.tns').is_file() for c in locales), 'Requested locales are missing or repeated')
    target.mkdir(parents=True, exist_ok=True); work.mkdir(parents=True, exist_ok=True)
    old_index = (source/'nSpire2.dat.tns').read_bytes()
    old_common = storage.join_parts(old_index, {n:(source/n).read_bytes() for n in storage.PART_FILES})
    english = split.reconstruct(old_common, (source/'nSpire2.eng.dat.tns').read_bytes(), 'eng')
    initial = colors.view(english); started = time.monotonic()
    choices = make_plan(initial, work/'encoded', args.workers)
    report = dict(schema=1, reference='v0.111.0 / 41cef1ea', version=VERSION,
        independent_group_boundaries_preserved=True, resident_pools_added=0,
        previous_installed_artwork_bytes=sum((source/n).stat().st_size for n in ('nSpire2.dat.tns',*storage.PART_FILES)),
        previous_installed_locale_bytes=sum((source/f'nSpire2.{c}.dat.tns').stat().st_size for c in locales),
        groups=len(choices), changed_groups=sum(c.payload is not None for c in choices),
        results=[c.receipt for c in choices], locales={})
    common = None
    for code in ('eng', *(c for c in split.LANGUAGES if c != 'eng' and c in locales)):
        before = english if code == 'eng' else split.reconstruct(old_common, (source/f'nSpire2.{code}.dat.tns').read_bytes(), code)
        old = initial if code == 'eng' else colors.view(before)
        converted = bytearray(colors.rewrite(old, choices)); struct.pack_into('<I', converted, 4, VERSION)
        converted = bytes(converted)
        proof = verify(old, converted, choices, decode_groups=code == 'eng', expected_version=VERSION)
        row = split.layout(converted, code); artwork = converted[row['prefix']:row['prefix']+row['artwork']]
        if common is None:
            common, index, parts, partition = partition_like(old_index, english, converted)
            t.atomic_bytes(target/'nSpire2.dat.tns', index)
            for name, payload in parts.items():t.atomic_bytes(target/name, payload)
            report['artwork'] = partition
        else:t.require(common[64:] == artwork, 'Locales lost exact shared artwork')
        locale = split.compress_language_directory(split.language_file(converted, row, code, bytes(32)))
        locale, compression = storage.compress_blocks(locale)
        t.require(split.reconstruct(common, locale, code) == converted, 'Planar split reconstruction differs')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns', locale)
        if code in ('eng','zhs'):t.atomic_bytes(work/(code+'-planes43.nsp'), converted)
        report['locales'][code] = dict(proof=proof, storage=compression, stored_bytes=len(locale))
        print(json.dumps(dict(stage='planar_locale_complete', locale=code, stored_bytes=len(locale), **proof)), flush=True)
    report.update(complete=True, elapsed_seconds=time.monotonic()-started,
        installed_artwork_bytes=sum((target/n).stat().st_size for n in ('nSpire2.dat.tns',*storage.PART_FILES)),
        installed_locale_bytes=sum((target/f'nSpire2.{c}.dat.tns').stat().st_size for c in locales))
    report['saved_installed_bytes'] = report['previous_installed_artwork_bytes']+report['previous_installed_locale_bytes']-report['installed_artwork_bytes']-report['installed_locale_bytes']
    t.atomic_bytes(target/'retained-planes-receipt.json', json.dumps(report, indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('results','locales','artwork')}), flush=True)


if __name__ == '__main__':main()
