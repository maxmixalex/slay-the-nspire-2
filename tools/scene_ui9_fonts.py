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
import struct
import content_tiles as tiles
import card_library_fonts as library
from replace_original_glyphs import Face, visible_alphabet, styled_alphabet, tooltip_small_bold_alphabet
import split_content

ROOT = Path(__file__).resolve().parents[1]
VERSION = 35
PROFILE = library.PROFILE
AUXILIARY = library.AUXILIARY
RICH_TAG = re.compile(r'\[/?(?:gold|red|blue|green|purple|orange|aqua|white|gray|b|i)\]')


def text_index():
    result = library.library_text_index()
    for name in ('profile_text_index.inc', 'auxiliary_text_index.inc'):
        result.update((key, int(slot)) for key, slot in re.findall(
            r'\{"([^"]+)",\s*(\d+)\}', (ROOT/'src'/name).read_text()))
    result.update({'settings_ui.TAB_GENERAL': 8103, 'settings_ui.TAB_INPUT': 8104})
    return result


def profile_requirements(source):
    index, result = text_index(), set()
    language = source.string_payload(split_content.MARKER)[:-1].decode('ascii')
    config = json.loads((ROOT/'config/profile-ui-fonts.json').read_text())
    for label in config['labels']:
        key = label['key']
        tiles.require(key in index, 'Unbound profile text: '+key)
        text = source.string_payload(index[key])[:-1].decode('utf-8')
        tiles.require(bool(text), 'Empty profile text: '+key)
        substitutions = dict(label.get('substitutions', {}))
        substitutions.update(label.get('substitutions_by_locale', {}).get(language, {}))
        for name, values in substitutions.items():
            text = text.replace('{'+name+'}', values)
        tiles.require('{' not in text and '}' not in text, 'Unknown profile substitution: '+key)
        plain = RICH_TAG.sub('', text)
        tiles.require('[' not in plain and ']' not in plain, 'Unknown profile markup: '+key)
        face = label.get('face_by_locale', label.get('faces', {})).get(language, label['face'])
        sizes = label['pixels'] if isinstance(label['pixels'], list) else [label['pixels']]
        result.update((cp, px, face) for cp in visible_alphabet(plain) for px in sizes)
    # Numeric values and canonical seeds are literal UI output, not localized
    # format strings. In particular a seed can contain brackets or braces.
    for item in config.get('alphabets', []):
        face = item.get('face_by_locale', item.get('faces', {})).get(language, item['face'])
        sizes = item['pixels'] if isinstance(item['pixels'], list) else [item['pixels']]
        result.update((cp, px, face) for cp in visible_alphabet(item['characters']) for px in sizes)
    return result


def intent_requirements(source):
    language = source.string_payload(split_content.MARKER)[:-1].decode('ascii')
    value_face = 5 if language == 'eng' else 0
    result = {(ord(c), 5, value_face) for c in '0123456789'}
    result.update((ord(c), 4, value_face) for c in '0123456789x×')
    for key, slot in text_index().items():
        if not key.startswith('intents.') or not key.endswith(('.title', '.description')):
            continue
        text = source.string_payload(slot)[:-1].decode('utf-8')
        # Both original branches contribute their visible alphabet. Placeholder
        # names, plural selectors and BBCode are never rendered as glyphs.
        text = re.sub(r'\{(?:Damage|Repeat|CardCount|IsMultiplayer)(?::choose\([^)]*\))?(?::|(?=\}))', '', text)
        plain = RICH_TAG.sub('', text).translate(str.maketrans('', '', '{}|'))
        tiles.require('[' not in plain and ']' not in plain, 'Unknown intent markup: '+key)
        face = (5 if language == 'eng' else 0) if key.endswith('.title') else 0
        result.update((cp, 5, face) for cp in visible_alphabet(plain))
    result.update((ord(c), 5, 0) for c in '0123456789')
    return result


def room_requirements(source, filename='room-ui-fonts.json'):
    path = ROOT/'config'/filename
    if not path.exists():
        return set()
    config, index, result = json.loads(path.read_text()), text_index(), set()
    language = source.string_payload(split_content.MARKER)[:-1].decode('ascii')

    def alphabet(text, item):
        substitutions = item.get('substitutions', '')
        if isinstance(substitutions, dict):
            for name, value in substitutions.items():
                text = text.replace('{'+name+'}', value)
            substitutions = ''
        # Keep the text of every formatter branch, including nested rich spans,
        # while excluding the parameter names and common selector prefixes.
        text = re.sub(r'\{[A-Za-z_][A-Za-z0-9_.]*(?::[a-z_]+\([^{}]*?\))?(?::|(?=\}))', '', text)
        text = text.translate(str.maketrans('', '', '{}|'))
        text = re.sub(r'\[/?font_size(?:=[0-9]+)?\]', '', text)
        faces = styled_alphabet([text])
        values = faces[1] if item.get('rich_face') == 'bold' else set().union(*faces)
        return values | visible_alphabet(substitutions)

    def add(item, text):
        sizes = item['pixels'] if isinstance(item['pixels'], list) else [item['pixels']]
        face = item.get('face_by_locale', item.get('faces', {})).get(language, item['face'])
        result.update((cp, px, face) for cp in alphabet(text, item) for px in sizes)

    for item in config.get('labels', []):
        key = item['key']
        tiles.require(key in index, 'Unbound room text: '+key)
        add(item, source.string_payload(index[key])[:-1].decode('utf-8'))
    groups = list(config.get('groups', []))
    # Event prose must also cover every page, not only the initial acceptance
    # seed or its option captions. Explicit bold spans keep their own family.
    for face in ((0, 5) if filename == 'room-ui-fonts.json' else ()):
        item = dict(key_pattern=r'^events\..*\.pages\.(?!.*\.options\.).*\.description$',
                    pixels=5, face=face)
        if face == 5: item['rich_face'] = 'bold'
        groups.append(item)
    for item in groups:
        keys = [key for key in index if re.search(item['key_pattern'], key)]
        tiles.require(bool(keys), 'Empty room text group: '+item['key_pattern'])
        for key in keys:
            add(item, source.string_payload(index[key])[:-1].decode('utf-8'))
    for item in config.get('dynamic_labels', []):
        values = set()
        for family in item['families']:
            if family == 'card_titles':
                slots = {row[1] for row in getattr(source, 'cards', ())}
            else:
                prefixes = {'relic_titles':'relics.', 'potion_titles':'potions.', 'character_titles':'characters.'}
                tiles.require(family in prefixes, 'Unknown dynamic label family')
                slots = {slot for key, slot in index.items() if key.startswith(prefixes[family]) and key.endswith('.title')}
            for slot in slots:
                values.update(alphabet(source.string_payload(slot)[:-1].decode('utf-8'), {}))
        sizes = item['pixels'] if isinstance(item['pixels'], list) else [item['pixels']]
        result.update((cp, px, face) for cp in values for px in sizes for face in item['faces'])
    for item in config.get('alphabets', []):
        characters = item.get('characters_by_locale', {}).get(language, item['characters'])
        sizes = item['pixels'] if isinstance(item['pixels'], list) else [item['pixels']]
        face = item.get('face_by_locale', item.get('faces', {})).get(language, item['face'])
        result.update((cp, px, face) for cp in visible_alphabet(characters) for px in sizes)
    return result


def requirements(source):
    language = source.string_payload(split_content.MARKER)[:-1].decode('ascii')
    library_labels = {(cp, px, 0 if language == 'zhs' and face == 5 else face)
                      for cp, px, face in library.glyph_requirements(source)}
    hover_titles = {(cp, 5, 0) for cp in tooltip_small_bold_alphabet(source)} if language == 'zhs' else set()
    return (profile_requirements(source) | intent_requirements(source) | library_labels |
            room_requirements(source) | room_requirements(source, 'common-ui-fonts.json') |
            room_requirements(source, 'history-ui-fonts.json') |
            room_requirements(source, 'run-setup-ui-fonts.json') | hover_titles)


def patch(data, regular, bold, display=None, italic=None, fallback=None):
    # Later font-only iterations preserve the complete existing artwork and
    # its encoding/version, including RGR2 and the six source UI37 pictures.
    version = struct.unpack_from('<I', data, 4)[0] if len(data) >= 8 else 0
    auxiliary = 2154 if version == 39 else 2068 if version == 38 else 2067 if version == 37 else AUXILIARY
    source = tiles.PackView(data, auxiliary, versions=(34, 35, 36, 37, 38, 39),
                           profile=(*PROFILE[:3], auxiliary))
    output_version = max(VERSION, source.version)
    language = source.string_payload(split_content.MARKER)[:-1].decode('ascii')
    required = requirements(source)
    tiles.require(source.version >= 39 or not any(px == 9 for _cp, px, _face in required),
                  'Append the NSP2/39 artwork catalog before adding nine-pixel scene fonts')
    known = {(row[0], row[2]) for row in source.glyphs}
    missing = sorted(row for row in required if (row[0], library.encoded_band(row[1], row[2])) not in known)
    additions, fallback_glyphs = [], []
    for cp, px, face in missing:
        font = bold if face == 5 else display if face == 4 else italic if face == 6 else regular
        tiles.require(font is not None, 'Original display font is required')
        if cp not in font.coverage and face == 0 and fallback is not None and cp in fallback.coverage:
            # Preserve the established original Noto Mono fallback for native
            # language labels and symbols absent from English Kreon.
            font = fallback
            fallback_glyphs.append(dict(codepoint=cp, pixels=px, face=face))
        tiles.require(cp in font.coverage, 'Original scene font lacks codepoint '+str(cp))
        additions.append(font.glyph(cp, px, face))
    out = library.rewrite_directory(data, auxiliary, set(), additions,
                                    profile=(*PROFILE[:3], auxiliary), output_version=output_version)
    proof = library.verify_preserved(data, out, auxiliary, set())
    target = tiles.PackView(out, auxiliary, versions=(output_version,),
                           profile=(*PROFILE[:3], auxiliary))
    keys = {(row[0], row[2]) for row in target.glyphs}
    tiles.require(all((cp, library.encoded_band(px, face)) in keys for cp, px, face in required),
                  'An exact scene glyph is missing')
    layout = split_content.layout(out, language)
    tiles.require(layout['prefix'] <= split_content.DIRECTORY_LIMIT,
                  'Scene fonts exceed the unchanged directory pool')
    return out, dict(schema=1, reference='v0.111.0/41cef1ea', version=output_version,
        language=language, input_bytes=len(data), output_bytes=len(out),
        input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(out),
        input_glyphs=len(source.glyphs), glyphs=len(target.glyphs),
        added_glyphs=[dict(codepoint=cp, pixels=px, face=face) for cp, px, face in missing],
        missing_required_glyphs=0, retired_glyphs=0, directory_bytes=layout['prefix'],
        directory_capacity=split_content.DIRECTORY_LIMIT, resident_pool_growth_bytes=0,
        fonts=dict(regular=regular.metadata(), bold=bold.metadata(),
                   display=display.metadata() if display else None,
                   italic=italic.metadata() if italic else None,
                   fallback=fallback.metadata() if fallback else None),
        fallback_glyphs=fallback_glyphs, verification=proof)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('input-eng', 'input-zhs', 'font-dir', 'output-dir'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    fonts = tiles.private_path(args.font_dir, 'Original font directory')
    output = tiles.private_path(args.output_dir, 'Private scene-font output')
    reports = {}
    for language, names in (('eng', ('kreon_regular.ttf', 'kreon_bold.ttf', 'spectral_bold.ttf', 'bitter_medium_italic.ttf')),
                            ('zhs', ('NotoSansMonoCJKsc-Regular.otf', 'SourceHanSerifSC-Bold.otf', 'SourceHanSerifSC-Medium.otf', 'SourceHanSerifSC-Medium.otf'))):
        path = tiles.private_path(getattr(args, 'input_'+language), 'Immutable input pack')
        target = output/('ui9-'+language+'.nsp')
        tiles.require(path.resolve() != target.resolve(), 'Preserve the input pack')
        fallback = Face(fonts/'NotoSansMonoCJKsc-Regular.otf') if language == 'eng' else None
        out, reports[language] = patch(path.read_bytes(), *(Face(fonts/name) for name in names), fallback=fallback)
        tiles.atomic_bytes(target, out)
    tiles.require(reports['eng']['verification']['common_art_sha256'] ==
                  reports['zhs']['verification']['common_art_sha256'], 'Locale artwork differs')
    tiles.atomic_bytes(output/'scene-fonts.json', json.dumps(reports, indent=2).encode()+b'\n')
    print(json.dumps(reports), flush=True)


if __name__ == '__main__':
    main()
