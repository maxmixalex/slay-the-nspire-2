#!/usr/bin/env python3
"""NSP2/42: remove unreferenced slots inside existing direct tile groups.

Group IDs, the divisor in TIL1 and every frame's group set are preserved. The
existing pixel codec decodes fewer complete 770-byte records. A wholly unused
group retains one unreferenced 1x1 zero dummy; no empty decoder allocation is
introduced. Raw size is always items*770, and references are bounded by items.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import struct
import time

import numpy as np

import content_tiles as t
import coalesce_tile_resources as coalesce
from recompress_resources import Relocation
import split_content as split
import storage_parts as storage
from tile_filter import restore_tiles


def weighted_sample(reader, eligible, count):
    if not count or count >= len(eligible):
        return eligible
    weights=np.array([reader.groups[g][1]+32 for g in eligible],dtype=np.int64)
    cumulative=np.cumsum(weights)
    return sorted({eligible[int(np.searchsorted(cumulative,(i+.5)*int(cumulative[-1])/count))]
                   for i in range(count)})


def make_plan(source, work, *, workers=2, sample=None):
    reader=t.TileReader(source)
    indices,frames,referenced=coalesce.index_inventory(source,reader)
    cache=Path(work)/'compact-encoded';cache.mkdir(parents=True,exist_ok=True)
    eligible=[]
    for group in range(reader.tile_groups):
        first=group*reader.per_group;items=reader.groups[group][5]
        if int(referenced[first:first+items].sum()) < items:
            eligible.append(group)
    selected=weighted_sample(reader,eligible,sample)
    original={g:bytes(source.data[row[0]-32:row[0]+row[1]]) for g,row in enumerate(reader.groups)}
    plan={g:(payload,payload,payload) for g,payload in original.items()}
    mapping=np.arange(reader.tile_count,dtype='<u4')
    slots={}

    def convert(group):
        items=reader.groups[group][5];first=group*reader.per_group
        kept=np.flatnonzero(referenced[first:first+items])
        raw=t.decode_lzma2(original[group][32:],items*t.TILE_BYTES)
        if reader.filters[group]: raw=restore_tiles(raw,reader.filters[group])
        t.validate_tiles(raw)
        compact=(np.frombuffer(raw,dtype='u1').reshape(items,t.TILE_BYTES)[kept].tobytes()
                 if len(kept) else b'\x01\x01'+bytes(t.TILE_BYTES-2))
        header=list(t.GROUP.unpack_from(original[group]));header[3]=len(compact);header[6]=max(1,len(kept))
        payload,physical=coalesce.encode(compact,t.GROUP.pack(*header)+original[group][32:],reader,group,cache)
        # Do not exchange additional disk bytes for a smaller raw group. The
        # complete candidate must also pay for every changed index stream.
        if len(physical)>len(original[group]): return group,None,None,None
        return group,payload,physical,kept

    with ThreadPoolExecutor(max_workers=workers) as pool:
        for done,(group,payload,physical,kept) in enumerate(pool.map(convert,selected),1):
            if payload is not None:
                plan[group]=original[group],payload,physical;slots[group]=kept
                if len(kept):mapping[group*reader.per_group+kept]=group*reader.per_group+np.arange(len(kept),dtype='<u4')
            if done%64==0 or done==len(selected):
                print(json.dumps(dict(stage='compact_tile_groups',tested=done,total=len(selected),
                    selected=len(slots),pixel_bytes_saved=sum(len(a)-len(c) for a,_b,c in plan.values()))),flush=True)

    def index_job(local):
        group=reader.tile_groups+local;old=indices[local]
        raw=coalesce.mapped_index(old,reader.groups[group][5],reader.tile_count,mapping)
        if old==raw:return group,original[group],original[group]
        payload,physical=coalesce.encode(raw,original[group],reader,group,cache)
        return group,payload,physical

    for _local,_at,active in frames:
        t.require(np.array_equal(active//reader.per_group,mapping[active]//reader.per_group),
                  'Slot compaction changes a frame group')
    with ThreadPoolExecutor(max_workers=workers) as pool:
        for done,(group,payload,physical) in enumerate(pool.map(index_job,range(reader.index_groups)),1):
            plan[group]=original[group],payload,physical
            if done%128==0 or done==reader.index_groups:
                print(json.dumps(dict(stage='compact_frame_indices',completed=done,groups=reader.index_groups,
                    net_bytes_saved=sum(len(a)-len(c) for a,_b,c in plan.values()))),flush=True)
    old_raw=sum(row[2] for row in reader.groups[:reader.tile_groups])
    new_raw=sum(t.GROUP.unpack_from(plan[g][1])[3] for g in range(reader.tile_groups))
    report=dict(eligible_tile_groups=len(eligible),tested_tile_groups=len(selected),
                compacted_tile_groups=len(slots),dummy_groups=sum(len(s)==0 for s in slots.values()),
                changed_index_groups=sum(plan[g][0]!=plan[g][1] for g in range(reader.tile_groups,len(reader.groups))),
                pixel_bytes_saved=sum(len(plan[g][0])-len(plan[g][2]) for g in range(reader.tile_groups)),
                index_bytes_saved=sum(len(plan[g][0])-len(plan[g][2]) for g in range(reader.tile_groups,len(reader.groups))),
                aligned_bytes_saved=sum(len(a)-len(c) for a,_b,c in plan.values()),
                raw_tile_bytes_before=old_raw,raw_tile_bytes_after=new_raw,raw_tile_bytes_removed=old_raw-new_raw,
                frame_group_sets_unchanged=True,group_ids_unchanged=True,resident_bytes_added=0,
                all_eligible_groups_considered=len(selected)==len(eligible),sample=sample)
    return plan,mapping,referenced,slots,report


def partition_like(reference_index, reference_logical, logical):
    """Keep proven concern ownership instead of voting again after ID changes."""
    old,new=coalesce.view(reference_logical),coalesce.view(logical)
    a,b=t.TileReader(old),t.TileReader(new)
    t.require((a.tile_count,a.tile_groups,a.index_groups)==(b.tile_count,b.tile_groups,b.index_groups),
              'Concern reference has different groups')
    replacements=[]
    for previous,current in zip(a.groups,b.groups):
        at,length=previous[:2];nat,nlength=current[:2]
        old_payload=reference_logical[at-32:at+length];payload=logical[nat-32:nat+nlength]
        if payload!=old_payload:
            physical=payload+bytes((len(old_payload)-len(payload))&1)
            replacements.append((at-32,len(old_payload),payload,physical))
    relocate=Relocation(replacements)
    old_language=old.string_payload(split.MARKER)[:-1].decode('ascii')
    new_language=new.string_payload(split.MARKER)[:-1].decode('ascii')
    op=split.layout(reference_logical,old_language)['prefix'];bounds=split.layout(logical,new_language)
    nprefix=bounds['prefix'];common=split.common_file(logical[nprefix:nprefix+bounds['artwork']])
    t.require(reference_index[:4]==b'NSM1' and len(reference_index)>=96,'Invalid concern reference')
    count=struct.unpack_from('<I',reference_index,24)[0]
    t.require(len(reference_index)==96+16*count,'Invalid concern reference size')
    files=[bytearray(32) for _ in storage.NAMES];rows=[]
    for i in range(count):
        first,last,_physical,part=struct.unpack_from('<4I',reference_index,96+i*16)
        start,end=relocate(op+first-64),relocate(op+last-64)
        t.require(nprefix<=start<end<=nprefix+bounds['artwork'] and part<len(files),
                  'Relocated concern leaves artwork')
        position=len(files[part]);files[part].extend(logical[start:end])
        rows.append((64+start-nprefix,64+end-nprefix,position,part))
    for i,buf in enumerate(files):struct.pack_into('<4s7I',buf,0,b'NSS1',1,t.REFERENCE,32,len(buf),i,0,0)
    index=struct.pack('<4s7I',b'NSM1',1,t.REFERENCE,96,96+len(rows)*16,len(common),len(rows),len(files))+common[:64]
    index+=b''.join(struct.pack('<4I',*row) for row in rows)
    parts={name:bytes(data) for name,data in zip(storage.PART_FILES,files)}
    t.require(storage.join_parts(index,parts)==common,'Preserved concerns changed artwork bytes')
    receipt=dict(spans=len(rows),virtual_bytes=len(common),index_bytes=len(index),
                 installed_bytes=len(index)+sum(len(data) for data in parts.values()),
                 original_concern_ownership_preserved=True,complete_byte_identical_roundtrip=True,
                 files={name:dict(bytes=len(data),sha256=t.sha256(data)) for name,data in parts.items()})
    return common,index,parts,receipt


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--work',type=Path,required=True)
    parser.add_argument('--layout-reference',type=Path)
    parser.add_argument('--workers',type=int,default=2)
    parser.add_argument('--sample',type=int)
    parser.add_argument('--locales',nargs='+',choices=split.LANGUAGES)
    args=parser.parse_args();t.require(1<=args.workers<=2,'Use one or two workers')
    t.require(args.sample is None or args.sample>0,'Sample count must be positive')
    source=t.private_path(args.input,'Private input');target=t.private_path(args.output,'Private output')
    work=t.private_path(args.work,'Private work')
    t.require(source!=target and source not in target.parents and target not in source.parents,
              'Never overwrite the source resource directory')
    target.mkdir(parents=True,exist_ok=True);work.mkdir(parents=True,exist_ok=True)
    old_index=(source/'nSpire2.dat.tns').read_bytes()
    old_common=storage.join_parts(old_index,{n:(source/n).read_bytes() for n in storage.PART_FILES})
    inputs={code:source/f'nSpire2.{code}.dat.tns' for code in (args.locales or split.LANGUAGES)}
    t.require('eng' in inputs and all(p.is_file() for p in inputs.values()),'Requested locale is missing')
    english=split.reconstruct(old_common,inputs['eng'].read_bytes(),'eng')
    layout_index,layout_logical=old_index,english
    if args.layout_reference:
        folder=t.private_path(args.layout_reference,'Private layout reference')
        layout_index=(folder/'nSpire2.dat.tns').read_bytes()
        shared=storage.join_parts(layout_index,{n:(folder/n).read_bytes() for n in storage.PART_FILES})
        layout_logical=split.reconstruct(shared,(folder/'nSpire2.eng.dat.tns').read_bytes(),'eng')
    started=time.monotonic()
    plan,mapping,keep,slots,report=make_plan(coalesce.view(english),work,workers=args.workers,sample=args.sample)
    report.update(schema=1,reference='v0.111.0 / 41cef1ea',version=42,locales={})
    if report['aligned_bytes_saved']<=0:
        report.update(complete=False,beneficial=False,elapsed_seconds=time.monotonic()-started)
        t.atomic_bytes(target/'compact-slots-receipt.json',json.dumps(report,indent=2).encode()+b'\n')
        print(json.dumps(report),flush=True)
        t.require(args.sample is not None,'Full slot compaction is not smaller')
        return
    common=None
    for code in ('eng',*(c for c in split.LANGUAGES if c!='eng' and c in inputs)):
        original=english if code=='eng' else split.reconstruct(old_common,inputs[code].read_bytes(),code)
        converted,_=coalesce.rewrite(original,plan,version=42)
        proof=coalesce.verify(original,converted,plan,mapping,keep,None,
                              pixel_groups=code=='eng',compact_slots=slots)
        row=split.layout(converted,code);artwork=converted[row['prefix']:row['prefix']+row['artwork']]
        if common is None:
            common,index,parts,receipt=partition_like(layout_index,layout_logical,converted)
            t.atomic_bytes(target/'nSpire2.dat.tns',index)
            for name,payload in parts.items():t.atomic_bytes(target/name,payload)
            report['artwork']=receipt
        else:t.require(common[64:]==artwork,'Locales lost common artwork equality')
        language=split.compress_language_directory(split.language_file(converted,row,code,bytes(32)))
        language,details=storage.compress_blocks(language)
        t.require(split.reconstruct(common,language,code)==converted,'Split locale does not reconstruct')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns',language)
        if code in ('eng','zhs'):t.atomic_bytes(work/(code+'-compact42.nsp'),converted)
        report['locales'][code]=dict(proof=proof,storage=details,stored_bytes=len(language))
        print(json.dumps(dict(stage='locale_complete',locale=code,**proof,stored_bytes=len(language))),flush=True)
    report.update(complete=args.sample is None,beneficial=True,elapsed_seconds=time.monotonic()-started,
                  installed_artwork_bytes=sum((target/n).stat().st_size for n in ('nSpire2.dat.tns',*storage.PART_FILES)),
                  installed_locale_bytes=sum((target/f'nSpire2.{c}.dat.tns').stat().st_size for c in inputs))
    t.atomic_bytes(target/'compact-slots-receipt.json',json.dumps(report,indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('locales','artwork')}),flush=True)


if __name__=='__main__':main()
