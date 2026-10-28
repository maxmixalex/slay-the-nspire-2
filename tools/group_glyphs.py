#!/usr/bin/env python3
"""Losslessly group private NSP2 glyph payloads into bounded NSP2/32 blocks.

Glyph keys, native sizes, advances, alpha, existing raw/RLE encoding and every
artwork byte remain exact. Physical payload order preserves the preceding
frequency/locality pass. Only groups that save bytes including their descriptor
cost are used. The live glyph/string tail is rebuilt to discard obsolete copies.
The runtime reuses its existing RGR1 cache and glyph cache; no new pool is needed.
"""
from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass, field
import json
import lzma
import mmap
from pathlib import Path
import struct
import time

import content_tiles as tiles
from repack_retained import auxiliary_count
import split_content

VERSION = 32
FLAG = 0x8000
DESCRIPTOR = struct.Struct('<iIII')
DEFAULT_GROUP_BYTES = 32 * 1024
GROUP_CHOICES = (32 * 1024, 64 * 1024)
VERSIONS = tuple(range(20, VERSION + 1))


def aligned(value):
    return (value + 1) & ~1


def view(data):
    return tiles.PackView(data, auxiliary_count(data), versions=VERSIONS)


def language_of(source):
    tiles.require(len(source.strings) > split_content.MARKER, 'Missing locale marker')
    at, length = source.strings[split_content.MARKER]
    marker = bytes(source.block(at, length)).decode('ascii')
    tiles.require(marker in split_content.LANGUAGES, 'Unsupported locale marker')
    return marker


@dataclass
class Record:
    payload: bytes
    ids: list = field(default_factory=list)


@dataclass
class Batch:
    records: list
    encoded: bytes | None
    raw_size: int
    original_size: int


def plan(source, group_bytes=DEFAULT_GROUP_BYTES, *, order='physical'):
    tiles.require(group_bytes in GROUP_CHOICES, 'Glyph groups must be 32 or 64 KiB')
    tiles.require(order in ('physical', 'band-frequency'), 'Unknown glyph grouping order')
    records, identical = [], {}
    # Directory order remains sorted by key. The payload order was already
    # optimized for the locale's frequent characters and should not be undone.
    for glyph_id in sorted(range(len(source.glyphs)), key=lambda index: (source.glyphs[index][1], index)):
        payload = bytes(source.glyph_payload(glyph_id))
        record = identical.get(payload)
        if record is None:
            record = Record(payload)
            identical[payload] = record
            records.append(record)
        record.ids.append(glyph_id)
    if order == 'band-frequency':
        # Rendering measures every candidate size from resident metadata,
        # but requests pixels only at its chosen face/size. Keep those pixels
        # together instead of loading all sizes of every common character.
        # Decode old payloads once in physical order before sorting records;
        # otherwise a large locale repeatedly decompresses its old groups.
        frequency = Counter()
        for index in range(len(source.strings)):
            frequency.update(bytes(source.string_payload(index)[:-1]).decode('utf-8'))
        def priority(record):
            return min((source.glyphs[index][2], -frequency[chr(source.glyphs[index][0])],
                        source.glyphs[index][0], index) for index in record.ids)
        records.sort(key=priority)
    batches, pending, raw_size = [], [], 0

    def finish():
        nonlocal pending, raw_size
        if not pending:
            return
        raw = b''.join(record.payload for record in pending)
        candidate = lzma.compress(raw, format=lzma.FORMAT_RAW,
            filters=[dict(id=lzma.FILTER_LZMA2, preset=6, dict_size=tiles.GROUP_LIMIT)])
        tiles.require(tiles.decode_lzma2(candidate, len(raw)) == raw, 'Glyph codec roundtrip differs')
        previous = sum(aligned(len(record.payload)) for record in pending)
        stored = aligned(tiles.GROUP.size + len(candidate)) + DESCRIPTOR.size * len(pending)
        batches.append(Batch(pending, candidate if stored < previous else None, raw_size, previous))
        pending, raw_size = [], 0

    for record in records:
        if pending and raw_size + len(record.payload) > group_bytes:
            finish()
        pending.append(record)
        raw_size += len(record.payload)
    finish()
    return batches


def write(source, batches, bounds):
    art_end = bounds['prefix'] + bounds['artwork']
    out = bytearray(source.data[:art_end])

    def append(payload):
        out.extend(bytes(len(out) & 1))
        offset = len(out)
        out.extend(payload)
        return offset

    def glyphs(record, offset, grouped):
        for index in record.ids:
            cp, _old, px, width, height, advance, encoded = source.glyphs[index]
            tiles.GLYPH.pack_into(out, source.glyph_at + index * tiles.GLYPH.size,
                cp, offset, px, width, height, advance, (encoded & ~FLAG) | (FLAG if grouped else 0))

    grouped_entries = grouped_records = group_count = maximum_raw = maximum_encoded = 0
    payload_bytes = descriptor_bytes = raw_entries = 0
    unique_payloads = sum(len(batch.records) for batch in batches)
    for batch in batches:
        if batch.encoded is None:
            for record in batch.records:
                glyphs(record, append(record.payload), False)
                payload_bytes += aligned(len(record.payload))
                raw_entries += len(record.ids)
            continue
        group = tiles.GROUP.pack(b'RGR1', batch.raw_size, len(batch.encoded), 14,
            len(batch.records), 0, 0, 0) + batch.encoded
        group_at = append(group)
        raw_at = 0
        for record in batch.records:
            descriptor_at = aligned(len(out))
            descriptor = DESCRIPTOR.pack(group_at - descriptor_at, len(group), raw_at, len(record.payload))
            tiles.require(append(descriptor) == descriptor_at, 'Glyph descriptor alignment differs')
            glyphs(record, descriptor_at, True)
            raw_at += len(record.payload)
            grouped_entries += len(record.ids)
        tiles.require(raw_at == batch.raw_size, 'Glyph group coverage differs')
        grouped_records += len(batch.records)
        descriptor_bytes += DESCRIPTOR.size * len(batch.records)
        payload_bytes += aligned(len(group))
        maximum_raw = max(maximum_raw, batch.raw_size)
        maximum_encoded = max(maximum_encoded, len(group))
        group_count += 1
    glyph_end = len(out)
    strings = {}
    for index in range(len(source.strings)):
        payload = bytes(source.string_payload(index))
        if payload not in strings:
            strings[payload] = append(payload)
        struct.pack_into('<II', out, source.string_at + index * 8, strings[payload], len(payload)-1)
    tiles.require(len(out) <= tiles.FILE_LIMIT, 'Grouped glyph pack exceeds file bound')
    struct.pack_into('<I', out, 4, VERSION)
    struct.pack_into('<I', out, 12, len(out))
    struct.pack_into('<I', out, 36, 0)
    return bytes(out), dict(groups=group_count, unique_payloads=unique_payloads,
        grouped_unique_payloads=grouped_records, grouped_glyph_entries=grouped_entries,
        ungrouped_glyph_entries=raw_entries, descriptor_bytes=descriptor_bytes,
        glyph_payload_bytes=payload_bytes, glyph_tail_bytes=glyph_end - art_end,
        string_tail_bytes=len(out) - glyph_end, maximum_group_raw_bytes=maximum_raw,
        maximum_group_encoded_bytes=maximum_encoded,
        previous_unique_glyph_bytes=sum(batch.original_size for batch in batches))


def verify(original, result):
    source, target = view(original), view(result)
    language = language_of(source)
    old, new = split_content.layout(original, language), split_content.layout(result, language)
    tiles.require(target.version == VERSION and source.counts == target.counts,
        'Glyph pack version or counts differ')
    tiles.require((old['prefix'], old['artwork']) == (new['prefix'], new['artwork']),
        'Artwork boundaries changed')
    begin, end = old['prefix'], old['prefix'] + old['artwork']
    tiles.require(original[begin:end] == result[begin:end], 'Artwork bytes changed')
    tiles.require(source.images == target.images and source.clips == target.clips and
        source.cards == target.cards and source.aux == target.aux, 'Image/animation/model registration changed')
    # These untouched directory intervals also cover the TIL1 header, costs,
    # frame descriptors and alignment bytes before the artwork payloads.
    tiles.require(original[40:source.glyph_at] == result[40:target.glyph_at] and
        original[source.clip_at:begin] == result[target.clip_at:begin],
        'Non-font directory bytes changed')
    alpha_bytes = 0
    for index, (before, after) in enumerate(zip(source.glyphs, target.glyphs)):
        tiles.require(before[:1] + before[2:6] == after[:1] + after[2:6] and
            before[6] & ~FLAG == after[6] & ~FLAG, 'Glyph key, geometry or old encoding changed')
        tiles.require(source.glyph_payload(index) == target.glyph_payload(index), 'Glyph payload bytes changed')
        alpha = source.glyph_alpha(index)
        tiles.require(alpha == target.glyph_alpha(index), 'Glyph antialias alpha changed')
        alpha_bytes += len(alpha)
    for before, after in zip(source.strings, target.strings):
        tiles.require(before[1] == after[1] and
            source.block(before[0], before[1] + 1) == target.block(after[0], after[1] + 1),
            'Localized string bytes changed')
    return dict(glyphs_verified=len(source.glyphs), alpha_bytes_compared=alpha_bytes,
        strings_verified=len(source.strings), exact_payload_and_alpha=True,
        exact_keys_sizes_advances=True, exact_artwork=True, exact_image_and_animation_indices=True,
        exact_localized_strings=True, artwork_sha256=tiles.sha256(original[begin:end]))


def repack(data, group_bytes=DEFAULT_GROUP_BYTES):
    source = view(data)
    language = language_of(source)
    bounds = split_content.layout(data, language)
    batches = plan(source, group_bytes)
    result, statistics = write(source, batches, bounds)
    proof = verify(data, result)
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea', language=language,
        input_version=source.version, output_version=VERSION, input_bytes=len(data),
        output_bytes=len(result), saved_bytes=len(data) - len(result),
        input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(result),
        group_target_bytes=group_bytes, codec_raw_bound_bytes=tiles.GLYPH_GROUP_LIMIT,
        order='existing_physical_payload_order', resident_cache_growth_bytes=0,
        startup_font_decompression=False, **statistics, verification=proof)
    return result, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    parser.add_argument('--group-bytes', type=int, choices=GROUP_CHOICES, default=DEFAULT_GROUP_BYTES)
    args = parser.parse_args()
    source = tiles.private_path(args.input, 'Input')
    target = tiles.private_path(args.output, 'Output')
    receipt_path = tiles.private_path(args.receipt, 'Receipt')
    tiles.require(len({source, target, receipt_path}) == 3, 'Input, output and receipt must differ')
    started = time.monotonic()
    with source.open('rb') as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
        result, receipt = repack(data, args.group_bytes)
    receipt['elapsed_seconds'] = round(time.monotonic() - started, 3)
    # Publication follows complete key/pixel/art readback; a failure leaves the
    # previous output untouched. The input is never modified in place.
    tiles.atomic_bytes(target, result)
    tiles.atomic_bytes(receipt_path, json.dumps(receipt, indent=2, ensure_ascii=False).encode() + b'\n')
    print(json.dumps(receipt, ensure_ascii=False))


if __name__ == '__main__':
    main()
