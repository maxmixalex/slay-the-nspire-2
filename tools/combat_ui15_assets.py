"""Pinned fullscreen-heal textures, decoded only from the owner's PCK."""
from pathlib import Path
from PIL import Image, ImageOps
from pck import Pack
from patch_feedback_assets import write_image
from world_scene_assets import load_scene_texture

BASE_AUXILIARY = 5284
AUXILIARY_COUNT = 5288


def source_assets(pck, cache):
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    pack, rows = Pack(pck), []
    try:
        for name, source, size in (
            ('heal-cross', 'images/vfx/vfx_heal/cross_asset.png', (24, 24)),
            ('heal-falloff', 'images/vfx/vfx_heal/falloff_asset.png', (20, 20)),
            ('heal-glow', 'images/vfx/vfx_heal/heal_asset.png', (72, 72)),
            ('heal-vignette', 'images/vfx/vignette_cloud2.png', (320, 240)),
        ):
            image = load_scene_texture(pack, source).convert('RGBA')
            if name == 'heal-vignette':
                # NSmokyVignetteVfx uses KeepAspectCovered on the full viewport.
                image = ImageOps.fit(image, size, Image.Resampling.LANCZOS)
            else:
                image = image.resize(size, Image.Resampling.LANCZOS)
            row = write_image(cache, name, image)
            row.update(source=source, auxiliary=BASE_AUXILIARY+len(rows))
            image.save(cache/(name+'.png'))
            rows.append(row)
    finally:
        pack.close()
    assert BASE_AUXILIARY+len(rows) == AUXILIARY_COUNT
    return rows
