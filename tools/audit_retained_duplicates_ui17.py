#!/usr/bin/env python3
"""Measure exact retained-image slice duplication without changing any asset.

Repeated descriptors of one existing slice are aliases, not new savings.
The report distinguishes those aliases from distinct identical slices and
reports uncompressed bytes only; actual physical savings require a converter.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import time

import compact_retained_colors as colors
import content_tiles as tiles
from repack_retained import DESCRIPTOR
from retained_filter import group_layout, transform_group


def audit(source):
    groups = colors.groups(source)
    uses = {}
    for image, (at, width, height, fmt, length) in enumerate(source.images):
        if fmt != 7:
            continue
        tiles.require(length == DESCRIPTOR.size, 'Invalid retained descriptor')
        relative, size, raw_at, shape = DESCRIPTOR.unpack_from(source.data, at)
        uses.setdefault(at + relative, []).append((image, width, height, raw_at, shape))
    exact, seen_slices, duplicates, aliases = {}, set(), [], 0
    original_bytes = unique_bytes = duplicate_bytes = same_group_bytes = 0
    formats = Counter()
    for group in groups:
        payload = source.block(group.offset, group.size)
        length, start, mode, slices = group_layout(payload, source.version)
        raw = tiles.decode_lzma2(payload[start:], length)
        if mode:
            raw = transform_group(raw, slices, mode, True)
        for image, width, height, at, shape in uses[group.offset]:
            fmt, size = shape >> 24, shape & 0xffffff
            tiles.require(at <= len(raw) and size <= len(raw) - at, 'Retained slice exceeds raw group')
            identity = (group.offset, at, shape, width, height)
            if identity in seen_slices:
                aliases += 1
                continue
            seen_slices.add(identity)
            block = bytes(raw[at:at+size])
            digest = hashlib.sha256(block).hexdigest()
            key = (fmt, width, height, size, digest)
            original_bytes += size
            formats[fmt] += 1
            old = exact.get(key)
            if old is None:
                exact[key] = (group.offset, at, image, block)
                unique_bytes += size
                continue
            tiles.require(old[3] == block, 'Hash collision is not exact duplicate evidence')
            duplicate_bytes += size
            same = old[0] == group.offset
            same_group_bytes += size if same else 0
            duplicates.append(dict(image=image, canonical_image=old[2], group=group.offset,
                canonical_group=old[0], raw_at=at, canonical_raw_at=old[1],
                format=fmt, width=width, height=height, raw_bytes=size,
                same_group=same, raw_sha256=digest))
    return dict(schema=1, input_bytes=len(source.data), input_sha256=tiles.sha256(source.data),
        images=len(source.images), retained_groups=len(groups), existing_descriptor_aliases=aliases,
        distinct_slices=len(seen_slices), slice_formats=dict(formats),
        summed_raw_slice_bytes=original_bytes, unique_raw_slice_bytes=unique_bytes,
        duplicate_raw_slice_bytes=duplicate_bytes, same_group_duplicate_raw_bytes=same_group_bytes,
        distinct_exact_duplicate_slices=len(duplicates),
        physical_savings_not_measured=True, results=duplicates)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    src = tiles.private_path(args.input, 'Private source')
    dst = tiles.private_path(args.output, 'Private report')
    tiles.require(src != dst, 'Never overwrite source data')
    start = time.monotonic()
    # This diagnostic accepts the newer format explicitly; production format
    # acceptance remains in the matching converter and runtime boundaries.
    data = src.read_bytes()
    source = colors.view(data)
    report = audit(source)
    report['elapsed_seconds'] = time.monotonic() - start
    tiles.atomic_bytes(dst, json.dumps(report, indent=2).encode() + b'\n')
    print(json.dumps({k:v for k,v in report.items() if k != 'results'}), flush=True)


if __name__ == '__main__':
    main()
