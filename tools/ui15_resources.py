#!/usr/bin/env python3
"""Assemble private UI15 artwork and exact, demand-ordered locale fonts.

Inputs are the owner's immutable UI14 split delivery and original PCK/fonts.
No private pixels, fonts, strings or generated resource files belong in source.
The optional partial-art mode is for named integration gates, never a complete
delivery. Final assembly requires the verified continuous Vantom catalog.
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw

import content_tiles as tiles
import split_content as split
from build_locale_editions import face_set, glyph_for, rewrite
from combat_ui15_assets import source_assets as combat_assets
from glyph_groups_ui15 import repack as regroup_glyphs
from patch_frontend_assets import patch_pack
from patch_phobia_assets import grouped_art
from rooms_ui15_assets import source_assets as room_assets
from reclaim_unused_tiles_ui14 import reclaim
from scene_ui9_fonts import text_index
from ui12_resources import replace_images, rle, view
from ui14_resources import graft_locale

BASE_AUXILIARY = 5275
PARTIAL_AUXILIARY = 5288
VANTOM_AUXILIARY = 5404
SUBSTITUTE_LANGUAGES = frozenset(('zhs', 'zht', 'jpn', 'kor', 'pol', 'rus', 'tha'))
SORT_KEYS = ('OBTAINED', 'TYPE', 'COST', 'ALPHABET')


def emit(stage, **values):
    print(json.dumps(dict(stage=stage, **values), ensure_ascii=False), flush=True)


def absolute_rows(catalog, directory):
    return [dict(row, file=str((directory/row['file']).resolve())) for row in catalog]


def append_sort_fonts(data, font_directory):
    """Add only genuinely missing source-sized sort glyphs; keep old rasters."""
    source = view(data)
    language = source.string_payload(split.MARKER)[:-1].decode('ascii')
    index = text_index()
    style = 0 if language in SUBSTITUTE_LANGUAGES else 5
    band = 6 | ((style & 3) << 6) | ((style & 4) << 3)
    alphabet = set()
    for key in SORT_KEYS:
        alphabet.update(map(ord, source.string_payload(index['gameplay_ui.SORT_'+key])[:-1].decode('utf-8')))
    alphabet = {cp for cp in alphabet if cp >= 32}
    known = {(row[2], row[0]) for row in source.glyphs}
    missing = sorted(cp for cp in alphabet if (band, cp) not in known)
    if not missing:
        return data, dict(added_glyphs=0, required_sort_glyphs=len(alphabet), face=style)
    faces, fallback = face_set(font_directory, language)
    rows = {}
    for i, (cp, _offset, old_band, width, height, advance, encoded) in enumerate(source.glyphs):
        rows[(old_band, cp)] = (cp, old_band, width, height, advance,
            encoded & ~tiles.GLYPH_GROUP_FLAG, bytes(source.glyph_payload(i)))
    for cp in missing:
        chosen = next((face for face in (faces[style], *fallback) if cp in face.coverage), None)
        tiles.require(chosen is not None, 'Missing original sort glyph '+str(cp))
        row = glyph_for(chosen, cp, chr(cp), 6, style)
        if 32 <= cp < 0x530:
            font = chosen.fonts[6]
            advance = round(font.getlength(chr(cp))*64)
            left = min(0, font.getbbox(chr(cp))[0])
            width = max(1, math.ceil(advance/64), font.getbbox(chr(cp))[2])-left
            tiles.require(-7 <= left <= 0 and advance <= 2560, 'Sort metrics exceed UI40')
            image = Image.new('L', (width, 11))
            ImageDraw.Draw(image).text((-left, 7), chr(cp), font=font, fill=255, anchor='ls')
            raw, encoded = rle(image.tobytes())
            row = (cp, band, width, 11, 0x8000|((-left)<<12)|advance, encoded, raw)
        rows[(band, cp)] = row
    texts = [source.string_payload(i)[:-1].decode('utf-8') for i in range(len(source.strings))]
    result, proof = rewrite(data, texts, [rows[key] for key in sorted(rows)], language)
    target = view(result)
    target_keys = {(row[2], row[0]): i for i, row in enumerate(target.glyphs)}
    for i, old in enumerate(source.glyphs):
        j = target_keys[(old[2], old[0])]
        new = target.glyphs[j]
        tiles.require(old[2:6] == new[2:6] and source.glyph_payload(i) == target.glyph_payload(j),
                      'Adding sort glyphs changed an existing raster or metric')
    tiles.require(all((band, cp) in target_keys for cp in alphabet), 'Missing exact sort font')
    return result, dict(added_glyphs=len(missing), codepoints=missing,
        required_sort_glyphs=len(alphabet), face=style, existing_glyphs_preserved=len(source.glyphs), **proof)


def build_artwork(original, pck, cache, room_catalog=None, vantom_catalog=None, extra_catalogs=()):
    source = view(original)
    tiles.require(len(source.aux) == BASE_AUXILIARY, 'Expected immutable complete UI14 artwork')
    if room_catalog:
        room = json.loads(room_catalog.read_text())
        rows = absolute_rows(room['images'], room_catalog.parent)
        replacements = {int(key): Image.open(path).convert('RGBA')
                        for key, path in room['replacement_files'].items()}
    else:
        room_rows, changed = room_assets(pck, cache/'rooms', source=source)
        rows = absolute_rows(room_rows, cache/'rooms')
        replacements = {image: Image.open(path).convert('RGBA') for image, path in changed.items()}
    rows.extend(absolute_rows(combat_assets(pck, cache/'combat'), cache/'combat'))
    tiles.require([row['auxiliary'] for row in rows] == list(range(BASE_AUXILIARY, PARTIAL_AUXILIARY)),
                  'Missing/reordered Smith or heal artwork')
    auxiliary_rows = list(range(len(rows)))
    auxiliary = PARTIAL_AUXILIARY
    vantom_proof = None
    raw_replacements = {}
    replacement_art = None
    if vantom_catalog:
        vantom = json.loads(vantom_catalog.read_text())
        tiles.require(vantom['first_parameter_image'] == len(source.images)+len(rows),
                      'Vantom parameter image IDs were generated against different inputs')
        replacement_rows = []
        for row in vantom['replacement_images']:
            exact = dict(row,file=str(vantom_catalog.parent/row['file']))
            raw_replacements[row['image']] = exact
            replacement_rows.append(exact)
        # Reused image-directory slots need the same cross-frame compression
        # as appended layers. Keep each original decoded format and byte;
        # in particular, geometry words and dense UV textures are not rasters
        # that may be converted through PNG or a sparse representation.
        replacement_art = grouped_art(replacement_rows, cache)
        replacement_art['image_ids'] = [row['image'] for row in replacement_rows]
        first = len(rows)
        rows.extend(absolute_rows(vantom['images'], vantom_catalog.parent))
        auxiliary_rows.extend(first+i for i in vantom['auxiliary_rows'])
        auxiliary = vantom['auxiliary_count']
        tiles.require(auxiliary == VANTOM_AUXILIARY, 'Unexpected Vantom auxiliary catalog')
        vantom_proof = {key: value for key, value in vantom.items()
                        if key not in ('images', 'replacement_images', 'factored_image_ids')}
    for path in extra_catalogs:
        extra = json.loads(path.read_text())
        tiles.require(extra['base_auxiliary'] == auxiliary, 'Extra catalog is not contiguous')
        first = len(rows)
        rows.extend(absolute_rows(extra['images'], path.parent))
        auxiliary_rows.extend(first+i for i in extra.get('auxiliary_rows', range(len(extra['images']))))
        auxiliary = extra['auxiliary_count']
    artwork, changed = replace_images(original, replacements, raw_replacements=raw_replacements,
                                      replacement_art=replacement_art)
    manifest = dict(images=rows, clips=[], auxiliary_rows=auxiliary_rows,
                    localized_text={code:{} for code in split.LANGUAGES})
    grouped = grouped_art(rows, cache)
    artwork, appended = patch_pack(artwork, manifest, cache, base_aux=BASE_AUXILIARY,
        auxiliary=auxiliary, input_version=40, version=40, texts=(),
        base_clips=len(source.clips), base_strings=len(source.strings), appended_art=grouped)
    reclaimed = None
    if vantom_catalog:
        artwork, reclaimed = reclaim(artwork, cache/'reclaimed-vantom-tiles', 2)
    tiles.require(len(view(artwork).aux) == auxiliary, 'Incomplete UI15 artwork directory')
    return artwork, dict(auxiliary_count=auxiliary, changed_images=changed,
        appended=appended, appended_grouping=grouped['report'], vantom=vantom_proof,
        reclaimed_tiles=reclaimed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source-common', 'source-editions', 'pck', 'fonts', 'cache', 'output'):
        parser.add_argument('--'+name, required=True, type=Path)
    parser.add_argument('--room-catalog', type=Path)
    parser.add_argument('--vantom-catalog', type=Path)
    parser.add_argument('--extra-catalog', type=Path, action='append', default=[])
    parser.add_argument('--partial-art', action='store_true', help='Named integration gates only; not final delivery')
    parser.add_argument('--reuse-artwork', type=Path)
    parser.add_argument('--languages', default='all')
    args = parser.parse_args()
    tiles.require(args.partial_art or args.vantom_catalog is not None, 'Final artwork requires complete Vantom resources')
    output = tiles.private_path(args.output, 'Private UI15 output');output.mkdir(parents=True, exist_ok=True)
    cache = tiles.private_path(args.cache, 'Private UI15 cache');cache.mkdir(parents=True, exist_ok=True)
    common_source = tiles.private_path(args.source_common, 'Original common data').read_bytes()
    editions = tiles.private_path(args.source_editions, 'Original locale directory')
    if args.reuse_artwork:
        artwork = tiles.private_path(args.reuse_artwork, 'Previously verified UI15 artwork').read_bytes()
        art_report = json.loads(args.reuse_artwork.with_suffix('.json').read_text())
    else:
        original = split.reconstruct(common_source, (editions/'nSpire2.eng.dat.tns').read_bytes(), 'eng')
        artwork, art_report = build_artwork(original, args.pck, cache, args.room_catalog,
                                          args.vantom_catalog, args.extra_catalog)
        tiles.atomic_bytes(cache/'ui15-artwork.nsp', artwork)
        tiles.atomic_bytes(cache/'ui15-artwork.json', json.dumps(art_report, indent=2).encode()+b'\n')
    emit('artwork_ready', auxiliary_count=art_report['auxiliary_count'], bytes=len(artwork))
    codes = list(split.LANGUAGES) if args.languages == 'all' else args.languages.split(',')
    tiles.require(codes and set(codes) <= set(split.LANGUAGES) and len(codes) == len(set(codes)), 'Invalid locales')
    common, reports = None, {}
    for code in codes:
        previous = split.reconstruct(common_source, (editions/f'nSpire2.{code}.dat.tns').read_bytes(), code)
        localized, graft = graft_locale(artwork, previous, code)
        localized, fonts = append_sort_fonts(localized, args.fonts)
        localized, grouping = regroup_glyphs(localized, order='band-frequency', group_bytes=32768)
        bounds = split.layout(localized, code)
        candidate = split.common_file(localized[bounds['prefix']:bounds['prefix']+bounds['artwork']])
        if common is None:
            common = candidate;tiles.atomic_bytes(output/'nSpire2.dat.tns', common)
        tiles.require(candidate == common, 'Locale artwork differs')
        directory = split.compress_language_directory(split.language_file(localized, bounds, code, common[32:64]))
        tiles.require(split.reconstruct(common, directory, code) == localized, 'Split readback differs')
        tiles.atomic_bytes(output/f'nSpire2.{code}.dat.tns', directory)
        if code in ('eng', 'zhs'):tiles.atomic_bytes(output/f'ui15-{code}.nsp', localized)
        reports[code] = dict(graft=graft, fonts=fonts, grouping=grouping,
            bytes=len(directory), sha256=tiles.sha256(directory), logical_bytes=len(localized))
        tiles.atomic_bytes(output/f'{code}-receipt.json', json.dumps(reports[code], indent=2).encode()+b'\n')
        emit('locale_verified', language=code, bytes=len(directory), added_glyphs=fonts['added_glyphs'])
    receipt = dict(schema=1, reference='v0.111.0 / 41cef1ea', version=40,
        partial_art=args.partial_art, complete_locales=len(reports) == len(split.LANGUAGES),
        common_bytes=len(common), common_sha256=tiles.sha256(common), artwork=art_report, languages=reports)
    tiles.atomic_bytes(output/'locales-receipt.json', json.dumps(receipt, indent=2).encode()+b'\n')
    emit('editions_verified', languages=len(reports), common_bytes=len(common), partial_art=args.partial_art)


if __name__ == '__main__':
    main()
