"""Bake the pinned card-banner material controls for the small-screen palette.

The original material rotates hue in YIQ space (despite its HSV shader name).
Only its three numeric controls are read; no shader source is bundled. Texture
filtering and color-space differences from the full renderer remain possible.
"""
from __future__ import annotations
import math
import re
import numpy as np
from PIL import Image


def material_banner(pack, image: Image.Image, rarity: str) -> Image.Image:
    if rarity not in ('common', 'rare'):
        raise ValueError('Unsupported banner rarity')
    return material_tint(pack, image, f'materials/cards/banners/card_banner_{rarity}_mat.tres')


def material_tint(pack, image: Image.Image, material: str) -> Image.Image:
    text = pack.read(material).decode('utf-8')
    def value(name: str) -> float:
        match = re.search(rf'^shader_parameter/{name}\s*=\s*([0-9.]+)$', text, re.MULTILINE)
        if not match:
            raise ValueError('Missing banner material control ' + name)
        return float(match[1])
    hue, saturation, brightness = value('h'), value('s'), value('v')
    pixels = np.asarray(image.convert('RGBA'), dtype=np.float64) / 255.0
    transform = np.array([[.2989, .5870, .1140], [.5959, -.2774, -.3216], [.2115, -.5229, .3114]])
    yiq = pixels[:, :, :3] @ transform.T
    angle = (1.0 - hue) * 6.283185
    cosine, sine = math.cos(angle), math.sin(angle)
    i, q = yiq[:, :, 1].copy(), yiq[:, :, 2].copy()
    # Row-vector multiplication in the original hue rotation.
    yiq[:, :, 1] = (cosine * i - sine * q) * saturation
    yiq[:, :, 2] = (sine * i + cosine * q) * saturation
    pixels[:, :, :3] = (yiq * brightness) @ np.linalg.inv(transform).T
    return Image.fromarray(np.uint8(np.clip(pixels * 255.0, 0, 255)))
