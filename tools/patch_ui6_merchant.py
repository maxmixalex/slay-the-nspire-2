#!/usr/bin/env python3
"""Correct rotated merchant atlas regions in a private NSP2/23 resource pack.

Only the merchant backdrop and eight Spine clips are candidates. The two
candle clips use direct PNG/material samples, so they are not rebuilt. Stable
image, clip and auxiliary ids are retained; identical decoded candidates are
skipped. Shared historical retained groups are never modified in place.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ProcessPoolExecutor
import json
from pathlib import Path
import struct

import numpy as np

from animation import Skeleton, geometry, load_textures, pose, raster
from build_content import texture
import content_tiles as tiles
from patch_feedback_assets import (ACTORS, ROOM_SCALE, ROOM_X, SOURCE_PCK_SHA256,
                                   actor_images, source_checks, write_image)
from patch_merchant_effects import focus_frames
from pck import Pack
from repack_retained import DESCRIPTOR, DEFAULT_GROUP_BYTES, plan_images

VERSION, AUXILIARY, CLIPS = 23, 1320, 1705
BACKDROP_AUX = 1261
FIRST_CLIP, LAST_CLIP = 1695, 1702
SOURCE_FILES = ('animation.py', 'setup_pose.py', 'patch_feedback_assets.py',
                'patch_merchant_effects.py', 'patch_ui6_merchant.py',
                'repack_retained.py', 'sparse_image.py')


def converter_identity():
    return tiles.sha256(b''.join((Path(__file__).parent / name).read_bytes()
                                 for name in SOURCE_FILES))


def check_cache(manifest, cache):
    tiles.require(manifest.get('schema') == 1 and len(manifest.get('clips', [])) == 8 and
                  len(manifest.get('images', [])) == 1518, 'Incompatible corrected merchant cache')
    for row in manifest['images']:
        raw = (cache / row['file']).read_bytes()
        tiles.require(len(raw) == row['bytes'] and tiles.sha256(raw) == row['sha256'],
                      'Damaged corrected merchant sample: ' + row['file'])


def build_art(pck_path, cache, jobs=1):
    cache.mkdir(parents=True, exist_ok=True)
    manifest_path = cache / 'ui6-merchant.json'
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text())
        if manifest.get('converter_sha256') == converter_identity():
            check_cache(manifest, cache)
            return manifest
    pack = Pack(pck_path)
    try:
        source_checks(pack)
        skeleton = Skeleton(pack.read(pack.imported('shop_merchant_bottom.skel', '.spskel')))
        textures = load_textures(pack, 'shop_merchant_bottom', texture)
        draw = geometry(skeleton, *pose(skeleton, 'idle_loop', 0), textures,
                        tuple(name for name in textures if name != 'bg'))
        image = raster(draw, .505 * ROOM_SCALE,
                       np.array([ROOM_X - 10 * ROOM_SCALE, 20 * ROOM_SCALE]), (320, 218))
        image.save(cache / 'merchant-backdrop.png')
        backdrop = write_image(cache, 'merchant-backdrop', image)
        backdrop.update(key='merchant-backdrop', auxiliary=BACKDROP_AUX)
    finally:
        pack.close()
    tasks = [(str(pck_path), str(cache), spec) for spec in ACTORS]
    if jobs > 1:
        with ProcessPoolExecutor(max_workers=min(len(tasks), jobs)) as pool:
            actors = list(pool.map(actor_images, tasks))
    else:
        actors = [actor_images(task) for task in tasks]
    pack = Pack(pck_path)
    try:
        focused = focus_frames(pack, cache)
    finally:
        pack.close()
    focused['frames'] = focused['count']
    actors.append(focused)
    rows, clips = [backdrop], []
    for index, actor in enumerate(actors):
        clip = FIRST_CLIP + index
        clips.append(dict(clip=clip, key=actor['key'], count=actor['frames'],
                          frame_ms=actor['frame_ms'], pivot=actor['pivot']))
        for frame, source in enumerate(actor['images']):
            row = dict(source)
            row.update(key=actor['key'] + '-' + str(frame), clip=clip, frame=frame)
            rows.append(row)
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea',
                    reference_sha256=SOURCE_PCK_SHA256, converter_sha256=converter_identity(),
                    images=rows, clips=clips, untouched_material_clips=[1703, 1704],
                    correction='Inverse packed atlas rotation; no bone, scene, scale, frame or runtime reflection changes',
                    primary_uv_source='https://github.com/EsotericSoftware/spine-runtimes/blob/4.2/spine-ts/spine-core/src/attachments/MeshAttachment.ts',
                    primary_uv_blob_sha='7ad9104836e154f46a17f9fc6b6a8c6a03fb671c')
    check_cache(manifest, cache)
    tiles.atomic_bytes(manifest_path, json.dumps(manifest, indent=2).encode() + b'\n')
    return manifest


class ReplacementImages:
    """Small host-only view for the existing bounded retained-group encoder."""
    version = VERSION
    image_raw = tiles.PackView.image_raw

    def __init__(self, rows, cache):
        self.images, self.cards, self.aux = [], [], []
        self.data = bytearray()
        for index, row in enumerate(rows):
            self.data.extend(bytes(len(self.data) & 1))
            raw = (cache / row['file']).read_bytes()
            tiles.require(len(raw) == row['bytes'] and tiles.sha256(raw) == row['sha256'],
                          'Corrected image changed during relocation')
            self.images.append((len(self.data), row['width'], row['height'], row['format'], len(raw)))
            self.data.extend(raw)
            if row.get('auxiliary') == BACKDROP_AUX:
                # Keep the room backdrop independently decodable, as in v22.
                self.cards.append((index, 0, 0, 0))

    def block(self, offset, length):
        tiles.require(0 <= offset <= len(self.data) - length, 'Correction sample outside payload')
        return self.data[offset:offset + length]


def unreachable_payload(source, replaced):
    """Account separately for unreachable blocks and slices in shared groups."""
    descriptors, groups = {}, {}
    for image_id, (offset, _w, _h, fmt, length) in enumerate(source.images):
        descriptors.setdefault((offset, length), set()).add(image_id)
        if fmt == 7:
            relative, size, raw_at, shape = DESCRIPTOR.unpack_from(source.data, offset)
            group = groups.setdefault((offset + relative, size), {})
            group.setdefault((raw_at, shape & 0xffffff), set()).add(image_id)
    direct = sum(length for (_offset, length), ids in descriptors.items() if ids <= replaced)
    orphan_groups, shared_slices = 0, 0
    for (_offset, size), slices in groups.items():
        ids = set().union(*slices.values())
        if ids <= replaced:
            orphan_groups += size
        else:
            shared_slices += sum(length for (_at, length), owners in slices.items() if owners <= replaced)
    return dict(unreachable_image_payload_or_descriptor_bytes=direct,
                unreachable_retained_group_bytes=orphan_groups,
                unreachable_stored_bytes=direct + orphan_groups,
                unreachable_decoded_slices_in_shared_groups=shared_slices,
                retained_shared_group_bytes_modified=0)


def patch_pack(data, manifest, cache, *, auxiliary=AUXILIARY, version=VERSION):
    source = tiles.PackView(data, auxiliary, versions=(version,), profile=(558, 8192, CLIPS, auxiliary))
    old_tiles = tiles.TileReader(source)
    for clip in manifest['clips']:
        _first, count, milliseconds, x, y, reserved = source.clips[clip['clip']]
        tiles.require((count, milliseconds, x, y, reserved) ==
                      (clip['count'], clip['frame_ms'], *clip['pivot'], 0),
                      'Corrected merchant sampling or geometry differs from stable clip')
    candidates = ReplacementImages(manifest['images'], cache)
    selected, replacements, skipped = [], set(), 0
    for index, row in enumerate(manifest['images']):
        image_id = (source.aux[row['auxiliary']] if 'auxiliary' in row else
                    source.clips[row['clip']][0] + row['frame'])
        tiles.require(image_id not in replacements, 'Duplicate corrected merchant image')
        old = old_tiles.image_raw(image_id) if source.images[image_id][3] == 6 else source.image_raw(image_id)
        new = candidates.image_raw(index)
        tiles.require(old[:2] == new[:2], 'Corrected merchant image changed scene crop')
        tiles.require(tiles.sha256(new[3]) == row['raw_sha256'], 'Corrected merchant raw hash differs')
        if old[:3] == new[:3] and old[3] == new[3]:
            skipped += 1
            continue
        selected.append(dict(row, image_id=image_id))
        replacements.add(image_id)
    images = ReplacementImages(selected, cache)
    batches = plan_images(images, DEFAULT_GROUP_BYTES, fast_images='cards') if selected else []
    art_end = min([row[1] for row in source.glyphs] + [row[0] for row in source.strings])
    tiles.require(art_end % 2 == 0 and all(off + size <= art_end for off, _w, _h, _f, size in source.images),
                  'Historical art crosses locale insertion boundary')
    header = tiles.GLOBAL.unpack_from(data, source.base_end)
    cost_at = header[14]
    tiles.require(header[15] == len(source.images) and cost_at + len(source.images) <= art_end,
                  'Missing bounded image decoded-cost table')
    out = bytearray(data[:art_end])
    grouped, group_count, maximum_raw = 0, 0, 0

    def append(raw):
        out.extend(bytes(len(out) & 1))
        offset = len(out)
        out.extend(raw)
        return offset

    def picture(record, offset, fmt, length):
        for index in record.ids:
            row = selected[index]
            tiles.IMAGE.pack_into(out, 40 + row['image_id'] * tiles.IMAGE.size,
                                 offset, row['width'], row['height'], fmt, length)
            pages = (len(record.raw) + 4095) // 4096
            tiles.require(0 < pages <= 255, 'Corrected merchant decoded image exceeds bound')
            out[cost_at + row['image_id']] = pages

    for batch in batches:
        if batch.encoded is None:
            for record in batch.records:
                picture(record, append(record.payload), record.stored_format, len(record.payload))
            continue
        group = tiles.GROUP.pack(b'RGR1', batch.raw_size, len(batch.encoded), 14,
                                 len(batch.records), 0, 0, 0) + batch.encoded
        group_at, raw_at = append(group), 0
        for record in batch.records:
            out.extend(bytes(len(out) & 1))
            descriptor_at = len(out)
            out.extend(DESCRIPTOR.pack(group_at - descriptor_at, len(group), raw_at,
                                       (record.fmt << 24) | len(record.raw)))
            picture(record, descriptor_at, 7, DESCRIPTOR.size)
            raw_at += len(record.raw)
            grouped += len(record.ids)
        group_count += 1
        maximum_raw = max(maximum_raw, batch.raw_size)
    out.extend(bytes(len(out) & 1))
    inserted = len(out) - art_end
    out.extend(data[art_end:])
    for i, (cp, offset, px, w, h, advance, encoded) in enumerate(source.glyphs):
        tiles.GLYPH.pack_into(out, source.glyph_at + i * tiles.GLYPH.size,
                             cp, offset + inserted, px, w, h, advance, encoded)
    for i, (offset, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at + i * 8, offset + inserted, length)
    tiles.require(len(out) <= tiles.FILE_LIMIT, 'Corrected merchant pack exceeds file bound')
    struct.pack_into('<I', out, 12, len(out))
    struct.pack_into('<I', out, 36, 0)
    target = tiles.PackView(out, auxiliary, versions=(version,), profile=(558, 8192, CLIPS, auxiliary))
    tiles.TileReader(target)
    tiles.require(target.clips == source.clips and target.cards == source.cards and target.aux == source.aux,
                  'Merchant correction changed stable catalog ids')
    tiles.require(out[art_end + inserted:] == data[art_end:], 'Merchant correction changed localized payload')
    # All old image/tile/group bytes remain untouched, including relative
    # descriptors still owned by an unrelated frame in a shared group.
    protected = cost_at + ((len(source.images) + 1) & ~1)
    tiles.require(out[protected:art_end] == data[protected:art_end], 'Merchant correction modified historical art')
    for image_id, (old, new) in enumerate(zip(source.images, target.images)):
        if image_id not in replacements:
            tiles.require(new == old and out[cost_at + image_id] == data[cost_at + image_id],
                          'Unrelated image or decoded cost changed')
    for row in selected:
        width, height, fmt, raw = target.image_raw(row['image_id'])
        tiles.require((width, height, fmt) == (row['width'], row['height'], row['decoded_format']) and
                      tiles.sha256(raw) == row['raw_sha256'], 'Corrected merchant roundtrip differs')
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea',
                   input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(out),
                   input_bytes=len(data), output_bytes=len(out), candidate_images=len(manifest['images']),
                   replaced_images=len(replacements), identical_images_skipped=skipped,
                   added_common_art_bytes=inserted, appended_images=0, appended_clips=0, appended_auxiliary=0,
                   unchanged_clip_geometry_and_timing=True, unchanged_localized_payload=True,
                   unchanged_unrelated_image_entries=True, runtime_resident_buffer_growth=0,
                   grouped_images=grouped, retained_groups=group_count, max_group_raw_bytes=maximum_raw,
                   verified_replaced_images=len(replacements), **unreachable_payload(source, replacements))
    return bytes(out), receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--input-zhs', type=Path, required=True)
    parser.add_argument('--input-eng', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--cache-dir', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=1)
    args = parser.parse_args()
    pck = tiles.private_path(args.pck, 'Owner PCK')
    cache = tiles.private_path(args.cache_dir, 'Private merchant cache')
    output = tiles.private_path(args.output_dir, 'Private merchant output')
    tiles.require(cache != output, 'Cache and output must differ')
    manifest = build_art(pck, cache, max(1, args.jobs))
    for language, path in (('eng', args.input_eng), ('zhs', args.input_zhs)):
        path = tiles.private_path(path, 'Input UI6 pack')
        destination = output / ('ui6-' + language + '.nsp')
        tiles.require(path != destination, 'Never overwrite the previous verified pack')
        data, receipt = patch_pack(path.read_bytes(), manifest, cache)
        tiles.atomic_bytes(destination, data)
        tiles.atomic_bytes(output / ('ui6-merchant-' + language + '-receipt.json'),
                           json.dumps(receipt, indent=2).encode() + b'\n')
        print(json.dumps(dict(language=language, **receipt)), flush=True)


if __name__ == '__main__':
    main()
