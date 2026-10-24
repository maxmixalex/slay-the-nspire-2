#!/usr/bin/env python3
"""Append original player/merchant deaths and separate particle planes to v29.

Consumes only the owner's private pinned PCK and previous v28 pack. All older
images, glyphs, strings, clips and indices survive unchanged. Official Spine
sampling includes every 30-Hz frame and the exact final pose. Independent
particle planes retain additive composition and their complete decay. This
module contains paths and converter code, never original pixels or strings.
"""
from __future__ import annotations

import argparse
import json
import math
import mmap
from pathlib import Path

import numpy as np
from PIL import Image

import content_tiles as tiles
from character_death_particles import sample_particles, particle_bounds, render_particles
from measure_combat_registration import measure_players
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
from patch_phobia_assets import grouped_art
from pck import Pack
from world_enemy_assets import SpineSampler
from world_raster import raster
from world_scene_assets import SceneLoader, _property, load_scene_texture

CHARACTERS=('ironclad','silent','defect','regent','necrobinder')
BASE_VERSION,VERSION=28,29
BASE_CLIPS,CLIPS=1753,1765
BASE_AUX,AUXILIARY,STRINGS=2044,2045,8704
DURATIONS=(2333,1867,2233,4167,2333)
COUNTS=(71,57,68,126,71)
TIMER='images/atlases/ui_atlas.sprites/top_bar/timer_icon.tres'


def times(duration, fps=30):
    """Keep an exact terminal sample; float source durations are not rounded up."""
    intervals=max(1,round(duration*fps))
    return [duration*i/intervals for i in range(intervals)]+[duration]


def layout(points, desired_scale):
    points=np.asarray(points,dtype=float)
    tiles.require(points.ndim==2 and points.shape[1]==2 and len(points)>0 and np.isfinite(points).all(),
                  'Invalid death sampling bounds')
    low,high=points.min(axis=0),points.max(axis=0)
    extent=np.maximum(1.,high-low)
    # The picture reader has an existing320x240 bound. Registration stored
    # on each new clip restores the exact scene extent after this sampling
    # constraint; it never shrinks a rendered actor or clips its effects.
    scale=min(desired_scale,316/extent[0],236/extent[1])
    size=np.ceil(extent*scale).astype(int)+4
    tiles.require(0<scale and np.all(size<=[320,240]),'Death image cannot fit its bounded reader')
    return dict(scale=float(scale),size=list(map(int,size)),origin=[float(2-low[0]*scale),float(2+high[1]*scale)],
                bounds=[low.tolist(),high.tolist()],sampling_scale_limited=bool(scale<desired_scale))


def identity(runtime):
    root=Path(__file__).parent
    names=('character_death_assets.py','character_death_particles.py','world_enemy_assets.py','SpineSampleAdapter.cs',
           'world_raster.py','world_scene_assets.py','world_room_shaders.py','patch_frontend_assets.py','patch_phobia_assets.py')
    return tiles.sha256(b''.join((root/name).read_bytes() for name in names)+Path(runtime).read_bytes())


def clip_row(first,count,duration,geometry,source_scale,key):
    units=round(1024*source_scale/geometry['scale'])
    tiles.require(0<units<1000000,'Unbounded character clip registration')
    return dict(first=first,count=count,frame_ms=max(1,round(duration*1000/max(1,count-1))),
                pivot=[round(n) for n in geometry['origin']],sprite_units=units,key=key)


def material_overrides(pack,name,merchant):
    loader=SceneLoader(pack)
    path=f'scenes/merchant/characters/{name}_merchant.tscn' if merchant else f'scenes/creature_visuals/{name}.tscn'
    root=loader.instantiate(path);node_name='SpineSprite' if merchant else 'Visuals'
    node=next(child for child in root.children if child.name==node_name and child.kind=='SpineSprite')
    # The Spine Godot default screen material is SUB. These death clips do
    # not use it; reject an unhandled effective mode instead of painting MIX.
    result={3:4}
    for index,key in enumerate(('normal_material','additive_material','multiply_material','screen_material')):
        ref=_property(node.props,key)
        if ref is None:continue
        attrs,props=loader.resource(ref)
        tiles.require(attrs.get('type')=='CanvasItemMaterial','Unreviewed death scene material')
        result[index]={0:0,1:1,2:4,3:2,4:5}[int(_property(props,'blend_mode',0))]
    return result


def sample_bounded(sampler,resource,clip,sample_times,*,loop=False):
    # The adapter accepts at most 256 output poses per request. Each request
    # creates its skeleton at time zero and advances the official runtime to
    # these absolute times, so later batches must not restart their clock.
    # Regent's five-second particle tail needs all 301 binding poses at 60 Hz.
    frames=[]
    for first in range(0,len(sample_times),256):
        batch=sample_times[first:first+256]
        poses=sampler.sample(resource,clip,batch,loop=loop)
        tiles.require(len(poses)==len(batch),'Death sampler omitted a binding pose')
        frames.extend(poses)
    return frames


def build_art(pck_path,source,cache,*,runtime,dotnet=None):
    cache=Path(cache);cache.mkdir(parents=True,exist_ok=True)
    stamp=identity(runtime);catalog=cache/'character-death-assets.json'
    if catalog.exists():
        old=json.loads(catalog.read_text())
        if old.get('converter_sha256')==stamp:
            tiles.require(len(old['clips'])==12 and len(old['auxiliary_rows'])==1,'Death cache catalog changed')
            for row in old['images']:
                payload=(cache/row['file']).read_bytes()
                tiles.require(len(payload)==row['bytes'] and tiles.sha256(payload)==row['sha256'],'Death frame cache changed')
            return old
    pack=Pack(pck_path)
    try:
        registrations={row['model']:row for row in measure_players(pack,source)}
        images=[];clips=[];reports=[];particle_jobs=[]
        with SpineSampler(pack,runtime=runtime,dotnet=dotnet,work_dir=cache/'spine-adapter') as sampler:
            for merchant in (False,True):
                for index,name in enumerate(CHARACTERS):
                    resource=dict(skeleton_path=f'res://animations/characters/{name}/{name}.skel',
                        atlas_path=f'res://animations/characters/{name}/{name}.atlas')
                    if merchant:
                        resource=dict(skeleton_path=f'res://animations/characters/{name}/{name}.skel',
                            atlas_path=f'res://animations/merchant/{name}/{name}_shop.atlas')
                    metadata=sampler.metadata(resource)
                    animation=next(row for row in metadata['clips'] if row['name']=='die')
                    duration=animation['duration'];sample_times=times(duration)
                    tiles.require(animation['duration_ms']==DURATIONS[index] and len(sample_times)==COUNTS[index],
                                  'Original death timing changed: '+name)
                    frames=sample_bounded(sampler,resource,'die',sample_times,loop=False)
                    points=[(x,y) for frame in frames for draw in frame['draw']
                            for x,y in zip(draw['vertices'][::2],draw['vertices'][1::2])]
                    geometry=layout(points,registrations[name]['baked_scale'])
                    original_scale=(.23 if name=='defect' else .47) if merchant else registrations[name]['source_scale']
                    key=('merchant-' if merchant else '')+name
                    materials=material_overrides(pack,name,merchant)
                    first=len(images)
                    for frame_index,frame in enumerate(frames):
                        draw=[(*row[:-1],materials.get(row[-1],row[-1])) for row in sampler.draw(resource,frame)]
                        tiles.require(all(row[-1] in (0,1,2) for row in draw),'Unhandled death material blend: '+key)
                        image=raster(draw,geometry['scale'],np.asarray(geometry['origin']),geometry['size'])
                        images.append(write_image(cache,f'{key}-die-{frame_index:03}',image,sparse=True))
                        if frame_index in (0,len(frames)//2,len(frames)-1):image.save(cache/f'{key}-die-{frame_index:03}.png')
                    clips.append(clip_row(first,len(frames),duration,geometry,original_scale,key))
                    reports.append(dict(character=name,merchant=merchant,source=resource,source_duration_ms=animation['duration_ms'],
                        frame_count=len(frames),exact_terminal_sample=True,registration=geometry,source_scale=original_scale,
                        scene_material_overrides=materials))
                    print(json.dumps(dict(character=key,frames=len(frames),size=geometry['size'])),flush=True)
                    if not merchant and name in ('defect','regent'):
                        particle_duration=5. if name=='regent' else duration
                        particle_frames=sample_bounded(sampler,resource,'die',times(particle_duration,60),loop=False)
                        samples,report=sample_particles(pack,name,animation['events'],particle_frames)
                        particle_jobs.append((name,samples,report,particle_duration,original_scale,registrations[name]['baked_scale']))
            for name,samples,report,duration,original_scale,preferred in particle_jobs:
                first=len(images);geometry=layout(particle_bounds(samples),preferred)
                for i,sample in enumerate(samples):
                    image=render_particles(sample,geometry['scale'],np.asarray(geometry['origin']),geometry['size'])
                    images.append(write_image(cache,f'{name}-die-particles-{i:03}',image,sparse=True))
                    if i in (0,len(samples)//2,len(samples)-1):image.save(cache/f'{name}-die-particles-{i:03}.png')
                clips.append(clip_row(first,len(samples),duration,geometry,original_scale,name+'-particles'))
                reports.append(dict(character=name,particles=True,frame_count=len(samples),source_duration_ms=round(duration*1000),
                    registration=geometry,source_scale=original_scale,sampling=report,blend='additive_emission_behind_body'))
                print(json.dumps(dict(particles=name,frames=len(samples),size=geometry['size'])),flush=True)
        timer=load_scene_texture(pack,TIMER).convert('RGBA')
        factor=min(8/timer.width,15/timer.height)
        timer=timer.resize((max(1,round(timer.width*factor)),max(1,round(timer.height*factor))),Image.Resampling.LANCZOS)
        timer_row=len(images);images.append(write_image(cache,'run-timer-icon',timer,sparse=True))
        result=dict(schema=1,reference='v0.111.0/41cef1ea',converter_sha256=stamp,images=images,clips=clips,
                    auxiliary_rows=[timer_row],localized_text={'eng':{},'zhs':{}},reports=reports,
                    runtime_added_pixel_buffers=0,source_terminal_samples=10,skeleton_sampling_hz=30)
        tiles.require(len(clips)==CLIPS-BASE_CLIPS and [r['count'] for r in clips[:10]]==list(COUNTS)*2,
                      'Death clip directory changed')
        tiles.atomic_bytes(catalog,json.dumps(result,indent=2).encode()+b'\n')
        return result
    finally:pack.close()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck',required=True,type=Path)
    parser.add_argument('--input-eng',required=True,type=Path)
    parser.add_argument('--input-zhs',required=True,type=Path)
    parser.add_argument('--runtime',required=True,type=Path)
    parser.add_argument('--dotnet',default='dotnet')
    parser.add_argument('--cache-dir',required=True,type=Path)
    parser.add_argument('--output-dir',required=True,type=Path)
    args=parser.parse_args()
    sources={lang:tiles.private_path(getattr(args,'input_'+lang),'Previous private pack') for lang in ('eng','zhs')}
    cache=tiles.private_path(args.cache_dir,'Death cache');output=tiles.private_path(args.output_dir,'Death output')
    with sources['eng'].open('rb') as handle,mmap.mmap(handle.fileno(),0,access=mmap.ACCESS_READ) as data:
        source=tiles.PackView(data,BASE_AUX,versions=(BASE_VERSION,),profile=(558,STRINGS,BASE_CLIPS,BASE_AUX))
        art=build_art(tiles.private_path(args.pck,'Original PCK'),source,cache,
                      runtime=tiles.private_path(args.runtime,'Official Spine runtime'),dotnet=args.dotnet)
    grouped=grouped_art(art['images'],cache);receipts={}
    for language,source in sources.items():
        target=output/f'ui9-{language}.nsp'
        tiles.require(target.resolve()!=source.resolve(),'The v29 output must preserve its v28 input')
        data,receipt=patch_pack(source.read_bytes(),art,cache,base_aux=BASE_AUX,auxiliary=AUXILIARY,
            input_version=BASE_VERSION,version=VERSION,texts=(),base_clips=BASE_CLIPS,base_strings=STRINGS,appended_art=grouped)
        tiles.atomic_bytes(target,data);receipts[language]=receipt
    tiles.atomic_bytes(output/'character-death-conversion.json',json.dumps(dict(languages=receipts,grouping=grouped['report']),indent=2).encode()+b'\n')
    print(json.dumps(dict(clips=len(art['clips']),frames=sum(row['count'] for row in art['clips']),
        auxiliary=AUXILIARY,grouping=grouped['report'])),flush=True)


if __name__=='__main__':main()
