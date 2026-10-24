"""Compile private crystal shader textures to bounded integer lookup tables.

No frame sequence or runtime floating point is needed. Both original polar
noise speeds remain independent; cell coverage is supplied by the live UI.
The numeric tables use the existing image cache and RGB565 storage, including
lossless 16-bit words for the polar/noise data. This is a quantized software
shader, not a claim of bit identity to Godot's GPU derivatives.
"""
import math
import re
from pathlib import Path
import numpy as np
from PIL import Image
from content_tiles import require
from patch_feedback_assets import write_image
from patch_ui7_assets import authored_tint
from world_scene_assets import load_scene_texture

COUNT, RADII, ANGLES = 9, 240, 256
MAX_RADIUS = math.sqrt(.5)


def word_image(words):
    words = np.asarray(words, dtype=np.uint16)
    rgba = np.empty((*words.shape, 4), dtype=np.uint8)
    rgba[..., 0] = (words >> 11) << 3
    rgba[..., 1] = ((words >> 5) & 63) << 2
    rgba[..., 2] = (words & 31) << 3
    rgba[..., 3] = 255
    return Image.fromarray(rgba, 'RGBA')


def smooth(a, b, x):
    t = np.clip((x-a)/(b-a), 0., 1.)
    return t*t*(3.-2.*t)


def sample_repeat(texture, u, v):
    data = np.asarray(texture.convert('RGBA'), dtype=np.float64)[..., 0]/255.
    x, y = np.broadcast_arrays(u*texture.width-.5, v*texture.height-.5)
    ix, iy = np.floor(x).astype(np.int32), np.floor(y).astype(np.int32)
    fx, fy = x-ix, y-iy
    result = np.zeros(x.shape)
    for dx, dy, weight in ((0,0,(1-fx)*(1-fy)),(1,0,fx*(1-fy)),
                           (0,1,(1-fx)*fy),(1,1,fx*fy)):
        result += data[(iy+dy)%texture.height, (ix+dx)%texture.width]*weight
    return result


def shader_tables(scene, material):
    match = re.search(r'\[sub_resource[^\n]*id="'+material+r'"\](.*?)(?=\n\[)', scene, re.S)
    require(match is not None, 'Crystal material changed: '+material)
    section = match.group(1)
    def array(name, kind, count):
        m = re.search(r'shader_parameter/'+name+r' = '+kind+r'\(([^\n]*)\)', section)
        require(m is not None, 'Missing original crystal parameter '+name)
        values = np.array([float(v) for v in m.group(1).split(',')])
        return values.reshape((-1, count))
    colors = array('colors', 'PackedColorArray', 4)[:, :3]
    circles = array('circleData', 'PackedVector4Array', 4)
    outer = int(re.search(r'shader_parameter/outerCircle = (-?\d+)', section).group(1))
    radius = np.linspace(0, MAX_RADIUS, RADII)[:, None]
    noise = np.linspace(0, 2.8, 256)[None, :]
    output = []
    for inside in (True, False) if outer >= 0 else (True,):
        color = np.broadcast_to(colors[0], (RADII, 256, 3)).copy()
        last = np.ones((RADII,256))
        for index,(offset,size,power,edge) in enumerate(circles):
            circle = smooth(offset*.5,(offset+size)*.5,radius)
            raw = (1+(noise-1)*circle**power)*(1-circle)
            # Bound antialiasing to one output pixel's radial footprint.
            derivative = np.abs(np.gradient(raw, axis=0))*RADII/(MAX_RADIUS*158)
            width = np.maximum(derivative, .007)
            mask = np.clip((raw-edge)/width+.5, 0., 1.)
            if index == outer and not inside:
                mask = np.zeros_like(mask)
            next_color = colors[min(index+1,len(colors)-1)]
            color = next_color+(color-next_color)*mask[...,None]
            last = mask
        alpha = last if outer >= 0 else np.ones_like(last)
        output.append(Image.fromarray(np.uint8(np.rint(np.clip(np.dstack((color,alpha)),0,1)*255)), 'RGBA'))
    return output


def build_crystal_images(pack, cache, base_aux):
    cache = Path(cache)
    scene = pack.read('scenes/events/custom/crystal_sphere/crystal_sphere_screen.tscn').decode()
    for expected in ('scale = Vector2(1.04, 1.04)', 'shader_parameter/uvMargin = 0.145',
                     'anchor_left = 0.424', 'offset_left = -419.0'):
        require(expected in scene, 'Changed crystal screen: '+expected)
    rows=[]
    def add(key, image, source):
        image.save(cache/(key+'.png'))
        row=write_image(cache,key,image)
        row.update(key=key,source=source,auxiliary=base_aux+len(rows))
        rows.append(row)
    source='images/events/crystal_sphere/crystal_sphere_minigame_bg.png'
    original=load_scene_texture(pack,source).convert('RGBA')
    # TextureRect KEEP_ASPECT_CENTERED, transformed around (1280,600).
    scale=min(2560/original.width,1200/original.height)*1.04*4/21
    bg=original.resize((round(original.width*scale),round(original.height*scale)),Image.Resampling.LANCZOS)
    canvas=Image.new('RGBA',(320,240),(0,0,0,255))
    canvas.alpha_composite(bg,((320-bg.width)//2,round((630+12)*4/21-bg.height/2)))
    add('crystal-background',canvas,source)
    y,x=np.mgrid[:160,:160];dx=(x+.5)/160-.5;dy=(y+.5)/160-.5
    radius=np.minimum(RADII-1,np.rint(np.hypot(dx,dy)*(RADII-1)/MAX_RADIUS)).astype(np.uint16)
    angle=(np.rint((np.arctan2(dy,dx)+math.pi)*ANGLES/(2*math.pi)).astype(np.uint16)&255)
    add('crystal-polar',word_image((radius<<8)|angle),'derived polar coordinates')
    radius=np.linspace(0,MAX_RADIUS,RADII)[:,None]
    angle=np.arange(ANGLES)[None,:]/ANGLES
    for index,(source,multiplier) in enumerate((('images/vfx/crystal_sphere_noise.png',2.),('images/vfx/scry_voronoi_noise.png',.5))):
        values=sample_repeat(load_scene_texture(pack,source),radius*multiplier,angle)
        values=values**.8 if index==0 else (values+.4)/1.4
        add('crystal-noise-'+str(index+1),word_image(np.rint(values*65535)),source)
    outer=shader_tables(scene,'ShaderMaterial_r1hel')
    inner=shader_tables(scene,'ShaderMaterial_r3co4')[0]
    for key,image in (('crystal-outer-in',outer[0]),('crystal-outer-out',outer[1]),('crystal-inner',inner)):
        add(key,image,'derived quantized scry_reveal material')
    source='images/vfx/crystal_sphere_outline.png'
    add('crystal-outline',load_scene_texture(pack,source).convert('RGBA').resize((134,134),Image.Resampling.LANCZOS),source)
    source='images/packed/vfx/speech_bubble3.png'
    bubble=authored_tint(load_scene_texture(pack,source).convert('RGBA'),(.25,.75,1.))
    bubble=bubble.rotate(180).resize((round(bubble.width*.75*4/21),round(bubble.height*.75*4/21)),Image.Resampling.LANCZOS)
    add('crystal-dialogue',bubble,source)
    require(len(rows)==COUNT,'Crystal lookup/image count changed')
    return rows
