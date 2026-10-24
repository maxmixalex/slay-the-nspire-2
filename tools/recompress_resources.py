#!/usr/bin/env python3
"""Tighten existing private NSP2 groups without changing their decoded bytes.

Group membership, tile ids, image formats, geometry, sampling and random-access
limits stay unchanged. A stronger bounded LZMA2 encoding is used only when its
aligned footprint is smaller. This is a production pack conversion, with no
new runtime codec or startup work. Inputs are never overwritten.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import lzma
from pathlib import Path
import struct
import time

import content_tiles as tiles
from repack_retained import DESCRIPTOR, auxiliary_count


VERSIONS = tuple(range(20, 30))
ENCODER_ID = b'NSP2-adaptive-LZMA2-v1'


def view(data):
    return tiles.PackView(data, auxiliary_count(data), versions=VERSIONS)


def groups(pack):
    header = tiles.GLOBAL.unpack_from(pack.data, pack.base_end)
    table, count = header[9], header[5] + header[6]
    result = {}
    for index in range(count):
        offset, length = struct.unpack_from('<II', pack.data, table + index * 8)
        h = tiles.GROUP.unpack_from(pack.data, offset)
        tiles.require(h[0] == b'TGR1' and h[2] == index and h[4] == length - 32,
                      'Invalid existing tile/index group')
        result[offset] = length
    for offset, _w, _h, fmt, length in pack.images:
        if fmt != 7:
            continue
        tiles.require(length == DESCRIPTOR.size, 'Invalid retained descriptor')
        relative, size, _raw_at, _shape = DESCRIPTOR.unpack_from(pack.data, offset)
        begin = offset + relative
        h = tiles.GROUP.unpack_from(pack.data, begin)
        tiles.require(h[0] == b'RGR1' and h[2] == size - 32 and h[3] == 14,
                      'Invalid existing retained group')
        tiles.require(begin not in result or result[begin] == size,
                      'Shared group length disagrees')
        result[begin] = size
    ordered = sorted(result.items())
    previous = 0
    for offset, length in ordered:
        tiles.require(offset >= previous and offset % 2 == 0 and length > 32 and
                      offset + length <= len(pack.data), 'Overlapping/out-of-range group')
        previous = offset + length
    return ordered


def choose_group(payload, candidate, raw_size):
    physical = candidate + bytes((len(payload) - len(candidate)) & 1)
    if len(physical) >= len(payload):
        return payload, payload, raw_size
    return candidate, physical, raw_size


def encode_group(payload, raw=None):
    header = list(tiles.GROUP.unpack_from(payload))
    tiled = header[0] == b'TGR1'
    raw_size = header[3] if tiled else header[1]
    raw = tiles.decode_lzma2(payload[32:], raw_size) if raw is None else raw
    # Tile color planes benefit from position-aware literal contexts; frame
    # indices and sparse retained records have a different byte structure.
    # lc+lp never exceeds four, the decoder's existing fixed probability arena.
    profiles = ((3, 0, 2), (4, 0, 1), (3, 1, 1)) if tiled and header[1] == 1 else \
               ((3, 0, 2), (3, 0, 0), (4, 0, 1))
    candidate = payload
    for lc, lp, pb in profiles:
        encoded = lzma.compress(raw, format=lzma.FORMAT_RAW,
            filters=[dict(id=lzma.FILTER_LZMA2, preset=9, dict_size=tiles.GROUP_LIMIT,
                          nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
        tiles.require(tiles.decode_lzma2(encoded, raw_size) == raw,
                      'Recompressed group differs after independent decoding')
        if len(encoded) + 32 < len(candidate):
            header[4 if tiled else 2] = len(encoded)
            candidate = tiles.GROUP.pack(*header) + encoded
    # Preserve the replaced span's parity. Any extra byte is outside the
    # group, so exact-consumption runtime decoding does not accept padding.
    return choose_group(payload, candidate, raw_size)


class Relocation:
    def __init__(self, replacements):
        self.starts, self.ends, self.prefix = [], [], [0]
        for offset, old_length, _payload, physical in replacements:
            self.starts.append(offset)
            self.ends.append(offset + old_length)
            self.prefix.append(self.prefix[-1] + old_length - len(physical))

    def __call__(self, offset):
        index = bisect_right(self.ends, offset)
        tiles.require(index == len(self.starts) or offset <= self.starts[index],
                      'Directory points inside a replaced compressed stream')
        return offset - self.prefix[index]


def rewrite(data, plan):
    source = view(data)
    replacements = [(offset, length, payload, physical)
                    for offset, length, payload, physical, _raw in plan
                    if len(physical) < length]
    relocate = Relocation(replacements)
    out, previous = bytearray(), 0
    sizes = {}
    for offset, length, payload, physical in replacements:
        out.extend(data[previous:offset])
        out.extend(physical)
        previous = offset + length
        sizes[offset] = len(payload)
    out.extend(data[previous:])
    for index, (offset, width, height, fmt, length) in enumerate(source.images):
        tiles.IMAGE.pack_into(out, 40 + index * 16, relocate(offset), width, height, fmt, length)
    for index, (cp, offset, px, width, height, advance, encoded) in enumerate(source.glyphs):
        tiles.GLYPH.pack_into(out, source.glyph_at + index * 16,
            cp, relocate(offset), px, width, height, advance, encoded)
    for index, (offset, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at + index * 8, relocate(offset), length)
    header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    descriptors, table, frames, count = header[8], header[9], header[7], header[5] + header[6]
    for field in (8, 9, 14):
        header[field] = relocate(header[field])
    tiles.GLOBAL.pack_into(out, source.base_end, *header)
    for index in range(count):
        offset, length = struct.unpack_from('<II', data, table + index * 8)
        struct.pack_into('<II', out, relocate(table + index * 8),
                         relocate(offset), sizes.get(offset, length))
    for index in range(frames):
        offset = descriptors + index * 16
        global_at, group, raw_at, size = tiles.DESCRIPTOR.unpack_from(data, offset)
        tiles.DESCRIPTOR.pack_into(out, relocate(offset), relocate(global_at), group, raw_at, size)
    seen = set()
    for offset, _w, _h, fmt, _length in source.images:
        if fmt != 7 or offset in seen:
            continue
        seen.add(offset)
        relative, size, raw_at, shape = DESCRIPTOR.unpack_from(data, offset)
        group = offset + relative
        DESCRIPTOR.pack_into(out, relocate(offset), relocate(group) - relocate(offset),
                             sizes.get(group, size), raw_at, shape)
    struct.pack_into('<I', out, 12, len(out))
    struct.pack_into('<I', out, 36, 0)
    target = view(out)
    tiles.require(source.counts == target.counts and source.clips == target.clips and
                  source.cards == target.cards and source.aux == target.aux,
                  'Recompression changed catalog, geometry or timing')
    # Check every decoder input and every unchanged literal independently of
    # the writer. Equal decoded groups plus equal slice/index metadata imply
    # identical image pixels and alpha without decoding 30,000 frames twice.
    old_groups, new_groups = groups(source), groups(target)
    tiles.require(len(old_groups) == len(new_groups), 'Recompression changed group count')
    for (old_offset, old_length), (new_offset, new_length) in zip(old_groups, new_groups):
        tiles.require(new_offset == relocate(old_offset), 'Group relocation differs')
        old = data[old_offset:old_offset + old_length]
        new = out[new_offset:new_offset + new_length]
        a, b = list(tiles.GROUP.unpack_from(old)), list(tiles.GROUP.unpack_from(new))
        tiled = a[0] == b'TGR1'
        encoded_field, raw_field = (4, 3) if tiled else (2, 1)
        a[encoded_field] = b[encoded_field] = 0
        tiles.require(a == b and tiles.decode_lzma2(old[32:], a[raw_field]) ==
                      tiles.decode_lzma2(new[32:], b[raw_field]), 'Decoded group changed')
    for old, new in zip(source.images, target.images):
        offset, _w, _h, fmt, length = old
        tiles.require(new == (relocate(offset), *old[1:]), 'Image descriptor changed')
        if fmt < 6:
            tiles.require(data[offset:offset + length] == out[new[0]:new[0] + length],
                          'Literal image payload changed')
        elif fmt == 6:
            a, b = tiles.DESCRIPTOR.unpack_from(data, offset), tiles.DESCRIPTOR.unpack_from(out, new[0])
            tiles.require((relocate(a[0]), *a[1:]) == b, 'Frame slice/index metadata changed')
        else:
            a, b = DESCRIPTOR.unpack_from(data, offset), DESCRIPTOR.unpack_from(out, new[0])
            tiles.require((relocate(offset + a[0]) - new[0], sizes.get(offset + a[0], a[1]), *a[2:]) == b,
                          'Retained image slice changed')
    for old, new in zip(source.glyphs, target.glyphs):
        size = old[6] or old[3] * old[4]
        tiles.require(new == (old[0], relocate(old[1]), *old[2:]) and
                      data[old[1]:old[1] + size] == out[new[1]:new[1] + size], 'Glyph changed')
    for old, new in zip(source.strings, target.strings):
        tiles.require(new == (relocate(old[0]), old[1]) and
                      data[old[0]:old[0] + old[1] + 1] == out[new[0]:new[0] + new[1] + 1], 'Text changed')
    return bytes(out), dict(input_bytes=len(data), output_bytes=len(out), saved_bytes=len(data) - len(out),
        groups=len(plan), improved_groups=len(replacements), decoded_group_bytes=sum(row[4] for row in plan),
        all_decoded_groups_equal=True, all_image_slice_geometry_equal=True, unchanged_literal_images=True,
        unchanged_glyphs_and_text=True, unchanged_timing=True, unchanged_random_access_groups=True,
        runtime_memory_added=0, runtime_startup_checks_added=0, version=source.version,
        input_sha256=hashlib.sha256(data).hexdigest(), output_sha256=hashlib.sha256(out).hexdigest())


def recompress(data, *, workers=2, cache=None, cache_directory=None):
    source = view(data)
    entries = groups(source)
    cache = {} if cache is None else cache
    if cache_directory is not None:
        cache_directory = Path(cache_directory)
        cache_directory.mkdir(parents=True, exist_ok=True)

    def convert(entry):
        offset, length = entry
        old = bytes(data[offset:offset + length])
        header = list(tiles.GROUP.unpack_from(old))
        tiled = header[0] == b'TGR1'
        raw_size = header[3] if tiled else header[1]
        raw = tiles.decode_lzma2(old[32:], raw_size)
        header[4 if tiled else 2] = 0
        # A raw-content key survives earlier lossless repacking and locale
        # relocation. Final UI/font appends can reuse the completed encoding.
        key = hashlib.sha256(ENCODER_ID + tiles.GROUP.pack(*header) + raw).digest()
        known = cache.get(key)
        path = cache_directory / (key.hex() + '.bin') if cache_directory is not None else None
        if known is None and path is not None and path.exists():
            try:
                candidate = path.read_bytes()
                checked = list(tiles.GROUP.unpack_from(candidate))
                checked[4 if tiled else 2] = 0
                if checked == header and tiles.decode_lzma2(candidate[32:], raw_size) == raw:
                    known = candidate
            except (OSError, ValueError, struct.error, lzma.LZMAError):
                pass  # A disposable build cache never substitutes for input verification.
        result = choose_group(old, known, raw_size) if known is not None else encode_group(old, raw)
        if known is None and path is not None:
            tiles.atomic_bytes(path, result[0])
        return offset, length, result, key

    plan, saved = [], 0
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for offset, length, (payload, physical, raw), key in pool.map(convert, entries):
            plan.append((offset, length, payload, physical, raw))
            cache[key] = payload
            saved += length - len(physical)
            if len(plan) % 64 == 0:
                print(json.dumps(dict(stage='recompress', completed=len(plan), total=len(entries),
                                      saved_bytes=saved)), flush=True)
    return rewrite(data, plan)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--eng', type=Path, required=True)
    parser.add_argument('--zhs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--cache', type=Path, help='Private content-addressed encoder cache for later UI/font appends')
    args = parser.parse_args()
    tiles.require(1 <= args.workers <= 4, 'Use one to four bounded encoder workers')
    inputs = {lang: tiles.private_path(getattr(args, lang), 'Private source pack') for lang in ('eng', 'zhs')}
    output = tiles.private_path(args.output, 'Private compressed directory')
    cache_directory = tiles.private_path(args.cache, 'Private encoder cache') if args.cache else None
    tiles.require(len(set(inputs.values())) == 2 and all(
        output / ('compressed-' + lang + '.nsp') not in inputs.values() for lang in inputs),
        'Never overwrite an input pack')
    start, cache, receipts = time.monotonic(), {}, {}
    for language, path in inputs.items():
        result, receipt = recompress(path.read_bytes(), workers=args.workers, cache=cache,
                                    cache_directory=cache_directory)
        tiles.atomic_bytes(output / ('compressed-' + language + '.nsp'), result)
        receipts[language] = receipt
        print(json.dumps(dict(language=language, **receipt)), flush=True)
    summary = dict(schema=1, reference='v0.111.0 / 41cef1ea', languages=receipts,
        elapsed_seconds=time.monotonic() - start,
        encoder='Adaptive LZMA2 BT4 nice273 depth512 dictionary512KiB lc+lp<=4',
        encoder_cache=str(cache_directory) if cache_directory else None)
    tiles.atomic_bytes(output / 'recompressed-receipt.json', json.dumps(summary, indent=2).encode() + b'\n')


if __name__ == '__main__':
    main()
