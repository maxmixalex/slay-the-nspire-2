#!/usr/bin/env python3
"""Recover the original main-menu logo as one small private sprite atlas.

The title's Spine has a static text attachment and ten small flame drawings.
Its 160-second attachment timeline repeats those drawings, so neither a long
prebaked clip nor ten duplicates of the text are needed on the calculator. One
original light mask also supplies the separate animated additive glow.
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


def _scene_transform(pack, path, supersample):
    """Follow the actual Godot parent anchors and CanvasItem transforms."""
    root = SceneLoader(pack).instantiate(PROFILE['source_scene'])
    size = PROFILE['viewport']
    matrix = np.eye(3)
    node = root
    for name in path.split('/'):
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


def _logo_transform(pack, supersample):
    return _scene_transform(pack, PROFILE['source_node'], supersample)


def _scene_layer(pack, texture_loader, skeleton, path):
    """Keep each original Spine plane separate so particles sit between them."""
    supersample = 2
    sk = Skeleton(pack.read(pack.imported(skeleton+'.skel', '.spskel')))
    textures = load_textures(pack, skeleton, texture_loader)
    draw = geometry(sk, *pose(sk, PROFILE['animation'], 0), textures)
    transform = _scene_transform(pack, path, supersample)
    screen_draw = []
    for vertices, uv, triangles, image, tint, blend in draw:
        points = np.column_stack((vertices[:, 0], -vertices[:, 1], np.ones(len(vertices))))
        points = (transform @ points.T).T[:, :2]
        points[:, 1] *= -1
        screen_draw.append((points, uv, triangles, image, tint, blend))
    width, height = PROFILE['screen']
    return raster(screen_draw, 1., np.zeros(2), (width*supersample, height*supersample)).resize(
        (width, height), Image.Resampling.LANCZOS)


def build_particle_atlas(pack, texture_loader):
    """Seven authored fog drawings and the tiny original star, without a clip."""
    background = PROFILE['background']
    fog, stars = background['fog'], background['stars']
    source = texture_loader(pack, fog['texture']).convert('RGBA')
    fw, fh = fog['source_frame_size']
    if source.size != (fw, fh*fog['frames']):
        raise ValueError('The original fog texture no longer has seven vertical frames')
    atlas = Image.new('RGBA', tuple(fog['atlas_size']))
    width, height = fog['frame_size']
    for frame in range(fog['frames']):
        image = source.crop((0, frame*fh, fw, (frame+1)*fh)).resize(
            (width, height), Image.Resampling.LANCZOS)
        atlas.alpha_composite(image, ((frame % fog['columns'])*width,
                                     (frame // fog['columns'])*height))
    x, y, width, height = stars['atlas_rect']
    star = texture_loader(pack, stars['texture']).convert('RGBA').resize(
        (width, height), Image.Resampling.LANCZOS)
    atlas.alpha_composite(star, (x, y))
    return atlas


def build_title_background(pack, texture_loader):
    profile = PROFILE['background']
    return {
        'backdrop': _scene_layer(pack, texture_loader, profile['back_skeleton'], profile['back_node']),
        'foreground': _scene_layer(pack, texture_loader, profile['front_skeleton'], profile['front_node']),
        'particles': build_particle_atlas(pack, texture_loader),
    }


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
    glow = PROFILE['glow']
    mask = texture_loader(pack, glow['texture']).convert('RGBA').resize(
        tuple(glow['mask_size']), Image.Resampling.LANCZOS)
    pixels = np.asarray(mask).copy()
    # Additive black contributes exactly zero, so transparent black spans
    # need not occupy the sparse atlas or visit the framebuffer at runtime.
    pixels[:, :, 3][np.all(pixels[:, :, :3] == 0, axis=2)] = 0
    atlas.alpha_composite(Image.fromarray(pixels), (glow['mask_x'], glow['atlas_y']))
    return atlas


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--background', action='store_true',
                        help='Also export the original background/foreground and particle atlas for NSP2/21')
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
        background = build_title_background(pack, texture) if args.background else None
    finally:
        pack.close()
    raw = encode(atlas)
    atlas.save(output/'title-logo.png')
    (output/'title-logo.raw').write_bytes(raw)
    manifest = dict(PROFILE, width=atlas.width, height=atlas.height, format=4,
                    raw_file='title-logo.raw', png_file='title-logo.png', bytes=len(raw),
                    source='original main_menu_logo Spine attachments and light_large light texture',
                    replaces_image=4, added_images=0, added_clips=0)
    (output/'title-logo.json').write_text(json.dumps(manifest, indent=2)+'\n')
    if background is not None:
        rows = {}
        for key, image in background.items():
            raw = encode(image)
            image.save(output/('title-'+key+'.png'))
            (output/('title-'+key+'.raw')).write_bytes(raw)
            rows[key] = dict(width=image.width, height=image.height, format=4,
                             raw_file='title-'+key+'.raw', png_file='title-'+key+'.png', bytes=len(raw))
        scene_manifest = dict(reference=PROFILE['reference'], source_scene=PROFILE['source_scene'],
                              background=PROFILE['background'], images=rows,
                              replaces_auxiliary=0, added_auxiliary=['TitleForeground', 'TitleParticles'],
                              added_images=2, added_clips=0, output_version=21,
                              adaptations=PROFILE['background_adaptations'])
        (output/'title-background.json').write_text(json.dumps(scene_manifest, indent=2)+'\n')
    print(json.dumps({key: manifest[key] for key in ('width', 'height', 'format', 'bytes',
                                                  'replaces_image', 'added_images', 'added_clips')}))


if __name__ == '__main__':
    main()
