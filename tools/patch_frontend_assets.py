#!/usr/bin/env python3
"""Append ten original settings/potion/star UI images to private NSP2/21 packs.

The v22 extension changes directories and adds images, without re-encoding any
historical artwork. Its decoded-cost table grows with the image directory;
relative retained-group descriptors stay byte-identical. Both languages share
the same appended art and retain their complete original localized payloads.
"""
from __future__ import annotations

import argparse
from collections import OrderedDict
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image

import content_tiles as tiles
from patch_feedback_assets import SOURCE_PCK_SHA256, write_image
from pck import Pack
from world_scene_assets import load_scene_texture

VERSION, BASE_AUX, AUXILIARY, CLIPS = 22, 1265, 1275, 1705
SPECS = (
    ('settings-tab', 'images/packed/common_ui/settings_tab_selected.png', (112, 39)),
    ('settings-tab-stroke', 'images/packed/common_ui/settings_tab_stroke.png', (112, 39)),
    ('settings-ticked', 'images/atlases/ui_atlas.sprites/checkbox_ticked.tres', (20, 20)),
    ('settings-unticked', 'images/atlases/ui_atlas.sprites/checkbox_unticked.tres', (20, 20)),
    ('potion-popup', 'images/potions/potion_pop_up/potion_popup.png', (64, 59)),
    ('potion-popup-top', 'images/potions/potion_pop_up/potion_popup_top.png', (60, 32)),
    ('potion-popup-bottom', 'images/potions/potion_pop_up/potion_popup_bottom.png', (60, 23)),
    ('star-counter-base', 'images/ui/combat/energy_star.png', (42, 42)),
    ('star-counter-layer-2', 'images/ui/combat/energy_star_layer_2.png', (42, 42)),
    ('star-counter-layer-3', 'images/ui/combat/energy_star_layer_3.png', (42, 42)),
)


def source_checks(pack):
    expected = {
        'scenes/screens/settings_tab.tscn': (
            'custom_minimum_size = Vector2(256, 90)',
            'modulate = Color(0.3648, 0.9104, 0.96, 0.752941)',
            'shader_parameter/v = 0.9'),
        'scenes/ui/tickbox.tscn': ('scale = Vector2(0.8, 0.8)',),
        'scenes/potions/potion_popup.tscn': (
            'offset_left = -131.0', 'offset_right = 128.0', 'offset_bottom = 239.0'),
        'scenes/combat/energy_counters/star_counter.tscn': (
            'offset_left = 64.0', 'offset_top = -212.0', 'scale = Vector2(0.8, 0.8)',
            'pivot_offset = Vector2(64, 64)', 'theme_override_constants/margin_top = 41'),
    }
    for path, values in expected.items():
        scene = pack.read(path).decode()
        for value in values:
            tiles.require(value in scene, 'Pinned frontend scene changed: ' + path + ' / ' + value)


def build_art(pck_path, cache):
    cache.mkdir(parents=True, exist_ok=True)
    path = cache / 'frontend-assets.json'
    if path.exists():
        manifest = json.loads(path.read_text())
        tiles.require(manifest.get('schema') == 1 and manifest.get('output_version') == VERSION and
                      len(manifest.get('images', [])) == len(SPECS), 'Incompatible frontend cache')
        for row in manifest['images']:
            payload = (cache / row['file']).read_bytes()
            tiles.require(len(payload) == row['bytes'] and tiles.sha256(payload) == row['sha256'],
                          'Invalid cached frontend image: ' + row['file'])
        return manifest
    pack, textures = Pack(pck_path), OrderedDict()
    try:
        source_checks(pack)
        rows = []
        for key, source, size in SPECS:
            original = load_scene_texture(pack, source, cache=textures).convert('RGBA')
            if key in ('settings-ticked', 'settings-unticked'):
                # The original 64x64 visual is scaled 0.8 about its center.
                image = Image.new('RGBA', size)
                image.alpha_composite(original.resize((16, 16), Image.Resampling.LANCZOS), (2, 2))
            else:
                image = original.resize(size, Image.Resampling.LANCZOS)
            if key == 'settings-tab-stroke':
                pixels = np.asarray(image, dtype=np.float64)
                image = Image.fromarray(np.rint(pixels * np.asarray((.3648, .9104, .96, .752941))).astype(np.uint8))
            image.save(cache / (key + '.png'))
            row = write_image(cache, key, image)
            row.update(source=source, source_size=list(original.size), size=list(size),
                       auxiliary=BASE_AUX + len(rows))
            rows.append(row)
    finally:
        pack.close()
    manifest = dict(schema=1, reference='v0.111.0 / 41cef1ea', reference_sha256=SOURCE_PCK_SHA256,
                    output_version=VERSION, auxiliary_count=AUXILIARY, animation_count=CLIPS,
                    images=rows, clips=[], runtime_added_pixel_buffers=0)
    tiles.atomic_bytes(path, json.dumps(manifest, indent=2).encode() + b'\n')
    return manifest


def patch_pack(data, manifest, cache):
    source = tiles.PackView(data, BASE_AUX, versions=(21,), profile=(558, 8192, CLIPS, BASE_AUX))
    old_tiles = tiles.TileReader(source)
    ni, ng, ns, nc, nk, nu = source.counts
    rows = manifest['images']
    count = len(rows)
    tiles.require(count == AUXILIARY - BASE_AUX and ni + count <= 65535, 'Unexpected frontend extension')
    art_end = min([row[1] for row in source.glyphs] + [row[0] for row in source.strings])
    descriptor_end = old_tiles.descriptors + old_tiles.frame_count * tiles.DESCRIPTOR.size
    tiles.require(art_end % 2 == 0 and descriptor_end <= art_end, 'Invalid art/locale boundary')
    header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    old_table, cost_at = header[9], header[14]
    tiles.require(old_tiles.descriptors == cost_at + ((ni + 1) & ~1), 'Unexpected cost-table placement')
    for off, _w, _h, fmt, length in source.images:
        tiles.require(off + length <= art_end, 'Historical image overlaps locale')
        if fmt == 7:
            relative, group_size, _raw_at, _shape = struct.unpack_from('<iIII', data, off)
            tiles.require(descriptor_end <= off + relative <= art_end - group_size,
                          'Historical retained group crosses insertion boundary')
    payload, positions, costs = bytearray(), [], bytearray(data[cost_at:cost_at + ni])
    for row in rows:
        payload.extend(bytes(len(payload) & 1))
        positions.append(len(payload))
        raw = (cache / row['file']).read_bytes()
        tiles.require(len(raw) == row['bytes'] and tiles.sha256(raw) == row['sha256'],
                      'Frontend image changed during relocation')
        payload.extend(raw)
        decoded = struct.unpack_from('<I', raw, 1)[0] if row['format'] == 5 else len(raw)
        pages = (decoded + 4095) // 4096
        tiles.require(0 < pages <= 255, 'Frontend image exceeds decoded-page bound')
        costs.append(pages)
    payload.extend(bytes(len(payload) & 1))
    costs.extend(bytes(len(costs) & 1))
    directory_delta = count * 16 + count * 4
    cost_delta = len(costs) - ((ni + 1) & ~1)

    def shifted(off):
        return off + directory_delta + (cost_delta if off >= old_tiles.descriptors else 0) + (len(payload) if off >= art_end else 0)

    out = bytearray(data[:source.glyph_at])
    out.extend(bytes(count * 16))
    out.extend(data[source.glyph_at:source.base_end])
    out.extend(struct.pack('<' + 'I' * count, *range(ni, ni + count)))
    out.extend(data[source.base_end:cost_at])
    out.extend(costs)
    out.extend(data[old_tiles.descriptors:art_end])
    insertion = len(out)
    out.extend(payload)
    out.extend(data[art_end:])
    for i, (off, w, h, fmt, length) in enumerate(source.images):
        tiles.IMAGE.pack_into(out, 40 + i * 16, shifted(off), w, h, fmt, length)
    for i, row in enumerate(rows):
        tiles.IMAGE.pack_into(out, 40 + (ni + i) * 16, insertion + positions[i],
                             row['width'], row['height'], row['format'], row['bytes'])
    for i, (cp, off, px, w, h, advance, encoded) in enumerate(source.glyphs):
        tiles.GLYPH.pack_into(out, source.glyph_at + count * 16 + i * 16,
                             cp, shifted(off), px, w, h, advance, encoded)
    for i, (off, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at + count * 16 + i * 8, shifted(off), length)
    header[8], header[9], header[14], header[15] = shifted(header[8]), shifted(header[9]), shifted(header[14]), ni + count
    tiles.GLOBAL.pack_into(out, source.base_end + directory_delta, *header)
    for i in range(old_tiles.tile_groups + old_tiles.index_groups):
        off, length = struct.unpack_from('<II', data, old_table + i * 8)
        tiles.require(off + length <= art_end, 'Historical tile group overlaps locale')
        struct.pack_into('<II', out, shifted(old_table) + i * 8, shifted(off), length)
    for i in range(old_tiles.frame_count):
        at = old_tiles.descriptors + i * tiles.DESCRIPTOR.size
        global_at, group, offset, length = tiles.DESCRIPTOR.unpack_from(data, at)
        tiles.require(global_at == source.base_end, 'Invalid historical tile descriptor')
        tiles.DESCRIPTOR.pack_into(out, shifted(at), shifted(global_at), group, offset, length)
    tiles.HEADER.pack_into(out, 0, b'NSP2', VERSION, tiles.REFERENCE, len(out), ni + count, ng, ns, nc, nk, 0)
    target = tiles.PackView(out, AUXILIARY, versions=(VERSION,), profile=(nk, ns, nc, AUXILIARY))
    tiles.TileReader(target)
    tiles.require(target.clips == source.clips and target.cards == source.cards and target.aux[:nu] == source.aux,
                  'Historical catalog changed')
    tiles.require(out[shifted(descriptor_end):insertion] == data[descriptor_end:art_end] and
                  out[insertion + len(payload):] == data[art_end:], 'Historical payload changed')
    tiles.require(out[header[14]:header[14] + ni] == data[cost_at:cost_at + ni], 'Historical decoded costs changed')
    for old, new in zip(source.images, target.images):
        tiles.require(new == (shifted(old[0]), *old[1:]), 'Image relocation differs')
    for old, new in zip(source.glyphs, target.glyphs):
        tiles.require(new == (old[0], shifted(old[1]), *old[2:]), 'Glyph relocation differs')
    for old, new in zip(source.strings, target.strings):
        tiles.require(new == (shifted(old[0]), old[1]), 'String relocation differs')
    for i, row in enumerate(rows):
        decoded = target.image_raw(ni + i)
        tiles.require(tiles.sha256(decoded[3]) == row['raw_sha256'], 'New image roundtrip differs')
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea', input_version=21, output_version=VERSION,
                   input_sha256=tiles.sha256(data), output_sha256=tiles.sha256(out),
                   input_bytes=len(data), output_bytes=len(out), appended_images=count, appended_clips=0,
                   auxiliary_count=AUXILIARY, unchanged_old_payload=True, unchanged_old_directory_indices=True,
                   unchanged_old_decoded_costs=True, common_art_insertion=art_end,
                   added_common_art_bytes=len(payload), common_art_sha256=tiles.sha256(payload),
                   verified_new_images=count, verified_all_offset_relocations=True, runtime_integrity_scan=False)
    return bytes(out), receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', required=True, type=Path)
    parser.add_argument('--input-zhs', required=True, type=Path)
    parser.add_argument('--input-eng', required=True, type=Path)
    parser.add_argument('--output-dir', required=True, type=Path)
    parser.add_argument('--cache-dir', required=True, type=Path)
    args = parser.parse_args()
    pck = tiles.private_path(args.pck, 'Owner PCK')
    cache = tiles.private_path(args.cache_dir, 'Private frontend cache')
    output = tiles.private_path(args.output_dir, 'Private output directory')
    tiles.require(cache != output, 'Output and cache must differ')
    manifest = build_art(pck, cache)
    for language, path in (('zhs', args.input_zhs), ('eng', args.input_eng)):
        path = tiles.private_path(path, 'Input pack')
        destination = output / ('frontend-' + language + '.nsp')
        tiles.require(path != destination, 'Input and output must differ')
        data, receipt = patch_pack(path.read_bytes(), manifest, cache)
        tiles.atomic_bytes(destination, data)
        tiles.atomic_bytes(output / ('frontend-' + language + '-receipt.json'),
                           json.dumps(receipt, indent=2).encode() + b'\n')
        print(json.dumps(dict(language=language, **receipt)), flush=True)


if __name__ == '__main__':
    main()
