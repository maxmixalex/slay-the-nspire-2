#!/usr/bin/env python3
"""Four original Neow fires behind the animated foreground.

All546 source poses keep33ms sampling. The source foreground is an affine
Normal/Multiply/Screen transfer; a bounded scalar transmission and a proved
all-clock contribution support place the fire behind that foreground without
another framebuffer. Full mode uses cyclic8-level/8-error transmission hold;
reduced mode uses the original first pose. Both share the reviewed250-square
triangle field. Other source channels, shader uniforms and geometry are intact.
"""
from __future__ import annotations
import contextlib,hashlib,json,struct
from functools import lru_cache
from pathlib import Path
import numpy as np
import content_tiles as t
import ancient_fire_ui23 as fire
import ancient_lights_ui23 as lights
import tez_effects_ui23 as effects
import world_room_shaders as shader
import world_scene_assets as world

BASE_AUX,AUXILIARY,BASE_IMAGES=7228,7229,59741
FRAMES,STEP_MS,DURATION,KEY_INTERVAL=546,33,18000,32
TEXTURE_PAGES,FIRST_MASK,PARAMETER,IMAGES=6,59747,60293,60294
PIXELS,SPANS=17485,705
NODES_AT,MODELS_AT,ROWS_AT,SPANS_AT,LENGTH=64,192,448,1412,4232
TEXTURES=(
 ('images/vfx/water_texture.png',600,636,(2,),'000f167a2874c1a862db56af80ff4e382feded695772574a6cf41ae178c1d727'),
 ('images/vfx/light.png',256,256,(0,1,2),'3658f5fa81e9e00238496f5b3968da250a4ebc9dee31e567dbb45b36b3c3b62e'),
 ('images/vfx/tileable_noise_2.png',100,100,(0,1),'15d2959105ecba00bf71a44b812afab17aa08bed08b84a384672ef78b5219fef'))
EXPECTED_TEXTURES=dict(Noise1Texture=TEXTURES[0][0],Noise2Texture=TEXTURES[0][0],NoiseMask=TEXTURES[1][0],
 Distortion1Texture=fire.TEXTURES[1][0],Distortion2Texture=TEXTURES[1][0],DistortionMask=TEXTURES[2][0])


def encode(raw,reference,frame):
 t.require(len(raw)==len(reference)==PIXELS and 0<=frame<FRAMES,'Neow patch input extent')
 a=np.frombuffer(raw,'u1');b=np.frombuffer(reference,'u1');indices=np.flatnonzero(a!=b);out=bytearray();records=0;cursor=0
 def emit(start,count,repeat=False):
  nonlocal cursor,records
  skip=start-cursor
  out.extend(struct.pack('<HH',skip,count|(32768 if repeat else 0)));out.extend(raw[start:start+1]if repeat else raw[start:start+count]);cursor=start+count;records+=1
 at=0
 while at<len(indices):
  start=int(indices[at]);end=start+1;at+=1
  while at<len(indices)and indices[at]-end<=5:end=int(indices[at])+1;at+=1
  pos=start
  while pos<end:
   run=pos+1
   while run<end and run-pos<32767 and raw[run]==raw[pos]:run+=1
   if run-pos>=4:emit(pos,run-pos,True);pos=run;continue
   stop=pos+1
   while stop<end and stop-pos<32767:
    if stop+3<end and raw[stop]==raw[stop+1]==raw[stop+2]==raw[stop+3]:break
    stop+=1
   emit(pos,stop-pos);pos=stop
 raw=struct.pack('<IHH',len(out)+8,frame,records)+out
 return raw+bytes((-len(raw))%512)


def decode(raw,reference,frame):
 t.require(len(reference)==PIXELS and 8<=len(raw)<=256*240*2 and len(raw)%512==0,'Neow patch extent')
 length,index,records=struct.unpack_from('<IHH',raw)
 t.require(index==frame and 0<=frame<FRAMES and 8<=length<=len(raw) and records<=(length-8)//4,'Neow patch header')
 out=bytearray(reference);at=8;cursor=0
 for _ in range(records):
  t.require(at+4<=length,'Neow patch record truncated');skip,code=struct.unpack_from('<HH',raw,at);at+=4;count=code&32767;cursor+=skip
  t.require(cursor+count<=PIXELS and (count or skip and not code&32768),'Neow patch bounds/empty record')
  used=1 if code&32768 else count;t.require(at+used<=length,'Neow patch data truncated')
  out[cursor:cursor+count]=raw[at:at+1]*count if code&32768 else raw[at:at+count];at+=used;cursor+=count
 t.require(at==length and not any(raw[length:]),'Neow patch trailing bytes')
 return bytes(out)


def spans(support):
 t.require(support.shape==(240,320) and support.dtype==np.bool_,'Neow support shape')
 rows=[];segments=[];pixel=0
 for row in support:
  rows.append((len(segments),pixel));xs=np.flatnonzero(row);at=0
  while at<len(xs):
   begin=int(xs[at]);end=begin+1;at+=1
   while at<len(xs) and xs[at]==end:end+=1;at+=1
   segments.append((begin,end-begin));pixel+=end-begin
 rows.append((len(segments),pixel))
 t.require((pixel,len(segments))==(PIXELS,SPANS),'Neow source support changed')
 return rows,segments


def parameters(raw):
 version=struct.unpack_from('<H',raw,4)[0] if len(raw)>=6 else 0;bounded=version==2;length=LENGTH+(PIXELS+1)//2 if bounded else LENGTH
 t.require(version in (1,2,3) and len(raw)==(length+511)//512*512 and raw[:4]==b'NWF1','Neow parameter extent/magic')
 h=struct.unpack_from('<16H',raw,4);static=h[1]==2
 t.require(h==(version,2 if static else 1,4,FRAMES,KEY_INTERVAL,PIXELS,BASE_IMAGES,TEXTURE_PAGES,FIRST_MASK,PARAMETER,NODES_AT,MODELS_AT,ROWS_AT,SPANS_AT,SPANS,length),'Neow parameter header')
 t.require(struct.unpack_from('<2I',raw,36)==(200000,DURATION) and (struct.unpack_from('<2I',raw,44)==(LENGTH,(PIXELS+1)//2) if bounded else struct.unpack_from('<2I',raw,44)==(BASE_IMAGES,PIXELS*2) if version==3 else not any(raw[44:52])) and not any(raw[52:64]) and not any(raw[length:]),'Neow clocks/padding')
 colors=np.zeros(2,dtype='i8')
 for i in range(4):
  node=struct.unpack_from('<6i2H4B',raw,NODES_AT+i*32);q=node[:6]
  t.require(node[8]<2 and not any(node[9:]) and max(node[6:8])<=255,'Neow node color/model')
  t.require(all(abs(q[k])+320*abs(q[k+2])+240*abs(q[k+4])<=2147483647 for k in (0,1)),'Neow UV arithmetic bound')
  colors+=node[6:8]
 t.require(np.all(colors<=255),'Neow summed contribution arithmetic bound')
 for i in range(2):
  q=struct.unpack_from('<32i',raw,MODELS_AT+i*128)
  t.require(all(abs(x)<=4*16777216 for x in q) and 0<=q[30]<q[31]<=16777216 and q[31]-q[30]>=65536,'Neow model bounds')
 if bounded:
  flags=np.frombuffer(raw[LENGTH:length],'u1');t.require(not (flags[-1]&240) and np.all(flags[:-1]&15) and np.all(flags[:-1]>>4) and (flags[-1]&15),'Neow node-support flags')
 span=pixel=0
 for y in range(240):
  t.require(struct.unpack_from('<2H',raw,ROWS_AT+y*4)==(span,pixel),'Neow support row offset')
  end=struct.unpack_from('<H',raw,ROWS_AT+(y+1)*4)[0];t.require(span<=end<=SPANS,'Neow row span extent');right=0
  for i in range(span,end):
   x,count=struct.unpack_from('<2H',raw,SPANS_AT+i*4);t.require(count and x>=right and x+count<=320,'Neow sorted support spans');right=x+count;pixel+=count
  span=end
 t.require(span==SPANS and pixel==PIXELS and struct.unpack_from('<H',raw,ROWS_AT+240*4+2)[0]==PIXELS,'Neow final support extent')
 return dict(source_static=static,frames=FRAMES,key_interval=KEY_INTERVAL,source_shader_period_ms=200000,body_period_ms=DURATION,integer_bounds=bounded,baked_light=version==3)


def pack_parameters(records,support,static=False):
 rows,segments=spans(support);models=[];nodes=[]
 for record in records:
  u=record['uniforms'];t.require(record['texture']==TEXTURES[1][0] and record['textures']==EXPECTED_TEXTURES and record['tint']==[1,1,1,1] and shader.SHADERS[record['shader']][2]==1,'Neow fire source binding/blend')
  t.require(u['InnerColor'][:3]==[0,0,0] and np.rint(u['OuterColor'][0]*255)==0,'Neow zero inner/red specialization')
  model=fire.model(u)
  if model not in models:models.append(model)
  m=np.vstack([np.asarray(record['matrix']).reshape(2,3),[0,0,1]]);inverse=np.linalg.inv(m);x,y,w,h=record['rect']
  origin=(inverse@np.array([.5,.5,1]))[:2];origin=(origin-[x,y])/[w,h]
  q=lights.round_signed(np.array([*origin,inverse[0,0]/w,inverse[1,0]/h,inverse[0,1]/w,inverse[1,1]/h])*268435456)
  green,blue=np.rint(np.asarray(u['OuterColor'][1:3])*255).astype('i4')
  nodes.append(struct.pack('<6i2H4B',*q,int(green),int(blue),models.index(model),0,0,0))
 t.require(len(nodes)==4 and len(models)==2,'Neow source model/node cardinality')
 raw=bytearray(64);raw[:4]=b'NWF1';struct.pack_into('<16H',raw,4,1,2 if static else 1,4,FRAMES,KEY_INTERVAL,PIXELS,BASE_IMAGES,TEXTURE_PAGES,FIRST_MASK,PARAMETER,NODES_AT,MODELS_AT,ROWS_AT,SPANS_AT,SPANS,LENGTH)
 struct.pack_into('<2I',raw,36,200000,DURATION)
 raw+=b''.join(nodes)+b''.join(models)+b''.join(struct.pack('<2H',*r)for r in rows)+b''.join(struct.pack('<2H',*r)for r in segments)
 raw+=bytes(4608-len(raw));parameters(raw);return bytes(raw)


def hold(source):
 t.require(source.shape==(FRAMES,PIXELS) and source.dtype==np.uint8,'Neow hold source shape')
 previous=source[-1].copy()
 for _ in range(4):
  initial=previous.copy()
  for frame in source:
   change=(abs(frame.astype('i2')-previous.astype('i2'))>8)|(frame==0)|(frame==255)
   previous[change]=np.minimum(255,((frame[change].astype('u2')+4)//8)*8).astype('u1')
  unstable=previous!=initial
 initial=previous.copy();held=[]
 for frame in source:
  change=(abs(frame.astype('i2')-previous.astype('i2'))>8)|(frame==0)|(frame==255)|unstable
  previous[change]=np.minimum(255,((frame[change].astype('u2')+4)//8)*8).astype('u1');previous[unstable]=frame[unstable];held.append(previous.copy())
 out=np.asarray(held);t.require(np.array_equal(previous[~unstable],initial[~unstable]) and np.max(abs(out.astype('i2')-source.astype('i2')))<=8,'Neow cyclic/error closure')
 t.require(np.all(out[source==0]==0) and np.all(out[source==255]==255),'Neow exact opacity endpoints')
 return out,int(unstable.sum())

class TransferRenderer(lights.LightRenderer):
 def visit(self,node,parent_matrix,parent_size,parent_tint,parent_z,path,clips=(),bones=None):
  if node.kind=='Sprite2D' and self.material(node)[0] in effects.FIRE_SHADERS and world._property(node.props,'visible',True):
   return effects.SourceRenderer.visit(self,node,parent_matrix,parent_size,parent_tint,parent_z,path,clips,bones)
  return super().visit(node,parent_matrix,parent_size,parent_tint,parent_z,path,clips,bones)
 def finish(self,plane='all'):
  commands=sorted(self.commands,key=lambda r:r[0]);positions=[i for i,r in enumerate(commands)if isinstance(r[1],effects.Effect)]
  t.require(plane=='all' and self.canvas==(320,240) and positions==[1,2,3,4],'Neow behind-body source order changed')
  foreground=[r for r in commands[5:]if r[1] is not None and not isinstance(r[1],lights.Marker)]
  t.require(all(r[2]in(0,2,3)and not r[3]for r in foreground),'Nonaffine foreground requires a distinct source pass')
  transmission=np.ones((240,320,3),dtype='f8')
  for r in foreground:
   q=np.asarray(r[1],dtype='f8')/255;alpha=q[:,:,3:4]
   transmission*=1-alpha if r[2]==0 else 1-alpha+alpha*q[:,:,:3]if r[2]==2 else 1-alpha*q[:,:,:3]
  t.require(np.all(transmission[:16]==1),'Neow foreground enters source constant rows')
  green=np.rint(transmission[16:,:,1]*255).astype('u1');blue=np.rint(transmission[16:,:,2]*255).astype('u1');where=np.nonzero(blue!=green)
  exceptions=np.column_stack((where[0]*320+where[1],blue[where])).astype('<u4').tobytes()
  bg=commands[0];t.require(bg[2]==0 and not bg[3] and bg[1] is not None,'Neow background must be normal')
  background=np.asarray(bg[1]);t.require(np.all((background[:,:,3]==0)|(background[:,:,3]==255)) and np.all(transmission[background[:,:,3]==0]==1),'Transparent background may not overlap affine foreground')
  self.diagnostics['neow_transfer']=dict(mask=green.tobytes(),blue=exceptions,background=bg[1].tobytes(),
   records=[commands[i][1].record for i in positions],source_order='background /4fires /animated affine foreground /3lights /fog+stars /4lights /vignette /3lights /screen-post')
  self.commands=[r for r in commands if r[1] is not None and not isinstance(r[1],(lights.Marker,effects.Effect))]
  return lights.LightRenderer.__bases__[0].finish(self,plane)


def sample_source(pack,sampler,runtime,light_work,work):
 import animation
 from ancient_scene_ui23 import SCENES,SampledLoop,rgb565,emit,signature
 from build_content import texture
 from world_raster import raster
 scene=SCENES[0];body,body_receipt=lights.sample_scene(pack,sampler,runtime,scene,Path(light_work)/'sampled')
 identity=t.sha256((signature(pack,runtime,scene)+t.sha256(Path(__file__).read_bytes())+body_receipt['signature']).encode())
 directory=Path(work)/('sampled-'+identity[:16]);directory.mkdir(parents=True,exist_ok=True)
 records=[];pending=[]
 for frame in range(FRAMES):
  path=directory/f'{frame:04}.json'
  if not path.exists():pending.append(frame);continue
  row=json.loads(path.read_text(encoding='utf-8'));t.require(row['signature']==identity and row['body_sha256']==body_receipt['source_rgb565_sha256'][frame],'Neow source cache identity')
  for ext,key in (('mask','mask_sha256'),('blue','blue_sha256')):t.require(t.sha256((directory/f'{frame:04}.{ext}').read_bytes())==row[key],'Neow source cache corrupt')
 old_renderer,old_raster=world._Renderer,animation.raster;load=lru_cache(maxsize=32)(texture)
 try:
  t.require(old_renderer is lights.LightRenderer.__bases__[0],'Concurrent Neow source specialization')
  world._Renderer=TransferRenderer;animation.raster=raster;loop=SampledLoop(sampler,scene,[i*STEP_MS/1000 for i in range(FRAMES)]) if pending else None
  for frame in pending:
   picture,details=world.render_scene(pack,scene['path'],320,240,viewport=(1680,1260),sampler=loop,sample_time=frame*STEP_MS/1000,texture_loader=load)
   data=details['neow_transfer'];actual=t.sha256(rgb565(picture).tobytes());t.require(actual==body_receipt['source_rgb565_sha256'][frame],'Neow traversal changed the existing source body')
   for ext in ('mask','blue'):t.atomic_bytes(directory/f'{frame:04}.{ext}',data[ext])
   stable=dict(records=data['records'],background_sha256=t.sha256(data['background']),order=data['source_order'])
   if (directory/'nodes.json').exists():t.require(json.loads((directory/'nodes.json').read_text(encoding='utf-8'))==stable,'Neow fire geometry/background moves unexpectedly')
   else:
    t.atomic_bytes(directory/'nodes.json',(json.dumps(stable,sort_keys=True)+'\n').encode());t.atomic_bytes(directory/'background.rgba',data['background'])
   row=dict(signature=identity,frame=frame,mask_sha256=t.sha256(data['mask']),blue_sha256=t.sha256(data['blue']),body_sha256=actual)
   t.atomic_bytes(directory/f'{frame:04}.json',(json.dumps(row,sort_keys=True)+'\n').encode())
   if frame%20==0:emit('neow_fire_transfer',frame=frame,frames=FRAMES)
 finally:world._Renderer=old_renderer;animation.raster=old_raster;load.cache_clear()
 for frame in range(FRAMES):records.append(json.loads((directory/f'{frame:04}.json').read_text(encoding='utf-8')))
 nodes=json.loads((directory/'nodes.json').read_text(encoding='utf-8'));t.require(t.sha256((directory/'background.rgba').read_bytes())==nodes['background_sha256'],'Neow cached background corrupt')
 return directory,dict(complete=True,signature=identity,source_scene_sha256=t.sha256(pack.read(scene['path'])),body_signature=body_receipt['signature'],frames=records,source_order=nodes['order'])


def contribution_upper(records,textures):
 # Exact rectangle maxima bound every possible bilinear main-texture sample.
 # Source noise/distortion channels are nonnegative; this bound is independent
 # of global shader TIME and the body clock.
 base=np.rint(textures[TEXTURES[1][0]][:,:,2]*255).astype('u1');h,w=base.shape;levels={(0,0):base}
 for y in range(9):
  for x in range(9):
   if not x and not y:continue
   if x:a=levels[y,x-1];d=1<<(x-1);out=np.maximum(a,np.pad(a[:,d:],((0,0),(0,d)),mode='edge'))
   else:a=levels[y-1,0];d=1<<(y-1);out=np.maximum(a,np.pad(a[d:],((0,d),(0,0)),mode='edge'))
   levels[y,x]=out
 def rectmax(lo,hi):
  xy0=np.floor(lo*np.array([w,h])-.5).astype('i4');xy1=np.floor(hi*np.array([w,h])-.5).astype('i4')+1
  xy0=np.clip(xy0,[0,0],[w-1,h-1]);xy1=np.clip(xy1,[0,0],[w-1,h-1]);size=xy1-xy0+1;k=np.floor(np.log2(size)).astype('i4');out=np.zeros(lo.shape[:-1],'u1')
  for ky,kx in np.unique(k.reshape(-1,2)[:,::-1],axis=0):
   mask=(k[:,:,0]==kx)&(k[:,:,1]==ky);x0,y0=xy0[mask].T;x1,y1=xy1[mask].T;x1-=((1<<kx)-1);y1-=((1<<ky)-1);tab=levels[int(ky),int(kx)]
   out[mask]=np.maximum.reduce([tab[y0,x0],tab[y0,x1],tab[y1,x0],tab[y1,x1]])
  return out.astype('f8')/255
 upper=np.zeros((240,320,3),'f8')
 for r in records:
  matrix=np.vstack([np.asarray(r['matrix']).reshape(2,3),[0,0,1]]);uv,covered=shader.quad_uv((320,240),np.eye(3),matrix,r['rect']);u=r['uniforms'];tex={k:textures[p]for k,p in r['textures'].items()}
  mask_uv=(uv-np.array([0,.04],'f4'))*u['NoiseMaskScale']+[0,.04]+np.array(u['NoiseMaskOffset']);mask=1-shader.sample_texture(tex['NoiseMask'],mask_uv)[:,:,2]
  dm=shader.sample_texture(tex['DistortionMask'],uv*u['DistortionMaskScale']+u['DistortionMaskOffset'])[:,:,:2]
  high=(np.max(tex['Distortion1Texture'][:,:,:2],axis=(0,1))*u['Distortion1Strength']+np.max(tex['Distortion2Texture'][:,:,:2],axis=(0,1))*u['Distortion2Strength'])*dm
  main=rectmax(uv,uv+high);noise=float(tex['Noise1Texture'][:,:,2].max())*u['Noise1Strength']*float(tex['Noise2Texture'][:,:,2].max())*u['Noise2Strength']
  alpha=np.ceil(shader.smoothstep(np.asarray(u['OuterStep']),(noise+mask)*main)*covered*255)/255
  upper+=alpha[:,:,None]*(np.rint(np.asarray(u['OuterColor'][:3])*255)/255)
 return upper


def build_assets(pack,directory,static=False):
 from build_content import texture
 directory=Path(directory);nodes=json.loads((directory/'nodes.json').read_text(encoding='utf-8'));records=nodes['records'];textures={};images=[];texture_proof=[]
 for name,w,h,channels,digest in TEXTURES+(fire.TEXTURES[1],):
  rgba=np.asarray(texture(pack,name).convert('RGBA'),dtype='u1');t.require(rgba.shape==(h,w,4) and t.sha256(rgba.tobytes())==digest,'Neow source texture identity')
  textures[name]=rgba.astype('f4')/255
  if name==fire.TEXTURES[1][0]:continue
  raw=rgba[:,:,channels].tobytes()
  for start in range(0,len(raw),131072):
   part=raw[start:start+131072];part+=bytes((-len(part))%640);images.append((320,len(part)//640,0,part))
  texture_proof.append(dict(path=name,rgba_sha256=digest,channels=channels,channel_sha256=t.sha256(raw),source_size=[w,h]))
 t.require(len(images)==TEXTURE_PAGES,'Neow texture page count')
 upper=contribution_upper(records,textures);original=np.full((FRAMES,240,320),255,'u1')
 for frame in range(FRAMES):
  raw=(directory/f'{frame:04}.mask').read_bytes();t.require(len(raw)==224*320,'Neow source mask extent');original[frame,16:]=np.frombuffer(raw,'u1').reshape(224,320)
 support=np.any(upper>0,2)&np.any(original>0,0);spans(support)
 background=np.frombuffer((directory/'background.rgba').read_bytes(),'u1').reshape(240,320,4)[:,:,:3].astype('f8')/255
 saturation=np.any(background+upper>1+1e-9,2);t.require(not np.any(original[:,saturation]),'Neow fire saturation survives foreground')
 source=original[:,support];selected,fallback=(np.repeat(source[:1],FRAMES,axis=0),0) if static else hold(source)
 previous=bytes(PIXELS);maximum=np.zeros(2);mask_hashes=[]
 for frame in range(FRAMES):
  row=selected[frame].tobytes();ref=bytes(PIXELS) if frame%KEY_INTERVAL==0 or static else previous;raw=encode(row,ref,0 if static else frame)
  t.require(decode(raw,ref,0 if static else frame)==row,'Neow patch replay differs');images.append((256,len(raw)//512,0,raw));previous=row;mask_hashes.append(t.sha256(row))
  candidate=np.zeros((240,320),'u1');candidate[support]=selected[frame];index=0 if static else frame;blue=original[index].copy();rawblue=(directory/f'{index:04}.blue').read_bytes()
  t.require(len(rawblue)%8==0,'Neow blue exception width');exceptions=np.frombuffer(rawblue,'<u4').reshape(-1,2)
  if len(exceptions):
   t.require(int(exceptions[:,0].max())<224*320 and int(exceptions[:,1].max())<=255,'Neow blue exception bounds');blue[16:].ravel()[exceptions[:,0]]=exceptions[:,1].astype('u1')
  for channel,exact in enumerate((original[index],blue)):
   error=(abs(candidate.astype('i2')-exact.astype('i2'))+.5)/255*upper[:,:,channel+1];maximum[channel]=max(maximum[channel],float(error.max()))
 t.require(maximum[0]<1/63 and maximum[1]<1/31,'Neow all-clock contribution error exceeds one RGB565 step')
 raw=pack_parameters(records,support,static)
 rgba=[np.rint(textures[r[0]]*255).astype('u1') for r in TEXTURES]
 triangle=fire.half_noise(np.rint(textures[fire.TEXTURES[1][0]]*255).astype('u1'))
 raw=with_integer_bounds(raw,[rgba[0][:,:,(2,)],triangle[:,:,:3],rgba[1][:,:,:3],rgba[2][:,:,(0,1)]])
 images.append((256,len(raw)//512,0,raw))
 return images,dict(source_static=static,source_texture_channels=texture_proof,shared_triangle='reviewed250x250RGB',source_order=nodes['order'],mask_frames=FRAMES,
  mask_key_interval=KEY_INTERVAL,mask_state_bytes=PIXELS,source_support_spans=SPANS,mask_bound=0 if static else 8,cyclic_exact_fallback_pixels=fallback,
  maximum_green_blue_source_transmission_error=maximum.tolist(),visible_saturation_pixels=0,mask_sha256=mask_hashes,
  source_shader_period_ms=200000,source_body_period_ms=DURATION,source_body_pixels_changed=False,persistent_allocation_growth=0)


def require_reduced_source(source,image):
 _,_,_,raw=image(source.aux[BASE_AUX]);t.require(parameters(raw)['source_static'],'Reduced Neow fires require source first-pose transmission')


def validate(source,image,final=False):
 from no_animations import reduced
 t.require((len(source.aux),len(source.images))==(AUXILIARY,IMAGES) and source.aux[BASE_AUX]==PARAMETER,'Neow fire exact catalog endpoint')
 w,h,fmt,raw=image(PARAMETER);t.require(w==256 and h in (9,26) and fmt==0,'Neow parameter picture');info=parameters(raw)
 if final or reduced(source.data):t.require(info['source_static']==reduced(source.data),'Neow transmission profile does not match final marker')
 if info['baked_light']:
  width,height,fmt,raw=image(BASE_IMAGES);used=PIXELS*2
  t.require((width,height,fmt)==(256,(used+511)//512,0) and len(raw)==width*height*2 and not any(raw[used:]),'Neow baked light extent/padding')
  for index in range(1,TEXTURE_PAGES):t.require(image(BASE_IMAGES+index)==(1,1,0,bytes(2)),'Neow retired material page')
 else:
  at=BASE_IMAGES
  for _,w,h,channels,_ in TEXTURES:
   total=w*h*len(channels)
   for offset in range(0,total,131072):
    width,height,fmt,raw=image(at);used=min(131072,total-offset)
    t.require((width,height,fmt)==(320,(used+639)//640,0) and len(raw)==width*height*2 and not any(raw[used:]),'Neow texture page extent/padding');at+=1
 previous=bytes(PIXELS)
 for frame in range(FRAMES):
  w,h,fmt,raw=image(FIRST_MASK+frame);t.require(w==256 and 0<h<=240 and fmt==0 and len(raw)==w*h*2,'Neow mask picture')
  reference=bytes(PIXELS) if frame%KEY_INTERVAL==0 or info['source_static'] else previous
  previous=decode(raw,reference,0 if info['source_static'] else frame)
 return info


def append(data,images,proof,work):
 from coalesce_tile_resources import view
 from patch_frontend_assets import patch_pack
 from split_content import LANGUAGES
 from tanx_scene_ui23 import group_images
 from no_animations import reduced,mark
 source=view(data);t.require(source.version==46 and (len(source.aux),len(source.images),len(source.clips))==(BASE_AUX,BASE_IMAGES,1783),'Neow fires require exact preceding half-noise catalog')
 work=Path(work);work.mkdir(parents=True,exist_ok=True);rows=[]
 for i,(w,h,fmt,raw) in enumerate(images):
  name=f'{i:03}-{t.sha256(raw)}.bin';t.atomic_bytes(work/name,raw);rows.append(dict(width=w,height=h,format=fmt,file=name,bytes=len(raw),sha256=t.sha256(raw),raw_sha256=t.sha256(raw)))
 t.require(len(rows)==IMAGES-BASE_IMAGES,'Neow appended leaf count')
 art=group_images(rows,work,group_bytes=256*1024)
 manifest=dict(images=rows,auxiliary_image_ids=[PARAMETER],localized_text={code:{}for code in LANGUAGES})
 out,detail=patch_pack(data,manifest,work,base_aux=BASE_AUX,auxiliary=AUXILIARY,input_version=46,version=46,texts=(),base_clips=len(source.clips),base_strings=len(source.strings),appended_art=art)
 if reduced(data):out=mark(out)
 target=view(out)
 # Source-static intermediate is validated structurally here; only its final
 # no-animation output may carry that mode through the public release gate.
 for i,row in enumerate(images):t.require(target.image_raw(BASE_IMAGES+i)==row,'Neow page readback differs')
 t.require(target.clips==source.clips and target.cards==source.cards and target.aux[:BASE_AUX]==source.aux and reduced(out)==reduced(data),'Neow append changed prior identities/profile')
 validate(target,target.image_raw)
 return out,dict(proof,complete=True,input_sha256=t.sha256(data),output_sha256=t.sha256(out),added_bytes=len(out)-len(data),grouping=art['report'],append=detail,decoded_bytes=sum(len(r[3])for r in images))


def integer_bounds(parameter,fields):
 """Prove zero output for each static projected pixel at every shader clock.

 All calculations mirror the runtime integer sampler. Bilinear samples lie
 below the maximum of their four bytes, scaled by the exact native /255
 rounding. Positive distortion therefore lies in a closed UV rectangle;
 a range maximum of the original light blue channel bounds every texel the
 shader could reach. Only strength <= the source outer loweredge is skipped.
 """
 Q=1<<20;rows=[struct.unpack_from('<2H',parameter,ROWS_AT+y*4)for y in range(241)];xs=[];ys=[]
 for y in range(240):
  for span in range(rows[y][0],rows[y+1][0]):
   x,count=struct.unpack_from('<2H',parameter,SPANS_AT+span*4);xs.extend(range(x,x+count));ys.extend([y]*count)
 xs,ys=np.asarray(xs,dtype='i8'),np.asarray(ys,dtype='i8')
 def normalized(q):return (q*16843009+2147483648)>>32
 def constant(value):return normalized(np.asarray(value,dtype='i8')*Q)
 def sample(field,u,v,ch):
  h,w,_=field.shape;px=np.clip(u,0,Q)*w-Q//2;py=np.clip(v,0,Q)*h-Q//2;x=px>>20;y=py>>20;fx=px&(Q-1);fy=py&(Q-1)
  x0=np.clip(x,0,w-1);x1=np.clip(x+1,0,w-1);y0=np.clip(y,0,h-1);y1=np.clip(y+1,0,h-1)
  a=field[y0,x0,ch]*(Q-fx)+field[y0,x1,ch]*fx;b=field[y1,x0,ch]*(Q-fx)+field[y1,x1,ch]*fx
  return normalized((a*(Q-fy)+b*fy+Q//2)>>20)
 base=fields[2][:,:,2];levels={(0,0):base}
 for y in range(9):
  for x in range(9):
   if not x and not y:continue
   if x:a=levels[y,x-1];d=1<<(x-1);out=np.maximum(a,np.pad(a[:,d:],((0,0),(0,d)),mode='edge'))
   else:a=levels[y-1,0];d=1<<(y-1);out=np.maximum(a,np.pad(a[d:],((0,d),(0,0)),mode='edge'))
   levels[y,x]=out
 flags=np.zeros(PIXELS,'u1')
 for i in range(4):
  r=struct.unpack_from('<6i2H4B',parameter,NODES_AT+i*32);q=np.asarray(struct.unpack_from('<32i',parameter,MODELS_AT+r[8]*128),dtype='i8')
  t.require(all(q[j]>=0 for j in (4,9,14,19)),'Neow integer bounds require nonnegative source strengths')
  u=(r[0]+xs*r[2]+ys*r[4])>>8;v=(r[1]+xs*r[3]+ys*r[5])>>8;inside=(u>=0)&(v>=0)&(u<Q)&(v<Q)
  mask=Q-sample(fields[2],(u*q[20]>>24)+(q[22]>>4),((v-41943)*q[21]>>24)+41943+(q[23]>>4),2)
  mu=(u*q[24]>>24)+(q[26]>>4);mv=(v*q[25]>>24)+(q[27]>>4);maximum=[]
  for channel in (0,1):
   d=(constant(fields[1][:,:,channel].max())*q[14]>>24)+(constant(fields[2][:,:,channel].max())*q[19]>>24)
   maximum.append(d*sample(fields[3],mu,mv,channel)>>20)
  x0=np.clip((np.clip(u,0,Q)*256-Q//2)>>20,0,255);y0=np.clip((np.clip(v,0,Q)*256-Q//2)>>20,0,255)
  x1=np.clip(((np.clip(u+maximum[0],0,Q)*256-Q//2)>>20)+1,0,255);y1=np.clip(((np.clip(v+maximum[1],0,Q)*256-Q//2)>>20)+1,0,255)
  kx=np.floor(np.log2(x1-x0+1)).astype('i4');ky=np.floor(np.log2(y1-y0+1)).astype('i4');bound=np.zeros(PIXELS,'i8')
  for yk,xk in np.unique(np.column_stack([ky,kx]),axis=0):
   at=(ky==yk)&(kx==xk);tab=levels[int(yk),int(xk)];xx=x1[at]-((1<<xk)-1);yy=y1[at]-((1<<yk)-1)
   bound[at]=np.maximum.reduce([tab[y0[at],x0[at]],tab[y0[at],xx],tab[yy,x0[at]],tab[yy,xx]])
  n1=constant(fields[0][:,:,0].max())*q[4]>>24;n2=constant(fields[0][:,:,0].max())*q[9]>>24
  strength=(((n1*n2>>20)+mask)*constant(bound)>>20)*16
  flags|=((strength>q[30])&inside).astype('u1')<<i
 t.require(np.all(flags),'Neow global support has an impossible empty node bound')
 padded=np.pad(flags,(0,len(flags)%2));return (padded[::2]|padded[1::2]<<4).tobytes()


def with_integer_bounds(raw,fields):
 parameters(raw);flags=integer_bounds(raw,fields);out=bytearray(raw[:LENGTH]+flags);struct.pack_into('<H',out,4,2);struct.pack_into('<H',out,34,LENGTH+len(flags));struct.pack_into('<2I',out,44,LENGTH,len(flags));out+=bytes((-len(out))%512);parameters(out);return bytes(out)

def main():
 import argparse
 from pck import Pack
 from world_enemy_assets import SpineSampler
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('pck','runtime','work','light-work'):p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--input',type=Path);p.add_argument('--output',type=Path);p.add_argument('--sample-only',action='store_true');p.add_argument('--no-animations',action='store_true');args=p.parse_args()
 t.require(args.sample_only or args.input and args.output,'Supply input/output or sample-only')
 for path in (args.pck,args.runtime,args.work,args.light_work,args.input,args.output):
  if path is not None:t.private_path(path,'Private Neow fire input/output')
 args.work.mkdir(parents=True,exist_ok=True);pack=Pack(args.pck)
 try:
  with SpineSampler(pack,runtime=args.runtime,work_dir=args.work/'adapter')as sampler:
   directory,sampling=sample_source(pack,sampler,args.runtime,args.light_work,args.work)
  images,proof=build_assets(pack,directory,args.no_animations);proof['sampling']=sampling
  if args.sample_only:
   for i,row in enumerate(images):t.atomic_bytes(args.work/f'{i:03}.bin',row[3])
   t.atomic_bytes(args.work/'source.json',(json.dumps(proof,indent=2)+'\n').encode())
  else:
   out,proof=append(args.input.read_bytes(),images,proof,args.work/'append');t.atomic_bytes(args.output,out);t.atomic_bytes(args.output.with_suffix('.json'),(json.dumps(proof,indent=2)+'\n').encode())
 finally:pack.close()

if __name__=='__main__':main()
