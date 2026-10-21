#!/usr/bin/env python3
"""Append pinned UI skins to a private v15 pack; retain all old payload bytes.

Version 16 uses the same bounded tile codec. Only directories relocate: no
animation is resampled, deduplicated again, recompressed, or dropped.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image

import content_tiles as tiles
from native_ui_profile import BASE_AUXILIARY_COUNT, AUXILIARY_COUNT, SKINS
from lz4_block import encode as lz4_encode


def reference_hsv(image, h, s, v):
    """The pinned shader's RGB/YIQ rotation (not an HSV-space hue rotation)."""
    rgba = np.asarray(image.convert('RGBA'), dtype=np.float32).copy()
    matrix = np.array(((.2989, .5870, .1140),
                       (.5959, -.2774, -.3216),
                       (.2115, -.5229, .3114)), dtype=np.float32)
    angle = np.float32((1. - h) * 6.283185)
    c, sn = np.cos(angle), np.sin(angle)
    # GLSL col.rgb *= hue_shift is a row-vector multiplication.
    rotation = np.array(((1, 0, 0), (0, c, sn), (0, -sn, c)), dtype=np.float32)
    yiq = (rgba[:, :, :3] / 255.) @ matrix.T
    yiq = yiq @ rotation
    yiq[:, :, 1:] *= np.float32(s)
    rgb = (yiq * np.float32(v)) @ np.linalg.inv(matrix).T
    rgba[:, :, :3] = np.rint(np.clip(rgb, 0, 1) * 255.)
    return Image.fromarray(rgba.astype(np.uint8), 'RGBA')


def append_skins(data, auxiliary_count, skins, *, profile=None):
    """Pure, bounded relocation used by both the production tool and fixtures.

    Each skin is (width, height, dense_format, bytes). Never accept a nested
    sparse/tile skin: nine-patch sampling requires independent dense planes.
    """
    source = tiles.PackView(data, auxiliary_count, versions=(15,), profile=profile)
    old_tiles = tiles.TileReader(source)
    tiles.require(0 < len(skins) <= 256, 'Invalid appended skin count')
    for w, h, fmt, raw in skins:
        tiles.require(0 < w <= 320 and 0 < h <= 240 and fmt in (0, 1, 2, 3), 'Invalid UI skin')
        tiles.validate_raw(raw, w, h, fmt)
    ni, ng, ns, nc, nk, nu = source.counts
    count, delta = len(skins), len(skins) * 20
    tiles.require(ni + count <= 65535 and nu + count <= 65535, 'Extended catalog exceeds bound')
    out = bytearray(data[:source.glyph_at])
    out.extend(bytes(count * 16))
    out.extend(data[source.glyph_at:source.base_end])
    out.extend(struct.pack('<' + 'I' * count, *range(ni, ni + count)))
    out.extend(data[source.base_end:])
    for i, (off, w, h, fmt, length) in enumerate(source.images):
        tiles.IMAGE.pack_into(out, 40 + i * 16, off + delta, w, h, fmt, length)
    for i, row in enumerate(source.glyphs):
        cp, off, px, w, h, advance, encoded = row
        tiles.GLYPH.pack_into(out, source.glyph_at + count * 16 + i * 16,
                             cp, off + delta, px, w, h, advance, encoded)
    for i, (off, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at + count * 16 + i * 8, off + delta, length)
    header = list(tiles.GLOBAL.unpack_from(data, source.base_end))
    old_table = header[9]
    header[8] += delta  # absolute descriptor-array offset
    header[9] += delta  # absolute group-table offset
    tiles.GLOBAL.pack_into(out, source.base_end + delta, *header)
    for i in range(old_tiles.tile_groups + old_tiles.index_groups):
        off, length = struct.unpack_from('<II', data, old_table + i * 8)
        struct.pack_into('<II', out, old_table + delta + i * 8, off + delta, length)
    for i in range(old_tiles.frame_count):
        at = old_tiles.descriptors + i * tiles.DESCRIPTOR.size
        global_at, group, offset, length = tiles.DESCRIPTOR.unpack_from(data, at)
        tiles.require(global_at == source.base_end, 'Invalid historical descriptor reference')
        tiles.DESCRIPTOR.pack_into(out, at + delta, global_at + delta, group, offset, length)
    for i, (w, h, fmt, raw) in enumerate(skins):
        out.extend(bytes(len(out) % 2))
        off = len(out)
        compressed = struct.pack('<BI', fmt, len(raw)) + lz4_encode(raw)
        stored_fmt, payload = (5, compressed) if len(compressed) < len(raw) else (fmt, raw)
        out.extend(payload)
        tiles.IMAGE.pack_into(out, 40 + (ni + i) * 16, off, w, h, stored_fmt, len(payload))
    tiles.require(len(out) <= tiles.FILE_LIMIT, 'Extended pack exceeds file bound')
    tiles.HEADER.pack_into(out, 0, b'NSP2', 16, tiles.REFERENCE, len(out), ni + count, ng, ns, nc, nk,
                          tiles.fnv(memoryview(out)[40:]))
    target = tiles.PackView(out, nu + count, versions=(16,),
                            profile=(nk, ns, nc, nu + count) if profile else None)
    new_tiles = tiles.TileReader(target)
    tiles.require(target.clips == source.clips and target.cards == source.cards and
                  target.aux[:nu] == source.aux, 'Historical catalog changed')
    # Descriptors contain one absolute header pointer; their other 12 bytes
    # remain exact. Every compressed group, retained image and glyph is copied.
    payload_at = old_tiles.descriptors + old_tiles.frame_count * tiles.DESCRIPTOR.size
    tiles.require(memoryview(out)[payload_at + delta:len(data) + delta] ==
                  memoryview(data)[payload_at:], 'Historical payload changed')
    for i in range(old_tiles.frame_count):
        at = old_tiles.descriptors + i * tiles.DESCRIPTOR.size
        tiles.require(out[at + delta + 4:at + delta + 16] == data[at + 4:at + 16],
                      'Historical descriptor changed')
    for i, row in enumerate(source.images):
        tiles.require(target.images[i] == (row[0] + delta, *row[1:]), 'Image relocation mismatch')
    for i, row in enumerate(source.glyphs):
        tiles.require(target.glyphs[i] == (row[0], row[1] + delta, *row[2:]), 'Glyph relocation mismatch')
    for i, (off, length) in enumerate(source.strings):
        tiles.require(target.strings[i] == (off + delta, length), 'String relocation mismatch')
    for i, skin in enumerate(skins):
        tiles.require(target.image_raw(ni + i) == skin, 'New skin roundtrip mismatch')
    tiles.require((old_tiles.tile_count, old_tiles.frame_count) ==
                  (new_tiles.tile_count, new_tiles.frame_count), 'Tile catalog changed')
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea', input_version=15, output_version=16,
        input_bytes=len(data), output_bytes=len(out), added_bytes=len(out) - len(data),
        input_sha256=hashlib.sha256(data).hexdigest(), output_sha256=hashlib.sha256(out).hexdigest(),
        old_images=ni, appended_images=count, auxiliary_count=nu + count,
        unchanged_compressed_payload=True, unchanged_existing_catalog=True,
        verified_all_relocated_offsets=True, verified_all_new_skin_bytes=True)
    return bytes(out), receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    original = tiles.private_path(args.input, 'Input pack')
    pck_path = tiles.private_path(args.pck, 'Owner PCK')
    output = tiles.private_path(args.output, 'Output pack')
    receipt_path = tiles.private_path(args.receipt, 'Receipt')
    tiles.require(len({original, pck_path, output, receipt_path}) == 4, 'Input/output paths must differ')
    release = json.loads((pck_path.parent / 'release_info.json').read_text(encoding='utf-8-sig'))
    tiles.require(release.get('version') == 'v0.111.0' and release.get('commit') == '41cef1ea',
                  'Owner input must be v0.111.0 / 41cef1ea')
    from pck import Pack
    from build_content import texture, image_data
    pack, skins, rows = Pack(pck_path), [], []
    try:
        # The PCK reader verifies each entry's MD5; this private receipt also
        # records the exact derived RGB565/alpha bytes of every appended skin.
        for name, path, size, hsv in SKINS:
            image = texture(pack, path)
            original_size = image.size
            if hsv:
                image = reference_hsv(image, *hsv)
            if image.size != size:
                image = image.resize(size, Image.Resampling.LANCZOS)
            raw = image_data(image, True)
            skins.append((*size, 1, raw))
            rows.append(dict(key='common.' + name, source=path, source_size=original_size,
                             size=size, shader_hsv=hsv, raw_sha256=hashlib.sha256(raw).hexdigest()))
    finally:
        pack.close()
    data, receipt = append_skins(original.read_bytes(), BASE_AUXILIARY_COUNT, skins,
                                profile=(558, 6100, 1695, BASE_AUXILIARY_COUNT))
    tiles.require(receipt['auxiliary_count'] == AUXILIARY_COUNT, 'UI profile count mismatch')
    receipt['skins'] = rows
    tiles.atomic_bytes(output, data)
    tiles.atomic_bytes(receipt_path, json.dumps(receipt, indent=2, ensure_ascii=False).encode() + b'\n')
    print(json.dumps({k: v for k, v in receipt.items() if k != 'skins'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
