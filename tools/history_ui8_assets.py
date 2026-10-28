"""Original NTinyCard layers for the bounded run-history presentation.

Only paths, layer order and source-scene checks belong in the public tree.
Callers supply the private pinned PCK and keep all encoded output private.
"""
from pathlib import Path
from PIL import Image

from content_tiles import require
from patch_feedback_assets import write_image
from world_scene_assets import load_scene_texture

BASE_AUX, COUNT = 1945, 10
NAMES = ('card_back', 'desc_box', 'attack_portrait', 'attack_portrait_shadow',
         'skill_portrait', 'skill_portrait_shadow', 'power_portrait',
         'power_portrait_shadow', 'banner', 'banner_shadow')


def build_images(pack, cache):
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    scene = pack.read('scenes/cards/tiny_card.tscn').decode()
    for expected in ('custom_minimum_size = Vector2(32, 32)',
                     'modulate = Color(0, 0, 0, 0.25098)',
                     'modulate = Color(0.95, 0.920075, 0.6935, 1)',
                     'modulate = Color(0.392157, 1, 1, 1)'):
        require(expected in scene, 'Changed original tiny-card scene: ' + expected)
    rows = []
    for name in NAMES:
        key = 'history-tiny-' + name.replace('_', '-')
        source = 'images/packed/run_history/' + name + '.png'
        image = load_scene_texture(pack, source).convert('RGBA')
        require(image.size == (128, 128), 'Changed original tiny-card layer size: ' + source)
        # The authored 32px control explicitly selects nearest filtering.
        # Keep that control-resolution layer rather than ten 128px decode buffers.
        image = image.resize((32, 32), Image.Resampling.NEAREST)
        image.save(cache / (key + '.png'))
        row = write_image(cache, key, image)
        row.update(key=key, source=source, source_size=[128, 128],
                   source_control_size=[32, 32], filter='nearest',
                   auxiliary=BASE_AUX + len(rows))
        rows.append(row)
    require(len(rows) == COUNT, 'Tiny-card layer count changed')
    return rows
