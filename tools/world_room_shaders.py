"""Bounded CPU evaluation of four reviewed original room shaders.

Only fingerprints, resource paths and our evaluator are public. Shader source,
scene parameters and texture pixels are read from the owner's pinned PCK. This
is a sampled LDR composition, not an independent Godot/GPU visual oracle.

Generated texture rules follow Godot 4.5.1-stable scene/resources/{curve,
curve_texture,gradient,gradient_texture}.{cpp,h}: CurveXYZTexture samples its
baked curve at i/width; GradientTexture1D samples at i/(width-1), and non-HDR
gradient bytes truncate. See https://github.com/godotengine/godot/tree/4.5.1-stable/scene/resources
The pinned project has no HDR2D/filter/repeat override. Viewport defaults to
LDR, linear filtering and disabled repeat. Normalized framebuffer source colors
are clamped before blending: https://docs.vulkan.org/spec/latest/chapters/framebuffer.html

Limits: CPU bilinear samples have no GPU derivative/mipmap/anisotropic path,
MSAA, GPU precision equivalence or per-draw framebuffer quantization. The
shader images are quantized to RGBA8 before the existing scene compositor.
No procedural GPU hash/noise is substituted: all noise comes from the original
textures or curves. Particle simulation and unrelated shaders remain explicit
omissions. A snapshot time does not establish an animation loop period.
"""
from __future__ import annotations

from collections import OrderedDict
import numpy as np
from PIL import Image

from world_scene_assets import _Renderer, Call, SceneRenderError, _property, _vector


# Historical reference identities describe the source used for these evaluators.
# Dispatch uses the authored resource path; no shader fingerprint gate is needed.
SHADERS = {
    'scenes/rest_site/rest_site_light_shader.gdshader': (
        'light', '468d72309e2f8e2d7ae29458e4d453e094c731b136a5c8fe94d2dbd7c008642e', 0),
    'shaders/vfx/vfx_stepped_shader_fire_flat.tres': (
        'fire', '10365f5ad4c7cf285634a92558b35b4c13f1c2aa188502cef3859a4493dc7198', 0),
    'shaders/vfx/vfx_stepped_shader_fire_add.tres': (
        'fire', '020ed35c9ac3f2b3c5312d36c4fbe98d5d456023270a94c6fd44e2c40aaef721', 1),
    'shaders/vfx/vfx_stepped_shader_fire_dark.tres': (
        'dark_fire', '6fae0954ba6d4ae43947d8ad19be2a2f8a04252e9969b6d5faa0cc0bc16ed537', 0),
}

FIRE_DEFAULTS = dict(
    OuterColor=(1., 0., 0., 1.), InnerColor=(1., .85, 0., 1.),
    InnerColorStep=(.3, .4), OuterStep=(.1, .2),
    Noise1Strength=1., Noise2Strength=1., Noise1Scaling=(1., 1.), Noise2Scaling=(1., 1.),
    Noise1Panning=(1., 1.), Noise2Panning=(1., 1.),
    InvertNoiseMask=True, NoiseMaskScale=(1., 1.), NoiseMaskOffset=(0., 0.),
    Distortion1Scale=(1., 1.), Distortion2Scale=(1., 1.),
    Distortion1Panning=(1., 1.), Distortion2Panning=(1., 1.),
    Distortion1Strength=.02, Distortion2Strength=.02,
    DistortionMaskScale=(1., 1.), DistortionMaskOffset=(0., 0.), GlobalOffset=(0., 0.))
FIRE_TEXTURES = ('Noise1Texture', 'Noise2Texture', 'NoiseMask',
                 'Distortion1Texture', 'Distortion2Texture', 'DistortionMask')


def _finite(value, message):
    value = np.asarray(value, dtype=np.float32)
    if not np.all(np.isfinite(value)):
        raise SceneRenderError(message)
    return value


def sample_texture(pixels, uv, *, repeat=False):
    """Normalized linear sampling, including half-texel clamp/wrap boundaries."""
    uv = _finite(uv, 'Nonfinite room shader UV')
    if pixels.ndim != 3 or min(pixels.shape) < 1 or uv.shape[-1] != 2:
        raise SceneRenderError('Invalid room shader texture or UV shape')
    height, width = pixels.shape[:2]
    uv = uv % 1 if repeat else np.clip(uv, 0., 1.)
    position = uv * np.array([width, height], np.float32) - .5
    low = np.floor(position).astype(np.int64)
    fraction = position - low
    high = low + 1
    if repeat:
        low %= [width, height]
        high %= [width, height]
    else:
        low = np.clip(low, 0, [width - 1, height - 1])
        high = np.clip(high, 0, [width - 1, height - 1])
    x, y = fraction[..., 0:1], fraction[..., 1:2]
    top = pixels[low[..., 1], low[..., 0]] * (1 - x) + pixels[low[..., 1], high[..., 0]] * x
    bottom = pixels[high[..., 1], low[..., 0]] * (1 - x) + pixels[high[..., 1], high[..., 0]] * x
    return np.asarray(top * (1 - y) + bottom * y, dtype=np.float32)


def _sequence(value):
    return value.args if isinstance(value, Call) else value


def curve_channel(loader, ref, width):
    if ref is None:
        return np.zeros(width, np.float32)
    attrs, props = loader.resource(ref)
    if attrs.get('type') != 'Curve':
        raise SceneRenderError('CurveXYZTexture channel must reference a Curve')
    data = _sequence(_property(props, '_data', ()))
    if len(data) % 5 or len(data) > 5 * 4096:
        raise SceneRenderError('Invalid room light curve points')
    points = []
    for i in range(0, len(data), 5):
        xy = _vector(data[i])
        if data[i + 3] != 0 or data[i + 4] != 0:
            raise SceneRenderError('Unreviewed room light curve tangent mode')
        points.append([*xy, float(data[i + 1]), float(data[i + 2])])
    if int(_property(props, 'point_count', len(points))) != len(points):
        raise SceneRenderError('Room light curve point count mismatch')
    if not points:
        return np.zeros(width, np.float32)
    points = _finite(points, 'Nonfinite room light curve')
    if len(points) == 1:
        return np.full(width, points[0, 1], np.float32)
    if np.any(np.diff(points[:, 0]) <= 0):
        raise SceneRenderError('Room light curve domain must increase')
    limits = _vector(_property(props, '_limits', (0., 1., 0., 1.)), 4)
    low, high = limits[2:]
    resolution = int(_property(props, 'bake_resolution', 100))
    if not np.isfinite(high - low) or high <= low or not 2 <= resolution <= 4096:
        raise SceneRenderError('Invalid room light curve bake domain')
    x = np.linspace(low, high, resolution)
    indexes = np.clip(np.searchsorted(points[:, 0], x, side='right') - 1, 0, len(points) - 2)
    a, b = points[indexes], points[indexes + 1]
    span = b[:, 0] - a[:, 0]
    t = np.clip((x - a[:, 0]) / span, 0., 1.)
    c1, c2 = a[:, 1] + span * a[:, 3] / 3, b[:, 1] - span * b[:, 2] / 3
    cache = ((1 - t) ** 3 * a[:, 1] + 3 * (1 - t) ** 2 * t * c1 +
             3 * (1 - t) * t ** 2 * c2 + t ** 3 * b[:, 1]).astype(np.float32)
    # Godot fixes cache endpoints to the first/last authored point values.
    cache[0], cache[-1] = points[0, 1], points[-1, 1]
    return np.interp(np.arange(width) / width, x, cache).astype(np.float32)


def generated_texture(loader, ref):
    """Evaluate the reviewed linear gradient/curve resource classes as floats."""
    attrs, props = loader.resource(ref)
    kind = attrs.get('type')
    width = int(_property(props, 'width', 256))
    if not 2 <= width <= 4096:
        raise SceneRenderError('Invalid generated room texture width')
    if kind == 'CurveXYZTexture':
        return np.stack([curve_channel(loader, _property(props, 'curve_' + axis), width)
                         for axis in 'xyz'], axis=-1)[None, :, :]
    if kind != 'GradientTexture1D':
        raise SceneRenderError('Unreviewed generated room texture: ' + str(kind))
    gradient = _property(props, 'gradient')
    if gradient is None:
        raise SceneRenderError('Room light gradient is missing')
    gradient_attrs, values = loader.resource(gradient)
    if gradient_attrs.get('type') != 'Gradient':
        raise SceneRenderError('GradientTexture1D requires a Gradient')
    if (_property(values, 'interpolation_mode', 0) != 0 or
            _property(values, 'interpolation_color_space', 0) != 0):
        raise SceneRenderError('Only reviewed linear sRGB gradients are supported')
    offsets = _finite(_sequence(_property(values, 'offsets', (0., 1.))), 'Nonfinite gradient offsets')
    colors = _finite(_sequence(_property(values, 'colors', (0., 0., 0., 1., 1., 1., 1., 1.))),
                     'Nonfinite gradient colors')
    if (offsets.ndim != 1 or not 1 <= len(offsets) <= 4096 or
            colors.size != len(offsets) * 4 or np.any(np.diff(offsets) <= 0)):
        raise SceneRenderError('Invalid room light gradient stops')
    colors = colors.reshape(-1, 4)
    positions = np.arange(width) / (width - 1)
    pixels = np.stack([np.interp(positions, offsets, colors[:, channel]) for channel in range(4)], axis=-1)
    if not _property(props, 'use_hdr', False):
        pixels = np.uint8(np.clip(pixels * 255, 0, 255)).astype(np.float32) / 255.
    return np.asarray(pixels[None, :, :], dtype=np.float32)


def smoothstep(edges, value):
    lo, hi = _vector(edges)
    if not np.isfinite(lo + hi) or lo == hi:
        raise SceneRenderError('Undefined equal room shader smoothstep edges')
    # Seven authored underdocks reflections reverse these edges. GPU results
    # are formally undefined there; use the usual polynomial and report it.
    t = np.clip((value - lo) / (hi - lo), 0., 1.)
    return t * t * (3 - 2 * t)


def fire_fragment(main, uv, time, uniforms, textures, *, dark=False):
    """Sample the reviewed color/noise/distortion equations without node tint."""
    def scalar(key):
        return float(uniforms.get(key, FIRE_DEFAULTS[key]))
    def vector(key):
        default = FIRE_DEFAULTS[key]
        return _finite(_vector(uniforms.get(key, default), len(default)), 'Nonfinite fire parameter')
    offset = vector('GlobalOffset') if dark else np.zeros(2, np.float32)
    noise = np.ones(uv.shape[:-1], np.float32)
    distortion = np.zeros_like(uv)
    for index in (1, 2):
        prefix = 'Noise' + str(index)
        at = uv * vector(prefix + 'Scaling') + time * vector(prefix + 'Panning') + offset
        noise *= sample_texture(textures[prefix + 'Texture'], at, repeat=True)[..., 2] * scalar(prefix + 'Strength')
        prefix = 'Distortion' + str(index)
        at = uv * vector(prefix + 'Scale') + time * vector(prefix + 'Panning') + offset
        distortion += sample_texture(textures[prefix + 'Texture'], at, repeat=True)[..., :2] * scalar(prefix + 'Strength')
    anchor = np.array([0., .04], np.float32)
    mask_uv = (uv - anchor) * vector('NoiseMaskScale') + anchor + vector('NoiseMaskOffset')
    mask = sample_texture(textures['NoiseMask'], mask_uv)[..., 2]
    if uniforms.get('InvertNoiseMask', FIRE_DEFAULTS['InvertNoiseMask']):
        mask = 1 - mask
    distortion_mask = sample_texture(textures['DistortionMask'],
        uv * vector('DistortionMaskScale') + vector('DistortionMaskOffset'))[..., :2]
    strength = (noise + mask) * sample_texture(main, uv + distortion * distortion_mask)[..., 2]
    inner = smoothstep(vector('InnerColorStep'), 1 - strength if dark else strength)[..., None]
    outer_color, inner_color = vector('OuterColor')[:3], vector('InnerColor')[:3]
    rgb = outer_color * (inner + inner_color) if dark else outer_color + inner * inner_color
    alpha = smoothstep(vector('OuterStep'), strength)[..., None]
    return _finite(np.concatenate((rgb, alpha), axis=-1), 'Nonfinite fire fragment')


def light_fragment(main, uv, time, noise, ramp, pixel_offset, model_matrix, tint):
    """Curve warp and ramp color; original and distorted alpha are both used."""
    scale = np.linalg.norm(np.asarray(model_matrix)[:2, :2], axis=0)
    if np.any(scale <= 0) or not np.all(np.isfinite(scale)):
        raise SceneRenderError('Degenerate room light model scale')
    offset = np.asarray(pixel_offset) / scale / [main.shape[1], main.shape[0]]
    time_uv = np.array([time * .5, 0.], np.float32)
    distortion = sample_texture(noise, time_uv, repeat=True)[:2]
    if np.any(distortion == 0):
        raise SceneRenderError('Zero room light distortion divisor')
    at = (uv - .5) / distortion + .5 - offset
    original, warped = sample_texture(main, uv), sample_texture(main, at)
    color = sample_texture(ramp, time_uv, repeat=True)[:3]
    rgb = warped[..., :3] * color * np.asarray(tint)[:3]
    alpha = original[..., 3:4] * warped[..., 3:4] * tint[3]
    return _finite(np.concatenate((rgb, alpha), axis=-1), 'Nonfinite light fragment')


def quad_uv(canvas, screen, matrix, rect):
    """Interpolate local quad UV at pixel centers; preserve original geometry."""
    x, y, width, height = rect
    mapping = screen @ matrix
    if width <= 0 or height <= 0 or abs(np.linalg.det(mapping)) < 1e-12:
        return None, None
    yy, xx = np.mgrid[:canvas[1], :canvas[0]]
    points = np.stack((xx + .5, yy + .5, np.ones_like(xx)), axis=-1)
    local = points @ np.linalg.inv(mapping).T
    uv = (local[..., :2] - [x, y]) / [width, height]
    covered = np.all((uv >= 0) & (uv < 1), axis=-1)
    return _finite(uv, 'Nonfinite shader quad'), covered


class RoomRenderer(_Renderer):
    """Same constructor as _Renderer; room_shader_time may explicitly freeze VFX."""
    def _shader_texture(self, ref):
        if ref is None:
            raise SceneRenderError('Required room shader texture is missing')
        if not hasattr(self, '_room_textures'):
            self._room_textures = OrderedDict()
            self._room_texture_bytes = 0
        if ref in self._room_textures:
            self._room_textures.move_to_end(ref)
            return self._room_textures[ref]
        attrs, _ = self.loader.resource(ref)
        if attrs.get('type') in ('CurveXYZTexture', 'GradientTexture1D'):
            pixels = generated_texture(self.loader, ref)
        else:
            pixels = np.asarray(self.texture(ref), np.float32) / 255.
        while self._room_textures and self._room_texture_bytes + pixels.nbytes > 32 * 1024 * 1024:
            _, old = self._room_textures.popitem(last=False)
            self._room_texture_bytes -= old.nbytes
        if pixels.nbytes <= 32 * 1024 * 1024:
            self._room_textures[ref] = pixels
            self._room_texture_bytes += pixels.nbytes
        return pixels

    def shader_layer(self, node, path, image, rect, matrix, tint, shader,
                     material, blend, shader_info):
        if shader not in SHADERS:
            return None
        kind, digest, blend = SHADERS[shader]
        props = node.props
        if (_property(props, 'region_enabled', False) or _property(props, 'hframes', 1) != 1 or
                _property(props, 'vframes', 1) != 1 or
                (node.kind == 'TextureRect' and _property(props, 'stretch_mode', 0) == 1)):
            raise SceneRenderError('Unreviewed room shader cropped/tiled UV geometry: ' + path)
        if (_property(props, 'texture_filter', 0) not in (0, 2) or
                _property(props, 'texture_repeat', 0) not in (0, 1)):
            raise SceneRenderError('Unreviewed room shader texture sampling override: ' + path)
        texture_ref = _property(props, 'texture')
        if texture_ref is not None and self.loader.resource(texture_ref)[0].get('type') == 'AtlasTexture':
            raise SceneRenderError('Unreviewed room shader atlas UV geometry: ' + path)
        uv, covered = quad_uv(self.canvas, self.screen, matrix, rect)
        if uv is None:
            return Image.new('RGBA', self.canvas), blend
        # image_geometry flips pixels for ordinary sprites. A shader instead
        # receives flipped UV, which also affects all of its noise/mask inputs.
        for key, axis, transpose in [('flip_h', 0, Image.Transpose.FLIP_LEFT_RIGHT),
                                     ('flip_v', 1, Image.Transpose.FLIP_TOP_BOTTOM)]:
            if _property(props, key, False):
                image = image.transpose(transpose)
                uv[..., axis] = 1 - uv[..., axis]
        main = np.asarray(image, np.float32) / 255.
        time = float(getattr(self, 'room_shader_time', self.time))
        if not np.isfinite(time):
            raise SceneRenderError('Nonfinite room shader time')
        def parameter(key, default=None):
            return _property(material, 'shader_parameter/' + key, default)
        if kind == 'light':
            pixels = light_fragment(main, uv, time,
                self._shader_texture(parameter('noise_texture')),
                self._shader_texture(parameter('ramp_texture')),
                _vector(parameter('pixel_offset', (0., 0.))), matrix, tint)
        else:
            uniforms = {key: parameter(key, default) for key, default in FIRE_DEFAULTS.items()}
            textures = {key: self._shader_texture(parameter(key)) for key in FIRE_TEXTURES}
            pixels = fire_fragment(main, uv, time, uniforms, textures, dark=kind == 'dark_fire')
            for key in ('InnerColorStep', 'OuterStep'):
                edges = _vector(uniforms[key])
                if edges[0] > edges[1]:
                    self.report('approximations', node, path,
                        reason='authored_reversed_smoothstep_edges_cpu_polynomial_gpu_undefined',
                        parameter=key, edges=edges.tolist())
        pixels[..., 3] *= covered
        result = Image.fromarray(np.uint8(np.clip(np.rint(pixels * 255), 0, 255)))
        self.diagnostics.setdefault('room_shaders', []).append(dict(
            node=path, shader=shader, reference_shader_sha256=digest, kind=kind, blend=blend,
            sample_time=time, nonzero_alpha_pixels=int(np.count_nonzero(np.asarray(result)[..., 3]))))
        self.diagnostics['room_shader_sampling'] = dict(
            framebuffer='default_ldr_unorm', filter='linear_level_zero',
            color_space='default_ldr_srgb_numeric_channels', generated_texture_engine='Godot4.5.1',
            quantization='rgba8_before_existing_scene_compositor',
            limits=['no_gpu_pixel_oracle', 'no_derivative_mip_or_msaa_emulation',
                    'cpu_float_and_filter_precision', 'no_per_draw_framebuffer_quantization',
                    'sample_time_does_not_establish_a_loop'])
        return result, blend
