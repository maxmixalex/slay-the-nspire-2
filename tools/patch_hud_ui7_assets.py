#!/usr/bin/env python3
"""Append the pinned top bar's room/boss artwork to private v26 resource packs.

The public catalog is paths and indices only. Eight Ancient icons reuse their
existing collection images; their outlines and the other room/boss pairs are
new. All historical images, strings, clips and decoded costs retain their ids.
"""
from __future__ import annotations

import argparse
from collections import OrderedDict
import json
from pathlib import Path

from PIL import Image

import content_tiles as tiles
from patch_feedback_assets import SOURCE_PCK_SHA256, write_image
from patch_frontend_assets import patch_pack
from pck import Pack
from world_scene_assets import load_scene_texture

VERSION, BASE_AUX, AUXILIARY, CLIPS, STRINGS = 27, 1889, 1945, 1753, 8704
CATALOG = Path(__file__).resolve().parents[1]/'config/hud-ui7-0.17.json'


def load_catalog(path=CATALOG):
    catalog = json.loads(path.read_text())
    tiles.require(catalog.get('schema') == 1 and catalog.get('input_version') == 26 and
                  catalog.get('output_version') == VERSION and
                  catalog.get('first_auxiliary') == BASE_AUX and
                  catalog.get('auxiliary_count') == AUXILIARY,
                  'Incompatible top-bar catalog')
    rows = catalog['images']
    tiles.require(len(rows) == AUXILIARY-BASE_AUX and
                  [row['auxiliary'] for row in rows] == list(range(BASE_AUX, AUXILIARY)),
                  'Top-bar auxiliary sequence changed')
    tiles.require(len({row['source'] for row in rows}) == len(rows) and
                  all(row['source'].startswith('images/ui/run_history/') and
                      row['source'].endswith('.png') for row in rows),
                  'Top-bar images must use their authored history artwork')
    valid = set(range(BASE_AUX, AUXILIARY)) | set(range(1869, 1877))
    tiles.require(len(catalog['icon_pairs']) == 32 and
                  len({row['key'] for row in catalog['icon_pairs']}) == 32 and
                  all(row['icon'] in valid and row['outline'] in valid
                      for row in catalog['icon_pairs']), 'Invalid top-bar icon pairs')
    return catalog


def source_checks(pack):
    top = pack.read('scenes/ui/top_bar.tscn').decode()
    for expected in ('[node name="RoomIcons"', '[node name="RoomIcon"',
                     'custom_minimum_size = Vector2(44, 44)',
                     'offset_top = -44.0', 'offset_bottom = 44.0',
                     'modulate = Color(0, 0, 0, 0.176471)',
                     '[node name="BossIcon"'):
        tiles.require(expected in top, 'Pinned top-bar geometry changed: '+expected)
    second = pack.read('scenes/ui/top_bar/second_boss_icon.tscn').decode()
    tiles.require('show_behind_parent = true' in second and
                  'custom_minimum_size = Vector2(44, 44)' in second,
                  'Pinned second-boss geometry changed')


def build_art(path, cache):
    catalog = load_catalog()
    cache.mkdir(parents=True, exist_ok=True)
    pack, textures = Pack(path), OrderedDict()
    try:
        source_checks(pack)
        images = []
        for spec in catalog['images']:
            original = load_scene_texture(pack, spec['source'], cache=textures).convert('RGBA')
            picture = original.copy()
            # TextureRect KEEP_ASPECT_CENTERED must not stretch a tall boss
            # or a wide room icon into the same source texture dimensions.
            picture.thumbnail(tuple(spec['max_size']), Image.Resampling.LANCZOS)
            row = write_image(cache, 'hud-'+spec['key'].lower(), picture)
            row.update(key=spec['key'], source=spec['source'], source_size=list(original.size),
                       auxiliary=spec['auxiliary'])
            picture.save(cache/('hud-'+spec['key'].lower()+'.png'))
            images.append(row)
    finally:
        pack.close()
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea',
                    source_pck_sha256=SOURCE_PCK_SHA256, output_version=VERSION,
                    images=images, clips=[], localized_text={'eng': {}, 'zhs': {}},
                    auxiliary_count=AUXILIARY, reused_ancient_icons=8,
                    runtime_added_pixel_buffers=0)
    tiles.atomic_bytes(cache/'hud-ui7-assets.json', json.dumps(manifest, indent=2).encode()+b'\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--input-eng', type=Path, required=True)
    parser.add_argument('--input-zhs', type=Path, required=True)
    parser.add_argument('--cache-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    cache = tiles.private_path(args.cache_dir, 'Private top-bar cache')
    output = tiles.private_path(args.output_dir, 'Private top-bar output')
    tiles.require(cache != output, 'Cache and output must differ')
    manifest = build_art(tiles.private_path(args.pck, 'Owner PCK'), cache)
    for language, path in (('eng', args.input_eng), ('zhs', args.input_zhs)):
        path = tiles.private_path(path, 'Input pack');target = output/('ui7-'+language+'.nsp')
        tiles.require(path != target, 'Do not overwrite verified input')
        data, receipt = patch_pack(path.read_bytes(), manifest, cache, base_aux=BASE_AUX,
                                  auxiliary=AUXILIARY, input_version=26, version=VERSION,
                                  texts=(), base_clips=CLIPS, base_strings=STRINGS)
        tiles.atomic_bytes(target, data)
        tiles.atomic_bytes(output/('hud-'+language+'-receipt.json'),
                           json.dumps(receipt, indent=2).encode()+b'\n')
        print(json.dumps(dict(language=language, **receipt)), flush=True)


if __name__ == '__main__':
    main()
