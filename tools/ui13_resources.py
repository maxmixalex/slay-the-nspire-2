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
BASE_AUX, AUXILIARY = 2201, 3339
RANDOM_AUXILIARY = 3343


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
    from doom_ui13_assets import source_assets as doom
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
        ('attacks', attacks, dict(runtime=runtime, dotnet=args.dotnet)),
        ('doom', doom, {})):
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


def extend_doom(args):
    """Extend an already verified1130-image source build without resampling it."""
    from doom_ui13_assets import source_assets, BASE_AUXILIARY
    cache=tiles.private_path(args.cache,'Private UI13 source cache')
    path=cache/'ui13-assets.json';original=path.read_bytes();manifest=json.loads(original)
    tiles.require(manifest.get('reference')=='v0.111.0 / 41cef1ea' and
        [row['auxiliary'] for row in manifest['images']]==list(range(BASE_AUX,BASE_AUXILIARY)),
        'Doom extension requires a complete earlier UI13 source catalog')
    for row in manifest['images']:
        raw=(cache/row['file']).read_bytes()
        tiles.require(len(raw)==row['bytes'] and tiles.sha256(raw)==row['sha256'],'Earlier source image changed')
    rows=source_assets(tiles.private_path(args.pck,'Original PCK'),cache)
    tiles.require([row['auxiliary'] for row in rows]==list(range(BASE_AUXILIARY,AUXILIARY)), 'Incomplete Doom source catalog')
    manifest['images'].extend(rows)
    manifest['extension']=dict(previous_manifest_sha256=tiles.sha256(original),
        source_sha256=tiles.sha256((ROOT/'tools/doom_ui13_assets.py').read_bytes()),
        unchanged_previous_images=BASE_AUXILIARY-BASE_AUX)
    tiles.atomic_bytes(path,json.dumps(manifest,indent=2).encode()+b'\n')
    emit('doom_catalog_extended',images=len(rows),auxiliary=AUXILIARY)


def retained_endpoint():
    from fake_merchant_ui13_assets import AUXILIARY_COUNT
    return AUXILIARY_COUNT


def catalog_endpoint(manifest):
    rows=manifest['images'];end=BASE_AUX+len(rows)
    tiles.require(manifest.get('reference')=='v0.111.0 / 41cef1ea' and
        end in (AUXILIARY,RANDOM_AUXILIARY,retained_endpoint()) and
        [row['auxiliary'] for row in rows]==list(range(BASE_AUX,end)),
        'Incomplete UI13 source manifest')
    return end


def extend_retained(args):
    """Append Random/Fake Merchant while preserving the1138 prior payloads."""
    from random_character_ui13_assets import source_assets as random_character
    from fake_merchant_ui13_assets import source_assets as fake_merchant
    from pck import Pack
    cache=tiles.private_path(args.cache,'Private UI13 source cache')
    path=cache/'ui13-assets.json';original=path.read_bytes();manifest=json.loads(original)
    end=catalog_endpoint(manifest);prefix=manifest['images'][:AUXILIARY-BASE_AUX]
    tiles.require([row['auxiliary'] for row in prefix]==list(range(BASE_AUX,AUXILIARY)),
        'Retained extension needs the complete1138-image source prefix')
    for row in prefix:
        raw=(cache/row['file']).read_bytes()
        tiles.require(len(raw)==row['bytes'] and tiles.sha256(raw)==row['sha256'],
            'Earlier UI13 source image changed')
    baseline=cache/'ui13-assets.before-retained.json'
    if end==AUXILIARY:
        if baseline.exists():tiles.require(baseline.read_bytes()==original,'Different retained-extension baseline')
        else:tiles.atomic_bytes(baseline,original)
    else:
        tiles.require(baseline.is_file() and json.loads(baseline.read_text())['images']==prefix,
            'The retained extension lost its verified original manifest')
    pck=tiles.private_path(args.pck,'Original PCK')
    runtime=tiles.private_path(args.runtime,'Original runtime')
    noise=tiles.private_path(args.fastnoise_header,'Pinned public Godot FastNoiseLite header')
    stamp=hashlib.sha256((identity()+tiles.sha256(runtime.read_bytes())+tiles.sha256(noise.read_bytes())).encode())
    pack=Pack(pck)
    try:
        for name,entry in sorted(pack.entries.items()):stamp.update(name.encode());stamp.update(entry.md5)
    finally:pack.close()
    signature=stamp.hexdigest();added=[]
    for name,builder,start,finish,kwargs in (
        ('random-character',random_character,AUXILIARY,RANDOM_AUXILIARY,{}),
        ('fake-merchant',fake_merchant,RANDOM_AUXILIARY,retained_endpoint(),dict(runtime=runtime,dotnet=args.dotnet))):
        receipt=cache/(name+'-retained-catalog.json')
        saved=json.loads(receipt.read_text()) if receipt.exists() else {}
        if saved.get('signature')==signature:
            rows=saved['images'];emit('retained_source_cache_verified',catalog=name,images=len(rows))
        else:
            emit('retained_source_sampling',catalog=name)
            catalog_cache=cache/'retained-ui13'/name;catalog_cache.mkdir(parents=True,exist_ok=True)
            rows=builder(pck,catalog_cache,fastnoise_header=noise,**kwargs)
            # New writers receive a separate directory; a reused basename
            # cannot overwrite any of the already accepted1138 payloads.
            rows=[dict(row,file=str(catalog_cache.relative_to(cache)/row['file'])) for row in rows]
            tiles.atomic_bytes(receipt,json.dumps(dict(signature=signature,images=rows),indent=2).encode()+b'\n')
        tiles.require([row['auxiliary'] for row in rows]==list(range(start,finish)),
            'Incomplete retained source catalog: '+name)
        for row in rows:
            raw=(cache/row['file']).read_bytes()
            tiles.require(len(raw)==row['bytes'] and tiles.sha256(raw)==row['sha256'],'Changed retained source image')
        added.extend(rows)
    # Verify again after both builders: no catalog writer may overwrite an
    # earlier frame, including the RGB565 shader displacement planes.
    for row in prefix:
        raw=(cache/row['file']).read_bytes()
        tiles.require(len(raw)==row['bytes'] and tiles.sha256(raw)==row['sha256'],'Retained conversion changed the source prefix')
    manifest['images']=prefix+added
    manifest['retained_extension']=dict(previous_manifest_sha256=tiles.sha256(baseline.read_bytes()),
        signature=signature,unchanged_previous_images=len(prefix),added_images=len(added),auxiliary=retained_endpoint())
    catalog_endpoint(manifest)
    tiles.atomic_bytes(path,json.dumps(manifest,indent=2).encode()+b'\n')
    emit('retained_catalog_extended',images=len(added),unchanged_previous_images=len(prefix),auxiliary=retained_endpoint())


def pack_editions(args):
    from combat_ui13_assets import source_replacements
    cache=tiles.private_path(args.cache,'Private UI13 cache')
    output=tiles.private_path(args.output,'Private UI13 editions');output.mkdir(parents=True,exist_ok=True)
    original_common=tiles.private_path(args.base_common,'Previous shared artwork').read_bytes()
    artwork=tiles.private_path(args.artwork,'Dense enemy artwork').read_bytes()
    source=view(artwork)
    tiles.require(len(source.aux)==BASE_AUX,'Enemy pack has unexpected source catalog')
    manifest=json.loads((cache/'ui13-assets.json').read_text())
    auxiliary=catalog_endpoint(manifest)
    replacements={row['image']:Image.open(row['png']).convert('RGBA') for row in
                  source_replacements(tiles.private_path(args.pck,'Original PCK'),cache)}
    artwork,piles=replace_images(artwork,replacements)
    grouped=grouped_art(manifest['images'],cache)
    # Grouping is lossless, including the two RGB565 displacement planes.
    artwork,append=patch_pack(artwork,manifest,cache,base_aux=BASE_AUX,auxiliary=auxiliary,
        input_version=40,version=40,texts=(),base_clips=len(source.clips),
        base_strings=len(source.strings),appended_art=grouped)
    art=view(artwork);common=None;reports={}
    previous_common=(tiles.private_path(args.previous_editions,'Previous UI13 editions')/'nSpire2.common.tns').read_bytes() if args.previous_editions else None
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
        previous_locale=None
        if previous_common is not None:
            previous_locale=(args.previous_editions/f'nSpire2.{language}.tns').read_bytes()
            previous=view(split.reconstruct(previous_common,previous_locale,language))
            tiles.require(all(final.string_payload(i)==previous.string_payload(i)
                for i in range(len(previous.strings)) if i!=CREDIT_SLOT),'Previous UI13 localized text changed')
            for i,(cp,offset,band,w,h,advance,encoded) in enumerate(previous.glyphs):
                tiles.require(actual[cp,band]==(w,h,advance,encoded & ~tiles.GLYPH_GROUP_FLAG,bytes(previous.glyph_payload(i))),
                    'Previous UI13 glyph raster or metrics changed')
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
        if previous_locale is not None:reports[language]['previous_ui13_locale_sha256']=tiles.sha256(previous_locale)
        tiles.atomic_bytes(output/f'{language}-receipt.json',json.dumps(reports[language],ensure_ascii=False,indent=2).encode()+b'\n')
        emit('locale_verified',language=language,bytes=len(directory),glyphs=len(final.glyphs),directory_bytes=layout_report['directory_bytes'])
    receipt=dict(schema=1,reference='v0.111.0 / 41cef1ea',version=40,auxiliary_count=auxiliary,
        common_bytes=len(common),common_sha256=tiles.sha256(common),languages=reports,
        artwork=append,pile_replacements=piles,lossless_append_grouping=grouped['report'])
    tiles.atomic_bytes(output/'locales-receipt.json',json.dumps(receipt,ensure_ascii=False,indent=2).encode()+b'\n')
    emit('all_editions_verified',languages=len(reports),common_bytes=len(common))


def main():
    parser=argparse.ArgumentParser(description=__doc__);sub=parser.add_subparsers(dest='stage',required=True)
    sample=sub.add_parser('assets');sample.add_argument('--runtime',type=Path,required=True)
    sample.add_argument('--dotnet',default='dotnet')
    extension=sub.add_parser('doom')
    retained=sub.add_parser('extend-retained');retained.add_argument('--runtime',type=Path,required=True)
    retained.add_argument('--dotnet',default='dotnet');retained.add_argument('--fastnoise-header',type=Path,required=True)
    build=sub.add_parser('pack');build.add_argument('--artwork',type=Path,required=True)
    build.add_argument('--base-common',type=Path,required=True)
    build.add_argument('--editions',type=Path,action='append',required=True)
    build.add_argument('--fonts',type=Path,required=True);build.add_argument('--output',type=Path,required=True)
    build.add_argument('--keep-logical',default='eng,zhs')
    build.add_argument('--previous-editions',type=Path)
    for command in (sample,extension,retained,build):
        command.add_argument('--pck',type=Path,required=True);command.add_argument('--cache',type=Path,required=True)
    args=parser.parse_args()
    dict(assets=assets,doom=extend_doom,**{'extend-retained':extend_retained},pack=pack_editions)[args.stage](args)


if __name__=='__main__':main()
