#!/usr/bin/env python3
"""Source-bound four-frame alpha atlas and curves for Nonupeipe's six fog nodes.

The original sand-cloud texture has white RGB. Keeping its exact alpha in four
200x200 byte planes avoids redundant white color storage. Nothing is resized.
The original CurveTexture lookup grids and non-HDR gradient bytes are sampled
once; animation, affine transforms and per-particle life remain runtime work.
"""
from pathlib import Path
import json, struct
import numpy as np
import content_tiles as t
from world_scene_assets import _sections, _property, SceneLoader

SCENE='scenes/events/background_scenes/nonupeipe.tscn'
SCENE_SHA='29f03f3684d0c441a2d29e3078f21715519414d24cea77d9fffddd47a56ecc3e'
TEXTURE='images/vfx/monsters/the_insatiable/sand_cloud.png'
NODES=('fog3','fog4','fog5','fog6','fog7','fog8')
BASE_AUX,AUXILIARY,BASE_IMAGES,IMAGES=7222,7223,59712,59717
PARAMETER_BYTES=2048


def assets(pack):
    from build_content import texture
    from world_room_shaders import curve_channel, generated_texture
    raw=pack.read(SCENE)
    t.require(t.sha256(raw)==SCENE_SHA,'Nonupeipe source scene differs')
    sections=_sections(raw.decode('utf8'),SCENE)
    nodes=[(a,p) for k,a,p in sections if k=='node']
    # The sole static plate precedes every particle. There is no later opaque
    # actor/foreground for the fog to cross when composited on the room plate.
    t.require([a['name'] for a,p in nodes[:8]]==['Nonuepeipe','TextureRect',*NODES],
              'Nonupeipe fog/static source order differs')
    t.require(sum(a['type']=='TextureRect' for a,p in nodes)==1,
              'Nonupeipe gained an opaque foreground layer')
    props={a['name']:p for a,p in nodes};loader=SceneLoader(pack)
    counts=(5,5,7,7,10,10);lifetimes=(15,15,15,15,30,30)
    bindings=[]
    for index,name in enumerate(NODES):
        p=props[name]
        t.require(_property(p,'amount')==counts[index] and _property(p,'lifetime')==lifetimes[index]
                  and _property(p,'preprocess')==100 and _property(p,'fixed_fps')==0
                  and _property(p,'local_coords') is True and _property(p,'draw_order')==0,
                  'Nonupeipe fog lifecycle/order differs')
        bindings.append(dict(node=name,amount=counts[index],lifetime_seconds=lifetimes[index],
                             preprocess_seconds=100,fixed_fps=0))
    rgba=np.asarray(texture(pack,TEXTURE).convert('RGBA'),dtype='u1')
    t.require(rgba.shape==(200,800,4) and np.all(rgba[:,:,:3]==255) and
              t.sha256(rgba.tobytes())=='dfe82e2bed4870f41d55386dac056ac73ae92f94df2f031f196b7a3dec84207d',
              'Source sand-cloud is not the expected white four-frame atlas')
    rows=[(100,200,0,rgba[:,i*200:(i+1)*200,3].copy().tobytes()) for i in range(4)]
    _,material=loader.resource(_property(props['fog3'],'process_material'))
    def curve(name):
        attrs,p=loader.resource(_property(material,name))
        t.require(attrs.get('type')=='CurveTexture','Fog requires a source CurveTexture')
        return curve_channel(loader,_property(p,'curve'),256)
    angle=curve('angle_curve');scale=curve('scale_curve');alpha=curve('alpha_curve')
    t.require(np.all(alpha==1) and np.all((scale>0)&(scale<=1)) and np.all((angle>-200)&(angle<-180)),
              'Fog source curve range differs')
    gradient=np.rint(generated_texture(loader,_property(material,'color_ramp'))[0]*255).astype('u1')
    t.require(gradient.shape==(256,4) and np.all(gradient[:,:3]==gradient[:,:1])
              and np.all(gradient[:,3]==255),'Fog source gradient is not opaque grayscale')
    # Angle is source initial_angle15 degrees multiplied by CurveTexture.
    # Preserve unwrapped signed angles; wrapping before interpolation changes
    # the turn crossing. Q8 angle units use 4096 units per full revolution.
    angle_q8=np.rint(angle.astype('f8')*15*4096*256/360).astype('<i4')
    scale_q12=np.rint(scale.astype('f8')*4096).astype('<u2')
    parameters=bytearray(PARAMETER_BYTES)
    struct.pack_into('<4s6H',parameters,0,b'NFG1',1,4,BASE_IMAGES,200,200,256)
    parameters[256:1280]=angle_q8.tobytes()
    parameters[1280:1792]=scale_q12.tobytes()
    parameters[1792:2048]=gradient[:,0].tobytes()
    rows.append((256,4,0,bytes(parameters)))
    return rows,dict(scene_sha256=SCENE_SHA,texture_rgba_sha256=t.sha256(rgba.tobytes()),
        source_bindings=bindings,static_plate_precedes_all_emitters=True,
        exact_source_alpha_pixels=160000,source_texture_resized=False,
        source_rgb_constant_white=True,parameter_sha256=t.sha256(parameters),
        curve_grid=256,maximum_angle_quantization_turn_units=.5/256,
        maximum_scale_quantization=.5/4096,gradient_bytes_exact=True,
        variable_source_timestep=True,original_random_history_recovered=False)


def validate_parameter(raw,image_count):
    t.require(len(raw)==PARAMETER_BYTES and struct.unpack_from('<4s6H',raw)==
              (b'NFG1',1,4,BASE_IMAGES,200,200,256),'Nonupeipe fog parameter header differs')
    t.require(BASE_IMAGES+4<image_count and not any(raw[16:256]),'Fog leaves/padding differ')
    angles=np.frombuffer(raw[256:1280],dtype='<i4');scales=np.frombuffer(raw[1280:1792],dtype='<u2')
    t.require(np.all((angles>-9000000)&(angles<-7000000)) and np.all((scales>0)&(scales<=4096)),
              'Nonupeipe fog curve bounds differ')
    t.require(t.sha256(raw)=='e93df01f6e5213f12339a3d5f14237dfab2766360ba218a0e4fd2304a70d6073',
              'Nonupeipe fog source curve bytes differ')
    return tuple(range(BASE_IMAGES,BASE_IMAGES+4))


def validate(source,image):
    t.require((len(source.aux),len(source.images))==(AUXILIARY,IMAGES)
              and source.aux[BASE_AUX]==IMAGES-1,'Nonupeipe fog typed catalog differs')
    w,h,fmt,raw=image(IMAGES-1)
    t.require((w,h,fmt)==(256,4,0),'Nonupeipe fog parameter view differs')
    leaves=validate_parameter(raw,len(source.images))
    for leaf in leaves:
        w,h,fmt,raw=image(leaf)
        t.require((w,h,fmt)==(100,200,0) and len(raw)==40000,'Fog alpha plane differs')
    return dict(images=leaves,table=IMAGES-1)


def append(data,pck,work):
    from pck import Pack
    from coalesce_tile_resources import view
    from tanx_scene_ui23 import group_images
    from patch_frontend_assets import patch_pack
    from split_content import LANGUAGES
    from no_animations import mark,reduced
    source=view(data)
    t.require(source.version==46 and len(source.aux)==BASE_AUX and len(source.images)==BASE_IMAGES
              and len(source.clips)==1783,'Expected exact cinder predecessor before Nonupeipe fog')
    work=Path(work);cache=work/'source-fog';cache.mkdir(parents=True,exist_ok=True)
    pack=Pack(pck)
    try:rows,proof=assets(pack)
    finally:pack.close()
    records=[]
    for i,(w,h,fmt,raw) in enumerate(rows):
        name=f'{i:02}-{t.sha256(raw)}.bin';t.atomic_bytes(cache/name,raw)
        records.append(dict(file=name,width=w,height=h,format=fmt,bytes=len(raw),
                            sha256=t.sha256(raw),raw_sha256=t.sha256(raw)))
    art=group_images(records,cache)
    manifest=dict(images=records,auxiliary_image_ids=[IMAGES-1],localized_text={code:{} for code in LANGUAGES})
    result,append_proof=patch_pack(data,manifest,cache,base_aux=BASE_AUX,auxiliary=AUXILIARY,
        input_version=46,version=46,texts=(),base_clips=len(source.clips),base_strings=len(source.strings),appended_art=art)
    if reduced(data):result=mark(result)
    target=view(result);typed=validate(target,target.image_raw)
    for i,row in enumerate(rows):t.require(target.image_raw(BASE_IMAGES+i)==row,'Fog source bytes changed in append')
    t.require(target.clips==source.clips and target.cards==source.cards and target.aux[:BASE_AUX]==source.aux,
              'Fog append changed existing metadata')
    proof.update(complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(result),
        input_bytes=len(data),output_bytes=len(result),added_bytes=len(result)-len(data),
        grouping=art['report'],append=append_proof,typed=typed,profile_preserved=reduced(result)==reduced(data),
        decoded_bytes=sum(len(row[3]) for row in rows),framebuffer_growth=0,particle_pool_growth=0)
    return result,proof


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('input','output','pck','work'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();data,proof=append(args.input.read_bytes(),args.pck,args.work)
    t.atomic_bytes(args.output,data);t.atomic_bytes(args.output.with_suffix('.json'),(json.dumps(proof,indent=2)+'\n').encode())
    print(json.dumps({k:proof[k] for k in ('complete','input_bytes','output_bytes','added_bytes')}))
