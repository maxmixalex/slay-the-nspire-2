#!/usr/bin/env python3
"""Losslessly shrink retained image groups with the bounded NSP2/36 reader.

Only strictly smaller complete RGR2 groups replace RGR1. Original image
geometry, alpha, frame clocks, tiles, glyphs and text are preserved; inputs
are never overwritten. RGB transforms reuse the existing decoded group pool.
"""
from __future__ import annotations

import argparse
from bisect import bisect_right
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import json
import lzma
from pathlib import Path
import struct

import content_tiles as tiles
from repack_retained import DESCRIPTOR, auxiliary_count
from retained_filter import CONTENT_VERSION, SLICE, group_layout, transform_group
import split_content

VERSIONS = (34, 35, CONTENT_VERSION)
PROFILES = ((3, 0, 2), (3, 0, 0), (4, 0, 1))
ENCODER = b'RGR2-planar-rgb565-v1-lzma2-9-273-512'


def aligned(size):
    return (size + 1) & ~1


def view(data):
    return tiles.PackView(data, auxiliary_count(data), versions=VERSIONS)


@dataclass
class Group:
    offset: int
    size: int
    first_image: int
    slices: tuple


@dataclass
class Choice:
    original_sha256: str
    slices: tuple
    payload: bytes | None
    receipt: dict


def groups(source):
    found = {}
    for image, (offset, _w, _h, fmt, length) in enumerate(source.images):
        if fmt != 7:
            continue
        tiles.require(length == 16, 'Invalid retained image descriptor')
        relative, size, raw_at, shape = DESCRIPTOR.unpack(source.block(offset, 16))
        group_at = offset + relative
        group = found.setdefault(group_at, dict(size=size, first=image, slices=set()))
        tiles.require(group['size'] == size, 'Aliased retained group lengths disagree')
        group['slices'].add((raw_at, shape))
    result = [Group(at, item['size'], item['first'], tuple(sorted(item['slices'])))
              for at, item in sorted(found.items())]
    previous = 0
    for group in result:
        tiles.require(group.offset >= previous, 'Retained groups overlap')
        previous = group.offset + group.size
        source.block(group.offset, group.size)
    return result


def complete_slices(slices, size):
    end = 0
    for at, shape in slices:
        length = shape & 0xffffff
        if at != end or not length or shape >> 24 > 4 or length > size - end:
            return False
        end += length
    return end == size


def slice_hash(slices):
    return tiles.sha256(b''.join(SLICE.pack(*row) for row in slices))


def previous_results(path):
    """Read either the historical probe or this converter's own receipt.

    Early converter receipts have no cached-stream path or slice fingerprint.
    Their matching sibling output supplies those exact bytes without another
    compression pass. Every imported stream is still decoded against the new
    source's filtered bytes before it can become a candidate.
    """
    path = Path(path)
    receipt = json.loads(path.read_bytes())
    rows = {row['first_image']: dict(row) for row in receipt['results']
            if isinstance(row.get('first_image'), int) and isinstance(row.get('raw_sha256'), str)}
    if receipt.get('version') == CONTENT_VERSION and 'locales' in receipt:
        for language, locale in receipt['locales'].items():
            packed = path.parent / ('ui9-' + language + '.nsp')
            if not packed.is_file():
                continue
            data = packed.read_bytes()
            tiles.require(tiles.sha256(data) == locale['output_sha256'],
                          'Previous retained output does not match its receipt')
            source = view(data)
            for group in groups(source):
                row = rows.get(group.first_image)
                if row is None:
                    continue
                payload = source.block(group.offset, group.size)
                raw_size, start, mode, _table = group_layout(payload, source.version)
                tiles.require(row.get('mode') == mode and row.get('raw_bytes') == raw_size,
                              'Previous retained group mode or size differs')
                row['slice_sha256'] = slice_hash(group.slices)
                if mode:
                    row['_stream_bytes'] = bytes(payload[start:])
            break
    return {(row['first_image'], row['raw_sha256']): row for row in rows.values()}


def proven_previous_mode(prior, raw_size, original_size, slices, planar):
    if not prior:
        return None
    # A missing field is not the historical explicit "chosen: null" result.
    # The old probe's format-1 transform was incorrect, so those groups still
    # require the corrected planar transform and full candidate selection.
    if all(key in prior for key in ('chosen', 'stream', 'stored_bytes', 'best_bytes')):
        chosen = prior['chosen']
        mode = 0 if chosen is None else chosen.get('mode') if isinstance(chosen, dict) else None
        return mode if not planar and mode in (0, 1, 2) else None
    if (prior.get('mode') in (0, 1, 2) and prior.get('raw_bytes') == raw_size and
            prior.get('original_bytes') == original_size and prior.get('slice_sha256') == slices):
        return prior['mode']
    return None


def candidate(source, group, cache, previous):
    original = bytes(source.block(group.offset, group.size))
    raw_size, start, existing_mode, _table = group_layout(original, source.version)
    receipt = dict(first_image=group.first_image, records=len(group.slices),
                   raw_bytes=raw_size, original_bytes=len(original), output_bytes=len(original),
                   saved_bytes=0, mode=0, reused_candidate=False, slice_sha256=slice_hash(group.slices))
    result = Choice(tiles.sha256(original), group.slices, None, receipt)
    if existing_mode or not complete_slices(group.slices, raw_size):
        receipt['preserved_reason'] = 'already_filtered' if existing_mode else 'unreferenced_or_overlapping_slices'
        return result
    raw = tiles.decode_lzma2(original[start:], raw_size)
    receipt['raw_sha256'] = tiles.sha256(raw)
    prior = previous.get((group.first_image, receipt['raw_sha256']))
    planar = any(shape >> 24 == 1 for _at, shape in group.slices)
    previous_mode = proven_previous_mode(prior, raw_size, len(original), receipt['slice_sha256'], planar)
    # A previous non-beneficial screen can only keep the verified old stream;
    # it cannot substitute bytes. Dense-alpha groups must be recomputed because
    # the historical experiment treated their two planes as interleaved RGB/A.
    if previous_mode == 0:
        receipt['preserved_reason'] = 'previous_nonbeneficial_screen'
        return result
    best = original
    for mode in (1, 2):
        if previous_mode in (1, 2) and mode != previous_mode:
            continue
        filtered = transform_group(raw, group.slices, mode)
        tiles.require(transform_group(filtered, group.slices, mode, True) == raw,
                      'Retained forward/inverse bytes differ')
        encoded = None
        imported = False
        key = tiles.sha256(ENCODER + bytes((mode,)) + filtered)
        cached = cache / (key + '.lzma2')
        if cached.is_file():
            encoded = cached.read_bytes()
            tiles.require(tiles.decode_lzma2(encoded, raw_size) == filtered,
                          'Cached retained filtered bytes differ')
        if encoded is None and prior:
            chosen = prior.get('chosen')
            selected = chosen.get('mode') if isinstance(chosen, dict) else prior.get('mode')
            proposed = prior.get('_stream_bytes') if selected == mode else None
            if proposed is None and selected == mode and prior.get('stream'):
                path = Path(prior['stream'])
                if path.is_file():
                    proposed = path.read_bytes()
            if proposed is not None and tiles.decode_lzma2(proposed, raw_size) == filtered:
                encoded, imported = proposed, True
        if encoded is None:
            for lc, lp, pb in PROFILES:
                stream = lzma.compress(filtered, format=lzma.FORMAT_RAW,
                    filters=[dict(id=lzma.FILTER_LZMA2, preset=9, dict_size=tiles.GROUP_LIMIT,
                                  nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
                if encoded is None or len(stream) < len(encoded):
                    encoded = stream
            tiles.require(tiles.decode_lzma2(encoded, raw_size) == filtered,
                          'Compressed retained filtered bytes differ')
        # Imports also populate the content-addressed cache. A later reuse of
        # this converter's receipt must not silently lose the historical wins.
        if not cached.is_file():
            tiles.atomic_bytes(cached, encoded)
        table = b''.join(SLICE.pack(*row) for row in group.slices)
        payload = tiles.GROUP.pack(b'RGR2', raw_size, len(encoded), 14,
            len(group.slices), mode, len(table), 0) + table + encoded
        if len(payload) <= tiles.ENCODED_LIMIT and aligned(len(payload)) < aligned(len(best)):
            best = payload
            receipt.update(mode=mode, reused_candidate=imported, stream=str(cached))
    if best is not original:
        group_layout(best, CONTENT_VERSION)
        result.payload = best
        receipt.update(output_bytes=len(best), saved_bytes=aligned(len(original)) - aligned(len(best)))
    return result


def plan(source, cache, *, jobs=2, previous=None):
    tiles.require(1 <= jobs <= 8, 'Use one through eight compression workers')
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    catalog = groups(source)
    previous = previous or {}
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        choices = []
        for choice in pool.map(lambda row: candidate(source, row, cache, previous), catalog):
            choices.append(choice)
            if len(choices) % 40 == 0:
                print(json.dumps(dict(stage='retained_colors', complete=len(choices), total=len(catalog),
                    saved_bytes=sum(c.receipt['saved_bytes'] for c in choices))), flush=True)
    return choices


class Relocation:
    def __init__(self, replacements):
        self.rows = replacements
        self.ends = [end for _begin, end, _payload in replacements]
        self.savings = [0]
        for begin, end, payload in replacements:
            self.savings.append(self.savings[-1] + end - begin - len(payload))

    def __call__(self, offset):
        count = bisect_right(self.ends, offset)
        tiles.require(count == len(self.rows) or offset <= self.rows[count][0],
                      'A live pointer addresses the interior of a replaced retained group')
        return offset - self.savings[count]


def rewrite(source, choices):
    catalog = groups(source)
    tiles.require(len(catalog) == len(choices), 'Bilingual retained group counts differ')
    replacements, new_lengths = [], {}
    for group, choice in zip(catalog, choices):
        original = source.block(group.offset, group.size)
        tiles.require(tiles.sha256(original) == choice.original_sha256 and group.slices == choice.slices,
                      'Bilingual retained group or slice differs')
        if choice.payload is None:
            continue
        end = group.offset + group.size
        if end & 1:
            tiles.require(end < len(source.data) and source.data[end] == 0, 'Missing group alignment byte')
            end += 1
        payload = choice.payload + bytes(len(choice.payload) & 1)
        tiles.require(len(payload) < end - group.offset, 'Retained replacement is not smaller')
        replacements.append((group.offset, end, payload))
        new_lengths[group.offset] = len(choice.payload)
    relocate = Relocation(replacements)
    output, previous = bytearray(), 0
    for begin, end, payload in replacements:
        output.extend(source.data[previous:begin]); output.extend(payload); previous = end
    output.extend(source.data[previous:])
    for image, (off, w, h, fmt, length) in enumerate(source.images):
        tiles.IMAGE.pack_into(output, 40 + image * 16, relocate(off), w, h, fmt, length)
    for index, (cp, off, px, w, h, advance, encoded) in enumerate(source.glyphs):
        tiles.GLYPH.pack_into(output, source.glyph_at + index * 16,
            cp, relocate(off), px, w, h, advance, encoded)
    for index, (off, size) in enumerate(source.strings):
        struct.pack_into('<II', output, source.string_at + index * 8, relocate(off), size)
    header = list(tiles.GLOBAL.unpack_from(source.data, source.base_end))
    for index in (8, 9, 14):
        header[index] = relocate(header[index])
    tiles.GLOBAL.pack_into(output, relocate(source.base_end), *header)
    tiled = tiles.TileReader(source)
    old_table = tiles.GLOBAL.unpack_from(source.data, source.base_end)[9]
    for index in range(tiled.tile_groups + tiled.index_groups):
        off, length = struct.unpack_from('<II', source.data, old_table + index * 8)
        struct.pack_into('<II', output, relocate(old_table + index * 8), relocate(off), length)
    for index in range(tiled.frame_count):
        off = tiled.descriptors + index * 16
        global_at, group, raw_at, length = tiles.DESCRIPTOR.unpack_from(source.data, off)
        tiles.DESCRIPTOR.pack_into(output, relocate(off), relocate(global_at), group, raw_at, length)
    descriptors = {off for off, _w, _h, fmt, _size in source.images if fmt == 7}
    descriptors.update(off for _cp, off, _px, _w, _h, _advance, encoded in source.glyphs
                       if encoded & tiles.GLYPH_GROUP_FLAG)
    descriptors.update(off for off, length in source.strings if length & tiles.STRING_GROUP_FLAG)
    for off in descriptors:
        relative, length, raw_at, shape = DESCRIPTOR.unpack_from(source.data, off)
        group_at = off + relative
        new_off = relocate(off)
        DESCRIPTOR.pack_into(output, new_off, relocate(group_at) - new_off,
            new_lengths.get(group_at, length), raw_at, shape)
    struct.pack_into('<I', output, 4, CONTENT_VERSION)
    struct.pack_into('<I', output, 12, len(output))
    struct.pack_into('<I', output, 36, 0)
    return bytes(output)


def verify(source, data, *, all_tiles=True):
    target = view(data)
    tiles.require(source.counts == target.counts and source.clips == target.clips and
                  source.cards == target.cards and source.aux == target.aux, 'Stable catalog changed')
    old_tiles, new_tiles = tiles.TileReader(source), tiles.TileReader(target)
    tiles.require(old_tiles.frame_count == new_tiles.frame_count and
                  len(old_tiles.groups) == len(new_tiles.groups), 'Tile catalog changed')
    for before, after in zip(old_tiles.groups, new_tiles.groups):
        a, n = before[:2]; b, m = after[:2]
        tiles.require(n == m and source.block(a - 32, n + 32) == target.block(b - 32, m + 32),
                      'A tile/index compressed stream changed')
    decoded = 0
    for image, (old, new) in enumerate(zip(source.images, target.images)):
        tiles.require(old[1:] == new[1:], 'Image directory geometry changed')
        if old[3] == 6:
            tiles.require(source.block(old[0], 16) == target.block(new[0], 16), 'Tile descriptor changed')
            if not all_tiles:
                continue
            before, after = old_tiles.image_raw(image), new_tiles.image_raw(image)
        else:
            before, after = source.image_raw(image), target.image_raw(image)
        tiles.require(before == after, 'Decoded image differs: ' + str(image))
        decoded += 1
        if decoded % 10000 == 0:
            print(json.dumps(dict(stage='verify_retained_colors', complete=decoded, total=len(source.images))), flush=True)
    for index, (old, new) in enumerate(zip(source.glyphs, target.glyphs)):
        tiles.require(old[:1] + old[2:] == new[:1] + new[2:] and
                      source.glyph_payload(index) == target.glyph_payload(index), 'Glyph metrics/bytes changed')
    for index, (old, new) in enumerate(zip(source.strings, target.strings)):
        tiles.require(old[1] == new[1] and source.string_payload(index) == target.string_payload(index),
                      'Localized string bytes changed')
    return dict(decoded_images=decoded, unchanged_images=len(source.images), unchanged_glyphs=len(source.glyphs),
        unchanged_strings=len(source.strings), unchanged_clips=len(source.clips),
        unchanged_compressed_tile_groups=len(old_tiles.groups), all_images_decoded=all_tiles)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--eng', type=Path, required=True)
    parser.add_argument('--zhs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--reuse-receipt', type=Path)
    parser.add_argument('--jobs', type=int, default=2)
    args = parser.parse_args()
    inputs = {language: tiles.private_path(getattr(args, language), 'Input pack') for language in ('eng', 'zhs')}
    output = tiles.private_path(args.output, 'Retained output directory')
    cache = tiles.private_path(args.cache, 'Retained compression cache')
    outputs = {language: output / ('ui9-' + language + '.nsp') for language in inputs}
    tiles.require(not set(inputs.values()).intersection(outputs.values()), 'Never overwrite an input pack')
    previous = {}
    if args.reuse_receipt:
        previous = previous_results(args.reuse_receipt)
    sources = {language: view(path.read_bytes()) for language, path in inputs.items()}
    choices = plan(sources['eng'], cache, jobs=args.jobs, previous=previous)
    packs, receipts = {}, {}
    for language, source in sources.items():
        data = rewrite(source, choices)
        proof = verify(source, data)
        packs[language] = data
        receipts[language] = dict(input_bytes=len(source.data), output_bytes=len(data),
            saved_bytes=len(source.data) - len(data), input_sha256=tiles.sha256(source.data),
            output_sha256=tiles.sha256(data), verification=proof)
    common, localized, split_receipt = split_content.split(packs)
    for language in packs:
        tiles.require(split_content.reconstruct(common, localized[language], language) == packs[language],
                      'Filtered split reconstruction differs')
    result = dict(schema=1, reference='v0.111.0 / 41cef1ea', version=CONTENT_VERSION,
        groups=len(choices), changed_groups=sum(c.payload is not None for c in choices),
        new_runtime_pools=0, new_runtime_integrity_checks=False, locales=receipts,
        split=split_receipt, results=[c.receipt for c in choices])
    for language, data in packs.items():
        tiles.atomic_bytes(outputs[language], data)
    tiles.atomic_bytes(output / 'retained-colors.json', json.dumps(result, indent=2).encode() + b'\n')
    print(json.dumps({key: value for key, value in result.items() if key != 'results'}), flush=True)


if __name__ == '__main__':
    main()
