"""Resample the three original pile icons at their actual 24px LCD size.

Only source paths and conversion code belong in Git. Outputs remain in the
caller's private cache, replacing image IDs 24..26 without an auxiliary append.
"""
from pathlib import Path

from PIL import Image

import content_tiles as tiles
from pck import Pack
from world_scene_assets import load_scene_texture


def source_replacements(pck, cache):
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    owned = not hasattr(pck, 'read')
    pack = Pack(pck) if owned else pck
    rows = []
    try:
        for image_id, key in enumerate(('draw', 'discard', 'exhaust'), 24):
            scene = pack.read('scenes/combat/' + key + '_pile.tscn').decode()
            path = 'images/packed/combat_ui/' + key + '_pile.png'
            tiles.require(path in scene, 'Pinned pile texture changed: ' + key)
            original = load_scene_texture(pack, path).convert('RGBA')
            scale = min(24 / original.width, 24 / original.height)
            size = (max(1, round(original.width * scale)), max(1, round(original.height * scale)))
            image = Image.new('RGBA', (24, 24))
            image.alpha_composite(original.resize(size, Image.Resampling.LANCZOS),
                                  ((24 - size[0]) // 2, (24 - size[1]) // 2))
            png = cache / ('combat-' + key + '-pile.png')
            image.save(png)
            rows.append(dict(image=image_id, png=str(png), source=path,
                             source_size=list(original.size), size=[24, 24]))
        return rows
    finally:
        if owned:
            pack.close()
