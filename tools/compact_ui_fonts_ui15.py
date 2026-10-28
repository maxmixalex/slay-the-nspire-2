#!/usr/bin/env python3
"""Bound cold small-label glyph decoding without changing any native ink.

The calculator draws short labels at4..10px. A64KiB font cohort made one
Chinese character decode hundreds of unrelated glyphs when the shop opened.
Small-label payloads now use4KiB cohorts; larger display bands retain64KiB
cohorts. The existing RGR1/LZMA2 decoder, exact glyph metrics/raw/RLE bytes,
shared artwork, localized text and runtime pools are unchanged.
"""
from __future__ import annotations

import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import lzma
from pathlib import Path
import struct
import time

import content_tiles as tiles
import split_content as split
from group_glyphs import Record, Batch
from ui12_resources import view

SMALL_GROUP_BYTES=4096
LARGE_GROUP_BYTES=65536
ENCODER=b'UI15-small-label-glyphs-v1'


def plan(source,cache,workers=2):
    records=[];identical={};frequency=Counter()
    for i in range(len(source.strings)):
        frequency.update(source.string_payload(i)[:-1].decode('utf-8'))
    # Read each old cohort together, then order only its private payloads.
    for i in sorted(range(len(source.glyphs)),key=lambda j:(source.glyphs[j][1],j)):
        payload=bytes(source.glyph_payload(i))
        if payload not in identical:
            identical[payload]=Record(payload);records.append(identical[payload])
        identical[payload].ids.append(i)
    def key(record):
        return min((source.glyphs[i][2],-frequency[chr(source.glyphs[i][0])],source.glyphs[i][0],i)
            for i in record.ids)
    records.sort(key=key)
    batches=[];pending=[];size=0;limit=0
    for record in records:
        target=SMALL_GROUP_BYTES if any((source.glyphs[i][2]&31)<=10 for i in record.ids) else LARGE_GROUP_BYTES
        if pending and (target!=limit or size+len(record.payload)>target):
            batches.append((pending,size,limit));pending=[];size=0
        pending.append(record);size+=len(record.payload);limit=target
    if pending:batches.append((pending,size,limit))
    def encode(item):
        records,size,limit=item;raw=b''.join(r.payload for r in records)
        digest=hashlib.sha256(ENCODER+raw).hexdigest();path=cache/(digest+'.lzma2')
        if path.exists():encoded=path.read_bytes()
        else:
            candidates=[lzma.compress(raw,format=lzma.FORMAT_RAW,filters=[dict(id=lzma.FILTER_LZMA2,
                preset=9,dict_size=tiles.GROUP_LIMIT,nice_len=273,depth=512,lc=lc,lp=lp,pb=pb)])
                for lc,lp,pb in ((3,0,0),(4,0,1),(4,0,0))]
            encoded=min(candidates,key=len);tiles.atomic_bytes(path,encoded)
        tiles.require(tiles.decode_lzma2(encoded,size)==raw,'Glyph cohort bytes changed')
        previous=sum((len(r.payload)+1)&~1 for r in records)
        after=((len(encoded)+33)&~1)+16*len(records)
        return Batch(records,encoded if after<previous else None,size,previous),limit
    with ThreadPoolExecutor(max_workers=workers) as pool:return list(pool.map(encode,batches))


def write(source,batches,bounds):
    end=bounds['prefix']+bounds['artwork'];out=bytearray(source.data[:end])
    def append(raw):
        out.extend(bytes(len(out)&1));at=len(out);out.extend(raw);return at
    counts=Counter();maxima=Counter()
    for batch,limit in batches:
        group_at=0;group=b''
        if batch.encoded is not None:
            group=tiles.GROUP.pack(b'RGR1',batch.raw_size,len(batch.encoded),14,len(batch.records),0,0,0)+batch.encoded
            group_at=append(group);counts['small_groups' if limit==SMALL_GROUP_BYTES else 'large_groups']+=1
            maxima['small_raw_bytes' if limit==SMALL_GROUP_BYTES else 'large_raw_bytes']=max(
                maxima['small_raw_bytes' if limit==SMALL_GROUP_BYTES else 'large_raw_bytes'],batch.raw_size)
        raw_at=0
        for record in batch.records:
            at=(len(out)+1)&~1
            if group:append(struct.pack('<iIII',group_at-at,len(group),raw_at,len(record.payload)))
            else:append(record.payload)
            for i in record.ids:
                cp,_old,band,w,h,advance,encoded=source.glyphs[i]
                tiles.GLYPH.pack_into(out,source.glyph_at+i*16,cp,at,band,w,h,advance,
                    (encoded&~tiles.GLYPH_GROUP_FLAG)|(tiles.GLYPH_GROUP_FLAG if group else 0))
            raw_at+=len(record.payload)
            counts['grouped_glyphs' if group else 'literal_glyphs']+=len(record.ids)
        tiles.require(raw_at==batch.raw_size,'Glyph cohort coverage changed')
    glyph_end=len(out);copied={};descriptors={}
    def copy(at,size):
        key=at,size
        if key not in copied:copied[key]=append(source.block(at,size))
        return copied[key]
    for i,(at,size) in enumerate(source.strings):
        if size&tiles.STRING_GROUP_FLAG:
            if at not in descriptors:
                relative,length,raw_at,raw_size=struct.unpack_from('<iIII',source.data,at)
                group_at=copy(at+relative,length);target=(len(out)+1)&~1
                descriptors[at]=append(struct.pack('<iIII',group_at-target,length,raw_at,raw_size))
            target=descriptors[at]
        else:target=copy(at,size+1)
        struct.pack_into('<II',out,source.string_at+i*8,target,size)
    struct.pack_into('<I',out,12,len(out));struct.pack_into('<I',out,36,0)
    return bytes(out),dict(counts,**maxima,glyph_tail_bytes=glyph_end-end,
        localized_text_streams_copied_exactly=True,runtime_memory_added=0)


def verify(source,data,bounds):
    target=view(data);end=bounds['prefix']+bounds['artwork']
    tiles.require(source.counts==target.counts and source.images==target.images and source.cards==target.cards
        and source.clips==target.clips and source.aux==target.aux,'Stable catalogs changed')
    tiles.require(source.data[40:source.glyph_at]==target.data[40:target.glyph_at]
        and source.data[source.clip_at:end]==target.data[target.clip_at:end],
        'Artwork, animation streams or non-font metadata changed')
    for i,(old,new) in enumerate(zip(source.glyphs,target.glyphs)):
        tiles.require(old[:1]+old[2:6]==new[:1]+new[2:6] and
            old[6]&~tiles.GLYPH_GROUP_FLAG==new[6]&~tiles.GLYPH_GROUP_FLAG and
            source.glyph_payload(i)==target.glyph_payload(i),'Glyph native ink or metrics changed')
    for i in range(len(source.strings)):
        tiles.require(source.string_payload(i)==target.string_payload(i),'Localized string changed')
    return dict(exact_glyphs=len(source.glyphs),exact_strings=len(source.strings),
        exact_artwork_and_animation_bytes=end-bounds['prefix'],exact_native_glyph_encoding=True,
        exact_keys_bands_advances_bearings=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for key in ('source','output','cache'):parser.add_argument('--'+key,type=Path,required=True)
    parser.add_argument('--languages',default='all');parser.add_argument('--workers',type=int,default=2)
    args=parser.parse_args();source_dir=tiles.private_path(args.source,'Private source editions')
    output=tiles.private_path(args.output,'Private output editions');cache=tiles.private_path(args.cache,'Private compression cache')
    tiles.require(len({source_dir,output,cache})==3,'Input/output paths overlap')
    tiles.require(1<=args.workers<=4,'Invalid worker count')
    codes=list(split.LANGUAGES) if args.languages=='all' else args.languages.split(',')
    tiles.require(codes and set(codes)<=set(split.LANGUAGES) and len(codes)==len(set(codes)),'Invalid locales')
    output.mkdir(parents=True,exist_ok=True);cache.mkdir(parents=True,exist_ok=True)
    common=(source_dir/'nSpire2.dat.tns').read_bytes();tiles.atomic_bytes(output/'nSpire2.dat.tns',common)
    began=time.monotonic();reports={}
    for code in codes:
        original_locale=(source_dir/f'nSpire2.{code}.dat.tns').read_bytes()
        original=split.reconstruct(common,original_locale,code);source=view(original)
        bounds=split.layout(original,code);batches=plan(source,cache,args.workers)
        result,stats=write(source,batches,bounds);proof=verify(source,result,bounds)
        next_bounds=split.layout(result,code)
        tiles.require(next_bounds['artwork']==bounds['artwork'] and next_bounds['prefix']==bounds['prefix'],'Artwork bounds changed')
        localized=split.compress_language_directory(split.language_file(result,next_bounds,code,common[32:64]))
        tiles.require(split.reconstruct(common,localized,code)==result,'Split reconstruction changed bytes')
        tiles.atomic_bytes(output/f'nSpire2.{code}.dat.tns',localized)
        if code in ('eng','zhs'):tiles.atomic_bytes(output/f'ui15-{code}.nsp',result)
        reports[code]=dict(bytes=len(localized),sha256=tiles.sha256(localized),logical_bytes=len(result),
            original_localized_bytes=len(original_locale),delta_bytes=len(localized)-len(original_locale),
            input_sha256=tiles.sha256(original_locale),groups=stats,proof=proof)
        tiles.atomic_bytes(output/f'{code}-receipt.json',json.dumps(reports[code],indent=2).encode()+b'\n')
        print(json.dumps(dict(stage='small_ui_fonts_verified',language=code,**reports[code])),flush=True)
    receipt=dict(schema=1,reference='v0.111.0 / 41cef1ea',source=str(source_dir),
        small_group_raw_limit=SMALL_GROUP_BYTES,large_group_raw_limit=LARGE_GROUP_BYTES,
        common_bytes=len(common),common_sha256=tiles.sha256(common),common_unchanged=True,
        complete_locales=len(codes)==len(split.LANGUAGES),languages=reports,
        elapsed_seconds=round(time.monotonic()-began,3))
    tiles.atomic_bytes(output/'small-ui-fonts-receipt.json',json.dumps(receipt,indent=2).encode()+b'\n')


if __name__=='__main__':main()
