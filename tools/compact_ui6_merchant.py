#!/usr/bin/env python3
"""Remove proved unreachable old merchant blocks after the UI6 atlas repair.

The pre-repair and corrected packs are both required. Only fully unreferenced
old image payloads/descriptors and retained groups are removed. No image is
recompressed, no glyph is reordered and no current shared slice is discarded.
All current images are independently decoded again after offset relocation.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
import json
from pathlib import Path
import struct

import content_tiles as tiles
from repack_retained import DESCRIPTOR

VERSION, AUXILIARY, CLIPS = 23, 1320, 1705


def view(data, auxiliary):
    return tiles.PackView(data, auxiliary, versions=(VERSION,), profile=(558, 8192, CLIPS, auxiliary))


def art_end(pack):
    return min([row[1] for row in pack.glyphs] + [row[0] for row in pack.strings])


def merge_ranges(ranges):
    result = []
    for begin, end in sorted(ranges):
        if result and begin <= result[-1][1]:
            result[-1] = (result[-1][0], max(end, result[-1][1]))
        else:
            result.append((begin, end))
    return result


def references(pack, tiled):
    """Every byte range reachable through a current runtime directory."""
    ranges = [(0, tiled.descriptors + tiled.frame_count * tiles.DESCRIPTOR.size)]
    for offset, _width, _height, fmt, length in pack.images:
        ranges.append((offset, offset + length))
        if fmt == 7:
            relative, size, _raw_at, _shape = DESCRIPTOR.unpack_from(pack.data, offset)
            ranges.append((offset + relative, offset + relative + size))
    header = tiles.GLOBAL.unpack_from(pack.data, pack.base_end)
    for index in range(tiled.tile_groups + tiled.index_groups):
        offset, length = struct.unpack_from('<II', pack.data, header[9] + index * 8)
        ranges.append((offset, offset + length))
    ranges.extend((offset, offset + (encoded or width * height))
                  for _cp, offset, _px, width, height, _advance, encoded in pack.glyphs)
    ranges.extend((offset, offset + length + 1) for offset, length in pack.strings)
    return merge_ranges(ranges)


def removal_plan(original, corrected, old_tiles, current_tiles):
    tiles.require(original.counts == corrected.counts and original.cards == corrected.cards and
                  original.clips == corrected.clips and original.aux == corrected.aux,
                  'Merchant compaction requires the same stable catalog')
    original_end = art_end(original)
    protected = old_tiles.descriptors + old_tiles.frame_count * tiles.DESCRIPTOR.size
    tiles.require(original_end <= art_end(corrected) and
                  original.data[protected:original_end] == corrected.data[protected:original_end],
                  'Corrected pack does not preserve the original historical artwork')
    changed = {index for index, (old, new) in enumerate(zip(original.images, corrected.images)) if old != new}
    allowed = {original.aux[1261]}
    for clip in range(1695, 1703):
        first, count, *_ = original.clips[clip]
        allowed.update(range(first, first + count))
    # Synthetic fixtures may exercise the same offset algorithm with an
    # explicitly smaller auxiliary profile. Production repair stays scoped.
    if original.counts[-1] == AUXILIARY:
        tiles.require(changed <= allowed, 'Unrelated image replacement outside merchant correction')
    owners, groups = {}, {}
    for image_id, (offset, _width, _height, fmt, length) in enumerate(original.images):
        if fmt != 6:
            owners.setdefault((offset, length), set()).add(image_id)
        if fmt == 7:
            relative, length, raw_at, shape = DESCRIPTOR.unpack_from(original.data, offset)
            group = groups.setdefault((offset + relative, length), {})
            group.setdefault((raw_at, shape & 0xffffff), set()).add(image_id)
    candidates = [(offset, offset + length) for (offset, length), ids in owners.items() if ids <= changed]
    shared_decoded = 0
    for (offset, length), slices in groups.items():
        if set().union(*slices.values()) <= changed:
            candidates.append((offset, offset + length))
        else:
            shared_decoded += sum(size for (_at, size), ids in slices.items() if ids <= changed)
    live = references(corrected, current_tiles)
    starts = [begin for begin, _end in live]

    def unused(begin, end):
        index = bisect_right(starts, end - 1) - 1
        return index < 0 or live[index][1] <= begin

    removed, bytes_without_padding = [], 0
    for begin, end in merge_ranges(candidates):
        tiles.require(protected <= begin < end <= original_end and begin % 2 == 0,
                      'Unreachable candidate crosses a protected boundary')
        tiles.require(unused(begin, end), 'Old merchant candidate is still referenced by current content')
        bytes_without_padding += end - begin
        if end & 1:
            tiles.require(end < original_end and corrected.data[end] == 0 and unused(end, end + 1),
                          'Cannot remove odd payload without consuming referenced alignment byte')
            end += 1
        removed.append((begin, end))
    return merge_ranges(removed), dict(changed_image_entries=len(changed),
        unreachable_payload_bytes=bytes_without_padding,
        preserved_unreachable_decoded_slices_in_shared_groups=shared_decoded)


class Relocation:
    def __init__(self, ranges):
        self.ranges = ranges
        self.ends = [end for _begin, end in ranges]
        self.prefix = [0]
        for begin, end in ranges:
            self.prefix.append(self.prefix[-1] + end - begin)

    def __call__(self, offset):
        count = bisect_right(self.ends, offset)
        tiles.require(count == len(self.ranges) or offset < self.ranges[count][0],
                      'Attempt to relocate an offset inside removed artwork')
        return offset - self.prefix[count]


def compact(original_data, corrected_data, *, auxiliary=AUXILIARY):
    original, current = view(original_data, auxiliary), view(corrected_data, auxiliary)
    old_tiles, current_tiles = tiles.TileReader(original), tiles.TileReader(current)
    ranges, planned = removal_plan(original, current, old_tiles, current_tiles)
    relocate = Relocation(ranges)
    out, previous = bytearray(), 0
    for begin, end in ranges:
        out.extend(corrected_data[previous:begin])
        previous = end
    out.extend(corrected_data[previous:])
    for index, (offset, width, height, fmt, length) in enumerate(current.images):
        tiles.IMAGE.pack_into(out, 40 + index * tiles.IMAGE.size,
                             relocate(offset), width, height, fmt, length)
    for index, (cp, offset, px, width, height, advance, encoded) in enumerate(current.glyphs):
        tiles.GLYPH.pack_into(out, current.glyph_at + index * tiles.GLYPH.size,
                             cp, relocate(offset), px, width, height, advance, encoded)
    for index, (offset, length) in enumerate(current.strings):
        struct.pack_into('<II', out, current.string_at + index * 8, relocate(offset), length)
    header = list(tiles.GLOBAL.unpack_from(corrected_data, current.base_end))
    old_table = header[9]
    for index in (8, 9, 14):
        header[index] = relocate(header[index])
    tiles.GLOBAL.pack_into(out, relocate(current.base_end), *header)
    for index in range(current_tiles.tile_groups + current_tiles.index_groups):
        offset, length = struct.unpack_from('<II', corrected_data, old_table + index * 8)
        struct.pack_into('<II', out, relocate(old_table + index * 8), relocate(offset), length)
    for index in range(current_tiles.frame_count):
        offset = current_tiles.descriptors + index * tiles.DESCRIPTOR.size
        global_at, group, record_at, record_size = tiles.DESCRIPTOR.unpack_from(corrected_data, offset)
        tiles.DESCRIPTOR.pack_into(out, relocate(offset), relocate(global_at), group, record_at, record_size)
    descriptors = set()
    for offset, _width, _height, fmt, _length in current.images:
        if fmt != 7 or offset in descriptors:
            continue
        descriptors.add(offset)
        relative, size, raw_at, shape = DESCRIPTOR.unpack_from(corrected_data, offset)
        new_offset = relocate(offset)
        DESCRIPTOR.pack_into(out, new_offset, relocate(offset + relative) - new_offset, size, raw_at, shape)
    struct.pack_into('<I', out, 12, len(out))
    struct.pack_into('<I', out, 36, 0)
    target = view(out, auxiliary)
    target_tiles = tiles.TileReader(target)
    tiles.require(target.counts == current.counts and target.clips == current.clips and
                  target.cards == current.cards and target.aux == current.aux, 'Compaction changed stable catalog')
    verified = 0
    for image_id, (old, new) in enumerate(zip(current.images, target.images)):
        tiles.require(new == (relocate(old[0]), *old[1:]), 'Compacted image directory differs')
        a = current_tiles.image_raw(image_id) if old[3] == 6 else current.image_raw(image_id)
        b = target_tiles.image_raw(image_id) if new[3] == 6 else target.image_raw(image_id)
        tiles.require(a == b, 'Compacted image decoded pixels/alpha differ: ' + str(image_id))
        if old[3] == 7:
            relative, size, raw_at, shape = DESCRIPTOR.unpack_from(corrected_data, old[0])
            next_relative, next_size, next_at, next_shape = DESCRIPTOR.unpack_from(out, new[0])
            tiles.require((next_size, next_at, next_shape) == (size, raw_at, shape) and
                          corrected_data[old[0] + relative:old[0] + relative + size] ==
                          out[new[0] + next_relative:new[0] + next_relative + size],
                          'Current retained group encoding changed')
        elif old[3] != 6:
            tiles.require(corrected_data[old[0]:old[0] + old[4]] == out[new[0]:new[0] + new[4]],
                          'Current image encoding changed')
        verified += 1
        if verified % 10000 == 0:
            print(json.dumps(dict(stage='verify_compacted_images', complete=verified, total=len(current.images))), flush=True)
    for old, new in zip(current.glyphs, target.glyphs):
        tiles.require(new == (old[0], relocate(old[1]), *old[2:]) and
                      corrected_data[old[1]:old[1] + (old[6] or old[3] * old[4])] ==
                      out[new[1]:new[1] + (new[6] or new[3] * new[4])], 'Compaction changed glyph order, metrics or bytes')
    for old, new in zip(current.strings, target.strings):
        tiles.require(new == (relocate(old[0]), old[1]) and
                      corrected_data[old[0]:old[0] + old[1] + 1] == out[new[0]:new[0] + new[1] + 1],
                      'Compaction changed localized text')
    for index in range(current_tiles.tile_groups + current_tiles.index_groups):
        offset, length = struct.unpack_from('<II', corrected_data, old_table + index * 8)
        tiles.require(corrected_data[offset:offset + length] == out[relocate(offset):relocate(offset) + length],
                      'Compaction changed a tile/index group encoding')
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea',
        original_sha256=tiles.sha256(original_data), corrected_sha256=tiles.sha256(corrected_data),
        output_sha256=tiles.sha256(out), original_bytes=len(original_data),
        corrected_bytes=len(corrected_data), output_bytes=len(out),
        removed_bytes=len(corrected_data) - len(out), removed_ranges=len(ranges),
        image_entries=verified, all_images_decoded_equal=True, all_current_encoded_payloads_equal=True,
        unchanged_glyph_order=True, unchanged_localized_text=True, unchanged_tile_ids=True,
        unchanged_clip_geometry_and_timing=True, runtime_buffer_growth=0,
        relative_retained_pointers_relocated=len(descriptors), **planned)
    return bytes(out), receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--corrected', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    original = tiles.private_path(args.original, 'Pre-repair pack')
    corrected = tiles.private_path(args.corrected, 'Corrected pack')
    output = tiles.private_path(args.output, 'Compacted pack')
    receipt_path = tiles.private_path(args.receipt, 'Compaction receipt')
    tiles.require(len({original, corrected, output, receipt_path}) == 4, 'Never overwrite a verified input pack')
    data, receipt = compact(original.read_bytes(), corrected.read_bytes())
    tiles.atomic_bytes(output, data)
    tiles.atomic_bytes(receipt_path, json.dumps(receipt, indent=2).encode() + b'\n')
    print(json.dumps(receipt), flush=True)


if __name__ == '__main__':
    main()
