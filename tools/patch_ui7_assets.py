#!/usr/bin/env python3
"""Append original UI7 room/control/feedback art and missing locale glyphs.

The old common artwork, clip ids and save formats stay unchanged. Source art,
licensed samplers and all encoded output are private caller-supplied inputs.
"""
from __future__ import annotations

import argparse
from collections import OrderedDict
import json
import math
from pathlib import Path
import struct

import numpy as np
from PIL import Image

import content_tiles as tiles
from build_content import choose_font
from localize_content import glyph_subset
from patch_feedback_assets import SOURCE_PCK_SHA256, write_image
from patch_frontend_assets import patch_pack as extend_pack
from pck import Pack
from room_ui7_assets import build_images as room_images
from character_ui7_assets import build_art as character_images
from ui7_profile import TEXTS
from group_ui7_hands import build_payload as group_hands
from world_scene_assets import load_scene_texture

VERSION, BASE_AUX, AUXILIARY, BASE_CLIPS, CLIPS = 24, 1320, 1877, 1705, 1710
SPECS = (
    ('settings-arrow-left', 'images/packed/common_ui/settings_tiny_left_arrow.png', (9, 17)),
    ('settings-arrow-right', 'images/packed/common_ui/settings_tiny_right_arrow.png', (9, 17)),
    ('settings-arrow-down', 'images/packed/common_ui/settings_tiny_left_arrow.png', (6, 6)),
    ('tiny-panel', 'images/ui/tiny_nine_patch.png', (7, 7)),
    ('potion-error', 'images/atlases/ui_atlas.sprites/top_bar/top_bar_char_backdrop.tres', (30, 28)),
    ('block-break-left', 'images/ui/combat/block_break_left.png', (16, 16)),
    ('block-break-right', 'images/ui/combat/block_break_right.png', (16, 16)),
    ('glow-speck', 'images/vfx/common/common_glow_speck.png', (32, 32)),
    ('glow', 'images/vfx/common/common_glow.png', (64, 64)),
    ('speck', 'images/vfx/common/common_speck.png', (16, 16)),
    ('block-ring', 'images/vfx/common/common_ring_polar_b.png', (128, 128)),
    ('hit-streak', 'images/vfx/common/common_outward_streaks.png', (128, 128)),
    ('bestiary-pointer', 'images/packed/common_ui/bestiary_pointer.png', (8, 8)),
    ('bestiary-dialogue', 'images/ui/dialogue_nine_patch.png', (57, 41)),
    ('bestiary-tail', 'images/ui/dialogue_tail.png', (9, 10)),
    ('bestiary-thought-tail', 'images/ui/thought_tail.png', (9, 10)),
)


def authored_tint(image, hsv=None, modulate=None):
    pixels=np.asarray(image.convert('RGBA'),dtype=float)/255
    if hsv:
        transform=np.array([[.2989,.5870,.1140],[.5959,-.2774,-.3216],[.2115,-.5229,.3114]])
        yiq=pixels[:,:,:3]@transform.T;angle=(1-hsv[0])*6.283185
        i,q=yiq[:,:,1].copy(),yiq[:,:,2].copy()
        yiq[:,:,1]=(math.cos(angle)*i-math.sin(angle)*q)*hsv[1]
        yiq[:,:,2]=(math.sin(angle)*i+math.cos(angle)*q)*hsv[1]
        pixels[:,:,:3]=(yiq*hsv[2])@np.linalg.inv(transform).T
    if modulate:pixels*=np.asarray(modulate)
    return Image.fromarray(np.uint8(np.clip(pixels*255,0,255)))


def potion_error_tint(pack, image):
    scene = pack.read('scenes/ui/top_bar.tscn').decode()
    for value in ('shader_parameter/h = 0.463', 'shader_parameter/s = 3.918',
                  'shader_parameter/v = 2.0', '0.552941'):
        tiles.require(value in scene, 'Original potion-error material changed: ' + value)
    # The original hsv shader uses YIQ, exactly as card_banner.material_tint.
    pixels = np.asarray(image.convert('RGBA'), dtype=float) / 255
    transform = np.array([[.2989, .5870, .1140], [.5959, -.2774, -.3216], [.2115, -.5229, .3114]])
    yiq = pixels[:, :, :3] @ transform.T
    angle = (1 - .463) * 6.283185
    i, q = yiq[:, :, 1].copy(), yiq[:, :, 2].copy()
    yiq[:, :, 1] = (math.cos(angle)*i-math.sin(angle)*q)*3.918
    yiq[:, :, 2] = (math.sin(angle)*i+math.cos(angle)*q)*3.918
    pixels[:, :, :3] = (yiq*2) @ np.linalg.inv(transform).T
    # Authored self alpha is applied by the runtime fade, only once.
    return Image.fromarray(np.uint8(np.clip(pixels*255, 0, 255)))


def converter_identity():
    names = ('patch_ui7_assets.py', 'room_ui7_assets.py', 'SpineSampleAdapter.cs',
             'world_enemy_assets.py', 'ui7_profile.py', 'patch_frontend_assets.py','group_ui7_hands.py','repack_retained.py',
             'character_ui7_assets.py','character_static_layers.py','world_scene_assets.py','world_room_shaders.py','animation.py')
    configs=('frontend-ui7-0.17.json','frontend-ui7-text-keys.json','collection-ui7-0.17.json')
    root=Path(__file__).resolve().parents[1]
    return tiles.sha256(b''.join((Path(__file__).parent/name).read_bytes() for name in names)+
                        b''.join((root/'config'/name).read_bytes() for name in configs))


def build_art(pck_path, cache, *, runtime, dotnet=None, hand_cache=None):
    cache.mkdir(parents=True, exist_ok=True)
    saved = cache/'ui7-assets.json'
    identity = converter_identity()
    if saved.is_file():
        manifest = json.loads(saved.read_text())
        if manifest.get('converter_sha256') == identity:
            tiles.require(len(manifest['images']) == 1097 and len(manifest.get('auxiliary_rows', [])) == AUXILIARY-BASE_AUX and
                          len(manifest.get('clips', [])) == CLIPS-BASE_CLIPS, 'Invalid UI7 cache count')
            for row in manifest['images']:
                raw = (cache/row['file']).read_bytes()
                tiles.require(len(raw) == row['bytes'] and tiles.sha256(raw) == row['sha256'], 'Changed UI7 cache image')
            return manifest
    pack, texture_cache = Pack(pck_path), OrderedDict()
    try:
        images = room_images(pack, cache, runtime=runtime, dotnet=dotnet,
                             work_dir=cache/'spine-adapter', hand_cache=hand_cache)
        tiles.require(len(images) == 108, 'Room directory changed')
        for key, path, size in SPECS:
            original = load_scene_texture(pack, path, cache=texture_cache).convert('RGBA')
            image = original
            if key == 'settings-arrow-down': image = image.transpose(Image.Transpose.ROTATE_90)
            if key == 'potion-error': image = potion_error_tint(pack, image)
            image = image.resize(size, Image.Resampling.LANCZOS)
            row = write_image(cache, 'ui7-'+key, image)
            row.update(key=key, source=path, source_size=list(original.size))
            image.save(cache/('ui7-'+key+'.png'));images.append(row)
        catalog=json.loads((Path(__file__).resolve().parents[1]/'config/frontend-ui7-0.17.json').read_text())
        tiles.require(BASE_AUX+len(images)==catalog['first_auxiliary'],'Frontend UI7 asset boundary changed')
        for spec in catalog['images']:
            original=load_scene_texture(pack,spec['path'],cache=texture_cache).convert('RGBA')
            picture=authored_tint(original,spec.get('hsv'),spec.get('modulate')).resize(tuple(spec['size']),Image.Resampling.LANCZOS)
            row=write_image(cache,'ui7-'+spec['key'],picture)
            row.update(key=spec['key'],source=spec['path'],source_size=list(original.size))
            picture.save(cache/('ui7-'+spec['key']+'.png'));images.append(row)
        original=load_scene_texture(pack,'images/ui/fuzzy_nine_patch_char_select.png',cache=texture_cache).convert('RGBA')
        row=write_image(cache,'ui7-character-info-panel',original.resize((28,28),Image.Resampling.LANCZOS))
        row.update(key='character-info-panel',source='images/ui/fuzzy_nine_patch_char_select.png',source_size=list(original.size))
        images.append(row)
        tiles.require(len(images)==180,'UI7 character static prefix changed')
        auxiliary_rows=list(range(len(images)))
        character=character_images(pack,cache,runtime=runtime,dotnet=dotnet)
        prefix=len(images)
        clips=[dict(clip,first=prefix+clip['first']) for clip in character['clips']]
        auxiliary_rows.extend(prefix+index for index in character['auxiliary_rows'])
        images.extend(character['images'])
        collection=json.loads((Path(__file__).resolve().parents[1]/'config/collection-ui7-0.17.json').read_text())
        tiles.require(BASE_AUX+len(auxiliary_rows)==collection['first_auxiliary'],'UI7 collection boundary changed')
        for spec in collection['images']:
            original=load_scene_texture(pack,spec['path'],cache=texture_cache).convert('RGBA')
            picture=original.resize(tuple(spec['size']),Image.Resampling.LANCZOS)
            row=write_image(cache,'ui7-'+spec['key'],picture)
            row.update(key=spec['key'],source=spec['path'],source_size=list(original.size))
            auxiliary_rows.append(len(images));images.append(row)
        tiles.require(len(images)==1097 and len(auxiliary_rows)==AUXILIARY-BASE_AUX,'UI7 image/auxiliary count mismatch')
        for index, image_index in enumerate(auxiliary_rows):images[image_index]['auxiliary']=BASE_AUX+index
        localized = {}
        for language in ('eng', 'zhs'):
            tables, localized[language] = {}, {}
            for _, qualified in TEXTS:
                table, key = qualified.split('.', 1)
                if table not in tables: tables[table] = json.loads(pack.read(f'localization/{language}/{table}.json'))
                value = tables[table].get(key)
                tiles.require(isinstance(value, str) and value and '\0' not in value, 'Missing UI7 original text: '+qualified)
                localized[language][qualified] = value
    finally:
        pack.close()
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea', source_pck_sha256=SOURCE_PCK_SHA256,
                    converter_sha256=identity, output_version=VERSION, images=images, clips=clips, auxiliary_rows=auxiliary_rows,
                    auxiliary_count=AUXILIARY, localized_text=localized, runtime_added_pixel_buffers=0,
                    character_viewport=character['viewport'], character_static_layering=character['static_layering'],
                    character_static_layer_reports=character['static_layer_reports'])
    tiles.atomic_bytes(saved, json.dumps(manifest, indent=2).encode()+b'\n')
    return manifest


def grow_string_directory(data, count, auxiliary, clips, version):
    """Append empty localization slots; all existing payload bytes survive."""
    source=tiles.PackView(data, auxiliary, versions=(version,), profile=(558,8192,clips,auxiliary))
    old=tiles.TileReader(source);ni,ng,ns,nc,nk,_=source.counts
    tiles.require(ns<=count<=16384, 'Invalid extended string directory')
    delta=(count-ns)*8
    if not delta:return data
    at=source.string_at+ns*8
    empty=next(off for off,length in source.strings if not length)
    out=bytearray(data[:at])+bytearray(delta)+data[at:]
    for i,(off,w,h,fmt,length) in enumerate(source.images):tiles.IMAGE.pack_into(out,40+i*16,off+delta,w,h,fmt,length)
    for i,(cp,off,px,w,h,advance,encoded) in enumerate(source.glyphs):tiles.GLYPH.pack_into(out,source.glyph_at+i*16,cp,off+delta,px,w,h,advance,encoded)
    for i in range(count):
        off,length=source.strings[i] if i<ns else (empty,0)
        struct.pack_into('<II',out,source.string_at+i*8,off+delta,length)
    header=list(tiles.GLOBAL.unpack_from(data,source.base_end));table=header[9]
    for i in (8,9,14):header[i]+=delta
    tiles.GLOBAL.pack_into(out,source.base_end+delta,*header)
    for i in range(old.tile_groups+old.index_groups):
        off,length=struct.unpack_from('<II',data,table+i*8)
        struct.pack_into('<II',out,table+delta+i*8,off+delta,length)
    for i in range(old.frame_count):
        where=old.descriptors+i*16;global_at,group,offset,length=tiles.DESCRIPTOR.unpack_from(data,where)
        tiles.DESCRIPTOR.pack_into(out,where+delta,global_at+delta,group,offset,length)
    tiles.HEADER.pack_into(out,0,b'NSP2',version,tiles.REFERENCE,len(out),ni,ng,count,nc,nk,0)
    target=tiles.PackView(out,auxiliary,versions=(version,),profile=(nk,count,nc,auxiliary));tiles.TileReader(target)
    tiles.require(target.clips==source.clips and target.cards==source.cards and target.aux==source.aux,'String extension changed model indices')
    end=old.descriptors+old.frame_count*16
    tiles.require(out[end+delta:]==data[end:],'String extension changed old payload')
    return bytes(out)


def append_glyphs(data, auxiliary, clips, glyphs, *, version, profile=None):
    """Grow the sorted glyph directory, preserving every old payload byte.

    This small relocation does not resample old glyphs or move text indices.
    New glyph bytes follow the locale suffix; common art stays byte-identical.
    """
    if profile is None: profile = (558, 8704, clips, auxiliary)
    source = tiles.PackView(data, auxiliary, versions=(version,), profile=profile)
    old_tiles = tiles.TileReader(source)
    ni, ng, ns, nc, nk, _nu = source.counts
    known = {(r[0], r[2]) for r in source.glyphs}
    added = [g for g in glyphs if (g[0], g[1]) not in known]
    tiles.require(len({(g[0], g[1]) for g in added}) == len(added), 'Duplicate added glyph')
    if not added: return data, 0
    delta = len(added)*tiles.GLYPH.size
    out = bytearray(data[:source.glyph_at])
    out.extend(bytes((ng+len(added))*tiles.GLYPH.size))
    out.extend(data[source.string_at:])
    rows = [(cp, off+delta, px, w, h, advance, encoded) for cp, off, px, w, h, advance, encoded in source.glyphs]
    for cp, px, w, h, advance, encoded, raw in added:
        tiles.require(0 < w <= 40 and 0 < h <= 40 and 0 <= encoded < 0x8000 and len(raw) == (encoded or w*h), 'Invalid added glyph')
        rows.append((cp, len(out), px, w, h, advance, encoded));out.extend(raw)
    rows.sort(key=lambda r: (r[2], r[0]))
    for i, row in enumerate(rows): tiles.GLYPH.pack_into(out, source.glyph_at+i*16, *row)
    for i, (off, w, h, fmt, size) in enumerate(source.images): tiles.IMAGE.pack_into(out, 40+i*16, off+delta, w, h, fmt, size)
    for i, (off, size) in enumerate(source.strings): struct.pack_into('<II', out, source.string_at+delta+i*8, off+delta, size)
    header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    old_table = header[9]
    for index in (8, 9, 14): header[index] += delta
    tiles.GLOBAL.pack_into(out, source.base_end+delta, *header)
    for i in range(old_tiles.tile_groups+old_tiles.index_groups):
        off, size = struct.unpack_from('<II', data, old_table+i*8)
        struct.pack_into('<II', out, old_table+delta+i*8, off+delta, size)
    for i in range(old_tiles.frame_count):
        at = old_tiles.descriptors+i*16;global_at, group, offset, length = tiles.DESCRIPTOR.unpack_from(data, at)
        tiles.DESCRIPTOR.pack_into(out, at+delta, global_at+delta, group, offset, length)
    tiles.HEADER.pack_into(out, 0, b'NSP2', version, tiles.REFERENCE, len(out), ni, len(rows), ns, nc, nk, 0)
    target = tiles.PackView(out, auxiliary, versions=(version,), profile=profile)
    tiles.TileReader(target)
    tiles.require(target.clips == source.clips and target.cards == source.cards and target.aux == source.aux, 'Glyph insertion changed model indices')
    old_end = len(data)+delta
    tiles.require(out[old_tiles.descriptors+old_tiles.frame_count*16+delta:old_end] ==
                  data[old_tiles.descriptors+old_tiles.frame_count*16:], 'Glyph insertion changed old art or locale bytes')
    return out, len(added)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--input-eng', type=Path, required=True)
    parser.add_argument('--input-zhs', type=Path, required=True)
    parser.add_argument('--cache-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--dotnet')
    parser.add_argument('--font', type=Path)
    parser.add_argument('--room-hand-cache', type=Path)
    args = parser.parse_args()
    pck, cache, output = (tiles.private_path(p, name) for p, name in
                          ((args.pck, 'Owner PCK'), (args.cache_dir, 'Private UI7 cache'), (args.output_dir, 'Private UI7 output')))
    tiles.require(cache != output, 'Cache and output must differ')
    manifest = build_art(pck, cache, runtime=args.runtime, dotnet=args.dotnet, hand_cache=args.room_hand_cache)
    grouped=group_hands(manifest['images'],cache,extra_ranges=[(clip['first'],clip['count']) for clip in manifest['clips']])
    font = choose_font(str(args.font) if args.font else None)
    for language, input_path in (('eng', args.input_eng), ('zhs', args.input_zhs)):
        input_path = tiles.private_path(input_path, 'Input pack')
        target = output/('ui7-'+language+'.nsp');tiles.require(input_path != target, 'Do not overwrite verified input')
        input_bytes=input_path.read_bytes()
        expanded=grow_string_directory(input_bytes,8704,BASE_AUX,BASE_CLIPS,23)
        data, receipt = extend_pack(expanded, manifest, cache, base_aux=BASE_AUX,
                                   auxiliary=AUXILIARY, input_version=23, version=VERSION, texts=TEXTS,base_clips=BASE_CLIPS,base_strings=8704,appended_art=grouped)
        texts = list(manifest['localized_text'][language].values())+['中文']
        view = tiles.PackView(data, AUXILIARY, versions=(VERSION,), profile=(558, 8704, CLIPS, AUXILIARY))
        known = {r[0] for r in view.glyphs};missing = ''.join(sorted({ch for text in texts for ch in text if ord(ch) >= 32 and ord(ch) not in known}))
        glyphs = glyph_subset([missing], font, language) if missing else []
        data, added = append_glyphs(data, AUXILIARY, CLIPS, glyphs, version=VERSION)
        receipt.update(input_sha256=tiles.sha256(input_bytes),input_bytes=len(input_bytes),added_string_slots=512,added_glyphs=added, output_bytes=len(data), output_sha256=tiles.sha256(data),
                       character_viewport=manifest['character_viewport'],
                       character_static_layer_reports=manifest['character_static_layer_reports'])
        tiles.atomic_bytes(target, data)
        tiles.atomic_bytes(output/('ui7-'+language+'-receipt.json'), json.dumps(receipt, indent=2).encode()+b'\n')
        print(json.dumps(dict(language=language, **receipt)), flush=True)


if __name__ == '__main__': main()
