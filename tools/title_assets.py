#!/usr/bin/env python3
"""Recover the original main-menu logo as one small private sprite atlas.

The title's Spine has a static text attachment and ten small flame drawings.
Its 160-second attachment timeline repeats those drawings, so neither a long
prebaked clip nor ten duplicates of the text are needed on the calculator.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image

from animation import Skeleton, geometry, load_textures, pose, raster
from world_scene_assets import SceneLoader, _node_matrix, _property, _scale, _translate


ROOT = Path(__file__).resolve().parents[1]
PROFILE = json.loads((ROOT / 'config/title-scene-0.17.json').read_text())


def _control_rect(node, parent_size):
    props = node.props
    left = float(_property(props, 'anchor_left', 0)) * parent_size[0] + float(_property(props, 'offset_left', 0))
    top = float(_property(props, 'anchor_top', 0)) * parent_size[1] + float(_property(props, 'offset_top', 0))
    right = float(_property(props, 'anchor_right', 0)) * parent_size[0] + float(_property(props, 'offset_right', 0))
    bottom = float(_property(props, 'anchor_bottom', 0)) * parent_size[1] + float(_property(props, 'offset_bottom', 0))
    return (left, top), (right-left, bottom-top)


def _logo_transform(pack, supersample):
    """Follow the actual Godot parent anchors and CanvasItem transforms."""
    root = SceneLoader(pack).instantiate(PROFILE['source_scene'])
    size = PROFILE['viewport']
    matrix = np.eye(3)
    node = root
    for name in PROFILE['source_node'].split('/'):
        node = next(child for child in node.children if child.name == name)
        if node.kind == 'Control':
            origin, size = _control_rect(node, size)
            matrix = matrix @ _node_matrix(node.props, origin)
        else:
            matrix = matrix @ _node_matrix(node.props)
    width, height = PROFILE['screen']
    vw, vh = PROFILE['viewport']
    scale = max(width/vw, height/vh) * supersample
    screen = _translate((width*supersample-vw*scale)/2,
                        (height*supersample-vh*scale)/2) @ _scale(scale)
    return screen @ matrix


def build_title_atlas(pack, texture_loader):
    name = PROFILE['skeleton']
    sk = Skeleton(pack.read(pack.imported(name+'.skel', '.spskel')))
    textures = load_textures(pack, name, texture_loader)
    text_slot = next(i for i, slot in enumerate(sk.slots) if slot['name'] == 'text')
    fire_slot = next(i for i, slot in enumerate(sk.slots) if slot['name'] == 'fire')
    supersample = 2
    transform = _logo_transform(pack, supersample)
    width, height = PROFILE['screen']

    def layer(seconds, slot):
        world, slots, deforms, order = pose(sk, PROFILE['animation'], seconds)
        draw = geometry(sk, world, slots, deforms, [i for i in order if i == slot], textures)
        screen_draw = []
        for vertices, uv, triangles, image, tint, blend in draw:
            # Godot uses downward Y; the shared Spine rasterizer uses upward Y.
            points = np.column_stack((vertices[:, 0], -vertices[:, 1], np.ones(len(vertices))))
            points = (transform @ points.T).T[:, :2]
            points[:, 1] *= -1
            screen_draw.append((points, uv, triangles, image, tint, blend))
        return raster(screen_draw, 1., np.zeros(2), (width*supersample, height*supersample)).resize(
            (width, height), Image.Resampling.LANCZOS)

    atlas = Image.new('RGBA', tuple(PROFILE['atlas_size']))
    x, y, w, h = PROFILE['text_screen_rect']
    atlas.alpha_composite(layer(0., text_slot).crop((x, y, x+w, y+h)), (0, 0))
    x, y, w, h = PROFILE['fire_screen_rect']
    for frame in range(PROFILE['fire_frames']):
        # Midpoints avoid selecting the previous frame at Spine float keys.
        seconds = (frame+.5)*PROFILE['fire_frame_ms']/1000
        image = layer(seconds, fire_slot).crop((x, y, x+w, y+h))
        atlas.alpha_composite(image, (frame*w, PROFILE['fire_atlas_y']))
    return atlas


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    from build_content import texture
    from pck import Pack
    from sparse_image import encode
    output = args.output.resolve()
    if output == ROOT or ROOT in output.parents:
        raise ValueError('Original logo output must stay outside the public source repository')
    output.mkdir(parents=True, exist_ok=True)
    pack = Pack(args.pck)
    try:
        atlas = build_title_atlas(pack, texture)
    finally:
        pack.close()
    raw = encode(atlas)
    atlas.save(output/'title-logo.png')
    (output/'title-logo.raw').write_bytes(raw)
    manifest = dict(PROFILE, width=atlas.width, height=atlas.height, format=4,
                    raw_file='title-logo.raw', png_file='title-logo.png', bytes=len(raw),
                    source='original main_menu_logo Spine text and fire attachments',
                    replaces_image=4, added_images=0, added_clips=0)
    (output/'title-logo.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps({key: manifest[key] for key in ('width', 'height', 'format', 'bytes',
                                                  'replaces_image', 'added_images', 'added_clips')}))


if __name__ == '__main__':
    main()
