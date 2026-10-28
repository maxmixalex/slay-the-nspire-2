#!/usr/bin/env python3
"""Compress exact merchant moving planes in small existing retained groups.

The fixed/moving pixel split is unchanged. At most eight neighboring frames
share one at-most-64-KiB RGR1 decode. Group boundaries never cross a clip, and
the ordinary per-image cache retains each decoded sparse frame afterward.
This uses the existing format 7 decoder and adds no runtime buffers.
"""
from __future__ import annotations

import argparse
import json
import lzma
from pathlib import Path
import struct

import content_tiles as tiles
from merchant_ui14_assets import FIRST_CLIP, CLIP_KEYS
from repack_retained import DESCRIPTOR
from ui12_resources import replace_images, view

MAX_GROUP_BYTES = 64 * 1024
MAX_GROUP_FRAMES = 8


def build_payload(source, ranges, *, group_bytes=MAX_GROUP_BYTES, max_frames=MAX_GROUP_FRAMES):
    """Return relative image slices over one complete insertion blob.

    `ranges` contains (first image, frame count) for each separate clip. An
    independently encoded image is retained when a group would be larger.
    """
    tiles.require(0 < group_bytes <= MAX_GROUP_BYTES and 0 < max_frames <= MAX_GROUP_FRAMES,
                  'Merchant cold-decode group exceeds its measured bound')
    payload, entries, expected, groups = bytearray(), {}, {}, []
    previous_bytes = 0

    def append(raw):
        payload.extend(bytes(len(payload) & 1));at = len(payload);payload.extend(raw);return at

    def finish(records):
        nonlocal previous_bytes
        if not records:return
        raw = b''.join(record[4] for record in records)
        encoded = lzma.compress(raw, format=lzma.FORMAT_RAW,
            filters=[dict(id=lzma.FILTER_LZMA2, preset=6, dict_size=tiles.GROUP_LIMIT)])
        tiles.require(tiles.decode_lzma2(encoded, len(raw)) == raw,
                      'Merchant retained-group readback differs')
        original = sum((len(record[5])+1) & ~1 for record in records)
        previous_bytes += original
        grouped_size = ((tiles.GROUP.size+len(encoded)+1) & ~1)+len(records)*DESCRIPTOR.size
        if grouped_size >= original:
            for image, width, height, fmt, decoded, stored, stored_fmt in records:
                entries[image] = (append(stored), width, height, stored_fmt, len(stored))
            return
        block = tiles.GROUP.pack(b'RGR1', len(raw), len(encoded), 14, len(records), 0, 0, 0)+encoded
        block_at, raw_at = append(block), 0
        for image, width, height, fmt, decoded, stored, stored_fmt in records:
            at = append(bytes(DESCRIPTOR.size))
            DESCRIPTOR.pack_into(payload, at, block_at-at, len(block), raw_at, (fmt << 24)|len(decoded))
            entries[image] = (at, width, height, 7, DESCRIPTOR.size)
            raw_at += len(decoded)
        groups.append(dict(first=records[0][0], frames=len(records), raw_bytes=len(raw),
                           encoded_bytes=len(encoded), stored_bytes=grouped_size))

    for first, count in ranges:
        tiles.require(0 <= first < len(source.images) and 0 < count <= len(source.images)-first,
                      'Merchant frame range is outside the image directory')
        pending, size = [], 0
        for image in range(first, first+count):
            tiles.require(image not in expected, 'Merchant frame ranges overlap')
            offset, width, height, stored_fmt, length = source.images[image]
            tiles.require(stored_fmt <= 5, 'Merchant grouping requires independent moving planes')
            decoded = source.image_raw(image);fmt, raw = decoded[2:]
            tiles.require(fmt == 4 and 0 < len(raw) <= group_bytes,
                          'Merchant moving plane is not bounded sparse artwork')
            if pending and (len(pending) == max_frames or size+len(raw) > group_bytes):
                finish(pending);pending, size = [], 0
            expected[image] = decoded
            pending.append((image, width, height, fmt, raw, bytes(source.block(offset, length)), stored_fmt))
            size += len(raw)
        finish(pending)
    payload.extend(bytes(len(payload) & 1))
    tiles.require(entries and len(payload) <= previous_bytes, 'Merchant grouping enlarged or dropped payloads')
    report = dict(images=len(entries), retained_groups=len(groups), grouped_images=sum(g['frames'] for g in groups),
        max_group_frames=max((g['frames'] for g in groups), default=0),
        max_group_raw_bytes=max((g['raw_bytes'] for g in groups), default=0),
        group_raw_limit=group_bytes, group_frame_limit=max_frames,
        previous_frame_payload_bytes=previous_bytes, stored_frame_payload_bytes=len(payload),
        saved_frame_payload_bytes=previous_bytes-len(payload), groups=groups,
        runtime_memory_added=0, unchanged_frame_bytes=True)
    return bytes(payload), entries, expected, report


def regroup_existing(data, *, ranges=None, group_bytes=MAX_GROUP_BYTES, max_frames=MAX_GROUP_FRAMES):
    """Rewrite live references and physically remove the replaced LZ4 copies."""
    source = view(data)
    if ranges is None:
        tiles.require(len(source.clips) >= FIRST_CLIP+len(CLIP_KEYS), 'Missing complete merchant clip catalog')
        ranges = [(first, count) for first, count, *_ in source.clips[FIRST_CLIP:FIRST_CLIP+len(CLIP_KEYS)]]
    payload, entries, expected, report = build_payload(source, ranges,
        group_bytes=group_bytes, max_frames=max_frames)
    staged = bytearray(data);staged.extend(bytes(len(staged) & 1));start = len(staged);staged.extend(payload)
    for image, (offset, width, height, fmt, size) in entries.items():
        tiles.IMAGE.pack_into(staged, 40+image*16, start+offset, width, height, fmt, size)
    struct.pack_into('<I', staged, 12, len(staged));struct.pack_into('<I', staged, 36, 0)
    # This existing relocator copies only blocks referenced by the current
    # image/glyph/string directories. Old independent copies are not retained
    # behind a new descriptor, and shared foreign RGR1/RGR2 slices stay intact.
    output, _relocated = replace_images(bytes(staged), {})
    target = view(output)
    tiles.require(len(output) <= len(data), 'Merchant regrouping increased the final pack')
    for image, decoded in expected.items():
        tiles.require(target.image_raw(image) == decoded, 'Merchant grouped sparse frame changed')
    header_before = tiles.GLOBAL.unpack_from(data, source.base_end)
    header_after = tiles.GLOBAL.unpack_from(output, target.base_end)
    tiles.require(source.block(header_before[14], len(source.images)) ==
                  target.block(header_after[14], len(target.images)), 'Merchant decoded cache costs changed')
    report.update(input_bytes=len(data), output_bytes=len(output), saved_pack_bytes=len(data)-len(output),
                  unreachable_replacement_bytes=0, all_moving_frames_decoded_exactly=True,
                  source_sha256=tiles.sha256(data), output_sha256=tiles.sha256(output))
    return output, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    source = tiles.private_path(args.input, 'Private split merchant input')
    output = tiles.private_path(args.output, 'Private regrouped merchant output')
    tiles.require(source.resolve() != output.resolve(), 'Preserve the original private pack')
    packed, report = regroup_existing(source.read_bytes())
    tiles.atomic_bytes(output, packed)
    tiles.atomic_bytes(output.with_suffix('.merchant-groups.json'), json.dumps(report, indent=2).encode()+b'\n')
    print(json.dumps({key: value for key, value in report.items() if key != 'groups'}), flush=True)


if __name__ == '__main__':main()
