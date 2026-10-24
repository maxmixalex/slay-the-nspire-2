#!/usr/bin/env python3
"""Append original tiny-card history layers to private NSP2/27 resources."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import content_tiles as tiles
from history_ui8_assets import BASE_AUX, COUNT, build_images
from run_modes_ui8_assets import ICONS, TEXTS, build_mode_images, build_mode_text
from patch_feedback_assets import SOURCE_PCK_SHA256
from patch_frontend_assets import patch_pack
from pck import Pack
from world_scene_assets import load_scene_texture
from patch_feedback_assets import write_image
from patch_ui6_assets import cursor_image
from PIL import Image
from crystal_ui8_assets import COUNT as CRYSTAL_COUNT, build_crystal_images

MAP_IMAGES = [
    ('map-draw', 'images/packed/map/drawing_quill.png', (16, 16)),
    ('map-draw-glow', 'images/packed/map/drawing_quill_glow.png', (16, 16)),
    ('map-erase', 'images/packed/map/drawing_eraser.png', (16, 16)),
    ('map-erase-glow', 'images/packed/map/drawing_eraser_glow.png', (16, 16)),
    ('map-clear', 'images/packed/map/drawing_clear.png', (16, 16)),
    ('map-clear-glow', 'images/packed/map/drawing_clear_glow.png', (16, 16)),
    ('map-cursor-draw', 'images/packed/common_ui/cursor_quill.png', (16, 16)),
    ('map-cursor-draw-tilted', 'images/packed/common_ui/cursor_quill_tilted.png', (16, 16)),
    ('map-cursor-erase', 'images/packed/common_ui/cursor_eraser.png', (16, 16)),
    ('map-cursor-erase-tilted', 'images/packed/common_ui/cursor_eraser_tilted.png', (16, 16)),
    ('map-draw-trail', 'images/packed/vfx/trail2.png', None),
    ('map-erase-trail', 'images/packed/vfx/trail3.png', None),
]
PEEK_IMAGES = [
    ('combat-peek', 'images/atlases/ui_atlas.sprites/peek_button.tres', (24, 24)),
    ('combat-peek-flash', 'images/packed/common_ui/peek_button_sdf.png', (31, 31)),
    ('ancient-next-arrow', 'images/packed/common_ui/little_arrow.png', (5, 9)),
]
VERSION, AUXILIARY, CLIPS, STRINGS = 28, BASE_AUX + COUNT + len(ICONS) + 1 + len(MAP_IMAGES) + CRYSTAL_COUNT + len(PEEK_IMAGES), 1753, 8704


def build_art(path, cache):
    cache.mkdir(parents=True, exist_ok=True)
    pack = Pack(path)
    try:
        rows = build_images(pack, cache)
        rows.extend(build_mode_images(pack, cache))
        source = "images/orbs/empty_orb.png"
        orb = load_scene_texture(pack, source).convert("RGBA")
        orb = orb.resize((12, 12), Image.Resampling.LANCZOS)
        row = write_image(cache, "empty-orb", orb)
        row.update(key="empty-orb", source=source, auxiliary=BASE_AUX + len(rows))
        rows.append(row)
        for key, source, size in MAP_IMAGES:
            image = cursor_image(pack, source) if key.startswith('map-cursor-') else load_scene_texture(pack, source).convert('RGBA')
            if size is not None:
                image = image.resize(size, Image.Resampling.LANCZOS)
            row = write_image(cache, key, image)
            row.update(key=key, source=source, auxiliary=BASE_AUX + len(rows))
            rows.append(row)
        rows.extend(build_crystal_images(pack, cache, BASE_AUX + len(rows)))
        for key, source, size in PEEK_IMAGES:
            image = load_scene_texture(pack, source).convert('RGBA').resize(size, Image.Resampling.LANCZOS)
            row = write_image(cache, key, image)
            row.update(key=key, source=source, auxiliary=BASE_AUX + len(rows))
            rows.append(row)
        localized = build_mode_text(pack)
    finally:
        pack.close()
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea',
                    source_pck_sha256=SOURCE_PCK_SHA256, output_version=VERSION,
                    images=rows, clips=[], localized_text=localized,
                    auxiliary_count=AUXILIARY, runtime_added_pixel_buffers=0)
    tiles.atomic_bytes(cache/'history-ui8-assets.json', json.dumps(manifest, indent=2).encode()+b'\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--input-eng', type=Path, required=True)
    parser.add_argument('--input-zhs', type=Path, required=True)
    parser.add_argument('--cache-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    cache = tiles.private_path(args.cache_dir, 'Private history cache')
    output = tiles.private_path(args.output_dir, 'Private history output')
    tiles.require(cache != output, 'Cache and output must differ')
    manifest = build_art(tiles.private_path(args.pck, 'Owner PCK'), cache)
    for language, path in (('eng', args.input_eng), ('zhs', args.input_zhs)):
        path = tiles.private_path(path, 'Input pack'); target = output/('ui8-'+language+'.nsp')
        tiles.require(path != target, 'Do not overwrite verified input')
        data, receipt = patch_pack(path.read_bytes(), manifest, cache, base_aux=BASE_AUX,
                                  auxiliary=AUXILIARY, input_version=27, version=VERSION,
                                  texts=TEXTS, base_clips=CLIPS, base_strings=STRINGS)
        tiles.atomic_bytes(target, data)
        tiles.atomic_bytes(output/('history-'+language+'-receipt.json'),
                           json.dumps(receipt, indent=2).encode()+b'\n')
        print(json.dumps(dict(language=language, **receipt)), flush=True)


if __name__ == '__main__':
    main()
