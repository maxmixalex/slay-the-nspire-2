#!/usr/bin/env python3
"""Append only original Kaiser pixels missing from the Bestiary world crop.

All sampling uses the owner's PCK and reviewed official Spine runtime. The
existing world frame, its sampling clock and its decoded pixels stay intact.
New frames contain only the part of that same skeleton raster outside the
world crop which is visible after the authored Bestiary translation. They
are projected to native pixels offline; the runtime adds one sparse image.
"""
from __future__ import annotations

import argparse
import json
import lzma
import mmap
from pathlib import Path

import numpy as np
from PIL import Image

import content_tiles as tiles
from group_ui7_hands import RowImages
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
from pck import Pack
from repack_retained import DESCRIPTOR, plan_images
from world_enemy_assets import SpineSampler, _compose
from world_enemy_profile import plan
from world_raster import raster

ROOT=Path(__file__).resolve().parents[1]
BASE_VERSION,VERSION=33,34
BASE_CLIPS,CLIPS,AUXILIARY,STRINGS=1765,1783,2061,8704
SOURCES=(179,203,227,228,263,286,298,310,
         842,866,890,913,926,949,973,1009,1021,1034)
WORLD_SIZE=(320,218)
WORLD_SCALE=109/720
WORLD_ORIGIN=np.array([2773/360,18203/540])
TARGET_SIZE=(302,206)
TOP=23


def jobs():
    by_id={row['id']:row for row in plan()['jobs']}
    rows=[by_id[index] for index in SOURCES]
    tiles.require(len(rows)==18 and sum(row['frames'] for row in rows)==344,
                  'Bestiary source frame catalogue changed')
    tiles.require(all(row['model'] in ('Crusher','Rocket') and row['skin']==0 and
                      not any(row['visual_state']['facts'].values()) for row in rows),
                  'Bestiary source catalogue contains another actor/context')
    return rows


def source_checks(pack):
    scene=pack.read('scenes/screens/bestiary/bestiary_layout_kaiser_crab.tscn').decode()
    for fragment in ('position = Vector2(265, 288)','scale = Vector2(0.75, 0.75)'):
        tiles.require(fragment in scene,'Pinned Kaiser Bestiary registration changed')


def coordinates(side):
    """Exact inverse of Canvas::picture's existing320x218 ->302x206 path."""
    tiles.require(side in ('Crusher','Rocket'),'Unknown shared Bestiary actor')
    left=173 if side=='Crusher' else -114
    xx=(np.arange(320,dtype=np.int32)-left)*WORLD_SIZE[0]//TARGET_SIZE[0]
    yy=(np.arange(240,dtype=np.int32)-TOP)*WORLD_SIZE[1]//TARGET_SIZE[1]
    owned=((xx>=0)&(xx<WORLD_SIZE[0]))[None,:]&((yy>=0)&(yy<WORLD_SIZE[1]))[:,None]
    return xx,yy,owned


def missing_image(draw,side):
    """Keep every visible missing sample and no sample owned by old artwork."""
    xx,yy,owned=coordinates(side)
    offset=np.array([int(xx[0]),int(yy[0])])
    size=(int(xx[-1]-xx[0]+1),int(yy[-1]-yy[0]+1))
    expanded=raster(draw,WORLD_SCALE,WORLD_ORIGIN-offset,size)
    complete=np.asarray(expanded)[(yy-yy[0])[:,None],(xx-xx[0])[None,:]].copy()
    complete[owned]=0
    tiles.require(not np.any(complete[owned]),'Bestiary supplement overlaps historical world pixels')
    return Image.fromarray(complete)


def frame_times(job,count):
    tiles.require(count==job['frames'] and 1<=count<=256,'Historical source frame count changed')
    # Keep the original world sampler clock, including its current endpoint
    # convention. Adding or dropping a terminal pose here would desynchronize
    # the supplemental pixels from the retained world frame.
    return [index*job['duration']/count for index in range(count)]


def normal_geometry(source):
    result=[]
    for job in jobs():
        first,count,ms,px,py,reserved=source.clips[job['id']]
        tiles.require(count==job['frames'] and (px,py)==(8,34) and reserved==0,
                      'Historical shared skeleton registration changed')
        shapes=[tuple(source.images[index][1:3]) for index in range(first,first+count)]
        tiles.require(all(shape==WORLD_SIZE for shape in shapes),'Historical Kaiser image crop changed')
        result.append((job['id'],count,ms,px,py))
    return result


def identity(runtime):
    names=('bestiary_kaiser_assets.py','world_enemy_assets.py','world_enemy_profile.py',
           'world_raster.py','animation.py','SpineSampleAdapter.cs','patch_feedback_assets.py')
    return tiles.sha256(b''.join((ROOT/'tools'/name).read_bytes() for name in names)+
                        (ROOT/'config/enemy-visuals-0.17.json').read_bytes()+Path(runtime).read_bytes())


def build_art(pck_path,source,cache,*,runtime,dotnet='dotnet'):
    cache=Path(cache);cache.mkdir(parents=True,exist_ok=True)
    fingerprint=identity(runtime);geometry=normal_geometry(source)
    geometry_hash=tiles.sha256(tiles.canonical(geometry));catalog=cache/'bestiary-kaiser-assets.json'
    if catalog.is_file():
        saved=json.loads(catalog.read_text())
        if saved.get('converter_sha256')==fingerprint and saved.get('source_geometry_sha256')==geometry_hash:
            tiles.require(len(saved['clips'])==18 and len(saved['images'])==344,'Cached Kaiser catalogue changed')
            for row in saved['images']:
                raw=(cache/row['file']).read_bytes()
                tiles.require(len(raw)==row['bytes'] and tiles.sha256(raw)==row['sha256'],'Cached Kaiser pixels changed')
            return saved
    models={row['model']:row for row in plan()['models']};images=[];clips=[];reports=[]
    pack=Pack(pck_path)
    try:
        source_checks(pack)
        with SpineSampler(pack,runtime=runtime,dotnet=dotnet,work_dir=cache/'spine-adapter') as sampler:
            for job,registration in zip(jobs(),geometry):
                original,count,ms,_px,_py=registration
                samples=_compose(sampler,models[job['model']],job['clip'],frame_times(job,count),job['skins'],job['visual_state'])
                tiles.require(len(samples)==count,'Sampler omitted a source pose')
                frames=[missing_image(draw,job['model']) for draw in samples]
                bounds=[image.getbbox() for image in frames if image.getbbox()]
                box=(min(row[0] for row in bounds),min(row[1] for row in bounds),
                     max(row[2] for row in bounds),max(row[3] for row in bounds)) if bounds else (0,0,1,1)
                tiles.require(0<=box[0]<box[2]<=320 and 0<=box[1]<box[3]<=240,'Unbounded Kaiser patch')
                first=len(images)
                for index,image in enumerate(frames):
                    part=image.crop(box)
                    row=write_image(cache,f'kaiser-{original:04d}-{index:03d}',part,sparse=True)
                    images.append(row)
                    if index in (0,len(frames)//2,len(frames)-1):
                        image.save(cache/f'kaiser-{original:04d}-{index:03d}.png')
                clips.append(dict(first=first,count=count,frame_ms=ms,pivot=[-box[0],-box[1]],
                                  sprite_units=original+1,key='bestiary_patch.'+job['key']))
                report=dict(source_directory=original,appended_directory=BASE_CLIPS+len(clips)-1,
                            model=job['model'],clip=job['clip'],move=job['move'],frames=count,
                            frame_ms=ms,bbox=list(box),maximum_opaque_pixels=max(
                                int(np.count_nonzero(np.asarray(frame)[:,:,3])) for frame in frames),
                            unchanged_source_clock=True,old_world_pixels_excluded=True)
                reports.append(report);print(json.dumps(report),flush=True)
        result=dict(schema=1,reference='v0.111.0 / 41cef1ea',converter_sha256=fingerprint,
                    source_geometry_sha256=geometry_hash,output_version=VERSION,animation_count=CLIPS,
                    auxiliary_count=AUXILIARY,images=images,clips=clips,auxiliary_rows=[],
                    localized_text={'eng':{},'zhs':{}},reports=reports,runtime_added_pixel_buffers=0)
        tiles.atomic_bytes(catalog,json.dumps(result,indent=2).encode()+b'\n')
        return result
    finally:pack.close()


def group_art(rows,cache,group_bytes=512*1024):
    """Global exact aliases; bounded adjacent-frame RGR1 using existing reader."""
    payloads=[(cache/row['file']).read_bytes() for row in rows]
    for row,raw in zip(rows,payloads):
        tiles.require(len(raw)==row['bytes'] and tiles.sha256(raw)==row['sha256'],'Kaiser frame changed before grouping')
    source=RowImages(rows,payloads)
    decoded=[len(source.image_raw(i)[3]) for i in range(len(rows))]
    batches=plan_images(source,group_bytes=group_bytes,fast_images='none')
    payload=bytearray();entries=[None]*len(rows);groups=[]
    def append(raw):
        payload.extend(bytes(len(payload)&1));at=len(payload);payload.extend(raw);return at
    for batch in batches:
        raw=b''.join(record.raw for record in batch.records)
        candidate=batch.encoded;context=None
        for lc,lp,pb in ((3,0,2),(3,0,0),(4,0,1)):
            encoded=lzma.compress(raw,format=lzma.FORMAT_RAW,filters=[dict(id=lzma.FILTER_LZMA2,
                dict_size=tiles.GROUP_LIMIT,lc=lc,lp=lp,pb=pb,nice_len=273,depth=512,
                mode=lzma.MODE_NORMAL,mf=lzma.MF_BT4)])
            tiles.require(tiles.decode_lzma2(encoded,len(raw))==raw,'Kaiser retained group roundtrip differs')
            cost=((tiles.GROUP.size+len(encoded)+1)&~1)+len(batch.records)*DESCRIPTOR.size
            if cost<batch.previous_size and (candidate is None or len(encoded)<len(candidate)):
                candidate=encoded;context=[lc,lp,pb]
        if candidate is None:
            for record in batch.records:
                at=append(record.payload)
                for image in record.ids:entries[image]=(at,record.stored_format,len(record.payload))
            continue
        group=tiles.GROUP.pack(b'RGR1',len(raw),len(candidate),14,len(batch.records),0,0,0)+candidate
        at=append(group);raw_at=0
        for record in batch.records:
            payload.extend(bytes(len(payload)&1));descriptor=len(payload)
            payload.extend(DESCRIPTOR.pack(at-descriptor,len(group),raw_at,(record.fmt<<24)|len(record.raw)))
            for image in record.ids:entries[image]=(descriptor,7,DESCRIPTOR.size)
            raw_at+=len(record.raw)
        groups.append(dict(raw_bytes=len(raw),stored_bytes=len(group),images=sum(len(r.ids) for r in batch.records),
                           distinct_payloads=len(batch.records),context=context))
    payload.extend(bytes(len(payload)&1));old_bytes=sum((len(row)+1)&~1 for row in payloads)
    tiles.require(all(row is not None for row in entries) and len(payload)<=old_bytes,'Kaiser grouping enlarged or omitted a frame')
    report=dict(retained_groups=len(groups),max_group_raw_bytes=max((g['raw_bytes'] for g in groups),default=0),
                target_group_raw_bytes=group_bytes,groups=groups,
                exact_frame_aliases=len(rows)-sum(len(batch.records) for batch in batches),
                original_payload_bytes=old_bytes,stored_payload_bytes=len(payload),
                maximum_decoded_frame_bytes=max(decoded),unchanged_decoded_pixels=True,
                runtime_buffer_growth=0,unreachable_payload_bytes=0)
    return dict(payload=bytes(payload),entries=entries,decoded_sizes=decoded,report=report)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for option in ('pck','input-eng','input-zhs','runtime','cache-dir','output-dir'):
        parser.add_argument('--'+option,type=Path,required=True)
    parser.add_argument('--dotnet',default='dotnet');parser.add_argument('--measure-only',action='store_true')
    args=parser.parse_args()
    sources={lang:tiles.private_path(getattr(args,'input_'+lang),'Previous private pack') for lang in ('eng','zhs')}
    cache=tiles.private_path(args.cache_dir,'Kaiser cache');output=tiles.private_path(args.output_dir,'Kaiser output')
    with sources['eng'].open('rb') as handle,mmap.mmap(handle.fileno(),0,access=mmap.ACCESS_READ) as data:
        source=tiles.PackView(data,AUXILIARY,versions=(BASE_VERSION,),profile=(558,STRINGS,BASE_CLIPS,AUXILIARY))
        art=build_art(tiles.private_path(args.pck,'Original PCK'),source,cache,
                      runtime=tiles.private_path(args.runtime,'Official Spine runtime'),dotnet=args.dotnet)
    grouped=group_art(art['images'],cache);receipts={}
    if not args.measure_only:
        for language,source in sources.items():
            target=output/f'ui9-{language}.nsp'
            tiles.require(target.resolve()!=source.resolve(),'The supplemental pack must preserve its input')
            data,receipt=patch_pack(source.read_bytes(),art,cache,base_aux=AUXILIARY,auxiliary=AUXILIARY,
                input_version=BASE_VERSION,version=VERSION,texts=(),base_clips=BASE_CLIPS,base_strings=STRINGS,appended_art=grouped)
            tiles.atomic_bytes(target,data);receipts[language]=receipt
    # The split common file contains artwork only. Both locale files retain
    # their own directories/cost tables, so installation accounting charges
    # this prefix twice rather than calling it shared artwork.
    prefix_growth=len(art['images'])*17+len(art['clips'])*16
    receipt=dict(languages=receipts,grouping=grouped['report'],frames=len(art['images']),clips=len(art['clips']),
                 image_directory_bytes=len(art['images'])*16,clip_directory_bytes=len(art['clips'])*16,
                 appended_common_artwork_bytes=len(grouped['payload']),added_prefix_bytes_per_locale=prefix_growth,
                 bilingual_install_growth_bytes=len(grouped['payload'])+2*prefix_growth,
                 unchanged_world_images=True,unchanged_source_clocks=True)
    tiles.atomic_bytes(output/'bestiary-kaiser-conversion.json',json.dumps(receipt,indent=2).encode()+b'\n')
    print(json.dumps(receipt),flush=True)


if __name__=='__main__':main()
