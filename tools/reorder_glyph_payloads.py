#!/usr/bin/env python3
"""Reorder an existing private NSP2/20-22 glyph section without changing it.

Frequently used localized characters can be ordered within each native size
or with their different native sizes together. The ordinary 4 KiB reader then
reuses more of each requested page. Compare both policies with real visible
pages before selecting either; grouping sizes can add reads to a page that
only uses one size. There is no new runtime cache, font pre-read, preparation
phase, image encoding or installation byte. Only glyph offsets and the
original packed glyph section may change.
"""
from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
import json
from pathlib import Path
import struct

from content_tiles import private_path, atomic_bytes


HEADER = struct.Struct('<4s9I')
GLYPH = struct.Struct('<IIHBBHH')
REFERENCE = 0x41CEF1EA
ORDERS = ('character-frequency', 'size-frequency')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def aligned(value):
    return (value + 1) & ~1


@dataclass
class Layout:
    version: int
    glyph_at: int
    glyphs: list
    strings: list
    first: int
    last: int
    units: dict


def layout(data):
    """Accept the dedicated glyph tail emitted by the retained-pack writer.

    Reject an interleaved/overlapping layout rather than move unidentified
    payloads. Aliases of an identical offset/length remain one physical unit.
    These are converter-time bounds, not startup integrity work.
    """
    require(len(data) >= HEADER.size, 'Truncated pack header')
    magic, version, reference, size, ni, ng, ns, _nc, _nk, _checksum = HEADER.unpack_from(data)
    require(magic == b'NSP2' and version in (20, 21, 22) and reference == REFERENCE,
            'Expected a pinned retained NSP2/20, 21 or 22 pack')
    require(size == len(data) and 0 < ni <= 65535 and 0 < ng <= 32768 and 0 < ns <= 16384,
            'Invalid retained pack bounds')
    glyph_at = HEADER.size + ni * 16
    string_at = glyph_at + ng * GLYPH.size
    require(string_at + ns * 8 <= len(data), 'Truncated glyph/string directory')
    glyphs = [GLYPH.unpack_from(data, glyph_at + i * GLYPH.size) for i in range(ng)]
    strings = [struct.unpack_from('<II', data, string_at + i * 8) for i in range(ns)]
    units, previous = {}, -1
    for cp, offset, px, width, height, advance, encoded in glyphs:
        key = (px << 24) | cp
        require(cp <= 0x10ffff and 0 < px <= 32 and key > previous and
                0 < width <= 40 and 0 < height <= 40 and advance <= 40 and
                (not encoded or encoded % 2 == 0 and encoded <= 2 * width * height),
                'Invalid or unordered glyph geometry')
        previous = key
        length = encoded or width * height
        require(offset >= string_at + ns * 8 and offset <= len(data) - length and offset % 2 == 0,
                'Glyph payload outside its retained tail')
        units[(offset, length)] = bytes(data[offset:offset + length])
    first = min(offset for offset, _ in units)
    last = min(offset for offset, _ in strings)
    require(first < last <= len(data), 'Missing dedicated glyph section')
    cursor = first
    for (offset, length), _payload in sorted(units.items()):
        require(offset == aligned(cursor) and not any(data[cursor:offset]),
                'Glyph section is interleaved, overlapping or has unknown padding')
        cursor = offset + length
    require(last == aligned(cursor) and not any(data[cursor:last]),
            'Glyph section does not end at the unchanged string section')
    for offset, length in strings:
        require(offset >= last and length < 16384 and offset <= len(data) - length - 1 and
                data[offset + length] == 0, 'Invalid string payload')
    for i in range(ni):
        offset, _w, _h, _format, length = struct.unpack_from('<IHHII', data, HEADER.size + i * 16)
        require(offset + length <= first or offset >= last, 'Image payload overlaps the glyph section')
    return Layout(version, glyph_at, glyphs, strings, first, last, units)


def verify(source, result):
    before, after = layout(source), layout(result)
    require(len(result) == len(source), 'Reordering must not add installation bytes')
    require((before.version, before.glyph_at, before.first, before.last) ==
            (after.version, after.glyph_at, after.first, after.last), 'Section boundaries changed')
    directory_end = before.glyph_at + len(before.glyphs) * GLYPH.size
    require(source[:before.glyph_at] == result[:after.glyph_at] and
            source[directory_end:before.first] == result[directory_end:after.first] and
            source[before.last:] == result[after.last:],
            'Header, images, tile groups, strings or other non-glyph bytes changed')
    aliases = {}
    for old, new in zip(before.glyphs, after.glyphs):
        require(old[:1] + old[2:] == new[:1] + new[2:], 'Glyph key or geometry changed')
        length = old[6] or old[3] * old[4]
        require(source[old[1]:old[1] + length] == result[new[1]:new[1] + length],
                'Encoded glyph pixels changed')
        original = (old[1], length)
        require(aliases.setdefault(original, new[1]) == new[1], 'An existing glyph alias was duplicated')
    require(len(set(aliases.values())) == len(aliases), 'Different physical glyph units were merged')
    return dict(glyphs=len(before.glyphs), physical_units=len(before.units),
                glyph_pixels_exact=True, glyph_geometry_exact=True, aliases_exact=True,
                non_glyph_bytes_exact=True, unchanged_file_bytes=len(source))


def reorder(data, order='character-frequency'):
    require(order in ORDERS, 'Unknown glyph locality order')
    source = layout(data)
    frequency = Counter()
    # A repeated localization entry is a repeated UI use, while payload aliases
    # remain shared in the resource. Count decoded characters, never UTF-8 bytes.
    for offset, length in source.strings:
        frequency.update(data[offset:offset + length].decode('utf-8'))
    priority = {}
    for index, row in enumerate(source.glyphs):
        cp, offset, px, width, height, _advance, encoded = row
        unit = (offset, encoded or width * height)
        key = ((-frequency[chr(cp)], cp, px, index) if order == 'character-frequency'
               else (px, -frequency[chr(cp)], cp, index))
        if unit not in priority or key < priority[unit]:
            priority[unit] = key
    moved, offsets, cursor = bytearray(), {}, source.first
    for unit in sorted(source.units, key=lambda item: priority[item]):
        padding = aligned(cursor) - cursor
        moved.extend(bytes(padding)); cursor += padding
        offsets[unit] = cursor
        payload = source.units[unit]
        moved.extend(payload); cursor += len(payload)
    moved.extend(bytes(aligned(cursor) - cursor))
    require(len(moved) == source.last - source.first, 'Glyph physical size changed')
    result = bytearray(data)
    result[source.first:source.last] = moved
    changed = 0
    for index, row in enumerate(source.glyphs):
        cp, offset, px, width, height, advance, encoded = row
        target = offsets[(offset, encoded or width * height)]
        changed += target != offset
        GLYPH.pack_into(result, source.glyph_at + index * GLYPH.size,
                        cp, target, px, width, height, advance, encoded)
    result = bytes(result)
    proof = verify(data, result)
    return result, dict(schema=1, reference='v0.111.0 / 41cef1ea', version=source.version,
        method=order + '; localized codepoint frequency; existing 2-byte alignment',
        input_bytes=len(data), output_bytes=len(result), installation_growth_bytes=0,
        glyph_section_bytes=source.last - source.first, relocated_glyph_entries=changed,
        runtime_buffer_growth_bytes=0, runtime_preload_bytes=0, verification=proof)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--eng', type=Path, required=True)
    parser.add_argument('--zhs', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--order', choices=ORDERS, default='character-frequency',
                        help='Physical locality policy; keep the same runtime for comparison')
    args = parser.parse_args()
    sources = {language: private_path(getattr(args, language), 'Private input pack')
               for language in ('eng', 'zhs')}
    output = private_path(args.output, 'Private output directory')
    require(len(set(sources.values())) == 2, 'Language inputs must be separate files')
    targets = {language: output / ('glyph-locality-' + language + '.nsp') for language in sources}
    require(not set(target.resolve() for target in targets.values()) & set(sources.values()),
            'Outputs must not replace either input')
    output.mkdir(parents=True, exist_ok=True)
    receipts = {}
    for language, path in sources.items():
        result, receipts[language] = reorder(path.read_bytes(), args.order)
        atomic_bytes(targets[language], result)
    atomic_bytes(output / 'glyph-locality.json', json.dumps(receipts, indent=2).encode() + b'\n')
    print(json.dumps(receipts, indent=2))


if __name__ == '__main__':
    main()
