#!/usr/bin/env python3
"""Losslessly compact NSP2 tile streams without changing their random access.

TGR2 separates tile headers, RGB565 words and alpha, and decorrelates the two
five-bit color channels from green. Every bit is restored before the existing
runtime tile reader uses a record. Groups, tile IDs, indices, geometry, frames,
timing and cache costs stay unchanged. The original group wins whenever the
filtered representation does not save physical bytes.
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

import content_tiles as tiles
from repack_retained import auxiliary_count
from recompress_resources import Relocation, choose_group
from tile_filter import filter_tiles, restore_tiles

VERSION = 33
VERSIONS = tuple(range(20, VERSION + 1))
ENCODER_ID = b'TGR2-green-linear-or-zigzag-v1-lzma2-9-273-512'
SLICE = struct.Struct('<iIII')


def view(data):
    return tiles.PackView(data, auxiliary_count(data), versions=VERSIONS)


def decoded(payload):
    header = tiles.GROUP.unpack_from(payload)
    tag, kind, _number, size, encoded, _crc, items, mode = header
    tiles.require(encoded == len(payload) - 32 and 0 < size <= tiles.GROUP_LIMIT,
                  'Invalid compressed tile group length')
    tiles.require((tag == b'TGR1' and mode == 0) or
                  (tag == b'TGR2' and kind == 1 and mode in (1, 2)), 'Invalid tile group filter')
    raw = tiles.decode_lzma2(payload[32:], size)
    if tag == b'TGR2':
        raw = restore_tiles(raw, mode)
    if kind == 1:
        tiles.require(items > 0 and size == items * tiles.TILE_BYTES, 'Invalid tile group shape')
        tiles.validate_tiles(raw)
    return raw


def candidate(raw, cache_directory=None):
    key = hashlib.sha256(ENCODER_ID + raw).hexdigest()
    path = cache_directory / (key + '.bin') if cache_directory is not None else None
    if path is not None and path.is_file():
        try:
            cached = path.read_bytes()
            mode, = struct.unpack_from('<I', cached)
            restored = restore_tiles(tiles.decode_lzma2(cached[4:], len(raw)), mode)
            tiles.require(restored == raw, 'Cached tile pixels differ')
            return mode, cached[4:]
        except (OSError, ValueError, struct.error, lzma.LZMAError):
            pass  # A disposable build cache is never a source of truth.
    best = None
    for mode in (1, 2):
        filtered = filter_tiles(raw, mode)
        tiles.require(restore_tiles(filtered, mode) == raw, 'Tile transform is not byte exact')
        for lc, lp, pb in ((4, 0, 1), (3, 1, 1)):
            compressed = lzma.compress(filtered, format=lzma.FORMAT_RAW,
                filters=[dict(id=lzma.FILTER_LZMA2, preset=9, dict_size=tiles.GROUP_LIMIT,
                              nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
            tiles.require(tiles.decode_lzma2(compressed, len(raw)) == filtered,
                          'Filtered tile compression roundtrip differs')
            if best is None or len(compressed) < len(best[1]):
                best = mode, compressed
    if path is not None:
        tiles.atomic_bytes(path, struct.pack('<I', best[0]) + best[1])
    return best


def grouped_slices(source):
    result = {}
    for offset, _w, _h, fmt, _length in source.images:
        if fmt == 7:
            result[offset] = SLICE.unpack_from(source.data, offset)
    for _cp, offset, _px, _w, _h, _advance, encoded in source.glyphs:
        if encoded & tiles.GLYPH_GROUP_FLAG:
            result[offset] = SLICE.unpack_from(source.data, offset)
    for offset, length in source.strings:
        if length & tiles.STRING_GROUP_FLAG:
            result[offset] = SLICE.unpack_from(source.data, offset)
    return result


def rewrite(data, plan):
    source = view(data)
    old_tiles = tiles.TileReader(source)
    replacements = [(offset, length, payload, physical)
                    for offset, length, payload, physical in plan if len(physical) < length]
    relocate = Relocation(replacements)
    out, previous, sizes = bytearray(), 0, {}
    for offset, length, payload, physical in replacements:
        out.extend(data[previous:offset]);out.extend(physical)
        previous = offset + length;sizes[offset] = len(payload)
    out.extend(data[previous:])
    for i, (offset, w, h, fmt, length) in enumerate(source.images):
        tiles.IMAGE.pack_into(out, 40 + i * 16, relocate(offset), w, h, fmt, length)
    for i, (cp, offset, px, w, h, advance, encoded) in enumerate(source.glyphs):
        tiles.GLYPH.pack_into(out, source.glyph_at + i * 16,
            cp, relocate(offset), px, w, h, advance, encoded)
    for i, (offset, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at + i * 8, relocate(offset), length)
    header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    table = header[9]
    for field in (8, 9, 14):
        header[field] = relocate(header[field])
    tiles.GLOBAL.pack_into(out, source.base_end, *header)
    for i in range(header[5] + header[6]):
        offset, length = struct.unpack_from('<II', data, table + i * 8)
        struct.pack_into('<II', out, relocate(table + i * 8), relocate(offset), sizes.get(offset, length))
    for i in range(old_tiles.frame_count):
        offset = old_tiles.descriptors + i * 16
        global_at, group, raw_at, size = tiles.DESCRIPTOR.unpack_from(data, offset)
        tiles.DESCRIPTOR.pack_into(out, relocate(offset), relocate(global_at), group, raw_at, size)
    for offset, (relative, length, raw_at, shape) in grouped_slices(source).items():
        SLICE.pack_into(out, relocate(offset), relocate(offset + relative) - relocate(offset),
                        length, raw_at, shape)
    struct.pack_into('<I', out, 4, VERSION)
    struct.pack_into('<I', out, 12, len(out))
    struct.pack_into('<I', out, 36, 0)

    target = view(out)
    new_tiles = tiles.TileReader(target)
    tiles.require(source.counts == target.counts and source.clips == target.clips and
                  source.cards == target.cards and source.aux == target.aux,
                  'Tile conversion changed model, image or animation registration')
    raw_bytes, filtered_groups, modes = 0, 0, {1: 0, 2: 0}
    for i, (old, new) in enumerate(zip(old_tiles.groups, new_tiles.groups)):
        off, length, raw_size, crc, kind, items = old
        noff, nlength, nraw, ncrc, nkind, nitems = new
        tiles.require((raw_size, crc, kind, items) == (nraw, ncrc, nkind, nitems) and
                      noff == relocate(off - 32) + 32, 'Tile/index group registration differs')
        old_payload = data[off - 32:off + length]
        new_payload = out[noff - 32:noff + nlength]
        tiles.require(decoded(old_payload) == decoded(new_payload), 'Decoded tile/index bytes changed')
        if kind == 2:
            tiles.require(old_payload == new_payload, 'Frame-index compressed bytes changed')
        if new_tiles.filters[i]:
            filtered_groups += 1;modes[new_tiles.filters[i]] += 1
        raw_bytes += raw_size
    for old, new in zip(source.images, target.images):
        offset, _w, _h, fmt, length = old
        tiles.require(new == (relocate(offset), *old[1:]), 'Image descriptor changed')
        if fmt < 6:
            tiles.require(data[offset:offset + length] == out[new[0]:new[0] + length],
                          'Literal image bytes changed')
        elif fmt == 6:
            a = tiles.DESCRIPTOR.unpack_from(data, offset)
            b = tiles.DESCRIPTOR.unpack_from(out, new[0])
            tiles.require((relocate(a[0]), *a[1:]) == b, 'Frame slice or geometry changed')
    checked = set()
    for offset, descriptor in grouped_slices(source).items():
        relative, length, raw_at, shape = descriptor
        group = offset + relative
        tiles.require(SLICE.unpack_from(out, relocate(offset)) ==
                      (relocate(group) - relocate(offset), length, raw_at, shape),
                      'Retained image/glyph/string slice changed')
        if group not in checked:
            tiles.require(data[group:group + length] == out[relocate(group):relocate(group) + length],
                          'Retained artwork/glyph/string encoded bytes changed')
            checked.add(group)
    for i, (old, new) in enumerate(zip(source.glyphs, target.glyphs)):
        tiles.require(new == (old[0], relocate(old[1]), *old[2:]) and
                      source.glyph_payload(i) == target.glyph_payload(i), 'Glyph key or pixels changed')
    for i, (old, new) in enumerate(zip(source.strings, target.strings)):
        tiles.require(new == (relocate(old[0]), old[1]) and
                      source.string_payload(i) == target.string_payload(i), 'String bytes changed')
    before_costs = tiles.GLOBAL.unpack_from(data, source.base_end)[14]
    after_costs = header[14]
    tiles.require(data[before_costs:before_costs + len(source.images)] ==
                  out[after_costs:after_costs + len(target.images)], 'Decoded image cache costs changed')
    return bytes(out), dict(input_bytes=len(data), output_bytes=len(out), saved_bytes=len(data) - len(out),
        input_version=source.version, output_version=VERSION, tile_groups=old_tiles.tile_groups,
        improved_groups=len(replacements), filtered_groups=filtered_groups, modes=modes,
        all_decoded_tile_and_index_bytes_equal=True, decoded_group_bytes_compared=raw_bytes,
        index_streams_byte_exact=True, retained_streams_byte_exact=True,
        frames_and_geometry_exact=True, original_timing_exact=True,
        image_cache_costs_exact=True, random_access_groups_exact=True,
        glyphs_verified=len(source.glyphs), strings_verified=len(source.strings),
        no_resident_buffer_added=True, largest_group_raw_bytes=max(row[2] for row in new_tiles.groups),
        input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(out))


def repack(data, *, workers=2, cache_directory=None):
    source = view(data)
    reader = tiles.TileReader(source)
    if cache_directory is not None:
        cache_directory = Path(cache_directory)
        cache_directory.mkdir(parents=True, exist_ok=True)
    entries = [(off - 32, length + 32) for off, length, *_ in reader.groups[:reader.tile_groups]]

    def convert(entry):
        offset, length = entry
        old = bytes(data[offset:offset + length])
        raw = decoded(old)
        mode, compressed = candidate(raw, cache_directory)
        header = list(tiles.GROUP.unpack_from(old))
        header[0], header[4], header[7] = b'TGR2', len(compressed), mode
        new = tiles.GROUP.pack(*header) + compressed
        payload, physical, _ = choose_group(old, new, len(raw))
        return offset, length, payload, physical

    plan, saved = [], 0
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for record in pool.map(convert, entries):
            plan.append(record);saved += record[1] - len(record[3])
            if len(plan) % 64 == 0:
                print(json.dumps(dict(stage='tile_filter', completed=len(plan), total=len(entries),
                                      saved_bytes=saved)), flush=True)
    return rewrite(data, plan)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--eng', type=Path, required=True)
    parser.add_argument('--zhs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    args = parser.parse_args()
    tiles.require(1 <= args.workers <= 4, 'Use one to four bounded encoder workers')
    inputs = {lang: tiles.private_path(getattr(args, lang), 'Private source pack') for lang in ('eng', 'zhs')}
    output = tiles.private_path(args.output, 'Private output directory')
    cache = tiles.private_path(args.cache, 'Private encoder cache')
    tiles.require(len(set(inputs.values())) == 2 and all(output / ('ui9-' + lang + '.nsp')
                  not in inputs.values() for lang in inputs), 'Never overwrite input packs')
    start, reports = time.monotonic(), {}
    for language, path in inputs.items():
        result, receipt = repack(path.read_bytes(), workers=args.workers, cache_directory=cache)
        tiles.atomic_bytes(output / ('ui9-' + language + '.nsp'), result)
        reports[language] = receipt
        print(json.dumps(dict(language=language, **receipt)), flush=True)
    report = dict(schema=1, reference='v0.111.0 / 41cef1ea', languages=reports,
        elapsed_seconds=time.monotonic() - start, encoder_cache=str(cache),
        note='No pixel, alpha, original frame or timing loss; cold decode performs one bounded inverse transform')
    tiles.atomic_bytes(output / 'compact-tiles-receipt.json', json.dumps(report, indent=2).encode() + b'\n')


if __name__ == '__main__':
    main()
