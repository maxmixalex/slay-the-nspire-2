#!/usr/bin/env python3
"""Append the original merchant focus skin and sampled candles to private v18.

NSP2/19 keeps all v18 ids and encoded payloads. The two language inputs receive
the same appended artwork so the existing common/locale split remains exact.
No images are generated from descriptions: all samples use the pinned owner's
Spine attachments, material equations, uniforms and textures.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

import numpy as np

from animation import Skeleton, geometry, load_textures, pose, raster
from build_content import texture
import content_tiles as tiles
from merchant_effects import FLAME_FRAMES, FLAME_FRAME_MS, MerchantFlames
from patch_feedback_assets import ROOM_SCALE, ROOM_X, SOURCE_PCK_SHA256, patch_pack, source_checks, write_image
from pck import Pack

BASE_AUX, BASE_CLIPS = 1262, 1702
ADDED_CLIPS = 3
MAX_ADDED_ART = 2*1024*1024
SOURCE_FILES = ('animation.py', 'merchant_effects.py', 'patch_merchant_effects.py', 'patch_feedback_assets.py')


def converter_identity():
    return tiles.sha256(b''.join((Path(__file__).parent/name).read_bytes() for name in SOURCE_FILES))


def focus_frames(pack, cache):
    skeleton = Skeleton(pack.read(pack.imported('shop_merchant_top.skel', '.spskel')))
    textures = load_textures(pack, 'shop_merchant_top', texture)
    skin = next(i for i, (name, _) in enumerate(skeleton.skins) if name == 'outline')
    tiles.require(len(skeleton.skins[0][1]) == 14 and len(skeleton.skins[skin][1]) == 11,
                  'Unexpected merchant default/outline attachments')
    duration = skeleton.animations['idle_loop']['duration']
    tiles.require(abs(duration-6.) < .00001, 'Changed merchant idle duration')
    count = min(256, max(2, math.ceil(duration*24)))  # Exactly the v18 sampler.
    samples = [geometry(skeleton, *pose(skeleton, 'idle_loop', duration*i/count), textures, skin=skin)
               for i in range(count)]
    scale = .470095*ROOM_SCALE
    origin = np.array([ROOM_X+83.3*ROOM_SCALE, 71.32*ROOM_SCALE])
    pixels = np.concatenate([np.column_stack((vertices[:, 0]*scale+origin[0], origin[1]-vertices[:, 1]*scale))
                             for sample in samples for vertices, *_ in sample])
    lo = np.maximum([0, 0], np.floor(pixels.min(axis=0)).astype(int)-2)
    hi = np.minimum([320, 218], np.ceil(pixels.max(axis=0)).astype(int)+2)
    size = hi-lo
    tiles.require(all(size > 0) and all(size <= [128, 128]), 'Focus crop exceeds static decode budget')
    images = []
    for i, sample in enumerate(samples):
        picture = raster(sample, scale, origin-lo, tuple(size))
        if i in (0, count//3, 2*count//3):
            picture.save(cache/f'merchant-focus-{i:03d}.png')
        images.append(write_image(cache, f'focus-{i:03d}', picture, sparse=True))
    return {'key': 'merchant-focused', 'images': images, 'count': count,
            'frame_ms': round(duration*1000/count), 'pivot': [-int(lo[0]), -int(lo[1])],
            'size': [int(v) for v in size], 'source_duration_ms': 6000,
            'attachment_resolution': 'Selected outline attachment, otherwise default attachment, each with its owning skin deform timeline',
            'focus_method': 'NMerchantButton.RefreshFocus 0x06007d3d; OnUnfocus 0x06007d3a; ready callback 0x0600c31d'}


def flame_frames(pack, cache):
    skeleton = Skeleton(pack.read(pack.imported('shop_merchant_bottom.skel', '.spskel')))
    material = MerchantFlames(pack, skeleton)
    times = [i*FLAME_FRAME_MS/1000 for i in range(FLAME_FRAMES)]
    transforms = [material.transforms(t) for t in times]
    result = []
    for name, selection in (('flames-behind', slice(0, 1)), ('flames-front', slice(1, 4))):
        bounds = material.bounds([m for matrices in transforms for m in matrices[selection]])
        images = []
        for i, (matrices, seconds) in enumerate(zip(transforms, times)):
            picture = material.render(matrices[selection], seconds, bounds)
            if i in (0, 50, 100):
                picture.save(cache/f'{name}-{i:03d}.png')
            images.append(write_image(cache, f'{name}-{i:03d}', picture, sparse=True))
        tiles.require(len({row['raw_sha256'] for row in images}) > 20, 'Flat/empty candle animation')
        result.append({'key': name, 'images': images, 'count': FLAME_FRAMES, 'frame_ms': FLAME_FRAME_MS,
                       'pivot': [-bounds[0], -bounds[1]], 'size': [bounds[2]-bounds[0], bounds[3]-bounds[1]]})
    return result, material.evidence


def check_cache(manifest, cache):
    tiles.require(manifest['schema'] == 1 and manifest['reference_sha256'] == SOURCE_PCK_SHA256 and
                  manifest['converter_sha256'] == converter_identity(), 'Stale merchant effects cache')
    stored = 0
    for row in manifest['images']:
        data = (cache/row['file']).read_bytes()
        tiles.require(len(data) == row['bytes'] and tiles.sha256(data) == row['sha256'], 'Damaged merchant cache')
        stored += len(data)+(len(data)&1)
    tiles.require(stored <= MAX_ADDED_ART and len(manifest['clips']) == ADDED_CLIPS, 'Merchant effects exceed profile budget')


def build_art(pck_path, cache):
    manifest_path = cache/'merchant-effects.json'
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text())
        check_cache(manifest, cache)
        return manifest
    cache.mkdir(parents=True, exist_ok=True)
    pack = Pack(pck_path)
    try:
        source_checks(pack)
        actors = [focus_frames(pack, cache)]
        flames, evidence = flame_frames(pack, cache)
        actors.extend(flames)
    finally:
        pack.close()
    images, clips = [], []
    for actor in actors:
        clips.append({'key': actor['key'], 'first': len(images), 'count': actor['count'],
                      'frame_ms': actor['frame_ms'], 'pivot': actor['pivot']})
        images.extend(actor['images'])
    for row in images:
        payload = (cache/row['file']).read_bytes()
        row['decoded_bytes'] = struct.unpack_from('<I', payload, 1)[0] if row['format'] == 5 else len(payload)
        tiles.require(row['decoded_bytes'] <= 320*240*8+2*240, 'Existing image decoder bound exceeded')
    manifest = {'schema': 1, 'reference': 'v0.111.0 / 41cef1ea', 'reference_sha256': SOURCE_PCK_SHA256,
                'converter_sha256': converter_identity(), 'images': images, 'clips': clips,
                'actors': [{k: v for k, v in actor.items() if k != 'images'} for actor in actors],
                'shader_evidence': evidence, 'stored_art_budget_bytes': MAX_ADDED_ART,
                'stored_art_bytes': sum(row['bytes']+(row['bytes']&1) for row in images),
                'max_decoded_image_bytes': max(row['decoded_bytes'] for row in images),
                'directory_growth_bytes': len(images)*16+len(clips)*16,
                'resident_directory_capacity_unchanged': 2*1024*1024,
                'resident_image_pages_unchanged': 748}
    check_cache(manifest, cache)
    tiles.atomic_bytes(manifest_path, json.dumps(manifest, indent=2).encode()+b'\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--input-zhs', type=Path, required=True)
    parser.add_argument('--input-eng', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--cache-dir', type=Path, required=True)
    args = parser.parse_args()
    pck = tiles.private_path(args.pck, 'Owner PCK')
    cache = tiles.private_path(args.cache_dir, 'Private merchant cache')
    output = tiles.private_path(args.output_dir, 'Private merchant output')
    tiles.require(cache != output, 'Cache and output must differ')
    with pck.open('rb') as source:
        tiles.require(hashlib.file_digest(source, 'sha256').hexdigest() == SOURCE_PCK_SHA256, 'Wrong pinned PCK')
    manifest = build_art(pck, cache)
    for language, path in (('zhs', args.input_zhs), ('eng', args.input_eng)):
        path = tiles.private_path(path, 'Verified v18 input')
        target = output/('merchant-'+language+'.nsp')
        tiles.require(path != target, 'Never overwrite the verified v18 source')
        data, receipt = patch_pack(path.read_bytes(), manifest, cache, base_aux=BASE_AUX, base_clips=BASE_CLIPS,
                                   added_aux=0, expected_added_clips=ADDED_CLIPS, input_version=18, output_version=19)
        view = tiles.PackView(data, BASE_AUX, versions=(19,), profile=(558, 8192, BASE_CLIPS+ADDED_CLIPS, BASE_AUX))
        tiles.require(view.clips[1696][1:3] == view.clips[BASE_CLIPS][1:3], 'Focused and default sampling disagree')
        receipt.update(reference_art=manifest['shader_evidence'], added_art_budget=MAX_ADDED_ART,
                       max_decoded_image_bytes=manifest['max_decoded_image_bytes'],
                       directory_growth_bytes=manifest['directory_growth_bytes'], resident_cache_growth_bytes=0)
        tiles.atomic_bytes(target, data)
        tiles.atomic_bytes(output/('merchant-'+language+'-receipt.json'), json.dumps(receipt, indent=2).encode()+b'\n')
        print(json.dumps({'language': language, **receipt}), flush=True)


if __name__ == '__main__':
    main()
