#!/usr/bin/env python3
"""Append pinned room speech and popup images to retained NSP2/36 packs.

NSP2/37 adds six ordinary auxiliary images. The existing append path relocates
all old art and complete grouped-font/text tails without changing their bytes.
The source shader uses YIQ hue rotation; its Blue parameters are distinct from
the merchant rug and crystal-ball speech materials.
"""
import argparse
import json
from pathlib import Path

from PIL import Image
import content_tiles as tiles
from card_banner import material_tint
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
from patch_ui7_assets import authored_tint
from patch_ui6_assets import POPUPS
from pck import Pack
from world_scene_assets import load_scene_texture

VERSION, BASE_AUX, AUXILIARY, CLIPS, STRINGS = 37, 2061, 2067, 1783, 8704
SOURCE = 'images/packed/vfx/speech_bubble3.png'


def build_image(pack, cache):
    scene = pack.read('scenes/vfx/vfx_speech_bubble.tscn').decode()
    for value in (SOURCE, 'position = Vector2(62, -47)', 'scale = Vector2(0.75, 0.75)',
                  'offset_right = 155.0', 'offset_left = -78.0', 'offset_top = -131.0',
                  'offset_right = 202.0', 'offset_bottom = -1.0'):
        tiles.require(value in scene, 'Pinned speech bubble geometry changed: '+value)
    original = load_scene_texture(pack, SOURCE).convert('RGBA')
    tiles.require(original.size == (420, 285), 'Pinned speech bubble image changed')
    # NSpeechBubbleVfx.SetSpeechBubbleColor(Blue), then DialogueSide.Right.
    # No parent rotation is baked: its seven-degree entry is a live tween.
    image = authored_tint(original, hsv=(.05, 1.3, .55)).transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    image = image.resize((60, 41), Image.Resampling.LANCZOS)
    row = write_image(cache, 'fake-merchant-welcome-blue', image)
    row.update(key='fake-merchant-welcome-blue', auxiliary=BASE_AUX, source=SOURCE,
               source_size=list(original.size), hue=.05, saturation=1.3, brightness=.55,
               horizontal_flip=True, source_scale=.75, viewport_scale=[4, 21])
    return row


def build_images(pack, cache):
    rows = [build_image(pack, cache)]
    for index, (key, source, _old_size, material) in enumerate(POPUPS):
        original = load_scene_texture(pack, source).convert('RGBA')
        image = material_tint(pack, original, material) if material else original
        size = (109, 126) if index == 0 else (37, 18)
        image = image.resize(size, Image.Resampling.LANCZOS)
        row = write_image(cache, 'ui10-'+key, image)
        row.update(key=key, auxiliary=BASE_AUX+len(rows), source=source,
                   source_size=list(original.size), size=list(size), material=material)
        rows.append(row)
    tiles.require(len(rows) == AUXILIARY-BASE_AUX, 'Room UI37 image count changed')
    return rows


def append(data, rows, cache):
    tiles.require(len(rows) == AUXILIARY-BASE_AUX, 'Room UI37 requires all six images')
    manifest = dict(images=rows, clips=[], localized_text={'eng': {}, 'zhs': {}})
    result, receipt = patch_pack(data, manifest, cache, base_aux=BASE_AUX, auxiliary=AUXILIARY,
        input_version=36, version=VERSION, texts=(), base_clips=CLIPS, base_strings=STRINGS)
    receipt.update(fake_merchant_welcome=True, source_size_popups=True, retained_filter_version=36,
                   old_group_encodings_preserved=True, runtime_added_pixel_buffers=0)
    return result, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('pck', 'input-eng', 'input-zhs', 'cache-dir', 'output-dir'):
        parser.add_argument('--'+key, required=True, type=Path)
    args = parser.parse_args()
    cache = tiles.private_path(args.cache_dir, 'Speech bubble cache')
    cache.mkdir(parents=True, exist_ok=True)
    output = tiles.private_path(args.output_dir, 'Speech bubble output')
    pack = Pack(tiles.private_path(args.pck, 'Pinned original PCK'))
    try:
        rows = build_images(pack, cache)
    finally:
        pack.close()
    tiles.atomic_bytes(cache/'fake-merchant-ui10-assets.json', json.dumps(rows, indent=2).encode()+b'\n')
    receipts = {}
    for language in ('eng', 'zhs'):
        source = tiles.private_path(getattr(args, 'input_'+language), 'Previous private pack')
        target = output/f'ui10-{language}.nsp'
        tiles.require(source.resolve() != target.resolve(), 'Speech append must preserve its input')
        data, receipt = append(source.read_bytes(), rows, cache)
        tiles.atomic_bytes(target, data)
        receipts[language] = receipt
    tiles.require(receipts['eng']['common_art_sha256'] == receipts['zhs']['common_art_sha256'],
                  'Room UI37 artwork differs between languages')
    tiles.atomic_bytes(output/'fake-merchant-ui10-conversion.json', json.dumps(receipts, indent=2).encode()+b'\n')
    print(json.dumps(dict(auxiliary=AUXILIARY, languages=receipts)), flush=True)


if __name__ == '__main__':
    main()
