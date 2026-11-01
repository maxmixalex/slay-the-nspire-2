#!/usr/bin/env python3
"""One source-derived Vakuu cinder geometry root reusing exact Tez assets.

No pixels or curves are duplicated. The pinned original Vakuu node uses the
same box-emission physics, sprite, and three lookup curves as the Tez family.
"""
from pathlib import Path
import json,struct
import numpy as np
import content_tiles as t
from world_scene_assets import _sections,_property,_node_matrix,SceneLoader
import tez_cinders_ui23 as tez
BASE_AUX,AUXILIARY,BASE_IMAGES,IMAGES=7225,7226,59730,59731
SCENE='scenes/events/background_scenes/vakuu.tscn'
SCENE_SHA='5aa121b200b8a94b432f3254307c74b5208eeb2eaefacefa7b59d764432cb146'

def source_parameters(pack):
    from build_content import texture
    from world_room_shaders import curve_channel
    import ancient_lights_ui23 as lights
    raw=pack.read(SCENE);t.require(t.sha256(raw)==SCENE_SHA,'Vakuu cinder scene differs')
    sections=list(_sections(raw.decode('utf8'),SCENE));nodes=[(a,p) for k,a,p in sections if k=='node']
    t.require([a['name'] for a,p in nodes[:3]]==['Vakuu','TextureRect','sparks big'],
              'Vakuu cinder must follow the static plate and precede all lights')
    p=nodes[2][1];loader=SceneLoader(pack);_,material=loader.resource(_property(p,'process_material'))
    t.require(_property(p,'amount')==24 and _property(p,'lifetime')==7 and _property(p,'preprocess')==6
              and _property(p,'fixed_fps')==60 and _property(p,'local_coords') is True,
              'Vakuu cinder lifecycle differs')
    for key,value in {**tez.COMMON_PROCESS,**tez.BOX}.items():
        actual=_property(material,key)
        if hasattr(actual,'args'):actual=list(actual.args)
        t.require(actual==value,'Vakuu cinder process differs: '+key)
    curves=[]
    for name in ('orbit_velocity_curve','scale_curve','alpha_curve'):
        a,r=loader.resource(_property(material,name));t.require(a.get('type')=='CurveTexture','Vakuu cinder curve type differs')
        curves.append(curve_channel(loader,_property(r,'curve'),256))
    curves=lights.round_signed(np.array(curves)*1048576).astype('<i4').tobytes()
    sprite=np.asarray(texture(pack,'images/vfx/fire/cinder_particle.png').convert('RGBA'),dtype='u1')
    t.require(sprite.shape==(24,24,4) and t.sha256(sprite.tobytes())==tez.CINDER_SHA,'Vakuu cinder shared sprite differs')
    s=3195634/16777216
    camera=np.array([[s,0,-114518200509/16777216/256],[0,s,89989748010/16777216/256],[0,0,1.]])
    geometry=tez.projection(camera@_node_matrix(p))
    header=struct.pack('<4s6H',b'VKC1',1,24,tez.BASE_IMAGES,tez.BASE_IMAGES+1,tez.BASE_AUX,0)
    raw=header+struct.pack('<6i',*geometry)+bytes(24)
    return raw,curves,dict(scene_sha256=SCENE_SHA,node='sparks big',count=24,lifetime_seconds=7,
        preprocess_seconds=6,fixed_hz=60,local_coordinates=True,source_layer='afterstaticplatebeforealllights',
        source_matrix_q20_q8=list(map(int,geometry)),source_sprite_sha256=tez.CINDER_SHA,
        source_curves_sha256=t.sha256(curves),source_parameter_sha256=t.sha256(raw),
        new_texture_bytes=0,new_curve_bytes=0,new_particle_capacity=0,original_gpu_random_history_recovered=False)

def validate_parameter(raw,image_count):
    t.require(len(raw)==64 and struct.unpack_from('<4s6H',raw)==
      (b'VKC1',1,24,tez.BASE_IMAGES,tez.BASE_IMAGES+1,tez.BASE_AUX,0),'Vakuu cinder parameter header differs')
    values=struct.unpack_from('<6i',raw,16)
    t.require(all(-8388608<v<8388608 for v in values) and not any(raw[40:])
              and tez.BASE_IMAGES+1<image_count,'Vakuu cinder geometry/references differ')
    t.require(t.sha256(raw)=='9796d61ab5212a85d76b138d74d0b487e60ee01b36ce00da13286b56fcb508ad',
              'Vakuu cinder authored geometry bytes differ')
    return dict(images=(tez.BASE_IMAGES,tez.BASE_IMAGES+1),table=BASE_IMAGES)

def validate(source,image):
    t.require((len(source.aux),len(source.images))==(AUXILIARY,IMAGES)
              and source.aux[BASE_AUX]==BASE_IMAGES,'Vakuu cinder typed catalog differs')
    w,h,fmt,raw=image(BASE_IMAGES);t.require((w,h,fmt)==(32,1,0),'Vakuu cinder parameter view differs')
    return validate_parameter(raw,len(source.images))

def append(data,pck,work):
    from pck import Pack
    from coalesce_tile_resources import view
    from patch_frontend_assets import patch_pack
    from split_content import LANGUAGES
    from no_animations import mark,reduced
    source=view(data);t.require(source.version==46 and len(source.aux)==BASE_AUX and len(source.images)==BASE_IMAGES
       and len(source.clips)==1783,'Expected exact Neow-post predecessor before Vakuu cinder')
    pack=Pack(pck)
    try:raw,curves,proof=source_parameters(pack)
    finally:pack.close()
    w,h,fmt,existing=source.image_raw(tez.BASE_IMAGES+1)
    t.require((w,h,fmt)==(256,51,0) and existing[tez.CURVE_OFFSET:tez.CURVE_OFFSET+3072]==curves,
              'Vakuu and existing source-bound Tez cinder curves are not identical')
    work=Path(work);work.mkdir(parents=True,exist_ok=True);name=t.sha256(raw)+'.bin';t.atomic_bytes(work/name,raw)
    record=dict(file=name,width=32,height=1,format=0,bytes=64,sha256=t.sha256(raw),raw_sha256=t.sha256(raw))
    manifest=dict(images=[record],auxiliary_image_ids=[BASE_IMAGES],localized_text={c:{} for c in LANGUAGES})
    result,appended=patch_pack(data,manifest,work,base_aux=BASE_AUX,auxiliary=AUXILIARY,
      input_version=46,version=46,texts=(),base_clips=len(source.clips),base_strings=len(source.strings))
    if reduced(data):result=mark(result)
    target=view(result);typed=validate(target,target.image_raw)
    t.require(target.image_raw(BASE_IMAGES)==(32,1,0,raw) and target.clips==source.clips and target.cards==source.cards
       and target.aux[:BASE_AUX]==source.aux and reduced(result)==reduced(data),'Vakuu cinder append changed earlierdata/profile')
    proof.update(complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(result),input_bytes=len(data),
       output_bytes=len(result),added_bytes=len(result)-len(data),typed=typed,append=appended,shared_curves_exact=True)
    return result,proof

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('input','output','pck','work'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();data,proof=append(a.input.read_bytes(),a.pck,a.work)
    t.atomic_bytes(a.output,data);t.atomic_bytes(a.output.with_suffix('.json'),(json.dumps(proof,indent=2)+'\n').encode())
    print(json.dumps({k:proof[k] for k in ('complete','added_bytes','output_sha256')}))
