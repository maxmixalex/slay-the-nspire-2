#!/usr/bin/env python3
"""Restore source density for moving enemies without storing their travel canvas.

Pinned original Spine frames retain every clock/sample and world registration.
Each frame is rastered at LCD density, alpha-trimmed and split only when an
individual frame exceeds the existing 320x240 image bound. Private word tables
register these pieces; no source geometry is compiled into the public program.
Old clip IDs/counts/clocks remain exact, permitting all locale editions to share
the repaired artwork. Final tile reclamation/compression is a separate stage.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import time

import numpy as np
from PIL import Image
import content_tiles as tiles
from lz4_block import encode as lz4_encode
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
from patch_phobia_assets import grouped_art
from pck import Pack
from quantize_animation_resources import quantize_image
from sparse_image import encode as sparse_encode
from ui12_resources import replace_images, view
from world_enemy_assets import SpineSampler, _compose
from world_enemy_profile import plan as enemy_plan
from world_raster import raster

ROOT=Path(__file__).resolve().parents[1]
CLIPS=(105,123,124,125,548,706,707,1263,1626)
BASE_AUX,AUXILIARY=5409,5422
MAGIC,MAX_LAYERS=0x4652,16
NATIVE_UNITS=5376
NATIVE_DENSITY=1024/NATIVE_UNITS


def emit(event,**values):
    print(json.dumps(dict(event=event,**values)),flush=True)


def frame_parts(draw,scale):
    """Integer source-pixel registration, never fit a moving union to a box."""
    points=[layer[0] for layer in draw if len(layer[0])]
    if not points:return [(Image.new('RGBA',(1,1)),0,0)]
    low=np.asarray([v.min(axis=0) for v in points]).min(axis=0)
    high=np.asarray([v.max(axis=0) for v in points]).max(axis=0)
    tiles.require(np.isfinite(low).all() and np.isfinite(high).all() and scale>0,
                  'Invalid source frame bounds')
    left,top=math.floor(low[0]*scale)-2,math.floor(-high[1]*scale)-2
    right,bottom=math.ceil(high[0]*scale)+2,math.ceil(-low[1]*scale)+2
    tiles.require(right-left<=2560 and bottom-top<=1920,'Unexpected source extent')
    parts=[]
    for y in range(top,bottom,240):
        for x in range(left,right,320):
            image=raster(draw,scale,(-x,-y),(min(320,right-x),min(240,bottom-y)))
            box=image.getchannel('A').getbbox()
            if box:
                image=image.crop(box);px,py=x+box[0],y+box[1]
                tiles.require(-32768<=px<=32767 and -32768<=py<=32767,
                              'Frame registration exceeds signed word')
                parts.append((image,px,py))
    tiles.require(len(parts)<=MAX_LAYERS,'Source frame exceeds bounded part list')
    return parts or [(Image.new('RGBA',(1,1)),0,0)]


def signature(pack,runtime,job):
    digest=hashlib.sha256(json.dumps(job,sort_keys=True).encode())
    for name in ('framed_animation_ui20_assets.py','world_enemy_assets.py',
                 'world_raster.py','SpineSampleAdapter.cs','animation.py'):
        digest.update((ROOT/'tools'/name).read_bytes())
    digest.update(Path(runtime).read_bytes())
    for name,entry in sorted(pack.entries.items()):
        digest.update(name.encode());digest.update(entry.md5)
    return digest.hexdigest()


def store_frames(draws,scale,cache,metadata):
    records=[]
    for i,draw in enumerate(draws):
        parts=[]
        for j,(picture,x,y) in enumerate(frame_parts(draw,scale)):
            name=f'{i:03d}-{j:02d}.png';picture.save(cache/name)
            parts.append(dict(file=name,x=x,y=y,size=list(picture.size)))
        records.append(parts)
    metadata['parts']=records
    tiles.atomic_bytes(cache/'frames.json',json.dumps(metadata,indent=2).encode()+b'\n')
    return metadata


def cached_frames(cache,key):
    path=cache/'frames.json'
    if not path.exists():return None
    record=json.loads(path.read_text())
    if record.get('signature')!=key:return None
    return record if all((cache/p['file']).exists() for f in record['parts'] for p in f) else None


def enemy_frames(pack,sampler,source,cache,runtime):
    from build_world import job_state
    plan=enemy_plan();models={row['model']:row for row in plan['models']}
    by_id={row['id']:row for row in plan['jobs']}
    # The complete ordinary-enemy audit must not silently miss another travel
    # canvas. Vantom has its own continuous layered UI15 renderer.
    candidates=tuple(job['id'] for job in plan['jobs'] if
        source.clips[job['id']][5]>NATIVE_UNITS and job['model']!='Vantom' and
        not models[job['model']].get('shared_group'))
    tiles.require(candidates==CLIPS,'Under-density enemy audit changed: '+repr(candidates))
    for clip in CLIPS:
        job=by_id[clip];original=source.clips[clip]
        tiles.require(original[1]==job['frames'] and original[2]==max(1,round(job['duration']*1000/job['frames'])),
                      'Original frame count or clock changed')
        key=signature(pack,runtime,job);directory=cache/f'clip-{clip}-{key[:16]}'
        directory.mkdir(parents=True,exist_ok=True);record=cached_frames(directory,key)
        if record is None:
            times=[i*job['duration']/job['frames'] for i in range(job['frames'])]
            draws=_compose(sampler,models[job['model']],job['clip'],times,job['skins'],job_state(job))
            record=store_frames(draws,NATIVE_DENSITY,directory,dict(signature=key,
                clip=clip,key=job['key'],frame_ms=original[2],frames=original[1],
                units=NATIVE_UNITS,reference=original[5],precision=2,
                original_scale=1024/original[5],scale=NATIVE_DENSITY))
        emit('source_clip_ready',clip=clip,key=job['key'],frames=original[1],
             density_gain=round(original[5]/NATIVE_UNITS,3),parts=sum(map(len,record['parts'])))
        yield directory,record


def osty_frames(pack,sampler,source,cache,runtime):
    import animation
    from build_content import texture
    from measure_combat_registration import player_transform
    job=dict(model='osty',clip='dead_loop',fps=12)
    key=signature(pack,runtime,job);directory=cache/f'osty-dead-{key[:16]}'
    directory.mkdir(parents=True,exist_ok=True);record=cached_frames(directory,key)
    if record is None:
        skeleton=animation.Skeleton(pack.read(pack.imported('osty.skel','.spskel')))
        textures=animation.load_textures(pack,'osty',texture)
        measured={'osty':(skeleton,textures)};previous=animation.raster
        try:
            animation.raster=lambda *unused:None
            animation.bake(pack,'osty','idle_loop',texture,frames=1,height=55,max_width=100,cache=measured)
            _,actual=animation.bake(pack,'osty','idle_loop',texture,frames=source.clips[32][1],height=55,max_width=100,cache=measured)
        finally:animation.raster=previous
        scale=measured[('osty-scale',())]
        tiles.require(tuple(actual['pivot'])==source.clips[32][3:5] and
                      tuple(actual['size'])==source.images[source.clips[32][0]][1:3],
                      'Osty source registration does not match the existing resource')
        units=round(1024*player_transform(pack,'osty')[0,0]/scale)
        tiles.require(units==4254,'Osty source scene units changed')
        duration=skeleton.animations['dead_loop']['duration'];count=max(1,math.ceil(duration*12))
        frames=sampler.sample('osty','dead_loop',[i*duration/count for i in range(count)],loop=True)
        draws=[sampler.draw('osty',f) for f in frames]
        record=store_frames(draws,scale,directory,dict(signature=key,clip=-2,
            key='osty.dead_loop',frame_ms=max(1,round(duration*1000/count)),frames=count,
            units=4254,reference=4254,precision=4,scale=scale))
    emit('source_clip_ready',clip=-2,key=record['key'],frames=record['frames'])
    return directory,record


def pixel_row(cache,key,picture,precision):
    # Preserve the already accepted UI13 454a6 enemy / UI11 555a6 hero
    # precision. Resolution increases never lower colour or alpha precision.
    raw=quantize_image(sparse_encode(picture),picture.width,picture.height,4,precision)
    compressed=struct.pack('<BI',4,len(raw))+lz4_encode(raw)
    payload,fmt=(compressed,5) if len(compressed)<len(raw) else (raw,4)
    file=key+'.bin';tiles.atomic_bytes(cache/file,payload)
    return dict(width=picture.width,height=picture.height,format=fmt,file=file,
                bytes=len(payload),sha256=tiles.sha256(payload),raw_sha256=tiles.sha256(raw))


def word_row(cache,key,frames,frame_ms,units,reference):
    words=[MAGIC,1,len(frames),frame_ms,units&65535,units>>16,reference&65535,reference>>16,0,3]
    words.extend([0]*(2*len(frames)))
    for i,parts in enumerate(frames):
        words[10+2*i:12+2*i]=[len(words),len(parts)]
        for image,x,y in parts:words.extend([image,x&65535,y&65535])
    words[8]=len(words);width=min(320,len(words));height=math.ceil(len(words)/width)
    tiles.require(height<=240 and len(words)<65536,'Frame parameter table exceeds word image bounds')
    words.extend([0]*(width*height-len(words)));raw=struct.pack('<'+'H'*len(words),*words)
    file=key+'.bin';tiles.atomic_bytes(cache/file,raw)
    return dict(width=width,height=height,format=0,file=file,bytes=len(raw),
                sha256=tiles.sha256(raw),raw_sha256=tiles.sha256(raw))


def repair(source,pack,sampler,work,runtime,extras):
    cache=work/'encoded';cache.mkdir(parents=True,exist_ok=True)
    replacements,appended,auxiliary,receipts={ },[],[],[]
    sampled=list(enemy_frames(pack,sampler,source,work/'sampled',runtime))
    sampled.append(osty_frames(pack,sampler,source,work/'sampled',runtime))
    for directory,record in sampled:
        first=source.clips[record['clip']][0] if record['clip']>=0 else None
        frames=[]
        for frame,parts in enumerate(record['parts']):
            registered=[]
            for part,p in enumerate(parts):
                row=pixel_row(cache,f"frame-{record['clip']}-{frame}-{part}",
                    Image.open(directory/p['file']).convert('RGBA'),record['precision'])
                if first is not None and part==0:
                    image=first+frame;replacements[image]=row
                else:image=len(source.images)+len(appended);appended.append(row)
                registered.append((image,p['x'],p['y']))
            frames.append(registered)
        auxiliary.append(len(appended));table=word_row(cache,f"parameters-{record['clip']}",
            frames,record['frame_ms'],record['units'],record['reference']);appended.append(table)
        receipts.append(dict(clip=record['clip'],key=record['key'],frames=record['frames'],
            frame_ms=record['frame_ms'],units=record['units'],reference=record['reference'],
            auxiliary=BASE_AUX+len(auxiliary)-1,parts=sum(map(len,frames)),
            max_parts=max(map(len,frames)),registered_frames=frames))
    from world_scene_assets import load_scene_texture
    for name in ('rare','uncommon','special_card'):
        path='images/ui/reward_screen/reward_icon_'+name+'.png'
        image=load_scene_texture(pack,path).convert('RGBA').resize((23,23),Image.Resampling.LANCZOS)
        row=write_image(cache,'reward-'+name,image);row['source']=path
        auxiliary.append(len(appended));appended.append(row)
    for manifest in extras:
        extra=json.loads(manifest.read_text());extra=extra.get('images',extra) if isinstance(extra,dict) else extra
        for row in extra:
            image=int(row['image_id'] if 'image_id' in row else row['image']);tiles.require(image not in replacements,'Duplicate extra replacement')
            row=dict(row);row['file']=str((manifest.parent/row['file']).resolve());replacements[image]=row
    tiles.require(len(auxiliary)==AUXILIARY-BASE_AUX,'Missing UI20 auxiliary image')
    replacement_ids=sorted(replacements);rows=[replacements[i] for i in replacement_ids]
    art=grouped_art(rows,cache);art['image_ids']=replacement_ids
    raw={i:dict(replacements[i],file=str(cache/replacements[i]['file'])) for i in replacement_ids}
    data,replaced=replace_images(source.data,{},raw_replacements=raw,replacement_art=art)
    from split_content import LANGUAGES
    manifest=dict(images=appended,auxiliary_rows=auxiliary,localized_text={code:{} for code in LANGUAGES})
    data,added=patch_pack(data,manifest,cache,base_aux=BASE_AUX,auxiliary=AUXILIARY,
        input_version=44,version=44,texts=(),base_clips=len(source.clips),base_strings=len(source.strings),
        appended_art=grouped_art(appended,cache))
    target=view(data);tiles.require(target.clips==source.clips,'Stable animation metadata changed')
    # Every untouched image descriptor is relocated without a shape/codec
    # change; the append/replacement utilities validate exact payload copies.
    unchanged=0
    for i,(old,new) in enumerate(zip(source.images,target.images)):
        if i not in replacements:
            tiles.require(old[1:]==new[1:],'Untouched image dimensions/encoding changed');unchanged+=1
    receipt=dict(reference='v0.111.0 / 41cef1ea',replaced_images=replacement_ids,
        appended_images=list(range(len(source.images),len(target.images))),
        auxiliary_rows={str(BASE_AUX+i):len(source.images)+row for i,row in enumerate(auxiliary)},
        clips=receipts,replacement=replaced,append=added,unchanged_image_descriptors=unchanged,
        all_clip_metadata_unchanged=True,runtime_buffer_growth=0,
        enemy_precision='454a6',osty_precision='555a6',source_geometry_public=False)
    return data,receipt


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('input','pck','runtime','work','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--dotnet',default='dotnet');parser.add_argument('--extra-replacements',action='append',type=Path,default=[])
    args=parser.parse_args()
    for path in (args.input,args.pck,args.runtime,args.work,args.output):tiles.private_path(path,'Private UI20 source/output')
    tiles.require(args.input.resolve()!=args.output.resolve(),'Do not overwrite the input pack')
    args.work.mkdir(parents=True,exist_ok=True);started=time.monotonic();source=view(args.input.read_bytes())
    tiles.require(source.version==44 and len(source.aux)==BASE_AUX,'Expected complete UI19 NSP2/44 source')
    pack=Pack(args.pck)
    try:
        with SpineSampler(pack,runtime=args.runtime,dotnet=args.dotnet,work_dir=args.work/'adapter') as sampler:
            data,receipt=repair(source,pack,sampler,args.work,args.runtime,args.extra_replacements)
    finally:pack.close()
    tiles.atomic_bytes(args.output,data)
    receipt.update(path=str(args.output),bytes=len(data),sha256=tiles.sha256(data),elapsed_seconds=round(time.monotonic()-started,3))
    tiles.atomic_bytes(args.output.with_suffix('.json'),json.dumps(receipt,indent=2).encode()+b'\n')
    emit('framed_animation_complete',bytes=len(data),replaced=len(receipt['replaced_images']),
         appended=len(receipt['appended_images']),elapsed_seconds=receipt['elapsed_seconds'])


if __name__=='__main__':main()
