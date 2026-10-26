"""Bounded offline evaluation of the Architect's authored GPU particle scenes.

This is an independent CPU adapter. Textures, emitter topology, flipbooks, LUTs,
curves, transforms and delays come from the owner's PCK. The presentation seed
does not touch gameplay RNG. Godot's GPU turbulence is the one substituted field:
a deterministic curl field retains its authored strength, scale and life curve.
Canvas screen displacement is retained separately, at integer LCD-pixel precision.

Process equations were checked against the public 4.5.1 engine implementation:
https://github.com/godotengine/godot/blob/4.5.1-stable/scene/resources/particle_process_material.cpp
"""
from __future__ import annotations

from dataclasses import dataclass
import math
from pathlib import PurePosixPath

import numpy as np
from PIL import Image

from world_room_shaders import generated_texture, sample_texture
from world_scene_assets import Call, SceneLoader, _node_matrix, _property, _vector, load_scene_texture


def sequence(value):
    return value.args if isinstance(value, Call) else value


def line(values, unit):
    """The same half-texel clamp/linear lookup used by source CurveTextures."""
    values = np.asarray(values)
    at = np.clip(float(unit)*len(values)-.5, 0, len(values)-1)
    lo = int(at); hi = min(len(values)-1, lo+1)
    return values[lo]*(1-at+lo)+values[hi]*(at-lo)


def smooth(low, width, value):
    t = np.clip((value-low)/max(1e-9, width), 0, 1)
    return t*t*(3-2*t)


def hue(rgb, shift):
    gray = np.array([.57735]*3)
    projection = np.sum(rgb*gray, axis=-1, keepdims=True)*gray
    offset = rgb-projection
    phase = np.asarray(shift)*math.tau
    return projection+offset*np.cos(phase)[..., None]+np.cross(gray, offset)*np.sin(phase)[..., None]


def process_hue(rgb, shift):
    # ParticleProcessMaterial uses a luma-weighted hue matrix. The custom
    # flipbook shader above uses a different gray-axis rotation.
    base = np.array([[.299, .299, .299], [.587, .587, .587], [.114, .114, .114]])
    cosine = np.array([[.701, -.299, -.300], [-.587, .413, -.588], [-.114, -.114, .886]])
    sine = np.array([[.168, -.328, 1.250], [.330, .035, -1.050], [-.497, .292, -.203]])
    return (base+cosine*math.cos(shift*math.tau)+sine*math.sin(shift*math.tau))@rgb


class Resources:
    def __init__(self, pack, loader):
        self.pack, self.loader = pack, loader
        self.textures = {}; self.curves = {}; self.generated = {}

    def texture(self, ref):
        path = self.loader.path(ref)
        if path not in self.textures:
            self.textures[path] = np.asarray(load_scene_texture(self.pack, path).convert('RGBA'), dtype=np.float32)/255
        return self.textures[path]

    def curve(self, ref, default=1.):
        if ref is None:
            return np.full((256, 3), default)
        if ref not in self.curves:
            attrs, props = self.loader.resource(ref)
            kind = attrs.get('type')
            if kind == 'CurveTexture':
                values = self._channel(_property(props, 'curve'))
                result = np.repeat(values[:, None], 3, axis=1)
            elif kind == 'CurveXYZTexture':
                result = np.stack([self._channel(_property(props, 'curve_'+axis)) for axis in 'xyz'], axis=-1)
            else:
                raise ValueError('Unreviewed attack curve: '+str(kind))
            self.curves[ref] = result
        return self.curves[ref]

    def _channel(self, ref):
        if ref is None:
            return np.zeros(256)
        attrs, props = self.loader.resource(ref)
        if attrs.get('type') != 'Curve':
            raise ValueError('Attack curve channel does not name a Curve')
        values = sequence(_property(props, '_data', ()))
        points = []
        for i in range(0, len(values), 5):
            xy = _vector(values[i])
            points.append([*xy, float(values[i+1]), float(values[i+2]), int(values[i+3]), int(values[i+4])])
        if not points:
            return np.zeros(256)
        points = np.asarray(points, dtype=float)
        if len(points) == 1:
            return np.full(256, points[0, 1])
        if np.any(np.diff(points[:, 0]) <= 0) or np.any((points[:, 4:] != 0)&(points[:, 4:] != 1)):
            raise ValueError('Unreviewed attack curve domain/tangent')
        # Godot's LINEAR tangent follows its adjacent segment; FREE retains
        # the authored slope. Both appear in RockShatter/DaggerSpray.
        slopes = np.diff(points[:, 1])/np.diff(points[:, 0])
        for i, point in enumerate(points):
            if point[4] == 1 and i: point[2] = slopes[i-1]
            if point[5] == 1 and i+1 < len(points): point[3] = slopes[i]
        bounds = _vector(_property(props, '_limits', (0., 1., 0., 1.)), 4)
        resolution = int(_property(props, 'bake_resolution', 100))
        x = np.linspace(bounds[2], bounds[3], resolution)
        indexes = np.clip(np.searchsorted(points[:, 0], x, side='right')-1, 0, len(points)-2)
        a, b = points[indexes], points[indexes+1]
        span = b[:, 0]-a[:, 0]; t = np.clip((x-a[:, 0])/span, 0, 1)
        result = ((2*t**3-3*t*t+1)*a[:, 1]+(t**3-2*t*t+t)*span*a[:, 3]+
                  (-2*t**3+3*t*t)*b[:, 1]+(t**3-t*t)*span*b[:, 2])
        result[0], result[-1] = points[0, 1], points[-1, 1]
        return np.interp(np.arange(256)/256, x, result)

    def gradient(self, ref):
        if ref is None:
            return np.ones((256, 4))
        if ref not in self.generated:
            attrs, texture = self.loader.resource(ref)
            if attrs.get('type') != 'GradientTexture1D':
                raise ValueError('Attack gradient requires GradientTexture1D')
            _, gradient = self.loader.resource(_property(texture, 'gradient'))
            mode = int(_property(gradient, 'interpolation_mode', 0))
            if mode == 1 and _property(gradient, 'interpolation_color_space', 0) == 0:
                width = int(_property(texture, 'width', 256))
                offsets = np.asarray(sequence(_property(gradient, 'offsets', (0, 1))), dtype=float)
                colors = np.asarray(sequence(_property(gradient, 'colors')), dtype=float).reshape(-1, 4)
                indexes = np.clip(np.searchsorted(offsets, np.linspace(0, 1, width), side='right')-1, 0, len(offsets)-1)
                result = colors[indexes]
                if not _property(texture, 'use_hdr', False):
                    result = np.uint8(np.clip(result*255, 0, 255)).astype(float)/255
                self.generated[ref] = result
            else:
                self.generated[ref] = generated_texture(self.loader, ref)[0]
        return self.generated[ref]


@dataclass
class SceneItem:
    path: str
    node: object
    matrix: np.ndarray
    tint: np.ndarray
    z: int
    order: int
    visible: bool


def scene_items(root):
    result = []
    def walk(node, path, parent, inherited, z, visible):
        matrix = (_node_matrix(node.props) if _property(node.props, 'top_level', False)
                  else parent @ _node_matrix(node.props))
        modulate = inherited*_vector(_property(node.props, 'modulate', (1, 1, 1, 1)), 4)
        tint = modulate*_vector(_property(node.props, 'self_modulate', (1, 1, 1, 1)), 4)
        z = (z if _property(node.props, 'z_as_relative', True) else 0)+int(_property(node.props, 'z_index', 0))
        visible = visible and bool(_property(node.props, 'visible', True))
        row = SceneItem(path, node, matrix, tint, z, len(result), visible)
        result.append(row)
        for child in node.children:
            walk(child, (path+'/' if path else '')+child.name, matrix, modulate, z, visible)
    walk(root, '', np.eye(3), np.ones(4), 0, True)
    return result


class DrawMaterial:
    SUPPORTED = {'vfx_grayscale_particle_shader', 'vfx_flipbook_shader', 'vfx_row_flipbook_shader',
                 'vfx_heavy_blunt_erosion_shader', 'vfx_ring_polar_shader',
                 'vfx_screen_distortion_outward_shader', 'vfx_poof_shader'}

    def __init__(self, resources, props):
        self.resources = resources; self.props = {}; self.shader = ''; self.blend = 0
        self.flip = (1, 1); self.loop = False
        ref = _property(props, 'material')
        if ref is not None:
            attrs, material = resources.loader.resource(ref)
            if attrs.get('type') == 'ShaderMaterial':
                path = resources.loader.path(_property(material, 'shader'))
                self.shader = PurePosixPath(path).stem
                if self.shader not in self.SUPPORTED:
                    raise ValueError('Unreviewed attack shader: '+path)
                self.props = {k.removeprefix('shader_parameter/'): _property(material, k)
                              for k in material if k.startswith('shader_parameter/')}
                # These seven audited shader families explicitly use blend_mix.
                self.blend = 2 if self.shader == 'vfx_screen_distortion_outward_shader' else 0
            elif attrs.get('type') == 'CanvasItemMaterial':
                self.blend = int(_property(material, 'blend_mode', 0))
                if self.blend not in (0, 1):
                    raise ValueError('Unreviewed CanvasItem particle blend')
                if _property(material, 'particles_animation', False):
                    self.flip = (int(_property(material, 'particles_anim_h_frames', 1)),
                                 int(_property(material, 'particles_anim_v_frames', 1)))
                    self.loop = bool(_property(material, 'particles_anim_loop', True))
            else:
                raise ValueError('Unreviewed attack draw material: '+str(attrs.get('type')))
        self.pivot = _vector(self.props.get('pivot_offset', (0, 0)))
        if 'flipbook' in self.shader:
            self.flip = tuple(int(x) for x in _vector(self.props.get('flipbook_size', (3, 3))))
        self.lut = resources.gradient(self.props.get('lut'))

    def curve(self, key, unit, default=0.):
        return float(line(self.resources.curve(self.props.get(key), default), unit)[0])

    def fragment(self, texture, uv, age, animation, color, time):
        shader = self.shader
        coord = uv.copy()
        if self.flip != (1, 1):
            cols, rows = self.flip; total = cols*rows
            if shader == 'vfx_row_flipbook_shader':
                progress = self.curve('flipbook_curve', age)
                frame = math.floor((math.floor(animation*rows)/rows+
                                    math.floor(progress*cols)/cols/rows)*float(self.props.get('frame_count', total)))
            elif shader:
                progress = self.curve('flipbook_curve', age)
                frame = math.floor(float(self.props.get('frame_count', total))*(animation+progress)*.99)
            else:
                frame = math.floor(animation*total)
            frame = frame % total if shader or self.loop else min(total-1, max(0, frame))
            coord = uv/[cols, rows]+[frame % cols/cols, frame//cols/rows]
        if shader == 'vfx_ring_polar_shader':
            delta = uv-.5
            coord = np.stack((np.linalg.norm(delta, axis=-1)*2,
                              np.arctan2(delta[..., 1], delta[..., 0])/(3.1416*2)), axis=-1)
        elif shader == 'vfx_poof_shader':
            pan = _vector(self.props.get('distortion_panning_speed', (0, 0)))
            noise = sample_texture(texture, uv+time*pan, repeat=True)[..., 2:3]
            coord = uv+noise*float(self.props.get('distortion_strength', 0.))
        pixel = sample_texture(texture, coord, repeat=shader in ('vfx_ring_polar_shader', 'vfx_poof_shader'))
        if not shader:
            return pixel*color
        if self.blend == 2:
            direction = .5-uv
            direction = np.divide(direction, np.linalg.norm(direction, axis=-1, keepdims=True),
                                  out=np.zeros_like(direction), where=np.linalg.norm(direction, axis=-1, keepdims=True)>1e-9)
            return direction*pixel[..., 0:1]*color[3]*float(self.props.get('distortion_base_intensity', .05))*[320, 240]
        red = pixel[..., 0]
        lut = sample_texture(self.lut[None, :, :], np.stack((red, red), axis=-1))
        result = lut*color
        if shader == 'vfx_grayscale_particle_shader':
            result[..., 3] = red*color[3]
        elif shader == 'vfx_ring_polar_shader':
            result[..., :3] = color[:3]
            result[..., 3] = smooth(self.curve('erosion_curve', age), float(self.props.get('erosion_offset', .1)), red)*color[3]
        elif shader in ('vfx_heavy_blunt_erosion_shader', 'vfx_poof_shader'):
            result[..., 3] = smooth(self.curve('erosion_curve', age), float(self.props.get('erosion_offset', .1)), red)*color[3]
        else:
            result[..., 3] = pixel[..., 3]*color[3]
            if self.props.get('use_erosion_texture', 0):
                erosion = self.resources.texture(self.props['erosion_texture'])
                values = sample_texture(erosion, uv)[..., 0]
                result[..., 3] *= smooth(self.curve('erosion_curve', age), float(self.props.get('erosion_offset', .1)), values)
            if self.props.get('use_hue_shift', 0):
                curve = self.resources.curve(self.props.get('hue_shift_curve'), 0.)
                shifts = sample_texture(curve[None, :, :], np.stack((red, red), axis=-1))[..., 0]
                shifted = hue(result[..., :3], shifts)
                mix = float(self.props['use_hue_shift'])
                result[..., :3] = result[..., :3]*(1-mix)+shifted*mix
        return result


class Emitter:
    """One original emitter; child systems retain their actual capacities."""
    RANDOM_VALUES = {'scale': 1., 'angle': 0., 'angular_velocity': 0., 'initial_velocity': 0.,
                     'linear_accel': 0., 'radial_accel': 0., 'radial_velocity': 0.,
                     'directional_velocity': 0., 'damping': 0., 'orbit_velocity': 0.,
                     'hue_variation': 0., 'anim_offset': 0., 'anim_speed': 0.,
                     'turbulence_influence': .1, 'turbulence_initial_displacement': 0.}

    def __init__(self, resources, item, index, start=0., sub_only=False):
        self.resources, self.item = resources, item
        self.node = item.node; props = item.node.props
        attrs, self.material = resources.loader.resource(_property(props, 'process_material'))
        if attrs.get('type') != 'ParticleProcessMaterial':
            raise ValueError('Attack emitter requires ParticleProcessMaterial')
        self.draw = DrawMaterial(resources, props)
        self.texture = resources.texture(_property(props, 'texture'))
        self.lifetime = float(_property(props, 'lifetime', 1.))
        self.speed = float(_property(props, 'speed_scale', 1.))
        self.amount = int(_property(props, 'amount', 8))
        self.rng = np.random.default_rng(0x41cef1ea+index*31337)
        self.parameters = {name: resources.curve(_property(self.material, name+'_curve'))
                           for name in ('scale', 'angle', 'angular_velocity', 'linear_accel', 'radial_accel',
                                        'radial_velocity', 'directional_velocity', 'damping', 'orbit_velocity',
                                        'anim_offset', 'anim_speed', 'alpha', 'emission', 'hue_variation')}
        self.parameters['turbulence'] = resources.curve(_property(self.material, 'turbulence_influence_over_life'))
        self.color_ramp = resources.gradient(_property(self.material, 'color_ramp'))
        self.color_initial = resources.gradient(_property(self.material, 'color_initial_ramp'))
        self.scale_velocity = resources.curve(_property(self.material, 'scale_over_velocity_curve'))
        self.child = None; self.live = []; self.spawned = 0; self.capacity_rejected = 0
        self.colliders = []
        self.time = start; self.start = start
        self.trails = bool(_property(props, 'trail_enabled', False))
        self.trail_lifetime = float(_property(props, 'trail_lifetime', .3))
        self.trail_sections = int(_property(props, 'trail_sections', 8))*int(_property(props, 'trail_section_subdivisions', 4))
        self.sub_only = sub_only
        explosive = float(_property(props, 'explosiveness', 0.))
        random = float(_property(props, 'randomness', 0.))
        self.pending = [] if sub_only else [start+(i+self.rng.random()*random)/self.amount*(1-explosive)*self.lifetime/self.speed
                                            for i in range(self.amount)]
        self.pending.sort()

    def prop(self, key, default=None):
        return _property(self.material, key, default)

    def cv(self, name, age):
        return line(self.parameters[name], age)

    def random(self, name, default):
        low = float(self.prop(name+'_min', default)); high = float(self.prop(name+'_max', default))
        return low+(high-low)*self.rng.random()

    def spawn(self, time, world=None, inherited=None):
        if len(self.live) >= self.amount:
            # The renderer accepts a subparticle only in an inactive slot.
            # Replacing an active oldest particle would suppress its AT_END
            # spark, changing the source DaggerSpray emitter chain.
            self.capacity_rejected += 1
            return
        values = {key: self.random(key, default) for key, default in self.RANDOM_VALUES.items()}
        origin = np.zeros(2)
        shape = int(self.prop('emission_shape', 0)); angle = self.rng.random()*math.tau
        if shape in (1, 2, 6):
            radius = float(self.prop('emission_sphere_radius', 1.))
            if shape == 6:
                radius = float(self.prop('emission_ring_radius', 1.))
                inner = float(self.prop('emission_ring_inner_radius', 0.))
                radius = math.sqrt(inner*inner+self.rng.random()*(radius*radius-inner*inner))
            else:
                latitude = self.rng.random()*2-1
                radius *= math.sqrt(1-latitude*latitude)
                if shape == 1: radius *= self.rng.random()
            origin = np.array([math.cos(angle), math.sin(angle)])*radius
        elif shape != 0:
            raise ValueError('Unreviewed attack emission shape '+str(shape))
        origin *= _vector(self.prop('emission_shape_scale', (1, 1, 1)), 3)[:2]
        if world is not None:
            origin = (np.linalg.inv(self.item.matrix)@np.r_[world, 1.])[:2]
        direction = _vector(self.prop('direction', (1, 0, 0)), 3)
        angle = math.atan2(direction[1], direction[0])+(self.rng.random()*2-1)*math.radians(float(self.prop('spread', 45)))
        velocity = np.array([math.cos(angle), math.sin(angle)])*values['initial_velocity']
        if inherited is not None:
            velocity = np.linalg.inv(self.item.matrix[:2, :2])@inherited
        life = self.lifetime*(1-self.rng.random()*float(self.prop('lifetime_randomness', 0.)))
        color = _vector(self.prop('color', (1, 1, 1, 1)), 4)*line(self.color_initial, self.rng.random())
        p = dict(values=values, birth=time, age=0., life=life, position=origin.copy(), velocity=velocity,
                 final_velocity=velocity.copy(),
                 color=color, angle=math.radians(values['angle']), animation=values['anim_offset'],
                 next_child=1/max(.001, float(self.prop('sub_emitter_frequency', 4.))), history=[])
        self.live.append(p); self.spawned += 1
        self._remember(p)

    def _remember(self, particle):
        if self.trails:
            history = particle['history']
            history.append((particle['age'], particle['position'].copy()))
            while len(history)>2 and history[1][0] < particle['age']-self.trail_lifetime:
                history.pop(0)

    def _sub(self, p, time, count=1):
        if self.child is None:
            return
        position = (self.item.matrix@np.r_[p['position'], 1.])[:2]
        velocity = self.item.matrix[:2, :2]@p['velocity'] if self.prop('sub_emitter_keep_velocity', False) else None
        for _ in range(count):
            self.child.spawn(time, position, velocity)

    def collide(self, particle):
        if not self.colliders or not int(self.prop('collision_mode', 0)):
            return
        radius = float(_property(self.node.props, 'collision_base_size', 1.))*(self.texture.shape[0]+self.texture.shape[1])/4
        if self.prop('collision_use_scale', False): radius *= abs(particle['values']['scale'])
        for polygon in self.colliders:
            for index, start in enumerate(polygon):
                delta = polygon[(index+1)%len(polygon)]-start
                squared = float(delta@delta)
                if squared < 1e-8: continue
                relative = particle['position']-start
                point = start+delta*np.clip(float(relative@delta)/squared, 0, 1)
                offset = particle['position']-point; distance = np.linalg.norm(offset)
                if not 1e-9 < distance < radius: continue
                normal = offset/distance
                response = float(normal@particle['velocity'])
                if response >= 0: continue
                particle['position'] += normal*(radius-distance)
                friction = float(self.prop('collision_friction', 0.))
                bounce = float(self.prop('collision_bounce', 0.))
                tangent = particle['velocity']-normal*response
                trigger = abs(response) >= 2/min(2., max(1., bounce+1))
                particle['velocity'] = tangent*(1-friction)-normal*response*bounce*trigger
                particle['final_velocity'] = particle['velocity'].copy()

    def advance(self, time):
        previous = self.time
        while self.pending and self.pending[0] <= time+1e-9:
            self.spawn(self.pending.pop(0))
        alive = []
        gravity = _vector(self.prop('gravity', (0, 98, 0)), 3)[:2]
        pivot = _vector(self.prop('velocity_pivot', (0, 0, 0)), 3)[:2]
        for p in self.live:
            dt = max(0., time-max(previous, p['birth']))*self.speed
            if not dt:
                alive.append(p); continue
            dt = min(dt, max(0., p['life']-p['age']))
            p['age'] += dt; age = min(1., p['age']/p['life']); values = p['values']
            velocity = p['velocity']; speed = np.linalg.norm(velocity)
            if speed > 1e-9:
                velocity += velocity/speed*values['linear_accel']*self.cv('linear_accel', age)[0]*dt
            radial = p['position']-pivot; length = np.linalg.norm(radial)
            if length > .01:
                radial /= length
            else:
                angle = self.rng.random()*math.tau
                radial = np.array([math.cos(angle), math.sin(angle)])
            velocity += (gravity+radial*values['radial_accel']*self.cv('radial_accel', age)[0])*dt
            speed = np.linalg.norm(velocity)
            damping = values['damping']*self.cv('damping', age)[0]
            if speed > 1e-9:
                reduction = speed*speed*damping*.05 if self.prop('particle_flag_damping_as_friction', False) else damping
                velocity *= max(0., speed-reduction*dt)/speed
            animated = (radial*values['radial_velocity']*self.cv('radial_velocity', age)[0]+
                        values['directional_velocity']*self.cv('directional_velocity', age)[:2])
            if self.prop('turbulence_enabled', False):
                influence = float(np.clip(values['turbulence_influence']*self.cv('turbulence', age)[0], 0., 1.))
                strength = float(self.prop('turbulence_noise_strength', 1.))
                scale = max(.001, float(self.prop('turbulence_noise_scale', 9.)))
                pan = _vector(self.prop('turbulence_noise_speed', (0, 0, 0)), 3)[:2]
                position = p['position']/(scale*10)+pan*time
                curl = np.array([math.sin(position[1]+time*1.625*strength), -math.sin(position[0]+time*strength)])
                curl /= max(1e-9, np.linalg.norm(curl))
                # Keep the engine's bounded direction blend and speed response;
                # only its GPU noise vector is replaced by the CPU curl field.
                boost = 1+(1-influence)*.2
                velocity[:] = velocity*(1-influence)+curl*np.linalg.norm(velocity)*boost*influence
                animated = animated*(1-influence)+curl*np.linalg.norm(animated)*boost*influence
            p['final_velocity'] = velocity+animated
            p['position'] += p['final_velocity']*dt
            self.collide(p)
            rotation = values['orbit_velocity']*self.cv('orbit_velocity', age)[0]*dt*math.tau
            if rotation:
                sn, cs = math.sin(rotation), math.cos(rotation); offset = p['position']-pivot
                p['position'] = pivot+np.array([cs*offset[0]-sn*offset[1], sn*offset[0]+cs*offset[1]])
            p['angle'] = math.radians(values['angle']*self.cv('angle', age)[0]+
                                      p['age']*values['angular_velocity']*self.cv('angular_velocity', age)[0])
            offset_curve = float(self.cv('anim_offset', age)[0]) if self.prop('anim_offset_curve') is not None else 0.
            p['animation'] = values['anim_offset']+offset_curve+age*values['anim_speed']*self.cv('anim_speed', age)[0]
            self._remember(p)
            mode = int(self.prop('sub_emitter_mode', 0))
            if mode == 1:
                interval = 1/max(.001, float(self.prop('sub_emitter_frequency', 4.)))
                while p['age']+1e-9 >= p['next_child']:
                    self._sub(p, time); p['next_child'] += interval
            if p['age'] >= p['life']-1e-8:
                if mode == 2:
                    self._sub(p, time, int(self.prop('sub_emitter_amount_at_end', 1)))
            else:
                alive.append(p)
        self.live = alive; self.time = time

    def particles(self):
        if not self.item.visible:
            return
        for p in self.live:
            age = p['age']/p['life']; values = p['values']
            size = values['scale']*self.cv('scale', age)[:2]
            if self.prop('scale_over_velocity_curve') is not None:
                low = float(self.prop('scale_over_velocity_min', 0.)); high = float(self.prop('scale_over_velocity_max', 0.))
                at = np.linalg.norm(p['final_velocity'])/max(1e-9, high-low)
                size *= line(self.scale_velocity, at)[:2]
            if np.any(np.abs(size)<1e-8):
                continue
            angle = -p['angle']
            if self.prop('particle_flag_align_y', False) and np.linalg.norm(p['final_velocity']) > 1e-9:
                angle = math.atan2(p['final_velocity'][1], p['final_velocity'][0])-math.pi/2
            sn, cs = math.sin(angle), math.cos(angle)
            matrix = self.item.matrix[:2, :2]@np.array([[cs*size[0], -sn*size[1]], [sn*size[0], cs*size[1]]])
            position = (self.item.matrix@np.r_[p['position'], 1.])[:2]
            color = p['color']*line(self.color_ramp, age)*self.item.tint
            color[3] *= self.cv('alpha', age)[0]
            if self.prop('emission_curve') is not None:
                color[:3] *= 1+self.cv('emission', age)[0]
            if values['hue_variation']:
                color[:3] = process_hue(color[:3], values['hue_variation']*self.cv('hue_variation', age)[0])
            if not self.trails:
                yield dict(center=position, matrix=matrix, color=color, texture=self.texture,
                           material=self.draw, age=age, animation=p['animation'])
            if self.trails and len(p['history'])>1:
                # GPUParticles2D builds one strip instead of a billboard. Its
                # v coordinate spans the complete trail; never repeat a whole
                # texture per segment or invent an extra opacity fade.
                history = p['history'][-self.trail_sections:]
                for i in range(len(history)-1):
                    a, b = history[i][1], history[i+1][1]
                    delta = b-a; length = np.linalg.norm(delta)
                    if length < 1e-6: continue
                    axis = delta/length; half = (a+b)/2
                    ribbon = self.item.matrix[:2, :2]@np.column_stack((np.array([-axis[1], axis[0]])*size[0],
                                                                 axis*length/self.texture.shape[0]))
                    first = (p['age']-history[i][0])/self.trail_lifetime
                    last = (p['age']-history[i+1][0])/self.trail_lifetime
                    yield dict(center=(self.item.matrix@np.r_[half, 1.])[:2], matrix=ribbon,
                               color=color, texture=self.texture, material=self.draw, age=age,
                               animation=p['animation'], uv_offset=(0, first), uv_scale=(1, last-first))


class Planes:
    def __init__(self, size=(320, 240), origin=(160, 120), density=4/21):
        self.size, self.origin, self.density = size, np.asarray(origin), density
        self.normal = np.zeros((size[1], size[0], 4), dtype=np.float32)
        self.additive = np.zeros((size[1], size[0], 3), dtype=np.float32)
        self.displacement = np.zeros((size[1], size[0], 2), dtype=np.float32)

    def stamp(self, particle, time=0.):
        texture = particle['texture']; material = particle['material']
        matrix = particle['matrix']*self.density
        center = particle['center']*self.density+self.origin
        # Godot ShaderMaterial flipbooks change UV only: their vertex quad
        # retains the full atlas size. CanvasItemMaterial divides quad size.
        extent = np.array([texture.shape[1], texture.shape[0]], dtype=float)
        if material.flip != (1, 1) and not material.shader:
            extent /= material.flip
        center += matrix@(material.pivot*extent)
        corners = (np.array([[-.5, -.5], [.5, -.5], [.5, .5], [-.5, .5]])*extent)@matrix.T+center
        low = np.maximum((0, 0), np.floor(corners.min(axis=0)).astype(int))
        high = np.minimum(self.size, np.ceil(corners.max(axis=0)).astype(int))
        if np.any(high <= low) or abs(np.linalg.det(matrix))<1e-9:
            return
        yy, xx = np.mgrid[low[1]:high[1], low[0]:high[0]]
        uv = (np.stack((xx+.5-center[0], yy+.5-center[1]), axis=-1)@np.linalg.inv(matrix).T)/extent+.5
        inside = np.all((uv>=0)&(uv<=1), axis=-1)
        if 'uv_offset' in particle:
            uv = uv*np.asarray(particle['uv_scale'])+np.asarray(particle['uv_offset'])
        pixel = material.fragment(texture, uv, particle.get('age', 0.), particle.get('animation', 0.), particle['color'], time)
        target = np.s_[low[1]:high[1], low[0]:high[0]]
        if material.blend == 2:
            # Encode the same integer displacement used for the already drawn
            # source layers, so the runtime background and baked local ink agree.
            shift = np.rint(pixel)*inside[..., None]
            at = np.stack((xx+shift[..., 0], yy+shift[..., 1]), axis=-1)
            sx = np.clip(at[..., 0].astype(int), 0, self.size[0]-1)
            sy = np.clip(at[..., 1].astype(int), 0, self.size[1]-1)
            self.normal[target] = self.normal[sy, sx].copy()
            self.additive[target] = self.additive[sy, sx].copy()
            self.displacement[target] = shift
        else:
            alpha = np.clip(pixel[..., 3:4], 0, 1)*inside[..., None]
            color = np.clip(pixel[..., :3], 0, None)
            if material.blend == 1:
                self.additive[target] += color*alpha
            else:
                self.normal[target][..., :3] = color*alpha+self.normal[target][..., :3]*(1-alpha)
                self.normal[target][..., 3:4] = alpha+self.normal[target][..., 3:4]*(1-alpha)
                self.additive[target] *= 1-alpha

    def images(self):
        straight = self.normal.copy()
        straight[..., :3] = np.divide(straight[..., :3], straight[..., 3:4],
                                      out=np.zeros_like(straight[..., :3]), where=straight[..., 3:4]>0)
        normal = Image.fromarray(np.uint8(np.clip(straight*255, 0, 255)))
        add = np.zeros_like(straight); add[..., :3] = self.additive
        add[..., 3] = np.any(self.additive>0, axis=-1)
        additive = Image.fromarray(np.uint8(np.clip(add*255, 0, 255)))
        warp = np.zeros_like(straight)
        warp[..., 0] = np.clip(self.displacement[..., 0]+16, 0, 31)/31
        warp[..., 1] = np.clip(self.displacement[..., 1]+32, 0, 63)/63
        warp[..., 3] = np.any(self.displacement != 0, axis=-1)
        # Round to nearest expands the intended 5/6-bit channel values exactly
        # through RGBA->RGB565 (truncating here would corrupt some G6 values).
        displacement = Image.fromarray(np.uint8(np.clip(np.rint(warp*255), 0, 255)))
        return {0: normal, 1: additive, 2: displacement}


def particle_systems(pack, scene, *, effect_index=0):
    loader = SceneLoader(pack); root = loader.instantiate(scene)
    resources = Resources(pack, loader); items = scene_items(root)
    particles = {row.path: row for row in items if row.node.kind == 'GPUParticles2D'}
    subpaths = {}
    for path, row in particles.items():
        child = _property(row.node.props, 'sub_emitter')
        if child:
            name = str(sequence(child)[0])
            target = str(PurePosixPath(path)/name)
            if target not in particles:
                raise ValueError('Missing original attack subemitter '+target)
            subpaths[path] = target
    impact = {str(sequence(value)[0]) for value in _property(root.props, '_impactParticles', ())}
    starts = {path: .2 if path in impact and scene.endswith('vfx_heavy_blunt.tscn') else 0. for path in particles}
    modulated = {str(sequence(value)[0]) for value in _property(root.props, '_modulateParticles', ())}
    systems = {}
    for index, (path, row) in enumerate(particles.items()):
        if scene.endswith('vfx_fire_burst.tscn') and path in modulated:
            row.tint = np.array([1., 139/255, 87/255, 1.])
        systems[path] = Emitter(resources, row, effect_index*100+index, starts[path], path in subpaths.values())
    for path, target in subpaths.items():
        systems[path].child = systems[target]
    # RockShatter's authored invisible LightOccluder is its SDF floor. Keep
    # its polygon/collision radius/bounce instead of letting debris fall away.
    occluders = []
    for item in items:
        if item.node.kind == 'LightOccluder2D' and _property(item.node.props, 'sdf_collision', True):
            _, props = loader.resource(_property(item.node.props, 'occluder'))
            polygon = np.asarray(sequence(_property(props, 'polygon')), dtype=float).reshape(-1, 2)
            occluders.append((item.matrix@np.column_stack((polygon, np.ones(len(polygon)))).T).T)
    for emitter in systems.values():
        if emitter.prop('collision_mode', 0):
            emitter.colliders = [(np.linalg.inv(emitter.item.matrix)@polygon.T).T[:, :2] for polygon in occluders]
    # A child's show_behind_parent precedes its ancestor's canvas item, not
    # merely the next sibling. Preserve that for the authored dagger trail.
    order = []
    def canvas_order(node, path=''):
        def child_path(child): return (path+'/' if path else '')+child.name
        for child in node.children:
            if _property(child.props, 'show_behind_parent', False): canvas_order(child, child_path(child))
        order.append(path)
        for child in node.children:
            if not _property(child.props, 'show_behind_parent', False): canvas_order(child, child_path(child))
    canvas_order(root)
    ranks = {path: index for index, path in enumerate(order)}
    ordered = sorted(systems.values(), key=lambda e: (e.item.z, ranks[e.item.path]))
    return systems, ordered


def sample_particle_frame(systems, ordered, time, *, previous=0.):
    clock = previous
    while clock < time-1e-9:
        clock = min(time, clock+1/60)
        for emitter in systems.values():
            if clock >= emitter.start:
                emitter.advance(clock)
    if time == 0:
        for emitter in systems.values():
            if emitter.start == 0:
                emitter.advance(0)
    out = Planes()
    for emitter in ordered:
        for particle in emitter.particles():
            out.stamp(particle, time)
    return out.images()
