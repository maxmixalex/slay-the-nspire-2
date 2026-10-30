#!/usr/bin/env python3
"""Pair adjacent direct tile groups losslessly into bounded NSP2/44 streams.

The complete index streams, lookup tables, alignment, concern maps and all
requested locale containers are included in the installed-size receipt. The
original 770-byte records are preserved, including hidden RGB and padding.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import lzma
from pathlib import Path
import struct
import time

import numpy as np

import coalesce_tile_resources as coalesce
import content_tiles as t
from recompress_resources import Relocation
from compact_tile_resources import SLICE, grouped_slices
import split_content as split
import storage_parts as storage
from tile_filter import filter_tiles, restore_tiles

VERSION = 44
ENCODER = b'NSP44-pair-direct-tiles-v1-dict1MiB-nice273-depth512'


def make_plan(source, work, *, workers=2):
    reader = t.TileReader(source)
    t.require(reader.dictionary == t.GROUP_LIMIT and reader.per_group <= 680,
              'Merge an existing512KiB tile layout only once')
    indices, frames, referenced = coalesce.index_inventory(source, reader)
    new_groups, per_group = (reader.tile_groups + 1) // 2, reader.per_group * 2
    mapping = np.full(reader.tile_count, 0xffffffff, dtype='<u4')
    originals = [bytes(source.data[row[0]-32:row[0]+row[1]]) for row in reader.groups]
    for group in range(reader.tile_groups):
        first, count = group * reader.per_group, reader.groups[group][5]
        offset = reader.groups[group-1][5] if group & 1 else 0
        mapping[first:first+count] = (group//2)*per_group + offset + np.arange(count, dtype='<u4')
    t.require(np.all(mapping[referenced] < reader.tile_count), 'Index references an absent source slot')
    occurrences_before = occurrences_after = 0
    for _local, _at, active in frames:
        before = np.unique(active // reader.per_group)
        after = np.unique(mapping[active] // per_group)
        t.require(np.array_equal(after, np.unique(before // 2)), 'Pairing changes an active frame group')
        occurrences_before += len(before); occurrences_after += len(after)
    cache = Path(work) / 'paired-encoded'; cache.mkdir(parents=True, exist_ok=True)

    def raw_group(group):
        row = reader.groups[group]
        raw = t.decode_lzma2(originals[group][32:], row[2])
        return restore_tiles(raw, reader.filters[group]) if reader.filters[group] else raw

    def pair(group):
        first = group * 2
        source_ids = list(range(first, min(first + 2, reader.tile_groups)))
        raw = b''.join(raw_group(g) for g in source_ids)
        mode = max(reader.filters[g] for g in source_ids)
        count = len(raw) // t.TILE_BYTES
        transformed = filter_tiles(raw, mode) if mode else raw
        lc, lp, pb = coalesce.source_lzma_profile(originals[first][32:])
        key = t.sha256(ENCODER + struct.pack('<5I', mode, count, lc, lp, pb) + transformed)
        path = cache / (key + '.lzma2')
        if path.is_file():
            payload = path.read_bytes()
        elif len(source_ids) == 1:
            payload = originals[first][32:]
        else:
            payload = lzma.compress(transformed, format=lzma.FORMAT_RAW, filters=[dict(
                id=lzma.FILTER_LZMA2, preset=9, dict_size=t.LARGE_GROUP_LIMIT,
                nice_len=273, depth=512, lc=lc, lp=lp, pb=pb)])
        decoded = t.decode_lzma2(payload, len(raw), t.LARGE_GROUP_LIMIT)
        t.require(decoded == transformed, 'Cached/new paired stream changed decoded bytes')
        t.require((restore_tiles(decoded, mode) if mode else decoded) == raw,
                  'Paired tile filter changed an original record')
        if not path.exists(): t.atomic_bytes(path, payload)
        block = t.GROUP.pack(b'TGR2' if mode else b'TGR1', 1, group,
                             len(raw), len(payload), 0, count, mode) + payload
        return dict(old=source_ids, payload=block, items=count,
                    before=sum(len(originals[g]) for g in source_ids), after=len(block))

    pixels = []
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for done, row in enumerate(pool.map(pair, range(new_groups)), 1):
            pixels.append(row)
            if done % 32 == 0 or done == new_groups:
                print(json.dumps(dict(stage='paired_tiles', complete=done, total=new_groups,
                    saved_bytes=sum(p['before']-p['after'] for p in pixels))), flush=True)

    def index(local):
        group = reader.tile_groups + local
        raw = coalesce.mapped_index(indices[local], reader.groups[group][5], reader.tile_count, mapping)
        if raw == indices[local]: block = originals[group]
        else: block = coalesce.encode(raw, originals[group], reader, group, cache)[0]
        block = bytearray(block); struct.pack_into('<I', block, 8, new_groups + local)
        return bytes(block)

    with ThreadPoolExecutor(max_workers=workers) as pool:
        index_blocks = list(pool.map(index, range(reader.index_groups)))
    report = dict(tile_groups_before=reader.tile_groups, tile_groups_after=new_groups,
                  per_group_before=reader.per_group, per_group_after=per_group,
                  index_groups=reader.index_groups, frame_group_occurrences_before=occurrences_before,
                  frame_group_occurrences_after=occurrences_after,
                  pixel_stream_bytes_saved=sum(p['before']-p['after'] for p in pixels),
                  index_stream_bytes_saved=sum(len(originals[reader.tile_groups+i])-len(b)
                                               for i,b in enumerate(index_blocks)),
                  group_lookup_bytes_saved=(reader.tile_groups-new_groups)*8,
                  largest_group_raw_bytes=max(p['items']*t.TILE_BYTES for p in pixels),
                  largest_group_encoded_bytes=max(len(p['payload']) for p in pixels),
                  resident_arena_bytes_added=0, raw_record_bytes_unchanged=True)
    plan = dict(originals=originals, pixels=pixels, indices=index_blocks, per_group=per_group)
    return plan, mapping, report


def rewrite(data, plan):
    source = coalesce.view(data); old = t.TileReader(source)
    originals, pixels, indices = plan['originals'], plan['pixels'], plan['indices']
    t.require(len(originals) == len(old.groups) and len(indices) == old.index_groups,
              'Locale has different animation groups')
    for original, row in zip(originals, old.groups):
        at, length = row[:2]
        t.require(data[at-32:at+length] == original, 'Locale artwork differs from paired plan')
    header = list(t.GLOBAL.unpack_from(data, source.base_end)); table = header[9]
    new_count = len(pixels) + len(indices)
    replacements = [(table, len(old.groups)*8, bytes(new_count*8), bytes(new_count*8))]
    targets = []
    for row in pixels:
        for index, old_id in enumerate(row['old']):
            original = originals[old_id]; at = old.groups[old_id][0]-32
            payload = row['payload'] if index == 0 else b''
            physical = payload + bytes((len(original)-len(payload)) & 1)
            replacements.append((at, len(original), payload, physical))
            if not index: targets.append((at, len(payload)))
    for local, payload in enumerate(indices):
        old_id = old.tile_groups+local; original=originals[old_id]; at=old.groups[old_id][0]-32
        replacements.append((at, len(original), payload, payload+bytes((len(original)-len(payload)) & 1)))
        targets.append((at, len(payload)))
    replacements.sort()
    # Keep all following retained groups on their proven4KiB page offsets.
    # The complete saved-byte figure pays for this padding and the new table.
    delta = sum(len(physical)-length for _at,length,_payload,physical in replacements)
    padding = (-delta) % 4096
    at, length, payload, physical = replacements[-1]
    replacements[-1] = at, length, payload, physical+bytes(padding)
    relocate = Relocation(replacements)
    out, prior = bytearray(), 0
    for at,length,_payload,physical in replacements:
        t.require(at >= prior, 'Replacement ranges overlap')
        out.extend(data[prior:at]); out.extend(physical); prior=at+length
    out.extend(data[prior:])
    for i,(offset,w,h,fmt,length) in enumerate(source.images):
        t.IMAGE.pack_into(out,40+i*16,relocate(offset),w,h,fmt,length)
    for i,(cp,offset,px,w,h,advance,encoded) in enumerate(source.glyphs):
        t.GLYPH.pack_into(out,source.glyph_at+i*16,cp,relocate(offset),px,w,h,advance,encoded)
    for i,(offset,length) in enumerate(source.strings):
        struct.pack_into('<II',out,source.string_at+i*8,relocate(offset),length)
    for field in (8,9,14): header[field]=relocate(header[field])
    header[3],header[5],header[11],header[12]=16,len(pixels),plan['per_group'],t.LARGE_GROUP_LIMIT
    t.GLOBAL.pack_into(out,source.base_end,*header)
    for i,(offset,length) in enumerate(targets):
        struct.pack_into('<II',out,header[9]+i*8,relocate(offset),length)
    for i in range(old.frame_count):
        offset=old.descriptors+i*16
        global_at,local,raw_at,size=t.DESCRIPTOR.unpack_from(data,offset)
        t.DESCRIPTOR.pack_into(out,relocate(offset),relocate(global_at),local,raw_at,size)
    for offset,(relative,length,raw_at,shape) in grouped_slices(source).items():
        SLICE.pack_into(out,relocate(offset),relocate(offset+relative)-relocate(offset),length,raw_at,shape)
    struct.pack_into('<I',out,4,VERSION);struct.pack_into('<I',out,12,len(out));struct.pack_into('<I',out,36,0)
    return bytes(out),relocate,padding


def verify(data, output, plan, mapping, relocate, *, pixels=True):
    source,target=coalesce.view(data),coalesce.view(output)
    old,new=t.TileReader(source),t.TileReader(target)
    t.require(source.counts==target.counts and source.clips==target.clips and
              source.cards==target.cards and source.aux==target.aux, 'Catalog or frame clocks changed')
    t.require(new.tile_count==old.tile_count and new.per_group==old.per_group*2 and
              new.tile_groups==(old.tile_groups+1)//2 and new.index_groups==old.index_groups,
              'Paired tile header changed logical frame coverage')
    compared=0
    for group,row in enumerate(plan['pixels']):
        at,size=new.groups[group][:2]
        t.require(output[at-32:at+size]==row['payload'],'Published paired stream differs')
        if pixels:
            expected=b''.join(old.group(g)[0] for g in row['old'])
            t.require(new.group(group)[0]==expected,'Paired group changed RGB, alpha, dimensions or padding')
            compared+=len(expected)
    for local,payload in enumerate(plan['indices']):
        group=new.tile_groups+local;at,size=new.groups[group][:2]
        t.require(output[at-32:at+size]==payload,'Published index differs')
        if pixels:
            expected=coalesce.mapped_index(old.group(old.tile_groups+local)[0],new.groups[group][5],old.tile_count,mapping)
            t.require(new.group(group)[0]==expected,'Frame geometry or complete mapped index changed')
    for a,b in zip(source.images,target.images):
        t.require(b==(relocate(a[0]),*a[1:]),'Image dimensions, format or length changed')
        if a[3]<6: t.require(data[a[0]:a[0]+a[4]]==output[b[0]:b[0]+b[4]],'Literal image changed')
        elif a[3]==6:
            d=t.DESCRIPTOR.unpack_from(data,a[0])
            t.require(t.DESCRIPTOR.unpack_from(output,b[0])==(relocate(d[0]),*d[1:]),'Frame descriptor changed')
    seen=set()
    for offset,(relative,length,raw_at,shape) in grouped_slices(source).items():
        group=offset+relative
        t.require(SLICE.unpack_from(output,relocate(offset))==(relocate(group)-relocate(offset),length,raw_at,shape),
                  'Retained image/glyph/string descriptor changed')
        if group not in seen:
            t.require(data[group:group+length]==output[relocate(group):relocate(group)+length], 'Retained compressed bytes changed')
            seen.add(group)
    for i,(a,b) in enumerate(zip(source.glyphs,target.glyphs)):
        t.require(b==(a[0],relocate(a[1]),*a[2:]) and source.glyph_payload(i)==target.glyph_payload(i), 'Glyph shape or metrics changed')
    for i,(a,b) in enumerate(zip(source.strings,target.strings)):
        t.require(b==(relocate(a[0]),a[1]) and source.string_payload(i)==target.string_payload(i), 'Localized text changed')
    return dict(input_bytes=len(data),output_bytes=len(output),saved_bytes=len(data)-len(output),
                tile_record_bytes_compared=compared,images=len(source.images),glyphs=len(source.glyphs),strings=len(source.strings),
                exact_tiles_and_indices=pixels,exact_catalog_and_retained=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--work',type=Path,required=True)
    parser.add_argument('--workers',type=int,default=2)
    parser.add_argument('--locales',nargs='+',choices=split.LANGUAGES)
    args=parser.parse_args();t.require(1<=args.workers<=2,'Use one or two bounded encoder workers')
    source=t.private_path(args.input,'Private source');target=t.private_path(args.output,'Private output');work=t.private_path(args.work,'Private cache')
    t.require(source!=target and source not in target.parents and target not in source.parents and work!=source,
              'Never overwrite the source resource directory')
    target.mkdir(parents=True,exist_ok=True);work.mkdir(parents=True,exist_ok=True)
    codes=args.locales or split.LANGUAGES;t.require('eng' in codes,'English is required for the shared plan')
    inputs={code:source/f'nSpire2.{code}.dat.tns' for code in codes}
    t.require(all(p.is_file() for p in inputs.values()),'Requested locale is missing')
    source_index=(source/'nSpire2.dat.tns').read_bytes()
    old_common=storage.join_parts(source_index,{n:(source/n).read_bytes() for n in storage.PART_FILES})
    english=split.reconstruct(old_common,inputs['eng'].read_bytes(),'eng')
    started=time.monotonic();plan,mapping,report=make_plan(coalesce.view(english),work,workers=args.workers)
    report.update(schema=1,version=VERSION,reference='v0.111.0 / 41cef1ea',locales={})
    common=None
    for code in ('eng',*(c for c in codes if c!='eng')):
        original=english if code=='eng' else split.reconstruct(old_common,inputs[code].read_bytes(),code)
        converted,relocate,padding=rewrite(original,plan)
        proof=verify(original,converted,plan,mapping,relocate,pixels=code=='eng')
        row=split.layout(converted,code);artwork=converted[row['prefix']:row['prefix']+row['artwork']]
        if common is None:
            common=split.common_file(artwork);index,parts,details=storage.partition(common,converted)
            t.atomic_bytes(target/'nSpire2.dat.tns',index)
            for name,payload in parts.items():t.atomic_bytes(target/name,payload)
            report['artwork']=details;report['alignment_padding_bytes']=padding
            t.atomic_bytes(work/'eng-paired44.nsp',converted)
        else:t.require(common[64:]==artwork,'Locales lost common artwork equality')
        language=split.compress_language_directory(split.language_file(converted,row,code,bytes(32)))
        language,details=storage.compress_blocks(language)
        t.require(split.reconstruct(common,language,code)==converted,'Locale reconstruction differs')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns',language)
        report['locales'][code]=dict(proof=proof,storage=details,stored_bytes=len(language))
        print(json.dumps(dict(stage='paired_locale',locale=code,**proof,stored_bytes=len(language))),flush=True)
    names=('nSpire2.dat.tns',*storage.PART_FILES,*(f'nSpire2.{c}.dat.tns' for c in codes))
    before=sum((source/n).stat().st_size for n in names);after=sum((target/n).stat().st_size for n in names)
    report.update(complete=True,beneficial=after<before,installed_bytes_before=before,installed_bytes_after=after,
                  installed_bytes_saved=before-after,elapsed_seconds=time.monotonic()-started)
    t.atomic_bytes(target/'paired-tiles-receipt.json',json.dumps(report,indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('artwork','locales')}),flush=True)
    t.require(after<before,'Complete paired layout is not smaller')


if __name__=='__main__':main()
