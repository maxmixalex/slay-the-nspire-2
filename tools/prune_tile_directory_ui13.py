#!/usr/bin/env python3
"""Remove unreferenced tile-index groups/descriptors without changing pixels.

Replacement conversions deliberately retain the old index stream. This pass
copies every live compressed tile/index group verbatim, drops only whole index
groups with no remaining image reference, and remaps live descriptors. No
source canvas, pivot, sample, codec, animation clock or runtime cache changes.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import time

import content_tiles as t
from retile_appended_resources import view, emit


def live_directory(source):
    reader = t.TileReader(source);descriptors = {}
    for offset, _width, _height, fmt, _length in source.images:
        if fmt != 6:continue
        t.require(offset >= reader.descriptors and (offset-reader.descriptors) % 16 == 0 and
                  (offset-reader.descriptors)//16 < reader.frame_count,
                  'An image refers outside the original frame directory')
        entry = t.DESCRIPTOR.unpack_from(source.data, offset)
        t.require(entry[0] == source.base_end and entry[1] < reader.index_groups,
                  'A live descriptor refers outside the original index catalog')
        descriptors[offset] = entry
    t.require(bool(descriptors), 'No live tile frames to preserve')
    groups = sorted({entry[1] for entry in descriptors.values()})
    return reader, dict(sorted(descriptors.items())), groups


def prune(data):
    source = view(data);old, descriptors, live = live_directory(source)
    remap = {group: index for index, group in enumerate(live)}
    old_global = t.GLOBAL.unpack_from(data, source.base_end)
    cost_size = (len(source.images)+1) & ~1
    t.require(old_global[14] > 0 and old_global[15] == len(source.images),
              'Pruning requires the existing per-image cost directory')
    table = source.base_end+64
    cost_at = table+(old.tile_groups+len(live))*8
    descriptor_at = cost_at+cost_size
    header = list(old_global)
    header[6], header[7], header[8], header[9], header[14] = len(live), len(descriptors), descriptor_at, table, cost_at
    out = bytearray(data[:source.base_end]);out.extend(t.GLOBAL.pack(*header))
    out.extend(bytes((old.tile_groups+len(live))*8))
    out.extend(source.block(old_global[14], cost_size))
    locations = {}
    for offset, (global_at, group, at, size) in descriptors.items():
        locations[offset] = len(out)
        out.extend(t.DESCRIPTOR.pack(global_at, remap[group], at, size))
    t.require(len(out) == descriptor_at+len(descriptors)*16, 'Compacted directory layout differs')

    def add(payload):
        out.extend(bytes((-len(out)) % 2));offset = len(out);out.extend(payload);return offset

    def group(previous, index):
        offset, size, *_ = old.groups[previous]
        payload = bytearray(source.block(offset-32, size+32))
        struct.pack_into('<I', payload, 8, index)
        new_at = add(payload);struct.pack_into('<II', out, table+index*8, new_at, len(payload))

    for index in range(old.tile_groups):group(index, index)
    for index, previous in enumerate(live):group(old.tile_groups+previous, old.tile_groups+index)
    copied, slices = {}, {}

    def copy(offset, size):
        key = offset, size
        if key not in copied:copied[key] = add(source.block(offset, size))
        return copied[key]

    def slice_copy(offset):
        if offset not in slices:
            relative, size, raw_at, shape = struct.unpack_from('<iIII', data, offset)
            target = copy(offset+relative, size);new_at = add(bytes(16))
            struct.pack_into('<iIII', out, new_at, target-new_at, size, raw_at, shape)
            slices[offset] = new_at
        return slices[offset]

    for image, (offset, width, height, fmt, length) in enumerate(source.images):
        new_at = locations[offset] if fmt == 6 else slice_copy(offset) if fmt == 7 else copy(offset, length)
        t.IMAGE.pack_into(out, 40+image*16, new_at, width, height, fmt, length)
    artwork_end = len(out)
    for index, (cp, offset, px, width, height, advance, encoded_size) in enumerate(source.glyphs):
        count = (encoded_size & ~t.GLYPH_GROUP_FLAG) or width*height
        new_at = slice_copy(offset) if encoded_size & t.GLYPH_GROUP_FLAG else copy(offset, count)
        t.GLYPH.pack_into(out, source.glyph_at+index*16, cp, new_at, px, width, height, advance, encoded_size)
    for index, (offset, length) in enumerate(source.strings):
        new_at = slice_copy(offset) if length & t.STRING_GROUP_FLAG else copy(offset, length+1)
        struct.pack_into('<II', out, source.string_at+index*8, new_at, length)
    struct.pack_into('<I', out, 12, len(out));struct.pack_into('<I', out, 36, 0)
    t.require(len(out) <= len(data), 'Pruning must not increase physical size')
    return bytes(out), dict(input_bytes=len(data), output_bytes=len(out), saved_bytes=len(data)-len(out),
        old_descriptors=old.frame_count, descriptors=len(descriptors),
        removed_descriptors=old.frame_count-len(descriptors), old_index_groups=old.index_groups,
        index_groups=len(live), removed_index_groups=old.index_groups-len(live),
        tile_groups=old.tile_groups, artwork_end=artwork_end)


def verify(source, output):
    target = view(output);old, descriptors, live = live_directory(source)
    new = t.TileReader(target);mapping = {group: index for index, group in enumerate(live)}
    t.require(source.counts == target.counts and source.clips == target.clips and
              source.cards == target.cards and source.aux == target.aux,
              'Pruning changed the content catalog or animation registration')
    t.require(new.tile_count == old.tile_count and new.tile_groups == old.tile_groups and
              new.index_groups == len(live) and new.frame_count == len(descriptors),
              'Pruned tile/index directory counts differ')
    # Equal compressed streams, filters and raw shapes prove every tile and
    # live index record is identical without a lossy re-encode or a heuristic.
    for old_group, new_group in [(i, i) for i in range(old.tile_groups)]+[
            (old.tile_groups+group, new.tile_groups+index) for index, group in enumerate(live)]:
        a, b = old.groups[old_group], new.groups[new_group]
        t.require(a[1:] == b[1:] and old.filters[old_group] == new.filters[new_group] and
                  source.block(a[0], a[1]) == target.block(b[0], b[1]),
                  'A live compressed group or its decode shape changed')
    tiled = retained = 0
    for image, (a, b) in enumerate(zip(source.images, target.images)):
        t.require(a[1:] == b[1:], 'An image changed dimensions, format or stored length')
        if a[3] == 6:
            before = t.DESCRIPTOR.unpack_from(source.data, a[0]);after = t.DESCRIPTOR.unpack_from(output, b[0])
            t.require(after == (target.base_end, mapping[before[1]], before[2], before[3]),
                      'An image was remapped to a different index record')
            tiled += 1
        else:
            t.require(source.image_raw(image) == target.image_raw(image), 'A retained image changed')
            retained += 1
    for index, (a, b) in enumerate(zip(source.glyphs, target.glyphs)):
        t.require(a[:1]+a[2:] == b[:1]+b[2:] and source.glyph_payload(index) == target.glyph_payload(index),
                  'A glyph changed during directory pruning')
    for index in range(len(source.strings)):
        t.require(source.string_payload(index) == target.string_payload(index), 'A localized string changed')
    return dict(tiled_images_exact=tiled, retained_images_exact=retained,
        glyphs_exact=len(source.glyphs), strings_exact=len(source.strings),
        all_image_dimensions_pivots_samples_and_clocks_exact=True,
        copied_tile_groups_exact=new.tile_groups, copied_index_groups_exact=new.index_groups)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    for path in [*args.input, args.output]:t.private_path(path, 'Private tile-directory prune path')
    t.require(all(path.resolve().parent != args.output.resolve() for path in args.input), 'Preserve unpruned source packs')
    args.output.mkdir(parents=True, exist_ok=True);started = time.monotonic();reports = []
    for path in args.input:
        data = path.read_bytes();output, sizes = prune(data);checked = verify(view(data), output)
        target = args.output/path.name;t.atomic_bytes(target, output)
        reports.append(dict(path=str(target), sha256=t.sha256(output), **sizes, **checked))
        emit('tile_directory_pruned', **reports[-1])
    receipt = dict(elapsed_seconds=round(time.monotonic()-started, 3), codec_unchanged=True, outputs=reports)
    t.atomic_bytes(args.output/'tile-directory-prune-receipt.json', json.dumps(receipt, indent=2).encode()+b'\n')


if __name__ == '__main__':main()
