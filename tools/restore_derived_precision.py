#!/usr/bin/env python3
"""Restore color in later artwork derived from historically quantized frames.

Merchant static/moving planes are factored again from the complete original
RGB565/alpha8 frames, so restoring a moving frame cannot duplicate its old
static background. Later independently registered legacy enemy clips are
sampled from the pinned original Spine data at their current exact geometry.
Existing registered UI20 pieces recover their original RGBA source caches.
All results are private raw spools for a single final exact tile import.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image

import content_tiles as t
from actor_animation_ui22 import input_identity
from build_world import job_state
from framed_animation_ui20_assets import CLIPS as FRAMED_CLIPS
from merchant_ui14_assets import (FIRST_CLIP,CLIP_KEYS,BASE_AUXILIARY,
    pixel_planes,rgba_image,separated_planes,verify_recomposition)
from pck import Pack
from restore_animation_precision import changed_owners,matching_precision,read_image
from sparse_image import encode as sparse_encode
from ui12_resources import view
from verify_framed_animation_ui20 import table_frames
from world_enemy_assets import SpineSampler,_compose
from world_enemy_profile import plan as enemy_plan
from world_raster import raster


def emit(event,**values):
    print(json.dumps(dict(event=event,**values)),flush=True)


class Spool:
    def __init__(self,path):
        self.path=path;self.handle=path.open('wb');self.rows=[];self.offset=0;self.digest=hashlib.sha256()

    def add(self,image,decoded,**provenance):
        width,height,fmt,raw=decoded;t.validate_raw(raw,width,height,fmt)
        self.handle.write(raw);self.digest.update(raw)
        row=dict(image=image,width=width,height=height,format=fmt,offset=self.offset,
            bytes=len(raw),raw_sha256=t.sha256(raw),**provenance)
        self.rows.append(row);self.offset+=len(raw);return row

    def finish(self):
        self.handle.close()
        return dict(file=str(self.path.resolve()),bytes=self.offset,sha256=self.digest.hexdigest(),rows=self.rows)


def sparse_planes(planes):
    image=rgba_image(*planes)
    return image.width,image.height,4,sparse_encode(image)


def merchant(original,current,work):
    old=t.TileReader(original);now=t.TileReader(current);spool=Spool(work/'merchant-original-color.raw');reports=[]
    for index,key in enumerate(CLIP_KEYS):
        clip=FIRST_CLIP+index
        t.require(original.clips[clip]==current.clips[clip],
                  'Merchant clock/registration changed after original source')
        first,count,*_=current.clips[clip]
        frames=[pixel_planes(read_image(original,old,image)) for image in range(first,first+count)]
        static,moving,fixed=separated_planes(frames)
        static_id=current.aux[BASE_AUXILIARY+index]
        for image,planes in [(static_id,static),*zip(range(first,first+count),moving)]:
            decoded=sparse_planes(planes)
            t.require(decoded[:2]==current.images[image][1:3],
                      'Merchant restoration would change its current layout')
            spool.add(image,decoded,source='UI10-original-disjoint-merchant-factor',clip=clip)
        for source,delta in zip(frames,moving):verify_recomposition(source,static,delta)
        # Independently check the old split really belongs to the same
        # original source under its historical map; this rejects a later
        # artwork edit even when its dimensions and clip tuple are identical.
        current_static=pixel_planes(read_image(current,now,static_id));profiles=[]
        for frame,expected in enumerate(frames):
            current_delta=pixel_planes(read_image(current,now,first+frame))
            ca=np.maximum(current_static[1],current_delta[1])
            cc=np.where(current_static[1]!=0,current_static[0],current_delta[0])
            t.require(not np.any((current_static[1]!=0)&(current_delta[1]!=0)),
                      'Current merchant planes unexpectedly overlap')
            profile=matching_precision(sparse_planes(expected),sparse_planes((cc,ca)))
            # Old alpha quantization can create zero-alpha samples without
            # changing the old span list. Compare dense planes as well.
            if profile is None:
                from quantize_animation_resources import lookup
                profile=next((p for p in (1,2,4,8) if
                    np.array_equal(lookup(p)[1][expected[1]],ca) and
                    np.all((lookup(p)[0][expected[0]]==cc)|(ca==0))),None)
            t.require(profile is not None,'Current merchant is not the same original source frame')
            profiles.append(profile)
        reports.append(dict(clip=clip,key=key,frames=count,static_image=static_id,invariant_pixels=fixed,
                            current_original_precision_profiles=sorted(set(profiles))))
        emit('merchant_original_color',**reports[-1])
    result=spool.finish();result.update(clips=reports,frames=sum(r['frames'] for r in reports),
        exact_recomposition=True,palette_quantization=False,alpha_quantization=False)
    t.atomic_bytes(work/'merchant-original-color.json',json.dumps(result,indent=2).encode()+b'\n')
    return result


def legacy_enemies(original,current,pack,sampler,runtime,actor_jobs,work):
    plan=enemy_plan();models={m['model']:m for m in plan['models']}
    active={j['clip'] for j in actor_jobs};changed=changed_owners(original,current)
    # UI15 recycled every old Vantom whole-frame slot into direct-source
    # layers, mesh words and texture pages. The retained old clip directory is
    # a semantic map, not the owner of a picture sequence in this format.
    vantom=[job for job in plan['jobs'] if job['model']=='Vantom']
    preserved=preserved_vantom_catalog(current,vantom)
    jobs=[job for job in plan['jobs'] if job['id'] in changed-active and job['model']!='Vantom']
    identity=input_identity(pack,runtime)
    identity=t.sha256(identity.encode()+Path(__file__).read_bytes());reports=[]
    for job in jobs:
        clip=job['id'];first,count,ms,px,py,units=current.clips[clip]
        t.require(units>0 and count==job['frames'],'Legacy source registration is ambiguous')
        shapes={current.images[i][1:3] for i in range(first,first+count)}
        t.require(len(shapes)==1,'Legacy independently registered clip changes canvas')
        width,height=next(iter(shapes))
        signature=t.sha256((identity+json.dumps(job,sort_keys=True)+
            repr(current.clips[clip])+repr((width,height))).encode())
        folder=work/'legacy-enemies'/f'{clip:04d}-{signature[:16]}';folder.mkdir(parents=True,exist_ok=True)
        receipt=folder/'source.json';saved=json.loads(receipt.read_text()) if receipt.is_file() else None
        if not (saved and saved.get('signature')==signature and
                Path(saved['file']).is_file() and Path(saved['file']).stat().st_size==saved['bytes']):
            draws=_compose(sampler,models[job['model']],job['clip'],
                [i*job['duration']/count for i in range(count)],job['skins'],job_state(job))
            spool=Spool(folder/'source.raw')
            for frame,draw in enumerate(draws):
                picture=raster(draw,1024/units,(px,py),(width,height))
                spool.add(first+frame,(width,height,4,sparse_encode(picture)),
                    source='official-Spine-current-registered-canvas',clip=clip,frame=frame)
            saved=spool.finish();saved.update(signature=signature,clip=clip,key=job['key'],
                frames=count,frame_ms=ms,units=units,pivot=[px,py],geometry_unchanged=True,
                palette_quantization=False,alpha_quantization=False)
            t.require(len(saved['rows'])==count,'Legacy source sampler lost a frame')
            t.atomic_bytes(receipt,json.dumps(saved,indent=2).encode()+b'\n')
        reports.append(saved)
        emit('legacy_enemy_original_color',complete=len(reports),total=len(jobs),clip=clip,
             key=job['key'],frames=count)
    result=dict(clips=reports,frames=sum(r['frames'] for r in reports),preserved_vantom=preserved,
        palette_quantization=False,alpha_quantization=False)
    t.atomic_bytes(work/'legacy-enemies-original-color.json',json.dumps(result,indent=2).encode()+b'\n')
    return result


def preserved_vantom_catalog(current,jobs):
    """Resolve the live UI15 graph before interpreting any old frame slots."""
    t.require(len(current.aux)>=5404 and len(jobs)==232,'Missing completed Vantom source replacement')
    reader=t.TileReader(current);referenced=set();parameters=[];frames=layers=0
    for auxiliary in range(5288,5404):
        image=current.aux[auxiliary];width,height,fmt,raw=read_image(current,reader,image)
        t.require(fmt==0,'Vantom parameters are exact words, not a color raster')
        words=struct.unpack('<'+'H'*(len(raw)//2),raw)
        t.require(words[0]==0x5654 and words[1]==3 and words[8:10]==(7,12),
                  'Vantom live resource version changed')
        count,used=words[2],words[10]
        t.require(0<count<=256 and 12+count*5<=used<=len(words),'Unbounded Vantom frame table')
        for frame in range(count):
            start,n=words[12+frame*5:14+frame*5]
            t.require(n<=64 and 0<=start<=used and n*7<=used-start,'Vantom layer table escapes resource')
            for layer in range(n):
                row=words[start+layer*7:start+(layer+1)*7];mode=row[3]
                t.require(mode<=7,'Unrecognized Vantom live layer')
                ids=[row[0]]
                if mode&3==3:ids.append(row[1])
                ids.extend(i for i in row[5:7] if i!=65535)
                t.require(all(i<len(current.images) for i in ids),'Vantom live image ID escapes catalog')
                referenced.update(ids);layers+=1
        frames+=count;parameters.append(dict(image=image,raw_sha256=t.sha256(raw)))
    reused={i for job in jobs for i in range(current.clips[job['id']][0],
                                            sum(current.clips[job['id']][:2]))}
    unchanged=[]
    for image in sorted(reused|referenced):
        width,height,fmt,raw=read_image(current,reader,image)
        unchanged.append(dict(image=image,width=width,height=height,format=fmt,
                              raw_sha256=t.sha256(raw),live=image in referenced))
    root=Path(__file__).resolve().parent
    provenance={name:t.sha256((root/name).read_bytes()) for name in
        ('vantom_ui15_assets.py','vantom_ui15_texture_pages.py','patch_feedback_assets.py','sparse_image.py')}
    return dict(retained_clip_ids=[job['id'] for job in jobs],reused_old_frame_slots=len(reused),
        live_referenced_images=len(referenced),parameters=parameters,frames=frames,layers=layers,
        preserved_images=unchanged,source_generator_sha256=provenance,
        source_pipeline='original PCK RGBA raster/texture -> RGB565 plus alpha8; exact mesh/parameter words',
        new_color_mapping=False,new_rasterization=False,
        old_whole_frame_ids_are_not_current_raster_sequences=True)


def previous_registered(current,cache,work,active_images):
    spool=Spool(work/'previous-registered-original-color.raw');reports=[]
    for index,clip in enumerate((*FRAMED_CLIPS,-2)):
        matches=[]
        for file in cache.glob('*/frames.json'):
            record=json.loads(file.read_text())
            if record.get('clip')==clip:matches.append((file.parent,record))
        t.require(len(matches)==1,'Ambiguous old registered source cache')
        folder,record=matches[0];frames=table_frames(current,5409+index)
        t.require(len(frames)==len(record['parts']),'Old registered sample clock changed')
        count=0
        for frame,(parts,saved) in enumerate(zip(frames,record['parts'])):
            t.require(len(parts)==len(saved),'Old registered partition changed')
            for (image,x,y),piece in zip(parts,saved):
                if image in active_images:continue
                picture=Image.open(folder/piece['file']).convert('RGBA')
                t.require((x,y)==(piece['x'],piece['y']) and picture.size==current.images[image][1:3],
                          'Old registered source cache changed geometry')
                spool.add(image,(picture.width,picture.height,4,sparse_encode(picture)),
                    source='original-RGBA-registered-piece-cache',clip=clip,frame=frame,
                    source_signature=record['signature'],png_sha256=t.sha256((folder/piece['file']).read_bytes()))
                count+=1
        reports.append(dict(clip=clip,pieces=count))
    result=spool.finish();result['clips']=reports
    t.atomic_bytes(work/'previous-registered-original-color.json',json.dumps(result,indent=2).encode()+b'\n')
    emit('previous_registered_original_color',images=len(result['rows']))
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('original','current','actor-jobs','pck','runtime','work','ui20-cache'):
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--dotnet',default='dotnet');p.add_argument('--stage',choices=('all','merchant','legacy','registered'),default='all')
    args=p.parse_args()
    for path in (args.original,args.current,args.actor_jobs,args.pck,args.runtime,args.work,args.ui20_cache):
        t.private_path(path,'Private original color provenance')
    args.work.mkdir(parents=True,exist_ok=True)
    original,current=view(args.original.read_bytes()),view(args.current.read_bytes())
    jobs=json.loads(args.actor_jobs.read_text())['jobs_manifest']
    if args.stage in ('all','merchant'):merchant(original,current,args.work)
    if args.stage in ('all','registered'):
        active={i for job in jobs if job['clip']>=0 for i in
                range(current.clips[job['clip']][0],sum(current.clips[job['clip']][:2]))}
        previous_registered(current,args.ui20_cache,args.work,active)
    if args.stage in ('all','legacy'):
        pack=Pack(args.pck)
        try:
            with SpineSampler(pack,runtime=args.runtime,dotnet=args.dotnet,work_dir=args.work/'adapter') as sampler:
                legacy_enemies(original,current,pack,sampler,args.runtime,jobs,args.work)
        finally:pack.close()


if __name__=='__main__':main()
