#!/usr/bin/env python3
"""Exact key/copy-patch geometry for the five existing selection loops.

The original RGB565/alpha tile streams are copied byte for byte. Only index
geometry changes: every eighth frame is a checkpoint relative to the opaque
first frame; other frames name source pixels which differ from their predecessor.
The renderer restores a base, a checkpoint and at most seven time patches.
No image, source sample, particle, animation clock or color bit is discarded.
"""
from __future__ import annotations

import argparse
import json
import hashlib
import lzma
from pathlib import Path
import struct

import numpy as np

import coalesce_tile_resources as coalesce
import content_tiles as t
from patch_frontend_assets import patch_pack
from tile_prediction import index_transform
from tile_filter import filter_tiles, restore_tiles

FIRST_CLIP, FIRST_BASE, CHARACTERS = 1705, 1500, 5
BASE_AUXILIARY, AUXILIARY, KEY_INTERVAL = 7206, 7211, 8
WIDTH, HEIGHT = 320, 240
RESIDENT_PAGES = (1600, 2000, 2000, 1600, 1600)


def pixels(source, reader, image):
    width, height, fmt, raw = (reader.image_raw(image) if source.images[image][3] == 6
                               else source.image_raw(image))
    t.require((width, height) == (WIDTH, HEIGHT) and fmt == 4,
              'Selection source must be a native sparse frame')
    _, color, alpha = t.validate_sparse(raw, width, height, pixels=True)
    return color, alpha


class AddedTiles:
    def __init__(self, reader):
        self.reader, self.records, self.seen = reader, [], {}

    @property
    def count(self):
        return (self.reader.tile_groups*self.reader.per_group+len(self.records)
                if self.records else self.reader.tile_count)

    def add(self, record):
        digest = hashlib.sha256(record).digest()
        for previous in self.seen.get(digest, ()):
            if self.records[previous] == record:
                return self.reader.tile_groups*self.reader.per_group+previous
        index = len(self.records); self.records.append(record)
        self.seen.setdefault(digest, []).append(index)
        return self.reader.tile_groups*self.reader.per_group+index


def indices(source, reader, image, added):
    offset, width, height, fmt, _length = source.images[image]
    t.require((width, height) == (WIDTH, HEIGHT), 'Selection tile dimensions changed')
    if fmt != 6:
        color, alpha = pixels(source, reader, image)
        ids = []
        for y in range(0, HEIGHT, 16):
            for x in range(0, WIDTH, 16):
                ids.append(added.add(b'\x10\x10'+color[y:y+16, x:x+16].astype('<u2').tobytes()+
                                     alpha[y:y+16, x:x+16].tobytes()))
        return np.array(ids, dtype='<u4')
    _global, local, at, size = t.DESCRIPTOR.unpack_from(source.data, offset)
    raw, starts = reader.group(reader.tile_groups + local)
    t.require(starts.get(at) == size, 'Selection descriptor is not a whole record')
    _decoded, count, _geometry = t.FRAME.unpack_from(raw, at)
    t.require(count == (WIDTH // 16) * (HEIGHT // 16) * 4, 'Selection tile grid changed')
    return np.frombuffer(raw, dtype='<u4', count=count // 4, offset=at + 12).copy()


def runs(mask):
    rows, histogram, decoded = [], np.zeros(WIDTH+1, dtype='i8'), HEIGHT*2
    for row in mask:
        edges = np.diff(np.pad(row.astype('i1'), (1, 1)))
        starts, ends = np.flatnonzero(edges == 1), np.flatnonzero(edges == -1)
        rows.append((starts, ends))
        decoded += len(starts)*5+int(np.sum(ends-starts))*2
        if len(starts)>1:
            histogram += np.bincount(starts[1:]-ends[:-1], minlength=WIDTH+1)
    # Joining a gap copies its unchanged pixels, saving one five-byte span.
    # This exact cost permits a single budgeted choice over all source frames.
    costs = decoded+np.cumsum(histogram*(np.arange(WIDTH+1)*2-5))
    return rows, costs


def geometry(rows, gap):
    out, decoded = bytearray(), 0
    for starts, ends in rows:
        if len(starts)>1:
            separate = starts[1:]-ends[:-1]>gap
            starts, ends = starts[np.r_[True, separate]], ends[np.r_[separate, True]]
        out.extend(struct.pack('<H', len(starts))); decoded += 2
        for start, end in zip(starts, ends):
            out.extend(struct.pack('<HHB', int(start), int(end-start), 0))
            decoded += 5 + 2 * int(end-start)
    return bytes(out), decoded


def plan_frames(source, reader):
    frames, reports, added = {}, [], AddedTiles(reader)
    for character in range(CHARACTERS):
        first, count, *_ = source.clips[FIRST_CLIP+character]
        base = source.aux[FIRST_BASE+character]
        base_color, base_alpha = pixels(source, reader, base)
        base_ids = indices(source, reader, base, added)
        base_grid = base_alpha.reshape(HEIGHT//16, 16, WIDTH//16, 16).transpose(0, 2, 1, 3)
        t.require(np.all((base_grid == 0).all(axis=(2, 3)) |
                         (base_grid == 255).all(axis=(2, 3))),
                  'Static source planes no longer cover whole opaque tiles')
        static = (base_grid == 255).all(axis=(2, 3)).reshape(-1)
        previous, initial, hashes, pages, old_pages, planned, costs = None, None, [], [], [], [], []
        for frame in range(count):
            image = first+frame
            color, alpha = pixels(source, reader, image)
            t.require(not np.any((base_alpha != 0) & (alpha != 0)),
                      'Static and moving source planes overlap')
            full = np.where(base_alpha != 0, base_color, color).astype('<u2')
            t.require(np.all(np.where(base_alpha != 0, base_alpha, alpha) == 255),
                      'A selection frame is not opaque')
            ids = np.where(static, base_ids, indices(source, reader, image, added)).astype('<u4')
            selected = (np.ones(full.shape, dtype=bool) if frame == 0 else
                        full != (initial if frame % KEY_INTERVAL == 0 else previous))
            row_runs, sizes = runs(selected)
            planned.append((image, ids, row_runs, t.sha256(full.tobytes())))
            costs.append((sizes+4095)//4096)
            old_raw = reader.image_raw(image)[3] if source.images[image][3] == 6 else source.image_raw(image)[3]
            old_pages.append((len(old_raw)+4095)//4096)
            if initial is None:
                initial = full
            previous = full
        total_pages = np.sum(costs, axis=0)
        eligible = np.flatnonzero((np.arange(WIDTH+1)>=2) & (total_pages<=RESIDENT_PAGES[character]))
        t.require(len(eligible)>0, 'Exact selection loop exceeds its fixed decoded-image budget')
        gap = int(eligible[-1])
        for image, ids, row_runs, sha in planned:
            spans, decoded = geometry(row_runs, gap)
            active = coalesce.active_tiles(spans, WIDTH, HEIGHT)
            # Inactive indices still name an existing full-size tile. Reuse a
            # live reference so obsolete color tiles can later be reclaimed.
            ids[~active] = ids[np.flatnonzero(active)[0]] if np.any(active) else ids[0]
            raw_ids = ids.tobytes()
            record = t.FRAME.pack(decoded, len(raw_ids), len(spans)) + raw_ids + spans
            frames[image] = dict(raw=record, decoded=decoded, full_sha256=sha)
            hashes.append(frames[image]['full_sha256'])
            pages.append((decoded+4095)//4096)
        reports.append(dict(character=character, frames=count, key_interval=KEY_INTERVAL,
            frame_rgb565_sha256=hashes, decoded_pages=sum(pages), original_decoded_pages=sum(old_pages),
            maximum_frame_pages=max(pages), maximum_replay_patches=KEY_INTERVAL-1,
            maximum_seek_images=KEY_INTERVAL+1, merged_gap_pixels=gap,
            decoded_page_budget=RESIDENT_PAGES[character]))
        print(json.dumps(dict(stage='selection_geometry', **{k:v for k,v in reports[-1].items()
                                                             if k != 'frame_rgb565_sha256'})), flush=True)
    return frames, reports, added


def rewrite(source, frames, added):
    """Append bounded index groups, then drop unreferenced old descriptors/groups."""
    reader = t.TileReader(source)
    old_descriptors = {}
    for image, (offset, _w, _h, fmt, _size) in enumerate(source.images):
        if fmt == 6 and image not in frames:
            old_descriptors[offset] = t.DESCRIPTOR.unpack_from(source.data, offset)
    live = sorted({row[1] for row in old_descriptors.values()})
    remap = {old: new for new, old in enumerate(live)}
    tile_groups = []
    for first in range(0, len(added.records), reader.per_group):
        records = added.records[first:first+reader.per_group]; raw = b''.join(records)
        filtered = filter_tiles(raw, 2)
        compressed = lzma.compress(filtered, format=lzma.FORMAT_RAW, filters=[dict(
            id=lzma.FILTER_LZMA2, preset=6, dict_size=reader.dictionary, lc=4, lp=0, pb=1)])
        t.require(restore_tiles(t.decode_lzma2(compressed, len(raw), reader.dictionary), 2) == raw,
                  'Source plane tile compression is not exact')
        tile_groups.append(t.GROUP.pack(b'TGR2', 1, reader.tile_groups+len(tile_groups), len(raw),
            len(compressed), 0, len(records), 2)+compressed)
    tile_group_count = reader.tile_groups+len(tile_groups)
    groups, pending, size, locations = [], [], 0, {}

    def finish():
        nonlocal pending, size
        if not pending:
            return
        raw = b''.join(frames[image]['raw'] for image in pending)
        transformed = index_transform(raw, len(pending), 3, tile_count=added.count)
        compressed = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
            id=lzma.FILTER_LZMA2, preset=6, dict_size=reader.dictionary, lc=4, lp=0, pb=1)])
        t.require(t.decode_lzma2(compressed, len(raw), reader.dictionary) == transformed,
                  'Selection index compression is not exact')
        local, at = len(live)+len(groups), 0
        for image in pending:
            locations[image] = (local, at, len(frames[image]['raw']))
            at += len(frames[image]['raw'])
        groups.append(t.GROUP.pack(b'TGI2', 2, tile_group_count+local, len(raw), len(compressed),
                                   0, len(pending), 1)+compressed)
        pending, size = [], 0

    for image in sorted(frames):
        count = len(frames[image]['raw'])
        if pending and (len(pending) == t.INDEX_FRAMES or size+count > t.GROUP_LIMIT):
            finish()
        t.require(count <= t.GROUP_LIMIT, 'A selection geometry record exceeds its bound')
        pending.append(image); size += count
    finish()
    header = list(t.GLOBAL.unpack_from(source.data, source.base_end))
    old_cost = header[14]
    header[4], header[5] = added.count, tile_group_count
    header[6], header[7] = len(live)+len(groups), len(old_descriptors)+len(frames)
    header[9] = source.base_end+64
    header[14] = header[9]+(tile_group_count+header[6])*8
    header[8] = header[14]+((len(source.images)+1)&~1)
    t.require(header[8] <= t.DIRECTORY_LIMIT, 'Selection directory exceeds fixed metadata reservation')
    out = bytearray(source.data[:source.base_end]); out.extend(t.GLOBAL.pack(*header))
    out.extend(bytes((tile_group_count+header[6])*8))
    out.extend(source.block(old_cost, (len(source.images)+1)&~1))
    descriptor_at = {}
    for offset, (_global, group, at, count) in sorted(old_descriptors.items()):
        descriptor_at[offset] = len(out)
        out.extend(t.DESCRIPTOR.pack(source.base_end, remap[group], at, count))
    image_at = {}
    for image, (group, at, count) in locations.items():
        image_at[image] = len(out)
        out.extend(t.DESCRIPTOR.pack(source.base_end, group, at, count))
        out[header[14]+image] = (frames[image]['decoded']+4095)//4096

    def add(payload):
        out.extend(bytes(len(out)&1)); at = len(out); out.extend(payload); return at

    def group(payload, number):
        payload = bytearray(payload); struct.pack_into('<I', payload, 8, number)
        at = add(payload); struct.pack_into('<II', out, header[9]+number*8, at, len(payload))

    for number in range(reader.tile_groups):
        at, count, *_ = reader.groups[number]
        group(source.block(at-32, count+32), number)
    for number, payload in enumerate(tile_groups):
        group(payload, reader.tile_groups+number)
    for number, old in enumerate(live):
        at, count, *_ = reader.groups[reader.tile_groups+old]
        group(source.block(at-32, count+32), tile_group_count+number)
    for number, payload in enumerate(groups):
        group(payload, tile_group_count+len(live)+number)
    copied, slices = {}, {}

    def copy(at, count):
        key = at, count
        if key not in copied:
            copied[key] = add(source.block(at, count))
        return copied[key]

    def slice_copy(offset):
        if offset not in slices:
            relative, size, raw_at, shape = struct.unpack_from('<iIII', source.data, offset)
            target = copy(offset+relative, size); at = add(bytes(16))
            struct.pack_into('<iIII', out, at, target-at, size, raw_at, shape); slices[offset] = at
        return slices[offset]

    for image, (offset, width, height, fmt, length) in enumerate(source.images):
        at = (image_at[image] if image in frames else descriptor_at[offset] if fmt == 6
              else slice_copy(offset) if fmt == 7 else copy(offset, length))
        if image in frames:
            fmt, length = 6, 16
        t.IMAGE.pack_into(out, 40+image*16, at, width, height, fmt, length)
    for index, (cp, offset, band, width, height, advance, encoded) in enumerate(source.glyphs):
        size = (encoded & ~t.GLYPH_GROUP_FLAG) or width*height
        at = slice_copy(offset) if encoded & t.GLYPH_GROUP_FLAG else copy(offset, size)
        t.GLYPH.pack_into(out, source.glyph_at+index*16, cp, at, band, width, height, advance, encoded)
    for index, (offset, length) in enumerate(source.strings):
        at = slice_copy(offset) if length & t.STRING_GROUP_FLAG else copy(offset, length+1)
        struct.pack_into('<II', out, source.string_at+index*8, at, length)
    struct.pack_into('<I', out, 12, len(out)); struct.pack_into('<I', out, 36, 0)
    return bytes(out), dict(original_index_groups=reader.index_groups,
        copied_index_groups=len(live), added_index_groups=len(groups),
        tile_streams_copied_byte_exact=reader.tile_groups,
        new_exact_source_tiles=len(added.records), new_source_tile_bytes=sum(map(len, tile_groups)),
        source_bytes=len(source.data), geometry_bytes=len(out),
        geometry_byte_delta=len(out)-len(source.data))


def verify(source, result, reports):
    target = coalesce.view(result); reader = t.TileReader(target)
    t.require(source.clips == target.clips and source.cards == target.cards and
              source.aux == target.aux[:len(source.aux)], 'Selection changed a stable clock or catalog')
    checked = 0
    for row in reports:
        first, count, *_ = target.clips[FIRST_CLIP+row['character']]
        canvas = np.empty((HEIGHT, WIDTH), dtype='<u2'); base = None
        for frame in range(count):
            color, alpha = pixels(target, reader, first+frame)
            if frame == 0:
                t.require(np.all(alpha == 255), 'Selection key has transparent pixels')
                base = color.copy()
            elif frame % KEY_INTERVAL == 0:
                canvas[:] = base
            t.require(np.all((alpha == 0) | (alpha == 255)), 'Selection patch is not opaque')
            canvas[alpha != 0] = color[alpha != 0]
            t.require(t.sha256(canvas.tobytes()) == row['frame_rgb565_sha256'][frame],
                      'Reconstructed selection frame differs from its source RGB565')
            checked += 1
    return dict(exact_reconstructed_frames=checked, key_interval=KEY_INTERVAL,
                maximum_seek_patches=KEY_INTERVAL-1, maximum_seek_images=KEY_INTERVAL+1,
                lossless=True, runtime_arena_growth=0)


def convert(data, work):
    """Produce the exact selection representation for a logical private pack."""
    source = coalesce.view(data)
    t.require(source.version == 45 and len(source.aux) == BASE_AUXILIARY,
              'Selection conversion requires the complete UI22 source catalog')
    frames, reports, added = plan_frames(source, t.TileReader(source))
    output, receipt = rewrite(source, frames, added)
    language = source.string_payload(6110)[:-1].decode('ascii')
    output, registration = patch_pack(output, dict(images=[], clips=[], localized_text={language:{}},
        auxiliary_image_ids=[source.clips[FIRST_CLIP+i][0] for i in range(CHARACTERS)]), Path(work),
        base_aux=BASE_AUXILIARY, auxiliary=AUXILIARY, input_version=45, version=45, texts=(),
        base_clips=len(source.clips), base_strings=len(source.strings))
    proof = verify(source, output, reports)
    receipt.update(**proof, loops=reports, registration=registration, output_bytes=len(output),
                   input_sha256=t.sha256(data), output_sha256=t.sha256(output))
    return output, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    t.private_path(args.output, 'Private selection output')
    output, receipt = convert(args.input.read_bytes(), args.output.parent)
    t.atomic_bytes(args.output, output)
    t.atomic_bytes(args.output.with_suffix('.json'), json.dumps(receipt, indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('loops', 'registration')}), flush=True)


if __name__ == '__main__':
    main()
