"""Source-owned card transformation, enchantment and event UI append.

Only scene parameters and resource paths are public. The supplied private PCK
provides every pixel; the caller owns pack assembly and locale preservation.
"""
from __future__ import annotations
from pathlib import Path
import numpy as np
from PIL import Image
from pck import Pack
from world_scene_assets import load_scene_texture
from patch_feedback_assets import write_image

BASE_AUXILIARY = 2204
AUXILIARY_COUNT = 2228


def _white_alpha(source: Image.Image, size: tuple[int, int]) -> Image.Image:
    source = source.convert('RGBA').resize(size, Image.Resampling.LANCZOS)
    # vfx_grayscale_particle_shader uses red as coverage and a white LUT.
    result = Image.new('RGBA', size, 'white')
    result.putalpha(source.getchannel('R'))
    return result


def _polar_ring(source: Image.Image, lifetime: float) -> Image.Image:
    # vfx_ring_polar_shader + polar_coordinates.gdshaderinc, including the
    # source erosion Curve's (0,.499536,out=1.15666), (1,1,in=0) keys.
    size = 64
    tex = np.asarray(source.convert('RGBA'), dtype=np.float64)[:, :, 0] / 255
    yy, xx = np.mgrid[0:size, 0:size]
    x, y = (xx + .5) / size - .5, (yy + .5) / size - .5
    u = (np.sqrt(x*x + y*y) * 2) % 1
    v = (np.arctan2(y, x) / (3.1416 * 2)) % 1
    tx, ty = u * tex.shape[1] - .5, v * tex.shape[0] - .5
    ix, iy = np.floor(tx).astype(int), np.floor(ty).astype(int)
    fx, fy = tx - ix, ty - iy
    sample = np.zeros((size, size))
    for dx, wx in ((0, 1-fx), (1, fx)):
        for dy, wy in ((0, 1-fy), (1, fy)):
            sample += tex[(iy+dy) % tex.shape[0], (ix+dx) % tex.shape[1]] * wx * wy
    t = lifetime
    erosion = (2*t**3-3*t*t+1)*.499536 + (t**3-2*t*t+t)*1.15666 + (-2*t**3+3*t*t)
    alpha = np.clip((sample-erosion)/.1, 0, 1)
    alpha = alpha*alpha*(3-2*alpha)
    pixels = np.full((size, size, 4), 255, dtype=np.uint8)
    pixels[:, :, 3] = np.rint(alpha*255).astype(np.uint8)
    return Image.fromarray(pixels)


def source_assets(pck: str | Path, cache: str | Path) -> list[dict]:
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    pack, rows = Pack(pck), []

    def load(path):
        return load_scene_texture(pack, path).convert('RGBA')

    def add(key, path, image, **parameters):
        row = write_image(cache, key, image)
        row.update(source=path, auxiliary=BASE_AUXILIARY+len(rows), **parameters)
        image.save(cache/(key+'.png'))
        rows.append(row)

    try:
        path = 'images/atlases/ui_atlas.sprites/card/card_enchant_s.tres'
        add('card-enchantment', path, load(path).resize((36, 27), Image.Resampling.LANCZOS),
            source_control=[-166, -116, 72, 54], no_star_offset_y=-45,
            material_active=[.25, .4, .6], material_disabled=[.25, .1, .6])
        path = 'images/vfx/ui/ui_card_mask.png'
        mask = load(path).resize((80, 112), Image.Resampling.LANCZOS)
        add('card-transform-mask', path, mask)
        path = 'images/vfx/ui/card/ui_card_border_glow.png'
        add('card-transform-glow', path, load(path).resize((80, 112), Image.Resampling.LANCZOS))
        path = 'images/vfx/ui/card/transform/ui_card_transform_shine_flipbook.png'
        source = load(path)
        for frame in range(9):
            x, y = frame % 3, frame // 3
            art = source.crop((round(x*source.width/3), round(y*source.height/3),
                               round((x+1)*source.width/3), round((y+1)*source.height/3)))
            # The source particle quad is1024px at0.44117647. CardMask is
            # 300x422; crop/mask at that scale before the native raster bake.
            art = art.resize((120, 120), Image.Resampling.LANCZOS).crop((20, 4, 100, 116))
            rgba = np.full((112, 80, 4), 255, dtype=np.uint8)
            alpha = np.asarray(art)[:, :, 3].astype(np.uint16)
            rgba[:, :, 3] = ((alpha*np.asarray(mask)[:, :, 3]+127)//255).astype(np.uint8)
            add('card-transform-shine-'+str(frame), path, Image.fromarray(rgba),
                frame=frame, frame_count=9, flipbook=[3, 3], duration_ms=350)
        path = 'images/vfx/common/common_glow_speck.png'
        add('card-transform-speck', path, _white_alpha(load(path), (12, 12)))
        path = 'images/vfx/common/common_clean_flare_1.png'
        add('card-transform-flare', path, _white_alpha(load(path), (56, 56)))
        path = 'images/vfx/common/common_ring_polar_b.png'
        ring = load(path)
        for frame in range(8):
            add('card-transform-ring-'+str(frame), path, _polar_ring(ring, frame/8),
                shader='shaders/vfx/common/vfx_ring_polar_shader.gdshader',
                frame=frame, frame_count=8, duration_ms=500)
        for name, height in (('event_button', 40), ('event_button_outline', 41)):
            path = 'images/packed/common_ui/'+name+'.png'
            art = load(path)
            add(name.replace('_', '-'), path, art.resize((104, height), Image.Resampling.LANCZOS),
                runtime_nine_patch=[12, 10, 12, 10], source_size=list(art.size))
    finally:
        pack.close()
    if BASE_AUXILIARY+len(rows) != AUXILIARY_COUNT:
        raise ValueError('UI13 room resource directory changed')
    return rows
