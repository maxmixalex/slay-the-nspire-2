#!/usr/bin/env python3
"""Rebuild actor frames at their displayed density with exact RGB565/alpha.

All source pixels and generated files stay private. Stable clip IDs, clocks,
sample counts and gameplay routing remain unchanged. A resident auxiliary
index selects cropped registered pieces, including exact terminal death poses.
There is no palette reduction, alpha quantization or moving-union shrink.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct

import numpy as np
from PIL import Image

import content_tiles as t
from character_death_assets import (CHARACTERS, material_overrides,
                                    sample_bounded, times)
from character_death_particles import (particle_bounds, render_particles,
                                      sample_particles)
from framed_animation_ui20_assets import (CLIPS as PREVIOUS_ENEMY_CLIPS,
    MAX_LAYERS, NATIVE_DENSITY, cached_frames, pixel_row, word_row)
from measure_combat_registration import measure_players
from patch_frontend_assets import patch_pack
from patch_phobia_assets import grouped_art, original_jobs as phobia_jobs
from pck import Pack
from ui12_resources import replace_images, view
from world_enemy_assets import SpineSampler, _compose
from world_enemy_profile import plan as enemy_plan
from world_raster import raster

ROOT=Path(__file__).resolve().parents[1]
BASE_AUX,CLIP_COUNT=5422,1783
AUXILIARY=BASE_AUX+CLIP_COUNT+1
PLAYER_CLIPS=(('ironclad',(0,1,2,3),('idle_loop','attack','hurt','cast')),
    ('silent',(11,12,13,14,15),('idle_loop','attack','hurt','cast','shiv')),
    ('defect',(17,18,19,20),('idle_loop','attack','hurt','cast')),
    ('regent',(22,23,24,25),('idle_loop','attack','hurt','cast')),
    ('necrobinder',(27,28,29,30),('idle_loop','attack','hurt','cast')),
    ('osty',(32,33,34,35,36),('idle_loop','attack','hurt','die','revive')))


def emit(event,**values):
    print(json.dumps(dict(event=event,**values)),flush=True)


def raster_bounds(points,scale):
    points=np.asarray(points,dtype=float)
    t.require(points.ndim==2 and points.shape[1]==2 and len(points) and
              np.isfinite(points).all() and math.isfinite(scale) and scale>0,
              'Invalid native actor raster bounds')
    low,high=points.min(axis=0),points.max(axis=0)
    left,top=math.floor(low[0]*scale)-2,math.floor(-high[1]*scale)-2
    right,bottom=math.ceil(high[0]*scale)+2,math.ceil(-low[1]*scale)+2
    t.require(0<right-left<=2560 and 0<bottom-top<=1920,'Unexpected actor frame extent')
    return left,top,right-left,bottom-top


def source_frame(draw,scale):
    points=[layer[0] for layer in draw if len(layer[0])]
    if not points:return Image.new('RGBA',(1,1)),0,0
    left,top,width,height=raster_bounds(np.concatenate(points),scale)
    return raster(draw,scale,(-left,-top),(width,height)),left,top


def particle_frame(sample,scale):
    if not sample:return Image.new('RGBA',(1,1)),0,0
    left,top,width,height=raster_bounds(particle_bounds([sample]),scale)
    return render_particles(sample,scale,np.array([-left,-top]),(width,height)),left,top


def exact_parts(picture,left,top):
    """Crop one solved raster; independent triangle solves can round differently."""
    parts=[]
    for y in range(0,picture.height,240):
        for x in range(0,picture.width,320):
            part=picture.crop((x,y,min(x+320,picture.width),min(y+240,picture.height)))
            bounds=part.getchannel('A').getbbox()
            if bounds:
                px,py=left+x+bounds[0],top+y+bounds[1]
                t.require(-32768<=px<=32767 and -32768<=py<=32767,'Actor registration overflows')
                parts.append((part.crop(bounds),px,py))
    t.require(len(parts)<=MAX_LAYERS,'Actor exceeds bounded registered layer list')
    return parts or [(Image.new('RGBA',(1,1)),0,0)]


def input_identity(pack,runtime):
    digest=hashlib.sha256(Path(runtime).read_bytes())
    for name in ('actor_animation_ui22.py','framed_animation_ui20_assets.py',
                 'character_death_assets.py','character_death_particles.py',
                 'measure_combat_registration.py','world_enemy_assets.py',
                 'world_raster.py','SpineSampleAdapter.cs','animation.py'):
        digest.update((ROOT/'tools'/name).read_bytes())
    for name,entry in sorted(pack.entries.items()):
        digest.update(name.encode());digest.update(entry.md5)
    return digest.hexdigest()


def resource(name,merchant=False):
    family='monsters' if name=='osty' else 'characters'
    return dict(skeleton_path=f'res://animations/{family}/{name}/{name}.skel',
        atlas_path=f'res://animations/merchant/{name}/{name}_shop.atlas' if merchant else
                   f'res://animations/{family}/{name}/{name}.atlas')


def jobs(pack,source,sampler,enemy_limits=None):
    """Keep original sample schedules; never fit a complete moving union."""
    from build_world import job_state
    registrations={row['model']:row for row in measure_players(pack,source)}
    result=[]
    for name,ids,animations in PLAYER_CLIPS:
        registration=registrations[name]
        # Osty's HP growth reaches 2x; BigMushroom applies 1.5x to players.
        maximum=2 if name=='osty' else 1.5
        scale=max(registration['baked_scale'],registration['source_scale']*NATIVE_DENSITY*maximum)
        res=resource(name);metadata=sampler.metadata(res)
        for clip,animation in zip(ids,animations):
            old=source.clips[clip];duration=next(c['duration'] for c in metadata['clips'] if c['name']==animation)
            result.append(dict(kind='player',clip=clip,key=name+'.'+animation,resource=res,
                animation=animation,loop=animation.endswith('_loop'),
                times=[i*duration/old[1] for i in range(old[1])],frames=old[1],frame_ms=old[2],
                scale=scale,units=round(1024*registration['source_scale']/scale),
                reference=registration['sprite_units'],maximum_visual_scale=maximum,
                materials=material_overrides(pack,name,False)))
    for merchant in (False,True):
        for character,name in enumerate(CHARACTERS):
            clip=1753+int(merchant)*5+character;old=source.clips[clip]
            res=resource(name,merchant);metadata=sampler.metadata(res)
            animation=next(c for c in metadata['clips'] if c['name']=='die')
            sample_times=times(animation['duration'])
            t.require(len(sample_times)==old[1],'Original terminal death samples changed')
            registration=registrations[name]
            scene_scale=(.23 if name=='defect' else .47) if merchant else registration['source_scale']
            projection=218/1080 if merchant else NATIVE_DENSITY*1.5
            scale=max(registration['baked_scale'],scene_scale*projection)
            result.append(dict(kind='death',clip=clip,key=('merchant.' if merchant else '')+name+'.die',
                resource=res,animation='die',loop=False,times=sample_times,frames=old[1],frame_ms=old[2],
                scale=scale,units=round(1024*scene_scale/scale),reference=old[5],
                exact_terminal_sample=True,materials=material_overrides(pack,name,merchant)))
    for name,clip,duration in (('defect',1763,None),('regent',1764,5.)):
        registration=registrations[name];res=resource(name)
        animation=next(c for c in sampler.metadata(res)['clips'] if c['name']=='die')
        duration=animation['duration'] if duration is None else duration
        old=source.clips[clip]
        scale=max(registration['baked_scale'],registration['source_scale']*NATIVE_DENSITY*1.5)
        result.append(dict(kind='particles',clip=clip,key=name+'.die_particles',resource=res,
            animation='die',loop=False,times=times(duration,60),frames=old[1],frame_ms=old[2],
            scale=scale,units=round(1024*registration['source_scale']/scale),reference=old[5],
            events=animation['events'],character=name,blend='add',exact_terminal_sample=True))
    registration=registrations['osty'];res=resource('osty')
    duration=next(c['duration'] for c in sampler.metadata(res)['clips'] if c['name']=='dead_loop')
    scale=max(registration['baked_scale'],registration['source_scale']*NATIVE_DENSITY*2)
    count=max(1,math.ceil(duration*12))
    result.append(dict(kind='player',clip=-2,key='osty.dead_loop',resource=res,animation='dead_loop',
        loop=True,times=[i*duration/count for i in range(count)],frames=count,
        frame_ms=max(1,round(duration*1000/count)),scale=scale,
        units=round(1024*registration['source_scale']/scale),reference=registration['sprite_units'],
        maximum_visual_scale=2,materials=material_overrides(pack,'osty',False)))
    plan=enemy_plan();models={row['model']:row for row in plan['models']}
    baseline={name:int(units) for units,name in re.findall(r'(\d+),\s*//\s*(\w+)',
        (ROOT/'include/combat_sprite_units.inc').read_text())}
    bounds={name:max(int(w),int(h)) for w,h,name in re.findall(r'\{(\d+),(\d+)\},\s*//\s*(\w+)',
        (ROOT/'include/combat_creature_bounds.inc').read_text())}
    limits=enemy_limits or {}
    for original in (*plan['jobs'],*phobia_jobs()):
        clip=original['id'];model=models[original['model']];old=source.clips[clip]
        if model.get('shared_group') or original['model']=='Vantom':continue
        reference=old[5] or baseline[original['model']]
        small=max(0,min(1024,(250-bounds[original['model']])*1024//150))
        default=(1024+102+51*small//1024)/1024
        if original['model']=='PhantasmalGardener':default=1+math.log(10000)/10
        if original['model']=='TestSubject':default=1.2
        maximum=float(limits.get(original['model'],default))
        t.require(1<=maximum<=4,'Unbounded enemy display scale')
        # Include the earlier registered clips and phobia variants, whose
        # zero sprite_units intentionally select the model-wide registration.
        if clip not in PREVIOUS_ENEMY_CLIPS and clip<1710 and reference*maximum<=5376:continue
        scale=max(1024/reference,NATIVE_DENSITY*maximum)
        result.append(dict(kind='enemy',clip=clip,key=original['key'],model=original['model'],
            original_job=original,visual_state=job_state(original),frames=old[1],frame_ms=old[2],
            times=[i*original['duration']/old[1] for i in range(old[1])],
            scale=scale,units=round(1024/scale),reference=reference,maximum_visual_scale=maximum))
    t.require(len({job['clip'] for job in result})==len(result),'Duplicate actor repair job')
    return result


def render_job(pack,sampler,job):
    """Yield whole source rasters for both conversion and independent assembly checks."""
    if job['kind']=='enemy':
        original=job['original_job'];models={r['model']:r for r in enemy_plan()['models']}
        draws=_compose(sampler,models[job['model']],original['clip'],job['times'],
                       original['skins'],job['visual_state'])
        for draw in draws:yield source_frame(draw,job['scale'])
    elif job['kind']=='particles':
        poses=sample_bounded(sampler,job['resource'],job['animation'],job['times'],loop=False)
        samples,_=sample_particles(pack,job['character'],job['events'],poses)
        t.require(len(samples)==job['frames'],'Death particle clock/count changed')
        for sample in samples:yield particle_frame(sample,job['scale'])
    else:
        poses=sample_bounded(sampler,job['resource'],job['animation'],job['times'],loop=job['loop'])
        for pose in poses:
            draw=[(*row[:-1],job['materials'].get(row[-1],job['materials'].get(str(row[-1]),row[-1])))
                  for row in sampler.draw(job['resource'],pose)]
            t.require(all(row[-1] in (0,1,2) for row in draw),'Unhandled actor material')
            yield source_frame(draw,job['scale'])


def sample_job(pack,sampler,job,cache,identity):
    signature=hashlib.sha256((identity+json.dumps(job,sort_keys=True)).encode()).hexdigest()
    directory=cache/f"clip-{job['clip']}-{signature[:16]}";directory.mkdir(parents=True,exist_ok=True)
    saved=cached_frames(directory,signature)
    if saved is not None:return directory,saved
    records=[]
    for frame,(picture,x,y) in enumerate(render_job(pack,sampler,job)):
        parts=[]
        for part,(image,left,top) in enumerate(exact_parts(picture,x,y)):
            file=f'{frame:03d}-{part:02d}.png';image.save(directory/file)
            parts.append(dict(file=file,x=left,y=top,size=list(image.size)))
        records.append(parts)
    t.require(len(records)==job['frames'],'Actor sampling omitted a frame')
    saved=dict(job,signature=signature,precision=8,parts=records)
    t.atomic_bytes(directory/'frames.json',json.dumps(saved,indent=2).encode()+b'\n')
    return directory,saved


def build(data,pack,sampler,work,runtime,enemy_limits=None):
    source=view(data)
    t.require(len(source.clips)==CLIP_COUNT and len(source.aux)==BASE_AUX,
              'Expected complete matching UI21 resources')
    cache=work/'encoded';cache.mkdir(parents=True,exist_ok=True)
    stamp=input_identity(pack,runtime);replacements={};appended=[];tables=[0]*(CLIP_COUNT+1);reports=[]
    for job in jobs(pack,source,sampler,enemy_limits):
        directory,record=sample_job(pack,sampler,job,work/'sampled',stamp)
        first=source.clips[job['clip']][0] if job['clip']>=0 else None
        frames=[]
        for frame,parts in enumerate(record['parts']):
            registered=[]
            for part,piece in enumerate(parts):
                row=pixel_row(cache,f"actor-{job['clip']}-{frame}-{part}",
                              Image.open(directory/piece['file']).convert('RGBA'),8)
                if first is not None and part==0:
                    image=first+frame;replacements[image]=row
                else:
                    image=len(source.images)+len(appended);appended.append(row)
                registered.append((image,piece['x'],piece['y']))
            frames.append(registered)
        index=job['clip'] if job['clip']>=0 else CLIP_COUNT
        tables[index]=len(source.images)+len(appended)
        appended.append(word_row(cache,f"actor-parameters-{job['clip']}",frames,
                                 job['frame_ms'],job['units'],job['reference']))
        reports.append(dict(clip=job['clip'],key=job['key'],frames=job['frames'],frame_ms=job['frame_ms'],
            parts=sum(map(len,frames)),max_parts=max(map(len,frames)),units=job['units'],reference=job['reference'],
            auxiliary=BASE_AUX+index,scale=job['scale'],precision='RGB565/alpha8 exact',
            cache=str(directory),registered_frames=frames))
        emit('actor_clip_ready',**{k:v for k,v in reports[-1].items() if k not in ('registered_frames','cache')})
    ids=sorted(replacements);rows=[replacements[i] for i in ids]
    art=grouped_art(rows,cache);art['image_ids']=ids
    raw={i:dict(replacements[i],file=str(cache/replacements[i]['file'])) for i in ids}
    replaced,replace_receipt=replace_images(data,{},raw_replacements=raw,replacement_art=art)
    from split_content import LANGUAGES
    manifest=dict(images=appended,auxiliary_image_ids=tables,localized_text={code:{} for code in LANGUAGES})
    result,append_receipt=patch_pack(replaced,manifest,cache,base_aux=BASE_AUX,auxiliary=AUXILIARY,
        input_version=source.version,version=source.version,texts=(),base_clips=CLIP_COUNT,
        base_strings=len(source.strings),appended_art=grouped_art(appended,cache))
    target=view(result);t.require(target.clips==source.clips,'Stable actor IDs/clocks changed')
    for i,(old,new) in enumerate(zip(source.images,target.images)):
        if i not in replacements:t.require(old[1:]==new[1:],'Unchanged image metadata changed')
    receipt=dict(complete=True,reference='v0.111.0 / 41cef1ea',input_sha256=t.sha256(data),
        output_sha256=t.sha256(result),input_bytes=len(data),output_bytes=len(result),
        replaced_images=ids,appended_images=list(range(len(source.images),len(target.images))),
        auxiliary_count=AUXILIARY,clips=reports,all_clip_metadata_unchanged=True,
        runtime_pixel_buffers_added=0,added_resident_directory_bytes=(AUXILIARY-BASE_AUX)*4,
        palette_quantization=False,alpha_quantization=False,union_downsampling=False,
        replacement=replace_receipt,append=append_receipt)
    return result,receipt


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('input','pck','runtime','work','output'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--dotnet',default='dotnet');p.add_argument('--enemy-limits',type=Path)
    args=p.parse_args()
    for path in (args.input,args.pck,args.runtime,args.work,args.output):t.private_path(path,'Private actor source/output')
    args.work.mkdir(parents=True,exist_ok=True)
    limits=json.loads(args.enemy_limits.read_text()) if args.enemy_limits else None
    pack=Pack(args.pck)
    try:
        with SpineSampler(pack,runtime=args.runtime,dotnet=args.dotnet,work_dir=args.work/'adapter') as sampler:
            result,receipt=build(args.input.read_bytes(),pack,sampler,args.work,args.runtime,limits)
    finally:pack.close()
    t.atomic_bytes(args.output,result)
    t.atomic_bytes(args.work/'actor-animation-ui22-receipt.json',json.dumps(receipt,indent=2).encode()+b'\n')
    emit('actor_pack_ready',output=str(args.output),bytes=len(result),clips=len(receipt['clips']),
         frames=sum(row['frames'] for row in receipt['clips']),sha256=t.sha256(result))


if __name__=='__main__':main()
