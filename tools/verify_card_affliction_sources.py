"""Compare the production fixed-point shader with the pinned source equations.

This developer check needs private source assets and a separately compiled
tests/card_affliction_source_bridge.cpp shared library. Neither is bundled in
the player installation. Particle tracks have their own source-process proof.
"""
from pathlib import Path
import argparse,ctypes,json,struct
import numpy as np
from pck import Pack
from combat_ui14_assets import CardRenderer,AFFLICTIONS
from world_scene_assets import _translate,load_scene_texture
from world_room_shaders import sample_texture
from card_affliction_fields import decode

class Reference(CardRenderer):
 def shader_layer(self,*args):
  result=super().shader_layer(*args)
  if result is not None and '/afflictions/' in (args[6] or ''):
   self.samples[args[1]]=np.asarray(result[0],'u1');self.names[id(result[0])]=args[1]
  return result
 def visit(self,node,*args,**kwargs):
  if node.kind=='GPUParticles2D':return
  return super().visit(node,*args,**kwargs)

def verify(pck,work,bridge):
 raw=(work/'card-00.bin').read_bytes();pages=struct.unpack_from('<H',raw,8)[0]
 length=struct.unpack_from('<I',raw,32)[0]
 bank=b''.join((work/f'card-{i+1:02}.bin').read_bytes()[:131072] for i in range(pages))[:length]
 library=ctypes.CDLL(str(bridge));ptr=np.ctypeslib.ndpointer(dtype='i4',flags='C_CONTIGUOUS')
 library.affliction_sample.argtypes=[ctypes.c_char_p,ctypes.c_uint,ctypes.c_char_p,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ptr]
 library.affliction_particle_sample.argtypes=[ctypes.c_char_p,ctypes.c_uint,ctypes.c_char_p,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ptr]
 command_at=struct.unpack_from('<I',raw,48)[0];results=[]
 pack=Pack(pck)
 try:
  for family,name in enumerate(AFFLICTIONS):
   first,count=struct.unpack_from('<2H',raw,64+family*4)
   r=Reference(pack,156,218,(324,454),load_scene_texture,None,0,1)
   node=r.loader.instantiate('scenes/cards/overlays/afflictions/'+name+'.tscn')
   for clock in (37,723,2147,4173,17493):
    r.time=clock/1000;r.commands=[];r.samples={};r.names={}
    r.visit(node,_translate(162,227),np.zeros(2),np.ones(4),0,node.name)
    native=[i for i in range(first,first+count) if raw[command_at+i*128]==1]
    if len(native)!=len(r.samples):raise ValueError('Source material command count differs')
    order=[r.names[id(image)] for _,image,_,_ in sorted(r.commands,key=lambda row:row[0]) if id(image) in r.names]
    for index,path in zip(native,order):
     original=r.samples[path]
     out=np.zeros((218,156,4),'i4')
     if not library.affliction_sample(raw,len(raw),bank,len(bank),index,clock,out):raise ValueError('Native material rejected source')
     candidate=np.clip(out.astype('f8')/1048576,0,1)*255
     source=original.astype('f8')
     # The framebuffer observes premultiplied contribution, not arbitrary RGB
     # beneath zero alpha. Report alpha independently as well.
     error=np.abs(np.concatenate((candidate[:,:,:3]*candidate[:,:,3:]/255, candidate[:,:,3:]),axis=-1)-
                     np.concatenate((source[:,:,:3]*source[:,:,3:]/255,source[:,:,3:]),axis=-1))
     results.append(dict(family=name,node=path,clock_ms=clock,mean_error_8bit=float(error.mean()),
                         p999_error_8bit=float(np.quantile(error,.999)),maximum_error_8bit=float(error.max())))
  # Independently transcribe the three original particle fragments, including
  # rotate.gdshaderinc's row-vector order and flipbook.gdshaderinc's cell order.
  field_at,curve_at=struct.unpack_from('<2I',raw,40);seen=set()
  for index in range(struct.unpack_from('<H',raw,16)[0]):
   c=command_at+index*128;kind=raw[c+1]
   if raw[c]!=2 or kind in seen:continue
   seen.add(kind);field=struct.unpack_from('<h',raw,c+8)[0];at,size=struct.unpack_from('<2I',raw,field_at+field*8)
   texture=decode(bank[at:at+size]).astype('f4')/255
   curve=struct.unpack_from('<h',raw,c+16)[0];at,width,channels=struct.unpack_from('<I2H',raw,curve_at+curve*8)
   lut=np.frombuffer(bank,'<i4',width*channels,at).reshape(1,width,channels).astype('f4')/1048576
   y,x=np.mgrid[:128,:128];uv=np.stack(((x+.5)/128,(y+.5)/128),axis=-1).astype('f4')
   for clock,custom in ((37,0),(723,388392),(4173,870318),(17493,1048575)):
    offset=custom/1048576;at=uv.copy()
    if kind==8:
     noise=sample_texture(texture,np.array((offset,offset+clock/10000),'f4'),repeat=True)[1]
     angle=.35*(noise-.5)*2*np.pi*.5
     delta=uv-[.5,.8];at=np.stack((delta[:,:,0]*np.cos(angle)+delta[:,:,1]*np.sin(angle),
                                 -delta[:,:,0]*np.sin(angle)+delta[:,:,1]*np.cos(angle)),axis=-1)+[.5,.8]
    elif kind==10:
     frame=int(np.floor(offset*4*.99))%4;at=(uv+[frame%2,frame//2])/2
    main=sample_texture(texture,at,repeat=kind==8);color=sample_texture(lut,np.repeat(main[:,:,:1],2,axis=-1))[:,:,:3]
    alpha=main[:,:,0 if kind==9 else 2 if kind==8 else 1]
    original=np.concatenate((color,alpha[:,:,None]),axis=-1)
    out=np.zeros((128,128,4),'i4')
    if not library.affliction_particle_sample(raw,len(raw),bank,len(bank),index,clock,custom,out):raise ValueError('Native particle shader rejected source')
    candidate=np.clip(out.astype('f8')/1048576,0,1);original=np.clip(original,0,1)
    error=np.abs(np.concatenate((candidate[:,:,:3]*candidate[:,:,3:],candidate[:,:,3:]),axis=-1)-
                 np.concatenate((original[:,:,:3]*original[:,:,3:],original[:,:,3:]),axis=-1))*255
    results.append(dict(family='particle',node=str(kind),clock_ms=clock,mean_error_8bit=float(error.mean()),
                        p999_error_8bit=float(np.quantile(error,.999)),maximum_error_8bit=float(error.max())))
 finally:pack.close()
 return dict(samples=len(results),materials=9,source_equations='combat_ui14_assets.py source shader transcription',
             maximum_mean_error_8bit=max(r['mean_error_8bit'] for r in results),
             maximum_p999_error_8bit=max(r['p999_error_8bit'] for r in results),results=results)

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('pck','work','bridge','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();r=verify(a.pck,a.work,a.bridge);a.output.write_text(json.dumps(r,indent=2)+'\n')
 print(json.dumps({k:v for k,v in r.items() if k!='results'}))
 if r['maximum_mean_error_8bit']>0.2 or r['maximum_p999_error_8bit']>4:raise ValueError('Card material source error exceeds reviewed LCD bound')
if __name__=='__main__':main()
