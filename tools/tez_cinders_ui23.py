#!/usr/bin/env python3
"""Source-bound74*24 Tezcatara cinders, with source-order light routing.

Two moving eye emitters inherit source global velocity at60Hz. The stationary
emitters have zero inherited velocity. Curves and render geometry stay in data;
only the existing scene union carries live particle state.
"""
from __future__ import annotations
import json
from pathlib import Path
import struct
import numpy as np
import content_tiles as t
import ancient_lights_ui23 as lights
import tez_effects_ui23 as effects

BASE_AUX,AUXILIARY,BASE_IMAGES,IMAGES=7221,7222,59710,59712
MAGIC,VERSION=0x5443,1
EMITTERS,PARTICLES,FRAMES,DYNAMIC,VELOCITIES=74,24,506,2,1000
RECORD=32
EMITTER_OFFSET=32
DELTA_OFFSET=EMITTER_OFFSET+EMITTERS*RECORD
CURVE_OFFSET=DELTA_OFFSET+DYNAMIC*FRAMES*12
VELOCITY_OFFSET=CURVE_OFFSET+3*256*4
ORDER_OFFSET=VELOCITY_OFFSET+VELOCITIES*2*2*2
LENGTH=ORDER_OFFSET+(EMITTERS+90)*2
CINDER_SHA='0c132c380a09da3c5664592ace9c44c8d95c342fa49704dd12311eade92cbc66'
COMMON_PROCESS=dict(lifetime_randomness=.5,particle_flag_align_y=True,particle_flag_disable_z=True,
    inherit_velocity_ratio=1.,direction=[0,-.5,0],spread=16.,initial_velocity_min=100.,initial_velocity_max=200.,
    orbit_velocity_min=-1.,orbit_velocity_max=3.,gravity=[100,-20,0],color=[.772549,.54902,.239216,1],
    hue_variation_min=-.03,hue_variation_max=.04)
SPHERE=dict(emission_shape_scale=[50,50,1],emission_shape=1,emission_sphere_radius=1.,scale_max=1.2)
BOX=dict(emission_shape=3,emission_box_extents=[20,20,1],scale_min=.5,scale_max=.7)


def matrix(values):return np.array([*np.asarray(values).reshape(2,3),[0,0,1.]])

def bone(path):
    parts=path.split('/')
    for i,part in enumerate(parts):
        if part.startswith('SpineBoneNode'):return '/'.join(parts[:i+1])
    return None


def projection(value):
    a,b,x,c,d,y=np.asarray(value)[:2].ravel()
    return lights.round_signed(np.array([a*1048576,b*1048576,c*1048576,d*1048576,x*256,y*256]))


def make_parameters(rows,light_frames,velocity):
    t.require(len(light_frames)==FRAMES and all(len(f)==90 for f in light_frames),'Cinder requires the complete published light source family')
    baseline=light_frames[0]
    source_lights=[r['light'] for r in rows if r['kind']=='light']
    t.require([r['path'] for r in source_lights]==[r['path'] for r in baseline] and
              all(np.array_equal(a['matrix'],b['matrix']) for a,b in zip(source_lights,baseline)),
              'Cinder source draw order does not match current light-family geometry')
    cinders=[r for r in rows if r['kind']=='cinder']
    t.require(len(cinders)==EMITTERS,'Cinder emitter count changed')
    anchors={}
    for i,r in enumerate(baseline):
        if bone(r['path']):anchors.setdefault(bone(r['path']),i)
    records=bytearray();deltas=bytearray();curves=None
    for i,r in enumerate(cinders):
        sphere=i<2
        t.require(r['process']=={**COMMON_PROCESS,**(SPHERE if sphere else BOX)} and
                  r['properties']['lifetime']==(10 if sphere else 7) and r['properties']['preprocess']==6 and
                  r['tint']==[1,1,1,1] and r['blend']==1,'Cinder source physics/tint changed')
        raw_curves=np.asarray([r['curves'][name] for name in ('orbit_velocity_curve','scale_curve','alpha_curve')])
        t.require(raw_curves.shape==(3,256) and np.all(np.isfinite(raw_curves)), 'Invalid cinder curve table')
        if curves is None:curves=lights.round_signed(raw_curves*1048576)
        t.require(np.array_equal(curves,lights.round_signed(raw_curves*1048576)), 'Cinder source curves differ between emitters')
        group=bone(r['path']);t.require(bool(group)==sphere,'Cinder moving emitter set changed')
        source=matrix(r['matrix']);base=projection(source);slot=i if sphere else 255
        records.extend(struct.pack('<6i4BI',*base,slot,int(sphere),i if sphere else 255,0,0))
        if sphere:
            t.require(group in anchors,'Cinder dynamic bone lacks a source-bound anchor')
            anchor=anchors[group];relative=np.linalg.inv(matrix(baseline[anchor]['matrix']))@source
            values=np.array([projection(matrix(frame[anchor]['matrix'])@relative) for frame in light_frames])-base
            t.require(np.all((values>=-32768)&(values<=32767)),'Cinder projection delta exceeds signed16')
            deltas.extend(values.astype('<i2').tobytes())
    velocity=np.asarray(velocity)
    t.require(velocity.shape==(1000,2,2) and np.all((velocity>=-32768)&(velocity<=32767)),'Cinder inherited velocity extent')
    order=[];li=ci=0
    for r in rows:
        if r['kind']=='light':order.append(li);li+=1
        elif r['kind']=='cinder':order.append(0x8000|ci);ci+=1
    t.require(li==90 and ci==74,'Cinder/light routing count changed')
    header=(MAGIC,VERSION,EMITTERS,PARTICLES,FRAMES,DYNAMIC,33,16667,BASE_IMAGES,
            EMITTER_OFFSET,DELTA_OFFSET,CURVE_OFFSET,VELOCITY_OFFSET,ORDER_OFFSET,LENGTH,0)
    raw=struct.pack('<16H',*header)+records+deltas+curves.astype('<i4').tobytes()+velocity.astype('<i2').tobytes()+struct.pack('<164H',*order)
    t.require(len(raw)==LENGTH,'Cinder section offsets differ');raw+=bytes((-len(raw))%512)
    parameters(raw)
    return raw,dict(emitters=74,particles=1776,scene_bone_frames=506,inherited_velocity_frames=1000,
                    source_clock_hz=60,preprocess_seconds=6,raw_bytes=len(raw),source_curves_quantization_max=1/2097152)


def parameters(raw):
    expected=(MAGIC,VERSION,EMITTERS,PARTICLES,FRAMES,DYNAMIC,33,16667,BASE_IMAGES,
              EMITTER_OFFSET,DELTA_OFFSET,CURVE_OFFSET,VELOCITY_OFFSET,ORDER_OFFSET,LENGTH,0)
    t.require(len(raw)==(LENGTH+511)//512*512 and struct.unpack_from('<16H',raw)==expected and not any(raw[LENGTH:]),
              'Cinder header/extent/padding differs')
    for i in range(74):
        values=struct.unpack_from('<6i4BI',raw,EMITTER_OFFSET+i*RECORD)
        t.require(all(abs(v)<8388608 for v in values[:6]) and values[6:]==(i if i<2 else 255,int(i<2),i if i<2 else 255,0,0),
                  'Cinder projection flags/range changed')
    curve=np.frombuffer(raw,dtype='<i4',count=768,offset=CURVE_OFFSET).reshape(3,256)
    t.require(np.all(abs(curve[0])<1048576) and np.all((curve[1:]>=0)&(curve[1:]<=1048576)), 'Cinder curve range differs')
    li=ci=0
    for value in struct.unpack_from('<164H',raw,ORDER_OFFSET):
        if value&0x8000:t.require(value==0x8000|ci,'Cinder order skips/duplicates emitters');ci+=1
        else:t.require(value==li,'Cinder order skips/duplicates light nodes');li+=1
    t.require(li==90 and ci==74,'Cinder order coverage differs')
    return expected


def source_inputs(pack,sampler,runtime,light_work,work):
    import animation
    import world_scene_assets as world
    from world_raster import raster
    from ancient_scene_ui23 import SCENES
    from build_content import texture
    animation.raster=raster
    directory,report=lights.sample_scene(pack,sampler,runtime,SCENES[1],Path(light_work)/'sampled')
    frames=[json.loads((directory/f'{i:04}.json').read_text())['lights']['nodes'] for i in range(FRAMES)]
    with effects.source_renderer():_,diagnostics=world.render_scene(pack,effects.SCENE,320,240,viewport=(1680,1260),sampler=sampler)
    rows=diagnostics['tez_effects'];eyes=[r for r in rows if r['kind']=='cinder'][:2]
    loader=world.SceneLoader(pack);tree=loader.instantiate(effects.SCENE);nodes={}
    def visit(node,path):
        nodes[path]=node
        for child in node.children:visit(child,path+'/'+child.name)
    visit(tree,tree.name)
    resource=loader.path(world._property(nodes['Tezcatara/SpineSprite'].props,'skeleton_data_res'))
    names=[world._property(nodes[r['path'].rsplit('/',1)[0]].props,'bone_name') for r in eyes]
    t.require(names==['eye_l','eye_r'],'Cinder moving bone identity changed')
    local=[world._node_matrix(nodes[r['path']].props) for r in eyes]
    positions=[];bind=None;screen=np.diag([320/1680,240/1260,1])
    for start in range(0,1001,16):
        batch=sampler.sample(resource,SCENES[1]['clip'],[f/60 for f in range(start,min(1001,start+16))],loop=True)
        for frame in batch:
            bones={r['name']:matrix(r['node_matrix']) for r in frame['bones'] if r['name'] in names}
            t.require(len(bones)==2,'Missing source cinder eye bone')
            if bind is None:bind=[np.linalg.inv(screen)@matrix(r['matrix'])@np.linalg.inv(l)@np.linalg.inv(bones[name]) for r,l,name in zip(eyes,local,names)]
            positions.append([(a@bones[name]@l)[:2,2].tolist() for a,l,name in zip(bind,local,names)])
    velocity=np.diff(np.asarray(positions),axis=0)*60
    fixed=lights.round_signed(velocity*256)
    raw,proof=make_parameters(rows,frames,fixed)
    sprite=texture(pack,effects.CINDER).convert('RGBA')
    t.require(sprite.size==(24,24) and t.sha256(sprite.tobytes())==CINDER_SHA,'Original cinder sprite differs')
    proof.update(light_source_signature=report['signature'],source_scene_sha256=t.sha256(pack.read(effects.SCENE)),
                 source_sprite_rgba_sha256=CINDER_SHA,eye_source_bones=names,
                 inherited_velocity_q8_error=float(np.max(abs(velocity-fixed/256))),
                 shader_time_independent=True,fire_implemented=False,
                 cosmetic_boundary='stable native birth seed; original GPU random epoch is unavailable')
    work=Path(work);work.mkdir(parents=True,exist_ok=True)
    t.atomic_bytes(work/'parameters.bin',raw);t.atomic_bytes(work/'source.json',(json.dumps(proof,indent=2)+'\n').encode())
    return sprite,raw,proof


def validate(source,image):
    t.require((len(source.aux),len(source.images))==(AUXILIARY,IMAGES) and source.aux[-1]==BASE_IMAGES+1,'Cinder extension endpoint differs')
    w,h,fmt,raw=image(BASE_IMAGES)
    t.require((w,h,fmt)==(24,24,1) and len(raw)==24*24*3,'Cinder source sprite layout differs')
    w,h,fmt,raw=image(BASE_IMAGES+1)
    t.require((w,h,fmt)==(256,(LENGTH+511)//512,0),'Cinder parameter image layout differs')
    parameters(raw)


def append(data,sprite,raw,proof,work):
    from coalesce_tile_resources import view
    from build_content import image_data
    from patch_frontend_assets import patch_pack
    from split_content import LANGUAGES
    from tanx_scene_ui23 import group_images
    from no_animations import reduced
    source=view(data)
    t.require(source.version==46 and (len(source.aux),len(source.images),len(source.clips))==(BASE_AUX,BASE_IMAGES,1783) and not reduced(data),
              'Cinder append requires the complete stationary-material endpoint before reduction')
    parameters(raw)
    images=[(24,24,1,image_data(sprite,True)),(256,len(raw)//512,0,raw)]
    work=Path(work);work.mkdir(parents=True,exist_ok=True);rows=[]
    for i,(w,h,fmt,pixels) in enumerate(images):
        name=f'{i}.bin';t.atomic_bytes(work/name,pixels)
        rows.append(dict(width=w,height=h,format=fmt,file=name,bytes=len(pixels),sha256=t.sha256(pixels),raw_sha256=t.sha256(pixels)))
    art=group_images(rows,work)
    manifest=dict(images=rows,auxiliary_image_ids=[BASE_IMAGES+1],localized_text={code:{} for code in LANGUAGES})
    output,detail=patch_pack(data,manifest,work,base_aux=BASE_AUX,auxiliary=AUXILIARY,input_version=46,version=46,
        texts=(),base_clips=len(source.clips),base_strings=len(source.strings),appended_art=art)
    target=view(output);validate(target,target.image_raw)
    t.require(target.clips==source.clips and target.cards==source.cards and target.aux[:BASE_AUX]==source.aux,'Cinder append changed existing identities')
    for i,row in enumerate(images):t.require(target.image_raw(BASE_IMAGES+i)==row,'Cinder encoded pixels differ')
    return output,dict(complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(output),input_bytes=len(data),output_bytes=len(output),
                      added_bytes=len(output)-len(data),source=proof,grouping=art['report'],append=detail,particle_capacity=1776,
                      new_global_particle_bytes=0,scene_union_capacity_changed=False,full_fire_still_pending=True)


def main():
    import argparse
    from pck import Pack
    from world_enemy_assets import SpineSampler
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('pck','runtime','work','light-work'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--input',type=Path);parser.add_argument('--output',type=Path);parser.add_argument('--sample-only',action='store_true')
    args=parser.parse_args();t.require(args.sample_only or args.input and args.output,'Supply input/output or sample-only')
    for path in (args.pck,args.runtime,args.work,args.light_work,args.input,args.output):
        if path is not None:t.private_path(path,'Private original Tez cinder source/output')
    pack=Pack(args.pck)
    try:
        with SpineSampler(pack,runtime=args.runtime,work_dir=args.work/'adapter') as sampler:
            sprite,raw,proof=source_inputs(pack,sampler,args.runtime,args.light_work,args.work/'source')
        if not args.sample_only:
            output,receipt=append(args.input.read_bytes(),sprite,raw,proof,args.work/'append')
            t.atomic_bytes(args.output,output);t.atomic_bytes(args.output.with_suffix('.json'),(json.dumps(receipt,indent=2)+'\n').encode())
    finally:pack.close()


if __name__=='__main__':main()
