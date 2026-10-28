#!/usr/bin/env python3
"""Append the original non-resource card block overlay without rebaking art.

NSP2/32 and /33 both permit this one optional final auxiliary. Existing native
glyph groups and text groups are relocated as complete opaque payloads, so no
pixels, strings, compressed groups, animation frames or old indices change.
"""
import argparse
import json
from pathlib import Path
import struct

from PIL import Image
import content_tiles as tiles
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
from pck import Pack
from world_scene_assets import load_scene_texture

BASE_AUX, AUXILIARY = 2050, 2051
SOURCE = 'images/atlases/ui_atlas.sprites/card/card_unplayable_icon.tres'


def build_image(pack, cache):
    scene = pack.read('scenes/cards/card.tscn').decode()
    for name, inset in (('UnplayableEnergyIcon', 8), ('UnplayableStarIcon', 5)):
        marker = '[node name="'+name+'"'
        tiles.require(marker in scene, 'Pinned card overlay node missing: '+name)
        node = scene[scene.index(marker):].split('\n[node ', 1)[0]
        for value in (f'offset_left = {inset}.0', f'offset_top = {inset}.0',
                      f'offset_right = {inset+48}.0', f'offset_bottom = {inset+48}.0'):
            tiles.require(value in node, 'Pinned card overlay geometry changed: '+name)
    tiles.require(SOURCE in scene, 'Pinned original card overlay texture changed')
    original = load_scene_texture(pack, SOURCE).convert('RGBA')
    tiles.require(original.size == (104, 104), 'Pinned card overlay atlas margin changed')
    #16px retains the largest ordinary inspect projection; hand cache uses11px
    #and the settled300px source card uses the same48px authored child geometry.
    image = original.resize((16, 16), Image.Resampling.LANCZOS)
    row = write_image(cache, 'card-unplayable-original', image)
    row.update(key='card-unplayable-original', auxiliary=BASE_AUX, source=SOURCE,
               source_size=list(original.size), projected_source_size=[48, 48])
    return row


def append(data, row, cache):
    version = struct.unpack_from('<I', data, 4)[0]
    tiles.require(version in (32, 33), 'Card overlay requires a grouped-font pack')
    manifest = dict(images=[row], clips=[], localized_text={'eng': {}, 'zhs': {}})
    result, receipt = patch_pack(data, manifest, cache, base_aux=BASE_AUX, auxiliary=AUXILIARY,
        input_version=version, version=version, texts=(), base_clips=1765, base_strings=8704)
    receipt.update(optional_card_resource_overlay=True, old_group_encodings_preserved=True,
                   runtime_added_pixel_buffers=0)
    return result, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('pck', 'input-eng', 'input-zhs', 'cache-dir', 'output-dir'):
        parser.add_argument('--'+key, required=True, type=Path)
    args = parser.parse_args()
    cache = tiles.private_path(args.cache_dir, 'Card overlay cache')
    cache.mkdir(parents=True, exist_ok=True)
    output = tiles.private_path(args.output_dir, 'Card overlay output')
    pack = Pack(tiles.private_path(args.pck, 'Pinned original PCK'))
    try:
        row = build_image(pack, cache)
    finally:
        pack.close()
    tiles.atomic_bytes(cache/'card-resource-assets.json', json.dumps(row, indent=2).encode()+b'\n')
    receipts = {}
    for language in ('eng', 'zhs'):
        source = tiles.private_path(getattr(args, 'input_'+language), 'Previous private pack')
        target = output/f'ui9-{language}.nsp'
        tiles.require(source.resolve() != target.resolve(), 'Card append must preserve its input')
        data, receipt = append(source.read_bytes(), row, cache)
        tiles.atomic_bytes(target, data)
        receipts[language] = receipt
    tiles.atomic_bytes(output/'card-resource-conversion.json', json.dumps(receipts, indent=2).encode()+b'\n')
    print(json.dumps(dict(auxiliary=AUXILIARY, languages=receipts)), flush=True)


if __name__ == '__main__':
    main()
