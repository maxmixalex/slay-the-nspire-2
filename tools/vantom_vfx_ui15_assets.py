"""Original Vantom particle textures; no generated or resampled replacement art."""
import ctypes
import hashlib
import os
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image

from content_tiles import private_path
from patch_feedback_assets import write_image
from pck import Pack
from world_scene_assets import load_scene_texture

BASE_AUXILIARY, AUXILIARY_COUNT = 5404, 5409

_VANTOM_NOISE_CALLER = r'''#include "FastNoiseLite.h"
#if defined(_WIN32)
#define NSPIRE_VANTOM_EXPORT __declspec(dllexport)
#else
#define NSPIRE_VANTOM_EXPORT
#endif
extern "C" NSPIRE_VANTOM_EXPORT void vantom_noise(float*out){
 fastnoiselite::FastNoiseLite noise(0);
 noise.SetNoiseType(fastnoiselite::FastNoiseLite::NoiseType_ValueCubic);
 noise.SetFrequency(0.063f);
 noise.SetFractalType(fastnoiselite::FastNoiseLite::FractalType_FBm);
 noise.SetFractalOctaves(5);noise.SetFractalLacunarity(2.0f);
 noise.SetFractalGain(0.5f);noise.SetFractalWeightedStrength(0.0f);
 for(int y=0;y<512;++y)for(int x=0;x<512;++x)out[y*512+x]=noise.GetNoise(float(x),float(y));
}
'''


def tail_noise(pck, header, cache):
    """Original non-seamless NoiseTexture2D, prior to slot-UV sampling.

    Godot4.5.1 FastNoiseLite ValueCubic, seed0, frequency.063, defaultFBm
    five octaves. Noise::get_image normalizes the complete512-square field
    intoR8. There is no panning, shader-time term, or seamless skirt.
    """
    cache = private_path(cache, 'Private Vantom noise cache')
    cache.mkdir(parents=True, exist_ok=True)
    header = Path(header).resolve()
    expected = '38b24b9b04aa5e9f1e63336d4acbbbb73b11f15b697bb08a25f5ec0c6c274901'
    if not header.is_file() or hashlib.sha256(header.read_bytes()).hexdigest() != expected:
        raise ValueError('Supply the pinned Godot4.5.1 FastNoiseLite header')
    pack = Pack(pck)
    try:
        material = pack.read('materials/vfx/monsters/vantom_tail_dissolve_mat.tres').decode()
        shader = pack.read('shaders/vfx/vfx_noisy_dissolve_shader.tres').decode()
        for token in ('noise_type = 4', 'frequency = 0.063', 'shader_parameter/step = -0.1'):
            if token not in material:
                raise ValueError('Pinned Vantom dissolve material changed: '+token)
        # The PCK retains a VisualShader's generated variable names. These
        # source links encode texture-alpha*smoothstep(step,step+.1,noise.r).
        for token in ('uniform float step_spread = 0.10000000149012',
                      'float n_out19p0 = n_out8p0 + n_out21p0',
                      'smoothstep(n_out8p0, n_out19p0, n_out5p1)',
                      'float n_out12p0 = n_out7p0 * n_out11p4',
                      'uniform vec2 NoisePanning = vec2(0.000000, 0.000000)'):
            if token not in shader:
                raise ValueError('Pinned Vantom dissolve equation changed: '+token)
    finally:
        pack.close()
    source = _VANTOM_NOISE_CALLER
    identity = hashlib.sha256(header.read_bytes()+source.encode()).hexdigest()
    cpp, library = cache/('noise-'+identity+'.cpp'), cache/('noise-'+identity+'.so')
    bundled = os.environ.get('NSPIRE_FASTNOISE_LIBRARY')
    if bundled:
        library = Path(bundled).resolve()
        if not library.is_file():
            raise ValueError('The bundled Vantom noise library is missing: '+str(library))
    elif not library.is_file():
        cpp.write_text(source)
        subprocess.run([os.environ.get('CXX', 'clang++-18'), '-O2', '-ffp-contract=off',
                        '-shared', '-fPIC', '-I'+str(header.parent), str(cpp), '-o', str(library)], check=True)
    helper = ctypes.CDLL(str(library))
    helper.vantom_noise.argtypes = [ctypes.POINTER(ctypes.c_float)]
    helper.vantom_noise.restype = None
    values = np.empty((512, 512), dtype=np.float32)
    helper.vantom_noise(values.ctypes.data_as(ctypes.POINTER(ctypes.c_float)))
    low, high = values.min(), values.max()
    result = np.zeros_like(values, dtype=np.uint8) if low == high else np.uint8((values-low)/(high-low)*np.float32(255))
    Image.fromarray(result, 'L').save(cache/'vantom-tail-noise.png')
    np.save(cache/'vantom-tail-noise.npy', result)
    return result


def source_assets(pck, cache):
    cache = private_path(cache, 'Private Vantom effect texture cache')
    cache.mkdir(parents=True, exist_ok=True)
    rows = []
    pack = Pack(pck)
    try:
        source = 'images/vfx/monsters/vantom/vantom_basic_particle.png'
        basic = load_scene_texture(pack, source).convert('RGBA')
        if basic.size != (9, 20):
            raise ValueError('Pinned Vantom spray texture dimensions changed')
        image = write_image(cache, 'vantom-spray', basic)
        image.update(source=source, auxiliary=BASE_AUXILIARY)
        basic.save(cache/'vantom-spray.png')
        rows.append(image)
        source = 'images/vfx/monsters/vantom/vantom_death_particle.png'
        sheet = load_scene_texture(pack, source).convert('RGBA')
        if sheet.size != (96, 96):
            raise ValueError('Pinned Vantom2x2 death texture dimensions changed')
        for frame in range(4):
            x, y = frame % 2, frame // 2
            cell = sheet.crop((x*48, y*48, (x+1)*48, (y+1)*48))
            key = 'vantom-explosion-'+str(frame)
            image = write_image(cache, key, cell)
            image.update(source=source, source_scene='scenes/creature_visuals/vantom.tscn',
                         auxiliary=BASE_AUXILIARY+1+frame, frame=frame,
                         flipbook=[2, 2], random_initial_frame=True)
            cell.save(cache/(key+'.png'))
            rows.append(image)
    finally:
        pack.close()
    return rows
