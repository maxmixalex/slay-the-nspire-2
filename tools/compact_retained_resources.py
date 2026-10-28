#!/usr/bin/env python3
"""Compact private retained artwork with the existing RGR1/LZMA2 reader.

Tiles and frame-index streams stay byte-identical. Small UI controls are grouped
by nearby auxiliary slots, card art in eight-card cohorts, and animation frames
only with their own clip. Large base backgrounds remain independent. The tool
compares regrouping with preserving all existing retained groups, choosing the
smaller complete result; it never drops an image, changes a pixel, or truncates
an animation. Inputs are never overwritten.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
import hashlib
import json
import lzma
from pathlib import Path
import struct
import time

import content_tiles as tiles
from repack_retained import DESCRIPTOR, auxiliary_count
import split_content


VERSIONS = tuple(range(20, 32))
ENCODER = b'compact-retained-purpose-v1-lzma2-9-273-512'


@dataclass
class Record:
    fmt: int
    raw: bytes
    source_id: int
    category: tuple
    limit: int
    ids: list = field(default_factory=list)
    existing: int | None = None


@dataclass
class Batch:
    records: list
    encoded: bytes | None = None


def view(data):
    return tiles.PackView(data, auxiliary_count(data), versions=VERSIONS)


def categories(source):
    result = {}
    # A small static control should not pull a half-megabyte room stream.
    for key, image in enumerate(source.aux):
        result.setdefault(image, ((1, key // 16), 64 * 1024))
    for kind, row in enumerate(source.cards):
        result[row[0]] = ((0, kind // 8), 128 * 1024)
    for clip, (first, count, *_rest) in enumerate(source.clips):
        for image in range(first, first + count):
            result.setdefault(image, ((2, clip), 512 * 1024))
    for image in range(min(66, len(source.images))):
        result[image] = ((3, image), tiles.GROUP_LIMIT)
    return result


def records(source):
    purposes, by_raw, result = categories(source), {}, []
    for image, (off, _w, _h, fmt, size) in enumerate(source.images):
        if fmt == 6:
            continue
        _width, _height, inner, raw = source.image_raw(image)
        raw = bytes(raw)
        category, limit = purposes.get(image, ((4, image // 16), 128 * 1024))
        key = inner, raw
        record = by_raw.get(key)
        if record is None:
            record = Record(inner, raw, image, category, limit)
            result.append(record)
            by_raw[key] = record
        elif category < record.category:
            record.category, record.limit = category, limit
        if fmt == 7 and record.existing is None:
            # Keep the image id, not an English absolute offset: a localized
            # glyph directory moves all otherwise-identical artwork bytes.
            record.existing = image
        record.ids.append(image)
    return sorted(result, key=lambda row: (row.category, min(row.ids)))


def batches(records, preserve_existing):
    result, pending, size, previous = [], [], 0, None
    for record in records:
        identity = record.category
        separate = preserve_existing and record.existing is not None
        if pending and (identity != previous or size + len(record.raw) > record.limit or separate):
            result.append(Batch(pending)); pending, size = [], 0
        if separate or len(record.raw) > tiles.GROUP_LIMIT:
            result.append(Batch([record])); previous = None
            continue
        pending.append(record); size += len(record.raw); previous = identity
    if pending:
        result.append(Batch(pending))
    return result


def encode(batch, source, cache, preserve_existing):
    if preserve_existing and len(batch.records) == 1 and batch.records[0].existing is not None:
        return batch
    raw = b''.join(record.raw for record in batch.records)
    if len(raw) > tiles.GROUP_LIMIT:
        return batch
    key = hashlib.sha256(ENCODER + raw).hexdigest()
    path = cache / (key + '.lzma2')
    candidate = None
    if path.is_file():
        candidate = path.read_bytes()
        tiles.require(tiles.decode_lzma2(candidate, len(raw)) == raw, 'Cached retained bytes differ')
    else:
        for lc, lp, pb in ((3, 0, 2), (3, 0, 0), (4, 0, 1)):
            encoded = lzma.compress(raw, format=lzma.FORMAT_RAW,
                filters=[dict(id=lzma.FILTER_LZMA2, preset=9, dict_size=tiles.GROUP_LIMIT,
                              nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
            tiles.require(tiles.decode_lzma2(encoded, len(raw)) == raw, 'Retained compression differs')
            if candidate is None or len(encoded) < len(candidate):
                candidate = encoded
        tiles.atomic_bytes(path, candidate)
    previous = sum((source.images[record.source_id][4] + 1) & ~1 for record in batch.records)
    # Existing descriptors are not the size of their shared compressed group.
    # Compare complete output alternatives below, rather than treating their
    # sixteen-byte descriptors as an implausibly cheap standalone encoding.
    if any(record.existing is not None for record in batch.records) or \
       ((32 + len(candidate) + 1) & ~1) + 16 * len(batch.records) < previous:
        batch.encoded = candidate
    return batch


def write(source, plan):
    tiled = tiles.TileReader(source)
    descriptor_end = tiled.descriptors + tiled.frame_count * 16
    out = bytearray(source.data[:descriptor_end])
    header = tiles.GLOBAL.unpack_from(source.data, source.base_end)
    cost_at, group_table = header[14], header[9]
    copied_groups = {}
    counts = dict(groups=0, grouped_images=0, decoded_bytes=0, largest_group=0,
                  independent_literal_images=0, preserved_existing_groups=0)

    def append(payload):
        out.extend(bytes(len(out) & 1)); at = len(out); out.extend(payload); return at

    def picture(record, off, fmt, size):
        for image in record.ids:
            _old, w, h, _fmt, _size = source.images[image]
            tiles.IMAGE.pack_into(out, 40 + image * 16, off, w, h, fmt, size)
            out[cost_at + image] = (len(record.raw) + 4095) // 4096

    for batch in plan:
        if batch.encoded is not None:
            raw_size = sum(len(record.raw) for record in batch.records)
            payload = tiles.GROUP.pack(b'RGR1', raw_size, len(batch.encoded), 14,
                                       len(batch.records), 0, 0, 0) + batch.encoded
            group = append(payload); raw_at = 0
            for record in batch.records:
                at = (len(out) + 1) & ~1
                desc = append(DESCRIPTOR.pack(group - at, len(payload), raw_at,
                                             (record.fmt << 24) | len(record.raw)))
                picture(record, desc, 7, 16); raw_at += len(record.raw)
            counts['groups'] += 1; counts['grouped_images'] += sum(len(r.ids) for r in batch.records)
            counts['decoded_bytes'] += raw_size; counts['largest_group'] = max(counts['largest_group'], raw_size)
            continue
        for record in batch.records:
            if record.existing is not None:
                old_descriptor = source.images[record.existing][0]
                relative, size, raw_at, shape = DESCRIPTOR.unpack_from(source.data, old_descriptor)
                old_group = old_descriptor + relative
                if old_group not in copied_groups:
                    copied_groups[old_group] = append(source.block(old_group, size))
                    counts['preserved_existing_groups'] += 1
                at = (len(out) + 1) & ~1
                desc = append(DESCRIPTOR.pack(copied_groups[old_group] - at, size, raw_at, shape))
                picture(record, desc, 7, 16)
            else:
                off, _w, _h, fmt, size = source.images[record.source_id]
                picture(record, append(source.block(off, size)), fmt, size)
                counts['independent_literal_images'] += len(record.ids)
    # Copy every original compressed animation and index group byte-for-byte.
    for group, (off, size, _raw, _crc, _kind, _count) in enumerate(tiled.groups):
        payload = source.block(off - 32, size + 32)
        at = append(payload)
        struct.pack_into('<II', out, group_table + group * 8, at, size + 32)
    glyphs, strings = {}, {}
    for index, (cp, off, px, w, h, advance, encoded) in enumerate(source.glyphs):
        raw = bytes(source.block(off, encoded or w * h))
        if raw not in glyphs:
            glyphs[raw] = append(raw)
        tiles.GLYPH.pack_into(out, source.glyph_at + index * 16, cp, glyphs[raw], px, w, h, advance, encoded)
    for index, (off, size) in enumerate(source.strings):
        raw = bytes(source.block(off, size + 1))
        if raw not in strings:
            strings[raw] = append(raw)
        struct.pack_into('<II', out, source.string_at + index * 8, strings[raw], size)
    struct.pack_into('<I', out, 12, len(out)); struct.pack_into('<I', out, 36, 0)
    return bytes(out), counts


def verify(before, data):
    after = view(data)
    tiles.require(before.counts == after.counts and before.clips == after.clips and
                  before.cards == after.cards and before.aux == after.aux, 'Catalog or timing changed')
    old, new = tiles.TileReader(before), tiles.TileReader(after)
    tiles.require(old.frame_count == new.frame_count and len(old.groups) == len(new.groups), 'Tile catalog changed')
    for (a, n, *_rest), (b, m, *_tail) in zip(old.groups, new.groups):
        tiles.require(n == m and before.block(a - 32, n + 32) == after.block(b - 32, m + 32),
                      'Original tile/index compressed stream changed')
    retained = 0
    for image, (a, b) in enumerate(zip(before.images, after.images)):
        tiles.require(a[1:3] == b[1:3], 'Image dimensions changed')
        if a[3] == 6:
            tiles.require(b[3:] == a[3:] and before.block(a[0], 16) == after.block(b[0], 16),
                          'Tiled frame registration changed')
        else:
            tiles.require(before.image_raw(image) == after.image_raw(image), 'Retained decoded pixels/geometry differ')
            retained += 1
    for a, b in zip(before.glyphs, after.glyphs):
        tiles.require(a[:1] + a[2:] == b[:1] + b[2:] and
                      before.block(a[1], a[6] or a[3] * a[4]) == after.block(b[1], b[6] or b[3] * b[4]),
                      'Glyph changed')
    for a, b in zip(before.strings, after.strings):
        tiles.require(a[1] == b[1] and before.block(a[0], a[1] + 1) == after.block(b[0], b[1] + 1), 'Text changed')
    return dict(retained_images_exact=retained, tiled_images_exact=len(before.images) - retained,
                tile_and_index_streams_byte_identical=len(old.groups),
                glyphs_exact=len(before.glyphs), strings_exact=len(before.strings),
                clip_timing_exact=True, alpha_and_geometry_exact=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--eng', type=Path, required=True)
    parser.add_argument('--zhs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    args = parser.parse_args()
    output, cache = tiles.private_path(args.output, 'Output'), tiles.private_path(args.cache, 'Cache')
    inputs = {language: tiles.private_path(getattr(args, language), 'Input') for language in ('eng', 'zhs')}
    tiles.require(1 <= args.workers <= 4 and output != cache, 'Invalid workers/output/cache')
    for path in inputs.values():
        tiles.require(output not in path.parents and cache not in path.parents, 'Input overlaps output/cache')
    output.mkdir(parents=True, exist_ok=True); cache.mkdir(parents=True, exist_ok=True)
    source = {language: view(path.read_bytes()) for language, path in inputs.items()}
    layouts = {language: split_content.layout(pack.data, language) for language, pack in source.items()}
    art = lambda pack, row: bytes(pack.data[row['prefix']:row['prefix'] + row['artwork']])
    tiles.require(art(source['eng'], layouts['eng']) == art(source['zhs'], layouts['zhs']), 'Localized artwork differs')
    tiles.require(source['eng'].clips == source['zhs'].clips and source['eng'].aux == source['zhs'].aux and
                  source['eng'].cards == source['zhs'].cards, 'Localized catalog differs')
    start = time.monotonic(); images = records(source['eng']); alternatives = []
    for preserve in (True, False):
        plan = batches(images, preserve)
        with ThreadPoolExecutor(max_workers=args.workers) as pool:
            plan = list(pool.map(lambda batch: encode(batch, source['eng'], cache, preserve), plan))
        data, counts = write(source['eng'], plan)
        alternatives.append((len(data), preserve, plan, counts))
        print(json.dumps(dict(stage='candidate', preserve_existing=preserve, bytes=len(data), **counts)), flush=True)
    _size, preserve, plan, counts = min(alternatives, key=lambda row: row[0])
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea', codec='existing RGR1/LZMA2',
                   grouping='8-card cohorts; 16 auxiliary slots/64KiB; own animation clip/512KiB; independent base images',
                   preserve_existing_retained_groups=preserve, counts=counts, alternatives=[dict(bytes=n, preserve_existing=p) for n,p,*_ in alternatives], languages={})
    for language in ('eng', 'zhs'):
        data, _ = write(source[language], plan)
        tiles.require(len(data) < len(source[language].data), 'Compaction did not reduce complete resource size')
        proof = verify(source[language], data)
        path = output / ('ui9-' + language + '.nsp'); tiles.atomic_bytes(path, data)
        receipt['languages'][language] = dict(file=str(path), input=str(inputs[language]),
            input_sha256=tiles.sha256(source[language].data), output_sha256=tiles.sha256(data),
            original_bytes=len(source[language].data), output_bytes=len(data),
            saved_bytes=len(source[language].data) - len(data), **proof)
    receipt['seconds'] = time.monotonic() - start; receipt['complete'] = True
    (output / 'compact-retained-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt), flush=True)


if __name__ == '__main__':
    main()
