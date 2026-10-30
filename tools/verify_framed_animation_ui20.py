#!/usr/bin/env python3
"""Compare every private registered frame with a fresh full source raster.

The source oracle rasterizes one unpartitioned frame, while the shipped path
decodes independent cropped pieces and applies their private registrations.
This catches missing/misaligned pieces, density changes and alpha discontinuity.
Only cache PNGs, source assets and reports outside the repository are read/written.
"""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import struct
import numpy as np
from PIL import Image,ImageDraw
import content_tiles as tiles
from build_world import job_state
from framed_animation_ui20_assets import CLIPS,NATIVE_DENSITY,BASE_AUX
from pck import Pack
from quantize_animation_resources import lookup
from ui12_resources import view
from world_enemy_assets import SpineSampler,_compose
from world_enemy_profile import plan as enemy_plan
from world_raster import raster


def dense(raw,width,height,fmt):
    tiles.require(fmt==4,'Registered pixels must use the existing sparse format')
    _,color,alpha=tiles.validate_sparse(raw,width,height,pixels=True)
    return color,alpha


def quantized(picture,precision):
    rgba=np.asarray(picture,dtype=np.uint16)
    colors,alphas=lookup(precision)
    rgb=((rgba[:,:,0]>>3)<<11)|((rgba[:,:,1]>>2)<<5)|(rgba[:,:,2]>>3)
    color=colors[rgb];color[rgba[:,:,3]==0]=0
    return color,alphas[rgba[:,:,3]]


def display(color,alpha):
    rgba=np.stack(((color>>11)*255//31,((color>>5)&63)*255//63,(color&31)*255//31,alpha),axis=2).astype('u1')
    return Image.fromarray(rgba,'RGBA')


def reference_frame(draw,scale):
    points=[layer[0] for layer in draw if len(layer[0])]
    if not points:return Image.new('RGBA',(1,1)),0,0
    low=np.asarray([v.min(axis=0) for v in points]).min(axis=0)
    high=np.asarray([v.max(axis=0) for v in points]).max(axis=0)
    x,y=math.floor(low[0]*scale)-2,math.floor(-high[1]*scale)-2
    width,height=math.ceil(high[0]*scale)+2-x,math.ceil(-low[1]*scale)+2-y
    return raster(draw,scale,(-x,-y),(width,height)),x,y


def table_frames(source,auxiliary):
    w,h,fmt,raw=source.image_raw(source.aux[auxiliary]);tiles.require(fmt==0,'Expected exact word table')
    words=struct.unpack('<'+'H'*(len(raw)//2),raw)
    frames=[]
    for i in range(words[2]):
        start,count=words[10+2*i:12+2*i];frame=[]
        for j in range(count):frame.append(struct.unpack_from('<Hhh',raw,2*(start+3*j)))
        frames.append(frame)
    return frames


def verify_frames(source,record,directory,draws,scale,out,auxiliary):
    frames=table_frames(source,auxiliary);tiles.require(len(frames)==len(draws)==record['frames'],'Source frame clock changed')
    totals=dict(clip=record['clip'],frames=len(frames),parts=0,split_frames=0,raw_threshold_values=0,
                retained_color_threshold_pixels=0,retained_alpha_threshold_pixels=0,max_source_byte_error=0)
    for index,(draw,parts,saved) in enumerate(zip(draws,frames,record['parts'])):
        expected,left,top=reference_frame(draw,scale)
        expected_color,expected_alpha=quantized(expected,record['precision'])
        color=np.zeros(expected_alpha.shape,dtype='<u2');alpha=np.zeros(expected_alpha.shape,dtype='u1')
        occupied=np.zeros(expected_alpha.shape,dtype=bool)
        assembled=Image.new('RGBA',expected.size)
        for (image,x,y),cache in zip(parts,saved):
            w,h,fmt,raw=source.image_raw(image);part_color,part_alpha=dense(raw,w,h,fmt)
            cached=Image.open(directory/cache['file']).convert('RGBA');tiles.require(cached.size==(w,h),'Cached piece dimensions changed')
            cached_color,cached_alpha=quantized(cached,record['precision'])
            tiles.require(np.array_equal(cached_color,part_color) and np.array_equal(cached_alpha,part_alpha),
                          'Shipped pixel payload differs from the sampled piece')
            px,py=x-left,y-top
            tiles.require(0<=px<=expected.width-w and 0<=py<=expected.height-h,'Piece registration escapes source frame')
            tiles.require(not occupied[py:py+h,px:px+w].any(),'Piece rectangles overlap')
            occupied[py:py+h,px:px+w]=True
            color[py:py+h,px:px+w],alpha[py:py+h,px:px+w]=part_color,part_alpha
            assembled.paste(cached,(px,py));totals['parts']+=1
        delta=np.abs(np.asarray(assembled,dtype=np.int16)-np.asarray(expected,dtype=np.int16))
        # An integer translation of a floating point solve can straddle one
        # 8-bit threshold. It must never move coverage, geometry or detail.
        maximum=int(delta.max());tiles.require(maximum<=1,'Source pixels differ beyond one rounding threshold')
        totals['max_source_byte_error']=max(totals['max_source_byte_error'],maximum)
        totals['raw_threshold_values']+=int(np.count_nonzero(delta))
        totals['retained_color_threshold_pixels']+=int(np.count_nonzero(color!=expected_color))
        totals['retained_alpha_threshold_pixels']+=int(np.count_nonzero(alpha!=expected_alpha))
        tiles.require(np.array_equal(alpha>0,expected_alpha>0),'A partition changed visible alpha coverage')
        if len(parts)>1:
            totals['split_frames']+=1
            shown=display(color,alpha);comparison=Image.new('RGBA',(expected.width*2+12,expected.height+24),(25,27,31,255))
            comparison.alpha_composite(display(expected_color,expected_alpha),(0,24))
            comparison.alpha_composite(shown,(expected.width+12,24))
            label=ImageDraw.Draw(comparison);label.text((3,4),f"Full source {record['clip']} / {index}",fill='white')
            label.text((expected.width+15,4),f"Shipped {len(parts)} parts",fill='white')
            comparison.convert('RGB').save(out/f"split-{record['clip']}-{index:03d}.png")
    return totals


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('content','pck','runtime','cache','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--dotnet',default='dotnet');args=parser.parse_args()
    for path in (args.content,args.pck,args.runtime,args.cache,args.output):tiles.private_path(path,'Private pixel oracle path')
    args.output.mkdir(parents=True,exist_ok=True);source=view(args.content.read_bytes());plan=enemy_plan()
    jobs={row['id']:row for row in plan['jobs']};models={row['model']:row for row in plan['models']};reports=[]
    pack=Pack(args.pck)
    try:
        with SpineSampler(pack,runtime=args.runtime,dotnet=args.dotnet,work_dir=args.output/'adapter') as sampler:
            for row,clip in enumerate((*CLIPS,-2)):
                matches=[]
                for path in args.cache.glob('*/frames.json'):
                    data=json.loads(path.read_text())
                    if data['clip']==clip:matches.append((path.parent,data))
                tiles.require(len(matches)==1,'Ambiguous private sample cache');directory,record=matches[0]
                if clip>=0:
                    job=jobs[clip];times=[i*job['duration']/job['frames'] for i in range(job['frames'])]
                    draws=_compose(sampler,models[job['model']],job['clip'],times,job['skins'],job_state(job));scale=NATIVE_DENSITY
                else:
                    import animation
                    skeleton=animation.Skeleton(pack.read(pack.imported('osty.skel','.spskel')))
                    duration=skeleton.animations['dead_loop']['duration'];count=record['frames'];scale=record['scale']
                    frames=sampler.sample('osty','dead_loop',[i*duration/count for i in range(count)],loop=True)
                    draws=[sampler.draw('osty',frame) for frame in frames]
                report=verify_frames(source,record,directory,draws,scale,args.output,BASE_AUX+row)
                reports.append(report);print(json.dumps(report),flush=True)
    finally:pack.close()
    result=dict(clips=reports,frames=sum(row['frames'] for row in reports),
                source='fresh official Spine samples; unpartitioned source raster',
                exact_shipped_piece_decode=True,unchanged_visible_coverage=True)
    tiles.atomic_bytes(args.output/'pixel-oracle.json',json.dumps(result,indent=2).encode()+b'\n')


if __name__=='__main__':main()
