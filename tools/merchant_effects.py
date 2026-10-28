"""Offline evaluation of the pinned merchant's visible flat-fire material.

Read the owner's scene, material uniforms and textures. Output is ordinary
RGB565/alpha sprite data; the device does not run shaders or particle solvers.
The sampled six-second window is deliberately finite. GPU sparks, additive
light particles and the hidden additive-fire sprite are not synthesized here.
"""
from __future__ import annotations

import math
import re

import numpy as np
from PIL import Image

from animation import pose
from build_content import texture
from content_tiles import require, sha256
from patch_feedback_assets import ROOM_SCALE, ROOM_X

SUPERSAMPLE = 4
FLAME_FRAMES = 150
FLAME_FRAME_MS = 40


def scene_sections(text):
    """Small strict reader for the numeric/texture fields used by this scene."""
    result = []
    current = None
    for line in text.splitlines():
        if line.startswith('[') and line.endswith(']'):
            tag, _, attrs = line[1:-1].partition(' ')
            current = {'tag': tag, 'attrs': dict(re.findall(r'(\w+)="([^"]*)"', attrs)), 'values': {}}
            result.append(current)
        elif current is not None and ' = ' in line:
            key, value = line.split(' = ', 1)
            current['values'][key] = value
    return result


def numeric(value):
    if value.startswith(('Vector2(', 'Color(')):
        return np.array([float(v) for v in value[value.index('(')+1:-1].split(',')])
    if value in ('true', 'false'):
        return value == 'true'
    return float(value)


def translation(x, y):
    return np.array([[1., 0., x], [0., 1., y], [0., 0., 1.]])


def scaling(x, y):
    return np.diag([x, y, 1.])


def local_transform(node):
    values = node['values']
    x, y = numeric(values.get('position', 'Vector2(0, 0)'))
    sx, sy = numeric(values.get('scale', 'Vector2(1, 1)'))
    angle = numeric(values.get('rotation', '0'))
    co, si = math.cos(angle), math.sin(angle)
    return np.array([[co*sx, -si*sy, x], [si*sx, co*sy, y], [0., 0., 1.]])


def sample_texture(image, uv, repeat):
    """Linear sampling at texel centers, with the authored wrap mode."""
    h, w = image.shape[:2]
    xy = uv * np.array([w, h]) - .5
    lo = np.floor(xy).astype(np.int64)
    fraction = xy - lo
    hi = lo + 1
    if repeat:
        lo %= [w, h]
        hi %= [w, h]
    else:
        lo = np.clip(lo, [0, 0], [w-1, h-1])
        hi = np.clip(hi, [0, 0], [w-1, h-1])
    fx, fy = fraction[:, 0:1], fraction[:, 1:2]
    low = image[lo[:, 1], lo[:, 0]]*(1-fx) + image[lo[:, 1], hi[:, 0]]*fx
    high = image[hi[:, 1], lo[:, 0]]*(1-fx) + image[hi[:, 1], hi[:, 0]]*fx
    return low*(1-fy) + high*fy


def smoothstep(edges, value):
    require(edges[1] > edges[0], 'Invalid pinned fire smoothstep interval')
    v = np.clip((value-edges[0])/(edges[1]-edges[0]), 0., 1.)
    return v*v*(3.-2.*v)


def shade(uv, seconds, uniforms, images):
    """Evaluate the referenced flat material's distortion/noise/color graph."""
    def read(name, coords, repeat):
        return sample_texture(images[name], coords, repeat)
    def moving(prefix):
        return uv*uniforms[prefix+'Scaling'] + seconds*uniforms[prefix+'Panning']
    noise = (read('Noise1Texture', moving('Noise1'), True)[:, 2]*uniforms['Noise1Strength'] *
             read('Noise2Texture', moving('Noise2'), True)[:, 2]*uniforms['Noise2Strength'])
    pivot = np.array([0., .04])
    mask_uv = (uv-pivot)*uniforms['NoiseMaskScale'] + pivot + uniforms['NoiseMaskOffset']
    mask = read('NoiseMask', mask_uv, False)[:, 2]
    noise += 1.-mask if uniforms['InvertNoiseMask'] else mask
    distortion = np.zeros_like(uv)
    for prefix in ('Distortion1', 'Distortion2'):
        coords = uv*uniforms[prefix+'Scale'] + seconds*uniforms[prefix+'Panning']
        distortion += read(prefix+'Texture', coords, True)[:, :2]*uniforms[prefix+'Strength']
    mask = read('DistortionMask', uv*uniforms['DistortionMaskScale']+uniforms['DistortionMaskOffset'], False)
    value = noise*read('TEXTURE', uv+distortion*mask[:, :2], False)[:, 2]
    rgb = uniforms['OuterColor'][:3] + smoothstep(uniforms['InnerColorStep'], value)[:, None]*uniforms['InnerColor'][:3]
    return np.column_stack((np.clip(rgb, 0., 1.), smoothstep(uniforms['OuterStep'], value)))


class MerchantFlames:
    def __init__(self, pack, bottom_skeleton):
        self.skeleton = bottom_skeleton
        source = pack.read('scenes/rooms/merchant_room.tscn')
        sections = scene_sections(source.decode())
        resources = {s['attrs']['id']: s['attrs']['path'].removeprefix('res://')
                     for s in sections if s['tag'] == 'ext_resource'}
        self.nodes = {}
        for section in sections:
            if section['tag'] == 'node':
                attrs = section['attrs']
                parent = attrs.get('parent', '')
                path = attrs['name'] if not parent else attrs['name'] if parent == '.' else parent+'/'+attrs['name']
                self.nodes[path] = section
        fires = ('SceneContainer/BgContainer/SpineSprite/SpineBoneNode/fire4',
                 'SceneContainer/BgContainer/fire', 'SceneContainer/BgContainer/fire2',
                 'SceneContainer/BgContainer/fire3')
        self.fires = [self.nodes[path] for path in fires]
        self.sprites = [self.nodes[path+'/SteppedFireMix'] for path in fires]
        require(all(self.nodes[path+'/SteppedFireAdd']['values'].get('visible') == 'false' for path in fires),
                'The pinned additive fire sprite must remain hidden')
        materials = {s['attrs']['id']: s for s in sections if s['tag'] == 'sub_resource' and s['attrs'].get('type') == 'ShaderMaterial'}
        material_ids = [re.fullmatch(r'SubResource\("([^"]+)"\)', s['values']['material']).group(1) for s in self.sprites]
        require(len(set(material_ids)) == 1, 'Merchant flames do not share the expected material')
        material = materials[material_ids[0]]['values']
        shader_id = re.fullmatch(r'ExtResource\("([^"]+)"\)', material['shader']).group(1)
        shader_path = resources[shader_id]
        require(shader_path == 'shaders/vfx/vfx_stepped_shader_fire_flat.tres', 'Unexpected merchant fire shader')
        shader = pack.read(shader_path)
        require(b'render_mode blend_mix;' in shader, 'Expected source-over flat fire')
        self.uniforms, self.images = {}, {}
        self.texture_sources = {}
        for key, value in material.items():
            if not key.startswith('shader_parameter/'):
                continue
            name = key.split('/', 1)[1]
            resource = re.fullmatch(r'ExtResource\("([^"]+)"\)', value)
            if resource:
                path = resources[resource.group(1)]
                self.images[name] = np.asarray(texture(pack, path), dtype=float)/255.
                self.texture_sources[name] = path
            else:
                self.uniforms[name] = numeric(value)
        texture_ids = [re.fullmatch(r'ExtResource\("([^"]+)"\)', s['values']['texture']).group(1) for s in self.sprites]
        require(len(set(texture_ids)) == 1, 'Merchant flat fire uses unexpected base textures')
        path = resources[texture_ids[0]]
        self.images['TEXTURE'] = np.asarray(texture(pack, path), dtype=float)/255.
        self.texture_sources['TEXTURE'] = path
        self.camera = translation(ROOM_X, 0) @ scaling(ROOM_SCALE, ROOM_SCALE)
        bg = self.nodes['SceneContainer/BgContainer']['values']
        x, y = 960+numeric(bg['offset_left']), 540+numeric(bg['offset_top'])
        sx, sy = numeric(bg['scale'])
        self.background = translation(x, y) @ scaling(sx, sy)
        self.bottom = local_transform(self.nodes['SceneContainer/BgContainer/SpineSprite'])
        bone_node = self.nodes['SceneContainer/BgContainer/SpineSprite/SpineBoneNode']['values']
        require(bone_node['show_behind_parent'] == 'true', 'Changed candle draw order')
        bone_name = bone_node['bone_name'].strip('"')
        self.bone = next(i for i, b in enumerate(bottom_skeleton.bones) if b['name'] == bone_name)
        self.evidence = {'scene_sha256': sha256(source), 'shader': shader_path, 'shader_sha256': sha256(shader),
                         'texture_sources': self.texture_sources, 'bone': bone_name,
                         'frames': FLAME_FRAMES, 'frame_ms': FLAME_FRAME_MS,
                         'sample_window_ms': FLAME_FRAMES*FLAME_FRAME_MS, 'supersample': SUPERSAMPLE,
                         'loop_boundary': 'Samples TIME in [0, 6) seconds, then repeats. Bone pose repeats at 6 seconds; shader noise TIME is not intrinsically 6-second-periodic.',
                         'omitted': ['GPU cinder particles', 'additive light particles', 'Necrobinder flame shaders'],
                         'intentionally_hidden': ['SteppedFireAdd']}

    def transforms(self, seconds):
        world, *_ = pose(self.skeleton, 'idle_loop', seconds)
        invert_y = scaling(1., -1.)
        bone = invert_y @ world[self.bone] @ invert_y
        return [self.camera @ self.background @ (self.bottom @ bone if i == 0 else np.eye(3)) @
                local_transform(fire) @ local_transform(sprite)
                for i, (fire, sprite) in enumerate(zip(self.fires, self.sprites))]

    def bounds(self, transforms):
        height, width = self.images['TEXTURE'].shape[:2]
        corners = np.array([[-width/2, -height/2, 1], [width/2, -height/2, 1],
                            [width/2, height/2, 1], [-width/2, height/2, 1]])
        points = np.concatenate([(matrix @ corners.T).T[:, :2] for matrix in transforms])
        lo = np.maximum([0, 0], np.floor(points.min(axis=0)).astype(int)-1)
        hi = np.minimum([320, 218], np.ceil(points.max(axis=0)).astype(int)+1)
        require(all(hi > lo), 'Flames outside the pinned room camera')
        return tuple(int(v) for v in (*lo, *hi))

    def render(self, transforms, seconds, crop):
        x0, y0, x1, y1 = crop
        width, height = x1-x0, y1-y0
        ss = SUPERSAMPLE
        out = np.zeros((height*ss, width*ss, 4))  # premultiplied source-over
        yy, xx = np.mgrid[0:height*ss, 0:width*ss]
        positions = np.column_stack(((xx.ravel()+.5)/ss+x0, (yy.ravel()+.5)/ss+y0, np.ones(xx.size)))
        th, tw = self.images['TEXTURE'].shape[:2]
        for matrix in transforms:
            uv = (np.linalg.inv(matrix) @ positions.T).T[:, :2]/[tw, th]+.5
            inside = (uv >= 0).all(axis=1) & (uv <= 1).all(axis=1)
            layer = np.zeros((xx.size, 4))
            layer[inside] = shade(uv[inside], seconds, self.uniforms, self.images)
            layer = layer.reshape(out.shape)
            alpha = layer[:, :, 3:4]
            out[:, :, :3] = layer[:, :, :3]*alpha + out[:, :, :3]*(1-alpha)
            out[:, :, 3:4] = alpha + out[:, :, 3:4]*(1-alpha)
        out = out.reshape(height, ss, width, ss, 4).mean(axis=(1, 3))
        np.divide(out[:, :, :3], out[:, :, 3:4], out=out[:, :, :3], where=out[:, :, 3:4] > 0)
        return Image.fromarray(np.uint8(np.clip(out*255, 0, 255)))
