#!/usr/bin/env python3
"""Restore exact original Card Library font bands without raising glyph limits.

The Chinese selector append counted nine color-tag characters as italic4px
flavor glyphs. The actual RichReader consumes those tags; no other screen
selects that face/size. Retire only those proven unused directory keys, keeping
even their old payload bytes. Append only missing glyphs for the library's
original labels and counters. All artwork, groups, strings, animation clocks
and every other existing glyph remain byte-exact; NSP2/34 is unchanged.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import struct

import content_tiles as tiles
from replace_original_glyphs import Face, visible_alphabet
from selection_ui9_assets import flavor_visible_alphabet, source_text_index
import split_content

VERSION, AUXILIARY, CLIPS, STRINGS = 34, 2061, 1783, 8704
PROFILE = (558, STRINGS, CLIPS, AUXILIARY)
GLYPH_LIMIT = 32768
FLAVOR_BAND = 4 | 160
# Exact keys introduced by the original stage19 flavor collector. The pinned
# source's306 Chinese flavors were independently fed through C++ RichReader;
# none emits these characters. Do not expand this list to arbitrary ASCII.
RETIRED_FLAVOR_KEYS = frozenset((ord(char), FLAVOR_BAND) for char in '/[]deglor')
BOLD_LABEL_KEYS = tuple('gameplay_ui.SORT_'+key for key in
                        ('TYPE', 'RARITY', 'COST', 'ALPHABET')) + tuple(
    'card_library.'+key for key in
    ('RARITY_COMMON', 'RARITY_UNCOMMON', 'RARITY_RARE', 'RARITY_OTHER',
     'VIEW_STATS', 'VIEW_UPGRADES'))


def library_text_index():
    result = source_text_index()
    source = Path(__file__).resolve().parents[1]/'src/ui7_text_index.inc'
    result.update((key, int(slot)) for key, slot in
                  re.findall(r'\{"([^"]+)",\s*(\d+)\}', source.read_text()))
    return result


def glyph_requirements(source):
    """Only actual label bands; placeholders contribute their real values."""
    index, result = library_text_index(), set()

    def add(key, pixels, face, substitutions=()):
        tiles.require(key in index, 'Unbound original Card Library text: '+key)
        text = source.string_payload(index[key])[:-1].decode('utf-8')
        tiles.require(bool(text), 'Empty original Card Library text: '+key)
        for name in substitutions:
            text = text.replace('{'+name+'}', '0123456789')
        tiles.require('{' not in text and '}' not in text,
                      'Unrecognized Card Library substitution: '+key)
        alphabet = flavor_visible_alphabet(text)
        result.update((cp, pixels, face) for cp in alphabet)

    for key in BOLD_LABEL_KEYS:
        add(key, 4, 5)
    add('card_library.CARD_COUNT', 4, 0, ('Amount',))
    add('card_library.SEARCH_PLACEHOLDER', 5, 0)
    add('card_library.NO_RESULTS', 6, 0)
    for pixels in (5, 6):
        add('card_library.VICTORIES', pixels, 0, ('Victories',))
    # The native cost filters contain these original fixed symbols.
    result.update((cp, 5, 5) for cp in visible_alphabet('0123+X'))
    return result


def encoded_band(pixels, face):
    return pixels | ((face & 3) << 6) | ((face & 4) << 3)


def retireable_flavor_keys(source, language):
    """Refuse retirement if any currently reachable flavor needs a key."""
    tiles.require(language in ('eng', 'zhs'), 'Unexpected locale marker')
    if language != 'zhs':
        return set()
    index = source_text_index()
    flavors = tuple(key for key in index if
                    key.startswith('relics.') and key.endswith('.flavor'))
    tiles.require(bool(flavors), 'Relic flavor index is missing')
    alphabet = set()
    for key in flavors:
        text = source.string_payload(index[key])[:-1].decode('utf-8')
        tiles.require(bool(text), 'Empty retained original relic flavor: '+key)
        alphabet.update(flavor_visible_alphabet(text))
    known = {(row[0], row[2]) for row in source.glyphs}
    retire = known & RETIRED_FLAVOR_KEYS
    tiles.require(not {cp for cp, _band in retire} & alphabet,
                  'A proposed retired flavor key is visible in current text')
    return retire


def rewrite_directory(data, auxiliary, retired, additions, *, profile=None, output_version=VERSION):
    """Relocate the directory once; keep the entire old payload suffix.

    Relative RGR glyph/string/image descriptors move together with their
    groups. TIL1's absolute offsets are the only payload-adjacent fields
    rewritten. Unreferenced retired glyph payloads are deliberately retained.
    """
    source = tiles.PackView(data, auxiliary, versions=(VERSION, 35), profile=profile)
    tiles.require(source.version <= output_version <= 35, 'Unsupported font version transition')
    old_tiles = tiles.TileReader(source)
    known = {(row[0], row[2]) for row in source.glyphs}
    retired = set(retired)
    tiles.require(retired <= RETIRED_FLAVOR_KEYS and retired <= known,
                  'Retirement is restricted to proven selector markup keys')
    new_keys = {(row[0], row[1]) for row in additions}
    tiles.require(len(new_keys) == len(additions) and not new_keys & known,
                  'Added Card Library glyph keys must be new and unique')
    ni, ng, ns, nc, nk, _ = source.counts
    count = ng-len(retired)+len(additions)
    limit = 65535 if output_version >= 35 else GLYPH_LIMIT
    tiles.require(0 < count <= limit, 'Card Library glyph directory exceeds'+str(limit))
    delta = (count-ng)*tiles.GLYPH.size
    out = bytearray(data[:source.glyph_at])
    out.extend(bytes(count*tiles.GLYPH.size))
    out.extend(data[source.string_at:])
    rows = [(cp, off+delta, band, w, h, advance, encoded)
            for cp, off, band, w, h, advance, encoded in source.glyphs
            if (cp, band) not in retired]
    for cp, band, w, h, advance, encoded, raw in additions:
        tiles.require(0 < w <= 40 and 0 < h <= 40 and
                      0 <= encoded < 0x8000 and len(raw) == (encoded or w*h),
                      'Invalid added Card Library glyph')
        rows.append((cp, len(out), band, w, h, advance, encoded))
        out.extend(raw)
    rows.sort(key=lambda row: (row[2], row[0]))
    for i, row in enumerate(rows):
        tiles.GLYPH.pack_into(out, source.glyph_at+i*16, *row)
    for i, (off, w, h, fmt, length) in enumerate(source.images):
        tiles.IMAGE.pack_into(out, 40+i*16, off+delta, w, h, fmt, length)
    for i, (off, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at+delta+i*8, off+delta, length)
    header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    table = header[9]
    for i in (8, 9, 14):
        header[i] += delta
    tiles.GLOBAL.pack_into(out, source.base_end+delta, *header)
    for i in range(old_tiles.tile_groups+old_tiles.index_groups):
        off, length = struct.unpack_from('<II', data, table+i*8)
        struct.pack_into('<II', out, table+delta+i*8, off+delta, length)
    for i in range(old_tiles.frame_count):
        at = old_tiles.descriptors+i*16
        global_at, group, offset, length = tiles.DESCRIPTOR.unpack_from(data, at)
        tiles.DESCRIPTOR.pack_into(out, at+delta, global_at+delta, group, offset, length)
    tiles.HEADER.pack_into(out, 0, b'NSP2', output_version, tiles.REFERENCE,
                           len(out), ni, count, ns, nc, nk, 0)
    target = tiles.PackView(out, auxiliary, versions=(output_version,), profile=profile)
    tiles.TileReader(target)
    return bytes(out)


def verify_preserved(before, after, auxiliary, retired):
    source = tiles.PackView(before, auxiliary, versions=(VERSION, 35))
    target = tiles.PackView(after, auxiliary, versions=(VERSION, 35))
    tiles.require(source.cards == target.cards and source.clips == target.clips and
                  source.aux == target.aux, 'Card Library patch changed model or clip indices')
    delta = (len(target.glyphs)-len(source.glyphs))*tiles.GLYPH.size
    tiles.require(len(source.images) == len(target.images), 'Image count changed')
    for old, new in zip(source.images, target.images):
        tiles.require(old[1:] == new[1:] and old[0]+delta == new[0],
                      'An image directory entry changed beyond relocation')
    keys = {(row[0], row[2]): i for i, row in enumerate(target.glyphs)}
    for i, old in enumerate(source.glyphs):
        key = (old[0], old[2])
        if key in retired:
            tiles.require(key not in keys, 'A retired markup key is still present')
            continue
        at = keys.get(key)
        tiles.require(at is not None, 'A required old glyph key disappeared')
        new = target.glyphs[at]
        tiles.require(old[2:] == new[2:] and old[1]+delta == new[1] and
                      source.glyph_payload(i) == target.glyph_payload(at) and
                      source.glyph_alpha(i) == target.glyph_alpha(at),
                      'An existing glyph metric, encoding or alpha changed')
    tiles.require(len(source.strings) == len(target.strings), 'String count changed')
    for i, old in enumerate(source.strings):
        tiles.require((old[0]+delta, old[1]) == target.strings[i] and
                      source.string_payload(i) == target.string_payload(i),
                      'An existing string or grouped-string descriptor changed')
    language = source.string_payload(split_content.MARKER)[:-1].decode('ascii')
    old, new = split_content.layout(before, language), split_content.layout(after, language)
    tiles.require(old['artwork'] == new['artwork'] and
                  old['prefix']+delta == new['prefix'], 'Common artwork bounds changed')
    start = old['prefix']
    tiles.require(before[start:] == after[start+delta:len(before)+delta],
                  'Old artwork, glyph or string payload bytes changed')
    art = before[start:start+old['artwork']]
    return dict(old_glyphs_verified=len(source.glyphs)-len(retired),
                retired_payloads_retained=True, exact_other_glyph_keys_metrics_alpha=True,
                exact_strings=True, exact_model_and_clip_indices_clocks=True,
                exact_image_directories=True, exact_complete_old_payload=True,
                common_art_bytes=len(art), common_art_sha256=tiles.sha256(art),
                old_payload_sha256=tiles.sha256(before[start:]),
                runtime_cache_growth_bytes=0)


def patch(data, regular, bold):
    source = tiles.PackView(data, AUXILIARY, versions=(VERSION,), profile=PROFILE)
    language = source.string_payload(split_content.MARKER)[:-1].decode('ascii')
    retired = retireable_flavor_keys(source, language)
    known = {(row[0], row[2]) for row in source.glyphs}
    required = glyph_requirements(source)
    missing = sorted((cp, pixels, face) for cp, pixels, face in required
                     if (cp, encoded_band(pixels, face)) not in known)
    total = len(source.glyphs)-len(retired)+len(missing)
    tiles.require(total <= GLYPH_LIMIT,
                  f'Card Library requires{total} glyphs; unchanged limit32768')
    additions = []
    for cp, pixels, face in missing:
        font = bold if face == 5 else regular
        tiles.require(cp in font.coverage, 'Original library font lacks codepoint '+str(cp))
        additions.append(font.glyph(cp, pixels, face))
    out = rewrite_directory(data, AUXILIARY, retired, additions, profile=PROFILE)
    proof = verify_preserved(data, out, AUXILIARY, retired)
    target = tiles.PackView(out, AUXILIARY, versions=(VERSION,), profile=PROFILE)
    actual = {(row[0], row[2]) for row in target.glyphs}
    tiles.require(all((cp, encoded_band(px, face)) in actual for cp, px, face in required),
                  'A required Card Library glyph is absent')
    tiles.require(not retireable_flavor_keys(target, language), 'A retired key reappeared')
    receipt = dict(schema=1, reference='v0.111.0/41cef1ea', version=VERSION, language=language,
        input_bytes=len(data), output_bytes=len(out), input_sha256=tiles.sha256(data),
        output_sha256=tiles.sha256(out), input_glyphs=len(source.glyphs), glyphs=len(target.glyphs),
        retired_glyphs=[dict(codepoint=cp, pixels=4, face=6) for cp, _ in sorted(retired)],
        added_glyphs=[dict(codepoint=cp, pixels=px, face=face) for cp, px, face in missing],
        remaining_glyph_slots=GLYPH_LIMIT-len(target.glyphs), missing_required_glyphs=0,
        fonts=dict(regular=regular.metadata(), bold=bold.metadata()), verification=proof)
    return out, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('input-eng', 'input-zhs', 'font-dir', 'output-dir'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    fonts = tiles.private_path(args.font_dir, 'Original font directory')
    output = tiles.private_path(args.output_dir, 'Private Card Library font output')
    reports = {}
    for language, names in (('eng', ('kreon_regular.ttf', 'kreon_bold.ttf')),
                            ('zhs', ('NotoSansMonoCJKsc-Regular.otf', 'SourceHanSerifSC-Bold.otf'))):
        path = tiles.private_path(getattr(args, 'input_'+language), 'Immutable v34 input pack')
        target = output/('ui9-'+language+'.nsp')
        tiles.require(path.resolve() != target.resolve(), 'Preserve the previous pack')
        out, report = patch(path.read_bytes(), *(Face(fonts/name) for name in names))
        tiles.atomic_bytes(target, out)
        reports[language] = report
    tiles.require(reports['eng']['verification']['common_art_sha256'] ==
                  reports['zhs']['verification']['common_art_sha256'],
                  'Locale artwork cannot be shared exactly')
    tiles.atomic_bytes(output/'card-library-fonts.json',
                       json.dumps(reports, indent=2).encode()+b'\n')
    print(json.dumps(reports), flush=True)


if __name__ == '__main__':
    main()
