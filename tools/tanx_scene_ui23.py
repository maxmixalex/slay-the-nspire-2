#!/usr/bin/env python3
"""Append source-exact Tanx grain masks, scrolling smoke pages and parameters.

Only the original v0.111.0 scene/shader/decoded textures are accepted. The fog
sheet provides authored quad dimensions; its pixels are not sampled by this
shader. Six explicit image references and one bounded parameter image use the
existing retained-group format. No renderer/cache capacity is increased.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import struct
import numpy as np
import content_tiles as t

BASE_AUX,AUXILIARY,BASE_IMAGES,IMAGES=7214,7215,59667,59674
MAGIC=0x5458
SCENE='scenes/events/background_scenes/tanx.tscn'
SHADER='shaders/scrolling_smoke.tres'
SOURCES={SCENE:'35eb903ce9bb1f1fb48bcdbcb83d614eb3581b3a210ec7c538ace08a8ffc92a2',
         SHADER:'2cbce4cbe24ccaef8a6f7ab61c8f070dafdd62a4f13fd162ba53d012fca33f3d'}
TEXTURES=(
 ('images/vfx/short_grain_rice_particle.png',(32,32),'c44b62625f7ceb06fe0201d7a03fe4ad0f63f79ce96870ea67a4575f0ad4719c'),
 ('images/vfx/short_rice_no_glow_particle.png',(8,16),'267771f7ea11d66b962353447f0b7761a5f592f400aa0ade1bbdecac9d5a1dbe'),
 ('images/packed/vfx/event/tanx_smoke.png',(200,800),'4c7a5642ee2628312c8d4fcfa1585ddbb99476d2d3ae55216b2496e3267321fc'),
 ('images/vfx/monsters/lost_and_forgotten/lost_smoke_gradient.png',(10,200),'d56a4036f5de0064ef5629388f63ea5203c3d8527cccaa349f7196558bd40e05'))
HEADER=(MAGIC,1,*range(BASE_IMAGES,BASE_IMAGES+6),200,800,200,2472,3316,200,256,256)
PARAMETER_BYTES=256*129*2


def smooth_lut():
    # Exact nearest integer for 255*smoothstep(0,1,(a/255)*(b/255)).
    d=65025**3
    return bytes((255*(a*b)**2*(195075-2*a*b)+d//2)//d
                 for a in range(256) for b in range(256))


def parameter_bytes(gradient):
    t.require(len(gradient)==200,'Tanx gradient must contain every original R8 row')
    raw=struct.pack('<16H',*HEADER)+bytes(gradient)+smooth_lut()
    return raw+bytes(PARAMETER_BYTES-len(raw))


def parameters(raw,image_count):
    t.require(image_count in (IMAGES,59677) and len(raw)==PARAMETER_BYTES,
              'Tanx parameter size/catalog differs')
    t.require(struct.unpack_from('<16H',raw)==HEADER,'Tanx typed references or geometry differs')
    t.require(raw[232:232+65536]==smooth_lut() and not any(raw[232+65536:]),
              'Tanx smoothstep lookup or padding differs')
    return dict(images=tuple(range(BASE_IMAGES,BASE_IMAGES+6)),table=BASE_IMAGES+6,
                gradient=raw[32:232],lookup=raw[232:232+65536])


def validate(source,image):
    t.require((len(source.aux),len(source.images)) in ((AUXILIARY,IMAGES),(7217,59677),(7220,59700)) and
              source.aux[BASE_AUX]==BASE_IMAGES+6,'Tanx requires the exact complete typed extension')
    w,h,fmt,raw=image(BASE_IMAGES+6)
    t.require((w,h,fmt)==(256,129,0),'Tanx parameters must remain literal words')
    row=parameters(raw,len(source.images))
    for index,shape in zip(row['images'],((32,32),(8,16),*((200,200),)*4)):
        w,h,fmt,raw=image(index)
        t.require((w,h,fmt)==(*shape,1) and len(raw)==w*h*3 and
                  raw[:w*h*2]==b'\xff'*(w*h*2),'Tanx source masks must keep white RGB565 and alpha8')
    return row


def source_images(pack):
    from build_content import texture
    for name,digest in SOURCES.items():
        t.require(t.sha256(pack.read(name))==digest,'Tanx original scene/shader changed: '+name)
    arrays=[]
    for name,shape,digest in TEXTURES:
        im=texture(pack,name).convert('RGBA');a=np.asarray(im,dtype='u1')
        t.require(im.size==shape and t.sha256(a.tobytes())==digest,
                  'Tanx original texture pixels changed: '+name)
        arrays.append(a)
    t.require(all(np.all(a[:,:,:3]==255) for a in arrays[:3]),'Tanx mask RGB is not source-white')
    gradient=arrays[3][:,:,0]
    t.require(np.all(gradient==gradient[:,:1]),'Tanx gradient columns no longer agree')
    pages=[arrays[0],arrays[1],*(arrays[2][i:i+200] for i in range(0,800,200))]
    result=[]
    for a in pages:
        h,w=a.shape[:2];result.append((w,h,1,b'\xff'*(w*h*2)+a[:,:,3].tobytes()))
    result.append((256,129,0,parameter_bytes(gradient[:,0].tobytes())))
    return result


def group_images(rows,cache,group_bytes=128*1024):
    from group_ui7_hands import RowImages
    from repack_retained import DESCRIPTOR,plan_images
    payloads=[(cache/row['file']).read_bytes() for row in rows]
    source=RowImages(rows,payloads)
    t.require(group_bytes in (128*1024,256*1024),'Reviewed retained group target must remain128/256KiB')
    batches=plan_images(source,group_bytes=group_bytes,fast_images='none')
    payload=bytearray();entries=[None]*len(rows);maximum=groups=0
    def add(raw):
        payload.extend(bytes(len(payload)&1));at=len(payload);payload.extend(raw);return at
    for batch in batches:
        if batch.encoded is None:
            for record in batch.records:
                at=add(record.payload)
                for i in record.ids:entries[i]=(at,record.stored_format,len(record.payload))
            continue
        group=t.GROUP.pack(b'RGR1',batch.raw_size,len(batch.encoded),14,len(batch.records),0,0,0)+batch.encoded
        group_at=add(group);raw_at=0
        for record in batch.records:
            at=add(b'');payload.extend(DESCRIPTOR.pack(group_at-at,len(group),raw_at,(record.fmt<<24)|len(record.raw)))
            for i in record.ids:entries[i]=(at,7,DESCRIPTOR.size)
            raw_at+=len(record.raw)
        groups+=1;maximum=max(maximum,batch.raw_size)
    payload.extend(bytes(len(payload)&1))
    t.require(all(row is not None for row in entries) and maximum<=group_bytes,'Retained group exceeds its reviewed bound')
    return dict(payload=bytes(payload),entries=entries,decoded_sizes=[len(raw) for raw in payloads],
                report=dict(groups=groups,maximum_group_bytes=maximum,stored_bytes=len(payload)))


def append(data,pack,work):
    from coalesce_tile_resources import view
    from patch_frontend_assets import patch_pack
    from split_content import LANGUAGES
    from no_animations import reduced,mark
    source=view(data)
    t.require(source.version==46 and (len(source.aux),len(source.images),len(source.clips))==
              (BASE_AUX,BASE_IMAGES,1783),'Tanx append requires complete original Ancient NSP2/46')
    cache=Path(work);cache.mkdir(parents=True,exist_ok=True);images=source_images(pack);rows=[]
    for i,(w,h,fmt,raw) in enumerate(images):
        name=f'{i}.bin';t.atomic_bytes(cache/name,raw)
        rows.append(dict(width=w,height=h,format=fmt,file=name,bytes=len(raw),sha256=t.sha256(raw),raw_sha256=t.sha256(raw)))
    art=group_images(rows,cache)
    manifest=dict(images=rows,auxiliary_image_ids=[BASE_IMAGES+6],localized_text={code:{} for code in LANGUAGES})
    output,details=patch_pack(data,manifest,cache,base_aux=BASE_AUX,auxiliary=AUXILIARY,
        input_version=46,version=46,texts=(),base_clips=len(source.clips),base_strings=len(source.strings),appended_art=art)
    if reduced(data):output=mark(output)
    target=view(output);validate(target,target.image_raw)
    t.require(target.clips==source.clips and target.cards==source.cards and target.aux[:BASE_AUX]==source.aux and
              reduced(output)==reduced(data),'Tanx append changed old catalog or animation profile')
    for i,original in enumerate(images):t.require(target.image_raw(BASE_IMAGES+i)==original,'Tanx stored pixels differ from original')
    from animation_color_candidates_ui23 import protected_images
    t.require(set(range(BASE_IMAGES,IMAGES))<=protected_images(target),'Tanx textures must be protected from color approximation')
    return output,dict(complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(output),
        input_bytes=len(data),output_bytes=len(output),added_bytes=len(output)-len(data),
        source_files=SOURCES,textures=[dict(path=n,size=size,rgba_sha256=h) for n,size,h in TEXTURES],
        image_raw_sha256=[t.sha256(row[3]) for row in images],decoded_bytes=sum(len(row[3]) for row in images),
        parameter_decoded_bytes=PARAMETER_BYTES,parameter_pages=(PARAMETER_BYTES+4095)//4096,
        decoded_image_pages=sum((len(row[3])+4095)//4096 for row in images),
        grouping=art['report'],append=details,profile_preserved=True,framebuffer_growth=0,particle_pool_growth=0,
        lookup='exact integer alpha8-pair smoothstep; runtime subtexel interpolation is separately bounded',
        shader_clock='continuous visual clock modulo 10s; original process epoch is unknown')


def main():
    from pck import Pack
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('input','output','pck','work'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args()
    for path in (a.input,a.output,a.pck,a.work):t.private_path(path,'Private Tanx input/output')
    t.require(a.input.resolve()!=a.output.resolve(),'Preserve the Tanx input pack')
    pack=Pack(a.pck)
    try:output,proof=append(a.input.read_bytes(),pack,a.work)
    finally:pack.close()
    t.atomic_bytes(a.output,output);t.atomic_bytes(a.output.with_suffix('.json'),(json.dumps(proof,indent=2)+'\n').encode())
    print(json.dumps(dict(complete=True,added_bytes=proof['added_bytes'],output_sha256=proof['output_sha256'])),flush=True)

if __name__=='__main__':main()
