#!/usr/bin/env python3
"""Append pinned energy layers and scene-registered merchant animations.

Private NSP2/17 -> NSP2/18. All prior directory indices and compressed bytes
are retained. New art is inserted before the locale payload so Chinese and
English can share one identical contiguous art segment (NSC1/NSL1).
"""
from __future__ import annotations

import argparse
from concurrent.futures import ProcessPoolExecutor
import hashlib
import json
import math
from pathlib import Path
import struct

import numpy as np
from PIL import Image

import content_tiles as tiles
from animation import Skeleton, geometry, load_textures, pose, raster
from build_content import image_data, texture
from lz4_block import encode as lz4_encode
from pck import Pack
from sparse_image import encode as sparse_encode

BASE_AUX = 1236
BASE_CLIPS = 1695
ADDED_AUX = 26
CHARACTERS = ('ironclad', 'silent', 'defect', 'regent', 'necrobinder')
SOURCE_PCK_SHA256 = 'c60f672ee7804e6aefa1e19a582fa1c80b126a7b0eef4d084d3abf110df2eab7'
ROOM_SCALE = 218 / 1080
ROOM_X = (320 - 1920 * ROOM_SCALE) / 2
# The original merchant room's actual parent transforms, not fitted bounds.
# local origin, scale and atlas are all taken from its .tscn/.tres resources.
ACTORS = (
    ('bottom', 'shop_merchant_bottom', 'shop_merchant_bottom', 'idle_loop', (-10., 20.), .505, ('bg',)),
    ('top', 'shop_merchant_top', 'shop_merchant_top', 'idle_loop', (83.3, 71.32), .470095, ()),
) + tuple((name, name, name + '_shop', 'relaxed_loop', (564., 766.),
           .23 if name == 'defect' else .47, ()) for name in CHARACTERS)


def source_checks(pack):
    """Fail closed if the pinned scene transforms or sprite paths change."""
    room = pack.read('scenes/rooms/merchant_room.tscn').decode()
    button = pack.read('scenes/rooms/merchant_button.tscn').decode()
    for value in ('offset_left = -970.0', 'offset_top = -520.0', 'scale = Vector2(1.01, 1.01)',
                  'offset_left = -396.0', 'offset_top = 226.0'):
        tiles.require(value in room, 'Unexpected merchant room transform: ' + value)
    for value in ('offset_left = 246.0', 'offset_top = -72.0',
                  'position = Vector2(-1122.7, -396.68)', 'scale = Vector2(0.470095, 0.470095)'):
        tiles.require(value in button, 'Unexpected merchant body transform: ' + value)
    for name in CHARACTERS:
        scene = pack.read('scenes/merchant/characters/' + name + '_merchant.tscn').decode()
        scale = '0.23' if name == 'defect' else '0.47'
        tiles.require('scale = Vector2(' + scale + ', ' + scale + ')' in scene,
                      'Unexpected merchant character scale: ' + name)
        resource = pack.read('animations/merchant/' + name + '/' + name + '_merchant_skel_data.tres').decode()
        tiles.require(name + '_shop.atlas' in resource and name + '.skel' in resource,
                      'Unexpected merchant character resources: ' + name)


def stored_image(image, sparse=False):
    fmt = 4 if sparse else 1
    raw = sparse_encode(image) if sparse else image_data(image, True)
    compressed = struct.pack('<BI', fmt, len(raw)) + lz4_encode(raw)
    payload, stored = (compressed, 5) if len(compressed) < len(raw) else (raw, fmt)
    return dict(width=image.width, height=image.height, format=stored,
                decoded_format=fmt, raw_sha256=tiles.sha256(raw)), payload


def write_image(cache, key, image, sparse=False):
    row, payload = stored_image(image, sparse)
    row.update(file=key + '.bin', sha256=tiles.sha256(payload), bytes=len(payload))
    tiles.atomic_bytes(cache / row['file'], payload)
    return row


def actor_images(job):
    """One independent worker per original actor; crop never changes scale."""
    pck_path, cache_path, spec = job
    cache = Path(cache_path)
    key, skeleton, atlas, clip, position, scale, excluded = spec
    pack = Pack(pck_path)
    try:
        sk = Skeleton(pack.read(pack.imported(skeleton + '.skel', '.spskel')))
        textures = load_textures(pack, atlas, texture)
        for name in excluded:
            textures.pop(name, None)  # Do not retain the huge room background in every worker.
        tiles.require(clip in sk.animations, skeleton + ' is missing ' + clip)
        duration = sk.animations[clip]['duration']
        count = min(256, max(2, math.ceil(duration * 24)))
        samples = [geometry(sk, *pose(sk, clip, duration * i / count), textures, excluded)
                   for i in range(count)]
        origin = np.array([ROOM_X + position[0] * ROOM_SCALE, position[1] * ROOM_SCALE])
        pixels = np.concatenate([np.column_stack((v[:, 0] * scale * ROOM_SCALE + origin[0],
                                                  origin[1] - v[:, 1] * scale * ROOM_SCALE))
                                 for sample in samples for v, *_ in sample])
        lo = np.maximum([0, 0], np.floor(pixels.min(axis=0)).astype(int) - 2)
        hi = np.minimum([320, 218], np.ceil(pixels.max(axis=0)).astype(int) + 2)
        size = hi - lo
        tiles.require(all(size > 0), 'Merchant actor outside original camera: ' + key)
        rows = []
        for i, sample in enumerate(samples):
            image = raster(sample, scale * ROOM_SCALE, origin - lo, tuple(size))
            if i == 0:
                image.save(cache / (key + '-first.png'))
            rows.append(write_image(cache, f'{key}-{i:03d}', image, sparse=True))
        return dict(key=key, images=rows, frames=count, frame_ms=max(1, round(duration * 1000 / count)),
                    pivot=[-int(lo[0]), -int(lo[1])], size=[int(v) for v in size],
                    skeleton=skeleton, atlas=atlas, animation=clip, original_duration_ms=round(duration * 1000),
                    scene_position=list(position), scene_scale=scale, crop=[int(v) for v in lo])
    finally:
        pack.close()


def build_art(pck_path, cache, jobs):
    manifest_path = cache / 'feedback-assets.json'
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text())
        tiles.require(manifest.get('schema') == 1 and manifest.get('reference_sha256') == SOURCE_PCK_SHA256,
                      'Incompatible feedback cache')
        for row in manifest['images']:
            payload = (cache / row['file']).read_bytes()
            tiles.require(len(payload) == row['bytes'] and tiles.sha256(payload) == row['sha256'],
                          'Invalid cached feedback image: ' + row['file'])
        return manifest
    cache.mkdir(parents=True, exist_ok=True)
    pack = Pack(pck_path)
    try:
        source_checks(pack)
        images = []
        for character in CHARACTERS:
            sequence = (1, 2, 3, 5, 5) if character == 'regent' else (1, 2, 3, 0, 0) if character == 'necrobinder' else (1, 2, 3, 4, 5)
            for layer, number in enumerate(sequence):
                path = f'images/ui/combat/energy_counters/{character}/{character}_orb_layer_{number}.png'
                image = texture(pack, path).resize((42, 42), Image.Resampling.LANCZOS) if number else Image.new('RGBA', (42, 42))
                row = write_image(cache, f'energy-{character}-{layer}', image)
                row.update(source=path if number else None)
                images.append(row)
        sk = Skeleton(pack.read(pack.imported('shop_merchant_bottom.skel', '.spskel')))
        textures = load_textures(pack, 'shop_merchant_bottom', texture)
        # Render the background attachment through its authored transform too.
        draw = geometry(sk, *pose(sk, 'idle_loop', 0), textures,
                        tuple(name for name in textures if name != 'bg'))
        image = raster(draw, .505 * ROOM_SCALE,
                       np.array([ROOM_X - 10 * ROOM_SCALE, 20 * ROOM_SCALE]), (320, 218))
        image.save(cache / 'merchant-backdrop.png')
        images.append(write_image(cache, 'merchant-backdrop', image))
        del sk, textures, draw, image
    finally:
        pack.close()
    tasks = [(str(pck_path), str(cache), spec) for spec in ACTORS]
    if jobs > 1:
        with ProcessPoolExecutor(max_workers=min(7, jobs)) as pool:
            actors = list(pool.map(actor_images, tasks))
    else:
        actors = [actor_images(task) for task in tasks]
    clips = []
    for actor in actors:
        clips.append(dict(first=len(images), count=actor['frames'], frame_ms=actor['frame_ms'],
                          pivot=actor['pivot'], key=actor['key']))
        images.extend(actor['images'])
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea', reference_sha256=SOURCE_PCK_SHA256,
                    auxiliary_count=BASE_AUX + ADDED_AUX, animation_count=BASE_CLIPS + len(clips),
                    images=images, clips=clips, actors=actors,
                    limitations=['Merchant candle GPU particles and Necrobinder flame shaders are not prebaked.'])
    tiles.atomic_bytes(manifest_path, json.dumps(manifest, indent=2).encode() + b'\n')
    return manifest


def patch_pack(data, manifest, cache, *, base_aux=BASE_AUX, base_clips=BASE_CLIPS,
               added_aux=ADDED_AUX, expected_added_clips=7, input_version=17, output_version=18):
    """Append ordinary images/clips without changing any old payload or id.

    Defaults retain the fixed v17 -> v18 energy/merchant conversion. The v19
    merchant converter supplies another exact, known source/target profile.
    """
    source = tiles.PackView(data, base_aux, versions=(input_version,), profile=(558, 8192, base_clips, base_aux))
    old_tiles = tiles.TileReader(source)
    ni, ng, ns, nc, nk, nu = source.counts
    rows, clips = manifest['images'], manifest['clips']
    count, clip_count = len(rows), len(clips)
    tiles.require(count + ni <= 65535 and clip_count == expected_added_clips, 'Feedback profile outside bounds')
    # The source localization places every glyph/string after all art.
    art_end = min([row[1] for row in source.glyphs] + [row[0] for row in source.strings])
    tiles.require(art_end % 2 == 0, 'Locale segment must begin on an even boundary')
    tiles.require(all(off + length <= art_end for off, _w, _h, _fmt, length in source.images),
                  'Input art is interleaved with locale payload')
    payload = bytearray()
    positions = []
    for row in rows:
        payload.extend(bytes(len(payload) % 2))
        positions.append(len(payload))
        raw = (cache / row['file']).read_bytes()
        tiles.require(tiles.sha256(raw) == row['sha256'], 'Feedback image changed during relocation')
        payload.extend(raw)
    payload.extend(bytes(len(payload) % 2))
    delta = count * tiles.IMAGE.size + clip_count * tiles.CLIP.size + added_aux * 4
    def shifted(off):
        return off + delta + (len(payload) if off >= art_end else 0)
    out = bytearray(data[:source.glyph_at])
    out.extend(bytes(count * tiles.IMAGE.size))
    out.extend(data[source.glyph_at:source.card_at])
    out.extend(bytes(clip_count * tiles.CLIP.size))
    out.extend(data[source.card_at:source.base_end])
    out.extend(struct.pack('<' + 'I' * added_aux, *range(ni, ni + added_aux)))
    out.extend(data[source.base_end:art_end])
    out.extend(payload)
    out.extend(data[art_end:])
    for i, (off, w, h, fmt, length) in enumerate(source.images):
        tiles.IMAGE.pack_into(out, 40 + i * 16, shifted(off), w, h, fmt, length)
    for i, row in enumerate(rows):
        tiles.IMAGE.pack_into(out, 40 + (ni + i) * 16, art_end + delta + positions[i],
                             row['width'], row['height'], row['format'], row['bytes'])
    for i, row in enumerate(source.glyphs):
        cp, off, px, w, h, advance, encoded = row
        tiles.GLYPH.pack_into(out, source.glyph_at + count * 16 + i * 16,
                             cp, shifted(off), px, w, h, advance, encoded)
    for i, (off, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at + count * 16 + i * 8, shifted(off), length)
    for i, clip in enumerate(clips):
        tiles.CLIP.pack_into(out, source.card_at + count * 16 + i * 16,
                            ni + clip['first'], clip['count'], clip['frame_ms'], *clip['pivot'], 0)
    header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    old_table = header[9]
    header[8] = shifted(header[8])
    header[9] = shifted(header[9])
    tiles.GLOBAL.pack_into(out, source.base_end + delta, *header)
    for i in range(old_tiles.tile_groups + old_tiles.index_groups):
        off, length = struct.unpack_from('<II', data, old_table + i * 8)
        tiles.require(off + length <= art_end, 'Tile groups overlap locale segment')
        struct.pack_into('<II', out, old_table + delta + i * 8, shifted(off), length)
    for i in range(old_tiles.frame_count):
        at = old_tiles.descriptors + i * tiles.DESCRIPTOR.size
        global_at, group, offset, length = tiles.DESCRIPTOR.unpack_from(data, at)
        tiles.require(global_at == source.base_end, 'Invalid historical tile descriptor')
        tiles.DESCRIPTOR.pack_into(out, at + delta, global_at + delta, group, offset, length)
    tiles.HEADER.pack_into(out, 0, b'NSP2', output_version, tiles.REFERENCE, len(out), ni + count, ng, ns, nc + clip_count, nk,
                          tiles.fnv(memoryview(out)[40:]))
    target = tiles.PackView(out, nu + added_aux, versions=(output_version,), profile=(nk, ns, nc + clip_count, nu + added_aux))
    tiles.TileReader(target)
    tiles.require(target.clips[:nc] == source.clips and target.cards == source.cards and
                  target.aux[:nu] == source.aux, 'Historical catalog changed')
    descriptor_end = old_tiles.descriptors + old_tiles.frame_count * tiles.DESCRIPTOR.size
    tiles.require(out[descriptor_end + delta:art_end + delta] == data[descriptor_end:art_end] and
                  out[art_end + delta + len(payload):] == data[art_end:], 'Retained payload changed')
    for i, row in enumerate(source.images):
        tiles.require(target.images[i] == (shifted(row[0]), *row[1:]), 'Image relocation mismatch')
    for i, row in enumerate(rows):
        _w, _h, _fmt, raw = target.image_raw(ni + i)
        tiles.require(tiles.sha256(raw) == row['raw_sha256'], 'New image roundtrip failed')
    for old, new in zip(source.glyphs, target.glyphs):
        tiles.require(new == (old[0], shifted(old[1]), *old[2:]), 'Glyph relocation mismatch')
    for old, new in zip(source.strings, target.strings):
        tiles.require(new == (shifted(old[0]), old[1]), 'String relocation mismatch')
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea', input_version=input_version, output_version=output_version,
                   input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(out),
                   input_bytes=len(data), output_bytes=len(out), appended_images=count,
                   appended_clips=clip_count, auxiliary_count=nu + added_aux,
                   unchanged_old_payload=True, unchanged_old_directory_indices=True,
                   common_art_insertion=art_end, added_common_art_bytes=len(payload),
                   common_art_sha256=tiles.sha256(payload), verified_new_images=count,
                   verified_all_offset_relocations=True)
    return out, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', required=True, type=Path)
    parser.add_argument('--input-zhs', required=True, type=Path)
    parser.add_argument('--input-eng', required=True, type=Path)
    parser.add_argument('--output-dir', required=True, type=Path)
    parser.add_argument('--cache-dir', required=True, type=Path)
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args()
    pck_path = tiles.private_path(args.pck, 'Owner PCK')
    cache = tiles.private_path(args.cache_dir, 'Private animation cache')
    output = tiles.private_path(args.output_dir, 'Private output directory')
    tiles.require(cache != output, 'Output and cache must differ')
    with pck_path.open('rb') as handle:
        tiles.require(hashlib.file_digest(handle, 'sha256').hexdigest() == SOURCE_PCK_SHA256,
                      'Expected owner reference v0.111.0 / 41cef1ea')
    manifest = build_art(pck_path, cache, args.jobs)
    for language, path in (('zhs', args.input_zhs), ('eng', args.input_eng)):
        path = tiles.private_path(path, 'Input pack')
        destination = output / ('feedback-' + language + '.nsp')
        tiles.require(path != destination, 'Input and output must differ')
        data, receipt = patch_pack(path.read_bytes(), manifest, cache)
        tiles.atomic_bytes(destination, data)
        tiles.atomic_bytes(output / ('feedback-' + language + '-receipt.json'),
                           json.dumps(receipt, indent=2).encode() + b'\n')
        print(json.dumps(dict(language=language, **receipt)), flush=True)


if __name__ == '__main__':
    main()
