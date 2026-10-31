#!/usr/bin/env python3
"""Verify native actor pieces and export independent full-frame Canvas inputs.

Every shipped registered image is compared with exact source RGBA -> RGB565
plus alpha8. Selected or all clips additionally resample the official original
skeleton and compare a full unpartitioned raster, with zero byte tolerance.
The native regression consumes the full-frame reference through the actual
Canvas and range reader; it does not render from the converter's piece table.
"""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import struct

import numpy as np
from PIL import Image

import content_tiles as t
from actor_animation_ui22 import render_job
from pck import Pack
from restore_animation_precision import read_image
from ui12_resources import view
from verify_framed_animation_ui20 import quantized,table_frames
from world_enemy_assets import SpineSampler


def emit(event,**values):
    print(json.dumps(dict(event=event,**values)),flush=True)


def assembled_parts(parts):
    left=min(x for _p,x,y in parts);top=min(y for _p,x,y in parts)
    width=max(x+p.width for p,x,y in parts)-left;height=max(y+p.height for p,x,y in parts)-top
    t.require(0<width<=2560 and 0<height<=1920,'Unbounded source actor frame')
    picture=Image.new('RGBA',(width,height));occupied=np.zeros((height,width),dtype=bool)
    for image,x,y in parts:
        ox,oy=x-left,y-top
        t.require(not occupied[oy:oy+image.height,ox:ox+image.width].any(),'Source partition rectangles overlap')
        occupied[oy:oy+image.height,ox:ox+image.width]=True;picture.paste(image,(ox,oy))
    return picture,left,top


def exact_source(reference,assembled):
    expected,left,top=reference;picture,x,y=assembled
    if picture.getchannel('A').getbbox() is None:
        # Empty frames retain their source time with a 1x1 zero-alpha
        # sentinel at(0,0); its registration cannot affect any LCD pixel.
        t.require(expected.getchannel('A').getbbox() is None,
                  'Empty registered frame omitted visible original source pixels')
        return
    actual=Image.new('RGBA',expected.size)
    t.require(left<=x and top<=y and x+picture.width<=left+expected.width and
              y+picture.height<=top+expected.height,'Registered source frame escapes fresh original raster')
    actual.paste(picture,(x-left,y-top))
    expected_color,expected_alpha=quantized(expected,8);color,alpha=quantized(actual,8)
    t.require(np.array_equal(alpha,expected_alpha) and np.array_equal(color,expected_color),
              'Registered native source differs from the fresh full original raster')


def verify(content,receipt,jobs,output,*,sampler=None,pack=None,fresh=()):
    source=view(content);reader=t.TileReader(source);job_ids={j['clip']:j for j in jobs}
    total=sum(r['frames'] for r in receipt['clips']);frames=pieces=fresh_frames=0;reports=[]
    path=output/'source-canvas.bin'
    with path.open('wb') as oracle:
        oracle.write(b'ACTR0022'+struct.pack('<I',total))
        for record in receipt['clips']:
            clip=record['clip'];job=job_ids[clip];directory=Path(record['cache'])
            saved=json.loads((directory/'frames.json').read_text());table=table_frames(source,record['auxiliary'])
            t.require(saved['frames']==len(table)==record['frames'] and
                      table==[[tuple(x) for x in frame] for frame in record['registered_frames']],
                      'Stored native registration or original sample count changed')
            fresh_samples=iter(render_job(pack,sampler,job)) if clip in fresh else None
            for frame,(references,stored) in enumerate(zip(table,saved['parts'])):
                t.require(len(references)==len(stored),'Stored source part count changed');images=[];anchors=[]
                for (image,x,y),row in zip(references,stored):
                    picture=Image.open(directory/row['file']).convert('RGBA')
                    t.require((x,y)==(row['x'],row['y']),'Original source coordinates changed')
                    width,height,fmt,raw=read_image(source,reader,image)
                    t.require(fmt==4 and picture.size==(width,height),'Stored native pixel dimensions changed')
                    _geometry,color,alpha=t.validate_sparse(raw,width,height,pixels=True)
                    expected_color,expected_alpha=quantized(picture,8)
                    t.require(np.array_equal(color,expected_color) and np.array_equal(alpha,expected_alpha),
                              f'Original RGB565 or alpha8 changed in image {image}')
                    images.append((picture,x,y));anchors.append((160-x-width//2,120-y-height//2));pieces+=1
                reference=assembled_parts(images)
                if fresh_samples is not None:
                    expected=next(fresh_samples);exact_source(expected,reference);reference=expected;fresh_frames+=1
                picture,left,top=reference;color,alpha=quantized(picture,8)
                payload=color.astype('<u2').tobytes()+alpha.tobytes()
                scale=(1024*record['reference']+record['units']-1)//record['units']
                mode=1 if job['kind']=='particles' else 0
                oracle.write(struct.pack('<12i',clip,frame,record['frame_ms'],scale,left,top,
                    picture.width,picture.height,mode,len(anchors),len(payload),0))
                for x,y in anchors:oracle.write(struct.pack('<ii',x,y))
                oracle.write(payload);frames+=1
            if fresh_samples is not None:
                t.require(next(fresh_samples,None) is None,'Original source sampler added unexpected frames')
            reports.append(dict(clip=clip,key=record['key'],frames=record['frames'],
                fresh_official_source=clip in fresh,zero_rgb565_error=True,zero_alpha8_error=True))
            emit('actor_source_pixels',clips=len(reports),total_clips=len(receipt['clips']),
                 frames=frames,pieces=pieces,fresh_original_frames=fresh_frames)
    t.require(frames==total,'Native source-reference spool omitted a frame')
    result=dict(complete=True,input_sha256=t.sha256(content),frames=frames,pieces=pieces,
        fresh_original_frames=fresh_frames,cached_original_frames=frames-fresh_frames,
        source_canvas=str(path),source_canvas_bytes=path.stat().st_size,clips=reports,
        rgb565_target=True,alpha_bits=8,additional_palette_or_alpha_loss=False)
    t.atomic_bytes(output/'actor-source-pixels.json',json.dumps(result,indent=2).encode()+b'\n')
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('content','receipt','jobs','output'):p.add_argument('--'+name,type=Path,required=True)
    for name in ('pck','runtime'):p.add_argument('--'+name,type=Path)
    p.add_argument('--dotnet',default='dotnet');p.add_argument('--fresh-clip',type=int,action='append',default=[])
    p.add_argument('--fresh-all',action='store_true');args=p.parse_args()
    for path in (args.content,args.receipt,args.jobs,args.output,args.pck,args.runtime):
        if path is not None:t.private_path(path,'Private original-source actor verification')
    args.output.mkdir(parents=True,exist_ok=True);receipt=json.loads(args.receipt.read_text())
    jobs=json.loads(args.jobs.read_text())['jobs_manifest']
    fresh={r['clip'] for r in receipt['clips']} if args.fresh_all else set(args.fresh_clip)
    t.require(fresh<={r['clip'] for r in receipt['clips']},'Fresh source request is outside the completed artwork')
    if fresh:
        t.require(args.pck is not None and args.runtime is not None,'Fresh original verification requires its source inputs')
        pack=Pack(args.pck)
        try:
            with SpineSampler(pack,runtime=args.runtime,dotnet=args.dotnet,work_dir=args.output/'adapter') as sampler:
                verify(args.content.read_bytes(),receipt,jobs,args.output,sampler=sampler,pack=pack,fresh=fresh)
        finally:pack.close()
    else:verify(args.content.read_bytes(),receipt,jobs,args.output)


if __name__=='__main__':main()
