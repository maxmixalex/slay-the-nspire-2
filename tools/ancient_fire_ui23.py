#!/usr/bin/env python3
"""Source-bound237Tez fire nodes and exact-resolution source shader channels.

Noise color quantization is not enabled; a page-local, exactly reversible up
predictor is restored once in the existing image cache. Raw mode remains valid.
No actor pixels/geometry/cadence or
existing source caches change. The original20s shader clock is independent of
the16.667s body loop. Source shader blend modes override ShaderMaterial's
ordinary default. The four behind-body Neow fires are a separate ordered pass.
"""
from __future__ import annotations
import json,struct
from pathlib import Path
import numpy as np
import content_tiles as t
import ancient_lights_ui23 as lights
import tez_effects_ui23 as effects
import world_room_shaders as shader

BASE_AUX,AUXILIARY,BASE_IMAGES,IMAGES=7227,7228,59732,59751
NODES,FRAMES,DYNAMIC,MODELS,COMMANDS,PAGE_BYTES=237,506,21,3,401,131072
NODES_AT=64;MODELS_AT=NODES_AT+NODES*40;DELTAS_AT=MODELS_AT+MODELS*128
ORDER_AT=DELTAS_AT+DYNAMIC*FRAMES*12;LENGTH=ORDER_AT+COMMANDS*2
TEXTURES=(
 ('images/vfx/fire/basic_fire_noise.png',1024,1024,(2,),'ecf35f98adc41b1ff4a2ab6601a7b0eb335df4d6d0cf7284db78c1608df42ac9'),
 ('images/vfx/fire/triangle_noise_tile.png',500,500,(0,1,2),'5c5385b4903378661a1c605411d577e77669272fe21f84ef3e96f33064665d71'),
 ('images/vfx/fire/fire_base_slim.png',500,500,(2,),'61fcf46ac4c8ddff3b0aac21ea549e18b9d57ccd3e4dd9a30ed63c7934541f2f'),
 ('images/vfx/fire/fire_bottom_mask.png',50,500,(2,),'ab9fb251311ba2a0badc7b5e952d13761fbe526764c0f120e416a5ed0a71f55f'),
 ('images/vfx/fire/zigzag_fire_distortion.png',20,500,(2,),'d1222cab08bab073d48ca535562a81d25b3f8c5c7485d8eed422031d559c4cec'))
EXPECTED_TEXTURES=dict(Noise1Texture=TEXTURES[1][0],Noise2Texture=TEXTURES[0][0],NoiseMask=TEXTURES[3][0],
                      Distortion1Texture=TEXTURES[1][0],Distortion2Texture=TEXTURES[4][0],DistortionMask=TEXTURES[3][0])


def mat(a):return np.array([*np.asarray(a).reshape(2,3),[0,0,1.]])

def geometry(transform,rect):
 x,y,w,h=rect;m=np.asarray(transform);center=m@np.array([x+w/2,y+h/2,1.])
 return lights.round_signed(np.array([center[0],center[1],m[0,0]*w,m[1,0]*w,m[0,1]*h,m[1,1]*h])*256)


def model(uniforms):
 t.require(uniforms['InvertNoiseMask'] is True and list(uniforms['GlobalOffset'])==[0,0],'Unreviewed fire mask/offset mode')
 values=[]
 for name in ('Noise1','Noise2','Distortion1','Distortion2'):
  values.extend(uniforms[name+('Scaling' if name.startswith('Noise') else 'Scale')]);values.extend(uniforms[name+'Panning']);values.append(uniforms[name+'Strength'])
 for name in ('NoiseMask','DistortionMask'):values.extend(uniforms[name+'Scale']);values.extend(uniforms[name+'Offset'])
 values.extend(uniforms['InnerColorStep']);values.extend(uniforms['OuterStep'])
 t.require(len(values)==32 and np.all(np.isfinite(values)),'Fire model width/nonfinite')
 return lights.round_signed(np.asarray(values)*16777216).astype('<i4').tobytes()


def parameters(raw):
 t.require(len(raw)==(LENGTH+639)//640*640 and raw[:4]==b'AFR1','Fire parameter extent/magic')
 t.require(struct.unpack_from('<6H',raw,4)==(1,NODES,FRAMES,DYNAMIC,MODELS,COMMANDS) and
  struct.unpack_from('<5I2H2I',raw,16)==(NODES_AT,MODELS_AT,DELTAS_AT,ORDER_AT,LENGTH,BASE_IMAGES,18,20000,16667) and
  struct.unpack_from('<I',raw,48)[0] in (0,1) and not any(raw[52:64]) and not any(raw[LENGTH:]),'Fire header/offset/padding differs')
 dynamic=[]
 for i in range(NODES):
  values=struct.unpack_from('<6i6H4B',raw,NODES_AT+i*40)
  t.require(all(abs(q)<8388608 for q in values[:6]) and all(abs(q)<32768 for q in values[2:6]) and
    values[12]<MODELS and values[13] in (0,1) and values[14] in (*range(DYNAMIC),255) and not values[15], 'Fire geometry/model/delta range')
  if values[14]!=255:dynamic.append(values[14])
 t.require(dynamic==list(range(DYNAMIC)),'Fire moving source slots differ')
 for i in range(MODELS):
  q=struct.unpack_from('<32i',raw,MODELS_AT+i*128)
  t.require(all(abs(v)<=4*16777216 for v in q) and 0<=q[28]<q[29]<=16777216 and 0<=q[30]<q[31]<=16777216 and min(q[29]-q[28],q[31]-q[30])>=65536,'Fire model arithmetic bounds')
 counts=[0,0,0]
 for cmd in struct.unpack_from('<401H',raw,ORDER_AT):
  kind,index=cmd>>14,cmd&16383;t.require(kind<3 and index==counts[kind],'Fire source order skips/duplicates nodes');counts[kind]+=1
 t.require(counts==[90,74,237],'Fire/light/cinder source registry incomplete')
 return dict(nodes=NODES,frames=FRAMES,models=MODELS,order=COMMANDS,source_shader_period_ms=20000,noise_prediction=('raw','up')[struct.unpack_from('<I',raw,48)[0]])


def pack_parameters(rows,frames):
 t.require(len(frames)==FRAMES and all(len(f)==90 for f in frames),'Fire source light/bone cache incomplete')
 first=frames[0];anchors={}
 for i,r in enumerate(first):
  key=effects_bone(r['path'])
  if key:anchors.setdefault(key,i)
 nodes=[];deltas=[];models=[];order=[];counts=dict(light=0,cinder=0,fire=0)
 for r in rows:
  kind=r['kind'];index=counts[kind];counts[kind]+=1;order.append((('light','cinder','fire').index(kind)<<14)|index)
  if kind!='fire':continue
  t.require(r['texture']==TEXTURES[2][0] and r['textures']==EXPECTED_TEXTURES and r['tint']==[1,1,1,1], 'Fire source texture/tint binding differs')
  definition=shader.SHADERS[r['shader']];t.require(definition[0]=='fire','Unsupported dark fire family')
  blend=definition[2] # Actual pinned shader render_mode, not ShaderMaterial default.
  encoded=model(r['uniforms'])
  if encoded not in models:models.append(encoded)
  key=effects_bone(r['path']);base=mat(r['matrix'])
  if key:
   t.require(key in anchors,'Fire bone has no source sampled anchor')
   anchor=anchors[key];relative=np.linalg.inv(mat(first[anchor]['matrix']))@base
   values=np.asarray([geometry(mat(f[anchor]['matrix'])@relative,r['rect']) for f in frames])
  else:values=np.asarray([geometry(base,r['rect'])]*FRAMES)
  changed=bool(np.any(values!=values[0]));slot=len(deltas) if changed else 255
  if changed:
   difference=values-values[0];t.require(np.all((difference>=-32768)&(difference<=32767)), 'Fire bone delta exceeds signed16')
   deltas.append(difference.astype('<i2').tobytes())
  colors=np.asarray([*r['uniforms']['OuterColor'][:3],*r['uniforms']['InnerColor'][:3]])
  t.require(np.all((colors>=0)&(colors<=1)), 'Fire source color outside reviewed range')
  colors=lights.round_signed(colors*65535)
  nodes.append(struct.pack('<6i6H4B',*values[0],*colors,models.index(encoded),blend,slot,0))
 t.require(counts==dict(light=90,cinder=74,fire=NODES) and len(deltas)==DYNAMIC and len(models)==MODELS,'Fire original source cardinality changed')
 header=bytearray(64);header[:4]=b'AFR1';struct.pack_into('<6H',header,4,1,NODES,FRAMES,DYNAMIC,MODELS,COMMANDS)
 struct.pack_into('<5I2H2I',header,16,NODES_AT,MODELS_AT,DELTAS_AT,ORDER_AT,LENGTH,BASE_IMAGES,18,20000,16667)
 raw=bytes(header)+b''.join(nodes)+b''.join(models)+b''.join(deltas)+struct.pack('<401H',*order)
 raw+=bytes((-len(raw))%640);parameters(raw)
 return raw


def effects_bone(path):
 p=path.split('/')
 for i,v in enumerate(p):
  if v.startswith('SpineBoneNode'):return '/'.join(p[:i+1])
 return None


def noise_pages():
 for _,width,height,channels,_ in TEXTURES:
  total=width*height*len(channels)
  for offset in range(0,total,PAGE_BYTES):
   yield min(PAGE_BYTES,total-offset),width*len(channels)


def predict_page(raw,used,stride,inverse=False):
 t.require(0<stride<=used<=PAGE_BYTES and len(raw)==(used+639)//640*640 and not any(raw[used:]),'Fire predictor extent/padding differs')
 out=bytearray(raw)
 if inverse:
  for i in range(stride,used):out[i]=(out[i]+out[i-stride])&255
 else:
  for i in range(stride,used):out[i]=(raw[i]-raw[i-stride])&255
 return bytes(out)


def predicted_images(images):
 t.require(len(images)==19 and parameters(images[-1][3])['noise_prediction']=='raw','Fire prediction requires original raw source pages')
 out=[];records=[]
 for index,((w,h,fmt,raw),(used,stride)) in enumerate(zip(images[:-1],noise_pages())):
  t.require((w,h,fmt)==(320,(used+639)//640,0),'Fire prediction geometry differs')
  encoded=predict_page(raw,used,stride)
  t.require(predict_page(encoded,used,stride,True)==raw,'Fire up predictor is not exactly reversible')
  out.append((w,h,fmt,encoded));records.append(dict(image=BASE_IMAGES+index,used=used,stride=stride,
   source_sha256=t.sha256(raw),encoded_sha256=t.sha256(encoded)))
 raw=bytearray(images[-1][3]);struct.pack_into('<I',raw,48,1);parameters(raw)
 out.append((*images[-1][:3],bytes(raw)))
 return out,records


def source_assets(pack,sampler,runtime,light_work,noise_prediction="up"):
 import animation,world_scene_assets as world
 from world_raster import raster
 from ancient_scene_ui23 import SCENES
 from build_content import texture
 animation.raster=raster
 directory,record=lights.sample_scene(pack,sampler,runtime,SCENES[1],Path(light_work)/'sampled')
 frames=[json.loads((directory/f'{i:04}.json').read_text())['lights']['nodes']for i in range(FRAMES)]
 with effects.source_renderer():_,diagnostics=world.render_scene(pack,effects.SCENE,320,240,viewport=(1680,1260),sampler=sampler)
 raw=pack_parameters(diagnostics['tez_effects'],frames);images=[];proof=[]
 for name,w,h,channels,digest in TEXTURES:
  rgba=np.asarray(texture(pack,name).convert('RGBA'),dtype='u1')
  t.require(rgba.shape==(h,w,4) and t.sha256(rgba.tobytes())==digest,'Fire original texture identity differs')
  pixels=rgba[:,:,channels].tobytes();first=len(images)
  for offset in range(0,len(pixels),PAGE_BYTES):
   part=pixels[offset:offset+PAGE_BYTES];part+=bytes((-len(part))%640);images.append((320,len(part)//640,0,part))
  proof.append(dict(source=name,rgba_sha256=digest,width=w,height=h,channels=channels,
                    first=BASE_IMAGES+first,pages=len(images)-first,channel_bytes=len(pixels),channel_sha256=t.sha256(pixels)))
 t.require(len(images)==18,'Fire native texture page count differs')
 images.append((320,len(raw)//640,0,raw))
 result=dict(source_scene_sha256=t.sha256(pack.read(effects.SCENE)),light_source_signature=record['signature'],
   shader_sources={k:v[1]for k,v in shader.SHADERS.items()if k in {r['shader']for r in diagnostics['tez_effects']if r['kind']=='fire'}},
   textures=proof,source_nodes=237,source_order=401,original_texture_resolution=True,noise_precision='exact source byte channels',
   source_shader_clock_ms=20000,body_clock_ms=16667,neow_behind_body_fire_implemented=False)
 t.require(noise_prediction in ('raw','up'),'Unknown fire prediction')
 result['noise_prediction']=noise_prediction
 if noise_prediction=='up':images,result['predicted_pages']=predicted_images(images)
 return images,result


def validate(source,image):
 t.require((len(source.aux),len(source.images))==(AUXILIARY,IMAGES) and source.aux[BASE_AUX]==IMAGES-1,'Fire exact catalog endpoint differs')
 at=BASE_IMAGES
 for _,w,h,channels,_ in TEXTURES:
  count=w*h*len(channels)
  for start in range(0,count,PAGE_BYTES):
   width,height,fmt,raw=image(at);used=min(PAGE_BYTES,count-start)
   t.require(width==320 and height==(used+639)//640 and fmt==0 and len(raw)==width*height*2 and not any(raw[used:]),'Fire texture page range/padding differs')
   at+=1
 width,height,fmt,raw=image(at)
 t.require((width,height,fmt)==(320,(LENGTH+639)//640,0),'Fire parameter image geometry differs');parameters(raw)


def append(data,images,proof,work):
 from coalesce_tile_resources import view
 from patch_frontend_assets import patch_pack
 from split_content import LANGUAGES
 from tanx_scene_ui23 import group_images
 from no_animations import reduced,mark
 source=view(data);t.require(source.version==46 and (len(source.aux),len(source.images),len(source.clips))==(BASE_AUX,BASE_IMAGES,1783),'Fire requires exact preceding Neow particle catalog')
 work=Path(work);work.mkdir(parents=True,exist_ok=True);rows=[]
 for i,(w,h,fmt,raw)in enumerate(images):
  name=f'{i:02}-{t.sha256(raw)}.bin';t.atomic_bytes(work/name,raw)
  rows.append(dict(width=w,height=h,format=fmt,file=name,bytes=len(raw),sha256=t.sha256(raw),raw_sha256=t.sha256(raw)))
 art=group_images(rows,work,group_bytes=256*1024)
 manifest=dict(images=rows,auxiliary_image_ids=[IMAGES-1],localized_text={code:{}for code in LANGUAGES})
 out,detail=patch_pack(data,manifest,work,base_aux=BASE_AUX,auxiliary=AUXILIARY,input_version=46,version=46,texts=(),base_clips=len(source.clips),base_strings=len(source.strings),appended_art=art)
 if reduced(data):out=mark(out)
 target=view(out);validate(target,target.image_raw)
 t.require(target.clips==source.clips and target.cards==source.cards and target.aux[:BASE_AUX]==source.aux and reduced(out)==reduced(data),'Fire append changed prior identities/profile')
 for i,row in enumerate(images):t.require(target.image_raw(BASE_IMAGES+i)==row,'Fire page decode differs')
 return out,dict(proof,complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(out),added_bytes=len(out)-len(data),grouping=art['report'],append=detail,decoded_bytes=sum(len(row[3])for row in images),persistent_allocation_growth=0)


def main():
 import argparse
 from pck import Pack
 from world_enemy_assets import SpineSampler
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('pck','runtime','work','light-work'):p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--input',type=Path);p.add_argument('--output',type=Path);p.add_argument('--sample-only',action='store_true');p.add_argument('--raw-noise',action='store_true',help='Keep exact raw pages without lossless up prediction');a=p.parse_args()
 t.require(a.sample_only or a.input and a.output,'Supply input/output or sample-only')
 for path in (a.pck,a.runtime,a.work,a.light_work,a.input,a.output):
  if path is not None:t.private_path(path,'Private original fire input/output')
 a.work.mkdir(parents=True,exist_ok=True);pack=Pack(a.pck)
 try:
  with SpineSampler(pack,runtime=a.runtime,work_dir=a.work/'adapter')as sampler:images,proof=source_assets(pack,sampler,a.runtime,a.light_work,'raw' if a.raw_noise else 'up')
  if a.sample_only:
   for i,row in enumerate(images):t.atomic_bytes(a.work/f'{i:02}.bin',row[3])
   t.atomic_bytes(a.work/'source.json',(json.dumps(proof,indent=2)+'\n').encode())
  else:
   out,proof=append(a.input.read_bytes(),images,proof,a.work/'append');t.atomic_bytes(a.output,out)
   t.atomic_bytes(a.output.with_suffix('.json'),(json.dumps(proof,indent=2)+'\n').encode())
 finally:pack.close()

if __name__=='__main__':main()
