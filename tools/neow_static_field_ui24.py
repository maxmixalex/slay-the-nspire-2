#!/usr/bin/env python3
"""Bake UI24's held Neow light pose, retaining animated body/transmission IDs.

The integer sampler mirrors the calculator's LCD shader; a separate optional
C++ oracle evaluates the production Shader for every output pixel. The five
unused material pages and their unreachable retained slices are reclaimed.
"""
from pathlib import Path
import json
import struct
import numpy as np
import content_tiles as t
import neow_fire_ui23 as fire

Q = 1 << 20


def _sample(field, u, v, channel, repeat):
    h, w, _ = field.shape
    u, v = (u & (Q-1), v & (Q-1)) if repeat else (np.clip(u, 0, Q), np.clip(v, 0, Q))
    px, py = u*w-Q//2, v*h-Q//2
    x, y, fx, fy = px >> 20, py >> 20, (px & (Q-1)) >> 8, (py & (Q-1)) >> 8
    if repeat:
        x0, x1, y0, y1 = x % w, (x+1) % w, y % h, (y+1) % h
    else:
        x0, x1, y0, y1 = np.clip(x,0,w-1), np.clip(x+1,0,w-1), np.clip(y,0,h-1), np.clip(y+1,0,h-1)
    a = field[y0,x0,channel]*4096 + (field[y0,x1,channel]-field[y0,x0,channel])*fx
    b = field[y1,x0,channel]*4096 + (field[y1,x1,channel]-field[y1,x0,channel])*fx
    return ((a << 12)+(b-a)*fy+2040)//4080


def bake(parameter, fields):
    """Return exactly 17,485 little-endian G8/B8 words at source clock zero."""
    info = fire.parameters(parameter)
    t.require(not info.get('baked_light'), 'Already baked Neow input')
    expected = ((636,600,1),(250,250,3),(256,256,3),(100,100,2))
    t.require(tuple(x.shape for x in fields)==expected, 'Pinned Neow field dimensions')
    fields = [np.asarray(x,dtype='i8') for x in fields]
    xs, ys = [], []
    for y in range(240):
        begin = struct.unpack_from('<H',parameter,fire.ROWS_AT+y*4)[0]
        end = struct.unpack_from('<H',parameter,fire.ROWS_AT+(y+1)*4)[0]
        for span in range(begin,end):
            x,count = struct.unpack_from('<2H',parameter,fire.SPANS_AT+span*4)
            xs.extend(range(x,x+count)); ys.extend([y]*count)
    xs, ys = np.asarray(xs,dtype='i8'), np.asarray(ys,dtype='i8')
    t.require(len(xs)==fire.PIXELS, 'Neow support size')
    total = np.zeros((fire.PIXELS,2),dtype='i8')
    flags = np.frombuffer(parameter[fire.LENGTH:fire.LENGTH+(fire.PIXELS+1)//2],dtype='u1') if info['integer_bounds'] else None
    for node in range(4):
        r = struct.unpack_from('<6i2H4B',parameter,fire.NODES_AT+node*32)
        q = np.asarray(struct.unpack_from('<32i',parameter,fire.MODELS_AT+r[8]*128),dtype='i8')
        scale = q[0]
        t.require(scale in (8388608,16777216) and q[1]==scale and q[5]==scale and q[6]==scale and
                  all(q[i]==16777216 for i in (4,9,10,11,15,16,20,24)) and q[21]==33554432,
                  'Pinned Neow unit shader model')
        u, v = (r[0]+ys*r[4]+xs*r[2]) >> 8, (r[1]+ys*r[5]+xs*r[3]) >> 8
        active = (u>=0)&(u<Q)&(v>=0)&(v<Q)
        if flags is not None:
            indices=np.arange(fire.PIXELS);active &= ((flags[indices//2] >> ((indices%2)*4+node))&1)!=0
        # All authored nodes are evaluated at clock zero; all four pans are 0.
        ar=_sample(fields[1],u,v,0,True); ag=_sample(fields[1],u,v,1,True)
        br=_sample(fields[2],u,v,0,True); bg=_sample(fields[2],u,v,1,True)
        dx=(ar*q[14] >> 24)+(br*q[19] >> 24);dy=(ag*q[14] >> 24)+(bg*q[19] >> 24)
        mu=u+(q[26] >> 4);mv=(v*q[25] >> 24)+(q[27] >> 4)
        dx=dx*_sample(fields[3],mu,mv,0,False) >> 20;dy=dy*_sample(fields[3],mu,mv,1,False) >> 20
        base=_sample(fields[2],u+dx,v+dy,2,False)
        mask=Q-_sample(fields[2],u+(q[22] >> 4),2*(v-41943)+41943+(q[23] >> 4),2,False)
        nu,nv=(u >> 1,v >> 1) if scale==8388608 else (u,v)
        n1=_sample(fields[0],nu,nv,0,True);n2=n1
        strength=(((n1*n2 >> 20)+mask)*base >> 20)*16
        inverse=(Q << 24)//int(q[31]-q[30])
        x=np.clip((np.maximum(strength-q[30],0)*inverse) >> 24,0,Q)
        smooth=((x*x >> 20)*(3*Q-2*x)) >> 20
        alpha=np.where(strength<=q[30],0,np.where(strength>=q[31],Q,smooth))
        alpha=((alpha*255+Q//2) >> 20)*active
        total[:,0]+=alpha*r[6];total[:,1]+=alpha*r[7]
    t.require(np.all(total<=65535), 'Neow contribution sum bound')
    total+=128;total=(total+(total >> 8)) >> 8
    t.require(np.all(total<=255), 'Neow radiance channel bound')
    return ((total[:,0] << 8)|total[:,1]).astype('<u2').tobytes()


def source_fields(source):
    fields=[]
    for first,(h,w,channels) in zip((fire.BASE_IMAGES,59734,fire.BASE_IMAGES+3,fire.BASE_IMAGES+5),
                                  ((636,600,1),(250,250,3),(256,256,3),(100,100,2))):
        size=w*h*channels;data=bytearray()
        for offset in range(0,size,131072):
            width,height,fmt,raw=source.image_raw(first+offset//131072);used=min(131072,size-offset)
            t.require((width,height,fmt)==(320,(used+639)//640,0) and not any(raw[used:]),'Neow source page extent')
            data.extend(raw[:used])
        fields.append(np.frombuffer(data,dtype='u1').reshape(h,w,channels))
    return fields


def convert(data, work, workers=2):
    from coalesce_tile_resources import view
    from ui12_resources import replace_images
    from tanx_scene_ui23 import group_images
    import compact_retained_slices as compact
    source=view(data);work=Path(work);work.mkdir(parents=True,exist_ok=True)
    t.require(source.version==46 and len(source.images)>=fire.IMAGES and source.aux[fire.BASE_AUX]==fire.PARAMETER,
              'Complete UI23 Neow catalog required')
    width,height,fmt,parameter=source.image_raw(fire.PARAMETER)
    t.require((width,fmt)==(256,0), 'Neow parameter format')
    info=fire.parameters(parameter)
    if info.get('baked_light'):
        return data,dict(complete=True,already_baked=True,saved_bytes=0,output_sha256=t.sha256(data))
    fields=source_fields(source);radiance=bake(parameter,fields)
    t.atomic_bytes(work/'original-parameters.bin',parameter)
    for i,field in enumerate(fields):t.atomic_bytes(work/f'original-field-{i}.bin',field.tobytes())
    t.atomic_bytes(work/'radiance.bin',radiance)
    header=bytearray(parameter[:fire.LENGTH]);struct.pack_into('<H',header,4,3)
    struct.pack_into('<H',header,34,fire.LENGTH);struct.pack_into('<2I',header,44,fire.BASE_IMAGES,len(radiance))
    header+=bytes((-len(header))%512);fire.parameters(header)
    raw=radiance+bytes((-len(radiance))%512)
    replacements={fire.BASE_IMAGES:(256,len(raw)//512,0,raw),fire.PARAMETER:(256,len(header)//512,0,bytes(header))}
    for i in range(1,fire.TEXTURE_PAGES):replacements[fire.BASE_IMAGES+i]=(1,1,0,bytes(2))
    rows=[];raw_replacements={}
    for image,(width,height,fmt,raw) in sorted(replacements.items()):
        path=work/f'{image}.raw';t.atomic_bytes(path,raw)
        row=dict(file=path.name,width=width,height=height,format=fmt,bytes=len(raw),sha256=t.sha256(raw),raw_sha256=t.sha256(raw))
        rows.append(row);raw_replacements[image]=dict(row,file=str(path))
    grouped=group_images(rows,work,group_bytes=128*1024);grouped['image_ids']=sorted(replacements)
    replaced,details=replace_images(data,{},raw_replacements=raw_replacements,replacement_art=grouped)
    before=view(replaced);choices=compact.make_plan(before,work/'compact',workers=workers,preserve_profile=True)
    output=compact.rewrite(before,choices);proof=compact.verify(before,output,choices);target=view(output)
    t.require(len(output)<len(data), 'Baked field must reclaim physical bytes')
    t.require(source.clips==target.clips and source.cards==target.cards and source.aux==target.aux,'Neow stable identities changed')
    for i in range(fire.FIRST_MASK,fire.PARAMETER):
        t.require(source.image_raw(i)==target.image_raw(i),'Neow animated transmission changed')
    t.require(target.image_raw(fire.BASE_IMAGES)[3][:len(radiance)]==radiance,'Neow radiance readback differs')
    result=dict(complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(output),input_bytes=len(data),output_bytes=len(output),saved_bytes=len(data)-len(output),
                radiance_sha256=t.sha256(radiance),radiance_bytes=len(radiance),source_clock_ms=0,transmission_frames_unchanged=fire.FRAMES,
                body_frames_unchanged=True,shared_tez_fields_unchanged=True,old_material_pages_replaced=fire.TEXTURE_PAGES,
                replacement=details,compaction=proof,choices=[x.receipt for x in choices])
    return output,result


def main():
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('input','output','work'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--workers',type=int,choices=(1,2),default=2)
    args=parser.parse_args()
    for path in (args.output,args.work):t.private_path(path,'Private Neow baked resources')
    t.require(args.input.resolve()!=args.output.resolve(),'Preserve input resources')
    output,proof=convert(args.input.read_bytes(),args.work,args.workers)
    t.atomic_bytes(args.output,output);t.atomic_bytes(args.output.with_suffix('.json'),(json.dumps(proof,indent=2)+'\n').encode())
    print(json.dumps({k:proof[k] for k in ('complete','saved_bytes','radiance_sha256','output_sha256')}),flush=True)

if __name__=='__main__':main()
