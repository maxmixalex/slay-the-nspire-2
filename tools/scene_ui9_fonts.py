#!/usr/bin/env python3
"""Append source-sized profile and intent fonts in the fixed directory pool.

v35 keeps every old image, glyph, string and animation byte and uses the same
2 MiB directory. Only the count bound changes: all readers already use 32-bit
glyph indexes. No image resampling, runtime scan or new resident pool is added.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import re
import content_tiles as tiles
import card_library_fonts as library
from replace_original_glyphs import Face, visible_alphabet
import split_content

ROOT = Path(__file__).resolve().parents[1]
VERSION = 35
PROFILE = library.PROFILE
AUXILIARY = library.AUXILIARY
RICH_TAG = re.compile(r'\[/?(?:gold|red|blue|green|purple|orange|aqua|white|gray|b|i)\]')


def text_index():
    result = library.library_text_index()
    result.update((key, int(slot)) for key, slot in re.findall(
        r'\{"([^"]+)",\s*(\d+)\}', (ROOT/'src/profile_text_index.inc').read_text()))
    return result


def profile_requirements(source):
    index, result = text_index(), set()
    labels = json.loads((ROOT/'config/profile-ui-fonts.json').read_text())['labels']
    for label in labels:
        key = label['key']
        tiles.require(key in index, 'Unbound profile text: '+key)
        text = source.string_payload(index[key])[:-1].decode('utf-8')
        tiles.require(bool(text), 'Empty profile text: '+key)
        for name, values in label.get('substitutions', {}).items():
            text = text.replace('{'+name+'}', values)
        tiles.require('{' not in text and '}' not in text, 'Unknown profile substitution: '+key)
        plain = RICH_TAG.sub('', text)
        tiles.require('[' not in plain and ']' not in plain, 'Unknown profile markup: '+key)
        result.update((cp, label['pixels'], label['face']) for cp in visible_alphabet(plain))
    return result


def intent_requirements(source):
    result = {(ord(c), 5, 5) for c in '0123456789'}
    result.update((ord(c), 4, 5) for c in '0123456789x×')
    for key, slot in text_index().items():
        if not key.startswith('intents.') or not key.endswith(('.title', '.description')):
            continue
        text = source.string_payload(slot)[:-1].decode('utf-8')
        # Both original branches contribute their visible alphabet. Placeholder
        # names, plural selectors and BBCode are never rendered as glyphs.
        text = re.sub(r'\{(?:Damage|Repeat|CardCount|IsMultiplayer)(?::choose\([^)]*\))?(?::|(?=\}))', '', text)
        plain = RICH_TAG.sub('', text).translate(str.maketrans('', '', '{}|'))
        tiles.require('[' not in plain and ']' not in plain, 'Unknown intent markup: '+key)
        face = 5 if key.endswith('.title') else 0
        result.update((cp, 5, face) for cp in visible_alphabet(plain))
    result.update((ord(c), 5, 0) for c in '0123456789')
    return result


def requirements(source):
    return profile_requirements(source) | intent_requirements(source)


def patch(data, regular, bold):
    source = tiles.PackView(data, AUXILIARY, versions=(34, 35), profile=PROFILE)
    language = source.string_payload(split_content.MARKER)[:-1].decode('ascii')
    required = requirements(source)
    known = {(row[0], row[2]) for row in source.glyphs}
    missing = sorted(row for row in required if (row[0], library.encoded_band(row[1], row[2])) not in known)
    additions = []
    for cp, px, face in missing:
        font = bold if face == 5 else regular
        tiles.require(cp in font.coverage, 'Original scene font lacks codepoint '+str(cp))
        additions.append(font.glyph(cp, px, face))
    out = library.rewrite_directory(data, AUXILIARY, set(), additions,
                                    profile=PROFILE, output_version=VERSION)
    proof = library.verify_preserved(data, out, AUXILIARY, set())
    target = tiles.PackView(out, AUXILIARY, versions=(VERSION,), profile=PROFILE)
    keys = {(row[0], row[2]) for row in target.glyphs}
    tiles.require(all((cp, library.encoded_band(px, face)) in keys for cp, px, face in required),
                  'An exact scene glyph is missing')
    layout = split_content.layout(out, language)
    tiles.require(layout['prefix'] <= split_content.DIRECTORY_LIMIT,
                  'Scene fonts exceed the unchanged directory pool')
    return out, dict(schema=1, reference='v0.111.0/41cef1ea', version=VERSION,
        language=language, input_bytes=len(data), output_bytes=len(out),
        input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(out),
        input_glyphs=len(source.glyphs), glyphs=len(target.glyphs),
        added_glyphs=[dict(codepoint=cp, pixels=px, face=face) for cp, px, face in missing],
        missing_required_glyphs=0, retired_glyphs=0, directory_bytes=layout['prefix'],
        directory_capacity=split_content.DIRECTORY_LIMIT, resident_pool_growth_bytes=0,
        fonts=dict(regular=regular.metadata(), bold=bold.metadata()), verification=proof)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('input-eng', 'input-zhs', 'font-dir', 'output-dir'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    fonts = tiles.private_path(args.font_dir, 'Original font directory')
    output = tiles.private_path(args.output_dir, 'Private scene-font output')
    reports = {}
    for language, names in (('eng', ('kreon_regular.ttf', 'kreon_bold.ttf')),
                            ('zhs', ('NotoSansMonoCJKsc-Regular.otf', 'SourceHanSerifSC-Bold.otf'))):
        path = tiles.private_path(getattr(args, 'input_'+language), 'Immutable input pack')
        target = output/('ui9-'+language+'.nsp')
        tiles.require(path.resolve() != target.resolve(), 'Preserve the input pack')
        out, reports[language] = patch(path.read_bytes(), *(Face(fonts/name) for name in names))
        tiles.atomic_bytes(target, out)
    tiles.require(reports['eng']['verification']['common_art_sha256'] ==
                  reports['zhs']['verification']['common_art_sha256'], 'Locale artwork differs')
    tiles.atomic_bytes(output/'scene-fonts.json', json.dumps(reports, indent=2).encode()+b'\n')
    print(json.dumps(reports), flush=True)


if __name__ == '__main__':
    main()
