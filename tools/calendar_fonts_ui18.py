#!/usr/bin/env python3
"""Append only the original-font glyphs required by dynamic calendar labels.

The private original-culture oracle supplies all twelve months/times. Existing
glyph keys, metrics, alpha, strings and compressed artwork remain unchanged.
Thai calendar graphemes use eleven fixed display keys, disjoint from both the
locale's old PUA clusters and the ASCII-kerning metadata interval. No font,
generated pack or oracle payload belongs in the public source repository.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import shutil
import unicodedata

from PIL import Image, ImageDraw
import regex

import content_tiles as tiles
import split_content as split
import storage_parts
from build_locale_editions import FONT_NAMES, glyph_for
from card_library_fonts import rewrite_directory, verify_preserved
from replace_original_glyphs import Face, text_format_control
from ui12_resources import view

ROOT = Path(__file__).resolve().parents[1]
LANGUAGES = ('eng', 'zhs', 'zht', 'deu', 'esp', 'fra', 'ind', 'ita',
             'jpn', 'kor', 'pol', 'ptb', 'rus', 'spa', 'tha', 'tur')
SUBSTITUTED = frozenset(('zhs', 'zht', 'jpn', 'kor', 'pol', 'rus', 'tha'))


def calendar_clusters():
    text = (ROOT/'include/ui_calendar_locale_data.inc').read_text()
    body = text.split('ThaiMonthClusters[]={', 1)[1].split('};', 1)[0]
    result = {json.loads(word): int(cp, 16) for word, cp in
              re.findall(r'\{("(?:[^"\\]|\\.)*"), (0x[0-9a-f]+)\}', body)}
    tiles.require(len(result) == 11 and set(result.values()) == set(range(0x10f000, 0x10f00b)),
                  'Calendar cluster contract changed')
    for word in result:
        tiles.require(regex.findall(r'\X', word) == [word] and len(word.encode()) > 4 and
                      len(word) > 1 and any(unicodedata.category(c).startswith('M') for c in word),
                      'Calendar key is not one complete shrinking Thai grapheme')
    return result


def shaped(text, code, clusters):
    if code != 'tha':
        return text
    return ''.join(chr(clusters[word]) if word in clusters else word
                   for word in regex.findall(r'\X', text))


def requirements(path):
    clusters = calendar_clusters()
    wanted = {code: set() for code in LANGUAGES}
    months = {code: set() for code in LANGUAGES}
    patterns = {}
    count = 0
    for line in path.read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        fields = line.split('\t')
        tiles.require(len(fields) == 8, 'Invalid calendar vector row')
        language, timestamp = map(int, fields[:2])
        tiles.require(1 <= language <= 16 and 0 <= timestamp <= 0xffffffff,
                      'Calendar vector outside supported range')
        code = LANGUAGES[language-1]
        values = [bytes.fromhex(field).decode() for field in fields[2:]]
        prior = patterns.setdefault(code, values[:3])
        tiles.require(prior == values[:3], 'Conflicting calendar resource patterns')
        months[code].add(datetime.fromtimestamp(timestamp, timezone.utc).month)
        for value, size, face in ((values[3], 8, 0), (values[4], 8, 0),
                                  (values[5], 10, 0 if code in SUBSTITUTED else 5)):
            if size == 8:
                value = shaped(value, code, clusters)
            wanted[code].update((ord(c), size, face) for c in value if not text_format_control(ord(c)))
        count += 1
    tiles.require(all(months[code] == set(range(1, 13)) for code in LANGUAGES),
                  'The font gate requires all twelve months for all sixteen languages')
    return wanted, patterns, clusters, count


def new_glyph(font, cp, text, size, face):
    tiles.require(set(map(ord, text)) <= font.coverage,
                  'Original font does not cover a required calendar grapheme')
    row = glyph_for(font, cp, text, size, face)
    # Independently render the original shaped grapheme and compare with the
    # stored row's complete decoded alpha, including advance and baseline.
    measured = font.fonts[size]
    width, height, advance, encoded, raw = row[2:]
    expected_advance = max(1, int(measured.getlength(text)+0.5))
    tiles.require(advance == expected_advance and width == max(advance, measured.getbbox(text)[2]) and height == size+5,
                  'New calendar glyph metrics differ from the original font')
    pixels = bytearray()
    if encoded:
        for at in range(0, len(raw), 2):
            pixels.extend(bytes((raw[at+1],))*raw[at])
    else:
        pixels.extend(raw)
    reference = Image.new('L', (width, height))
    ImageDraw.Draw(reference).text((0, size+1), text, font=measured, fill=255, anchor='ls')
    tiles.require(bytes(pixels) == reference.tobytes(), 'New calendar glyph alpha differs from the original font')
    return row


def patch(data, wanted, patterns, clusters, fonts):
    source = view(data)
    code = source.string_payload(split.MARKER)[:-1].decode('ascii')
    for slot, pattern in zip((8258, 8261, 8420), patterns[code]):
        tiles.require(source.string_payload(slot) == pattern.encode()+b'\0',
                      'Reference oracle pattern differs from this locale resource')
    known = {(row[0], row[2]) for row in source.glyphs}
    missing = sorted((cp, px, face) for cp, px, face in wanted[code]
                     if (cp, px|((face&3)<<6)|((face&4)<<3)) not in known)
    result = dict(language=code, required_glyphs=len(wanted[code]),
                  added_glyphs=missing, added_logical_bytes=0, original_fonts=[])
    if not missing:
        return data, result
    names = FONT_NAMES.get(code, FONT_NAMES['latin'])
    inverse = {cp: text for text, cp in clusters.items()}
    loaded, additions = {}, []
    for cp, size, face in missing:
        name = names[1 if face in (1, 3, 5) else 2 if face in (2, 6) else 3 if face == 4 else 0]
        if name not in loaded:
            path = fonts/name
            loaded[name] = Face(path)
            result['original_fonts'].append(dict(name=name, sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
        additions.append(new_glyph(loaded[name], cp, inverse.get(cp, chr(cp)), size, face))
    output = rewrite_directory(data, len(source.aux), set(), additions, output_version=source.version)
    proof = verify_preserved(data, output, len(source.aux), set())
    after = view(output)
    keys = {(row[0], row[2]) for row in after.glyphs}
    tiles.require(all((cp, px|((face&3)<<6)|((face&4)<<3)) in keys for cp, px, face in wanted[code]),
                  'Patched calendar still lacks an exact glyph')
    result.update(added_logical_bytes=len(output)-len(data), verification=proof,
                  new_glyph_count=len(after.glyphs), resident_directory_bytes=split.u32(output, after.base_end+8*4)-len(after.images)*4)
    return output, result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--fonts', required=True, type=Path)
    parser.add_argument('--vectors', required=True, type=Path)
    parser.add_argument('--logical-output', type=Path)
    args = parser.parse_args()
    source, target = tiles.private_path(args.input, 'Private input'), tiles.private_path(args.output, 'Private output')
    tiles.require(source != target, 'Keep the original edition immutable')
    target.mkdir(parents=True, exist_ok=True)
    logical_output = tiles.private_path(args.logical_output, 'Private logical output') if args.logical_output else None
    if logical_output:
        logical_output.mkdir(parents=True, exist_ok=True)
    wanted, patterns, clusters, count = requirements(args.vectors)
    common_files = ('nSpire2.dat.tns', *storage_parts.PART_FILES)
    common = storage_parts.join_parts((source/common_files[0]).read_bytes(),
                                     {name: (source/name).read_bytes() for name in storage_parts.PART_FILES})
    for name in common_files:
        shutil.copyfile(source/name, target/name)
    reports = {}
    for code in LANGUAGES:
        name = 'nSpire2.'+code+'.dat.tns'
        old = (source/name).read_bytes()
        data = split.reconstruct(common, old, code)
        changed, report = patch(data, wanted, patterns, clusters, args.fonts)
        if report['added_glyphs']:
            bounds = split.layout(changed, code)
            leaf = split.compress_language_directory(split.language_file(changed, bounds, code, common[32:64]))
            leaf, storage = storage_parts.compress_blocks(leaf)
            report['storage'] = storage
            tiles.require(split.reconstruct(common, leaf, code) == changed, 'Patched locale reconstruction differs')
        else:
            leaf = old
        tiles.atomic_bytes(target/name, leaf)
        if logical_output:
            tiles.atomic_bytes(logical_output/(code+'.nsp'), changed)
        report.update(old_locale_bytes=len(old), new_locale_bytes=len(leaf),
                      locale_delta=len(leaf)-len(old), new_locale_sha256=tiles.sha256(leaf))
        reports[code] = report
        print(json.dumps(report, ensure_ascii=False), flush=True)
    result = dict(reference='v0.111.0 / 41cef1ea', vectors=count, locales=reports,
                  total_locale_delta=sum(r['locale_delta'] for r in reports.values()),
                  common_files_unchanged=all((source/name).read_bytes() == (target/name).read_bytes() for name in common_files))
    (target/'calendar-fonts.json').write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n')


if __name__ == '__main__':
    main()
