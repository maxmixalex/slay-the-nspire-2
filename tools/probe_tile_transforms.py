#!/usr/bin/env python3
"""Bounded, byte-exact preprocessing experiments for NSP2/15 tile groups.

This is a size diagnostic, not a pack writer or a runtime codec. It preserves
tile order, grouping, RGB565, alpha, hidden RGB, and the original 770-byte tile
records. Every compressed candidate is decoded and inversely transformed before
its size is reported. Variable-record offset tables are included in the bytes.
Actual resources and receipts must stay outside the public source repository.

Examples (long runs still require the repository checkpoint/journal workflow):
    python3 tools/probe_tile_transforms.py --self-test
    python3 tools/probe_tile_transforms.py --pack /private/world-v15.nsp.tns \
        --groups 0,1,100,300 --receipt /private/tile-probe.json
    python3 tools/probe_tile_transforms.py --tiles /private/unique-tiles.raw \
        --sample-groups 8 --receipt /private/tile-probe.json
"""
from __future__ import annotations

import argparse
import json
import lzma
import mmap
import os
from pathlib import Path
import struct
import tempfile
import time

import numpy as np

import content_tiles as ct


FILTERS = [dict(id=lzma.FILTER_LZMA2, dict_size=ct.GROUP_LIMIT, preset=6)]
METHODS = ('identity', 'byte_planes', 'byte_row_delta', 'native_planes',
           'native_row_delta', 'exact_bbox', 'exact_bbox_solid', 'exact_palette')
MAX_GROUPS = 32
MAX_TRANSFORMED_BYTES = 2 * ct.GROUP_LIMIT


def planes(raw):
    records = np.frombuffer(raw, dtype='u1').reshape(-1, ct.TILE_BYTES)
    dimensions = records[:, :2].copy()
    color = records[:, 2:514].copy().view('<u2').reshape(-1, ct.SIDE, ct.SIDE)
    alpha = records[:, 514:].reshape(-1, ct.SIDE, ct.SIDE).copy()
    return dimensions, color, alpha


def records(dimensions, color, alpha):
    result = np.empty((len(dimensions), ct.TILE_BYTES), dtype='u1')
    result[:, :2] = dimensions
    result[:, 2:514] = np.ascontiguousarray(color, dtype='<u2').view('u1').reshape(-1, 512)
    result[:, 514:] = alpha.reshape(-1, 256)
    return result.tobytes()


def channels(color, alpha, native):
    if native:
        values = np.stack((color >> 11, (color >> 5) & 63, color & 31, alpha)).astype('u2')
        masks = np.array([31, 63, 31, 255], dtype='u2')[:, None, None, None]
    else:
        values = np.stack((color & 255, color >> 8, alpha)).astype('u2')
        masks = np.full((3, 1, 1, 1), 255, dtype='u2')
    return values, masks


def encode_planes(raw, native, delta):
    dimensions, color, alpha = planes(raw)
    values, masks = channels(color, alpha, native)
    if delta:
        previous = np.zeros_like(values)
        previous[..., 1:] = values[..., :-1]
        values = (values - previous) & masks
    return dimensions.tobytes() + values.astype('u1').tobytes()


def decode_planes(raw, count, native, delta):
    channel_count = 4 if native else 3
    ct.require(len(raw) == count * (2 + channel_count * 256), 'Invalid planar length')
    dimensions = np.frombuffer(raw, 'u1', count * 2).reshape(count, 2)
    values = np.frombuffer(raw, 'u1', offset=count * 2).reshape(channel_count, count, 16, 16).astype('u2')
    masks = np.array([31, 63, 31, 255] if native else [255, 255, 255], 'u2')[:, None, None, None]
    ct.require(not np.any(values & ~masks), 'Planar component outside its native bit depth')
    if delta:
        values = np.cumsum(values, axis=-1, dtype='u2') & masks
    color = (values[0] << 11) | (values[1] << 5) | values[2] if native else values[0] | (values[1] << 8)
    return records(dimensions, color, values[-1].astype('u1'))


def encode_bbox(raw, solid):
    dimensions, colors, alphas = planes(raw)
    chunks = []
    for dimension, color, alpha in zip(dimensions, colors, alphas):
        w, h = map(int, dimension)
        prefix = bytes((w, h))
        active = (color != 0) | (alpha != 0)  # Never discard invisible nonzero RGB.
        if solid and not np.any(active):
            chunks.append(prefix + b'\x00')
            continue
        if solid and np.all(color[:h, :w] == color[0, 0]) and np.all(alpha[:h, :w] == alpha[0, 0]):
            chunks.append(prefix + b'\x01' + struct.pack('<HB', int(color[0, 0]), int(alpha[0, 0])))
            continue
        ys, xs = np.nonzero(active)
        x, y = (int(xs.min()), int(ys.min())) if len(xs) else (0, 0)
        cw, ch = (int(xs.max()) + 1 - x, int(ys.max()) + 1 - y) if len(xs) else (0, 0)
        if solid:
            prefix += b'\x02'
        chunks.append(prefix + bytes((x, y, cw, ch)) + color[y:y + ch, x:x + cw].tobytes()
                      + alpha[y:y + ch, x:x + cw].tobytes())
    offsets = [4 * (len(chunks) + 1)]
    for chunk in chunks:
        offsets.append(offsets[-1] + len(chunk))
    return struct.pack('<' + 'I' * len(offsets), *offsets) + b''.join(chunks)


def decode_bbox(raw, count, solid):
    ct.require(len(raw) >= 4 * (count + 1), 'Truncated bbox offsets')
    offsets = struct.unpack_from('<' + 'I' * (count + 1), raw)
    ct.require(offsets[0] == 4 * (count + 1) and offsets[-1] == len(raw), 'Invalid bbox offsets')
    dimensions = np.zeros((count, 2), 'u1')
    colors, alphas = np.zeros((count, 16, 16), '<u2'), np.zeros((count, 16, 16), 'u1')
    for i, (start, end) in enumerate(zip(offsets[:-1], offsets[1:])):
        ct.require(0 <= start < end <= len(raw) and end - start >= (3 if solid else 6), 'Invalid bbox record')
        w, h = raw[start:start + 2]
        ct.require(1 <= w <= 16 and 1 <= h <= 16, 'Invalid bbox dimensions')
        dimensions[i] = (w, h)
        mode = raw[start + 2] if solid else 2
        at = start + (3 if solid else 2)
        if mode == 0:
            ct.require(at == end, 'Trailing zero-tile bytes')
        elif mode == 1:
            ct.require(at + 3 == end, 'Invalid solid tile')
            color, alpha = struct.unpack_from('<HB', raw, at)
            colors[i, :h, :w], alphas[i, :h, :w] = color, alpha
        else:
            ct.require(mode == 2 and at + 4 <= end, 'Invalid bbox mode')
            x, y, cw, ch = raw[at:at + 4]
            at += 4
            ct.require(x + cw <= w and y + ch <= h and at + cw * ch * 3 == end, 'Invalid bbox extent')
            colors[i, y:y + ch, x:x + cw] = np.frombuffer(raw, '<u2', cw * ch, at).reshape(ch, cw)
            alphas[i, y:y + ch, x:x + cw] = np.frombuffer(raw, 'u1', cw * ch, at + cw * ch * 2).reshape(ch, cw)
    return records(dimensions, colors, alphas)


def encode_palette(raw):
    """Sorted RGB565 palette, minimum-width LSB-first indices, exact alpha8."""
    dimensions, colors, alphas = planes(raw)
    chunks = []
    for dimension, color, alpha in zip(dimensions, colors, alphas):
        palette, indexes = np.unique(color.reshape(-1), return_inverse=True)
        bits = (len(palette) - 1).bit_length()
        packed = b''
        if bits:
            bit_values = ((indexes[:, None] >> np.arange(bits)) & 1).astype('u1').reshape(-1)
            packed = np.packbits(bit_values, bitorder='little').tobytes()
        chunks.append(bytes(dimension) + struct.pack('<H', len(palette))
                      + palette.astype('<u2').tobytes() + packed + alpha.tobytes())
    offsets = [4 * (len(chunks) + 1)]
    for chunk in chunks:
        offsets.append(offsets[-1] + len(chunk))
    return struct.pack('<' + 'I' * len(offsets), *offsets) + b''.join(chunks)


def decode_palette(raw, count):
    ct.require(len(raw) >= 4 * (count + 1), 'Truncated palette offsets')
    offsets = struct.unpack_from('<' + 'I' * (count + 1), raw)
    ct.require(offsets[0] == 4 * (count + 1) and offsets[-1] == len(raw), 'Invalid palette offsets')
    dimensions = np.zeros((count, 2), 'u1')
    colors, alphas = np.zeros((count, 16, 16), '<u2'), np.zeros((count, 16, 16), 'u1')
    for i, (start, end) in enumerate(zip(offsets[:-1], offsets[1:])):
        ct.require(0 <= start < end <= len(raw) and end - start >= 262, 'Invalid palette record')
        w, h, size = struct.unpack_from('<BBH', raw, start)
        ct.require(1 <= w <= 16 and 1 <= h <= 16 and 1 <= size <= 256, 'Invalid palette shape')
        bits = (size - 1).bit_length()
        index_bytes = 32 * bits  # 256 pixels; every bit width is byte-aligned.
        ct.require(end - start == 4 + size * 2 + index_bytes + 256, 'Invalid palette record length')
        palette = np.frombuffer(raw, '<u2', size, start + 4)
        ct.require(np.all(palette[1:] > palette[:-1]), 'Palette is not strictly increasing')
        at = start + 4 + size * 2
        if bits:
            packed = np.frombuffer(raw, 'u1', index_bytes, at)
            values = np.unpackbits(packed, bitorder='little').reshape(256, bits).astype('u2')
            indexes = np.sum(values << np.arange(bits, dtype='u2'), axis=1, dtype='u2')
        else:
            indexes = np.zeros(256, 'u2')
        ct.require(np.all(indexes < size), 'Palette index outside color table')
        dimensions[i] = (w, h)
        colors[i] = palette[indexes].reshape(16, 16)
        alphas[i] = np.frombuffer(raw, 'u1', 256, at + index_bytes).reshape(16, 16)
    return records(dimensions, colors, alphas)


def transform(raw, method):
    if method == 'identity':
        return raw
    if method == 'exact_palette':
        return encode_palette(raw)
    if method.startswith('exact_bbox'):
        return encode_bbox(raw, method.endswith('_solid'))
    return encode_planes(raw, method.startswith('native_'), method.endswith('_delta'))


def inverse(raw, count, method):
    if method == 'identity':
        return raw
    if method == 'exact_palette':
        return decode_palette(raw, count)
    if method.startswith('exact_bbox'):
        return decode_bbox(raw, count, method.endswith('_solid'))
    return decode_planes(raw, count, method.startswith('native_'), method.endswith('_delta'))


def probe(raw, method):
    ct.validate_tiles(raw)
    start = time.perf_counter()
    transformed = transform(raw, method)
    ct.require(0 < len(transformed) <= MAX_TRANSFORMED_BYTES, 'Transform outside diagnostic bound')
    encode_start = time.perf_counter()
    encoded = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=FILTERS)
    decode_start = time.perf_counter()
    decoder = lzma.LZMADecompressor(format=lzma.FORMAT_RAW,
                                  filters=[dict(id=lzma.FILTER_LZMA2, dict_size=ct.GROUP_LIMIT)])
    decoded = decoder.decompress(encoded, max_length=len(transformed) + 1)
    ct.require(len(decoded) == len(transformed) and decoder.eof and not decoder.unused_data,
               'Invalid candidate LZMA2 stream')
    restored = inverse(decoded, len(raw) // ct.TILE_BYTES, method)
    ct.require(restored == raw, 'Candidate changed original tile bytes')
    ct.validate_tiles(restored)
    end = time.perf_counter()
    return dict(method=method, transformed_bytes=len(transformed), payload_bytes=len(encoded),
                existing_group_header_and_lookup_bytes=ct.GROUP.size + 8,
                payload_plus_existing_group_metadata_bytes=len(encoded) + ct.GROUP.size + 8,
                fits_existing_group_raw_bound=len(transformed) <= ct.GROUP_LIMIT,
                fits_existing_encoded_bound=len(encoded) + ct.GROUP.size <= ct.ENCODED_LIMIT,
                original_tile_bytes_restored_exactly=True,
                transform_seconds_host=encode_start - start, encode_seconds_host=decode_start - encode_start,
                decode_and_inverse_seconds_host=end - decode_start)


def self_test():
    rng = np.random.default_rng(0)
    dimensions = np.array([(16, 16), (16, 16), (16, 16), (1, 1), (7, 11), (16, 1), (1, 16)], 'u1')
    colors = rng.integers(0, 65536, (len(dimensions), 16, 16), dtype='u2')
    alpha = rng.integers(0, 256, (len(dimensions), 16, 16), dtype='u1')
    colors[0], alpha[0] = 0, 0
    colors[1], alpha[1] = 0x1234, 255
    alpha[2] = 0  # Nonzero RGB underneath zero alpha must survive.
    for i, (w, h) in enumerate(dimensions):
        colors[i, h:], alpha[i, h:] = 0, 0
        colors[i, :h, w:], alpha[i, :h, w:] = 0, 0
    # Exercise every palette index bit width, including non-power-of-two counts.
    for size in (2, 3, 5, 9, 17, 33, 65, 129, 256):
        dimensions = np.concatenate((dimensions, np.array([[16, 16]], 'u1')))
        colors = np.concatenate((colors, (np.arange(256, dtype='u2') % size).reshape(1, 16, 16)))
        alpha = np.concatenate((alpha, np.full((1, 16, 16), 255, 'u1')))
    raw = records(dimensions, colors, alpha)
    results = [probe(raw, method) for method in METHODS]
    print(json.dumps(dict(kind='synthetic byte-inverse smoke check', tiles=len(dimensions),
                          methods=len(results), all_original_tile_bytes_equal=True)))


def write_receipt(path, result):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name + '.', delete=False) as handle:
        temporary = Path(handle.name)
        try:
            handle.write((json.dumps(result, indent=2) + '\n').encode())
            handle.flush()
            os.fsync(handle.fileno())
        except BaseException:
            temporary.unlink(missing_ok=True)
            raise
    try:
        os.link(temporary, path)  # Atomically publish without replacing earlier evidence.
    finally:
        temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--pack', type=Path, help='Existing NSP2/15 pack; selected groups only are decoded')
    source.add_argument('--tiles', type=Path, help='Existing 770-byte-record unique-tiles.raw spool')
    source.add_argument('--self-test', action='store_true', help='Synthetic inverse smoke check; no assets')
    parser.add_argument('--auxiliary-count', type=int, default=1175, help='Expected pack auxiliary count')
    parser.add_argument('--tiles-per-group', type=int, default=ct.MAX_TILES_PER_GROUP, help='Raw spool only')
    parser.add_argument('--groups', help='Comma-separated existing group IDs; at most 32')
    parser.add_argument('--sample-groups', type=int, default=8, help='Evenly spaced groups when --groups is absent')
    parser.add_argument('--time-budget-seconds', type=float, default=45, help='Soft boundary between candidates; 1..60')
    parser.add_argument('--receipt', type=Path, help='New private JSON receipt; never overwritten')
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    ct.require(args.receipt is not None, 'A private --receipt is required')
    receipt = ct.private_path(args.receipt, 'Receipt')
    ct.require(not receipt.exists(), 'Receipt already exists')
    ct.require(1 <= args.time_budget_seconds <= 60 and 1 <= args.sample_groups <= MAX_GROUPS, 'Invalid probe bound')
    ct.require(1 <= args.tiles_per_group <= ct.MAX_TILES_PER_GROUP, 'Invalid raw-spool grouping')
    input_path = ct.private_path(args.pack or args.tiles, 'Input')
    start = time.perf_counter()
    result = dict(schema=1, kind='bounded tile transform size diagnostic; not a production pack',
                  input_path=str(input_path), input_kind='NSP2/15' if args.pack else 'raw tiles',
                  script_sha256=ct.sha256(Path(__file__).read_bytes()),
                  reader_sha256=ct.sha256(Path(ct.__file__).read_bytes()),
                  settings=dict(codec='raw LZMA2', preset=6, dictionary_bytes=ct.GROUP_LIMIT,
                                group_raw_bound_bytes=ct.GROUP_LIMIT, methods=list(METHODS),
                                variable_record_offsets_included=True, time_budget_seconds=args.time_budget_seconds),
                  groups=[], full_pack_size_measured=False, physical_device_performance_established=False,
                  original_source_and_pack_unmodified=True)
    with input_path.open('rb') as handle:
        before = os.fstat(handle.fileno())
        ct.require(0 < before.st_size <= ct.SPOOL_LIMIT, 'Input outside diagnostic size bound')
        with mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
            result.update(input_bytes=len(data), input_sha256=ct.sha256(data))
            if args.pack:
                reader = ct.TileReader(ct.PackView(data, args.auxiliary_count, versions=(15,)))
                count, per_group, tile_count = reader.tile_groups, reader.per_group, reader.tile_count
                get_group = lambda i: reader.group(i)[0]
            else:
                ct.require(len(data) % ct.TILE_BYTES == 0, 'Truncated raw tile spool')
                per_group, tile_count = args.tiles_per_group, len(data) // ct.TILE_BYTES
                count = (tile_count + per_group - 1) // per_group
                get_group = lambda i: data[i * per_group * ct.TILE_BYTES:min(len(data), (i + 1) * per_group * ct.TILE_BYTES)]
            if args.groups:
                selected = sorted(set(int(value) for value in args.groups.split(',')))
            else:
                n = min(args.sample_groups, count)
                selected = [0] if n == 1 else [i * (count - 1) // (n - 1) for i in range(n)]
            ct.require(0 < len(selected) <= MAX_GROUPS and all(0 <= i < count for i in selected), 'Invalid sample group IDs')
            result.update(tile_count=tile_count, tile_groups=count, tiles_per_group=per_group, requested_group_ids=selected)
            deadline = start + args.time_budget_seconds
            for group_id in selected:
                if time.perf_counter() >= deadline:
                    break
                raw = get_group(group_id)
                entry = dict(group_id=group_id, tiles=len(raw) // ct.TILE_BYTES,
                             raw_bytes=len(raw), raw_sha256=ct.sha256(raw), candidates=[])
                result['groups'].append(entry)
                for method in METHODS:
                    if time.perf_counter() >= deadline:
                        break
                    entry['candidates'].append(probe(raw, method))
                if len(entry['candidates']) != len(METHODS):
                    break
        after = input_path.stat()
        ct.require((before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) ==
                   (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns), 'Input changed during probe')
    complete = len(result['groups']) == len(selected) and all(len(g['candidates']) == len(METHODS) for g in result['groups'])
    full_groups = [g for g in result['groups'] if len(g['candidates']) == len(METHODS)]
    result.update(complete=complete, elapsed_seconds=time.perf_counter() - start,
                  completed_group_ids=[g['group_id'] for g in full_groups],
                  sample_summary=[dict(method=method,
                      payload_bytes=sum(next(c['payload_bytes'] for c in g['candidates'] if c['method'] == method) for g in full_groups),
                      groups_exceeding_existing_raw_bound=sum(not next(c['fits_existing_group_raw_bound'] for c in g['candidates'] if c['method'] == method) for g in full_groups))
                      for method in METHODS])
    write_receipt(receipt, result)
    print(json.dumps(dict(receipt=str(receipt), complete=complete, completed_groups=len(full_groups),
                          sample_summary=result['sample_summary'], elapsed_seconds=result['elapsed_seconds'])))
    return 0 if complete else 2


if __name__ == '__main__':
    raise SystemExit(main())
