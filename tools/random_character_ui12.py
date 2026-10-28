#!/usr/bin/env python3
"""Bake the authored Random selection gradient at the calculator viewport.

The scene, seven color stops and pixels are read from the user's private PCK.
Godot 4.5.1 GradientTexture2D linear fill and Gradient cubic interpolation are
evaluated at their authored/default texture dimensions. Existing character
scene geometry then projects the image into the same 1680x1260 camera.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import numpy as np
from PIL import Image

from build_content import texture
from pck import Pack
from world_scene_assets import _Renderer, _node_matrix, _property, _vector, Property, SceneRenderError


class RandomRenderer(_Renderer):
    def texture(self, ref):
        attrs, props = self.loader.resource(ref)
        if attrs.get('type') != 'GradientTexture2D':
            return super().texture(ref)
        if any(_property(props, key, default) != default for key, default in
               (('fill', 0), ('repeat', 0), ('use_hdr', False))):
            raise SceneRenderError('Unexpected Random gradient texture mode')
        _, gradient = self.loader.resource(_property(props, 'gradient'))
        if (_property(gradient, 'interpolation_mode', 0) != 2 or
                _property(gradient, 'interpolation_color_space', 0) != 0):
            raise SceneRenderError('Expected original cubic sRGB Random gradient')
        offsets = np.asarray(_property(gradient, 'offsets').args, dtype=np.float32)
        colors = np.asarray(_property(gradient, 'colors').args, dtype=np.float32).reshape(-1, 4)
        width, height = (int(_property(props, key, 64)) for key in ('width', 'height'))
        start, end = (_vector(_property(props, key)) for key in ('fill_from', 'fill_to'))
        direction = end-start
        y, x = np.mgrid[:height, :width]
        uv = np.stack((x/(width-1), y/(height-1)), axis=-1)
        t = np.clip(np.sum((uv-start)*direction, axis=-1)/np.dot(direction, direction), 0, 1)
        left = np.clip(np.searchsorted(offsets, t, side='right')-1, 0, len(offsets)-2)
        weight = np.clip((t-offsets[left])/(offsets[left+1]-offsets[left]), 0, 1)[..., None]
        p0, p1, p2, p3 = (colors[np.clip(left+shift, 0, len(colors)-1)] for shift in (-1, 0, 1, 2))
        # Godot Math::cubic_interpolate (Catmull-Rom), including duplicated
        # endpoint controls and UNORM truncation before texture filtering.
        pixels = .5*((2*p1)+(-p0+p2)*weight+(2*p0-5*p1+4*p2-p3)*weight**2+
                     (-p0+3*p1-3*p2+p3)*weight**3)
        return Image.fromarray(np.uint8(np.clip(pixels*255, 0, 255)), 'RGBA')


def build(pack, output):
    renderer = RandomRenderer(pack, 320, 240, (1680, 1260), texture, None, 0, 1)
    screen = renderer.loader.instantiate('scenes/screens/character_select_screen.tscn')
    container = next(node for node in screen.children if node.name == 'AnimatedBg')
    origin, size = renderer.control_size(container, np.asarray((1680, 1260), dtype=float))
    container.props['scale'] = Property('Vector2(1.2683, 1.2683)', container.scope)
    scene = 'scenes/screens/char_select/char_select_bg_random_character.tscn'
    node = renderer.loader.instantiate(scene)
    renderer.visit(node, _node_matrix(container.props, origin), size, np.ones(4), 0, node.name)
    image, report = renderer.finish()
    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output)
    report.update(auxiliary=2160, source=scene, output=str(output.resolve()),
                  limits=['Original water distortion held at authored static gradient',
                          'Independent decorative particle systems omitted'])
    output.with_suffix('.json').write_text(json.dumps(report, indent=2)+'\n')
    return image


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    pack = Pack(args.pck)
    try:
        build(pack, args.output)
    finally:
        pack.close()
