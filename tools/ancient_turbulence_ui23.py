#!/usr/bin/env python3
"""Offline Godot 4.5.1 turbulence and bounded native-screen particle tracks.

The desktop helper evaluates the source four-dimensional gradient/curl field.
The calculator reads compact trajectories instead of evaluating GPU noise. This
is a deterministic source-seed bank, not a recording of an unknown GPU random
history. Actor images, actor frame cadence and particle counts are unchanged.
"""
from __future__ import annotations
import ctypes, hashlib, math, os, struct, subprocess
from pathlib import Path
import numpy as np

# Equations adapted from Godot 4.5.1 ParticleProcessMaterial (MIT),
# scene/resources/particle_process_material.cpp. Curl construction credits
# Isaac Cohen as in that source. The helper uses float32 and disables FMA.
_TURBULENCE_CALLER = r'''
#include <cmath>
#include <cstdint>
#include <algorithm>
#if defined(_WIN32)
#define NSPIRE_TURB_EXPORT __declspec(dllexport)
#else
#define NSPIRE_TURB_EXPORT
#endif
namespace nspire_source_turbulence {
struct V3 {float x,y,z;};
struct V4 {float v[4];};
inline float fract(float x){return x-std::floor(x);}
inline float mix(float a,float b,float t){return a*(1.0f-t)+b*t;}
inline V3 add(V3 a,V3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V3 mul(V3 a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float length(V3 a){return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);}
inline V3 normal(V3 a){float n=length(a);return n>0?mul(a,1.0f/n):V3{0,0,0};}
inline V3 blend(V3 a,V3 b,float t){return {mix(a.x,b.x,t),mix(a.y,b.y,t),mix(a.z,b.z,t)};}
inline V4 gradient(V4 p){
 static constexpr float m[4][4]={{.143081f,.001724f,.280166f,.262771f},
  {.645401f,-.047791f,-.146698f,.595016f},{-.499665f,-.095734f,.425674f,-.207367f},
  {-.013596f,-.848588f,.423736f,.17044f}};
 V4 q{},r{};for(unsigned j=0;j<4;++j)q.v[j]=fract(((p.v[0]*m[j][0]+p.v[1]*m[j][1])+p.v[2]*m[j][2])+p.v[3]*m[j][3]);
 for(unsigned j=0;j<4;++j)r.v[j]=fract((q.v[j]*q.v[(j+1)&3])*2365.952041f)*2.0f-1.0f;
 return r;
}
inline float noise(V4 p){
 float r=((p.v[0]*-.1666667f+p.v[1]*-.1666667f)+p.v[2]*-.1666667f)+p.v[3]*-.5f;
 V4 q{{p.v[0]+r,p.v[1]+r,p.v[2]+r,((p.v[0]*.5f+p.v[1]*.5f)+p.v[2]*.5f)+p.v[3]*.5f}};
 V4 base{},delta{},fade{};for(unsigned j=0;j<4;++j){base.v[j]=std::floor(q.v[j]);delta.v[j]=q.v[j]-base.v[j];float t=delta.v[j];fade.v[j]=((t*t)*t)*(10.0f+t*(-15.0f+t*6.0f));}
 float value[16];for(unsigned i=0;i<16;++i){V4 point{},d{};for(unsigned j=0;j<4;++j){float bit=float((i>>j)&1);point.v[j]=base.v[j]+bit;d.v[j]=delta.v[j]-bit;}V4 g=gradient(point);value[i]=((d.v[0]*g.v[0]+d.v[1]*g.v[1])+d.v[2]*g.v[2])+d.v[3]*g.v[3];}
 // Source shader interpolates W, Z, Y, X, in this order.
 for(unsigned i=0;i<8;++i)value[i]=mix(value[i],value[i+8],fade.v[3]);
 for(unsigned i=0;i<4;++i)value[i]=mix(value[i],value[i+4],fade.v[2]);
 for(unsigned i=0;i<2;++i)value[i]=mix(value[i],value[i+2],fade.v[1]);
 return mix(value[0],value[1],fade.v[0]);
}
inline float shifted(V4 p,unsigned axis,float shift,unsigned channel){
 p.v[axis]+=shift;constexpr float offset=1.7320508f*2048.333333f;
 if(channel==1)p.v[3]+=offset;else if(channel==2)p.v[3]-=offset;return noise(p);
}
inline V3 curl(V4 p,float strength){
 const float e=.001f+std::max(strength-1.0f,0.0f)*70.0f;
 // Only the twelve source scalar samples that contribute to curl are needed.
 float x=(shifted(p,1,e,2)-shifted(p,1,-e,2))-(shifted(p,2,e,1)-shifted(p,2,-e,1));
 float y=(shifted(p,2,e,0)-shifted(p,2,-e,0))-(shifted(p,0,e,2)-shifted(p,0,-e,2));
 float z=(shifted(p,0,e,1)-shifted(p,0,-e,1))-(shifted(p,1,e,0)-shifted(p,1,-e,0));
 // get_noise_direction multiplies this normalized vector by a positive scalar
 // for all authored strengths (1 and20); process() immediately normalizes it.
 return normal({x,y,z});
}
inline uint32_t hash(uint32_t x){x=((x>>16)^x)*73244475u;x=((x>>16)^x)*73244475u;return (x>>16)^x;}
inline float random(uint32_t&seed){int32_t s=int32_t(seed);if(!s)s=305420679;int32_t k=s/127773;
 int64_t n=int64_t(16807)*(int64_t(s)-int64_t(k)*127773)-int64_t(2836)*k;
 // Source integer arithmetic is signed two's-complement. This wide expression
 // has the same value for its Park-Miller domain and avoids host signed UB.
 s=int32_t(uint32_t(n));if(s<0)s+=2147483647;seed=uint32_t(s);return float(seed%65536u)/65535.0f;}
inline float sample(const float*p,unsigned n,float phase){float x=phase*float(n)-.5f;int low=int(std::floor(x));float f=x-float(low);unsigned a=unsigned(std::max(0,std::min(int(n)-1,low))),b=unsigned(std::max(0,std::min(int(n)-1,low+1)));return mix(p[a],p[b],f);}
// Config: life,fps,originXY,shape,emissionScaleXYZ,radius,boxXYZ,gravityXY,
// scaleMinMax,angularMinMax,tangentMinMax,radialMinMax,strength,noiseUIScale,
// noiseSpeedXYZ,noiseSpeedRandom,localCoords,initialVelocityMinMax,spread,
// influenceMinMax,timeOffset,radialAccelMinMax.
inline int trajectory(const float*c,uint32_t number,uint32_t random_seed,const float*influence,unsigned width,unsigned ticks,float*out,float*constants){
 if(!c||!influence||width<2||width>4096||ticks<1||ticks>3600||!out||!constants||c[1]<1||c[0]<=0)return 0;
 uint32_t seed=hash(number+1u+random_seed);
 float scale=mix(c[14],c[15],random(seed));for(unsigned i=0;i<4;++i)random(seed);
 random(seed);float angular=mix(c[16],c[17],random(seed));float initial=mix(c[29],c[30],random(seed));
 random(seed);float radial=mix(c[20],c[21],random(seed));random(seed);float influence_birth=mix(c[32],c[33],random(seed));
 random(seed);float radial_accel=mix(c[35],c[36],random(seed));float tangent=mix(c[18],c[19],random(seed));random(seed);random(seed);
 V3 pos{0,0,0};constexpr float pi=3.14159f;
 if(int(c[4])==2){float s=random(seed)*2.0f-1.0f,t=random(seed)*2.0f*pi;float radius=c[8]*std::sqrt(std::max(0.0f,1.0f-s*s));pos={radius*std::cos(t),radius*std::sin(t),c[8]*s};}
 else if(int(c[4])==3){pos={(random(seed)*2.0f-1.0f)*c[9],(random(seed)*2.0f-1.0f)*c[10],(random(seed)*2.0f-1.0f)*c[11]};}
 else return 0;
 pos={pos.x*c[5],pos.y*c[6],0};random(seed);
 float spread=(random(seed)*2.0f-1.0f)*c[31]*pi/180.0f;V3 velocity{std::cos(spread)*initial,std::sin(spread)*initial,0};
 const float dt=1.0f/c[1],noise_scale=std::pow(c[23],.25f)*(4.0f/std::pow(10.0f,.25f))-4.0f;
 constants[0]=std::max(std::abs(scale),.001f)*(scale<0?-1.0f:1.0f);constants[1]=angular;
 out[0]=pos.x;out[1]=pos.y;
 for(unsigned step=1;step<=ticks;++step){float age=float(step)*dt,phase=std::min(age/c[0],1.0f);
  V3 control=mul(normal(pos),radial);V3 force{c[12],c[13],0};float distance=std::sqrt(pos.x*pos.x+pos.y*pos.y);
  if(distance>0){force.x+=(pos.x*radial_accel-pos.y*tangent)/distance;force.y+=(pos.y*radial_accel+pos.x*tangent)/distance;}
  velocity=add(velocity,mul(force,dt));float time=age+c[34];
  V3 noise_pos=pos;if(!c[28]){noise_pos.x+=c[2];noise_pos.y+=c[3];}
  V4 n{{noise_pos.x*noise_scale+time*c[24],noise_pos.y*noise_scale+time*c[25],time*c[26],time*c[27]}};
  V3 direction=curl(n,c[22]);float amount=std::max(0.0f,std::min(1.0f,influence_birth*sample(influence,width,phase)));
  float gain=1.0f+(1.0f-amount)*.2f;
  velocity=blend(velocity,mul(direction,length(velocity)*gain),amount);control=blend(control,mul(direction,length(control)*gain),amount);
  V3 final_velocity=add(velocity,control);pos.x+=final_velocity.x*dt;pos.y+=final_velocity.y*dt;
  out[step*2]=pos.x;out[step*2+1]=pos.y;
 }
 return 1;
}
}
extern "C" NSPIRE_TURB_EXPORT float nspire_godot_noise4(const float*p){return nspire_source_turbulence::noise({{p[0],p[1],p[2],p[3]}});}
extern "C" NSPIRE_TURB_EXPORT void nspire_godot_curl4(const float*p,float strength,float*out){auto v=nspire_source_turbulence::curl({{p[0],p[1],p[2],p[3]}},strength);out[0]=v.x;out[1]=v.y;out[2]=v.z;}
extern "C" NSPIRE_TURB_EXPORT int nspire_ancient_trajectory(const float*c,uint32_t number,uint32_t seed,const float*curve,unsigned width,unsigned ticks,float*out,float*constants){return nspire_source_turbulence::trajectory(c,number,seed,curve,width,ticks,out,constants);}
'''

FLOAT_P=ctypes.POINTER(ctypes.c_float)
PROJECTION=3195634/16777216
ERROR_PIXELS=.125
COORD_QUANT=16


def helper(cache):
    bundled=os.environ.get('NSPIRE_FASTNOISE_LIBRARY')
    if bundled:
        path=Path(bundled).resolve(strict=True)
    else:
        directory=Path(cache);directory.mkdir(parents=True,exist_ok=True)
        digest=hashlib.sha256(_TURBULENCE_CALLER.encode()).hexdigest()
        source=directory/('turbulence-'+digest+'.cpp');path=source.with_suffix('.so')
        if not path.is_file():
            source.write_text(_TURBULENCE_CALLER)
            subprocess.run([os.environ.get('CXX','clang++-18'),'-std=c++17','-O2','-fwrapv','-ffp-contract=off','-shared','-fPIC',str(source),'-o',str(path)],check=True)
    library=ctypes.CDLL(str(path))
    library.nspire_godot_noise4.argtypes=[FLOAT_P];library.nspire_godot_noise4.restype=ctypes.c_float
    library.nspire_godot_curl4.argtypes=[FLOAT_P,ctypes.c_float,FLOAT_P];library.nspire_godot_curl4.restype=None
    library.nspire_ancient_trajectory.argtypes=[FLOAT_P,ctypes.c_uint32,ctypes.c_uint32,FLOAT_P,ctypes.c_uint,ctypes.c_uint,FLOAT_P,FLOAT_P]
    library.nspire_ancient_trajectory.restype=ctypes.c_int
    return library


def trajectory(library,config,curve,index,seed):
    c=np.asarray(config,dtype='<f4');ramp=np.ascontiguousarray(curve,dtype='<f4')
    if c.shape!=(37,) or ramp.ndim!=1:raise ValueError('Invalid source trajectory parameters')
    ticks=round(float(c[0])*float(c[1]));out=np.empty((ticks+1,2),dtype='<f4');constants=np.empty(2,dtype='<f4')
    ok=library.nspire_ancient_trajectory(c.ctypes.data_as(FLOAT_P),index,seed,ramp.ctypes.data_as(FLOAT_P),len(ramp),ticks,out.ctypes.data_as(FLOAT_P),constants.ctypes.data_as(FLOAT_P))
    if ok!=1 or not np.isfinite(out).all() or not np.isfinite(constants).all():raise ValueError('Invalid native source trajectory')
    return out,constants


def compress_track(screen):
    """Select source samples; prove every interpolated source sample <=1/8px.

    Q4 LCD coordinates contribute <=sqrt(2)/32px error. The actual integer
    runtime interpolation (including truncation) is checked, not an ideal
    float approximation. Quantized coordinates are not image resampling.
    """
    source=np.asarray(screen,dtype=np.float64)
    if source.ndim!=2 or source.shape[1]!=2 or len(source)<2 or not np.isfinite(source).all():raise ValueError('Invalid particle track')
    quant=np.where(source>=0,np.floor(source*COORD_QUANT+.5),np.ceil(source*COORD_QUANT-.5)).astype(np.int64)
    if np.abs(quant).max()>=32768:raise ValueError('Projected particle trajectory exceeds signed Q4 bounds')
    keep={0,len(source)-1};pending=[(0,len(source)-1)]
    rounding=math.sqrt(2)/256
    while pending:
        a,b=pending.pop()
        if b<=a+1:continue
        indexes=np.arange(a+1,b,dtype=np.int64)
        predicted=(quant[a]+(quant[b]-quant[a])[None,:]*(indexes-a)[:,None]/(b-a))/COORD_QUANT
        error=np.linalg.norm(predicted-source[indexes],axis=1)
        worst=int(np.argmax(error))
        if error[worst]+rounding>ERROR_PIXELS:
            k=int(indexes[worst]);keep.add(k);pending.extend(((a,k),(k,b)))
    keys=sorted(keep);maximum=0.
    for a,b in zip(keys,keys[1:]):
        indexes=np.arange(a,b+1,dtype=np.int64)
        predicted=(quant[a]+(quant[b]-quant[a])[None,:]*(indexes-a)[:,None]/(b-a))/COORD_QUANT
        maximum=max(maximum,float(np.linalg.norm(predicted-source[indexes],axis=1).max()))
    bound=maximum+rounding
    if bound>ERROR_PIXELS+1e-9:raise ValueError('Particle compression error bound failed')
    raw=b''.join(struct.pack('<Hhh',i,int(quant[i,0]),int(quant[i,1])) for i in keys)
    return raw,dict(source_samples=len(source),keys=len(keys),bytes=len(raw),maximum_lcd_error_bound=bound)

SCENES={
 'pael':'c68a425b3c53253abcc0bcafb26440999a00b4c9104e91f08d437e27aa990159',
 'orobas':'c2fff8b68795171ed775a1a8b10003a84dff84b00ae12c9b3aa6fba92757ad74',
 'nonupeipe':'29f03f3684d0c441a2d29e3078f21715519414d24cea77d9fffddd47a56ecc3e',
 'vakuu':'5aa121b200b8a94b432f3254307c74b5208eeb2eaefacefa7b59d764432cb146'}
NODE_ORDER=(('pael','square_specks'),('pael','twinkle_star_specks'),('pael','twinkle_star_specks2'),
 ('orobas','square_specks'),('orobas','square_specks2'),('orobas','stars'),
 ('nonupeipe','square_specks'),('nonupeipe','twinkle_star_specks'),
 ('vakuu','square_specks'),('vakuu','twinkle_star_specks'),('vakuu','square_specks2'),('vakuu','twinkle_star_specks2'))
EXPECTED_COUNTS=(30,30,30,30,30,100,60,60,30,30,30,30)


def source_emitters(pack):
    from world_scene_assets import SceneLoader,_property,_vector
    from world_room_shaders import generated_texture,curve_channel
    loader=SceneLoader(pack);documents={}
    for scene,digest in SCENES.items():
        path=f'scenes/events/background_scenes/{scene}.tscn'
        if hashlib.sha256(pack.read(path)).hexdigest()!=digest:raise ValueError('Ancient turbulence scene differs: '+scene)
        documents[scene]={a['name']:(a,p) for k,a,p in loader.document(path).sections if k=='node'}
    result=[]
    def curve(ref,default):
        if ref is None:return np.full(256,default,dtype='<f4')
        attrs,props=loader.resource(ref)
        if attrs.get('type')!='CurveTexture' or _property(props,'width',256)!=256:raise ValueError('Unreviewed particle curve texture')
        return curve_channel(loader,_property(props,'curve'),256)
    for index,(scene,name) in enumerate(NODE_ORDER):
        attrs,node=documents[scene][name];get=lambda k,d=None:_property(node,k,d)
        ma,material=loader.resource(get('process_material'));m=lambda k,d=None:_property(material,k,d)
        if attrs.get('parent')!='.' or ma.get('type')!='ParticleProcessMaterial' or not m('turbulence_enabled',False) or not m('particle_flag_disable_z',False):raise ValueError('Unreviewed particle hierarchy/material')
        if not get('visible',True) or get('amount',8)!=EXPECTED_COUNTS[index]:raise ValueError('Particle visibility/count differs')
        for key,default in [('explosiveness',0.),('randomness',0.),('speed_scale',1.),('amount_ratio',1.),('rotation',0.)]:
            if get(key,default)!=default:raise ValueError('Unhandled source node property '+key)
        # These source emitters have no drag, arbitrary force textures, collision,
        # initial displacement or moving parent. Reject such input, do not omit it.
        for key in ('linear_accel_min','linear_accel_max','damping_min','damping_max','initial_angle_min','initial_angle_max',
                    'turbulence_initial_displacement_min','turbulence_initial_displacement_max','lifetime_randomness',
                    'collision_mode','hue_variation_min','hue_variation_max','initial_velocity_min','initial_velocity_max'):
            if m(key,0)!=0:raise ValueError('Unhandled source particle property '+key)
        if abs(m('orbit_velocity_min',0))>=.01 or abs(m('orbit_velocity_max',0))>=.01:raise ValueError('Source orbit exceeds its .01 early-return bound')
        for key in ('linear_accel_curve','radial_accel_curve','tangential_accel_curve','damping_curve','radial_velocity_curve','directional_velocity_curve','velocity_limit_curve','angular_velocity_curve','color_initial_ramp'):
            if m(key) is not None:raise ValueError('Unhandled source particle curve '+key)
        origin=_vector(get('position',(0,0)));node_scale=_vector(get('scale',(1,1)))
        emission=_vector(m('emission_shape_scale',(1,1,1)),3);box=_vector(m('emission_box_extents',(1,1,1)),3)
        gravity=_vector(m('gravity',(0,-9.8,0)),3);speed=_vector(m('turbulence_noise_speed',(0,0,0)),3)
        if gravity[2] or not np.all(node_scale>0):raise ValueError('Unhandled particle transform')
        fixed=int(get('fixed_fps',30));fps=fixed if fixed else 30
        # Godot's fixed_fps=0 preprocess uses1/30; variable display deltas have
        # no single recorded history. The portable scene uses this same30Hz
        # source integrator and interpolates the motion on the display clock.
        config=[get('lifetime',1),fps,*origin,m('emission_shape',0),*emission,m('emission_sphere_radius',1),*box,
          *gravity[:2],m('scale_min',1),m('scale_max',1),m('angular_velocity_min',0),m('angular_velocity_max',0),
          m('tangential_accel_min',0),m('tangential_accel_max',0),m('radial_velocity_min',0),m('radial_velocity_max',0),
          m('turbulence_noise_strength',1),m('turbulence_noise_scale',9),*speed,m('turbulence_noise_speed_random',.2),
          float(bool(get('local_coords',False))),0,0,m('spread',45),m('turbulence_influence_min',.1),m('turbulence_influence_max',.1),0,
          m('radial_accel_min',0),m('radial_accel_max',0)]
        if len(config)!=37 or not np.isfinite(config).all():raise ValueError('Invalid source config')
        ref=m('color_ramp');colors=generated_texture(loader,ref)[0] if ref else np.ones((256,4),dtype='<f4')
        tint=_vector(m('color',(1,1,1,1)),4)*_vector(get('modulate',(1,1,1,1)),4)
        alpha=curve(m('alpha_curve'),1);scale=curve(m('scale_curve'),1)
        if colors.shape!=(256,4) or np.min(scale)<0 or np.max(scale)>=16:raise ValueError('Unreviewed particle display ramp')
        # Multiplication is retained as Q16 tint; source ramp bytes remain exact.
        ramp=np.rint(np.clip(colors,0,1)*255).astype('u1').tobytes()+np.rint(np.clip(alpha,0,1)*65535).astype('<u2').tobytes()+np.rint(scale*4096).astype('<u2').tobytes()
        blend=0;material_ref=get('material')
        if material_ref is not None:
            ba,bp=loader.resource(material_ref)
            if ba.get('type')!='CanvasItemMaterial':raise ValueError('Unreviewed particle Canvas material')
            blend=int(_property(bp,'blend_mode',0))
        if blend not in (0,1):raise ValueError('Unreviewed particle blend')
        result.append(dict(scene=scene,node=name,count=EXPECTED_COUNTS[index],config=config,curve=curve(m('turbulence_influence_over_life'),1),
          texture=loader.path(get('texture')),origin=origin,node_scale=node_scale,ramp=ramp,tint=np.rint(tint*65535).astype('<u2'),
          lifetime_ms=round(config[0]*1000),preprocess_ms=round(float(get('preprocess',0))*1000),fps=fps,
          source_fixed_fps=fixed,blend=blend,seed=0x534c5900+index))
    return result


def build_tracks(pack,cache,progress=print):
    library=helper(cache);results=[]
    for emitter in source_emitters(pack):
        tracks=[];reports=[];constants=[]
        config=list(emitter['config'])
        for index in range(emitter['count']):
            # The bank records source NUMBER=index and a stable scene seed;
            # different nodes retain independent source fields and birth seeds.
            config[34]=index*config[0]/emitter['count']
            points,c=trajectory(library,config,emitter['curve'],index,emitter['seed'])
            screen=(points.astype(np.float64)*emitter['node_scale']+emitter['origin'])*PROJECTION
            screen[:,0]-=114518200509/16777216/256
            screen[:,1]+=89989748010/16777216/256
            raw,report=compress_track(screen);tracks.append(raw);reports.append(report);constants.append(c)
        results.append((emitter,tracks,np.asarray(constants),reports))
        progress(f"turbulence {emitter['scene']}/{emitter['node']}: {sum(len(r) for r in tracks)} bytes, {sum(r['keys'] for r in reports)} keys, {max(r['maximum_lcd_error_bound'] for r in reports):.6f}px",flush=True)
    return results

BASE_AUX,AUXILIARY,BASE_IMAGES,IMAGES=7223,7224,59717,59729
HEADER_BYTES,RAMP_BYTES,DIRECTORY_BYTES=64,2048,12
TEXTURES={
 'images/packed/vfx/generic/square_speck.png':'0246f62894fd2ce4ea1d440fce9f6c13f4a5898e5060933864590349eb6ff3ea',
 'images/packed/vfx/generic/twinkle_star.png':'5424a60f4f87f62b0cd347d95be4e4998b6eb100378f8c09a9f4d0edad5e6161',
 'images/vfx/dot.png':'65d3d3c2d7b33ab973cb2291e94f13a37164a3d762556c868ddb6000cee445e8'}


def pack_table(pack,emitter,tracks,constants):
    from build_content import texture
    rgba=np.asarray(texture(pack,emitter['texture']).convert('RGBA'),dtype='u1')
    if hashlib.sha256(rgba.tobytes()).hexdigest()!=TEXTURES[emitter['texture']] or not np.all(rgba[:,:,:3]==255):raise ValueError('Original particle mask differs')
    height,width=rgba.shape[:2];count=emitter['count'];palette=HEADER_BYTES
    texture_offset=palette+512;ramp_offset=texture_offset+width*height;directory=ramp_offset+RAMP_BYTES
    offset=directory+count*DIRECTORY_BYTES;records=[]
    for raw,constant in zip(tracks,constants):
        scale=round(float(constant[0])*4096);angular=round(float(constant[1])*128)
        if not 0<scale<=65535 or not -32768<=angular<32768:raise ValueError('Source particle display constants exceed table')
        records.append(struct.pack('<IHHhH',offset,len(raw)//6,scale,angular,0));offset+=len(raw)
    ex=round(width*PROJECTION*emitter['node_scale'][0]*256);ey=round(height*PROJECTION*emitter['node_scale'][1]*256)
    head=struct.pack('<4sHHII4H6I4HII',b'ATP1',2,count,emitter['lifetime_ms'],emitter['preprocess_ms'],emitter['fps'],emitter['blend'],width,height,
      texture_offset,ramp_offset,directory,offset,ex,ey,*map(int,emitter['tint']),palette,0)
    if len(head)!=HEADER_BYTES:raise ValueError('Particle table header size')
    planes=[np.frombuffer(track,dtype='u1').reshape(-1,6).T.copy().tobytes() for track in tracks]
    raw=head+b'\xff'*512+rgba[:,:,3].tobytes()+emitter['ramp']+b''.join(records)+b''.join(planes)
    if len(raw)!=offset:raise ValueError('Particle table size differs')
    return table_image(raw)


def validate_table(raw):
    if len(raw)<HEADER_BYTES:raise ValueError('Truncated trajectory header')
    fields=struct.unpack_from('<4sHHII4H6I4HII',raw)
    magic,version,count,life,pre,fps,blend,width,height,tex,ramp,directory,total,ex,ey,*tail=fields
    if magic!=b'ATP1' or version!=2 or not 0<count<=100 or fps!=30 or not 0<life<=30000 or pre>100000 or blend not in (0,1):raise ValueError('Invalid trajectory identity')
    palette,reserved=tail[4:]
    if reserved or palette!=HEADER_BYTES or tex!=palette+512 or ramp!=tex+width*height or directory!=ramp+RAMP_BYTES or total>len(raw) or any(raw[total:]):raise ValueError('Invalid trajectory regions')
    if raw[palette:tex]!=b'\xff'*512 or not 0<ex<=65535 or not 0<ey<=65535:raise ValueError('Invalid source mask palette or extent')
    end=directory+count*DIRECTORY_BYTES;ticks=life*fps//1000
    for i in range(count):
        start,n,scale,angular,reserved=struct.unpack_from('<IHHhH',raw,directory+i*DIRECTORY_BYTES)
        if start!=end or not 2<=n<=ticks+1 or start+n*6>total or not scale or reserved:raise ValueError('Invalid trajectory record')
        interleaved=np.frombuffer(raw[start:start+n*6],dtype='u1').reshape(6,n).T.copy().tobytes()
        keys=np.frombuffer(interleaved,dtype=np.dtype([('tick','<u2'),('x','<i2'),('y','<i2')]))
        if keys['tick'][0]!=0 or keys['tick'][-1]!=ticks or np.any(np.diff(keys['tick'].astype(int))<=0):raise ValueError('Invalid source trajectory ticks')
        end=start+n*6
    if end!=total:raise ValueError('Unreferenced trajectory bytes')
    return dict(count=count,life_ms=life,preprocess_ms=pre,fps=fps,blend=blend,texture_size=[width,height],used_bytes=total)



def table_image(raw):
    """Keep source table bytes intact inside a decoder-supported image view.

    The one 100-particle Orobas table exceeds the existing240-row bound at
    width256. Only that reviewed131072-byte padded table uses320x205; all
    offsets and source trajectory bytes are unchanged. No decoder bound grows.
    """
    report=validate_table(raw);used=report['used_bytes']
    padded=(used+511)//512*512
    if padded<=256*240*2:
        width,height=256,padded//512
    elif padded==131072 and report['count']==100:
        width,height=320,205
    else:raise ValueError('Source trajectory exceeds reviewed image geometry')
    return width,height,0,raw[:used]+bytes(width*height*2-used)


def view_geometry(index,width,height):
    return (width,height)==(320,205) if index==5 else width==256 and 0<height<=240


def sample_assets(pack,cache):
    import json
    from content_tiles import atomic_bytes
    directory=Path(cache);directory.mkdir(parents=True,exist_ok=True)
    rows=[];proof=[]
    for index,(emitter,tracks,constants,reports) in enumerate(build_tracks(pack,directory/'helper')):
        image=pack_table(pack,emitter,tracks,constants);validate_table(image[3])
        filename=f'turbulence-{index:02d}.bin';atomic_bytes(directory/filename,image[3]);rows.append(image)
        proof.append(dict(scene=emitter['scene'],node=emitter['node'],count=emitter['count'],source_fixed_fps=emitter['source_fixed_fps'],
          source_texture=emitter['texture'],texture_rgba_sha256=TEXTURES[emitter['texture']],image_width=image[0],image_height=image[1],image_bytes=len(image[3]),image_sha256=hashlib.sha256(image[3]).hexdigest(),
          keys=sum(r['keys'] for r in reports),source_samples=sum(r['source_samples'] for r in reports),maximum_lcd_error_bound=max(r['maximum_lcd_error_bound'] for r in reports),file=filename))
    atomic_bytes(directory/'trajectories.json',(json.dumps(dict(schema=1,source_scenes=SCENES,source_equations='Godot4.5.1 ParticleProcessMaterial float32, FMA disabled',
      recorded_gpu_random_history=False,source_seed_bank=True,source_fixed_fps_zero_portable_step=30,
      actor_dimensions_changed=False,actor_frames_changed=False,persistent_allocation_growth=0,emitters=proof),indent=2)+'\n').encode())
    return rows,proof


def validate(source,image):
    if (len(source.aux),len(source.images))!=(AUXILIARY,IMAGES) or source.aux[BASE_AUX]!=BASE_IMAGES:raise ValueError('Ancient trajectories require their exact typed endpoint')
    for i,count in enumerate(EXPECTED_COUNTS):
        w,h,fmt,raw=image(BASE_IMAGES+i)
        if not view_geometry(i,w,h) or fmt or len(raw)!=w*h*2:raise ValueError('Invalid trajectory data view')
        if table_image(raw)!=(w,h,fmt,raw):raise ValueError('Noncanonical trajectory image padding')
        report=validate_table(raw)
        if report['count']!=count:raise ValueError('Original particle count differs')
    return dict(images=tuple(range(BASE_IMAGES,IMAGES)),auxiliary=(BASE_AUX,),particle_count=sum(EXPECTED_COUNTS))


def append(data,pack,work,sampled=None):
    import json
    import content_tiles as t
    from coalesce_tile_resources import view
    from patch_frontend_assets import patch_pack
    from split_content import LANGUAGES
    from no_animations import reduced,mark
    from tanx_scene_ui23 import group_images
    source=view(data)
    t.require(source.version==46 and (len(source.aux),len(source.images),len(source.clips))==(BASE_AUX,BASE_IMAGES,1783),
              'Turbulent particle append requires the exact source Nonu fog endpoint')
    directory=Path(work);directory.mkdir(parents=True,exist_ok=True)
    if sampled is None:images,proof=sample_assets(pack,directory)
    else:
        sampled=Path(sampled);receipt=json.loads((sampled/'trajectories.json').read_text());proof=receipt['emitters'];images=[]
        t.require(receipt['source_scenes']==SCENES and len(proof)==len(NODE_ORDER),'Sampled trajectory source differs')
        for row,(scene,node),count in zip(proof,NODE_ORDER,EXPECTED_COUNTS):
            raw=(sampled/row['file']).read_bytes();info=validate_table(raw)
            t.require((row['scene'],row['node'],info['count'])==(scene,node,count) and t.sha256(raw)==row['image_sha256'],
                      'Sampled source trajectory bytes differ')
            images.append(table_image(raw))
    rows=[]
    for i,(w,h,fmt,raw) in enumerate(images):
        filename=f'{i}.bin';t.atomic_bytes(directory/filename,raw)
        rows.append(dict(width=w,height=h,format=fmt,file=filename,bytes=len(raw),sha256=t.sha256(raw),raw_sha256=t.sha256(raw)))
    art=group_images(rows,directory,group_bytes=256*1024)
    manifest=dict(images=rows,auxiliary_image_ids=[BASE_IMAGES],localized_text={code:{} for code in LANGUAGES})
    output,detail=patch_pack(data,manifest,directory,base_aux=BASE_AUX,auxiliary=AUXILIARY,input_version=46,version=46,
      texts=(),base_clips=len(source.clips),base_strings=len(source.strings),appended_art=art)
    if reduced(data):output=mark(output)
    target=view(output);validate(target,target.image_raw)
    t.require(target.clips==source.clips and target.cards==source.cards and target.aux[:BASE_AUX]==source.aux and reduced(output)==reduced(data),
              'Turbulent append changed old identities/profile')
    for i,row in enumerate(images):t.require(target.image_raw(BASE_IMAGES+i)==row,'Source trajectory table differs after encoding')
    return output,dict(complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(output),added_bytes=len(output)-len(data),
      grouping=art['report'],append=detail,emitters=proof,decoded_bytes=sum(len(row[3]) for row in images),appended_images=len(images),
      persistent_allocation_growth=0,actor_frame_changes=0,actor_dimension_changes=0,source_gpu_random_history_recorded=False)



def helper_self_test(library):
    library.nspire_godot_noise4.argtypes=[FLOAT_P];library.nspire_godot_noise4.restype=ctypes.c_float
    library.nspire_godot_curl4.argtypes=[FLOAT_P,ctypes.c_float,FLOAT_P];library.nspire_godot_curl4.restype=None
    library.nspire_ancient_trajectory.argtypes=[FLOAT_P,ctypes.c_uint32,ctypes.c_uint32,FLOAT_P,ctypes.c_uint,ctypes.c_uint,FLOAT_P,FLOAT_P]
    library.nspire_ancient_trajectory.restype=ctypes.c_int
    points=np.array([[0,0,0,0],[.25,-.5,1.125,2.75],[-31.5,50.25,0,70.125],[2000,-1300,0,3547.]],dtype='<f4')
    values=np.array([library.nspire_godot_noise4(p.ctypes.data_as(FLOAT_P)) for p in points],dtype='<f4')
    digest=hashlib.sha256(values.tobytes()).hexdigest()
    if digest!='1c0ca5e2f8b26acee6d23c1c24313040becd77f483e63180a87498764b75435f':raise ValueError('Bundled Godot4D gradient differs from source oracle')
    c=np.zeros(37,dtype='<f4');c[[0,1,4,5,6,7,8,14,15,22,23,28]]=[1,30,2,1,0,1,1,1,1,1,9,1];c[13]=70
    track,constants=trajectory(library,c,np.ones(256,dtype='<f4'),0,12345)
    n=np.arange(31,dtype=np.float64);expected=70/(30*30)*n*(n+1)/2
    if not np.allclose(track[:,1],expected,rtol=1e-6,atol=1e-5):raise ValueError('Bundled source particle integrator differs')
    return dict(passed=True,noise_sha256=digest,gravity_source_samples=len(track),source_gpu_random_history_recorded=False)


def main():
    import argparse,json
    import content_tiles as t
    from pck import Pack
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck',type=Path,required=True);parser.add_argument('--work',type=Path,required=True)
    parser.add_argument('--input',type=Path);parser.add_argument('--output',type=Path);parser.add_argument('--sample-only',action='store_true')
    parser.add_argument('--sampled',type=Path);args=parser.parse_args()
    if not args.sample_only and not(args.input and args.output):parser.error('Supply input/output or sample-only')
    for path in (args.pck,args.work,args.input,args.output,args.sampled):
        if path is not None:t.private_path(path,'Private original particle input/output')
    pack=Pack(args.pck)
    try:
        if args.sample_only:
            images,proof=sample_assets(pack,args.work);print(json.dumps(dict(complete=True,emitters=len(proof),decoded_bytes=sum(len(row[3]) for row in images))),flush=True)
        else:
            output,proof=append(args.input.read_bytes(),pack,args.work,args.sampled);t.atomic_bytes(args.output,output)
            t.atomic_bytes(args.output.with_suffix('.json'),(json.dumps(proof,indent=2)+'\n').encode());print(json.dumps(dict(complete=True,added_bytes=proof['added_bytes'])),flush=True)
    finally:pack.close()

if __name__=='__main__':main()
