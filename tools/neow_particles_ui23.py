#!/usr/bin/env python3
"""Neow's three GPU cloud emitters and ten CPU grain stars, in source order.

Shares the unchanged four200x200 Nonu alpha masks. Only exact source curves,
three CPU gradient keys and the original32x32grain alpha are appended.
"""
from pathlib import Path
import json,struct
import numpy as np
import content_tiles as t
from world_scene_assets import SceneLoader,_property,_vector
from world_room_shaders import curve_channel,generated_texture
BASE_AUX,AUXILIARY,BASE_IMAGES,IMAGES=7226,7227,59731,59732
ATLAS_FIRST=59712
SCENE='scenes/events/background_scenes/neow.tscn'
SCENE_SHA='39274c6148a90e63d91322e741bc000b1e64e172442bcfa8fedc1a43cdb01389'
GRAIN='images/vfx/short_grain_rice_particle.png'
GRAIN_SHA='c44b62625f7ceb06fe0201d7a03fe4ad0f63f79ce96870ea67a4575f0ad4719c'
BYTES=4608


def assets(pack):
    from build_content import texture
    raw=pack.read(SCENE);t.require(t.sha256(raw)==SCENE_SHA,'Neow source scene differs')
    loader=SceneLoader(pack);nodes={a['name']:(a,p) for k,a,p in loader.document(SCENE).sections if k=='node'}
    curves=[];bindings=[]
    for node,position,angle in [('fog3',(1088,1302),15),('fog5',(36,1311),15),('fog4',(1088,1302),-17)]:
        attrs,p=nodes[node];get=lambda k,d=None:_property(p,k,d)
        t.require(attrs['parent']=='.' and attrs['type']=='GPUParticles2D' and get('visible',True) and get('amount')==5 and get('lifetime')==10 and
          get('preprocess')==100 and get('fixed_fps')==0 and get('local_coords') is True and get('draw_order')==0 and tuple(_vector(get('position')))==position,
          'Neow cloud lifecycle/transform differs')
        ma,material=loader.resource(get('process_material'));m=lambda k,d=None:_property(material,k,d)
        t.require(ma['type']=='ParticleProcessMaterial' and m('particle_flag_disable_z') is True and m('angle_min')==m('angle_max')==angle and
          m('scale_min')==4 and m('scale_max')==6 and tuple(_vector(m('gravity'),3))==((-50,0,0) if node=='fog4' else (50,0,0)) and
          m('emission_shape',0)==0 and m('turbulence_enabled',False) is False and m('anim_offset_min',0)==0 and m('anim_offset_max')==1 and
          m('anim_speed_min',0)==m('anim_speed_max',0)==0,'Neow cloud particle material differs')
        ba,bp=loader.resource(get('material'));b=lambda k,d=None:_property(bp,k,d)
        t.require(ba['type']=='CanvasItemMaterial' and b('blend_mode')==1 and b('particles_animation') is True and b('particles_anim_h_frames')==4 and
          b('particles_anim_v_frames')==1 and b('particles_anim_loop') is False,'Neow cloud source atlas differs')
        t.require(loader.path(get('texture'))=='images/vfx/monsters/the_insatiable/sand_cloud.png','Neow cloud source texture differs')
        def curve(key):
            ca,cp=loader.resource(m(key));t.require(ca['type']=='CurveTexture' and _property(cp,'width',256)==256,'Neow cloud curve differs')
            return curve_channel(loader,_property(cp,'curve'),256)
        angles=curve('angle_curve');scale=curve('scale_curve');alpha=curve('alpha_curve');gradient=np.rint(generated_texture(loader,m('color_ramp'))[0]*255).astype('u1')
        tint=np.rint(_vector(m('color'),4)*255).astype('u1')
        t.require(np.all(alpha==1) and np.all((scale>0)&(scale<=1)) and gradient.shape==(256,4) and np.all(gradient[:,:3]==gradient[:,:1]) and
          np.all(gradient[:,3]==255) and tint.tolist()==[145,185,194,255],'Neow cloud display range differs')
        curves.append((np.rint(angles.astype('f8')*angle*4096*256/360).astype('<i4').tobytes(),np.rint(scale*4096).astype('<u2').tobytes(),gradient[:,0].tobytes()))
        bindings.append(dict(node=node,count=5,life_ms=10000,preprocess_ms=100000,source_fixed_fps=0))
    t.require(curves[0]==curves[1] and curves[0][1:]==curves[2][1:],'Neow shared source curves differ')
    attrs,p=nodes['stars'];get=lambda k,d=None:_property(p,k,d)
    t.require(attrs['parent']=='.' and attrs['type']=='CPUParticles2D' and get('visible',True) and get('amount')==10 and get('lifetime')==5 and
      get('preprocess')==5 and get('local_coords') is True and get('emission_shape')==3 and tuple(_vector(get('position')))==(1003,590) and
      tuple(_vector(get('scale')))==(.999997,.999997) and tuple(_vector(get('emission_rect_extents')))==(1500,800) and get('spread')==180 and
      tuple(_vector(get('gravity')))==(-1,0) and get('initial_velocity_min')==get('initial_velocity_max')==50 and get('scale_amount_min',1)==1 and get('scale_amount_max')==2,
      'Neow grain source parameters differ')
    t.require(loader.path(get('texture'))==GRAIN,'Neow grain source texture differs')
    blend_attrs,blend=loader.resource(get('material'))
    t.require(blend_attrs['type']=='CanvasItemMaterial' and _property(blend,'blend_mode')==1,
              'Neow grain additive blend differs')
    ga,gp=loader.resource(get('color_ramp'));t.require(ga['type']=='Gradient' and _property(gp,'interpolation_mode',0)==0 and _property(gp,'interpolation_color_space',0)==0,'Neow CPU gradient differs')
    offsets=_vector(_property(gp,'offsets'),3);colors=_vector(_property(gp,'colors'),12).reshape(3,4)
    t.require(offsets.tolist()==[0,.258845,1] and colors.shape==(3,4),'Neow CPU gradient key count differs')
    gradient=b''.join(struct.pack('<5H',round(float(x)*65535),*np.rint(np.clip(color,0,1)*65535).astype('u2')) for x,color in zip(offsets,colors))
    grain=np.asarray(texture(pack,GRAIN).convert('RGBA'),dtype='u1');t.require(grain.shape==(32,32,4) and np.all(grain[:,:,:3]==255) and t.sha256(grain.tobytes())==GRAIN_SHA,'Original grain mask differs')
    head=struct.pack('<4s6H8I8B8x',b'NNP1',1,ATLAS_FIRST,4,32,32,3,64,1088,2112,2624,2880,2910,3422,4446,145,185,194,255,*np.rint(_vector(get('color'),4)*255).astype('u1'))
    t.require(len(head)==64,'Neow particle header differs')
    data=head+curves[0][0]+curves[2][0]+curves[0][1]+curves[0][2]+gradient+b'\xff'*512+grain[:,:,3].tobytes()
    t.require(len(data)==4446,'Neow particle data extent differs');data+=bytes(BYTES-len(data))
    return [(256,9,0,data)],dict(scene_sha256=SCENE_SHA,source_grain_rgba_sha256=GRAIN_SHA,shared_cloud_images=list(range(ATLAS_FIRST,ATLAS_FIRST+4)),
      exact_cloud_alpha_pixels=160000,new_grain_alpha_pixels=1024,source_texture_resized=False,source_order=['bone_cpu_lights3','fog3','fog5','fog4','stars10','static_cpu_lights4','vignette','cpu_lights3'],
      cloud_emitters=bindings,source_cpu_preprocess_fps=30,portable_integration_fps=30,source_random_history_recorded=False,parameter_sha256=t.sha256(data))


def validate_parameter(raw,image_count):
    t.require(len(raw)==BYTES and raw[:4]==b'NNP1' and struct.unpack_from('<6H8I',raw,4)==(1,ATLAS_FIRST,4,32,32,3,64,1088,2112,2624,2880,2910,3422,4446),
      'Neow particle table regions differ')
    t.require(ATLAS_FIRST+4<image_count and raw[48:52]==bytes((145,185,194,255)) and raw[52:56]==bytes((226,187,71,113)) and not any(raw[56:64]) and
      not any(raw[4446:]) and raw[2910:3422]==b'\xff'*512,'Neow particle source tint/palette/padding differs')
    keys=np.frombuffer(raw[2880:2910],dtype='<u2').reshape(3,5)
    t.require(keys[0,0]==0 and keys[-1,0]==65535 and 0<keys[1,0]<65535,'Neow CPU gradient keys differ')
    return tuple(range(ATLAS_FIRST,ATLAS_FIRST+4))


def validate(source,image):
    t.require((len(source.aux),len(source.images))==(AUXILIARY,IMAGES) and source.aux[BASE_AUX]==BASE_IMAGES,'Neow particles require exact typed endpoint')
    w,h,fmt,raw=image(BASE_IMAGES);t.require((w,h,fmt)==(256,9,0),'Neow particle view differs');validate_parameter(raw,IMAGES)
    return dict(images=(*range(ATLAS_FIRST,ATLAS_FIRST+4),BASE_IMAGES),auxiliary=(BASE_AUX,))


def append(data,pck,work):
    from pck import Pack
    from coalesce_tile_resources import view
    from patch_frontend_assets import patch_pack
    from split_content import LANGUAGES
    from no_animations import mark,reduced
    source=view(data)
    t.require(source.version==46 and (len(source.aux),len(source.images))==(BASE_AUX,BASE_IMAGES)
              and len(source.clips)==1783,'Expected exact Vakuu-cinder predecessor before Neow particles')
    # The shared images are 200x200 alpha bytes stored in 100x200 word views.
    for image_id in range(ATLAS_FIRST,ATLAS_FIRST+4):
        w,h,fmt,raw=source.image_raw(image_id)
        t.require((w,h,fmt,len(raw))==(100,200,0,40000),'Neow shared original cloud mask differs')
    pack=Pack(pck)
    try:leaves,proof=assets(pack)
    finally:pack.close()
    work=Path(work);work.mkdir(parents=True,exist_ok=True)
    records=[]
    for w,h,fmt,raw in leaves:
        digest=t.sha256(raw);name=digest+'.bin';t.atomic_bytes(work/name,raw)
        records.append(dict(file=name,width=w,height=h,format=fmt,bytes=len(raw),sha256=digest,raw_sha256=digest))
    manifest=dict(images=records,auxiliary_image_ids=[BASE_IMAGES],localized_text={c:{} for c in LANGUAGES})
    result,appended=patch_pack(data,manifest,work,base_aux=BASE_AUX,auxiliary=AUXILIARY,
      input_version=46,version=46,texts=(),base_clips=len(source.clips),base_strings=len(source.strings))
    if reduced(data):result=mark(result)
    target=view(result);typed=validate(target,target.image_raw)
    t.require(target.image_raw(BASE_IMAGES)==leaves[0] and target.clips==source.clips and target.cards==source.cards
      and target.aux[:BASE_AUX]==source.aux and reduced(result)==reduced(data),'Neow particle append changed earlier data/profile')
    proof.update(complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(result),input_bytes=len(data),
      output_bytes=len(result),added_bytes=len(result)-len(data),typed=typed,append=appended)
    return result,proof


if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('input','output','pck','work'):parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();result,proof=append(args.input.read_bytes(),args.pck,args.work)
    t.atomic_bytes(args.output,result)
    t.atomic_bytes(args.output.with_suffix('.json'),(json.dumps(proof,indent=2)+'\n').encode())
    print(json.dumps({k:proof[k] for k in ('complete','added_bytes','output_sha256')}))
