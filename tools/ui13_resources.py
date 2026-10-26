#!/usr/bin/env python3
"""Build private UI13 source artwork and all sixteen compatible locale files.

The artwork stage runs independently of the dense enemy resampling stage.
The pack stage copies that finished art, changes only three pile textures,
and preserves every pre-existing localized string/glyph except the reserved
port-credit label. No original pixels or fonts are written into the repo.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image

import content_tiles as tiles
import split_content as split
from build_locale_editions import rewrite
from frontend_ui13_assets import LANGUAGES, CREDIT_SLOT, localize_credits
from patch_frontend_assets import patch_pack
from patch_phobia_assets import grouped_art
from ui12_resources import replace_images, view

ROOT = Path(__file__).resolve().parents[1]
BASE_AUX, AUXILIARY = 2201, 3331


def emit(stage, **values):
    print(json.dumps(dict(stage=stage, **values), ensure_ascii=False), flush=True)


def identity():
    digest = hashlib.sha256()
    # Changes to the shared source-scene/particle/raster reader invalidate the
    # cached output as well as changes to the four catalog writers themselves.
    for path in sorted((ROOT/'tools').glob('*.py')):
        digest.update(path.name.encode());digest.update(path.read_bytes())
    for path in (ROOT/'tools/SpineSampleAdapter.cs', ROOT/'config/enemy-visuals-0.17.json'):
        digest.update(path.read_bytes())
    return digest.hexdigest()


def assets(args):
    from frontend_ui13_assets import source_assets as frontend
    from rooms_ui13_assets import source_assets as rooms
    from architect_ui13_assets import source_assets as architect
    from architect_attack_assets import source_assets as attacks
    from pck import Pack
    cache=tiles.private_path(args.cache, 'Private UI13 source cache');cache.mkdir(parents=True, exist_ok=True)
    pck=tiles.private_path(args.pck, 'Original PCK');runtime=tiles.private_path(args.runtime, 'Original runtime')
    stamp=hashlib.sha256((identity()+tiles.sha256(runtime.read_bytes())).encode())
    pack=Pack(pck)
    try:
        for name, entry in sorted(pack.entries.items()):stamp.update(name.encode());stamp.update(entry.md5)
    finally:pack.close()
    signature=stamp.hexdigest();rows=[]
    for name, builder, kwargs in (
        ('frontend', frontend, {}), ('rooms', rooms, {}),
        ('architect', architect, dict(runtime=runtime, dotnet=args.dotnet)),
        ('attacks', attacks, dict(runtime=runtime, dotnet=args.dotnet))):
        receipt=cache/(name+'-catalog.json')
        saved=json.loads(receipt.read_text()) if receipt.exists() else {}
        if saved.get('signature')==signature:
            batch=saved['images']
            for row in batch:
                raw=(cache/row['file']).read_bytes()
                tiles.require(len(raw)==row['bytes'] and tiles.sha256(raw)==row['sha256'], 'Changed cached source artwork')
            emit('source_cache_verified', catalog=name, images=len(batch))
        else:
            emit('source_sampling', catalog=name)
            batch=builder(pck,cache,**kwargs)
            tiles.atomic_bytes(receipt,json.dumps(dict(signature=signature,images=batch),indent=2).encode()+b'\n')
            emit('source_sampled', catalog=name, images=len(batch))
        rows.extend(batch)
    tiles.require([row['auxiliary'] for row in rows]==list(range(BASE_AUX,AUXILIARY)), 'Incomplete UI13 directory')
    manifest=dict(schema=1,reference='v0.111.0 / 41cef1ea',signature=signature,
        images=rows,clips=[],localized_text={language:{} for language in LANGUAGES})
    tiles.atomic_bytes(cache/'ui13-assets.json',json.dumps(manifest,indent=2).encode()+b'\n')
    emit('source_catalog_complete', images=len(rows),auxiliary=AUXILIARY)


def pack_editions(args):
    from combat_ui13_assets import source_replacements
    cache=tiles.private_path(args.cache,'Private UI13 cache')
    output=tiles.private_path(args.output,'Private UI13 editions');output.mkdir(parents=True,exist_ok=True)
    original_common=tiles.private_path(args.base_common,'Previous shared artwork').read_bytes()
    artwork=tiles.private_path(args.artwork,'Dense enemy artwork').read_bytes()
    source=view(artwork)
    tiles.require(len(source.aux)==BASE_AUX,'Enemy pack has unexpected source catalog')
    manifest=json.loads((cache/'ui13-assets.json').read_text())
    tiles.require([row['auxiliary'] for row in manifest['images']]==list(range(BASE_AUX,AUXILIARY)), 'Incomplete UI13 source manifest')
    replacements={row['image']:Image.open(row['png']).convert('RGBA') for row in
                  source_replacements(tiles.private_path(args.pck,'Original PCK'),cache)}
    artwork,piles=replace_images(artwork,replacements)
    grouped=grouped_art(manifest['images'],cache)
    # Grouping is lossless, including the two RGB565 displacement planes.
    artwork,append=patch_pack(artwork,manifest,cache,base_aux=BASE_AUX,auxiliary=AUXILIARY,
        input_version=40,version=40,texts=(),base_clips=len(source.clips),
        base_strings=len(source.strings),appended_art=grouped)
    art=view(artwork);common=None;reports={}
    keep=set(args.keep_logical.split(',')) if args.keep_logical else set()
    for language in LANGUAGES:
        candidates=[Path(folder)/f'nSpire2.{language}.tns' for folder in args.editions]
        candidates=[path for path in candidates if path.is_file()]
        tiles.require(len(candidates)==1,'Expected one previous locale file: '+language)
        old_locale=candidates[0].read_bytes()
        original=split.reconstruct(original_common,old_locale,language);old=view(original)
        tiles.require(old.cards==art.cards and old.aux==art.aux[:BASE_AUX] and
            len(old.clips)==len(art.clips) and all(a[:3]==b[:3] for a,b in zip(old.clips,art.clips)),
            'Source model IDs, frame counts or clocks changed')
        texts,glyphs,credits=localize_credits(old,language,tiles.private_path(args.fonts,'Original fonts'))
        tiles.require(all(texts[i].encode()+b'\0'==old.string_payload(i)
            for i in range(len(texts)) if i!=CREDIT_SLOT),'Unrelated localized text changed')
        localized,layout_report=rewrite(artwork,texts,glyphs,language)
        final=view(localized)
        actual={(cp,band):(w,h,advance,encoded & ~tiles.GLYPH_GROUP_FLAG,bytes(final.glyph_payload(i)))
            for i,(cp,offset,band,w,h,advance,encoded) in enumerate(final.glyphs)}
        for i,(cp,offset,band,w,h,advance,encoded) in enumerate(old.glyphs):
            tiles.require(actual[cp,band]==(w,h,advance,encoded & ~tiles.GLYPH_GROUP_FLAG,bytes(old.glyph_payload(i))),
                'Pre-existing glyph raster or metrics changed')
        bounds=split.layout(localized,language)
        candidate=split.common_file(localized[bounds['prefix']:bounds['prefix']+bounds['artwork']])
        if common is None:
            common=candidate;tiles.atomic_bytes(output/'nSpire2.common.tns',common)
        tiles.require(candidate==common,'Locale art differs')
        directory=split.compress_language_directory(split.language_file(localized,bounds,language,common[32:64]))
        tiles.require(split.reconstruct(common,directory,language)==localized,'Split directory differs')
        tiles.atomic_bytes(output/f'nSpire2.{language}.tns',directory)
        if language in keep:tiles.atomic_bytes(output/f'ui13-{language}.nsp',localized)
        reports[language]=dict(bytes=len(directory),sha256=tiles.sha256(directory),
            logical_bytes=len(localized),layout=layout_report,credits=credits,
            previous_locale_sha256=tiles.sha256(old_locale),previous_glyphs_unchanged=True,
            previous_texts_unchanged_except_credit=True)
        tiles.atomic_bytes(output/f'{language}-receipt.json',json.dumps(reports[language],ensure_ascii=False,indent=2).encode()+b'\n')
        emit('locale_verified',language=language,bytes=len(directory),glyphs=len(final.glyphs),directory_bytes=layout_report['directory_bytes'])
    receipt=dict(schema=1,reference='v0.111.0 / 41cef1ea',version=40,
        common_bytes=len(common),common_sha256=tiles.sha256(common),languages=reports,
        artwork=append,pile_replacements=piles,lossless_append_grouping=grouped['report'])
    tiles.atomic_bytes(output/'locales-receipt.json',json.dumps(receipt,ensure_ascii=False,indent=2).encode()+b'\n')
    emit('all_editions_verified',languages=len(reports),common_bytes=len(common))


def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest='stage',required=True)
    sample=sub.add_parser('assets');sample.add_argument('--runtime',type=Path,required=True)
    sample.add_argument('--dotnet',default='dotnet')
    build=sub.add_parser('pack');build.add_argument('--artwork',type=Path,required=True)
    build.add_argument('--base-common',type=Path,required=True)
    build.add_argument('--editions',type=Path,action='append',required=True)
    build.add_argument('--fonts',type=Path,required=True);build.add_argument('--output',type=Path,required=True)
    build.add_argument('--keep-logical',default='eng,zhs')
    for command in (sample,build):
        command.add_argument('--pck',type=Path,required=True);command.add_argument('--cache',type=Path,required=True)
    args=parser.parse_args()
    (assets if args.stage=='assets' else pack_editions)(args)


if __name__=='__main__':main()
