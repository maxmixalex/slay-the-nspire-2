"""Evaluate reviewed slot materials at the original interpolated Spine UV.

Scene parameters, shader textures and skeletons are read from the owner's PCK.
The actor rasterizer invokes a fragment sampler after triangle interpolation;
there is no intermediate shader bitmap or change to actor sample density.
"""
from __future__ import annotations

import numpy as np

from world_scene_assets import SceneLoader, SceneRenderError, _property, _vector
from world_room_shaders import sample_texture, smoothstep


class NecrobinderSlash:
    """NNecrobinderVfx's attack event drives the authored slash slot shader."""
    def __init__(self, pack, events):
        from build_content import texture
        loader = SceneLoader(pack)
        root = loader.instantiate('scenes/creature_visuals/necrobinder.tscn')
        spine = next(child for child in root.children if child.name == 'Visuals')
        slot = next(child for child in spine.children
                    if child.kind == 'SpineSlotNode' and _property(child.props, 'slot_name') == 'slash')
        attrs, props = loader.resource(_property(slot.props, 'normal_material'))
        if attrs.get('type') != 'ShaderMaterial' or loader.path(_property(props, 'shader')) != 'images/vfx/slash_shader_flat.tres':
            raise SceneRenderError('Unreviewed Necrobinder slash material')
        self.parameters = {key.removeprefix('shader_parameter/'): _property(props, key) for key in props
                           if key.startswith('shader_parameter/')}
        self.textures = {}
        for key in ('shape_mask_tex', 'noise_1_tex', 'noise_2_tex'):
            ref = self.parameters.get(key)
            # An unbound Godot sampler2D supplies white. This uniform is not
            # TEXTURE: the atlas placeholder's printed RGB/alpha is unused.
            self.textures[key] = (np.asarray(texture(pack, loader.path(ref)).convert('RGBA'), np.float32) / 255
                                  if ref else np.ones((1, 1, 4), np.float32))
        starts = [float(row['time']) for row in events if row['name'] == 'attack_slash_start']
        if len(starts) != 1:
            raise SceneRenderError('Necrobinder attack must have one slash event')
        self.started = starts[0]
        self.time = 0.

    @staticmethod
    def tween(age):
        """Pinned Quad/In master step and Quad/InOut opacity callbacks."""
        step = min(1., max(0., age / .4))
        fade = min(1., max(0., age / .45))
        fade = 2 * fade * fade if fade < .5 else 1 - (-2 * fade + 2) ** 2 / 2
        return .04 + (.235 - .04) * step * step, .4 * (1 - fade)

    def at(self, time):
        self.time = float(time)
        return self

    def __call__(self, uv):
        parameters = self.parameters
        if self.time < self.started:
            master = float(parameters.get('master_step', 1.))
            opacity = float(parameters.get('opacity', 1.))
        else:
            master, opacity = self.tween(self.time - self.started)
        value = sample_texture(self.textures['shape_mask_tex'], uv)[..., 2]
        pivot = np.array([-.275, 0.], np.float32)
        for index in (1, 2):
            prefix = f'noise_{index}_'
            scale = np.asarray(_vector(parameters.get(prefix + 'scale', (1., 1.))), np.float32)
            pan = np.asarray(_vector(parameters.get(prefix + 'panning', (0., 0.))), np.float32)
            at = (uv - pivot) * scale + pivot + self.time * pan
            value *= sample_texture(self.textures[prefix + 'tex'], at, repeat=True)[..., 0] * float(parameters.get(prefix + 'strength', 1.))
        edges = np.asarray(_vector(parameters['step']), np.float32) * master
        alpha = smoothstep(edges, value) * opacity
        color = np.asarray(_vector(parameters['ColorParameter'], 4), np.float32)[:3]
        result = np.empty((*uv.shape[:-1], 4), np.float32)
        result[..., :3] = np.clip(color, 0., 1.)
        result[..., 3] = np.clip(alpha, 0., 1.)
        return result


def apply_slot_materials(frame, draw, slash):
    """Preserve source draw order, clipping geometry and atlas-coordinate UVs."""
    if slash is None:
        return draw
    result = []
    for source, layer in zip(frame['draw'], draw):
        if source['slot'] == 'slash':
            if source['blend'] != 0:
                raise SceneRenderError('Necrobinder slash changed its material blend')
            vertices, uv, triangles, _, _, _ = layer
            result.append((vertices, uv, triangles, slash.at(frame['time']), np.ones(4), 0))
        else:
            result.append(layer)
    return result
