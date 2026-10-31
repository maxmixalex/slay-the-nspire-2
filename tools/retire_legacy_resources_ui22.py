#!/usr/bin/env python3
"""Reclaim only UI22's proved unreachable merchant/Kaiser legacy images.

IDs, dimensions, clip tuples and every reachable decoded byte are preserved.
The complete profile gains one guarded metadata marker. No animation samples,
active actor layers, colors, alpha precision, or runtime caches are removed.
"""
from __future__ import annotations
import argparse
import json
import lzma
from pathlib import Path
import struct
import time

import content_tiles as t
import coalesce_tile_resources as coalesce
import compact_tile_slots as compact
from patch_frontend_assets import patch_pack
from prune_tile_directory_ui13 import live_directory,prune,verify as verify_prune
from release_content_guard import validate_release_content,validate_retirement_intermediate,validate_ui22
import retired_assets_ui22 as contract
from sparse_tile_filter import transform as sparse_transform,restore as sparse_restore
from tile_filter import filter_tiles,restore_tiles
from tile_prediction import index_transform
from ui12_resources import replace_images

ENCODER=b'ui22-retire-same-codec-and-source-profile-v1'


def encode_same(raw,original,reader,group,cache):
    """Preserve the already accepted codec/mode/profile for every group."""
    header=list(t.GROUP.unpack_from(original));count=header[6]
    t.require(header[3]==len(raw),'Compaction group raw length differs')
    sparse=header[0]==b'TGR3'
    filtered=header[0]==b'TGR2'
    predicted=header[0]==b'TGI2'
    transformed=(sparse_transform(raw,header[7]) if sparse else
                 filter_tiles(raw,header[7]) if filtered else
                 index_transform(raw,count,3,tile_count=reader.tile_count) if predicted else raw)
    profile=coalesce.source_lzma_profile(original[32:])
    identity=ENCODER+struct.pack('<8I',header[1],header[6],header[7],reader.dictionary,
        reader.tile_count,*profile)+header[0]+transformed
    path=Path(cache)/(t.sha256(identity)+'.lzma2')
    if path.is_file():payload=path.read_bytes()
    else:
        lc,lp,pb=profile
        payload=lzma.compress(transformed,format=lzma.FORMAT_RAW,filters=[dict(
            id=lzma.FILTER_LZMA2,preset=9,dict_size=reader.dictionary,
            nice_len=273,depth=512,lc=lc,lp=lp,pb=pb)])
        t.atomic_bytes(path,payload)
    decoded=t.decode_lzma2(payload,len(transformed),reader.dictionary)
    restored=(sparse_restore(decoded,count,header[7]) if sparse else
              restore_tiles(decoded,header[7]) if filtered else
              index_transform(decoded,count,3,True,reader.tile_count) if predicted else decoded)
    t.require(restored==raw,'Retirement group encoder changed exact raw bytes')
    header[4]=len(payload);header[5]=len(transformed) if sparse else 0
    result=t.GROUP.pack(*header)+payload
    return result,result+bytes((len(original)-len(result))&1)


def image_reader(source):
    reader=t.TileReader(source)
    return lambda index:reader.image_raw(index) if source.images[index][3]==6 else source.image_raw(index)


def verify_replacements(source,converted,ids):
    target=coalesce.view(converted);before,after=t.TileReader(source),t.TileReader(target)
    t.require(source.counts==target.counts and source.clips==target.clips and
              source.cards==target.cards and source.aux==target.aux,'Retirement changed stable catalog identities')
    t.require((before.tile_count,before.tile_groups,before.index_groups,before.frame_count)==
              (after.tile_count,after.tile_groups,after.index_groups,after.frame_count),
              'Replacement changed a tile directory')
    for a,b in zip(before.groups,after.groups):
        t.require(a[1:]==b[1:] and source.block(a[0]-32,a[1]+32)==target.block(b[0]-32,b[1]+32),
                  'Replacement changed an existing compressed tile/index group')
    original=image_reader(source);retired=[]
    for image,(a,b) in enumerate(zip(source.images,target.images)):
        t.require(a[1:3]==b[1:3],'Retirement changed original image dimensions')
        if image in ids:
            w,h,fmt,raw=original(image);now=target.image_raw(image)
            t.require(now==(w,h,4,bytes(2*h)),'Retired image is not an exact empty sparse replacement')
            retired.append(dict(image=image,width=w,height=h,original_format=fmt,
                                original_raw_sha256=t.sha256(raw),retired_format=4,
                                retired_raw_sha256=t.sha256(now[3]),retired_raw_bytes=len(now[3])))
        elif a[3]==6:
            t.require(a[1:]==b[1:] and source.block(a[0],a[4])==target.block(b[0],b[4]),
                      'An active image descriptor or geometry changed')
        else:t.require(source.image_raw(image)==target.image_raw(image),'A nonretired image changed')
    for i,(a,b) in enumerate(zip(source.glyphs,target.glyphs)):
        t.require(a[:1]+a[2:]==b[:1]+b[2:] and source.glyph_payload(i)==target.glyph_payload(i),
                  'Retirement changed a glyph')
    for i in range(len(source.strings)):
        t.require(source.string_payload(i)==target.string_payload(i),'Retirement changed text')
    return retired


def group_changes(source,target,live_indices):
    """Bind renewed decoder gates to exact changed streams and raw content."""
    old,new=t.TileReader(source),t.TileReader(target)
    pairs=[(g,g) for g in range(old.tile_groups)]+[
        (old.tile_groups+g,new.tile_groups+i) for i,g in enumerate(live_indices)]
    def record(reader,group):
        at,size,raw_size,_extra,kind,items=reader.groups[group]
        header=reader.pack.block(at-32,32)
        payload=reader.pack.block(at,size)
        return dict(group=group,kind=kind,items=items,raw_bytes=raw_size,
            codec=header[:4].decode('ascii'),filter=reader.filters[group],
            lzma_profile=coalesce.source_lzma_profile(payload),
            stream_bytes=size,stream_sha256=t.sha256(payload),
            raw_sha256=t.sha256(reader.group(group)[0]))
    changed=[];unchanged=0
    for before,after in pairs:
        a,b=old.groups[before],new.groups[after]
        same=(a[1:]==b[1:] and old.filters[before]==new.filters[after] and
              old.pack.block(a[0],a[1])==new.pack.block(b[0],b[1]))
        if same:unchanged+=1
        else:changed.append(dict(before=record(old,before),after=record(new,after)))
    kept=set(live_indices)
    removed=[record(old,old.tile_groups+i) for i in range(old.index_groups) if i not in kept]
    return dict(unchanged_streams=unchanged,changed_streams=changed,removed_index_streams=removed,
                pixel_group_ids_unchanged=True)


def convert(data,work,workers=2):
    # Offline decoding shares this host-side LRU across bounded passes. Keep
    # concurrent original/candidate readers below the conversion RAM budget.
    t.HOST_CACHE_GROUPS=16
    contract.verify_runtime_routes();source=coalesce.view(data)
    t.require(source.version==45 and len(source.aux)==contract.BASE_AUX and
              len(source.images)==contract.BASE_IMAGES,'Retirement input must be complete unretired NSP2/45 UI22')
    # Both prior source-art coverage and the new roots are checked before any
    # image is replaced. A marker alone never grants deletion authority.
    validate_release_content(data,'ui21')
    image=image_reader(source);validate_ui22(source,image)
    ids,roots=contract.validate_roots(source,image)
    work=Path(work);work.mkdir(parents=True,exist_ok=True)
    replacements={}
    for image_id in sorted(ids):
        _off,width,height,_fmt,_length=source.images[image_id]
        raw=bytes(2*height);path=work/f'empty-rows-{height}.raw'
        if not path.exists():t.atomic_bytes(path,raw)
        replacements[image_id]=dict(file=str(path),width=width,height=height,format=4,
            bytes=len(raw),sha256=t.sha256(raw),raw_sha256=t.sha256(raw))
    empty,replaced=replace_images(data,{},raw_replacements=replacements)
    retired=verify_replacements(source,empty,ids)
    live_indices=live_directory(coalesce.view(empty))[2]
    pruned,directory=prune(empty);directory['proof']=verify_prune(coalesce.view(empty),pruned)
    plan,mapping,keep,slots,compacted=compact.make_plan(coalesce.view(pruned),work,
        workers=workers,encode_group=encode_same)
    converted,_=coalesce.rewrite(pruned,plan,version=45)
    compacted['proof']=coalesce.verify(pruned,converted,plan,mapping,keep,None,
                                      pixel_groups=True,compact_slots=slots)
    changed=group_changes(source,coalesce.view(converted),live_indices)
    marker=contract.marker_bytes();name='retirement-marker.raw';t.atomic_bytes(work/name,marker)
    language=source.string_payload(6110)[:-1].decode('ascii')
    manifest=dict(images=[dict(file=name,width=8,height=1,format=0,bytes=len(marker),
        sha256=t.sha256(marker),raw_sha256=t.sha256(marker))],clips=[],localized_text={language:{}})
    result,extension=patch_pack(converted,manifest,work,base_aux=contract.BASE_AUX,
        auxiliary=contract.AUXILIARY,input_version=45,version=45,texts=[],base_clips=1783,
        base_strings=len(source.strings))
    complete=validate_retirement_intermediate(result)
    target=coalesce.view(result)
    prior_marker,current=t.TileReader(coalesce.view(converted)),t.TileReader(target)
    t.require(len(prior_marker.groups)==len(current.groups) and all(a[1:]==b[1:] and
        prior_marker.pack.block(a[0]-32,a[1]+32)==current.pack.block(b[0]-32,b[1]+32)
        for a,b in zip(prior_marker.groups,current.groups)),
        'The profile marker changed an animation/index stream')
    t.require(source.clips==target.clips and source.cards==target.cards and
              source.aux==target.aux[:contract.BASE_AUX] and
              all(a[1:3]==b[1:3] for a,b in zip(source.images,target.images)),
              'Completed retirement changed an old identity, clock, or dimension')
    t.require(len(result)<len(data),'Complete guarded retirement must actually reduce physical bytes')
    receipt=dict(complete=True,input_bytes=len(data),output_bytes=len(result),saved_bytes=len(data)-len(result),
        input_sha256=t.sha256(data),output_sha256=t.sha256(result),plan_sha256=contract.PLAN['plan_sha256'],
        auxiliary_count=contract.AUXILIARY,retired_images=retired,retired_clips=contract.PLAN['retired_clips'],
        active_images_exact=len(source.images)-len(ids),new_metadata_images=1,
        active_pixels_alpha_geometry_and_clip_clocks_unchanged=True,
        codec_and_source_lzma_profiles_preserved=True,roots=roots,
        directory=directory,compaction=compacted,changed_groups=changed,
        extension=extension,intermediate_guard=complete,final_release_ready=False)
    return result,receipt


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--work',type=Path,required=True);p.add_argument('--workers',type=int,default=2)
    args=p.parse_args();t.require(1<=args.workers<=2,'Use one or two workers')
    source=t.private_path(args.input,'Private unretired input');output=t.private_path(args.output,'Private retired output')
    work=t.private_path(args.work,'Private retirement work');t.require(source!=output,'Never overwrite the source pack')
    started=time.monotonic();result,receipt=convert(source.read_bytes(),work,args.workers)
    t.atomic_bytes(output,result);receipt['elapsed_seconds']=time.monotonic()-started
    t.atomic_bytes(work/'retirement-receipt.json',json.dumps(receipt,indent=2).encode()+b'\n')
    print(json.dumps({k:receipt[k] for k in ('complete','input_bytes','output_bytes','saved_bytes',
        'output_sha256','active_images_exact','auxiliary_count','elapsed_seconds')}),flush=True)


if __name__=='__main__':main()
