#!/usr/bin/env python3
"""Build the UI14 private artwork and all original-language data files.

The source pack and locale inputs are explicit build inputs. Runtime output
uses only data/nSpire2.dat.tns and data/nSpire2.<language>.dat.tns. All source
fonts, localized strings and existing animation clocks remain byte exact.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import struct
from PIL import Image

import content_tiles as tiles
import split_content as split
from combat_ui14_assets import BASE_AUXILIARY, AUXILIARY_COUNT as COMBAT_AUXILIARY, ancient_portrait_overrides, source_assets
from merchant_ui14_assets import AUXILIARY_COUNT, source_assets as merchant_assets
from game_over_ui8_assets import build_game_over_images
from patch_frontend_assets import patch_pack
from patch_phobia_assets import grouped_art
from pck import Pack
from tighten_ui14_resources import tighten
from reclaim_unused_tiles_ui14 import reclaim
from ui12_resources import replace_images, view
from world_scene_assets import load_scene_texture


def emit(stage, **values):
    print(json.dumps(dict(stage=stage, **values), ensure_ascii=False), flush=True)


def graft_locale(artwork, previous, language):
    """Move the complete existing locale tail, including its compressed groups.

    No font regeneration or lossy image work is involved. Relative descriptors
    inside the tail move together; only absolute directory offsets change.
    """
    art, old = view(artwork), view(previous)
    before = split.layout(previous, language);after = split.layout(artwork, 'eng')
    tiles.require(art.cards == old.cards and art.clips == old.clips and
                  old.aux == art.aux[:len(old.aux)] and len(art.strings) == len(old.strings),
                  'Stable model, string or animation identifiers changed')
    art_end = (after['prefix']+after['artwork']+1) & ~1
    old_end = (before['prefix']+before['artwork']+1) & ~1
    delta = (len(old.glyphs)-len(art.glyphs))*16
    out = bytearray(artwork[:art.glyph_at]);out.extend(bytes(len(old.glyphs)*16))
    out.extend(artwork[art.string_at:art_end]);tail_start = len(out)
    out.extend(previous[old_end:]);tail_delta = tail_start-old_end
    for i, (at, w, h, fmt, size) in enumerate(art.images):
        tiles.IMAGE.pack_into(out, 40+i*16, at+delta, w, h, fmt, size)
    for i, (cp, at, band, w, h, advance, encoded) in enumerate(old.glyphs):
        tiles.GLYPH.pack_into(out, art.glyph_at+i*16, cp, at+tail_delta, band, w, h, advance, encoded)
    for i, (at, size) in enumerate(old.strings):
        struct.pack_into('<II', out, art.string_at+delta+i*8, at+tail_delta, size)
    tiled = tiles.TileReader(art);header = list(tiles.GLOBAL.unpack_from(artwork, art.base_end))
    table = header[9]
    for field in (8, 9, 14):header[field] += delta
    tiles.GLOBAL.pack_into(out, art.base_end+delta, *header)
    for i in range(tiled.tile_groups+tiled.index_groups):
        at, size = struct.unpack_from('<II', artwork, table+i*8)
        struct.pack_into('<II', out, table+delta+i*8, at+delta, size)
    for i in range(tiled.frame_count):
        at = tiled.descriptors+i*16;global_at, group, raw_at, size = tiles.DESCRIPTOR.unpack_from(artwork, at)
        tiles.DESCRIPTOR.pack_into(out, at+delta, global_at+delta, group, raw_at, size)
    struct.pack_into('<I', out, 12, len(out));struct.pack_into('<I', out, 20, len(old.glyphs))
    struct.pack_into('<I', out, 36, 0)
    target = view(out);bounds = split.layout(out, language)
    tiles.require(bytes(out[bounds['prefix']:bounds['prefix']+bounds['artwork']]) ==
                  artwork[after['prefix']:after['prefix']+after['artwork']], 'Shared art changed by locale graft')
    for i in range(len(old.strings)):
        tiles.require(old.string_payload(i) == target.string_payload(i), 'Localized string changed')
    for i, record in enumerate(old.glyphs):
        tiles.require(record[2:] == target.glyphs[i][2:] and
                      old.glyph_payload(i) == target.glyph_payload(i), 'Font raster or metric changed')
    return bytes(out), dict(glyphs=len(old.glyphs), strings=len(old.strings),
        locale_tail_bytes=len(previous)-old_end, compressed_locale_tail_unchanged=True,
        unchanged_all_text_and_glyph_pixels=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('artwork', 'source-common', 'source-editions', 'pck', 'cache', 'output'):
        parser.add_argument('--'+key, type=Path, required=True)
    parser.add_argument('--locale-pattern', default='nSpire2.{language}.dat.tns')
    parser.add_argument('--combat-catalog', type=Path)
    parser.add_argument('--merchant-cache', type=Path)
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--keep-logical', default='eng,zhs')
    parser.add_argument('--skip-tighten', action='store_true')
    args = parser.parse_args()
    tiles.require(1 <= args.workers <= 4, 'Use at most four bounded encoder workers')
    cache = tiles.private_path(args.cache, 'Private UI14 cache');cache.mkdir(parents=True, exist_ok=True)
    output = tiles.private_path(args.output, 'Private UI14 editions');output.mkdir(parents=True, exist_ok=True)
    artwork = tiles.private_path(args.artwork, 'Original logical art pack').read_bytes();source = view(artwork)
    tiles.require(len(source.aux) == BASE_AUXILIARY, 'UI14 requires the complete current source catalog')
    pck = tiles.private_path(args.pck, 'Original owner-supplied PCK')
    replacements = {};pack = Pack(pck)
    try:
        rows = build_game_over_images(pack, cache)
        for row in rows:
            image = load_scene_texture(pack, row['source']).convert('RGBA')
            replacements[source.aux[row['auxiliary']]] = image.resize((row['width'], row['height']), Image.Resampling.LANCZOS)
    finally:pack.close()
    portraits = ancient_portrait_overrides(pck, cache)
    for row in portraits:replacements[source.cards[row['card']][0]] = Image.open(row['png']).convert('RGBA')
    merchant_cache = args.merchant_cache or cache/'merchant'
    merchant_rows, merchant_overrides = merchant_assets(args.artwork, merchant_cache)
    for image, png in merchant_overrides.items():replacements[int(image)] = Image.open(png).convert('RGBA')
    artwork, replaced = replace_images(artwork, replacements)
    emit('replaced_original_art', game_over_images=len(rows), ancient_portraits=len(portraits),
         merchant_frames=len(merchant_overrides))
    if args.combat_catalog:
        catalog_path = tiles.private_path(args.combat_catalog, 'Verified private combat catalog')
        manifest = json.loads(catalog_path.read_text());rows = manifest['images']
        art_cache = catalog_path.parent
    else:
        art_cache = cache/'combat';rows = source_assets(pck, art_cache)
    tiles.require([row['auxiliary'] for row in rows] == list(range(BASE_AUXILIARY, COMBAT_AUXILIARY)),
                  'UI14 combat catalog has missing or reordered images')
    rows = [dict(row, file=str(art_cache/row['file'])) for row in rows]
    rows.extend(dict(row, file=str(merchant_cache/row['file'])) for row in merchant_rows)
    tiles.require([row['auxiliary'] for row in rows] == list(range(BASE_AUXILIARY, AUXILIARY_COUNT)),
                  'UI14 merchant static directory is incomplete')
    art_cache = cache
    manifest = dict(images=rows, clips=[], localized_text={code:{} for code in split.LANGUAGES})
    grouped = grouped_art(rows, art_cache)
    artwork, appended = patch_pack(artwork, manifest, art_cache, base_aux=BASE_AUXILIARY,
        auxiliary=AUXILIARY_COUNT, input_version=40, version=40, texts=(),
        base_clips=len(source.clips), base_strings=len(source.strings), appended_art=grouped)
    emit('card_art_appended', auxiliary=AUXILIARY_COUNT, added_images=len(rows))
    artwork, reclaimed = reclaim(artwork, cache/'reclaimed-tiles', args.workers)
    emit('unreferenced_payload_reclaimed', **reclaimed)
    compressed = {}
    if not args.skip_tighten:
        artwork, compressed = tighten(artwork, cache/'stronger-streams', args.workers, artwork_only=True)
        emit('artwork_tightened', **compressed)
    tiles.atomic_bytes(cache/'ui14-artwork.nsp', artwork)
    common_source = tiles.private_path(args.source_common, 'Original common input').read_bytes()
    editions = tiles.private_path(args.source_editions, 'Original locale inputs')
    common = None;reports = {};keep = set(args.keep_logical.split(','))
    for code in split.LANGUAGES:
        path = editions/args.locale_pattern.format(language=code)
        original_locale = path.read_bytes();previous = split.reconstruct(common_source, original_locale, code)
        localized, report = graft_locale(artwork, previous, code);bounds = split.layout(localized, code)
        candidate = split.common_file(localized[bounds['prefix']:bounds['prefix']+bounds['artwork']])
        if common is None:
            common = candidate;tiles.atomic_bytes(output/'nSpire2.dat.tns', common)
        tiles.require(candidate == common, 'Different locale artwork')
        directory = split.compress_language_directory(split.language_file(localized, bounds, code, common[32:64]))
        tiles.require(split.reconstruct(common, directory, code) == localized, 'Split directory roundtrip differs')
        tiles.atomic_bytes(output/f'nSpire2.{code}.dat.tns', directory)
        if code in keep:tiles.atomic_bytes(output/f'ui14-{code}.nsp', localized)
        reports[code] = dict(report, bytes=len(directory), sha256=tiles.sha256(directory), logical_bytes=len(localized))
        emit('locale_verified', language=code, **reports[code])
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea', version=40,
        auxiliary_count=AUXILIARY_COUNT, common_bytes=len(common), common_sha256=tiles.sha256(common),
        replacements=replaced, ancient_portraits=portraits, appended=appended, reclaimed_tiles=reclaimed,
        appended_grouping=grouped['report'], compression=compressed, languages=reports)
    tiles.atomic_bytes(output/'locales-receipt.json', json.dumps(receipt, ensure_ascii=False, indent=2).encode()+b'\n')
    emit('all_editions_verified', languages=len(reports), common_bytes=len(common))


if __name__ == '__main__':main()
