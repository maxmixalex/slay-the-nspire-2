#!/usr/bin/env python3
"""Append the pinned History custom/daily reminder lock without repacking art."""
import argparse
import json
from pathlib import Path

from PIL import Image
import content_tiles as tiles
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
from pck import Pack
from world_scene_assets import load_scene_texture

VERSION, BASE_AUX, AUXILIARY, CLIPS, STRINGS = 38, 2067, 2068, 1783, 8704
SOURCE = 'images/ui/top_panel/reminder_lock.png'


def build_image(pack, cache):
    original = load_scene_texture(pack, SOURCE).convert('RGBA')
    image = original.resize((6, 6), Image.Resampling.LANCZOS)
    row = write_image(cache, 'history-source-reminder-lock', image)
    row.update(key='history-source-reminder-lock', auxiliary=BASE_AUX,
               source=SOURCE, source_size=list(original.size), source_control_size=[30, 30],
               size=[6, 6], viewport_scale=[4, 21])
    return row


def append(data, row, cache):
    manifest = dict(images=[row], clips=[], localized_text={'eng': {}, 'zhs': {}})
    result, proof = patch_pack(data, manifest, cache, base_aux=BASE_AUX, auxiliary=AUXILIARY,
        input_version=37, version=VERSION, texts=(), base_clips=CLIPS, base_strings=STRINGS)
    proof.update(source_history_reminder=True, existing_artwork_recompressed=False,
                 runtime_added_pixel_buffers=0)
    return result, proof


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for key in ('pck', 'input-eng', 'input-zhs', 'cache-dir', 'output-dir'):
        p.add_argument('--'+key, type=Path, required=True)
    args = p.parse_args()
    cache = tiles.private_path(args.cache_dir, 'History image cache')
    output = tiles.private_path(args.output_dir, 'History resource output')
    cache.mkdir(parents=True, exist_ok=True)
    pack = Pack(tiles.private_path(args.pck, 'Pinned original PCK'))
    try:
        row = build_image(pack, cache)
    finally:
        pack.close()
    tiles.atomic_bytes(cache/'history-header-ui10-assets.json', json.dumps(row, indent=2).encode()+b'\n')
    receipts = {}
    for language in ('eng', 'zhs'):
        source = tiles.private_path(getattr(args, 'input_'+language), 'Previous private pack')
        target = output/f'ui10-{language}.nsp'
        tiles.require(source.resolve() != target.resolve(), 'Preserve the previous private pack')
        result, receipts[language] = append(source.read_bytes(), row, cache)
        tiles.atomic_bytes(target, result)
    tiles.require(receipts['eng']['common_art_sha256'] == receipts['zhs']['common_art_sha256'],
                  'History artwork differs between locales')
    tiles.atomic_bytes(output/'history-header-ui10-conversion.json', json.dumps(receipts, indent=2).encode()+b'\n')
    print(json.dumps(receipts), flush=True)


if __name__ == '__main__':
    main()
