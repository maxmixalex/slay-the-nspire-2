"""Original Neow screen-water noise sampling for the bundled desktop helper.

Godot4.5.1's default FastNoiseLite mode is OpenSimplex2S, unlike the explicit
OpenSimplex2 used by Random/Orobas. Neow also explicitly disables normalization.
The renderer/typed post-processing stage is kept separate from this helper.
"""

from __future__ import annotations

import ctypes

import numpy as np

_NEOW_NOISE_CALLER = r"""#include "FastNoiseLite.h"
#if defined(_WIN32)
#define NSPIRE_NEOW_EXPORT __declspec(dllexport)
#else
#define NSPIRE_NEOW_EXPORT
#endif
extern "C" NSPIRE_NEOW_EXPORT void nspire_neow_noise(float*out,int w,int h){
 fastnoiselite::FastNoiseLite noise(0);
 noise.SetNoiseType(fastnoiselite::FastNoiseLite::NoiseType_OpenSimplex2S);
 noise.SetFrequency(0.0022f);
 noise.SetFractalType(fastnoiselite::FastNoiseLite::FractalType_FBm);
 noise.SetFractalOctaves(5);noise.SetFractalLacunarity(2.0f);
 noise.SetFractalGain(0.5f);noise.SetFractalWeightedStrength(0.0f);
 for(int y=0;y<h;++y)for(int x=0;x<w;++x)out[y*w+x]=noise.GetNoise(float(x),float(y));
}
"""


def source_values(library, width, height):
    """Raw original float samples; callers apply source non-normalized R8."""
    if not 0 < width <= 1024 or not 0 < height <= 1024:
        raise ValueError("Neow source noise dimensions exceed the reviewed helper bound")
    helper = ctypes.CDLL(str(library))
    helper.nspire_neow_noise.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_int, ctypes.c_int]
    helper.nspire_neow_noise.restype = None
    result = np.empty((height, width), dtype=np.float32)
    helper.nspire_neow_noise(result.ctypes.data_as(ctypes.POINTER(ctypes.c_float)), width, height)
    return result


def nonnormalized_r8(values):
    return np.uint8(np.clip(values * np.float32(127.5) + np.float32(127.5), 0, 255))
