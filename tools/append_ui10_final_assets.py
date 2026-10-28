#!/usr/bin/env python3
"""Append exact History and Settings artwork once, preserving existing payloads."""
import argparse
import json
from pathlib import Path

import content_tiles as tiles
import history_body_ui10_assets as history
import settings_ui10_assets as settings
from patch_frontend_assets import patch_pack
from pck import Pack

VERSION, BASE_AUX, AUXILIARY, CLIPS, STRINGS = 39, 2068, 2154, 1783, 8704


def build_images(pack, cache):
    rows = history.build_images(pack, cache, BASE_AUX)
    tiles.require(len(rows) == history.AUXILIARY-BASE_AUX, 'History catalog count differs')
    rows.extend(settings.build_images(pack, cache, history.AUXILIARY))
    tiles.require(len(rows) == AUXILIARY-BASE_AUX, 'Combined UI39 catalog count differs')
    tiles.require([row['auxiliary'] for row in rows] == list(range(BASE_AUX, AUXILIARY)),
                  'Combined UI39 image order differs')
    return rows


def append(data, rows, cache):
    manifest = dict(images=rows, clips=[], localized_text={'eng': {}, 'zhs': {}})
    result, proof = patch_pack(data, manifest, cache, base_aux=BASE_AUX, auxiliary=AUXILIARY,
        input_version=38, version=VERSION, texts=(), base_clips=CLIPS, base_strings=STRINGS)
    proof.update(history_original_images=64, settings_original_images=22,
                 existing_artwork_recompressed=False, runtime_added_pixel_buffers=0)
    return result, proof


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('pck', 'input-eng', 'input-zhs', 'cache-dir', 'output-dir'):
        parser.add_argument('--'+key, type=Path, required=True)
    args = parser.parse_args()
    cache = tiles.private_path(args.cache_dir, 'Source UI image cache')
    output = tiles.private_path(args.output_dir, 'Private UI39 output')
    cache.mkdir(parents=True, exist_ok=True)
    pack = Pack(tiles.private_path(args.pck, 'Pinned original PCK'))
    try:
        rows = build_images(pack, cache)
    finally:
        pack.close()
    tiles.atomic_bytes(cache/'ui10-final-artwork.json', json.dumps(rows, indent=2).encode()+b'\n')
    receipts = {}
    for language in ('eng', 'zhs'):
        source = tiles.private_path(getattr(args, 'input_'+language), 'Immutable UI38 source')
        target = output/f'ui10-{language}.nsp'
        tiles.require(source.resolve() != target.resolve(), 'Preserve input resource files')
        result, receipts[language] = append(source.read_bytes(), rows, cache)
        tiles.atomic_bytes(target, result)
    tiles.require(receipts['eng']['common_art_sha256'] == receipts['zhs']['common_art_sha256'],
                  'UI39 artwork differs between locales')
    tiles.atomic_bytes(output/'ui10-final-artwork-conversion.json', json.dumps(receipts, indent=2).encode()+b'\n')
    print(json.dumps(receipts), flush=True)


if __name__ == '__main__':
    main()
