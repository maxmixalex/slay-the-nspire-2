#!/usr/bin/env python3
"""Exact key/copy-patch geometry for the five existing selection loops.

The original RGB565/alpha tile streams are copied byte for byte. Only index
geometry changes: every eighth frame is a complete opaque key, and the other
frames name source pixels which differ from their predecessor. The production
renderer restores at most seven patches into its existing shared scene raster.
No image, source sample, particle, animation clock or color bit is discarded.
"""
from __future__ import annotations

import argparse
import json
import lzma
from pathlib import Path
import struct

import numpy as np

import coalesce_tile_resources as coalesce
import content_tiles as t
from patch_frontend_assets import patch_pack
from tile_prediction import index_transform

FIRST_CLIP, FIRST_BASE, CHARACTERS = 1705, 1500, 5
BASE_AUXILIARY, AUXILIARY, KEY_INTERVAL = 7206, 7211, 8
WIDTH, HEIGHT = 320, 240


def pixels(source, reader, image):
    width, height, fmt, raw = (reader.image_raw(image) if source.images[image][3] == 6
                               else source.image_raw(image))
    t.require((width, height) == (WIDTH, HEIGHT) and fmt == 4,
              'Selection source must be a native sparse frame')
    _, color, alpha = t.validate_sparse(raw, width, height, pixels=True)
    return color, alpha


def indices(source, reader, image):
    offset, width, height, fmt, _length = source.images[image]
    t.require((width, height, fmt) == (WIDTH, HEIGHT, 6),
              'Tile exact source artwork before building selection patches')
    _global, local, at, size = t.DESCRIPTOR.unpack_from(source.data, offset)
    raw, starts = reader.group(reader.tile_groups + local)
    t.require(starts.get(at) == size, 'Selection descriptor is not a whole record')
    _decoded, count, _geometry = t.FRAME.unpack_from(raw, at)
    t.require(count == (WIDTH // 16) * (HEIGHT // 16) * 4, 'Selection tile grid changed')
    return np.frombuffer(raw, dtype='<u4', count=count // 4, offset=at + 12).copy()


def geometry(mask):
    """Keep short unchanged gaps when two extra pixel words cost less than a span."""
    mask = mask.copy()
    for gap in (1, 2):
        fill = mask[:, :-gap-1] & mask[:, gap+1:]
        for i in range(1, gap+1):
            mask[:, i:i+fill.shape[1]] |= fill
    out, decoded = bytearray(), 0
    for row in mask:
        edges = np.diff(np.pad(row.astype('i1'), (1, 1)))
        starts, ends = np.flatnonzero(edges == 1), np.flatnonzero(edges == -1)
        out.extend(struct.pack('<H', len(starts))); decoded += 2
        for start, end in zip(starts, ends):
            out.extend(struct.pack('<HHB', int(start), int(end-start), 0))
            decoded += 5 + 2 * int(end-start)
    return bytes(out), decoded


def plan_frames(source, reader):
    frames, reports = {}, []
    for character in range(CHARACTERS):
        first, count, *_ = source.clips[FIRST_CLIP+character]
        base = source.aux[FIRST_BASE+character]
        base_color, base_alpha = pixels(source, reader, base)
        base_ids = indices(source, reader, base)
        base_grid = base_alpha.reshape(HEIGHT//16, 16, WIDTH//16, 16).transpose(0, 2, 1, 3)
        t.require(np.all((base_grid == 0).all(axis=(2, 3)) |
                         (base_grid == 255).all(axis=(2, 3))),
                  'Static source planes no longer cover whole opaque tiles')
        static = (base_grid == 255).all(axis=(2, 3)).reshape(-1)
        previous, hashes, pages, old_pages = None, [], [], []
        for frame in range(count):
            image = first+frame
            color, alpha = pixels(source, reader, image)
            t.require(not np.any((base_alpha != 0) & (alpha != 0)),
                      'Static and moving source planes overlap')
            full = np.where(base_alpha != 0, base_color, color).astype('<u2')
            t.require(np.all(np.where(base_alpha != 0, base_alpha, alpha) == 255),
                      'A selection frame is not opaque')
            ids = np.where(static, base_ids, indices(source, reader, image)).astype('<u4')
            selected = np.ones(full.shape, dtype=bool) if frame % KEY_INTERVAL == 0 else full != previous
            spans, decoded = geometry(selected)
            active = coalesce.active_tiles(spans, WIDTH, HEIGHT)
            # Inactive indices still name an existing full-size tile. Reuse a
            # live reference so obsolete color tiles can later be reclaimed.
            if np.any(active):
                ids[~active] = ids[np.flatnonzero(active)[0]]
            raw_ids = ids.tobytes()
            record = t.FRAME.pack(decoded, len(raw_ids), len(spans)) + raw_ids + spans
            frames[image] = dict(raw=record, decoded=decoded, full_sha256=t.sha256(full.tobytes()))
            hashes.append(frames[image]['full_sha256'])
            pages.append((decoded+4095)//4096)
            old_pages.append((reader.image_raw(image)[3].__len__()+4095)//4096)
            previous = full
        reports.append(dict(character=character, frames=count, key_interval=KEY_INTERVAL,
            frame_rgb565_sha256=hashes, decoded_pages=sum(pages), original_decoded_pages=sum(old_pages),
            maximum_frame_pages=max(pages), maximum_replay_patches=KEY_INTERVAL-1))
        print(json.dumps(dict(stage='selection_geometry', **{k:v for k,v in reports[-1].items()
                                                             if k != 'frame_rgb565_sha256'})), flush=True)
    return frames, reports


def rewrite(source, frames):
    """Append bounded index groups, then drop unreferenced old descriptors/groups."""
    reader = t.TileReader(source)
    old_descriptors = {}
    for image, (offset, _w, _h, fmt, _size) in enumerate(source.images):
        if fmt == 6 and image not in frames:
            old_descriptors[offset] = t.DESCRIPTOR.unpack_from(source.data, offset)
    live = sorted({row[1] for row in old_descriptors.values()})
    remap = {old: new for new, old in enumerate(live)}
    groups, pending, size, locations = [], [], 0, {}

    def finish():
        nonlocal pending, size
        if not pending:
            return
        raw = b''.join(frames[image]['raw'] for image in pending)
        transformed = index_transform(raw, len(pending), 3, tile_count=reader.tile_count)
        compressed = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
            id=lzma.FILTER_LZMA2, preset=6, dict_size=reader.dictionary, lc=4, lp=0, pb=1)])
        t.require(t.decode_lzma2(compressed, len(raw), reader.dictionary) == transformed,
                  'Selection index compression is not exact')
        local, at = len(live)+len(groups), 0
        for image in pending:
            locations[image] = (local, at, len(frames[image]['raw']))
            at += len(frames[image]['raw'])
        groups.append(t.GROUP.pack(b'TGI2', 2, reader.tile_groups+local, len(raw), len(compressed),
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
    header[6], header[7] = len(live)+len(groups), len(old_descriptors)+len(frames)
    header[9] = source.base_end+64
    header[14] = header[9]+(reader.tile_groups+header[6])*8
    header[8] = header[14]+((len(source.images)+1)&~1)
    t.require(header[8] <= t.DIRECTORY_LIMIT, 'Selection directory exceeds fixed metadata reservation')
    out = bytearray(source.data[:source.base_end]); out.extend(t.GLOBAL.pack(*header))
    out.extend(bytes((reader.tile_groups+header[6])*8))
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
    for number, old in enumerate(live):
        at, count, *_ = reader.groups[reader.tile_groups+old]
        group(source.block(at-32, count+32), reader.tile_groups+number)
    for number, payload in enumerate(groups):
        group(payload, reader.tile_groups+len(live)+number)
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
        source_bytes=len(source.data), geometry_bytes=len(out),
        geometry_byte_delta=len(out)-len(source.data))


def verify(source, result, reports):
    target = coalesce.view(result); reader = t.TileReader(target)
    t.require(source.clips == target.clips and source.cards == target.cards and
              source.aux == target.aux[:len(source.aux)], 'Selection changed a stable clock or catalog')
    checked = 0
    for row in reports:
        first, count, *_ = target.clips[FIRST_CLIP+row['character']]
        canvas = np.empty((HEIGHT, WIDTH), dtype='<u2')
        for frame in range(count):
            color, alpha = pixels(target, reader, first+frame)
            if frame % KEY_INTERVAL == 0:
                t.require(np.all(alpha == 255), 'Selection key has transparent pixels')
            t.require(np.all((alpha == 0) | (alpha == 255)), 'Selection patch is not opaque')
            canvas[alpha != 0] = color[alpha != 0]
            t.require(t.sha256(canvas.tobytes()) == row['frame_rgb565_sha256'][frame],
                      'Reconstructed selection frame differs from its source RGB565')
            checked += 1
    return dict(exact_reconstructed_frames=checked, key_interval=KEY_INTERVAL,
                maximum_seek_patches=KEY_INTERVAL-1, lossless=True, runtime_arena_growth=0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    t.private_path(args.output, 'Private selection output')
    data = args.input.read_bytes(); source = coalesce.view(data)
    t.require(source.version == 45 and len(source.aux) == BASE_AUXILIARY,
              'Selection conversion requires the complete UI22 source catalog')
    frames, reports = plan_frames(source, t.TileReader(source))
    output, receipt = rewrite(source, frames)
    language = source.string_payload(6110)[:-1].decode('ascii')
    output, registration = patch_pack(output, dict(images=[], clips=[], localized_text={language:{}},
        auxiliary_image_ids=[source.clips[FIRST_CLIP+i][0] for i in range(CHARACTERS)]), args.output.parent,
        base_aux=BASE_AUXILIARY, auxiliary=AUXILIARY, input_version=45, version=45, texts=(),
        base_clips=len(source.clips), base_strings=len(source.strings))
    proof = verify(source, output, reports)
    receipt.update(**proof, loops=reports, registration=registration, output_bytes=len(output),
                   input_sha256=t.sha256(data), output_sha256=t.sha256(output))
    t.atomic_bytes(args.output, output)
    t.atomic_bytes(args.output.with_suffix('.json'), json.dumps(receipt, indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('loops', 'registration')}), flush=True)


if __name__ == '__main__':
    main()
