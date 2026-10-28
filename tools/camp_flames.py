"""Offline samples of the pinned campfire's layered-noise flame material.

Uses original texture inputs and the scene's material values. This is a bounded
CPU evaluation of that material, not a general Godot shader runtime. Lighting,
particles and renderer color-space differences are not reproduced here.
"""
import re
import numpy as np
from PIL import Image
from build_content import texture

def bake(pack, frames=160, width=46, height=58):
    scene=pack.read('scenes/rest_site/overgrowth_rest_site.tscn').decode()
    paths={i:p for p,i in re.findall(r'path="res://([^"]+)" id="([^"]+)"',scene)}
    section=scene.split('[sub_resource type="ShaderMaterial" id="ShaderMaterial_d3ocq"]',1)[1].split('\n[',1)[0]
    params={}
    for name,value in re.findall(r'shader_parameter/(\w+) = (.+)',section):
        if value.startswith('ExtResource'):
            params[name]=np.asarray(texture(pack,paths[re.search(r'"([^"]+)"',value)[1]]).convert('RGBA'),dtype=np.float64)/255
        elif value.startswith(('Vector2','Color')):params[name]=np.array([float(v) for v in value[value.index('(')+1:-1].split(',')])
        elif value in ('true','false'):params[name]=value=='true'
        else:params[name]=float(value)
    base=np.asarray(texture(pack,'images/vfx/fire/fire_base_campfire.png'),dtype=np.float64)/255
    yy,xx=np.mgrid[:height,:width];uv=np.stack(((xx+.5)/width,(yy+.5)/height),axis=-1)
    def sample(im, coord, repeat=True):
        h,w=im.shape[:2];p=coord*np.array([w,h])-.5;xy=np.floor(p).astype(int);t=p-xy
        def pixel(dx,dy):
            x,y=xy[:,:,0]+dx,xy[:,:,1]+dy
            return im[y%h,x%w] if repeat else im[np.clip(y,0,h-1),np.clip(x,0,w-1)]
        tx,ty=t[:,:,0,None],t[:,:,1,None]
        return (pixel(0,0)*(1-tx)+pixel(1,0)*tx)*(1-ty)+(pixel(0,1)*(1-tx)+pixel(1,1)*tx)*ty
    def smooth(edges,v):
        t=np.clip((v-edges[0])/(edges[1]-edges[0]),0,1);return t*t*(3-2*t)
    result=[]
    for i in range(frames):
        # All panning rates in this material repeat after 20 seconds.
        time=i*20/frames
        n1=sample(params['Noise1Texture'],uv*params['Noise1Scaling']+time*params['Noise1Panning'])[:,:,2]*params['Noise1Strength']
        n2=sample(params['Noise2Texture'],uv*params['Noise2Scaling']+time*params['Noise2Panning'])[:,:,2]*params['Noise2Strength']
        mask=sample(params['NoiseMask'],(uv-[0,.04])*params['NoiseMaskScale']+[0,.04]+params['NoiseMaskOffset'],False)[:,:,2]
        if params['InvertNoiseMask']:mask=1-mask
        d1=sample(params['Distortion1Texture'],uv*params['Distortion1Scale']+time*params['Distortion1Panning'])[:,:,:2]*params['Distortion1Strength']
        d2=sample(params['Distortion2Texture'],uv*params['Distortion2Scale']+time*params['Distortion2Panning'])[:,:,:2]*params['Distortion2Strength']
        dm=sample(params['DistortionMask'],uv*params['DistortionMaskScale']+params['DistortionMaskOffset'],False)[:,:,:2]
        v=(n1*n2+mask)*sample(base,uv+(d1+d2)*dm,False)[:,:,2]
        rgb=params['OuterColor'][:3]+smooth(params['InnerColorStep'],v)[:,:,None]*params['InnerColor'][:3]
        alpha=smooth(params['OuterStep'],v)
        result.append(Image.fromarray(np.uint8(np.clip(np.dstack((rgb,alpha)),0,1)*255),'RGBA'))
    return result
