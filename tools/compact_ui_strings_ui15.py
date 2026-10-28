#!/usr/bin/env python3
"""Keep cold localized UI text groups small without changing UTF-8 bytes.

One short card description previously decoded a32KiB string cohort. Use
at most4KiB per compressed cohort, retain exact aliases, and leave unusually
long individual strings literal. Artwork, animation and glyph streams are
copied exactly. The existing RGR1 codec and runtime cache sizes are unchanged.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import lzma
from pathlib import Path
import struct
import time

import content_tiles as tiles
import split_content as split
from group_strings import Record, Batch, DESCRIPTOR, FLAG, aligned, nonstring_end
from ui12_resources import view

GROUP_BYTES=4096
ENCODER=b'UI15-small-localized-strings-v1'


def plan(source,cache,workers):
    identical={};records=[]
    for index in sorted(range(len(source.strings)),key=lambda i:(source.strings[i][0],i)):
        raw=bytes(source.string_payload(index))
        if raw not in identical:
            identical[raw]=Record(raw);records.append(identical[raw])
        identical[raw].ids.append(index)
    groups=[];pending=[];size=0
    for record in records:
        literal=split.MARKER in record.ids or len(record.payload)>GROUP_BYTES
        if pending and (literal or size+len(record.payload)>GROUP_BYTES):
            groups.append((pending,size,False));pending=[];size=0
        if literal:groups.append(([record],len(record.payload),True))
        else:pending.append(record);size+=len(record.payload)
    if pending:groups.append((pending,size,False))
    def encode(item):
        records,size,literal=item;previous=sum(aligned(len(r.payload)) for r in records)
        if literal:return Batch(records,None,size,previous)
        raw=b''.join(r.payload for r in records)
        digest=hashlib.sha256(ENCODER+raw).hexdigest();path=cache/(digest+'.lzma2')
        if path.exists():encoded=path.read_bytes()
        else:
            variants=[lzma.compress(raw,format=lzma.FORMAT_RAW,filters=[dict(id=lzma.FILTER_LZMA2,
                preset=9,dict_size=tiles.GROUP_LIMIT,nice_len=273,depth=512,lc=lc,lp=lp,pb=pb)])
                for lc,lp,pb in ((3,0,0),(4,0,1),(4,0,0))]
            encoded=min(variants,key=len);tiles.atomic_bytes(path,encoded)
        tiles.require(tiles.decode_lzma2(encoded,size)==raw,'String cohort roundtrip changed')
        after=aligned(32+len(encoded))+len(records)*DESCRIPTOR.size
        return Batch(records,encoded if after<previous else None,size,previous)
    with ThreadPoolExecutor(max_workers=workers) as pool:return list(pool.map(encode,groups))


def write(source,batches,end):
    out=bytearray(source.data[:end])
    def append(raw):
        out.extend(bytes(len(out)&1));at=len(out);out.extend(raw);return at
    grouped=literal=groups=maximum=0
    for batch in batches:
        group=b'';group_at=0;raw_at=0
        if batch.encoded is not None:
            tiles.require(batch.raw_size<=GROUP_BYTES,'Cold string cohort exceeds4KiB')
            group=tiles.GROUP.pack(b'RGR1',batch.raw_size,len(batch.encoded),14,len(batch.records),0,0,0)+batch.encoded
            group_at=append(group);groups+=1;maximum=max(maximum,batch.raw_size)
        for record in batch.records:
            at=aligned(len(out))
            if group:
                append(DESCRIPTOR.pack(group_at-at,len(group),raw_at,len(record.payload)))
                grouped+=len(record.ids)
            else:append(record.payload);literal+=len(record.ids)
            for index in record.ids:
                struct.pack_into('<II',out,source.string_at+index*8,at,
                    len(record.payload)-1|(FLAG if group else 0))
            raw_at+=len(record.payload)
        tiles.require(raw_at==batch.raw_size,'String cohort coverage changed')
    struct.pack_into('<I',out,12,len(out));struct.pack_into('<I',out,36,0)
    return bytes(out),dict(groups=groups,maximum_group_raw_bytes=maximum,
        grouped_string_entries=grouped,literal_string_entries=literal,
        string_tail_bytes=len(out)-end,runtime_memory_added=0)


def verify(source,data,bounds,end,code):
    target=view(data);next_bounds=split.layout(data,code)
    tiles.require(source.version==target.version and source.counts==target.counts and
        source.images==target.images and source.glyphs==target.glyphs and
        source.cards==target.cards and source.clips==target.clips and source.aux==target.aux,
        'Stable image/font/model catalogs changed')
    tiles.require((bounds['prefix'],bounds['artwork'])==(next_bounds['prefix'],next_bounds['artwork']) and
        nonstring_end(target,next_bounds)==end,'Non-string payload boundaries changed')
    tiles.require(source.data[40:source.string_at]==target.data[40:target.string_at] and
        source.data[source.clip_at:end]==target.data[target.clip_at:end],
        'Artwork, animation, glyph or non-string directory bytes changed')
    compared=0
    for i in range(len(source.strings)):
        old,new=source.string_payload(i),target.string_payload(i)
        tiles.require(old==new,'Localized UTF-8 or alias changed');compared+=len(old)
    tiles.require(not target.strings[split.MARKER][1]&FLAG and
        target.string_payload(split.MARKER)==code.encode()+b'\0','Boot marker changed')
    return dict(exact_strings=len(source.strings),utf8_bytes_compared=compared,
        exact_artwork_animation_and_glyph_streams=True,exact_keys_and_aliases=True,literal_boot_marker=True)


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
        bounds=split.layout(original,code);end=nonstring_end(source,bounds)
        result,stats=write(source,plan(source,cache,args.workers),end)
        proof=verify(source,result,bounds,end,code);next_bounds=split.layout(result,code)
        localized=split.compress_language_directory(split.language_file(result,next_bounds,code,common[32:64]))
        tiles.require(split.reconstruct(common,localized,code)==result,'Split reconstruction changed bytes')
        tiles.atomic_bytes(output/f'nSpire2.{code}.dat.tns',localized)
        if code in ('eng','zhs'):tiles.atomic_bytes(output/f'ui15-{code}.nsp',result)
        reports[code]=dict(bytes=len(localized),sha256=tiles.sha256(localized),logical_bytes=len(result),
            original_localized_bytes=len(original_locale),delta_bytes=len(localized)-len(original_locale),
            input_sha256=tiles.sha256(original_locale),groups=stats,proof=proof)
        tiles.atomic_bytes(output/f'{code}-receipt.json',json.dumps(reports[code],indent=2).encode()+b'\n')
        print(json.dumps(dict(stage='small_ui_strings_verified',language=code,**reports[code])),flush=True)
    receipt=dict(schema=1,reference='v0.111.0 / 41cef1ea',source=str(source_dir),
        string_group_raw_limit=GROUP_BYTES,common_bytes=len(common),common_sha256=tiles.sha256(common),
        common_unchanged=True,complete_locales=len(codes)==len(split.LANGUAGES),languages=reports,
        elapsed_seconds=round(time.monotonic()-began,3))
    tiles.atomic_bytes(output/'small-ui-strings-receipt.json',json.dumps(receipt,indent=2).encode()+b'\n')


if __name__=='__main__':main()
