#!/usr/bin/env python3
"""NSP2/45: omit exact zero tile pixels before bounded LZMA2 compression.

Every group keeps its complete original records, IDs, cache pages and frame
references. TGR3 is selected only when smaller, occupancy is at most 60%, and
its temporary planes fit the existing TGR2 reservation. Dense groups retain
their earlier stream: a smaller file alone does not establish faster decoding.
Full-group comparisons cover hidden RGB and padding.
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
from compact_tile_slots import partition_like
import content_tiles as t
from recompress_resources import choose_group
import sparse_tile_filter as sparse
import split_content as split
import storage_parts as storage

ENCODER = b'NSP45-TGR3-exact-occupancy-planes-v1-9-273-512'
MAXIMUM_OCCUPANCY_PERCENT = 60


def make_plan(source, work, *, workers=2, maximum_occupancy=MAXIMUM_OCCUPANCY_PERCENT):
    t.require(0 <= maximum_occupancy <= MAXIMUM_OCCUPANCY_PERCENT,
              'Sparse occupancy limit must be between zero and sixty percent')
    reader = t.TileReader(source)
    cache = Path(work)/'sparse-encoded'
    cache.mkdir(parents=True, exist_ok=True)
    original = {g:bytes(source.data[row[0]-32:row[0]+row[1]]) for g,row in enumerate(reader.groups)}
    plan = {g:(payload,payload,payload) for g,payload in original.items()}
    report_rows = []

    def convert(group,raw=None):
        old = original[group]
        header = list(t.GROUP.unpack_from(old))
        items, mode = header[6], reader.filters[group] or 2
        row = dict(group=group, input_bytes=len(old), output_bytes=len(old),
                   raw_bytes=header[3], decoded_bytes=header[3], saved_bytes=0)
        if reader.sparse[group]:
            row['preserved_reason']='already_sparse'
            return group,old,old,row
        t.require(raw is not None and len(raw)==header[3], 'Missing exact source tile records')
        try:
            transformed = sparse.transform(raw, mode)
        except ValueError as error:
            if str(error) != 'Sparse tile planes exceed the existing scratch bound':
                raise
            row['preserved_reason']='existing_scratch_bound'
            return group,old,old,row
        t.require(sparse.restore(transformed,items,mode)==raw, 'Sparse tile transform changed an original byte')
        occupied = sparse.pixel_count(items, len(transformed))
        if occupied * 100 > items * 256 * maximum_occupancy:
            row['preserved_reason'] = 'dense_decode_cost'
            return group,old,old,row
        profiles=tuple(dict.fromkeys((coalesce.source_lzma_profile(old[32:]),(0,1,1),(2,0,1))))
        key=t.sha256(ENCODER+struct.pack('<II',mode,reader.dictionary)+
                     bytes(x for profile in profiles for x in profile)+transformed)
        path=cache/(key+'.lzma2')
        if path.is_file():
            encoded=path.read_bytes()
        else:
            encoded=None
            for lc,lp,pb in profiles:
                candidate=lzma.compress(transformed,format=lzma.FORMAT_RAW,filters=[dict(
                    id=lzma.FILTER_LZMA2,preset=9,dict_size=reader.dictionary,
                    nice_len=273,depth=512,lc=lc,lp=lp,pb=pb)])
                if encoded is None or len(candidate)<len(encoded):encoded=candidate
        t.require(t.decode_lzma2(encoded,len(transformed),reader.dictionary)==transformed,
                  'Cached/new sparse tile stream differs')
        if not path.exists():t.atomic_bytes(path,encoded)
        header[0],header[4],header[5],header[7]=b'TGR3',len(encoded),len(transformed),mode
        candidate=t.GROUP.pack(*header)+encoded
        payload,physical,_=choose_group(old,candidate,len(raw))
        if payload != old:
            row.update(output_bytes=len(payload),saved_bytes=len(old)-len(physical),
                       decoded_bytes=len(transformed),occupied_pixels=occupied,
                       scratch_bytes=items*34+occupied,previous_scratch_bytes=items*258)
        else:row['preserved_reason']='not_smaller'
        return group,payload,physical,row

    # TileReader's bounded cache is not thread-safe. Materialize one exact raw
    # group per worker directly, without sharing its cache mutation.
    def isolated(group):
        row=reader.groups[group];old=original[group]
        if reader.sparse[group]:return convert(group)
        raw=t.decode_lzma2(old[32:],row[2],reader.dictionary)
        if reader.filters[group]:
            from tile_filter import restore_tiles
            raw=restore_tiles(raw,reader.filters[group])
        # Each worker gets only its local group from this decoder shim.
        return convert(group,raw)

    with ThreadPoolExecutor(max_workers=workers) as pool:
        for done,(group,payload,physical,row) in enumerate(pool.map(isolated,range(reader.tile_groups)),1):
            plan[group]=original[group],payload,physical
            report_rows.append(row)
            if done%32==0 or done==reader.tile_groups:
                print(json.dumps(dict(stage='sparse_tiles',complete=done,total=reader.tile_groups,
                    changed=sum(row['saved_bytes']>0 for row in report_rows),
                    saved_bytes=sum(row['saved_bytes'] for row in report_rows))),flush=True)
    delta=sum(len(a)-len(c) for a,_b,c in plan.values())
    padding=delta%4096
    last=max(plan,key=lambda g:sum(reader.groups[g][:2]))
    a,b,c=plan[last];plan[last]=a,b,c+bytes(padding)
    return plan,dict(tile_groups=reader.tile_groups,index_groups=reader.index_groups,
        maximum_occupancy_percent=maximum_occupancy,
        changed_groups=sum(row['saved_bytes']>0 for row in report_rows),
        saved_stream_bytes=delta,alignment_padding_bytes=padding,
        complete_raw_tile_bytes=sum(row['raw_bytes'] for row in report_rows),
        decoded_tile_bytes=sum(row['decoded_bytes'] for row in report_rows),
        group_ids_sizes_pages_and_frame_references_unchanged=True,
        resident_bytes_added=0,scratch_never_exceeds_previous_plane_bytes=True,groups=report_rows)


def restore_dense_groups(source, reference, *, maximum_occupancy=MAXIMUM_OCCUPANCY_PERCENT):
    """Apply the bounded policy to an already converted private package.

    Only the corresponding original dense stream may replace a sparse group;
    complete decoded records must match, including invisible RGB and padding.
    This permits a converter-only correction without rerasterizing artwork.
    """
    t.require(0 <= maximum_occupancy <= MAXIMUM_OCCUPANCY_PERCENT,
              'Sparse occupancy limit must be between zero and sixty percent')
    current, previous = t.TileReader(source), t.TileReader(reference)
    t.require((current.tile_count, current.tile_groups, current.index_groups,
               current.frame_count, current.per_group) ==
              (previous.tile_count, previous.tile_groups, previous.index_groups,
               previous.frame_count, previous.per_group), 'Dense reference group identity differs')
    plan, changed, compared = {}, [], 0
    for group, row in enumerate(current.groups):
        at, size, raw_size, compact, kind, items = row
        original = bytes(source.data[at-32:at+size])
        payload = original
        if group < current.tile_groups and current.sparse[group]:
            occupied = sparse.pixel_count(items, compact)
            if occupied * 100 > items * 256 * maximum_occupancy:
                before = previous.groups[group]
                t.require(not previous.sparse[group] and (raw_size, kind, items) ==
                          (before[2], before[4], before[5]), 'Dense reference shape differs')
                t.require(current.group(group)[0] == previous.group(group)[0],
                          'Dense reference changes complete tile records')
                payload = bytes(reference.data[before[0]-32:before[0]+before[1]])
                compared += raw_size
                changed.append(dict(group=group, occupied_pixels=occupied, pixels=items*256,
                                    stored_bytes_added=len(payload)-len(original)))
        plan[group] = original, payload, payload + bytes((len(original)-len(payload)) & 1)
    delta = sum(len(a)-len(c) for a, _b, c in plan.values())
    padding = delta % 4096
    last = max(plan, key=lambda g:sum(current.groups[g][:2]))
    a,b,c = plan[last];plan[last] = a,b,c+bytes(padding)
    return plan, dict(maximum_occupancy_percent=maximum_occupancy,
        restored_dense_groups=len(changed), compared_complete_tile_bytes=compared,
        stored_bytes_added=-delta+padding, alignment_padding_bytes=padding, groups=changed,
        group_ids_sizes_pages_and_frame_references_unchanged=True, resident_bytes_added=0)


def verify(original,result,plan,*,pixels=True):
    source=coalesce.view(original);reader=t.TileReader(source)
    mapping=np.arange(reader.tile_count,dtype='<u4')
    return coalesce.verify(original,result,plan,mapping,None,None,
                           pixel_groups=pixels,compact_slots={})


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--work',type=Path,required=True)
    parser.add_argument('--workers',type=int,default=2)
    parser.add_argument('--locales',nargs='+',choices=split.LANGUAGES)
    parser.add_argument('--maximum-occupancy',type=int,default=MAXIMUM_OCCUPANCY_PERCENT,
                        help='Maximum occupied pixels in percent (0..60)')
    parser.add_argument('--dense-reference',type=Path,
                        help='Private logical pack with matching original dense groups; restore over-dense TGR3 groups')
    args=parser.parse_args();t.require(1<=args.workers<=4,'Use one through four bounded workers')
    source=t.private_path(args.input,'Private source');target=t.private_path(args.output,'Private output')
    work=t.private_path(args.work,'Private work')
    t.require(source!=target and source not in target.parents and target not in source.parents,'Never overwrite the source resources')
    target.mkdir(parents=True,exist_ok=True);work.mkdir(parents=True,exist_ok=True)
    codes=args.locales or list(split.LANGUAGES)
    t.require('eng' in codes and len(codes)==len(set(codes)),'English is required and locales must be unique')
    t.require(all((source/f'nSpire2.{code}.dat.tns').is_file() for code in codes),'Requested locale is missing')
    index=(source/'nSpire2.dat.tns').read_bytes()
    old_common=storage.join_parts(index,{name:(source/name).read_bytes() for name in storage.PART_FILES})
    english=split.reconstruct(old_common,(source/'nSpire2.eng.dat.tns').read_bytes(),'eng')
    started=time.monotonic()
    if args.dense_reference:
        reference=t.private_path(args.dense_reference,'Private dense reference').read_bytes()
        plan,report=restore_dense_groups(coalesce.view(english),coalesce.view(reference),
                                        maximum_occupancy=args.maximum_occupancy)
    else:
        plan,report=make_plan(coalesce.view(english),work,workers=args.workers,
                              maximum_occupancy=args.maximum_occupancy)
    report.update(schema=1,version=sparse.CONTENT_VERSION,reference='v0.111.0 / 41cef1ea',locales={})
    common=None
    for code in ('eng',*(code for code in codes if code!='eng')):
        before=english if code=='eng' else split.reconstruct(old_common,(source/f'nSpire2.{code}.dat.tns').read_bytes(),code)
        result,_=coalesce.rewrite(before,plan,version=sparse.CONTENT_VERSION)
        # restore_dense_groups already compares every changed complete group;
        # verification below also proves all unchanged encoded groups and all
        # directory/slice relocations. Repeating every pixel per locale adds
        # no distinct coverage for identical shared artwork.
        proof=verify(before,result,plan,pixels=code=='eng' and not args.dense_reference)
        bounds=split.layout(result,code);artwork=result[bounds['prefix']:bounds['prefix']+bounds['artwork']]
        if common is None:
            common,new_index,parts,partition=partition_like(index,english,result)
            t.atomic_bytes(target/'nSpire2.dat.tns',new_index)
            for name,payload in parts.items():t.atomic_bytes(target/name,payload)
            report['artwork']=partition;t.atomic_bytes(work/'eng-sparse45.nsp',result)
        else:t.require(common[64:]==artwork,'Locales no longer share identical artwork')
        language=split.compress_language_directory(split.language_file(result,bounds,code,bytes(32)))
        language,details=storage.compress_blocks(language)
        t.require(split.reconstruct(common,language,code)==result,'Sparse tile locale reconstruction differs')
        t.atomic_bytes(target/f'nSpire2.{code}.dat.tns',language)
        report['locales'][code]=dict(proof=proof,storage=details,stored_bytes=len(language))
        print(json.dumps(dict(stage='sparse_locale',locale=code,stored_bytes=len(language),**proof)),flush=True)
    names=('nSpire2.dat.tns',*storage.PART_FILES,*(f'nSpire2.{code}.dat.tns' for code in codes))
    before=sum((source/name).stat().st_size for name in names);after=sum((target/name).stat().st_size for name in names)
    report.update(complete=True,installed_bytes_before=before,installed_bytes_after=after,
                  installed_bytes_saved=before-after,elapsed_seconds=time.monotonic()-started)
    t.atomic_bytes(target/'sparse-tiles-receipt.json',json.dumps(report,indent=2).encode()+b'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('artwork','locales','groups')}),flush=True)


if __name__=='__main__':main()
