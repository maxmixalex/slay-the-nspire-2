"""Original Random gradient, seamless Godot noise and decorative textures.

The public converter contains equations and source identifiers only. The
user supplies the original PCK and Godot 4.5.1's MIT FastNoiseLite.h; source
images and compiled helpers remain in the private conversion cache.
"""
from __future__ import annotations

import ctypes
import hashlib
import os
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image

import content_tiles as tiles
from build_content import texture
from patch_feedback_assets import write_image
from pck import Pack
from random_character_ui12 import RandomRenderer
from world_scene_assets import _property, load_scene_texture

BASE_AUX, AUXILIARY = 3339, 3343
SCENE = 'scenes/screens/char_select/char_select_bg_random_character.tscn'
SHADER = 'shaders/vfx/vfx_water_reflection.gdshader'

_RANDOM_NOISE_CALLER = r'''#include "FastNoiseLite.h"
#if defined(_WIN32)
#define NSPIRE_RANDOM_EXPORT __declspec(dllexport)
#else
#define NSPIRE_RANDOM_EXPORT
#endif
extern "C" NSPIRE_RANDOM_EXPORT void nspire_noise(float*out,int w,int h,float frequency){
 fastnoiselite::FastNoiseLite noise(0);
 noise.SetNoiseType(fastnoiselite::FastNoiseLite::NoiseType_OpenSimplex2);
 noise.SetFrequency(frequency);
 noise.SetFractalType(fastnoiselite::FastNoiseLite::FractalType_FBm);
 noise.SetFractalOctaves(5);noise.SetFractalLacunarity(2.0f);
 noise.SetFractalGain(0.5f);noise.SetFractalWeightedStrength(0.0f);
 for(int y=0;y<h;++y)for(int x=0;x<w;++x)out[y*w+x]=noise.GetNoise(float(x),float(y));
}
'''


def _noise_values(width, height, frequency, header, cache):
    header = Path(header).resolve()
    tiles.require(header.is_file(), 'Supply Godot 4.5.1 FastNoiseLite.h with NSPIRE_FASTNOISE_HEADER')
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    bundled=os.environ.get('NSPIRE_FASTNOISE_LIBRARY')
    if bundled:
        library=Path(bundled).resolve()
        tiles.require(library.is_file(),'The bundled noise sampling library is missing')
    else:
        identity = hashlib.sha256(header.read_bytes()+_RANDOM_NOISE_CALLER.encode()).hexdigest()
        cpp, library = cache/('noise-'+identity+'.cpp'), cache/('noise-'+identity+'.so')
        if not library.is_file():
            cpp.write_text(_RANDOM_NOISE_CALLER)
            subprocess.run([os.environ.get('CXX', 'clang++-18'), '-O2', '-ffp-contract=off',
                            '-shared', '-fPIC', '-I'+str(header.parent), str(cpp), '-o', str(library)], check=True)
    helper = ctypes.CDLL(str(library))
    helper.nspire_noise.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_int, ctypes.c_int, ctypes.c_float]
    helper.nspire_noise.restype = None
    values = np.empty((height, width), dtype=np.float32)
    helper.nspire_noise(values.ctypes.data_as(ctypes.POINTER(ctypes.c_float)), width, height, frequency)
    minimum, maximum = values.min(), values.max()
    return np.zeros_like(values, dtype=np.uint8) if minimum == maximum else np.uint8((values-minimum)/(maximum-minimum)*np.float32(255))


def godot_noise_image(width, height, frequency, skirt, header, cache):
    """Godot Noise::get_seamless_image R8, preserved losslessly in alpha.

    Implements the source quadrant shuffle and overlapping skirt blends,
    including Godot's integer (alpha+1, 256-alpha) grayscale blend.
    """
    sw, sh = max(1, int(width*skirt)), max(1, int(height*skirt))
    source = _noise_values(width+sw, height+sh, frequency, header, cache)
    hw, hh = width//2, height//2
    def read(x, y, alt_x=False, alt_y=False):
        return int(source[(y+hh) % (height if alt_y else height+sh),
                          (x+hw) % (width if alt_x else width+sw)])
    def alpha(i, size):
        value = np.clip(np.float32((np.float32(i)/np.float32(size)-np.float32(.1))/np.float32(.8)), 0, 1)
        return int(np.float32(255)*(np.float32(1)-value*value*(np.float32(3)-np.float32(2)*value)))
    def blend(background, foreground, a):
        return ((a+1)*foreground+(256-a)*background)>>8
    result = np.asarray([[read(x, y, True, True) for x in range(width)] for y in range(height)], dtype=np.uint8)
    for x in range(hw, hw+sw):
        a = alpha(x-hw, sw)
        for y in range(height):
            if not hh <= y < hh+sh: result[y, x] = blend(int(result[y, x]), read(x, y, False, True), a)
    for y in range(hh, hh+sh):
        a = alpha(y-hh, sh)
        for x in range(width):
            if not hw <= x < hw+sw: result[y, x] = blend(int(result[y, x]), read(x, y, True, False), a)
    for y in range(hh, hh+sh):
        for x in range(hw, hw+sw):
            a, b = alpha(x-hw, sw), alpha(y-hh, sh)
            top = blend(read(x, y, True, False), read(x, y), a)
            bottom = blend(read(x, y, True, True), read(x, y, False, True), a)
            result[y, x] = blend(bottom, top, b)
    rgba = np.full((height, width, 4), 255, dtype=np.uint8)
    rgba[:, :, 3] = result
    return Image.fromarray(rgba, 'RGBA')


def linear_texture_image(image):
    """Carry all 256x256 source texels inside native 320x240 image bounds.

    Texture's logical 256x256 view addresses the original flat texel order.
    No resampling or cropping occurs; the final physical row has64 padding
    texels, outside the logical view. Dense/indexed sampling needs no divide.
    """
    tiles.require(image.mode == 'RGBA' and image.size == (256, 256),
                  'Original decorative texture dimensions changed')
    return Image.frombytes('RGBA', (320, 205), image.tobytes()+bytes(64*4))


def source_assets(pck, cache, fastnoise_header=None):
    """Four fixed-auxiliary rows; the caller owns append and shared packing."""
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    owned = not hasattr(pck, 'read')
    pack = Pack(pck) if owned else pck
    header = fastnoise_header or os.environ.get('NSPIRE_FASTNOISE_HEADER', '')
    try:
        scene, shader = pack.read(SCENE).decode(), pack.read(SHADER).decode()
        for value in ('noise_type = 0', 'frequency = 0.0259', 'width = 10', 'height = 5',
                      'seamless_blend_skirt = 0.093', 'shader_parameter/speed = 0.023',
                      'shader_parameter/strength = 0.1', 'amount = 20', 'lifetime = 15.0',
                      'preprocess = 15.0', 'orbit_velocity_max = 0.03', 'radial_accel_max = 1.0'):
            tiles.require(value in scene, 'Random source contract changed: '+value)
        tiles.require('UV - (strength * 0.5)' in shader and 'TIME*speed' in shader,
                      'Random water shader expression changed')
        mask = load_scene_texture(pack, 'images/rooms/underdocks/underdocks_00.png').convert('RGBA')
        tiles.require(mask.getchannel('A').getextrema() == (255, 255), 'Random mask is no longer fully opaque')
        renderer = RandomRenderer(pack, 320, 240, (1680, 1260), texture, None, 0, 1)
        root = renderer.loader.instantiate(SCENE)
        node = next(child for child in root.children if child.name == 'TextureRect')
        gradient = renderer.texture(_property(node.props, 'texture')).convert('RGBA')
        tiles.require(gradient.size == (64, 64), 'Random full gradient size changed')
        noise = godot_noise_image(10, 5, .0259, .093, header, cache/'noise-helper')
        specs = [('random-gradient', gradient, SCENE), ('random-noise', noise, SCENE),
                 ('random-light', load_scene_texture(pack, 'images/vfx/light.png').convert('RGBA'), 'images/vfx/light.png'),
                 ('random-dot', load_scene_texture(pack, 'images/vfx/dot.png').convert('RGBA'), 'images/vfx/dot.png')]
        rows = []
        for i, (key, image, source) in enumerate(specs):
            source_size = list(image.size)
            if i == 2:
                tiles.require(image.getchannel('A').getextrema() == (255, 255),
                              'Random light texture is no longer opaque; update color-only sampler')
            if i == 3:
                tiles.require(all(channel.getextrema() == (255, 255) for channel in image.split()[:3]),
                              'Random dot texture is no longer RGB white; update alpha-only sampler')
            if i >= 2:
                image = linear_texture_image(image)
            row = write_image(cache, key, image)
            image.save(cache/(key+'.png'))
            row.update(auxiliary=BASE_AUX+i, source=source, source_size=source_size,
                       channel='original R8 noise in alpha; RGB white' if i == 1 else 'original RGBA')
            if i >= 2:
                row.update(logical_size=source_size, storage_layout='lossless linear texels;64 transparent padding texels')
            if i == 1:
                row.update(fastnoise_header_sha256=hashlib.sha256(Path(header).read_bytes()).hexdigest(),
                           generator='Godot 4.5.1 FastNoiseLite FBm and Noise::get_seamless_image',
                           source_alpha_sha256=hashlib.sha256(image.getchannel('A').tobytes()).hexdigest())
            rows.append(row)
        return rows
    finally:
        if owned: pack.close()
