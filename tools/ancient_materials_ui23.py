#!/usr/bin/env python3
"""Source-bound native light plates and paged Ancient water samplers.

The six stationary light quads are rasterized at their original LCD projection,
with no texture downscale. Their source 256-sample alpha ramps remain separate.
Orobas water uses a three-sample-per-LCD-pixel texture and exact source R8 noise.
Tanx uses five native shader responses and the reviewed250x250R4 noise policy.
These are explicit projected sampling policies, not source-pixel identity claims.
Reduced water instead uses exact original-clock0 native shader poses. Every page
uses the existing retained format; only typed full-profile leaves hold residuals.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import struct
import numpy as np
import content_tiles as t

BASE_AUX,AUXILIARY,BASE_IMAGES,IMAGES=7217,7220,59677,59700
MAGIC=0x4d41
TANX_STORED_NOISE_SHA='c997eb056c521d274734fe5280dbc8b636baf2e7d40edc466f031c9095c61822'
SCALE=3195634/16777216
ORIGIN=np.array([-114518200509,89989748010],dtype=float)/(16777216*256)
SOURCES={
 'scenes/events/background_scenes/pael.tscn':'c68a425b3c53253abcc0bcafb26440999a00b4c9104e91f08d437e27aa990159',
 'scenes/events/background_scenes/orobas.tscn':'c2fff8b68795171ed775a1a8b10003a84dff84b00ae12c9b3aa6fba92757ad74',
 'scenes/events/background_scenes/tanx.tscn':'35eb903ce9bb1f1fb48bcdbcb83d614eb3581b3a210ec7c538ace08a8ffc92a2',
 'shaders/vfx/vfx_water_reflection.gdshader':'cfd5ba28dcfd923c2bf72f11ad941475b737802f1dd74e6efd2861075d79c4ff',
 'themes/canvas_item_material_additive_shared.tres':'cb02b1e0f9bfd93eeadfc83e2b4ea5e34fba4db37d40cde5fc275c994d902177'}
TEXTURES=(
 ('images/packed/vfx/event/trial_stand_light.png',(1251,2400),'82cd1ba827603463857f3204a9079278b7c62024f065e861a898ddf9cc8439fc'),
 ('images/ancients/orobas_placeholder.png',(2560,1200),'c7e48b3fa86bec6b5432241207d1898bc63e5187bd7a37d8049c7665694d4dfc'),
 ('images/ancients/tanx_placeholder.png',(2560,1200),'f08306640f359d298ec501a92ac3a4d76cae8bbc512029e16df35b40646b3364'),
 ('images/vfx/fire/triangle_noise_tile.png',(500,500),'5c5385b4903378661a1c605411d577e77669272fe21f84ef3e96f33064665d71'),
 ('images/rooms/underdocks/underdocks_00.png',(2048,960),'5e42a0d9cb8f29e54e87794909f41a7e0e1ca10502dae7f2c17d45200a27cdf6'))
# Authored world positions/rotation/node scale/modulate, then original blend.
# All four Pael nodes resolve CanvasItemMaterial_cwn76, blend_mode=1.
LIGHTS=(
 (1579,302,-.141372,1,1,(1,.705882,.301961,.333333),1),
 (293,348,.233874,1,1,(1,.705882,.301961,.333333),1),
 (614,450,0,.5,1,(1,.705882,.301961,.556863),1),
 (1508,418,-.253073,.25,1,(.521569,.470588,.207843,.639216),1),
 (365,288,.416432,1,1,(.454902,1,.788235,.419608),1),
 (1569,370,-.231258,1,1,(.454902*.90972,.207225,.788235*.89416,.419608),1))
OFFSETS=((0,.0446194,.178478,.317585,.611354,.720524,.889764,1),
         (0,.170306,.286026,.423581,.611354,.720524,.847162,1))
ALPHAS=(0,1,.513726,1,1,.513726,1,0)
# Tables: Pael1024B; Orobas4096B with50x50 noise; Tanx1024B plus two31250B expandedR4 noise pages.
PARAMETER_SHAPES=((256,2),(256,8),(256,2))
DRAWS=((232,0,88,199,1,0,0),(0,0,71,211,1,0,88),(61,0,59,221,1,0,159),
       (232,0,72,215,1,2,0),(0,0,93,203,1,2,72),(236,0,84,215,1,2,165))
NOISE_SHA=('cab7f7059a3552d3dd3f7275a989f36f8a3e8bbd8bfbeaad1f08649e72dc756b',
           '0be3be8229f059ca2ae26dc875adc8f65b6b850e39de94dec1b967431caf40c3')


def bilinear(a,u,v,repeat=False):
    h,w=a.shape[:2];x=u*w-.5;y=v*h-.5
    ix=np.floor(x).astype(np.int64);iy=np.floor(y).astype(np.int64)
    fx=x-ix;fy=y-iy
    if repeat:x0=ix%w;x1=(ix+1)%w;y0=iy%h;y1=(iy+1)%h
    else:x0=np.clip(ix,0,w-1);x1=np.clip(ix+1,0,w-1);y0=np.clip(iy,0,h-1);y1=np.clip(iy+1,0,h-1)
    if a.ndim==3:fx=fx[...,None];fy=fy[...,None]
    return (a[y0,x0]*(1-fx)+a[y0,x1]*fx)*(1-fy)+(a[y1,x0]*(1-fx)+a[y1,x1]*fx)*fy


def rgb565(a):
    a=np.clip(np.floor(a+.5),0,255).astype('u2')
    return ((a[...,0]>>3)<<11)|((a[...,1]>>2)<<5)|(a[...,2]>>3)


def ramps():
    # Godot GradientTexture1D non-HDR image truncates sampled normalized RGBA.
    return tuple(np.uint8(np.interp(np.arange(256)/255,offsets,ALPHAS)*255).tobytes() for offsets in OFFSETS)


def native_lights(texture):
    y,x=np.mgrid[:240,:320];a=texture.astype(float);crops=[];records=[]
    for cx,cy,angle,sx,sy,tint,blend in LIGHTS:
        dx=x+.5-(cx*SCALE+ORIGIN[0]);dy=y+.5-(cy*SCALE+ORIGIN[1])
        cs,sn=np.cos(angle),np.sin(angle)
        u=.5+(dx*cs+dy*sn)/(1251*.5*sx*SCALE)
        v=.5+(-dx*sn+dy*cs)/(2400*.5*sy*SCALE)
        value=bilinear(a,u,v)*np.array(tint)
        value[(u<0)|(u>=1)|(v<0)|(v>=1)]=0
        alpha=np.clip(np.floor(value[:,:,3]+.5),0,255).astype('u1')
        yy,xx=np.nonzero(alpha)
        t.require(len(xx)>0,'An authored light disappeared from the LCD')
        left,top,right,bottom=int(xx.min()),int(yy.min()),int(xx.max()+1),int(yy.max()+1)
        crop=np.zeros((bottom-top,right-left,3),dtype='u1')
        colors=rgb565(value[top:bottom,left:right,:3])
        crop[:,:,0]=(colors&255).astype('u1');crop[:,:,1]=(colors>>8).astype('u1');crop[:,:,2]=alpha[top:bottom,left:right]
        records.append((left,top,right-left,bottom-top,blend));crops.append(crop)
    # Two native atlases, each split into112-row cache views. Fixed source
    # geometry is checked before packing, so no skyline/order heuristic varies.
    atlases=[np.zeros((224,256,3),dtype='u1') for _ in range(2)];draws=[]
    for i,(crop,record) in enumerate(zip(crops,records)):
        atlas=i//3;x=sum(c.shape[1] for c in crops[atlas*3:i]);h,w=crop.shape[:2]
        t.require(x+w<=256 and h<=224,'Authored light crops exceed reviewed native atlas')
        atlases[atlas][:h,x:x+w]=crop
        draws.append((*record,atlas*2,x))
    images=[]
    for atlas in atlases:
        for y in (0,112):
            a=atlas[y:y+112];raw=a[:,:,:2].copy().tobytes()+a[:,:,2].tobytes()
            images.append((256,112,1,raw))
    t.require(tuple(draws)==DRAWS,'Native light projection differs from reviewed source geometry')
    return images,tuple(draws)


def water_mapping(scene):
    # SourceTextureRect must remain2582x1221 with these authored origins.
    left,top=(-330,-49) if scene==0 else (-331,-39)
    strength=(23,2)[scene]
    step=np.array([1/(SCALE*2582),1/(SCALE*1221)])
    uv0=(np.array([.5,.5])-ORIGIN)/SCALE/np.array([2582,1221])-np.array([left/2582,top/1221])
    lo=uv0-strength/2000
    hi=uv0+step*np.array([319,239])
    return uv0,step,lo,hi


def water_images(texture,scene):
    _,_,lo,hi=water_mapping(scene);y,x=np.mgrid[:720,:960]
    pixels=rgb565(bilinear(texture[:,:,:3].astype(float),lo[0]+(x+.5)/960*(hi[0]-lo[0]),lo[1]+(y+.5)/720*(hi[1]-lo[1])))
    return [(320,240,0,pixels[y:y+240,x:x+320].astype('<u2').tobytes()) for y in range(0,720,240) for x in range(0,960,320)]


def residual_mapping(scene):
    uv0,step,lo,hi=water_mapping(scene)
    increment=(hi-lo)/np.array([960,720])/step
    origin=(lo-uv0)/step+increment*.5
    return tuple(int(np.floor(x*65536+.5)) for x in (*origin,*increment))


def predicted_base(base,mapping):
    # Exact production integer predictor; 8-bit fractions and two rounded
    # bilinear passes. Quantization of the map does not create output error:
    # each encoded residual is measured against this same integer prediction.
    y,x=np.mgrid[:720,:960];ox,oy,dx,dy=mapping;xx=ox+x*dx;yy=oy+y*dy
    left=np.clip(xx>>16,0,319);right=np.clip((xx>>16)+1,0,319)
    top=np.clip(yy>>16,0,239);bottom=np.clip((yy>>16)+1,0,239)
    fx=((xx&65535)>>8)[...,None];fy=((yy&65535)>>8)[...,None]
    b=channels(base)
    a=(b[top,left]*(256-fx)+b[top,right]*fx+128)>>8
    c=(b[bottom,left]*(256-fx)+b[bottom,right]*fx+128)>>8
    return (a*(256-fy)+c*fy+128)>>8


def channels(colors):
    return np.stack([colors>>11,(colors>>5)&63,colors&31],axis=-1).astype(np.int32)


def residual_pages(rows,base,scene):
    original=np.empty((720,960),dtype='<u2')
    for i,(_,_,_,raw) in enumerate(rows):original[i//3*240:(i//3+1)*240,i%3*320:(i%3+1)*320]=np.frombuffer(raw,dtype='<u2').reshape(240,320)
    prediction=predicted_base(base,residual_mapping(scene));values=channels(original)
    quantum=np.array([3,5,3]);delta=np.floor((values-prediction)/quantum+.5).astype(np.int32)*quantum
    restored=np.clip(prediction+delta,[0,0,0],[31,63,31]);delta=restored-prediction
    error=np.abs(values-restored);t.require(np.all(error<=np.array([1,2,1])),'Water residual color bound exceeded')
    encoded=(((delta[:,:,0]&31)<<11)|((delta[:,:,1]&63)<<5)|(delta[:,:,2]&31)).astype('<u2')
    recovered=(prediction+channels(encoded))&np.array([31,63,31])
    t.require(np.array_equal(recovered,restored),'Water modular residual is not exact after clamping')
    result=[(320,240,0,encoded[y:y+240,x:x+320].copy().tobytes()) for y in range(0,720,240) for x in range(0,960,320)]
    return result,dict(base_rgb565_sha256=t.sha256(base.astype('<u2').tobytes()),mapping=residual_mapping(scene),
        maximum_sampler_channel_error=error.max(axis=(0,1)).tolist(),reconstruction_exact=True,
        sampler_rgb565_sha256=t.sha256(original.tobytes()),
        reconstructed_rgb565_sha256=t.sha256(((restored[:,:,0]<<11)|(restored[:,:,1]<<5)|restored[:,:,2]).astype('<u2').tobytes()))




def exact_entry_pose(texture,noise,scene):
    # Source TIME=0 is explicit; an unrecorded historical GPU epoch is not
    # invented. Keep the original noise top-level pixels and full native image.
    y,x=np.mgrid[:240,:320];uv0,step,_,_=water_mapping(scene)
    u=uv0[0]+x*step[0];v=uv0[1]+y*step[1]
    n=bilinear(noise.astype(float),u,v,True)/255
    shift=(n-1)*(23 if scene==0 else 2)/2000
    colors=rgb565(bilinear(texture[:,:,:3].astype(float),u+shift,v+shift)).astype('<u2')
    alpha=np.where((u>=0)&(u<1)&(v>=0)&(v<1),255,0).astype('u1');colors[alpha==0]=0
    return 320,240,1,colors.tobytes()+alpha.tobytes()

def response_pages(texture,base):
    y,x=np.mgrid[:240,:320];uv0,step,_,_=water_mapping(1)
    u=uv0[0]+x*step[0];v=uv0[1]+y*step[1];b=channels(base)
    frames=[None]*5;rows=[None]*5;proof=[]
    for index in (0,4,2,1,3):
        original=channels(rgb565(bilinear(texture[:,:,:3].astype(float),u+(index/4-1)*.001,v+(index/4-1)*.001)))
        if index in (0,4):prediction=b;parents=[]
        else:
            gap=2 if index==2 else 1;parents=[index-gap,index+gap]
            prediction=(frames[index-gap]+frames[index+gap]+1)//2
        quantum=np.array([3,5,3]);delta=np.floor((original-prediction)/quantum+.5).astype(np.int32)*quantum
        restored=np.clip(prediction+delta,[0,0,0],[31,63,31]);delta=restored-prediction
        t.require(np.all(np.abs(restored-original)<=np.array([1,2,1])),'Tanx response color bound exceeded')
        encoded=(((delta[:,:,0]&31)<<11)|((delta[:,:,1]&63)<<5)|(delta[:,:,2]&31)).astype('<u2')
        t.require(np.array_equal((prediction+channels(encoded))&np.array([31,63,31]),restored),'Tanx response modular reconstruction differs')
        frames[index]=restored;rows[index]=(320,240,0,encoded.tobytes())
        proof.append(dict(index=index,parents=parents,source_rgb565_sha256=t.sha256(((original[:,:,0]<<11)|(original[:,:,1]<<5)|original[:,:,2]).astype('<u2').tobytes()),
            restored_rgb565_sha256=t.sha256(((restored[:,:,0]<<11)|(restored[:,:,1]<<5)|restored[:,:,2]).astype('<u2').tobytes())))
    return rows,dict(base_rgb565_sha256=t.sha256(base.astype('<u2').tobytes()),responses=proof,reconstruction_exact=True,
        maximum_response_channel_error=[1,2,1],projected_shader_max_observed_error=[3,4,2],
        policy='Five320x240 source shader responses; cold-only hierarchical residuals;250x250R4 noise')

def parameters(scene,draws,noise=b'',base_image=0,base_sha256=None,static=False):
    t.require(scene in (0,1,2),'Unknown material scene')
    w,h=PARAMETER_SHAPES[scene];raw=bytearray(w*h*2)
    first,count=(0,4) if scene==0 else (4,2) if scene==1 else (0,0)
    struct.pack_into('<16H',raw,0,MAGIC,1,scene,BASE_IMAGES,4,BASE_IMAGES+(4 if scene==1 else 13),9 if scene==1 else 5,
                     BASE_IMAGES+18,BASE_IMAGES+19,count,10000,2000,256,112,960 if scene==1 else 320,720 if scene==1 else 240)
    for i,draw in enumerate(draws[first:first+count]):struct.pack_into('<7H',raw,32+i*14,*draw)
    raw[128:384]=ramps()[int(scene==1)]
    if scene:
        uv0,step,lo,hi=water_mapping(scene-1)
        # Q24 retains source UV and maximum-displacement halo below1/100 LCDpx.
        values=np.concatenate([uv0,step,lo,1/(hi-lo)])
        struct.pack_into('<8i',raw,384,*(int(np.floor(v*16777216+.5)) for v in values))
        struct.pack_into('<4H',raw,416,23 if scene==1 else 40,23 if scene==1 else 2,50 if scene==1 else 250,50 if scene==1 else 250)
        t.require(0<=base_image<BASE_IMAGES and base_sha256 is not None and len(base_sha256)==32,'Water residual needs its explicit earlier base identity')
        struct.pack_into('<4i2H',raw,424,*residual_mapping(scene-1),base_image,0x5350 if static else 0x3152 if scene==1 else 0x3252)
        raw[448:480]=base_sha256
    if scene==1:
        t.require(len(noise)==2500,'Orobas must preserve all50x50 FastNoise samples')
        raw[512:3012]=noise
    return bytes(raw)


def source_images(pack,header,work,source,no_animations=False):
    from build_content import texture
    from random_character_ui13_assets import godot_noise_image
    for name,digest in SOURCES.items():t.require(t.sha256(pack.read(name))==digest,'Authored material source changed: '+name)
    arrays=[]
    for name,size,digest in TEXTURES:
        image=texture(pack,name).convert('RGBA');a=np.asarray(image,dtype='u1')
        t.require(image.size==size and t.sha256(a.tobytes())==digest,'Authored material texture changed: '+name)
        arrays.append(a)
    t.require(np.all(arrays[4][:,:,3]==255),'The water mask is no longer source-constant opaque')
    t.require(all(np.all(a[:,:,3]==255) for a in arrays[1:3]),'The authored water source textures must remain opaque')
    # The shader reads only .r; other original channels are deliberately unused.
    from world_scene_assets import _sections,_property,SceneLoader
    loader=SceneLoader(pack)
    for room,count in (('pael',4),('orobas',2)):
        name='scenes/events/background_scenes/'+room+'.tscn'
        nodes={a['name']:props for kind,a,props in _sections(pack.read(name).decode('utf8'),name) if kind=='node'}
        for index in range(count):
            node=nodes['light_overlay'+(str(index+1) if index else '')]
            _,material=loader.resource(_property(node,'material'))
            t.require(_property(material,'blend_mode')==1,'Authored stationary overlay blend differs')
    generated=np.asarray(godot_noise_image(50,50,.0259,.093,Path(header),Path(work)/'noise'),dtype='u1')[:,:,3].tobytes()
    lights,draws=native_lights(arrays[0]);images=list(lights);base_records=[];residual_proofs=[]
    from merchant_ui14_assets import pixel_planes
    for scene,auxiliary in enumerate((366,385)):
        base_id=source.aux[auxiliary];base,alpha=pixel_planes(source.image_raw(base_id))
        t.require(base.shape==(240,320),'Water predictor must preserve the original native scene dimensions')
        if no_animations:
            original_noise=np.frombuffer(generated,dtype='u1').reshape(50,50) if scene==0 else arrays[3][:,:,0]
            row=exact_entry_pose(arrays[scene+1],original_noise,scene)
            rows=[row]*(9 if scene==0 else 5)
            proof=dict(base_rgb565_sha256=t.sha256(base.astype('<u2').tobytes()),entry_pose_raw_sha256=t.sha256(row[3]),exact_source_clock_ms=0,static=True)
        else:rows,proof=residual_pages(water_images(arrays[scene+1],scene),base,scene) if scene==0 else response_pages(arrays[2],base)
        images.extend(rows);base_records.append((base_id,bytes.fromhex(proof['base_rgb565_sha256'])))
        residual_proofs.append(proof)
    original_noise=arrays[3][:,:,0]
    y,x=np.mgrid[:250,:250]
    samples=bilinear(original_noise.astype(float),(x+.5)/250,(y+.5)/250,True)
    noise=(np.floor(samples/17+.5).clip(0,15).astype('u1')*17).tobytes()
    for start in (0,31250):images.append((125,125,0,noise[start:start+31250]))
    t.require((t.sha256(generated),t.sha256(original_noise.tobytes()))==NOISE_SHA,'Source water noise channels changed')
    for scene in range(3):
        w,h=PARAMETER_SHAPES[scene];images.append((w,h,0,parameters(scene,draws,generated if scene==1 else b'',*(base_records[scene-1] if scene else (0,None)),static=no_animations)))
    t.require(len(images)==23,'Material append cardinality differs')
    return images,dict(light_native_crops=draws,noise_sha256=[t.sha256(generated),t.sha256(original_noise.tobytes())],tanx_stored_noise_sha256=t.sha256(noise),
        water_policy='Orobas960x720 bounded residual sampler;Tanx5native responses and250R4 noise; continuous source clock',
        water_original_epoch_known=False,light_unknown_original_seed=False,residuals=residual_proofs,static_entry_pose=no_animations)


def parameter_refs(raw,scene):
    expected=parameters(scene,DRAWS,raw[512:3012] if scene==1 else b'',struct.unpack_from('<H',raw,440)[0] if scene else 0,raw[448:480] if scene else None,static=bool(scene and struct.unpack_from('<H',raw,442)[0]==0x5350))
    t.require(raw==expected,'Material typed refs/geometry/ramp/padding differ')
    if scene==1:t.require(t.sha256(raw[512:3012])==NOISE_SHA[0],'Orobas original FastNoise values changed')
    refs=(tuple(range(BASE_IMAGES,BASE_IMAGES+4)) if scene<2 else ())
    if scene:refs+=tuple(range(BASE_IMAGES+(4 if scene==1 else 13),BASE_IMAGES+(13 if scene==1 else 20)))
    return refs


def require_reduced_source(source,image):
    """Reject approximate animated samplers before the generic pose reducer.

    The normal producer supplies exact original-clock0 material poses when the
    CLI requests no animations. A sampler cannot recover that exact pose after
    its bounded color/noise transformation, so reduction must never relabel it.
    Call after the complete typed schema has been validated.
    """
    for scene in (1,2):
        raw=image(source.aux[BASE_AUX+scene])[3]
        t.require(struct.unpack_from('<H',raw,442)[0]==0x5350,
                  'Reduced water needs original-source --no-animations material poses; animated samplers cannot be relabeled')


def validate(source,image,final=False):
    t.require((len(source.aux),len(source.images)) in ((AUXILIARY,IMAGES),(7221,59710)),
              'Materials require an explicit complete typed catalog')
    refs=[];static_modes=[]
    for scene,(w,h) in enumerate(PARAMETER_SHAPES):
        index=BASE_IMAGES+20+scene
        t.require(source.aux[BASE_AUX+scene]==index,'Material table is not its reviewed auxiliary root')
        iw,ih,fmt,raw=image(index)
        t.require((iw,ih,fmt)==(w,h,0),'Material parameters must remain bounded literal words')
        refs.extend(parameter_refs(raw,scene));refs.append(index)
        if scene:
            static_modes.append(struct.unpack_from('<H',raw,442)[0]==0x5350)
            from merchant_ui14_assets import pixel_planes
            base_id=struct.unpack_from('<H',raw,440)[0]
            t.require(base_id==source.aux[(366,385)[scene-1]],'Water source base reference differs')
            base,alpha=pixel_planes(image(base_id))
            t.require(base.shape==(240,320) and t.sha256(base.astype('<u2').tobytes())==raw[448:480].hex(),'Water predictor base pixels differ')
    t.require(static_modes[0]==static_modes[1],'Water rooms must use one coherent material profile')
    if final:
        profile=struct.unpack_from('<I',source.data,36)[0] if hasattr(source,'data') else None
        t.require(profile is not None and static_modes[0]==(profile==0x3146414e),'Final water static profile does not match NAF1')
    t.require(set(refs)==set(range(BASE_IMAGES,IMAGES)),'Material references miss an appended image')
    for i in range(20):
        w,h,fmt,raw=image(BASE_IMAGES+i)
        shape=(256,112,1) if i<4 else (320,240,1 if static_modes[0] else 0) if i<18 else (125,125,0)
        t.require((w,h,fmt)==shape and len(raw)==w*h*(3 if fmt else 2),'Material sampler page dimensions/format differ')
    if static_modes[0]:
        for first,count in ((4,9),(13,5)):
            expected=image(BASE_IMAGES+first)
            t.require(all(image(BASE_IMAGES+first+i)==expected for i in range(count)),'Static water pose aliases differ')
    noise=b''.join(image(BASE_IMAGES+i)[3] for i in (18,19))
    t.require(t.sha256(noise)==TANX_STORED_NOISE_SHA,'Tanx reviewed250R4 noise pages changed')
    return tuple(sorted(set(refs)))


def append(data,pack,header,work,no_animations=False):
    from coalesce_tile_resources import view
    from patch_frontend_assets import patch_pack
    from split_content import LANGUAGES
    from no_animations import reduced,mark
    from tanx_scene_ui23 import group_images
    source=view(data)
    t.require(source.version==46 and (len(source.aux),len(source.images),len(source.clips))==
              (BASE_AUX,BASE_IMAGES,1783),'Material append requires complete source-authored Ancient lights')
    cache=Path(work);cache.mkdir(parents=True,exist_ok=True)
    images,witness=source_images(pack,header,cache,source,no_animations);rows=[]
    for i,(w,h,fmt,raw) in enumerate(images):
        name=f'{i}.bin';t.atomic_bytes(cache/name,raw)
        rows.append(dict(width=w,height=h,format=fmt,file=name,bytes=len(raw),sha256=t.sha256(raw),raw_sha256=t.sha256(raw)))
    art=group_images(rows,cache,group_bytes=256*1024)
    manifest=dict(images=rows,auxiliary_image_ids=list(range(BASE_IMAGES+20,IMAGES)),localized_text={code:{} for code in LANGUAGES})
    output,details=patch_pack(data,manifest,cache,base_aux=BASE_AUX,auxiliary=AUXILIARY,
        input_version=46,version=46,texts=(),base_clips=len(source.clips),base_strings=len(source.strings),appended_art=art)
    if reduced(data):output=mark(output)
    target=view(output);validate(target,target.image_raw)
    t.require(target.clips==source.clips and target.cards==source.cards and target.aux[:BASE_AUX]==source.aux and
              reduced(output)==reduced(data),'Material append changed previous catalog or animation profile')
    for i,expected in enumerate(images):t.require(target.image_raw(BASE_IMAGES+i)==expected,'Material group decode changed source-derived page')
    return output,dict(complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(output),
        added_bytes=len(output)-len(data),source_files=SOURCES,textures=TEXTURES,
        original_mask_constant_opaque_verified=True,source_witness=witness,image_raw_sha256=[t.sha256(r[3]) for r in images],
        decoded_bytes=sum(len(r[3]) for r in images),decoded_pages=sum((len(r[3])+4095)//4096 for r in images),
        grouping=art['report'],append=details,previous_catalog_exact=True,profile_preserved=True,
        new_framebuffer_bytes=0,new_particle_pool_bytes=0,storage_quality_acceptance_pending=True)


def main():
    from pck import Pack
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('input','output','pck','noise-header','work'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--no-animations',action='store_true',help='Keep exact source clock0 water poses for the reduced profile')
    a=p.parse_args()
    for path in (a.input,a.output,a.pck,a.work):t.private_path(path,'Private original material data')
    t.require(a.input.resolve()!=a.output.resolve(),'Preserve original material input')
    pack=Pack(a.pck)
    try:output,proof=append(a.input.read_bytes(),pack,a.noise_header,a.work,a.no_animations)
    finally:pack.close()
    t.atomic_bytes(a.output,output);t.atomic_bytes(a.output.with_suffix('.json'),(json.dumps(proof,indent=2)+'\n').encode())
    print(json.dumps(dict(complete=True,added_bytes=proof['added_bytes'],output_sha256=proof['output_sha256'])),flush=True)

if __name__=='__main__':main()
