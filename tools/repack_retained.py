#!/usr/bin/env python3
"""Losslessly group private NSP2/19 retained images into bounded NSP2/20 blocks.

The existing animation groups are copied. Image pixels, palette entries, alpha,
scanline geometry, frame order and timing are preserved byte for byte. English
and Chinese reuse one grouping plan so their artwork remains shareable.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass, field
import json
import lzma
from pathlib import Path
import struct
import time

import content_tiles as tiles

VERSION = 20
DESCRIPTOR = struct.Struct('<iIII')
DEFAULT_GROUP_BYTES = 128 * 1024


def auxiliary_count(data):
    ni, ng, ns, nc, nk = struct.unpack_from('<5I', data, 16)
    at = start = 40 + ni * 16 + ng * 16 + ns * 8 + nc * 16 + nk * 16
    bound = min(len(data), tiles.DIRECTORY_LIMIT)
    while at + tiles.GLOBAL.size <= bound and data[at:at + 4] != b'TIL1':
        tiles.require((at - start) // 4 < 65535 and struct.unpack_from('<I', data, at)[0] < ni,
                      'Invalid auxiliary catalog')
        at += 4
    tiles.require(at > start and at + tiles.GLOBAL.size <= bound and data[at:at + 4] == b'TIL1',
                  'Missing tile directory')
    return (at - start) // 4


def view(data):
    return tiles.PackView(data, auxiliary_count(data), versions=(19,))


def aligned(size):
    return (size + 1) & ~1


@dataclass
class Record:
    fmt: int
    raw: bytes
    stored_format: int
    payload: bytes
    ids: list = field(default_factory=list)


@dataclass
class Batch:
    records: list
    encoded: bytes | None
    raw_size: int
    previous_size: int


def plan_images(source, group_bytes=DEFAULT_GROUP_BYTES, replacements=None):
    tiles.require(4096 <= group_bytes <= tiles.GROUP_LIMIT, 'Group target must be 4..512 KiB')
    replacements = replacements or {}
    by_descriptor, by_raw, records = {}, {}, []
    for image_id, entry in enumerate(source.images):
        offset, _width, _height, stored_format, length = entry
        if stored_format == 6:
            continue
        record = by_descriptor.get(entry) if image_id not in replacements else None
        if record is None:
            if image_id in replacements:
                _width, _height, fmt, raw = replacements[image_id]
                stored_format, payload = fmt, bytes(raw)
            else:
                _width, _height, fmt, raw = source.image_raw(image_id)
                payload = bytes(source.block(offset, length))
            raw = bytes(raw)
            key = (fmt, raw)
            record = by_raw.get(key)
            if record is None:
                record = Record(fmt, raw, stored_format, payload)
                by_raw[key] = record
                records.append(record)
            elif len(payload) < len(record.payload):
                record.stored_format, record.payload = stored_format, payload
            if image_id not in replacements:
                by_descriptor[entry] = record
        record.ids.append(image_id)

    batches, pending, size = [], [], 0

    def finish():
        nonlocal pending, size
        if not pending:
            return
        previous = sum(aligned(len(record.payload)) for record in pending)
        encoded = None
        if size <= tiles.GROUP_LIMIT:
            raw = b''.join(record.raw for record in pending)
            candidate = lzma.compress(raw, format=lzma.FORMAT_RAW,
                filters=[dict(id=lzma.FILTER_LZMA2, preset=6, dict_size=tiles.GROUP_LIMIT)])
            tiles.require(tiles.decode_lzma2(candidate, size) == raw, 'Retained codec roundtrip differs')
            if aligned(tiles.GROUP.size + len(candidate)) + len(pending) * DESCRIPTOR.size < previous:
                encoded = candidate
        batches.append(Batch(pending, encoded, size, previous))
        pending, size = [], 0

    for record in records:
        if pending and size + len(record.raw) > group_bytes:
            finish()
        pending.append(record)
        size += len(record.raw)
        if size >= group_bytes:
            finish()
    finish()
    return batches


def write_pack(source, batches, replacements=None):
    replacements = replacements or {}
    tiled = tiles.TileReader(source)
    ni, _ng, _ns, _nc, _nk, _nu = source.counts
    cost_at, cost_size = tiled.prefix_end, aligned(ni)
    descriptor_end = tiled.descriptors + tiled.frame_count * tiles.DESCRIPTOR.size
    out = bytearray(source.data[:cost_at]) + bytearray(cost_size) + source.data[cost_at:descriptor_end]
    struct.pack_into('<I', out, source.base_end + 32, tiled.descriptors + cost_size)
    struct.pack_into('<II', out, source.base_end + 56, cost_at, ni)
    for image_id, (offset, width, height, fmt, length) in enumerate(source.images):
        if fmt != 6:
            continue
        tiles.IMAGE.pack_into(out, 40 + image_id * 16, offset + cost_size, width, height, fmt, length)
        _global, index_group, record_at, _record_size = tiles.DESCRIPTOR.unpack_from(source.data, offset)
        index, _starts = tiled.group(tiled.tile_groups + index_group)
        raw_size = struct.unpack_from('<I', index, record_at)[0]
        out[cost_at + image_id] = (raw_size + 4095) // 4096

    def append(payload):
        out.extend(bytes(len(out) & 1))
        offset = len(out)
        out.extend(payload)
        return offset

    def picture(record, offset, fmt, length):
        for image_id in record.ids:
            _old, width, height, _fmt, _length = source.images[image_id]
            if image_id in replacements:
                width, height = replacements[image_id][:2]
            tiles.IMAGE.pack_into(out, 40 + image_id * 16, offset, width, height, fmt, length)
            out[cost_at + image_id] = (len(record.raw) + 4095) // 4096

    group_count = grouped_images = grouped_raw = stored_bytes = maximum_raw = 0
    for batch in batches:
        if batch.encoded is None:
            for record in batch.records:
                offset = append(record.payload)
                picture(record, offset, record.stored_format, len(record.payload))
                stored_bytes += aligned(len(record.payload))
            continue
        group = tiles.GROUP.pack(b'RGR1', batch.raw_size, len(batch.encoded), 14,
                                 len(batch.records), 0, 0, 0) + batch.encoded
        group_at = append(group)
        raw_at = 0
        for record in batch.records:
            out.extend(bytes(len(out) & 1))
            descriptor_at = len(out)
            out.extend(DESCRIPTOR.pack(group_at - descriptor_at, len(group), raw_at,
                                       (record.fmt << 24) | len(record.raw)))
            picture(record, descriptor_at, 7, DESCRIPTOR.size)
            raw_at += len(record.raw)
            grouped_images += len(record.ids)
        group_count += 1
        grouped_raw += batch.raw_size
        maximum_raw = max(maximum_raw, batch.raw_size)
        stored_bytes += aligned(len(group)) + len(batch.records) * DESCRIPTOR.size

    table = source.base_end + tiles.GLOBAL.size
    for group_id, (offset, encoded, _raw, _crc, _kind, _count) in enumerate(tiled.groups):
        group = bytearray(source.block(offset - tiles.GROUP.size, encoded + tiles.GROUP.size))
        struct.pack_into('<I', group, 20, 0)
        new_at = append(group)
        struct.pack_into('<II', out, table + group_id * 8, new_at, len(group))

    # Keep all text/fonts after artwork so the common/locale boundary remains exact.
    glyphs, strings = {}, {}
    for i, (cp, offset, px, width, height, advance, encoded) in enumerate(source.glyphs):
        payload = bytes(source.block(offset, encoded or width * height))
        if payload not in glyphs:
            glyphs[payload] = append(payload)
        tiles.GLYPH.pack_into(out, source.glyph_at + i * tiles.GLYPH.size,
                             cp, glyphs[payload], px, width, height, advance, encoded)
    for i, (offset, length) in enumerate(source.strings):
        payload = bytes(source.block(offset, length + 1))
        if payload not in strings:
            strings[payload] = append(payload)
        struct.pack_into('<II', out, source.string_at + i * 8, strings[payload], length)
    tiles.require(len(out) <= tiles.FILE_LIMIT, 'Repacked file exceeds runtime bound')
    struct.pack_into('<I', out, 4, VERSION)
    struct.pack_into('<I', out, 12, len(out))
    struct.pack_into('<I', out, 36, 0)
    return bytes(out), dict(retained_groups=group_count, grouped_image_entries=grouped_images,
        grouped_raw_bytes=grouped_raw, retained_stored_bytes=stored_bytes,
        retained_unique_images=sum(len(batch.records) for batch in batches),
        maximum_group_raw_bytes=maximum_raw, tile_groups_copied=len(tiled.groups))


def verify(source, result, replacements=None):
    replacements = replacements or {}
    target = tiles.PackView(result, source.counts[-1], versions=(VERSION,))
    old, new = tiles.TileReader(source), tiles.TileReader(target)
    tiles.require(source.counts == target.counts and source.clips == target.clips and
                  source.cards == target.cards and source.aux == target.aux, 'Catalog or timing changed')
    size = old.frame_count * tiles.DESCRIPTOR.size
    tiles.require(source.data[old.descriptors:old.descriptors + size] ==
                  result[new.descriptors:new.descriptors + size], 'Tile descriptors changed')
    seen, retained, tiled = set(), 0, 0
    for image_id, entry in enumerate(source.images):
        if entry[3] == 6:
            tiles.require(target.images[image_id] == (entry[0] + aligned(len(source.images)), *entry[1:]),
                          'Tile image relocation differs')
            tiled += 1
        elif image_id in replacements:
            tiles.require(target.image_raw(image_id) == replacements[image_id], 'Replacement bytes differ')
            retained += 1
        elif entry not in seen:
            tiles.require(source.image_raw(image_id) == target.image_raw(image_id), 'Retained image bytes differ')
            seen.add(entry)
            retained += 1
    tiles.require(len(old.groups) == len(new.groups), 'Tile group count changed')
    for a, b in zip(old.groups, new.groups):
        tiles.require(a[1:3] == b[1:3] and a[4:] == b[4:] and
                      source.data[a[0]:a[0] + a[1]] == result[b[0]:b[0] + b[1]],
                      'Animation group encoded bytes changed')
    for a, b in zip(source.glyphs, target.glyphs):
        tiles.require(a[:1] + a[2:] == b[:1] + b[2:] and
                      source.block(a[1], a[6] or a[3] * a[4]) == target.block(b[1], b[6] or b[3] * b[4]),
                      'Glyph bytes or geometry changed')
    for a, b in zip(source.strings, target.strings):
        tiles.require(a[1] == b[1] and source.block(a[0], a[1] + 1) == target.block(b[0], b[1] + 1),
                      'Localized string bytes changed')
    return dict(unique_retained_images_verified=retained, tile_images_verified=tiled,
                animation_groups_verified=len(old.groups), glyphs_verified=len(source.glyphs),
                strings_verified=len(source.strings), replaced_image_ids=sorted(replacements),
                catalog_and_timing_unchanged=True, exact_raw_byte_comparison=True)


def repack(data, group_bytes=DEFAULT_GROUP_BYTES, batches=None, replacements=None):
    source = view(data)
    replacements = replacements or {}
    for image_id, (width, height, fmt, raw) in replacements.items():
        tiles.require(0 <= image_id < len(source.images) and source.images[image_id][3] != 6 and
                      0 < width <= 320 and 0 < height <= 240, 'Invalid replacement image')
        tiles.validate_raw(raw, width, height, fmt)
    if batches is None:
        batches = plan_images(source, group_bytes, replacements)
    else:
        # Reusing the same plan is what makes common artwork independent of locale offsets.
        for batch in batches:
            for record in batch.records:
                for image_id in record.ids:
                    _w, _h, fmt, raw = replacements.get(image_id) or source.image_raw(image_id)
                    tiles.require(fmt == record.fmt and raw == record.raw,
                                  'Language retained artwork differs; rebuild both languages together')
    result, statistics = write_pack(source, batches, replacements)
    proof = verify(source, result, replacements)
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea', input_version=19, output_version=VERSION,
        input_bytes=len(data), output_bytes=len(result), saved_bytes=len(data) - len(result),
        group_target_bytes=group_bytes, codec_raw_bound_bytes=tiles.GROUP_LIMIT,
        resident_group_cache_bytes=5 * 1024 * 1024, new_resident_pixel_bytes=0,
        startup_payload_scans=0, **statistics, verification=proof)
    return result, receipt, batches


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--eng', type=Path, required=True)
    parser.add_argument('--zhs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--group-kib', type=int, default=DEFAULT_GROUP_BYTES // 1024)
    parser.add_argument('--logo-atlas', type=Path)
    args = parser.parse_args()
    sources = {language: tiles.private_path(getattr(args, language), 'Input pack') for language in ('eng', 'zhs')}
    output = tiles.private_path(args.output, 'Private output directory')
    tiles.require(len(set(sources.values())) == 2, 'Both language inputs must be separate files')
    tiles.require(not any((output / ('retained-' + language + '.nsp')).resolve() in sources.values()
                          for language in sources), 'Outputs must not replace either input')
    replacements = {}
    if args.logo_atlas:
        manifest_path = tiles.private_path(args.logo_atlas, 'Private title atlas')
        logo = json.loads(manifest_path.read_text())
        raw = (manifest_path.parent / logo['raw_file']).read_bytes()
        replacements[4] = (logo['width'], logo['height'], logo['format'], raw)
    started, batches, receipts = time.monotonic(), None, {}
    for language, path in sources.items():
        result, receipt, batches = repack(path.read_bytes(), args.group_kib * 1024, batches, replacements)
        tiles.atomic_bytes(output / ('retained-' + language + '.nsp'), result)
        receipts[language] = receipt
        print(json.dumps(dict(language=language, **receipt), sort_keys=True), flush=True)
    tiles.atomic_bytes(output / 'retained-receipt.json',
        json.dumps(dict(schema=1, elapsed_seconds=time.monotonic() - started, languages=receipts), indent=2).encode() + b'\n')


if __name__ == '__main__':
    main()
