"""Original map registration and the complete Smith impact flipbook.

The caller owns the private logical pack and locale assembly. No pixel, font,
or localized source data lives in this module. Existing hammer/spark resources
are reused; all nine cells of the authored impact texture remain available.
"""
from __future__ import annotations

from collections import OrderedDict
from pathlib import Path

from PIL import Image

from pck import Pack
from patch_feedback_assets import write_image
from world_scene_assets import load_scene_texture

BASE_AUXILIARY, AUXILIARY_COUNT = 5275, 5284
MAP_IMAGES = ((36, 'map_monster'), (37, 'map_elite'), (38, 'map_unknown'),
              (39, 'map_shop'), (40, 'map_rest'), (41, 'map_chest'))
SMITH_SCENE = 'scenes/vfx/vfx_card_smith.tscn'
IMPACT_PATH = 'images/vfx/vfx_smith/impact_circle_smith.png'


def plan() -> dict:
    return dict(reference='v0.111.0 / 41cef1ea',
                base_auxiliary=BASE_AUXILIARY, auxiliary_count=AUXILIARY_COUNT,
                replacements=[dict(image=image,
                    source='images/atlases/ui_atlas.sprites/map/icons/'+name+'.tres',
                    size=[32, 32], retain_atlas_margin=True)
                    for image, name in MAP_IMAGES]+[
                    dict(auxiliary=4, source='images/ui/reward_screen/reward_banner.png',
                         size=[244, 61], source_control=[652, 162], keep_aspect_centered=True)],
                images=[dict(auxiliary=BASE_AUXILIARY+frame, source=IMPACT_PATH,
                    source_scene=SMITH_SCENE, frame=frame, frame_count=9,
                    flipbook=[3, 3], duration_ms=500, size=[120, 120])
                    for frame in range(9)])


def source_assets(pck: str | Path, cache: str | Path, *, source) -> tuple[list[dict], dict[int, Path]]:
    """Return appended rows and existing image-ID -> replacement PNG paths.

    UI14's base map images cropped only AtlasTexture.region, whereas their
    separate outline retained AtlasTexture.margin. Use the same loader and
    same32px logical canvas for both, so shape centers and scales agree.
    """
    from content_tiles import private_path

    cache = private_path(cache, 'Private UI15 room artwork cache')
    cache.mkdir(parents=True, exist_ok=True)
    pack, textures = Pack(pck), OrderedDict()
    rows, replacements = [], {}
    try:
        for item in plan()['replacements']:
            original = load_scene_texture(pack, item['source'], cache=textures)
            if item.get('retain_atlas_margin') and original.size != (128, 128):
                raise ValueError('Original map AtlasTexture registration changed: '+item['source'])
            image_id = item['image'] if 'image' in item else source.aux[item['auxiliary']]
            if item.get('keep_aspect_centered'):
                from build_content import prepared
                image = prepared(original, tuple(item['size']), 'contain')
            else:
                image = original.resize(tuple(item['size']), Image.Resampling.LANCZOS)
            path = cache/('room-image-'+str(image_id)+'.png')
            image.save(path)
            replacements[image_id] = path
        flipbook = load_scene_texture(pack, IMPACT_PATH, cache=textures).convert('RGBA')
        if flipbook.size != (1024, 1024):
            raise ValueError('Original Smith3x3 flipbook dimensions changed')
        for item in plan()['images']:
            frame = item['frame'];x, y = frame % 3, frame // 3
            # Godot divides the1024px sheet into thirds with fractional UVs.
            # Sampling that exact box avoids unequal341/342px cropped cells.
            image = flipbook.resize(tuple(item['size']), Image.Resampling.LANCZOS,
                box=(x*flipbook.width/3, y*flipbook.height/3,
                     (x+1)*flipbook.width/3, (y+1)*flipbook.height/3))
            key = 'smith-impact-'+str(frame)
            row = write_image(cache, key, image)
            image.save(cache/(key+'.png'))
            row.update(item, png=key+'.png')
            rows.append(row)
    finally:
        pack.close()
    if len(rows) != AUXILIARY_COUNT-BASE_AUXILIARY:
        raise ValueError('UI15 Smith resource count changed')
    return rows, replacements
