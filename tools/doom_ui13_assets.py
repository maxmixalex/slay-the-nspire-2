"""Private original Doom textures for the bounded runtime particle sampler.

The source flame's two noise/distortion layers remain animated at runtime.
No frozen scene render or shared creature canvas substitutes for that shader.
"""
from pathlib import Path

from PIL import Image

import content_tiles as tiles
from patch_feedback_assets import write_image
from pck import Pack
from world_scene_assets import load_scene_texture

BASE_AUXILIARY, AUXILIARY_COUNT = 3331, 3339


def source_assets(pck, cache):
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    owned = not hasattr(pck, 'read')
    pack = Pack(pck) if owned else pck
    specs = (
        ('doom-glow', 'images/vfx/vfx_doom/doom_glow.png', (64, 64)),
        ('doom-hole', 'images/vfx/vfx_doom/doom_hole_bottom.png', (64, 64)),
        ('doom-globs', 'images/vfx/vfx_doom/foom_glob_particles.png', (32, 32)),
        ('doom-flame', 'images/vfx/vfx_doom/doom_flame.png', (128, 128)),
        ('doom-fill', 'images/vfx/vfx_doom/doom_fill_particle.png', (32, 11)),
        ('doom-spear', 'images/vfx/vfx_doom/doom_spear_particle.png', (8, 75)),
        ('doom-noise', 'images/vfx/tileable_noise_1.png', (64, 64)),
        ('doom-flame-mask', 'images/vfx/fire/fire_bottom_mask.png', (64, 64)),
    )
    rows = []
    try:
        scene = pack.read('scenes/vfx/vfx_doom.tscn').decode()
        for index, (key, path, size) in enumerate(specs):
            tiles.require(path in scene, 'Pinned Doom texture changed: '+path)
            original = load_scene_texture(pack, path).convert('RGBA')
            image = original.resize(size, Image.Resampling.LANCZOS)
            image.save(cache/(key+'.png'))
            row = write_image(cache, key, image)
            row.update(auxiliary=BASE_AUXILIARY+index, source=path,
                       source_size=list(original.size), size=list(size))
            rows.append(row)
        return rows
    finally:
        if owned:
            pack.close()
