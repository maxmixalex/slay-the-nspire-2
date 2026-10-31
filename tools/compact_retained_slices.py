#!/usr/bin/env python3
"""Reclaim unreachable retained pixels without merging independent read groups.

Replace only an existing image group's payload. Preserve its codec, dictionary,
image membership, every live decoded byte, and all subsequent 4 KiB address
phases. Small savings that cannot pay for that alignment stay unchanged. This
is an offline conversion, with no runtime decoder, scan or workspace addition.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import json
import lzma
import os
from pathlib import Path
import struct
from tempfile import TemporaryDirectory

import coalesce_tile_resources as coalesce
import compact_retained_colors as colors
from compact_tile_slots import partition_like
from complete_retained_resources_ui17 import decoded_group
import content_tiles as t
from retained_filter import group_layout, transform_group
from repack_retained import DESCRIPTOR
import split_content as split
import storage_parts as storage

ENCODER = b'retained-live-slices-v1-fixed-groups-pages-codec'
PAGE = 4096


@dataclass
class SliceChoice(colors.Choice):
    raw_offsets: dict[tuple[int, int], int] | None = None
    padding: int = 0


def nonimage_groups(source):
    """Never compact an image group also addressed by another data domain."""
    offsets = {at for _cp, at, _px, _w, _h, _advance, encoded in source.glyphs
               if encoded & t.GLYPH_GROUP_FLAG}
    offsets.update(at for at, size in source.strings if size & t.STRING_GROUP_FLAG)
    return {at + DESCRIPTOR.unpack(source.block(at, 16))[0] for at in offsets}


def candidate(source, group, cache, protected):
    old = bytes(source.block(group.offset, group.size))
    raw_size, stream_at, mode, _table = group_layout(old, source.version)
    receipt = dict(first_image=group.first_image, input_bytes=len(old),
                   raw_bytes_before=raw_size, raw_bytes_after=raw_size,
                   removed_raw_bytes=0, saved_bytes=0, padding_bytes=0)
    result = SliceChoice(t.sha256(old), group.slices, None, receipt)
    if group.offset in protected:
        receipt['preserved_reason'] = 'nonimage_reference'
        return result
    end = 0
    for at, shape in group.slices:
        size = shape & 0xffffff
        t.require(size and shape >> 24 <= 4 and 0 <= at <= raw_size - size,
                  'Live retained slice outside original group')
        if at < end:
            receipt['preserved_reason'] = 'overlapping_live_slices'
            return result
        end = at + size
    raw = decoded_group(old, source.version)
    compact, slices, offsets, identical = bytearray(), [], {}, {}
    for at, shape in group.slices:
        payload = raw[at:at + (shape & 0xffffff)]
        # Dictionary equality compares the full bytes; a hash alone never
        # authorizes aliasing. Sharing remains inside this one existing group.
        key = shape, payload
        target = identical.get(key)
        if target is None:
            target = len(compact)
            identical[key] = target
            slices.append((target, shape))
            compact.extend(payload)
        offsets[at, shape] = target
    if len(compact) == raw_size:
        receipt['preserved_reason'] = 'no_unreachable_or_duplicate_bytes'
        return result
    compact = bytes(compact)
    t.require(0 < len(compact) < raw_size, 'Invalid retained compaction size')
    transformed = transform_group(compact, slices, mode) if mode else compact
    table = b''.join(struct.pack('<II', *row) for row in slices) if mode else b''
    # RGR3 mode zero is reported as 8, so its original table is retained.
    header = list(t.GROUP.unpack_from(old))
    header[1], header[4] = len(compact), len(slices)
    if mode:
        header[6] = len(table)
    key = t.sha256(ENCODER + old + b''.join(struct.pack('<II', *row) for row in group.slices))
    path = cache / (key + '.bin')
    if path.is_file():
        proposed = path.read_bytes()
    else:
        profiles = tuple(dict.fromkeys((coalesce.source_lzma_profile(old[stream_at:]), (3, 0, 2))))
        streams = [lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
            id=lzma.FILTER_LZMA2, preset=9, dict_size=t.GROUP_LIMIT,
            nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)]) for lc, lp, pb in profiles]
        stream = min(streams, key=len)
        header[2] = len(stream)
        proposed = t.GROUP.pack(*header) + table + stream
        t.atomic_bytes(path, proposed)
    size, _start, restored_mode, restored_slices = group_layout(proposed, source.version)
    expected = header.copy()
    expected[2] = t.GROUP.unpack_from(proposed)[2]
    t.require(tuple(expected) == t.GROUP.unpack_from(proposed) and size == len(compact) and
              restored_mode == mode and (not mode or restored_slices == tuple(slices)) and
              decoded_group(proposed, source.version) == compact,
              'Cached/new compact retained bytes differ')
    saved = colors.aligned(len(old)) - colors.aligned(len(proposed))
    if saved < PAGE:
        receipt['preserved_reason'] = 'cannot_preserve_page_phase_profitably'
        return result
    result.payload, result.raw_offsets, result.padding = proposed, offsets, saved % PAGE
    receipt.update(raw_bytes_after=len(compact), removed_raw_bytes=raw_size-len(compact),
                   saved_bytes=saved-result.padding, padding_bytes=result.padding,
                   live_slices=len(group.slices), stored_slices=len(slices),
                   pages_before=(raw_size+131071)//131072,
                   pages_after=(len(compact)+131071)//131072)
    return result


def make_plan(source, work, *, workers=2):
    t.require(1 <= workers <= 4, 'Use one through four bounded workers')
    t.require(source.version >= colors.CONTENT_VERSION, 'Preserve the current retained format version')
    cache = Path(work) / 'retained-slices'
    cache.mkdir(parents=True, exist_ok=True)
    protected = nonimage_groups(source)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        choices = list(pool.map(lambda group: candidate(source, group, cache, protected), colors.groups(source)))
    return choices


def rewrite(source, choices):
    catalog = colors.groups(source)
    t.require(len(catalog) == len(choices), 'Bilingual retained group counts differ')
    replacements, new_lengths, raw_offsets = [], {}, {}
    for group, choice in zip(catalog, choices):
        original = source.block(group.offset, group.size)
        t.require(t.sha256(original) == choice.original_sha256 and group.slices == choice.slices,
                      'Bilingual retained group or slice differs')
        if choice.payload is None:
            continue
        end = group.offset + group.size
        if end & 1:
            t.require(end < len(source.data) and source.data[end] == 0, 'Missing group alignment byte')
            end += 1
        t.require(isinstance(choice.padding, int) and 0 <= choice.padding < 4096 and
                      choice.padding % 2 == 0, 'Invalid retained physical padding')
        payload = choice.payload + bytes(len(choice.payload) & 1) + bytes(choice.padding)
        t.require(len(payload) < end - group.offset, 'Retained replacement is not smaller')
        replacements.append((group.offset, end, payload))
        new_lengths[group.offset] = len(choice.payload)
        if choice.raw_offsets is not None:
            t.require(set(choice.raw_offsets) == set(group.slices),
                          'Retained offset map does not cover all live slices')
            raw_offsets[group.offset] = choice.raw_offsets
    relocate = colors.Relocation(replacements)
    output, previous = bytearray(), 0
    for begin, end, payload in replacements:
        output.extend(source.data[previous:begin]); output.extend(payload); previous = end
    output.extend(source.data[previous:])
    for image, (off, w, h, fmt, length) in enumerate(source.images):
        t.IMAGE.pack_into(output, 40 + image * 16, relocate(off), w, h, fmt, length)
    for index, (cp, off, px, w, h, advance, encoded) in enumerate(source.glyphs):
        t.GLYPH.pack_into(output, source.glyph_at + index * 16,
            cp, relocate(off), px, w, h, advance, encoded)
    for index, (off, size) in enumerate(source.strings):
        struct.pack_into('<II', output, source.string_at + index * 8, relocate(off), size)
    header = list(t.GLOBAL.unpack_from(source.data, source.base_end))
    for index in (8, 9, 14):
        header[index] = relocate(header[index])
    t.GLOBAL.pack_into(output, relocate(source.base_end), *header)
    tiled = t.TileReader(source)
    old_table = t.GLOBAL.unpack_from(source.data, source.base_end)[9]
    for index in range(tiled.tile_groups + tiled.index_groups):
        off, length = struct.unpack_from('<II', source.data, old_table + index * 8)
        struct.pack_into('<II', output, relocate(old_table + index * 8), relocate(off), length)
    for index in range(tiled.frame_count):
        off = tiled.descriptors + index * 16
        global_at, group, raw_at, length = t.DESCRIPTOR.unpack_from(source.data, off)
        t.DESCRIPTOR.pack_into(output, relocate(off), relocate(global_at), group, raw_at, length)
    descriptors = {off for off, _w, _h, fmt, _size in source.images if fmt == 7}
    descriptors.update(off for _cp, off, _px, _w, _h, _advance, encoded in source.glyphs
                       if encoded & t.GLYPH_GROUP_FLAG)
    descriptors.update(off for off, length in source.strings if length & t.STRING_GROUP_FLAG)
    for off in descriptors:
        relative, length, raw_at, shape = DESCRIPTOR.unpack_from(source.data, off)
        group_at = off + relative
        new_off = relocate(off)
        if group_at in raw_offsets:
            t.require((raw_at, shape) in raw_offsets[group_at], 'Unmapped live retained slice')
            raw_at = raw_offsets[group_at][raw_at, shape]
        DESCRIPTOR.pack_into(output, new_off, relocate(group_at) - new_off,
            new_lengths.get(group_at, length), raw_at, shape)
    # Garbage collection uses the existing codec and never changes its version.
    struct.pack_into('<I', output, 4, source.version)
    struct.pack_into('<I', output, 12, len(output))
    struct.pack_into('<I', output, 36, 0)
    return bytes(output)


def verify(source, converted, choices):
    target = colors.view(converted)
    old_groups, new_groups = colors.groups(source), colors.groups(target)
    t.require(len(old_groups) == len(new_groups) == len(choices), 'Retained group count changed')
    compared = 0
    for old, new, choice in zip(old_groups, new_groups, choices):
        a, b = bytes(source.block(old.offset, old.size)), bytes(target.block(new.offset, new.size))
        t.require(old.first_image == new.first_image and (old.offset-new.offset) % PAGE == 0 and
                  t.sha256(a) == choice.original_sha256 and
                  b == (choice.payload if choice.payload is not None else a), 'Retained group identity changed')
        if choice.payload is None:
            continue
        ar, br = decoded_group(a, source.version), decoded_group(b, target.version)
        t.require(len(br) <= len(ar), 'Retained working set grew')
        for at, shape in old.slices:
            dest = choice.raw_offsets[at, shape]
            size = shape & 0xffffff
            t.require(ar[at:at+size] == br[dest:dest+size], 'A live retained byte changed')
            compared += size
    membership = {old.offset: (new.offset, new.size, choice)
                  for old, new, choice in zip(old_groups, new_groups, choices)}
    for old, new in zip(source.images, target.images):
        if old[3] != 7:
            continue
        relative, _size, at, shape = DESCRIPTOR.unpack(source.block(old[0], 16))
        nr, ns, na, nf = DESCRIPTOR.unpack(target.block(new[0], 16))
        group_at, group_size, choice = membership[old[0]+relative]
        expected_at = choice.raw_offsets[at, shape] if choice.raw_offsets is not None else at
        t.require((new[0]+nr, ns, na, nf) == (group_at, group_size, expected_at, shape),
                  'An image moved to a different retained group or slice')
    # This independently exercises every non-tile image and all font/text
    # slices. Tile/index streams are compared encoded byte-for-byte instead.
    proof = colors.verify(source, converted, all_tiles=False)
    t.require(source.version == target.version and (len(source.data)-len(converted)) % PAGE == 0,
              'Resource format or suffix page phase changed')
    for old, new in zip(source.glyphs, target.glyphs):
        t.require((old[1]-new[1]) % PAGE == 0, 'Glyph page phase changed')
    for old, new in zip(source.strings, target.strings):
        t.require((old[0]-new[0]) % PAGE == 0, 'String page phase changed')
    proof.update(compared_live_slice_bytes=compared, group_membership_unchanged=True,
                 page_phases_unchanged=True, runtime_memory_added=0)
    return proof


def convert(source, target, work, *, locales=None, workers=2):
    source, target, work = (t.private_path(path, 'Private resource path') for path in (source, target, work))
    t.require(source != target and source not in target.parents and target not in source.parents,
              'Never overwrite the source resources')
    codes = list(locales or split.LANGUAGES)
    t.require('eng' in codes and len(codes) == len(set(codes)) and set(codes) <= set(split.LANGUAGES),
              'Requested locales must be supported, unique and include English')
    names = ('nSpire2.dat.tns', *storage.PART_FILES, *(f'nSpire2.{code}.dat.tns' for code in codes))
    t.require(all((source/name).is_file() for name in names), 'A requested resource file is missing')
    t.require(not any((target/name).exists() for name in (*names, 'retained-slices-receipt.json')),
              'Output already contains resources; choose a fresh directory')
    target.mkdir(parents=True, exist_ok=True)
    index = (source/'nSpire2.dat.tns').read_bytes()
    shared = storage.join_parts(index, {n:(source/n).read_bytes() for n in storage.PART_FILES})
    english = split.reconstruct(shared, (source/'nSpire2.eng.dat.tns').read_bytes(), 'eng')
    initial = colors.view(english)
    choices = make_plan(initial, work, workers=workers)
    report = dict(schema=1, version=initial.version, complete=False, locales={},
                  groups=len(choices), changed_groups=sum(c.payload is not None for c in choices),
                  raw_bytes_removed=sum(c.receipt['removed_raw_bytes'] for c in choices),
                  page_aligned_bytes_saved=sum(c.receipt['saved_bytes'] for c in choices),
                  runtime_memory_added=0, results=[c.receipt for c in choices])
    before = sum((source/name).stat().st_size for name in names)
    # Publish no partial candidate: every locale must pass before replacing
    # any output .tns. An interrupted conversion leaves its source intact.
    with TemporaryDirectory(prefix='.retained-slices-', dir=target) as folder:
        stage, common = Path(folder), None
        for code in ('eng', *(c for c in codes if c != 'eng')):
            original = english if code == 'eng' else split.reconstruct(shared, (source/f'nSpire2.{code}.dat.tns').read_bytes(), code)
            old = colors.view(original)
            converted = rewrite(old, choices)
            proof = verify(old, converted, choices)
            bounds = split.layout(converted, code)
            artwork = converted[bounds['prefix']:bounds['prefix']+bounds['artwork']]
            if common is None:
                common, new_index, parts, partition = partition_like(index, english, converted)
                t.atomic_bytes(stage/'nSpire2.dat.tns', new_index)
                for name, payload in parts.items():
                    t.atomic_bytes(stage/name, payload)
                report['artwork'] = partition
            else:
                t.require(common[64:] == artwork, 'Locales lost identical shared artwork')
            locale = split.compress_language_directory(split.language_file(converted, bounds, code, bytes(32)))
            locale, _details = storage.compress_blocks(locale)
            t.require(split.reconstruct(common, locale, code) == converted, 'Compacted locale reconstruction differs')
            t.atomic_bytes(stage/f'nSpire2.{code}.dat.tns', locale)
            report['locales'][code] = proof
        candidate_bytes = sum((stage/name).stat().st_size for name in names)
        accepted = bool(report['changed_groups'] and candidate_bytes < before)
        for name in names:
            if not accepted:
                t.atomic_bytes(stage/name, (source/name).read_bytes())
            os.replace(stage/name, target/name)
        report.update(complete=True, beneficial=accepted, candidate_installed_bytes=candidate_bytes,
                      installed_bytes_before=before, installed_bytes_after=candidate_bytes if accepted else before,
                      installed_bytes_saved=before-candidate_bytes if accepted else 0,
                      candidate_selected=accepted)
    report['files'] = {name: dict(bytes=(target/name).stat().st_size,
                                  sha256=t.sha256((target/name).read_bytes())) for name in names}
    t.atomic_bytes(target/'retained-slices-receipt.json', json.dumps(report, indent=2).encode()+b'\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('input', 'output', 'work'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--workers', type=int, default=2)
    parser.add_argument('--locales', nargs='+', choices=split.LANGUAGES)
    args = parser.parse_args()
    report = convert(args.input, args.output, args.work, locales=args.locales, workers=args.workers)
    print(json.dumps({k:v for k,v in report.items() if k not in ('results', 'locales', 'artwork')}), flush=True)


if __name__ == '__main__':
    main()
