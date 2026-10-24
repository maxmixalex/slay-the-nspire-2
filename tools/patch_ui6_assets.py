#!/usr/bin/env python3
"""Append pinned UI6 artwork to private NSP2/22 packs without re-encoding them.

The shared relocation code preserves every historical image/clip/string index,
the chosen Chinese glyph payload order, and the existing save/runtime codecs.
Original source inputs and generated art remain outside the public repository.
"""
from __future__ import annotations

import argparse
from collections import OrderedDict
from io import BytesIO
import json
from pathlib import Path
import re

import numpy as np
from PIL import Image

import content_tiles as tiles
from card_banner import material_tint
from card_remove_assets import build_images as build_card_remove_images
from patch_feedback_assets import SOURCE_PCK_SHA256, write_image
from patch_frontend_assets import patch_pack as extend_pack
from pck import Pack
from world_scene_assets import load_scene_texture

VERSION, BASE_AUX, AUXILIARY, CLIPS = 23, 1275, 1320, 1705
TEXTS = ((8105, 'vfx.POWER_WEARS_OFF'),)
SPECS = (
    ('close-x', 'images/atlases/compressed.sprites/back_button_x.tres', (20, 20)),
    ('target-head', 'images/ui/combat/targeting_arrow_head.png', (36, 40)),
    ('target-segment', 'images/ui/combat/targeting_arrow_segment.png', (32, 32)),
) + tuple(('cursor-' + name, 'images/packed/common_ui/cursor_' + name + '.png', (16, 16))
          for name in ('default', 'tilted', 'inspect')) + tuple(
    ('sort-' + color, 'images/ui/color_tab_bar.png', (256, 12))
    for color in ('red', 'green', 'blue', 'orange', 'pink')) + (
    ('sort-descending', 'images/atlases/ui_atlas.sprites/sort_descending.tres', (12, 9)),
    ('scrollbar-track', 'images/atlases/ui_atlas.sprites/scrollbar_track_center.tres', (5, 12)),
    ('scrollbar-grabber', 'images/atlases/ui_atlas.sprites/scrollbar_train_large.tres', (9, 9)),
    ('scrollbar-edge', 'images/atlases/ui_atlas.sprites/scrollbar_track_edge2.tres', (5, 5)),
    ('common-banner', 'images/ui/reward_screen/reward_banner.png', (196, 49)),
)
POPUPS = (
    ('popup-vertical', 'images/atlases/ui_atlas.sprites/popup_vertical.tres', (174, 200), 'scenes/ui/vertical_popup.tscn'),
    ('popup-cancel', 'images/atlases/ui_atlas.sprites/popup_cancel_button.tres', (60, 24), 'scenes/ui/abandon_run_no_button.tscn'),
    ('popup-confirm', 'images/atlases/ui_atlas.sprites/popup_confirm_button.tres', (60, 24), 'scenes/ui/abandon_run_yes_button.tscn'),
    ('popup-cancel-outline', 'images/atlases/compressed.sprites/popup_cancel_button_outline.tres', (60, 24), None),
    ('popup-confirm-outline', 'images/atlases/compressed.sprites/popup_confirm_button_outline.tres', (60, 24), None),
)


def cursor_image(pack, source):
    """Godot's Image importer stores the untouched PNG after a GDIM header."""
    mapping = pack.read(source + '.import').decode()
    match = re.search(r'path="res://([^"]+\.image)"', mapping)
    tiles.require(match is not None, 'Missing pinned cursor import: ' + source)
    data = pack.read(match[1])
    tiles.require(data[:11] == b'GDIM\x03\0\0\0png' and data[11:19] == b'\x89PNG\r\n\x1a\n',
                  'Unsupported cursor Image encoding')
    return Image.open(BytesIO(data[11:])).convert('RGBA')


def build_art(pck_path, cache):
    cache.mkdir(parents=True, exist_ok=True)
    manifest_path = cache / 'ui6-assets.json'
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text())
        tiles.require(manifest.get('schema') == 1 and manifest.get('output_version') == VERSION and
                      len(manifest.get('images', [])) == AUXILIARY - BASE_AUX, 'Incompatible UI6 cache')
        for row in manifest['images']:
            raw = (cache / row['file']).read_bytes()
            tiles.require(len(raw) == row['bytes'] and tiles.sha256(raw) == row['sha256'], 'Invalid UI6 cache payload')
        return manifest
    pack, textures = Pack(pck_path), OrderedDict()
    try:
        rows = []
        for key, source, size in SPECS:
            original = cursor_image(pack, source) if key.startswith('cursor-') else load_scene_texture(pack, source, cache=textures).convert('RGBA')
            if key.startswith('sort-') and key != 'sort-descending':
                original = material_tint(pack, original, 'materials/cards/frames/card_frame_' + key[5:] + '_mat.tres')
            image = original.resize(size, Image.Resampling.LANCZOS)
            if key in ('scrollbar-track', 'scrollbar-edge'):
                # NScrollbar's authored TrackBody/TrackTop/TrackBot modulate.
                pixels = np.asarray(image, dtype=np.float64)
                image = Image.fromarray(np.rint(pixels * (42/255, 74/255, 82/255, 1)).astype(np.uint8))
            row = write_image(cache, key, image)
            row.update(key=key, source=source, source_size=list(original.size), size=list(size))
            image.save(cache / (key + '.png'))
            rows.append(row)
        removal = build_card_remove_images(pack, cache)
        tiles.require(len(removal) == 24, 'Pinned card removal frame count changed')
        rows.extend(removal)
        for key, source, size, material in POPUPS:
            original = load_scene_texture(pack, source, cache=textures).convert('RGBA')
            image = material_tint(pack, original, material) if material else original
            image = image.resize(size, Image.Resampling.LANCZOS)
            row = write_image(cache, key, image)
            row.update(key=key, source=source, source_size=list(original.size), size=list(size), material=material)
            image.save(cache / (key + '.png'));rows.append(row)
        tiles.require(len(rows) == AUXILIARY - BASE_AUX, 'UI6 image directory mismatch')
        for i, row in enumerate(rows): row['auxiliary'] = BASE_AUX + i
        localized = {}
        for language in ('eng', 'zhs'):
            localized[language] = {}
            for _, qualified in TEXTS:
                table, key = qualified.split('.', 1)
                value = json.loads(pack.read('localization/' + language + '/' + table + '.json')).get(key)
                tiles.require(isinstance(value, str) and value and '\0' not in value, 'Missing UI6 locale entry')
                localized[language][qualified] = value
    finally:
        pack.close()
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea', reference_sha256=SOURCE_PCK_SHA256,
                    output_version=VERSION, auxiliary_count=AUXILIARY, animation_count=CLIPS,
                    images=rows, clips=[], localized_text=localized, runtime_added_pixel_buffers=0)
    tiles.atomic_bytes(manifest_path, json.dumps(manifest, indent=2).encode() + b'\n')
    return manifest


def patch_pack(data, manifest, cache):
    return extend_pack(data, manifest, cache, base_aux=BASE_AUX, auxiliary=AUXILIARY,
                       input_version=22, version=VERSION, texts=TEXTS)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', required=True, type=Path)
    parser.add_argument('--input-zhs', required=True, type=Path)
    parser.add_argument('--input-eng', required=True, type=Path)
    parser.add_argument('--output-dir', required=True, type=Path)
    parser.add_argument('--cache-dir', required=True, type=Path)
    args = parser.parse_args()
    pck = tiles.private_path(args.pck, 'Owner PCK')
    cache = tiles.private_path(args.cache_dir, 'Private UI6 cache')
    output = tiles.private_path(args.output_dir, 'Private UI6 output')
    tiles.require(cache != output, 'Output and cache must differ')
    manifest = build_art(pck, cache)
    for language, path in (('zhs', args.input_zhs), ('eng', args.input_eng)):
        path = tiles.private_path(path, 'Input pack')
        destination = output / ('ui6-' + language + '.nsp')
        tiles.require(path != destination, 'Input and output must differ')
        data, receipt = patch_pack(path.read_bytes(), manifest, cache)
        tiles.atomic_bytes(destination, data)
        tiles.atomic_bytes(output / ('ui6-' + language + '-receipt.json'), json.dumps(receipt, indent=2).encode() + b'\n')
        print(json.dumps(dict(language=language, **receipt)), flush=True)


if __name__ == '__main__':
    main()
