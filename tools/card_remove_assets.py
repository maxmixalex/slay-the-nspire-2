"""Sample the pinned card-removal flare, slash and red erosion into private art.

The calculator samples the original NCardRemoveVfx time line at 20 Hz. This
bounded offline profile evaluates the authored texture shaders; it never saves
PCK material/shader source in the public tree and does not invent replacement
slash artwork. The secondary GPU ember/cloud particles remain unimplemented.
"""
from __future__ import annotations

import math
import numpy as np
from PIL import Image

from build_content import texture
from content_tiles import require
from merchant_effects import sample_texture as sample_points
from patch_feedback_assets import write_image

FRAME_COUNT = 24
FRAME_MS = 50
WIDTH, HEIGHT = 208, 180
PIVOT = (104, 90)
CARD_SCALE = 80 / 300


def sample_texture(image, uv, repeat):
    return sample_points(image, uv.reshape(-1, 2), repeat).reshape((*uv.shape[:-1], 4))


def smooth(a, b, x):
    t = np.clip((x-a)/(b-a), 0, 1)
    return t*t*(3-2*t)


def curve(t, y0, y1, tangent0=0., tangent1=0.):
    """Godot's two-point cubic Hermite Curve (unit time interval)."""
    t = min(1., max(0., t))
    return (2*t**3-3*t*t+1)*y0+(t**3-2*t*t+t)*tangent0+(-2*t**3+3*t*t)*y1+(t**3-t*t)*tangent1


def blend(destination, source):
    a = source[..., 3:4]
    combined = a+destination[..., 3:4]*(1-a)
    rgb = source[..., :3]*a+destination[..., :3]*destination[..., 3:4]*(1-a)
    destination[..., :3] = np.divide(rgb, combined, out=np.zeros_like(rgb), where=combined>0)
    destination[..., 3:4] = combined


def quad_uv(width, height, origin=(0., 0.), angle=0., pivot=(0., 0.)):
    y, x = np.mgrid[:HEIGHT, :WIDTH]
    x = (x+.5-PIVOT[0])/CARD_SCALE-origin[0]
    y = (y+.5-PIVOT[1])/CARD_SCALE-origin[1]
    ca, sa = math.cos(angle), math.sin(angle)
    u = (x*ca+y*sa)/width+.5-pivot[0]
    v = (-x*sa+y*ca)/height+.5-pivot[1]
    uv = np.stack((u, v), axis=-1)
    visible = (u>=0)&(u<1)&(v>=0)&(v<1)
    return uv, visible


def build_images(pack, cache):
    scene = pack.read('scenes/vfx/ui/card/vfx_card_remove.tscn').decode()
    for expected in ('scale_min = 1.15', 'scale_min = 0.55', 'lifetime = 0.8',
                     'position = Vector2(-166, 44)', 'position = Vector2(-146, 38.99994)'):
        require(expected in scene, 'Changed card-removal scene: '+expected)
    paths = {
        'erosion': 'images/vfx/ui/card/remove/ui_card_remove_erosion.png',
        'slash': 'images/vfx/slash/slash_quick_flat.png',
        'flare': 'images/vfx/common/common_clean_flare_1.png',
        'ring': 'images/vfx/common/common_ring_polar_a.png',
    }
    images = {key: np.asarray(texture(pack, path).convert('RGBA'), dtype=float)/255 for key, path in paths.items()}
    require(images['erosion'].shape[:2] == (512, 512) and images['slash'].shape[:2] == (256, 1024),
            'Changed card-removal source texture geometry')
    rows = []
    for index in range(FRAME_COUNT):
        seconds = index*FRAME_MS/1000
        output = np.zeros((HEIGHT, WIDTH, 4))
        # NCardRemoveVfx starts the original anticipation group immediately.
        # Use the midpoint of authored purely visual particle ranges; no game
        # or saved RNG stream is read by conversion or runtime playback.
        if seconds < .2:
            t = seconds/.2
            size = 1.625*max(.001, curve(t, 1., 0., -2.188963))
            tex = images['flare'];uv, visible = quad_uv(tex.shape[1]*size, tex.shape[0]*size, (-146., 39.), math.pi/6)
            value = sample_texture(tex, uv, False)[..., 0]
            source = np.zeros_like(output)
            intensity = 1+value*1.454227
            source[..., 0] = intensity;source[..., 1] = source[..., 2] = intensity*.4
            source[..., 3] = value*visible
            blend(output, np.clip(source, 0, 1))
        if seconds < .3:
            t = seconds/.3
            scale = 1.5*(1. if t<=.3510101 else curve((t-.3510101)/(1-.3510101), 1., 0., -3.3928916*(1-.3510101)))
            if scale > .001:
                tex = images['ring'];uv, visible = quad_uv(tex.shape[1]*scale, tex.shape[0]*scale, (-146.,39.), -math.pi/12)
                centered = uv-.5
                polar = np.mod(np.stack((np.linalg.norm(centered, axis=-1)*2, np.arctan2(centered[..., 1], centered[..., 0])/(2*3.1416)), axis=-1),1.)
                value = sample_texture(tex, polar, False)[..., 0]
                source = np.zeros_like(output)
                color_t = np.clip((t-.31843576)/(.575419-.31843576), 0, 1)
                emission = max(0., curve(t, 3., 0., -8.75114))
                source[..., 0] = (1-.42532474*color_t)*(1+emission)
                source[..., 1] = source[..., 2] = (1-color_t)*(1+emission)
                source[..., 3] = smooth(.66985846, .76985846, value)*smooth(.34848484,.35353535,t)*visible
                blend(output, np.clip(source, 0, 1))
        # Original slash: start at .25s, four frames in a 2x2 flipbook, .25s
        # lifetime, .55 scale and the authored .4 horizontal vertex pivot.
        if .25 <= seconds < .5:
            t = (seconds-.25)/.25
            uv, visible = quad_uv(1024*.55, 256*.55*curve(t, 1., .5014031), (-166.,44.), -math.pi/12, (.4, 0.))
            frame = min(3, int(4*curve(t,0.,1.)*.99))
            tile = (uv+np.array((frame%2, frame//2)))/2
            tex = sample_texture(images['slash'], tile, False)
            value = tex[..., 0]
            intensity = np.where(value<.48918918, .5+value*.5/.48918918, 1+(value-.48918918)*9.5787325/(1-.48918918))
            source = np.zeros_like(output);source[..., 0] = intensity
            source[..., 1] = source[..., 2] = intensity*.20405686
            source[..., 3] = tex[..., 3]*visible
            blend(output, np.clip(source,0,1))
        # Original erosion: starts .1s after the slash, lasts .8s. The shader
        # bends its alpha mask with the authored blue channel and red LUT.
        if .35 <= seconds < 1.15:
            t = (seconds-.35)/.8
            # Integral of the authored decreasing directional-velocity curve.
            distance = 250*.8*(t-2.188963*t*t/2+(2*2.188963-3)*t**3/3+(2-2.188963)*t**4/4)
            uv, visible = quad_uv(512*1.15,512*1.15,(distance,0.))
            distortion = sample_texture(images['erosion'],uv+seconds*.35,True)[..., 2]-.5
            displaced = uv.copy();displaced[...,0] += distortion*.2
            tex = sample_texture(images['erosion'],displaced,True)
            threshold = curve(t,0.,1.,2.119946)
            alpha = smooth(threshold,threshold+.15,tex[...,3])
            bright = smooth(threshold,threshold+1,tex[...,3])
            red = .5+(.3019608-.5)*np.minimum(bright/.25135136,1)
            tint = .6*(1-.75*min(t/.8072626,1))
            source = np.zeros_like(output);source[...,0] = red*tint;source[...,3] = alpha*visible
            blend(output,source)
        rgba = Image.fromarray(np.uint8(np.clip(output,0,1)*255+.5),'RGBA')
        row = write_image(cache,f'card-remove-{index:02d}',rgba,sparse=True)
        row['key'] = f'card_remove.{index:02d}'
        rows.append(row)
    return rows
