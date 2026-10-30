#!/usr/bin/env python3
"""Build NSP2/41 TGI2 frame indices and complete split language editions.

Every original group remains independently addressable with its original raw
length, tile IDs, frame records and cache allocation. Only beneficial frame
index streams use the reversible temporal/spatial predictor; tile pixels,
retained images, glyphs, strings, dimensions, clocks and references stay exact.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import lzma
from pathlib import Path
import struct
import time

import content_tiles as t
from compact_tile_resources import grouped_slices, SLICE
from recompress_resources import Relocation
from repack_retained import auxiliary_count
import split_content as split
import storage_parts as storage
from tile_prediction import index_transform

VERSION = 41
ENCODER = b'TGI2-spatial-temporal-zigzag-v1-lzma2-9-lc4-lp0-pb1-nice273-depth512'


def view(data):
    return t.PackView(data, auxiliary_count(data), versions=tuple(range(20, VERSION + 1)))


def make_plan(source, *, workers=2, cache=None):
    reader = t.TileReader(source)
    jobs = list(range(reader.tile_groups, len(reader.groups)))
    if cache is not None:
        cache = Path(cache); cache.mkdir(parents=True, exist_ok=True)

    def convert(group):
        at, encoded, size, _crc, _kind, count = reader.groups[group]
        original = bytes(source.data[at - 32:at + encoded])
        raw = t.decode_lzma2(original[32:], size)
        if reader.predictors[group]:raw = index_transform(raw, count, 3, True, reader.tile_count)
        filtered = index_transform(raw, count, 3, tile_count=reader.tile_count)
        t.require(index_transform(filtered, count, 3, True, reader.tile_count) == raw,
                  'Predictive index inverse changed raw records')
        key = t.sha256(ENCODER + struct.pack('<I', count) + filtered)
        path = cache / (key + '.lzma2') if cache is not None else None
        if path is not None and path.is_file():
            packed = path.read_bytes()
            t.require(t.decode_lzma2(packed, size) == filtered, 'Predictive index cache differs')
        else:
            packed = lzma.compress(filtered, format=lzma.FORMAT_RAW, filters=[dict(
                id=lzma.FILTER_LZMA2, preset=9, dict_size=t.GROUP_LIMIT,
                nice_len=273, depth=512, lc=4, lp=0, pb=1)])
            t.require(t.decode_lzma2(packed, size) == filtered, 'Predictive index compression differs')
            if path is not None:t.atomic_bytes(path, packed)
        header = list(t.GROUP.unpack_from(original))
        header[0], header[4], header[7] = b'TGI2', len(packed), 1
        candidate = t.GROUP.pack(*header) + packed
        # Preserve the replaced span's parity. Padding is outside the exact
        # compressed stream and never accepted by the runtime LZMA2 decoder.
        physical = candidate + bytes((len(original) - len(candidate)) & 1)
        selected = len(physical) < len(original)
        return group, original, candidate if selected else original, physical if selected else original, size

    plan = {}
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for number, (group, old, payload, physical, size) in enumerate(pool.map(convert, jobs), 1):
            plan[group] = (old, payload, physical, size)
            if number % 64 == 0 or number == len(jobs):
                print(json.dumps(dict(stage='encode_indices', completed=number, groups=len(jobs),
                    saved_bytes=sum(len(a)-len(c) for a, _b, c, _d in plan.values()))), flush=True)
    return plan


def rewrite(data, plan):
    source = view(data); old_tiles = t.TileReader(source)
    replacements = []
    for group, (original, payload, physical, _size) in sorted(plan.items()):
        t.require(old_tiles.tile_groups <= group < len(old_tiles.groups), 'Prediction plan has an invalid group')
        at, size, *_ = old_tiles.groups[group]
        t.require(bytes(data[at-32:at+size]) == original, 'Locale artwork differs from shared plan')
        if len(physical) < len(original):replacements.append((at-32, len(original), payload, physical))
    relocate = Relocation(replacements)
    out, prior, sizes = bytearray(), 0, {}
    for at, length, payload, physical in replacements:
        out.extend(data[prior:at]); out.extend(physical)
        prior = at + length; sizes[at] = len(payload)
    out.extend(data[prior:])
    for i, (offset, w, h, fmt, length) in enumerate(source.images):
        t.IMAGE.pack_into(out, 40+i*16, relocate(offset), w, h, fmt, length)
    for i, (cp, offset, px, w, h, advance, encoded) in enumerate(source.glyphs):
        t.GLYPH.pack_into(out, source.glyph_at+i*16, cp, relocate(offset), px, w, h, advance, encoded)
    for i, (offset, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at+i*8, relocate(offset), length)
    header = list(t.GLOBAL.unpack_from(data, source.base_end)); table = header[9]
    for field in (8, 9, 14):header[field] = relocate(header[field])
    t.GLOBAL.pack_into(out, source.base_end, *header)
    for i in range(len(old_tiles.groups)):
        offset, length = struct.unpack_from('<II', data, table+i*8)
        struct.pack_into('<II', out, relocate(table+i*8), relocate(offset), sizes.get(offset, length))
    for i in range(old_tiles.frame_count):
        offset = old_tiles.descriptors+i*16
        global_at, group, raw_at, size = t.DESCRIPTOR.unpack_from(data, offset)
        t.DESCRIPTOR.pack_into(out, relocate(offset), relocate(global_at), group, raw_at, size)
    for offset, (relative, length, raw_at, shape) in grouped_slices(source).items():
        SLICE.pack_into(out, relocate(offset), relocate(offset+relative)-relocate(offset), length, raw_at, shape)
    struct.pack_into('<I', out, 4, VERSION); struct.pack_into('<I', out, 12, len(out)); struct.pack_into('<I', out, 36, 0)
    target = view(out); new_tiles = t.TileReader(target)
    t.require(source.counts == target.counts and source.clips == target.clips and
              source.cards == target.cards and source.aux == target.aux,
              'Index conversion changed catalog, frame clocks or geometry')
    index_raw = 0
    for group, (old, new) in enumerate(zip(old_tiles.groups, new_tiles.groups)):
        at, size, raw_size, crc, kind, count = old
        nat, nsize, nraw_size, ncrc, nkind, ncount = new
        t.require(nat == relocate(at-32)+32 and
                  (raw_size, crc, kind, count) == (nraw_size, ncrc, nkind, ncount),
                  'Index conversion changed group identity or cache size')
        if kind == 1:
            t.require(data[at-32:at+size] == out[nat-32:nat+nsize], 'Encoded tile pixels changed')
        else:
            t.require(old_tiles.group(group)[0] == new_tiles.group(group)[0], 'Decoded index records changed')
            index_raw += raw_size
    for old, new in zip(source.images, target.images):
        offset, _w, _h, fmt, length = old
        t.require(new == (relocate(offset), *old[1:]), 'Image shape or encoding changed')
        if fmt < 6:
            t.require(data[offset:offset+length] == out[new[0]:new[0]+length], 'Literal image changed')
        elif fmt == 6:
            descriptor = t.DESCRIPTOR.unpack_from(data, offset)
            t.require(t.DESCRIPTOR.unpack_from(out, new[0]) == (relocate(descriptor[0]), *descriptor[1:]),
                      'Frame descriptor changed')
    checked = set()
    for offset, (relative, length, raw_at, shape) in grouped_slices(source).items():
        group = offset+relative
        t.require(SLICE.unpack_from(out, relocate(offset)) ==
                  (relocate(group)-relocate(offset), length, raw_at, shape), 'Grouped slice changed')
        if group not in checked:
            t.require(data[group:group+length] == out[relocate(group):relocate(group)+length],
                      'Retained image/font/text group changed')
            checked.add(group)
    for i, (old, new) in enumerate(zip(source.glyphs, target.glyphs)):
        t.require(new == (old[0], relocate(old[1]), *old[2:]) and
                  source.glyph_payload(i) == target.glyph_payload(i), 'Glyph metrics/pixels changed')
    for i, (old, new) in enumerate(zip(source.strings, target.strings)):
        t.require(new == (relocate(old[0]), old[1]) and source.string_payload(i) == target.string_payload(i),
                  'String changed')
    return bytes(out), dict(input_bytes=len(data), output_bytes=len(out), saved_bytes=len(data)-len(out),
        changed_index_groups=len(replacements), index_groups=old_tiles.index_groups,
        decoded_index_bytes_compared=index_raw, tile_groups_copied=old_tiles.tile_groups,
        images=len(source.images), glyphs=len(source.glyphs), strings=len(source.strings), clips=len(source.clips),
        all_decoded_index_records_equal=True, all_encoded_pixel_groups_equal=True,
        all_image_descriptors_equal=True, all_retained_groups_equal=True,
        all_glyphs_and_strings_equal=True, catalog_clocks_and_cache_costs_equal=True,
        resident_bytes_added=0, group_reads_added=0,
        input_sha256=t.sha256(data), output_sha256=t.sha256(out))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True, help='Matching original NSM1 + NSL3 directory')
    parser.add_argument('--extra-locales', type=Path, action='append', default=[])
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    args = parser.parse_args(); t.require(1 <= args.workers <= 4, 'Use one through four encoder workers')
    source = t.private_path(args.input, 'Private input'); target = t.private_path(args.output, 'Private output')
    work = t.private_path(args.work, 'Private work')
    t.require(source != target and source not in target.parents and target not in source.parents,
              'Output must not overwrite source packs')
    target.mkdir(parents=True, exist_ok=True); work.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    original_common = storage.join_parts((source/'nSpire2.dat.tns').read_bytes(),
                                         {name:(source/name).read_bytes() for name in storage.PART_FILES})
    inputs = {}
    for folder in (source, *args.extra_locales):
        for code in split.LANGUAGES:
            path = folder/f'nSpire2.{code}.dat.tns'
            if path.is_file():inputs[code] = path
    t.require('eng' in inputs and 'zhs' in inputs, 'Default English and Chinese are required')
    english = split.reconstruct(original_common, inputs['eng'].read_bytes(), 'eng')
    plan = make_plan(view(english), workers=args.workers, cache=work/'encoded')
    report = dict(schema=1, reference='v0.111.0 / 41cef1ea', version=VERSION, locales={})
    common = None
    for code in ('eng', *(c for c in split.LANGUAGES if c != 'eng' and c in inputs)):
        original = english if code == 'eng' else split.reconstruct(original_common, inputs[code].read_bytes(), code)
        converted, proof = rewrite(original, plan)
        row = split.layout(converted, code)
        artwork = converted[row['prefix']:row['prefix']+row['artwork']]
        if common is None:
            common = split.common_file(artwork)
            index, parts, part_receipt = storage.partition(common, converted)
            t.atomic_bytes(target/'nSpire2.dat.tns', index)
            for name, payload in parts.items():t.atomic_bytes(target/name, payload)
            report['artwork'] = part_receipt
        else:t.require(common[64:] == artwork, 'Converted locales no longer share exact artwork')
        locale = split.language_file(converted, row, code, bytes(32))
        locale = split.compress_language_directory(locale)
        locale, storage_receipt = storage.compress_blocks(locale)
        t.require(split.reconstruct(common, locale, code) == converted, 'New locale split roundtrip differs')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns', locale)
        if code in ('eng', 'zhs'):t.atomic_bytes(work/(code+'-index41.nsp'), converted)
        report['locales'][code] = dict(proof=proof, storage=storage_receipt,
                                      original_locale_bytes=inputs[code].stat().st_size)
        print(json.dumps(dict(stage='locale_complete', locale=code, **proof,
                             stored_locale_bytes=len(locale))), flush=True)
    report.update(complete=True, elapsed_seconds=time.monotonic()-started,
                  installed_artwork_bytes=sum((target/name).stat().st_size for name in ('nSpire2.dat.tns', *storage.PART_FILES)),
                  installed_locale_bytes=sum((target/f'nSpire2.{code}.dat.tns').stat().st_size for code in inputs))
    t.atomic_bytes(target/'index-prediction-receipt.json', json.dumps(report, indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('locales','artwork')}), flush=True)


if __name__ == '__main__':main()
