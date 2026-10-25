#!/usr/bin/env python3
"""Append the owner's original MegaCrit Spine startup to private UI40 packs.

Only one shared-art file and compressed locale directories are written. The
existing images, clips, fonts and strings are retained; no complete logical
locale pack is written to disk. The optional fixed 37-image catalog does not
change UI40's gameplay IDs or animation directory.
"""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import struct
import unicodedata

import numpy as np
from PIL import Image, ImageDraw
import content_tiles as tiles
import split_content as split
from animation import raster
from build_locale_editions import face_set, glyph_for, rewrite
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
from pck import Pack
from ui12_resources import rle
from world_enemy_assets import SpineSampler

BASE_AUX, FRAME_COUNT, CLIPS = 2161, 37, 1783
RESOURCE = 'animations/ui/logo/logo_megacrit_animate_skel_data.tres'
TEXTS = ((8699, 'settings_ui.SKIP_INTRO_LOGO_HEADER'),
         (8700, 'settings_ui.SKIP_INTRO_LOGO_DESCRIPTION'),
         (8701, 'settings_ui.TOAST_SKIP_INTRO_LOGO_ON'),
         (8702, 'settings_ui.TOAST_SKIP_INTRO_LOGO_OFF'))
LEFT, TOP, WIDTH, HEIGHT = 80, 40, 160, 160


def build_art(pack, runtime, cache):
    cache.mkdir(parents=True, exist_ok=True)
    with SpineSampler(pack, runtime=runtime, work_dir=cache/'spine') as sampler:
        meta = sampler.metadata(RESOURCE)
        clip = next(row for row in meta['animations'] if row['name'] == 'animation')
        tiles.require(abs(clip['duration_ms']-3000) < 1, 'Pinned logo duration changed')
        setup = sampler.sample(RESOURCE, '', [0])[0]
        vertices = np.concatenate([np.asarray(row['vertices']).reshape(-1, 2) for row in setup['draw']])
        size = vertices.max(axis=0)-vertices.min(axis=0)
        scale = min(320*.33/size[0], 240*.33/size[1])
        # NLogoAnimation._Ready centers by bounds.Size, without its origin.
        origin = np.asarray((160, 120))-size*scale*.5
        times = [index/12 for index in range(FRAME_COUNT)]
        frames = sampler.sample(RESOURCE, 'animation', times)
        rows, bounds = [], []
        for index, frame in enumerate(frames):
            draw = sampler.draw(RESOURCE, frame)
            if draw:
                xy = np.concatenate([row[0] for row in draw])*[scale, -scale]+origin
                low, high = xy.min(axis=0), xy.max(axis=0)
                tiles.require(low[0] >= LEFT and low[1] >= TOP and
                              high[0] < LEFT+WIDTH and high[1] < TOP+HEIGHT,
                              'Logo frame exceeds its bounded canvas')
                bounds.append([low.tolist(), high.tolist()])
            image = raster(draw, scale, origin-[LEFT, TOP], (WIDTH, HEIGHT))
            key = 'intro-logo-%02d' % index
            row = write_image(cache, key, image)
            row.update(auxiliary=BASE_AUX+index, source=RESOURCE, source_time_ms=round(times[index]*1000))
            rows.append(row)
            if index in (6, 12, 18, 24, 36):image.save(cache/(key+'.png'))
    return dict(images=rows, clips=[], source=RESOURCE, source_duration_ms=3000,
                sample_rate=12, setup_size=size.tolist(), scale=scale,
                source_origin=origin.tolist(), canvas=[LEFT, TOP, WIDTH, HEIGHT],
                sampled_bounds=bounds, runtime_added_pixel_buffers=0)


def localized_keys(pack, languages):
    fallback = json.loads(pack.read('localization/eng/settings_ui.json'))
    result = {}
    for language in languages:
        source = json.loads(pack.read('localization/'+language+'/settings_ui.json'))
        values = {}
        for _slot, key in TEXTS:
            name = key.split('.', 1)[1]
            text = source.get(name) or fallback.get(name)
            tiles.require(isinstance(text, str) and text and '\0' not in text, 'Missing startup text: '+key)
            values[key] = unicodedata.normalize('NFC', text)
        result[language] = values
    return result


def prepare_locale(data, values, fonts, language):
    """Add only missing label glyphs; existing grapheme IDs remain untouched."""
    import regex
    source = tiles.PackView(data, BASE_AUX, versions=(40,))
    rows = {(band, cp):(cp, band, w, h, advance, encoded & ~tiles.GLYPH_GROUP_FLAG,
                        bytes(source.glyph_payload(i)))
            for i, (cp, _off, band, w, h, advance, encoded) in enumerate(source.glyphs)}
    next_cluster = max((cp for _band, cp in rows if 0xf0000 <= cp < 0xffffd), default=0xeffff)+1
    clusters, cluster_ids = {}, {}
    def cluster(match):
        nonlocal next_cluster
        text = match[0]
        if len(text) == 1 or not any(unicodedata.category(ch).startswith('M') or ch == '\u0e33' for ch in text):
            return text
        if text not in cluster_ids:
            tiles.require(next_cluster < 0xffffd, 'Startup grapheme IDs exceed pack-local range')
            cluster_ids[text] = next_cluster;clusters[next_cluster] = text;next_cluster += 1
        return chr(cluster_ids[text])
    values = {key:regex.sub(r'\X', cluster, text) for key, text in values.items()}
    alphabet = set(map(ord, ''.join(values.values())))-{10, 13}
    needed = [(cp, px, style) for cp in sorted(alphabet) for px, style in ((10, 0), (12, 0), (10, 5), (12, 5))
              if (px|((style&3)<<6)|((style&4)<<3), cp) not in rows]
    if not needed:return data, values, dict(added_glyphs=0, shaped_graphemes=0)
    faces, fallback = face_set(fonts, language)
    for cp, px, style in needed:
        text = clusters.get(cp, chr(cp));codepoints = set(map(ord, text))
        chosen = next((face for face in [faces[style], *fallback] if codepoints <= face.coverage), None)
        tiles.require(chosen is not None, 'Original font cannot draw new startup glyph '+str(cp))
        row = glyph_for(chosen, cp, text, px, style)
        if 32 <= cp < 0x530:
            font = chosen.fonts[px];advance = round(font.getlength(text)*64);left = min(0, font.getbbox(text)[0])
            width = max(1, math.ceil(advance/64), font.getbbox(text)[2])-left
            image = Image.new('L', (width, px+5));ImageDraw.Draw(image).text((-left, px+1), text, font=font, fill=255, anchor='ls')
            raw, encoded = rle(image.tobytes())
            tiles.require(-7 <= left <= 0 and advance <= 2560, 'Startup glyph metrics exceed UI40')
            row = (cp, row[1], width, px+5, 0x8000|((-left)<<12)|advance, encoded, raw)
        rows[(row[1], cp)] = row
    tiles.require(len(rows) <= 65535, 'Startup locale exceeds the bounded glyph directory')
    texts = [source.string_payload(i)[:-1].decode('utf-8') for i in range(source.counts[2])]
    result, receipt = rewrite(data, texts, sorted(rows.values(), key=lambda row:(row[1], row[0])), language)
    receipt.update(added_glyphs=len(needed), shaped_graphemes=len(clusters))
    return result, values, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--cache-dir', type=Path, required=True)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--fonts', type=Path, required=True)
    parser.add_argument('--languages', default='all')
    args = parser.parse_args()
    source_dir = tiles.private_path(args.input_dir, 'Private UI40 inputs')
    output = tiles.private_path(args.output_dir, 'Private startup outputs')
    cache = tiles.private_path(args.cache_dir, 'Private startup samples')
    tiles.require(source_dir != output and output != cache, 'Input/output/cache must differ')
    languages = list(split.LANGUAGES) if args.languages == 'all' else args.languages.split(',')
    tiles.require(languages and all(code in split.LANGUAGES for code in languages), 'Unsupported locale')
    pack = Pack(tiles.private_path(args.pck, 'Owner PCK'))
    try:
        manifest = build_art(pack, args.runtime, cache)
        manifest['localized_text'] = localized_keys(pack, languages)
    finally:pack.close()
    tiles.atomic_bytes(cache/'startup-art.json', json.dumps(manifest, ensure_ascii=False, indent=2).encode()+b'\n')
    common = (source_dir/'nSpire2.common.tns').read_bytes();new_common = None
    report = dict(reference='v0.111.0 / 41cef1ea', startup=manifest, locales={}, logical_files_written=0)
    for language in languages:
        data = split.reconstruct(common, (source_dir/('nSpire2.'+language+'.tns')).read_bytes(), language)
        data, values, glyphs = prepare_locale(data, manifest['localized_text'][language], args.fonts, language)
        localized = dict(manifest);localized['localized_text'] = {language:values}
        result, receipt = patch_pack(data, localized, cache, base_aux=BASE_AUX,
            auxiliary=BASE_AUX+FRAME_COUNT, input_version=40, version=40,
            texts=TEXTS, base_clips=CLIPS, base_strings=8704)
        bounds = split.layout(result, language)
        art = result[bounds['prefix']:bounds['prefix']+bounds['artwork']]
        if new_common is None:
            new_common = split.common_file(art);tiles.atomic_bytes(output/'nSpire2.common.tns', new_common)
        else:tiles.require(art == new_common[64:], 'Startup artwork differs between locales')
        directory = split.compress_language_directory(split.language_file(result, bounds, language, new_common[32:64]))
        tiles.require(split.reconstruct(new_common, directory, language) == result, 'Startup split roundtrip differs')
        tiles.atomic_bytes(output/('nSpire2.'+language+'.tns'), directory)
        report['locales'][language] = dict(patch=receipt, glyphs=glyphs,
            localized_bytes=len(directory), logical_sha256=tiles.sha256(result), nsl2_sha256=tiles.sha256(directory))
        tiles.atomic_bytes(output/'startup-receipt.json', json.dumps(report, ensure_ascii=False, indent=2).encode()+b'\n')
        print(json.dumps(dict(language=language, common_bytes=len(new_common), added_common_bytes=len(new_common)-len(common),
            locale_bytes=len(directory), added_glyphs=glyphs['added_glyphs'], all_existing_art_unchanged=True)), flush=True)
    report['common_bytes'] = len(new_common);report['common_sha256'] = tiles.sha256(new_common)
    report['default_bytes'] = len(new_common)+sum(report['locales'][code]['localized_bytes'] for code in ('eng', 'zhs') if code in report['locales'])
    tiles.atomic_bytes(output/'startup-receipt.json', json.dumps(report, ensure_ascii=False, indent=2).encode()+b'\n')


if __name__ == '__main__':main()
