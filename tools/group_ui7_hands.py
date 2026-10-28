"""Pack newly sampled UI7 hands into bounded, lossless retained groups.

The existing format 7 reader already retains neighboring poses together. This
helper groups the 94 appended hand images and explicit new animation ranges; it neither
resamples pixels nor rewrites any historical pack payload. The caller inserts
the returned layout once, so no unreachable format 5 copies are introduced.
"""
from __future__ import annotations

from collections import OrderedDict
from pathlib import Path

import content_tiles as tiles
from repack_retained import DESCRIPTOR, DEFAULT_GROUP_BYTES, plan_images


class RowImages:
    """A small decoded-image view over checked converter outputs."""
    image_raw = tiles.PackView.image_raw

    def __init__(self, rows, payloads):
        self.version = 24
        self.cards, self.aux, self.images = [], [], []
        self.retained_cache = OrderedDict()
        data = bytearray()
        for row, payload in zip(rows, payloads):
            tiles.require(row['format'] <= 5, 'Hand input must be an independent image')
            data.extend(bytes(len(data) & 1))
            self.images.append((len(data), row['width'], row['height'], row['format'], len(payload)))
            data.extend(payload)
        self.data = bytes(data)

    def block(self, offset, size):
        tiles.require(0 <= offset <= len(self.data) and 0 <= size <= len(self.data)-offset,
                      'Appended image block outside payload')
        return self.data[offset:offset+size]


def build_payload(rows, cache, *, group_bytes=DEFAULT_GROUP_BYTES, extra_ranges=()):
    """Return one insertion blob and per-image relative descriptors.

    Ordered poses from the two distinct skeletons are grouped separately.
    Optional (first, count) ranges use the existing 512 KiB bound for full-screen
    character loops. Small static UI images keep their independent encodings. The runtime
    format, decoded-image costs, pose dimensions and order are unchanged.
    """
    cache = Path(cache)
    payloads = []
    for row in rows:
        raw = (cache/row['file']).read_bytes()
        tiles.require(len(raw) == row['bytes'] and tiles.sha256(raw) == row['sha256'],
                      'Appended image changed before grouping')
        payloads.append(raw)
    source = RowImages(rows, payloads)
    decoded_sizes = []
    for index, row in enumerate(rows):
        _width, _height, fmt, raw = source.image_raw(index)
        tiles.require(fmt == row.get('decoded_format', fmt) and
                      tiles.sha256(raw) == row['raw_sha256'], 'Appended image decoded identity changed')
        decoded_sizes.append(len(raw))

    # Refuse a shifted or incomplete pose catalog instead of grouping unrelated
    # art accidentally. Extra room/frontend/combat pictures may follow freely.
    tiles.require(len(rows) >= 94, 'Missing UI7 hand catalog')
    for hand, prefix in enumerate(('merchant', 'fake-merchant')):
        for pose in range(47):
            row = rows[hand*47+pose]
            tiles.require(row.get('key') == f'room_ui7.{prefix}-hand-{pose:02d}' and
                          (row['width'], row['height'], row['format'], row['decoded_format']) == (320, 240, 5, 4),
                          'UI7 hand catalog order or geometry changed')

    payload, entries = bytearray(), [None]*len(rows)
    groups = grouped_images = maximum_raw = 0

    def append(raw):
        payload.extend(bytes(len(payload) & 1))
        offset = len(payload)
        payload.extend(raw)
        return offset

    def group_range(first, count, limit):
        nonlocal groups, grouped_images, maximum_raw
        tiles.require(0 <= first < len(rows) and 0 < count <= len(rows)-first and
                      all(entry is None for entry in entries[first:first+count]), 'Overlapping appended image groups')
        source = RowImages(rows[first:first+count], payloads[first:first+count])
        batches = plan_images(source, group_bytes=limit, fast_images='none')
        for batch in batches:
            if batch.encoded is None:
                for record in batch.records:
                    offset = append(record.payload)
                    for image in record.ids:
                        entries[first+image] = (offset, record.stored_format, len(record.payload))
                continue
            group = tiles.GROUP.pack(b'RGR1', batch.raw_size, len(batch.encoded), 14,
                                     len(batch.records), 0, 0, 0) + batch.encoded
            group_at, raw_at = append(group), 0
            for record in batch.records:
                payload.extend(bytes(len(payload) & 1))
                descriptor_at = len(payload)
                payload.extend(DESCRIPTOR.pack(group_at-descriptor_at, len(group), raw_at,
                                                (record.fmt << 24) | len(record.raw)))
                for image in record.ids:
                    entries[first+image] = (descriptor_at, 7, DESCRIPTOR.size)
                    grouped_images += 1
                raw_at += len(record.raw)
            groups += 1
            maximum_raw = max(maximum_raw, batch.raw_size)
    for first in (0, 47):
        group_range(first, 47, group_bytes)
    hand_bytes = (len(payload)+1) & ~1
    hand_grouped, hand_groups, hand_maximum = grouped_images, groups, maximum_raw
    for first, count in extra_ranges:
        tiles.require(first >= 94, 'Extra appended group overlaps hand catalog')
        group_range(first, count, tiles.GROUP_LIMIT)
    for index in range(94, len(rows)):
        if entries[index] is None:
            entries[index] = (append(payloads[index]), rows[index]['format'], len(payloads[index]))
    payload.extend(bytes(len(payload) & 1))
    tiles.require(all(entry is not None for entry in entries), 'Incomplete appended image layout')
    original_hand_bytes = sum((len(p)+1) & ~1 for p in payloads[:94])
    original_all_bytes = sum((len(p)+1) & ~1 for p in payloads)
    tiles.require(len(payload) <= original_all_bytes, 'Hand grouping enlarged appended artwork')
    report = dict(grouped_images=grouped_images, retained_groups=groups,
                  max_group_raw_bytes=maximum_raw, target_group_raw_bytes=group_bytes,
                  hand_grouped_images=hand_grouped, hand_retained_groups=hand_groups,
                  max_hand_group_raw_bytes=hand_maximum,
                  extra_grouped_images=grouped_images-hand_grouped, extra_group_raw_limit=tiles.GROUP_LIMIT,
                  original_hand_bytes=original_hand_bytes, stored_hand_bytes=hand_bytes,
                  saved_common_art_bytes=original_all_bytes-len(payload),
                  unreachable_payload_bytes=0, runtime_buffer_growth=0,
                  unchanged_decoded_pixels=True, unchanged_pose_order=True)
    return dict(payload=bytes(payload), entries=entries, decoded_sizes=decoded_sizes, report=report)
