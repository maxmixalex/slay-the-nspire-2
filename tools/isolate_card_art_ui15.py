#!/usr/bin/env python3
"""Keep each independently drawn card portrait in its own bounded stream.

The previous eight-card cohorts make a random shop demand up to eight times
its visible picture bytes. This private converter preserves every native
pixel, image ID, geometry, animation stream, font and localized string while
removing that unrelated cold decode work. It uses the existing RGR1/RGR2
formats and adds no runtime memory or loading UI. Inputs are never overwritten.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import lzma
from pathlib import Path
import re
import struct
import time

import content_tiles as tiles
import split_content as split
from retained_filter import GROUP, SLICE, group_layout, transform
from ui12_resources import replace_images, view

ENCODER = b'UI15-independent-card-art-rgr2-v1'
PROFILES = ((3, 0, 0), (4, 0, 1), (4, 0, 0))


def inventory_materials(source):
    """Actual stable stock/UI catalogs; no encounter-specific test IDs."""
    root=Path(__file__).resolve().parents[1]
    # Card frames/resources, original potion catalog, removal/rug, native
    # material borders/banners, the47 original hand samples and fixed room
    # planes. Dynamic idle frame groups keep their existing shared streams.
    auxiliary={22,59,179,181,182,183,184,231,235,240,245}
    for first,last in ((29,46),(53,55),(63,72),(102,153),(248,257),
                       (1188,1228),(1320,1367),(5265,5275)):
        auxiliary.update(range(first,last))
    for key,index in re.findall(r'\{"([^"]+)",(\d+)\}',
            (root/'src/world_image_index.inc').read_text()):
        if key.startswith('relics.') or key in ('common.BACK','common.BACK_OUTLINE',
                'common.BACK_ARROW','merchant.SALE_TAG'):
            auxiliary.add(int(index))
    tiles.require(auxiliary and max(auxiliary)<len(source.aux),'Inventory catalog outside artwork directory')
    return {20,23,55,56,57}|{source.aux[i] for i in auxiliary}


def prepare(source, cache, workers=2, *, include_inventory=False):
    requested = {row[0] for row in source.cards}
    selected_inventory=inventory_materials(source) if include_inventory else set()
    requested.update(selected_inventory)
    descriptors = {source.images[image] for image in requested}
    # An exact directory alias must keep sharing the same native bytes.
    requested.update(i for i, row in enumerate(source.images) if row in descriptors)
    records = {}
    before_decoded = before_stored = 0
    for image in sorted(requested):
        width, height, fmt, raw = source.image_raw(image)
        raw = bytes(raw)
        key = width, height, fmt, raw
        record = records.setdefault(key, dict(width=width, height=height, format=fmt,
            raw=raw, ids=[], existing=None))
        record['ids'].append(image)
        at, _w, _h, stored_format, stored_bytes = source.images[image]
        if stored_format == 7:
            relative, length, raw_at, shape = struct.unpack_from('<iIII', source.data, at)
            payload = bytes(source.block(at+relative, length))
            size, _start, _mode, _slices = group_layout(payload, source.version)
            before_decoded += size;before_stored += length
            if raw_at == 0 and size == len(raw):record['existing'] = payload
        else:
            before_decoded += len(raw);before_stored += stored_bytes

    def encode(record):
        raw, fmt = record['raw'], record['format']
        shape = (fmt << 24) | len(raw)
        digest = hashlib.sha256(ENCODER+struct.pack('<III',record['width'],record['height'],fmt)+raw).hexdigest()
        path = cache/(digest+'.rgr')
        candidates = [record['existing']] if record['existing'] is not None else []
        if path.is_file():
            candidates.append(path.read_bytes())
        else:
            for mode in (0, 1, 2):
                native = raw if not mode else transform(raw, fmt, mode,
                    width=record['width'], height=record['height'])
                for lc, lp, pb in PROFILES:
                    encoded = lzma.compress(native, format=lzma.FORMAT_RAW, filters=[dict(
                        id=lzma.FILTER_LZMA2, preset=9, dict_size=tiles.GROUP_LIMIT,
                        nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
                    payload = GROUP.pack(b'RGR2' if mode else b'RGR1', len(raw), len(encoded),
                        14, 1, mode, 8 if mode else 0, 0)
                    if mode:payload += SLICE.pack(0,shape)
                    candidates.append(payload+encoded)
        best = min(candidates,key=len)
        size, begin, mode, slices = group_layout(best,source.version)
        decoded = tiles.decode_lzma2(best[begin:],size)
        if mode:
            tiles.require(slices == ((0,shape),),'Independent card has unrelated image slices')
            decoded = transform(decoded,fmt,mode,inverse=True,
                width=record['width'],height=record['height'])
        tiles.require(decoded == raw,'Independent card stream changed native pixels')
        tiles.atomic_bytes(path,best)
        native_path=cache/(digest+'.raw');tiles.atomic_bytes(native_path,raw)
        return record,best,native_path

    payload=bytearray();entries=[];sizes=[];ids=[];replacements={}
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for number,(record,stream,native_path) in enumerate(pool.map(encode,records.values()),1):
            payload.extend(bytes(len(payload)&1));group=len(payload);payload.extend(stream)
            payload.extend(bytes(len(payload)&1));descriptor=len(payload)
            raw=record['raw'];shape=(record['format']<<24)|len(raw)
            payload.extend(struct.pack('<iIII',group-descriptor,len(stream),0,shape))
            digest=tiles.sha256(raw)
            row={key:record[key] for key in ('width','height','format')}
            row.update(file=str(native_path),bytes=len(raw),sha256=digest,raw_sha256=digest)
            for image in record['ids']:
                ids.append(image);entries.append((descriptor,7,16));sizes.append(len(raw))
                replacements[image]=row
            if number%64==0:
                print(json.dumps(dict(stage='independent_card_streams',complete=number,
                    total=len(records),payload_bytes=len(payload))),flush=True)
    report=dict(images=len(ids),unique_streams=len(records),payload_bytes=len(payload),
        inventory_material_images=len(selected_inventory),
        sum_independent_cold_decoded_bytes=sum(sizes),sum_previous_cold_decoded_bytes=before_decoded,
        sum_previous_cold_stored_bytes=before_stored,unchanged_native_pixels=True,
        runtime_memory_added=0,codec='existing RGR1/RGR2/LZMA2')
    grouped=dict(image_ids=ids,entries=entries,decoded_sizes=sizes,payload=bytes(payload),report=report)
    return replacements,grouped


def verify(source, data, requested):
    target=view(data)
    tiles.require(source.counts==target.counts and source.cards==target.cards and
        source.clips==target.clips and source.aux==target.aux,'Stable catalogs or clocks changed')
    old,new=tiles.TileReader(source),tiles.TileReader(target)
    tiles.require(len(old.groups)==len(new.groups),'Animation stream catalog changed')
    for a,b in zip(old.groups,new.groups):
        x,n=a[:2];y,m=b[:2]
        tiles.require(n==m and source.block(x-32,n+32)==target.block(y-32,m+32),
            'Animation tile/index stream changed')
    groups=set()
    for image,(a,b) in enumerate(zip(source.images,target.images)):
        tiles.require(a[1:3]==b[1:3],'Image geometry changed')
        if image in requested:
            tiles.require(source.image_raw(image)==target.image_raw(image),'Card native pixels changed')
            at,_,_,fmt,length=b
            tiles.require(fmt==7 and length==16,'Card is not an independent retained stream')
            relative,size,raw_at,shape=struct.unpack_from('<iIII',data,at)
            raw_size,_,_,_=group_layout(target.block(at+relative,size),target.version)
            tiles.require(raw_at==0 and raw_size==shape&0xffffff,'Card still decodes another picture')
            groups.add(at+relative)
        elif a[3]==7:
            ax,an,ar,ash=struct.unpack_from('<iIII',source.data,a[0])
            bx,bn,br,bsh=struct.unpack_from('<iIII',data,b[0])
            tiles.require((an,ar,ash)==(bn,br,bsh) and
                source.block(a[0]+ax,an)==target.block(b[0]+bx,bn),'Unselected retained stream changed')
        else:
            tiles.require(a[3:]==b[3:] and source.block(a[0],a[4])==target.block(b[0],b[4]),
                'Unselected image bytes changed')
    for i,(a,b) in enumerate(zip(source.glyphs,target.glyphs)):
        tiles.require(a[:1]+a[2:]==b[:1]+b[2:] and source.glyph_payload(i)==target.glyph_payload(i),
            'Localized native glyph or metric changed')
    for i in range(len(source.strings)):
        tiles.require(source.string_payload(i)==target.string_payload(i),'Localized text changed')
    return dict(exact_card_images=len(requested),independent_card_streams=len(groups),
        unchanged_other_images=len(source.images)-len(requested),
        exact_animation_streams=len(old.groups),exact_glyphs=len(source.glyphs),
        exact_strings=len(source.strings),exact_clip_timing=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('source','output','cache'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--languages',default='all')
    parser.add_argument('--workers',type=int,default=2)
    parser.add_argument('--inventory-materials',action='store_true')
    args=parser.parse_args()
    source_dir=tiles.private_path(args.source,'Private source editions')
    output=tiles.private_path(args.output,'Private converted editions')
    cache=tiles.private_path(args.cache,'Private compression cache')
    tiles.require(output!=source_dir and cache!=source_dir and output!=cache,'Overlapping input/output')
    tiles.require(1<=args.workers<=4,'Invalid worker count')
    codes=list(split.LANGUAGES) if args.languages=='all' else args.languages.split(',')
    tiles.require(codes and set(codes)<=set(split.LANGUAGES) and len(codes)==len(set(codes)), 'Invalid locales')
    output.mkdir(parents=True,exist_ok=True);cache.mkdir(parents=True,exist_ok=True)
    began=time.monotonic();original_common=(source_dir/'nSpire2.dat.tns').read_bytes()
    english=split.reconstruct(original_common,(source_dir/'nSpire2.eng.dat.tns').read_bytes(),'eng')
    replacements,grouped=prepare(view(english),cache,args.workers,include_inventory=args.inventory_materials)
    common=None;reports={}
    for code in codes:
        original=english if code=='eng' else split.reconstruct(original_common,
            (source_dir/f'nSpire2.{code}.dat.tns').read_bytes(),code)
        source=view(original)
        result,changed=replace_images(original,{},raw_replacements=replacements,replacement_art=grouped)
        proof=verify(source,result,set(replacements));bounds=split.layout(result,code)
        shared=split.common_file(result[bounds['prefix']:bounds['prefix']+bounds['artwork']])
        if common is None:
            common=shared;tiles.atomic_bytes(output/'nSpire2.dat.tns',common)
        tiles.require(common==shared,'Localized shared artwork differs')
        language=split.compress_language_directory(split.language_file(result,bounds,code,common[32:64]))
        tiles.require(split.reconstruct(common,language,code)==result,'Split reconstruction differs')
        tiles.atomic_bytes(output/f'nSpire2.{code}.dat.tns',language)
        if code in ('eng','zhs'):tiles.atomic_bytes(output/f'ui15-{code}.nsp',result)
        reports[code]=dict(bytes=len(language),sha256=tiles.sha256(language),logical_bytes=len(result),
            original_logical_bytes=len(original),input_sha256=tiles.sha256(original),proof=proof)
        tiles.atomic_bytes(output/f'{code}-receipt.json',json.dumps(reports[code],indent=2).encode()+b'\n')
        print(json.dumps(dict(stage='independent_cards_verified',language=code,
            common_bytes=len(common),localized_bytes=len(language))),flush=True)
    receipt=dict(schema=1,reference='v0.111.0 / 41cef1ea',source=str(source_dir),
        original_common_bytes=len(original_common),original_common_sha256=tiles.sha256(original_common),
        common_bytes=len(common),common_sha256=tiles.sha256(common),
        common_delta_bytes=len(common)-len(original_common),complete_locales=len(reports)==len(split.LANGUAGES),
        cards=grouped['report'],languages=reports,elapsed_seconds=round(time.monotonic()-began,3))
    tiles.atomic_bytes(output/'independent-cards-receipt.json',json.dumps(receipt,indent=2).encode()+b'\n')
    print(json.dumps(receipt),flush=True)


if __name__=='__main__':main()
