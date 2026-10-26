"""Pinned card overlays and Sovereign Blade, read from the owner's PCK.

Scene geometry, materials and texture pixels are interpreted directly. The
calculator stores eight shader phases per affliction, all thirty infection
frames, and all ten ancient flames. This is a bounded LDR CPU sampling of the
authored shaders, not a claim of GPU or particle-simulation equivalence.
"""
from __future__ import annotations
import copy
import json
from pathlib import Path
import numpy as np
from PIL import Image
from pck import Pack
from patch_feedback_assets import write_image
from world_scene_assets import (Property, Call, Ref, SceneRenderError, _property,
    _vector, _translate, load_scene_texture)
from world_room_shaders import RoomRenderer, generated_texture, quad_uv, sample_texture, smoothstep

BASE_AUXILIARY = 5156
AUXILIARY_COUNT = 5265
AFFLICTIONS = ('bound', 'entangled', 'galvanized', 'hexed', 'tainted', 'ringing', 'smog')
PERIODS = (4., 40/7, 10., 20., 40/7, 2.5, 20.)


def _sequence(value):
    return value.args if isinstance(value, Call) else value


def _curve(loader, ref, width=256):
    if ref is None:
        return np.zeros(width, np.float32)
    _, props = loader.resource(ref)
    raw = _sequence(_property(props, '_data', ()))
    if not raw:
        return np.zeros(width, np.float32)
    points = np.array([[*_vector(raw[i]), float(raw[i+1]), float(raw[i+2]),
                       int(raw[i+3]), int(raw[i+4])] for i in range(0, len(raw), 5)])
    if len(points) == 1:
        return np.full(width, points[0, 1], np.float32)
    resolution = int(_property(props, 'bake_resolution', 100))
    x = np.linspace(0., 1., resolution)
    index = np.clip(np.searchsorted(points[:, 0], x, side='right')-1, 0, len(points)-2)
    a, b = points[index], points[index+1]
    span = b[:, 0]-a[:, 0]
    t = np.clip((x-a[:, 0])/span, 0., 1.)
    slope = (b[:, 1]-a[:, 1])/span
    outgoing = np.where(a[:, 5] == 1, slope, a[:, 3])
    incoming = np.where(b[:, 4] == 1, slope, b[:, 2])
    cache = ((2*t**3-3*t*t+1)*a[:, 1] + (t**3-2*t*t+t)*outgoing*span +
             (-2*t**3+3*t*t)*b[:, 1] + (t**3-t*t)*incoming*span)
    cache[x <= points[0, 0]] = points[0, 1]
    cache[x >= points[-1, 0]] = points[-1, 1]
    return np.interp(np.arange(width)/width, x, cache).astype(np.float32)


class CardRenderer(RoomRenderer):
    def visit(self, node, *args, **kwargs):
        # Infection's texture track is sampled explicitly by source_assets.
        # AnimationPlayer is not a CanvasItem and has no independent pixels.
        if node.kind == 'AnimationPlayer':
            return
        return super().visit(node, *args, **kwargs)

    def _shader_texture(self, ref):
        attrs, props = self.loader.resource(ref)
        if attrs.get('type') == 'CurveXYZTexture':
            return np.stack([_curve(self.loader, _property(props, 'curve_'+axis))
                             for axis in 'xyz'], axis=-1)[None, :, :]
        if attrs.get('type') == 'GradientTexture1D':
            gradient = _property(props, 'gradient')
            _, values = self.loader.resource(gradient)
            mode = int(_property(values, 'interpolation_mode', 0))
            if mode == 2:
                offsets = np.array(_sequence(_property(values, 'offsets', (0., 1.))))
                colors = np.array(_sequence(_property(values, 'colors'))).reshape(-1, 4)
                width = int(_property(props, 'width', 256))
                x = np.arange(width)/(width-1)
                index = np.clip(np.searchsorted(offsets, x)-1, 0, len(offsets)-2)
                a, b = colors[index], colors[index+1]
                t = np.clip((x-offsets[index])/(offsets[index+1]-offsets[index]), 0., 1.)
                # Godot's cubic_interpolate uses the neighbouring colors;
                # endpoints repeat their nearest authored stop.
                p0 = colors[np.maximum(0, index-1)]
                p3 = colors[np.minimum(len(colors)-1, index+2)]
                t = t[:, None]
                result = .5*((2*a)+(-p0+b)*t+(2*p0-5*a+4*b-p3)*t*t+(-p0+3*a-3*b+p3)*t*t*t)
                return result[None].astype(np.float32)
        return super()._shader_texture(ref)

    def shader_layer(self, node, path, image, rect, matrix, tint, shader, material, blend, info):
        if not shader or '/afflictions/' not in shader and shader not in (
                'images/vfx/simple_noise_flat_shader.tres', 'images/vfx/simple_noise_add_shader.tres'):
            return super().shader_layer(node, path, image, rect, matrix, tint, shader, material, blend, info)
        uv, covered = quad_uv(self.canvas, self.screen, matrix, rect)
        if uv is None:
            return Image.new('RGBA', self.canvas), blend
        main = np.asarray(image, np.float32)/255.
        color = np.asarray(tint)
        time = self.time
        def value(name, default=None):
            return _property(material, 'shader_parameter/'+name, default)
        def vector(name, size=2, default=None):
            return _vector(value(name, default), size)
        def tex(name, at):
            return sample_texture(self._shader_texture(value(name)), at)
        def sample(at):
            return sample_texture(main, at, repeat=_property(node.props, 'texture_repeat', 0) == 2)
        def erosion(name, v):
            a, b = vector(name)
            return smoothstep((a, a+b), v)
        def tiling(at, name):
            st = vector(name, 4)
            return at*st[:2]+time*st[2:]
        def polar(at, zoom=1):
            d = at-.5
            return np.stack((np.sqrt(np.sum(d*d, axis=-1))*2*zoom,
                             np.arctan2(d[..., 1], d[..., 0])/(3.1416*2)), axis=-1)%1
        pixel = np.zeros((*uv.shape[:2], 4), np.float32)
        base = sample(uv)
        if shader.endswith('bound_border_shader.gdshader'):
            pixel[..., :3] = sample(tiling(polar(uv), 'main_st'))[..., :3]*color[:3]
            pixel[..., 3] = erosion('mask_erosion', color[3]*tex('mask', uv)[..., 3])
        elif shader.endswith('bound_main_shader.gdshader'):
            energy = 1+(float(value('bright_energy'))-1)*sample(tiling(uv, 'bright_st'))[..., 1]
            pixel[..., :3] = base[..., 0:1]*color[:3]*energy[..., None]
            pixel[..., 3] = base[..., 3]*color[3]
        elif shader.endswith('entangled_main_shader.gdshader'):
            st = vector('distortion_st', 4)
            distortion = sample(uv*st[:2]+[st[2], time*st[3]])[..., 2]*float(value('distortion_intensity'))
            shifted = uv.copy();shifted[..., 0] += distortion*base[..., 1]
            art = sample(shifted)
            pixel[..., :3] = tex('vine_lut', np.repeat(art[..., 0:1], 2, axis=-1))[..., :3]*color[:3]
            pixel[..., 3] = art[..., 3]*color[3]
        elif shader.endswith('galvanized_main_shader.gdshader'):
            distortion_tex = sample(tiling(polar(uv), 'distortion_st'))[..., 2]
            distortion = (uv+(distortion_tex-.5)[..., None]*2*float(value('distortion_offset'))-.5)
            distortion *= distortion_tex[..., None]*-float(value('distortion_intensity'))
            blink_uv = tiling(uv, 'blink_st');blink_uv[..., 1] = 0
            blink = erosion('blink_smoothstep', sample(blink_uv)[..., 2])
            at = np.stack((blink, np.zeros_like(blink)), axis=-1)
            lut = tex('lut', at)
            distorted = sample(uv+distortion)
            pixel[..., :3] = lut[..., :3]*color[:3]
            pixel[..., 3] = np.maximum(distorted[..., 0], distorted[..., 1]*lut[..., 3]*blink)*erosion('smoothstep_factors', base[..., 3])*color[3]
        elif shader.endswith('hexed_main_shader.gdshader'):
            noise = erosion('noise_smoothstep', sample(tiling(polar(uv), 'noise_st'))[..., 2])
            inner = vector('main_color', 4)+(vector('bright_color', 4)-vector('main_color', 4))*noise[..., None]
            mixed = vector('border_color', 4)+(inner-vector('border_color', 4))*base[..., 0:1]
            pixel[..., :3] = mixed[..., :3]*color[:3]
            pixel[..., 3] = base[..., 3]*color[3]
        elif shader.endswith('ringing_main_shader.gdshader'):
            frac = time*float(value('animation_speed', 1))%1
            at = np.array([frac, frac], np.float32)
            thin, alpha = tex('thin_ring_animation', at), tex('ring_base_alpha', at)
            pixel[..., :3] = color[:3]
            pixel[..., 3] = np.maximum(np.max(base[..., :3]*alpha[:3], axis=-1),
                sample(polar(uv, .8)*.5+[thin[0], 0])[..., 3]*thin[1])
        elif shader.endswith('smog_main_shader.gdshader'):
            st, st2 = vector('main_st', 4), vector('secondary_st', 4)
            first = sample((1-uv)*st[:2]+[time*st[2], st[3]])[..., 0]
            second = sample((1-uv)*st2[:2]+[time*st2[2], st2[3]])[..., 0]
            mask = tex('mask', uv)
            smoke = erosion('erosion_factors', (first+(second-first)*float(value('secondary_smoke_influence')))*mask[..., 0])
            pixel[..., :3] = tex('lut', np.repeat(smoke[..., None], 2, axis=-1))[..., :3]*color[:3]
            pixel[..., 3] = smoke*color[3]+mask[..., 1]
        elif shader.startswith('images/vfx/simple_noise_'):
            pivot = np.array([-.275, 0.])
            at = (uv-pivot)*vector('mask_scale', default=(1, 1))+pivot+time*vector('mask_panning', default=(2, 3))
            noise = sample_texture(self._shader_texture(value('noise_tex')), at, repeat=True)[..., 0]
            if shader.endswith('flat_shader.tres'):
                at2 = (uv-pivot)*vector('mask2_scale', default=(1, 1))+pivot+time*vector('mask2_panning', default=(.2, .3))
                noise *= sample_texture(self._shader_texture(value('noise_tex2')), at2, repeat=True)[..., 0]
            pixel[..., :3] = color[:3]
            pixel[..., 3] = smoothstep(vector('Step'), base[..., 0]*noise*color[3])
        else:
            raise SceneRenderError('Unreviewed card shader: '+shader)
        pixel[..., 3] *= covered
        result = Image.fromarray(np.uint8(np.clip(np.rint(pixel*255), 0, 255)))
        self.diagnostics.setdefault('card_shaders', []).append(dict(node=path, shader=shader, time=time))
        return result, blend


def _walk(root):
    yield root
    for child in root.children:
        yield from _walk(child)


def _renderer(pack, size, viewport, time=0):
    return CardRenderer(pack, *size, viewport, load_scene_texture, None, time, 2)


def ancient_portrait_overrides(pck, cache):
    """Replace existing Ancient portrait image IDs; no auxiliary duplication."""
    cache = Path(cache);cache.mkdir(parents=True, exist_ok=True)
    pack, rows = Pack(pck), []
    try:
        mask = load_scene_texture(pack, 'images/atlases/compressed.sprites/card_template/ancient_portrait_mask_large.tres').convert('RGBA')
        glass = load_scene_texture(pack, 'images/vfx/ui/card/ancient/ui_card_ancient_border_main.png').convert('RGBA')
        for card in json.loads((Path(__file__).resolve().parents[1]/'config/world-cards-0.17.json').read_text())['cards']:
            if card['rarity'] != 'Ancient':
                continue
            source = load_scene_texture(pack, card['portrait']).convert('RGBA').resize((299, 421), Image.Resampling.LANCZOS)
            art = np.asarray(source, np.float32)/255
            alpha = np.asarray(mask.resize(source.size, Image.Resampling.LANCZOS), np.float32)[..., 3]/255
            art[..., 3] *= alpha
            # With blur_amount=0 the source glass shader reads this portrait
            # directly. Bake its additive .15 lift and .2 corner weighting.
            g = np.asarray(glass.resize(source.size, Image.Resampling.LANCZOS), np.float32)/255
            weight = g[..., 1:2]*.2
            raised = np.minimum(1., art[..., :3]+.15)*(1-weight)+weight
            ga = (g[..., 3:4]*(1-weight)+weight)*alpha[..., None]*g[..., 3:4]
            art[..., :3] = art[..., :3]*(1-ga)+raised*ga
            image = Image.fromarray(np.uint8(np.clip(np.rint(art*255), 0, 255))).resize((96, 136), Image.Resampling.LANCZOS)
            target = cache/('ancient-portrait-'+card['key'].lower()+'.png');image.save(target)
            rows.append(dict(card=card['id'], png=str(target), source=card['portrait'], size=list(image.size)))
    finally:
        pack.close()
    return rows


def source_assets(pck, cache):
    cache = Path(cache);cache.mkdir(parents=True, exist_ok=True)
    pack, rows = Pack(pck), []
    def add(key, source, image, **facts):
        row = write_image(cache, key, image)
        row.update(source=source, auxiliary=BASE_AUXILIARY+len(rows), **facts)
        image.save(cache/(key+'.png'));rows.append(row)
    try:
        directory = 'images/atlases/compressed.sprites/card_template/'
        for key, path, size in (
            ('ancient-border', directory+'ancient_card_border.tres', (104, 148)),
            ('ancient-text-attack', directory+'ancient_card_text_bg_attack.tres', (86, 66)),
            ('ancient-text-skill', directory+'ancient_card_text_bg_skill.tres', (86, 66)),
            ('ancient-text-power', directory+'ancient_card_text_bg_power.tres', (86, 66)),
            ('ancient-banner', 'images/atlases/ui_atlas.sprites/card/ancient_banner.tres', (110, 28)),
            ('ancient-highlight', directory+'card_highlight_ancient.tres', (112, 154))):
            add(key, path, load_scene_texture(pack, path).convert('RGBA').resize(size, Image.Resampling.LANCZOS))
        for frame in range(10):
            path = directory+'ancient_flame/ancient_card_flame_'+str(frame)+'.tres'
            image = load_scene_texture(pack, path).convert('RGBA')
            size = tuple(max(1, round(v*.6*96/300)) for v in image.size)
            add('ancient-flame-'+str(frame), path, image.resize(size, Image.Resampling.LANCZOS), duration_ms=1000, frame=frame)
        path = 'scenes/cards/overlays/infection.tscn'
        renderer = _renderer(pack, (104, 146), (324, 454))
        root = renderer.loader.instantiate(path)
        layer = next(n for n in _walk(root) if n.name == 'VisualLayer2')
        for frame in range(30):
            layer.props['texture'] = Property('"images/card_overlays/infection/infection_%02d.png"'%frame, path)
            renderer.commands=[]
            renderer.visit(root, _translate(162, 227), np.array([0., 0.]), np.ones(4), 0, root.name)
            image, info = renderer.finish()
            add('infection-'+str(frame), path, image, duration_ms=2000, frame=frame, diagnostics=info)
        for kind, period in zip(AFFLICTIONS, PERIODS):
            path = 'scenes/cards/overlays/afflictions/'+kind+'.tscn'
            for frame in range(8):
                renderer = _renderer(pack, (104, 146), (324, 454), frame*period/8)
                root = renderer.loader.instantiate(path)
                renderer.visit(root, _translate(162, 227), np.array([0., 0.]), np.ones(4), 0, root.name)
                image, info = renderer.finish()
                add('affliction-'+kind+'-'+str(frame), path, image, sample_time=frame*period/8,
                    sampled_period_ms=round(period*1000), frame=frame, diagnostics=info)
        path = 'scenes/vfx/sovereign_blade.tscn'
        for stage in range(3):
            renderer = _renderer(pack, (32, 96), (500, 1500), .35)
            whole = renderer.loader.instantiate(path)
            root = next(n for n in _walk(whole) if n.name == 'ScaleContainer')
            root = copy.deepcopy(root)
            root.props['scale'] = Property('Vector2(1, 1)', path)
            root.props['rotation'] = Property('0', path)
            root.props['position'] = Property('Vector2(0, 0)', path)
            for node in _walk(root):
                if node is root:
                    continue
                if stage == 2:
                    node.props['visible'] = Property('true' if node.name == 'BladeGlow' else 'false', path)
                    if node.name == 'BladeGlow':
                        node.props['modulate'] = Property('Color(1, 1, 1, 1)', path)
                elif node.name in ('Hilt','Hilt2','Detail','BladeGlow'):
                    # Pinned source checks bladeDamage>=.66 (not normalized
                    # size), so the decoration is already visible at10.
                    visible = node.name == ('Hilt' if stage == 0 else 'Hilt2') or node.name == 'Detail'
                    node.props['visible'] = Property('true' if visible else 'false', path)
            renderer.visit(root, _translate(250, 1135), np.array([0., 0.]), np.ones(4), 0, root.name)
            image, info = renderer.finish()
            add('sovereign-blade-'+str(stage), path, image, stage=stage, diagnostics=info)
        for name, size in (('hammer_mark',(40,40)),('impact_circle_smith',(40,40)),('spark_smith',(12,12))):
            path='images/vfx/vfx_smith/'+name+'.png'
            add('smith-'+name,path,load_scene_texture(pack,path).convert('RGBA').resize(size,Image.Resampling.LANCZOS))
        path='images/vfx/star1.png'
        add('enchantment-spark',path,load_scene_texture(pack,path).convert('RGBA').resize((16,16),Image.Resampling.LANCZOS))
    finally:
        pack.close()
    if BASE_AUXILIARY+len(rows) != AUXILIARY_COUNT:
        raise ValueError('UI14 combat auxiliary extent changed')
    return rows
