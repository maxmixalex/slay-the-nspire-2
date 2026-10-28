#!/usr/bin/env python3
"""Store exact localized strings in bounded NSP2/33 RGR1 groups.

String ids and UTF-8 bytes do not change. The locale marker remains raw for
constant-cost boot probing. Existing artwork and grouped glyph bytes remain
untouched. The runtime uses the existing 102 KiB string cache and group/transfer
workspaces; only cold demanded strings are decoded.
"""
from __future__ import annotations

import argparse
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

VERSION = 33
FLAG = 0x80000000
GROUP_BYTES = 32768
DESCRIPTOR = struct.Struct('<iIII')


def aligned(value):
    return (value + 1) & ~1


def view(data):
    return tiles.PackView(data, auxiliary_count(data), versions=tuple(range(20, VERSION + 1)))


def locale(source):
    marker = source.string_payload(split_content.MARKER)[:-1].decode('ascii')
    tiles.require(marker in split_content.LANGUAGES, 'Unsupported locale marker')
    return marker


def nonstring_end(source, bounds):
    end = bounds['prefix'] + bounds['artwork']
    for _cp, off, _px, width, height, _advance, encoded in source.glyphs:
        if encoded & tiles.GLYPH_GROUP_FLAG:
            relative, count = struct.unpack_from('<iI', source.data, off)
            end = max(end, off + 16, off + relative + count)
        else:
            end = max(end, off + (encoded or width * height))
    for off, length in source.strings:
        first = off
        if length & FLAG:
            relative = struct.unpack_from('<i', source.data, off)[0]
            first = min(first, off + relative)
        tiles.require(first >= end, 'Strings interleave live glyphs; rebuild the glyph tail first')
    return end


@dataclass
class Record:
    payload: bytes
    ids: list = field(default_factory=list)


@dataclass
class Batch:
    records: list
    encoded: bytes | None
    raw_size: int
    previous_size: int


def plan(source):
    identical, records = {}, []
    for index in sorted(range(len(source.strings)), key=lambda i: (source.strings[i][0], i)):
        raw = bytes(source.string_payload(index))
        record = identical.get(raw)
        if record is None:
            record = Record(raw);identical[raw] = record;records.append(record)
        record.ids.append(index)
    batches, pending, count = [], [], 0

    def finish():
        nonlocal pending, count
        if not pending:
            return
        raw = b''.join(record.payload for record in pending)
        encoded = lzma.compress(raw, format=lzma.FORMAT_RAW,
            filters=[dict(id=lzma.FILTER_LZMA2, preset=6, dict_size=tiles.GROUP_LIMIT)])
        tiles.require(tiles.decode_lzma2(encoded, len(raw)) == raw, 'String compression roundtrip differs')
        before = sum(aligned(len(record.payload)) for record in pending)
        after = aligned(len(encoded) + tiles.GROUP.size) + len(pending) * DESCRIPTOR.size
        batches.append(Batch(pending, encoded if after < before else None, count, before))
        pending, count = [], 0

    for record in records:
        if split_content.MARKER in record.ids:
            finish();batches.append(Batch([record], None, len(record.payload), aligned(len(record.payload))));continue
        if pending and count + len(record.payload) > GROUP_BYTES:
            finish()
        pending.append(record);count += len(record.payload)
    finish()
    return batches


def write(source, batches, prefix_end):
    out = bytearray(source.data[:prefix_end])

    def append(payload):
        out.extend(bytes(len(out) & 1));at = len(out);out.extend(payload);return at

    def directory(record, offset, grouped):
        length = len(record.payload) - 1
        for index in record.ids:
            struct.pack_into('<II', out, source.string_at + index * 8,
                offset, length | (FLAG if grouped else 0))

    groups = grouped_entries = grouped_records = maximum_raw = maximum_encoded = 0
    for batch in batches:
        if batch.encoded is None:
            for record in batch.records:
                directory(record, append(record.payload), False)
            continue
        group = tiles.GROUP.pack(b'RGR1', batch.raw_size, len(batch.encoded), 14,
            len(batch.records), 0, 0, 0) + batch.encoded
        group_at = append(group);raw_at = 0
        for record in batch.records:
            descriptor_at = aligned(len(out))
            actual = append(DESCRIPTOR.pack(group_at - descriptor_at, len(group), raw_at, len(record.payload)))
            tiles.require(actual == descriptor_at, 'String descriptor alignment differs')
            directory(record, actual, True);raw_at += len(record.payload)
            grouped_entries += len(record.ids)
        tiles.require(raw_at == batch.raw_size, 'String group coverage differs')
        groups += 1;grouped_records += len(batch.records)
        maximum_raw = max(maximum_raw, batch.raw_size);maximum_encoded = max(maximum_encoded, len(group))
    tiles.require(len(out) <= tiles.FILE_LIMIT, 'Grouped string pack exceeds file bound')
    struct.pack_into('<I', out, 4, VERSION);struct.pack_into('<I', out, 12, len(out));struct.pack_into('<I', out, 36, 0)
    return bytes(out), dict(groups=groups, grouped_string_entries=grouped_entries,
        grouped_unique_strings=grouped_records, unique_strings=sum(len(b.records) for b in batches),
        previous_unique_string_bytes=sum(b.previous_size for b in batches),
        string_tail_bytes=len(out) - prefix_end, maximum_group_raw_bytes=maximum_raw,
        maximum_group_encoded_bytes=maximum_encoded)


def verify(original, result):
    source, target = view(original), view(result)
    language = locale(source)
    old, new = split_content.layout(original, language), split_content.layout(result, language)
    end = nonstring_end(source, old)
    tiles.require(target.version == VERSION and source.counts == target.counts and
        source.glyphs == target.glyphs and source.images == target.images and
        source.clips == target.clips and source.cards == target.cards and source.aux == target.aux,
        'String conversion changed a font/image/model index')
    tiles.require((old['prefix'], old['artwork']) == (new['prefix'], new['artwork']) and
        end == nonstring_end(target, new), 'Non-string boundaries changed')
    tiles.require(original[40:source.string_at] == result[40:target.string_at] and
        original[source.clip_at:end] == result[target.clip_at:end],
        'Existing artwork, glyph groups or non-string directory bytes changed')
    compared = 0
    for i in range(len(source.strings)):
        a, b = source.string_payload(i), target.string_payload(i)
        tiles.require(a == b and source.string_length(i) == target.string_length(i),
            'Localized UTF-8 bytes or string index changed')
        compared += len(a)
    tiles.require(not target.strings[split_content.MARKER][1] & FLAG and locale(target) == language,
        'The boot locale marker must stay literal')
    return dict(strings_verified=len(source.strings), utf8_bytes_compared=compared,
        exact_utf8_and_indices=True, glyph_payloads_and_keys_unchanged=True,
        artwork_and_animation_unchanged=True, literal_boot_marker=True)


def repack(data):
    source = view(data);language = locale(source);bounds = split_content.layout(data, language)
    result, statistics = write(source, plan(source), nonstring_end(source, bounds))
    proof = verify(data, result)
    return result, dict(schema=1, reference='v0.111.0 / 41cef1ea', language=language,
        input_version=source.version, output_version=VERSION, input_bytes=len(data),
        output_bytes=len(result), saved_bytes=len(data) - len(result),
        input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(result),
        group_target_bytes=GROUP_BYTES, resident_cache_growth_bytes=0,
        startup_string_decompression=False, **statistics, verification=proof)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('input', 'output', 'receipt'):
        parser.add_argument('--' + option, type=Path, required=True)
    args = parser.parse_args()
    source = tiles.private_path(args.input, 'Input');target = tiles.private_path(args.output, 'Output')
    receipt_path = tiles.private_path(args.receipt, 'Receipt')
    tiles.require(len({source, target, receipt_path}) == 3, 'Input, output and receipt must differ')
    started = time.monotonic()
    with source.open('rb') as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
        result, receipt = repack(data)
    receipt['elapsed_seconds'] = round(time.monotonic() - started, 3)
    tiles.atomic_bytes(target, result)
    tiles.atomic_bytes(receipt_path, json.dumps(receipt, indent=2, ensure_ascii=False).encode() + b'\n')
    print(json.dumps(receipt, ensure_ascii=False))


if __name__ == '__main__':
    main()
