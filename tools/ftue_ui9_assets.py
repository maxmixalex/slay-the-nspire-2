#!/usr/bin/env python3
"""Append original solo tutorials without regenerating prior private content.

Only source paths, keys and layout contracts belong in this module. The pinned
PCK, fonts, original text, derived pictures and generated packs stay private.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from PIL import Image

import content_tiles as tiles
from append_ui9_tooltip_glyphs import verify_append
from patch_feedback_assets import SOURCE_PCK_SHA256, write_image
from patch_frontend_assets import patch_pack
from patch_ui7_assets import append_glyphs
from pck import Pack
from replace_original_glyphs import Face, text_format_control, visible_alphabet
from world_scene_assets import load_scene_texture

BASE_AUX, AUXILIARY, VERSION, CLIPS, STRINGS = 2051, 2058, 33, 1765, 8704
TEXT_FIRST = 8347
TEXT_KEYS = (
    'ftues.ASCENSION_SINGLEPLAYER_FTUE_DESCRIPTION',
    'ftues.ASCENSION_SINGLEPLAYER_FTUE_DISCLAIMER',
    'ftues.ASCENSION_SINGLEPLAYER_FTUE_TITLE',
    'ftues.CANNOT_PLAY_CARD_FTUE_DESCRIPTION',
    'ftues.CANNOT_PLAY_CARD_FTUE_TITLE',
    'ftues.CAN_PLAY_CARDS_FTUE_DESCRIPTION',
    'ftues.CAN_PLAY_CARDS_FTUE_TITLE',
    'ftues.COMBAT_BASICS_FTUE_HEADER',
    'ftues.COMBAT_BASICS_FTUE_PAGE_COUNT',
    'ftues.CONFIRM_BUTTON',
    'ftues.MAP_SELECT_DESCRIPTION',
    'ftues.MAP_SELECT_TITLE',
    'ftues.MERCHANT_FTUE_DESCRIPTION',
    'ftues.MERCHANT_FTUE_TITLE',
    'ftues.POTION_FTUE_DESCRIPTION',
    'ftues.POTION_FTUE_TITLE',
    'ftues.POWER_FTUE_DESCRIPTION',
    'ftues.POWER_FTUE_TITLE',
    'ftues.RELIC_FTUE_DESCRIPTION',
    'ftues.RELIC_FTUE_TITLE',
    'ftues.REST_SITE_FTUE_DESCRIPTION',
    'ftues.REST_SITE_FTUE_TITLE',
    'ftues.REWARDS_FTUE_DESCRIPTION',
    'ftues.REWARDS_FTUE_TITLE',
    'ftues.SHUFFLE_FTUE_DESCRIPTION',
    'ftues.SHUFFLE_FTUE_TITLE',
    'ftues.TUTORIAL_FTUE_BODY_1',
    'ftues.TUTORIAL_FTUE_BODY_2',
    'ftues.TUTORIAL_FTUE_BODY_3',
    'main_menu_ui.ENABLE_TUTORIALS.description',
    'main_menu_ui.ENABLE_TUTORIALS.title',
)
TEXTS = tuple(enumerate(TEXT_KEYS, TEXT_FIRST))
SMALL_BODY_KEYS = frozenset('ftues.'+key for key in (
    'CANNOT_PLAY_CARD_FTUE_DESCRIPTION', 'CAN_PLAY_CARDS_FTUE_DESCRIPTION',
    'MAP_SELECT_DESCRIPTION', 'MERCHANT_FTUE_DESCRIPTION',
    'POTION_FTUE_DESCRIPTION', 'POWER_FTUE_DESCRIPTION',
    'RELIC_FTUE_DESCRIPTION', 'REST_SITE_FTUE_DESCRIPTION',
    'REWARDS_FTUE_DESCRIPTION', 'SHUFFLE_FTUE_DESCRIPTION'))
SPECS = (
    ('popup', 'ftue_popup', (579, 257), (146, 65)),
    ('confirm', 'ftue_confirm_button', (195, 63), (56, 18)),
    ('confirm-outline', 'ftue_confirm_button_outline', (195, 63), (56, 18)),
    ('arrow', 'ftue_pointer_arrow', (244, 204), (47, 39)),
    ('combat-0', 'combat_ftue_0', (930, 728), (128, 98)),
    ('combat-1', 'combat_ftue_1', (930, 728), (128, 98)),
    ('combat-2', 'combat_ftue_2', (930, 728), (128, 98)),
)
assert len(TEXTS) == 31 and TEXT_FIRST + len(TEXTS) <= 8400
assert len(SPECS) == AUXILIARY - BASE_AUX


def source_checks(pack):
    checks = {
        'scenes/ftue/combat_rules_ftue.tscn': (
            'custom_minimum_size = Vector2(671, 512)',
            'offset_left = -668.0', 'offset_top = -288.0',
            'offset_right = 3.0', 'offset_bottom = 224.0'),
        'scenes/ftue/ftue_confirm_button.tscn': (
            'custom_minimum_size = Vector2(237, 75)',
            'pivot_offset = Vector2(137, 37)', 'stretch_mode = 5'),
        'scenes/ftue/cannot_play_card_ftue.tscn': (
            'offset_left = -289.0', 'offset_right = 290.0',
            'offset_top = -128.0', 'offset_bottom = 129.0'),
        'shaders/ftue_popup.tres': (
            'shader_parameter/h = 0.0', 'shader_parameter/s = 0.5',
            'shader_parameter/v = 1.2'),
    }
    for path, expected in checks.items():
        scene = pack.read(path).decode()
        for value in expected:
            tiles.require(value in scene, 'Pinned tutorial source changed: '+path+' / '+value)


def build_art(pck_path, cache):
    cache.mkdir(parents=True, exist_ok=True)
    pack = Pack(pck_path)
    try:
        source_checks(pack)
        rows = []
        for key, name, source_size, size in SPECS:
            source = 'images/ftue/'+name+'.png'
            original = load_scene_texture(pack, source).convert('RGBA')
            tiles.require(original.size == source_size, 'Pinned tutorial texture changed: '+source)
            image = original.resize(size, Image.Resampling.LANCZOS)
            image.save(cache/('ftue-'+key+'.png'))
            row = write_image(cache, 'ftue-'+key, image)
            row.update(key=key, source=source, source_size=list(source_size),
                       size=list(size), auxiliary=BASE_AUX+len(rows))
            rows.append(row)
        localized = {}
        for language in ('eng', 'zhs'):
            tables = {name:json.loads(pack.read('localization/'+language+'/'+name+'.json'))
                      for name in ('ftues', 'main_menu_ui')}
            localized[language] = {}
            for _, qualified in TEXTS:
                table, key = qualified.split('.', 1)
                value = tables[table].get(key)
                tiles.require(isinstance(value, str) and value and '\0' not in value,
                              'Missing original tutorial text: '+language+' / '+qualified)
                localized[language][qualified] = value
    finally:
        pack.close()
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea',
                    reference_sha256=SOURCE_PCK_SHA256, output_version=VERSION,
                    images=rows, clips=[], localized_text=localized,
                    runtime_added_pixel_buffers=0)
    tiles.atomic_bytes(cache/'ftue-assets.json', json.dumps(manifest, indent=2).encode()+b'\n')
    return manifest


def append(data, manifest, cache):
    source = tiles.PackView(data, BASE_AUX, versions=(VERSION,),
                           profile=(558, STRINGS, CLIPS, BASE_AUX))
    language = source.string_payload(6110)[:-1].decode('ascii')
    missing = []
    for slot, key in TEXTS:
        previous = source.string_payload(slot)
        expected = manifest['localized_text'][language][key].encode('utf-8')+b'\0'
        tiles.require(previous in (b'\0', expected), 'Tutorial slot already belongs to another string')
        if previous == b'\0':
            missing.append((slot, key))
    result, receipt = patch_pack(data, manifest, cache, base_aux=BASE_AUX,
        auxiliary=AUXILIARY, input_version=VERSION, version=VERSION,
        texts=tuple(missing), base_clips=CLIPS, base_strings=STRINGS)
    target = tiles.PackView(result, AUXILIARY, versions=(VERSION,),
                           profile=(558, STRINGS, CLIPS, AUXILIARY))
    changed = {slot for slot, _ in missing}
    for i in range(STRINGS):
        if i not in changed:
            tiles.require(source.string_payload(i) == target.string_payload(i),
                          'A preexisting tutorial-independent string changed')
    for slot, key in TEXTS:
        tiles.require(target.string_payload(slot) ==
            manifest['localized_text'][language][key].encode('utf-8')+b'\0',
            'Tutorial text readback differs')
    receipt.update(optional_tutorial_catalog=True, original_text_keys=len(TEXTS),
                   old_payloads_and_indices_exact=True, resident_cache_growth_bytes=0)
    return result, receipt


def glyph_requirements(source):
    text = {key:source.string_payload(slot)[:-1].decode('utf-8') for slot, key in TEXTS}
    alphabet = set().union(*(visible_alphabet(value) for value in text.values()))
    # Normal and explicitly authored bold are separate original font faces.
    required = {(cp, 5, face) for cp in alphabet for face in (0, 5)}
    # Standard popups use the source 22px body at 320/1680 scale. This
    # dedicated 4px alphabet does not change the general fitting minimum.
    required.update((cp, 4, face) for key in SMALL_BODY_KEYS
                    for cp in visible_alphabet(text[key]) for face in (0, 5))
    required.update((cp, 6, 5) for cp in visible_alphabet(text[TEXT_KEYS[2]]))
    # The existing original vertical popup uses its larger body/title bands.
    for key in ('main_menu_ui.ENABLE_TUTORIALS.description', 'main_menu_ui.ENABLE_TUTORIALS.title'):
        required.update((cp, px, face) for cp in visible_alphabet(text[key])
                        for px in (6, 8, 10) for face in (0, 5))
    return {(cp, px, face) for cp, px, face in required
            if cp >= 32 and not text_format_control(cp)}


def append_fonts(data, regular, bold):
    source = tiles.PackView(data, AUXILIARY, versions=(VERSION,))
    known = {(row[0], row[2]) for row in source.glyphs}
    rows = []
    for cp, px, style in sorted(glyph_requirements(source)):
        packed = px | ((style & 3) << 6) | ((style & 4) << 3)
        if (cp, packed) in known:
            continue
        face = bold if style == 5 else regular
        tiles.require(cp in face.coverage, 'Original tutorial font lacks codepoint '+str(cp))
        rows.append(face.glyph(cp, px, style))
    out, count = append_glyphs(data, AUXILIARY, CLIPS, rows, version=VERSION)
    target = tiles.PackView(out, AUXILIARY, versions=(VERSION,))
    tiles.require(len(target.glyphs) <= 32768, 'Tutorial font directory exceeds existing bound')
    proof = verify_append(data, out, AUXILIARY, VERSION)
    actual = {(row[0], row[2]) for row in target.glyphs}
    for cp, px, style in glyph_requirements(target):
        packed = px | ((style & 3) << 6) | ((style & 4) << 3)
        tiles.require((cp, packed) in actual, 'Tutorial glyph coverage incomplete')
    return bytes(out), dict(added_glyphs=count, glyphs=len(target.glyphs),
        regular_font=regular.metadata(), bold_font=bold.metadata(),
        original_glyphs_exact=proof, missing_required_glyphs=0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('pck', 'input-eng', 'input-zhs', 'cache-dir', 'output-dir', 'font-dir'):
        parser.add_argument('--'+name, required=True, type=Path)
    args = parser.parse_args()
    cache = tiles.private_path(args.cache_dir, 'Private tutorial cache')
    output = tiles.private_path(args.output_dir, 'Private tutorial output')
    fonts = tiles.private_path(args.font_dir, 'Original font directory')
    manifest = build_art(tiles.private_path(args.pck, 'Pinned PCK'), cache)
    reports = {}
    for language, names in (('eng', ('kreon_regular.ttf', 'kreon_bold.ttf')),
                            ('zhs', ('NotoSansMonoCJKsc-Regular.otf', 'SourceHanSerifSC-Bold.otf'))):
        path = tiles.private_path(getattr(args, 'input_'+language), 'Previous private content')
        destination = output/('ui9-'+language+'.nsp')
        tiles.require(path.resolve() != destination.resolve(), 'Preserve the previous pack')
        data, receipt = append(path.read_bytes(), manifest, cache)
        out, font_receipt = append_fonts(data, Face(fonts/names[0]), Face(fonts/names[1]))
        receipt.update(fonts=font_receipt, final_bytes=len(out), final_sha256=tiles.sha256(out))
        tiles.atomic_bytes(destination, out)
        reports[language] = receipt
    tiles.atomic_bytes(output/'ftue-conversion.json', json.dumps(reports, indent=2).encode()+b'\n')
    print(json.dumps(reports), flush=True)


if __name__ == '__main__':
    main()
