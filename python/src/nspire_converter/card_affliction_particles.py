"""Source particle samples for the seven card materials.

The source Godot 4.5.1 process equations run on the desktop. The calculator
interpolates no turbulence and owns no additional particle pool. A deterministic
seed bank gives the authored number of particles their own lifetime/appearance;
it does not claim to recover the original GPU's unknown random history.
"""

from __future__ import annotations

import math
import struct

import numpy as np

from nspire_converter import content_tiles as t
from nspire_converter.ancient_turbulence import FLOAT_P, helper
from nspire_converter.world_room_shaders import _sequence, sample_texture
from nspire_converter.world_scene_assets import _property, _vector


def seed_hash(value):
    value = ((value >> 16) ^ value) * 73244475 & 0xFFFFFFFF
    value = ((value >> 16) ^ value) * 73244475 & 0xFFFFFFFF
    return (value >> 16) ^ value


class Random:
    def __init__(self, value):
        self.value = seed_hash(value)

    def __call__(self):
        s = self.value if self.value < 0x80000000 else self.value - 0x100000000
        if not s:
            s = 305420679
        k = math.trunc(s / 127773)
        s = (16807 * (s - k * 127773) - 2836 * k) & 0xFFFFFFFF
        if s & 0x80000000:
            s -= 0x100000000
        if s < 0:
            s += 2147483647
        self.value = s
        return np.float32((s % 65536) / 65535)


def curve_channel(loader, ref, width):
    if ref is None:
        return np.zeros(width, np.float32)
    attrs, props = loader.resource(ref)
    t.require(attrs.get("type") == "Curve", "Particle curve class changed")
    data = _sequence(_property(props, "_data", ()))
    t.require(len(data) % 5 == 0 and len(data) <= 20480, "Particle curve extent")
    points = []
    for i in range(0, len(data), 5):
        t.require(data[i + 3] in (0, 1) and data[i + 4] in (0, 1), "Particle tangent mode")
        points.append(
            [*_vector(data[i]), float(data[i + 1]), float(data[i + 2]), data[i + 3], data[i + 4]]
        )
    if not points:
        return np.zeros(width, np.float32)
    points = np.asarray(points, dtype="f8")
    if len(points) == 1:
        return np.full(width, points[0, 1], np.float32)
    t.require(np.all(np.diff(points[:, 0]) > 0), "Particle curve order")
    # Curve::TANGENT_LINEAR uses the neighbouring point's secant; free
    # tangents retain their authored derivative. Godot bakes the cubic first.
    slopes = np.diff(points[:, 1]) / np.diff(points[:, 0])
    for i in range(len(points)):
        if points[i, 4] == 1:
            points[i, 2] = slopes[max(0, i - 1)]
        if points[i, 5] == 1:
            points[i, 3] = slopes[min(len(slopes) - 1, i)]
    limits = _vector(_property(props, "_limits", (0, 1, 0, 1)), 4)
    low, high = limits[2:]
    resolution = int(_property(props, "bake_resolution", 100))
    t.require(high > low and 2 <= resolution <= 4096, "Particle curve bake domain")
    x = np.linspace(low, high, resolution)
    indexes = np.clip(np.searchsorted(points[:, 0], x, side="right") - 1, 0, len(points) - 2)
    a, b = points[indexes], points[indexes + 1]
    span = b[:, 0] - a[:, 0]
    p = np.clip((x - a[:, 0]) / span, 0, 1)
    c1, c2 = a[:, 1] + span * a[:, 3] / 3, b[:, 1] - span * b[:, 2] / 3
    cache = (
        (1 - p) ** 3 * a[:, 1]
        + 3 * (1 - p) ** 2 * p * c1
        + 3 * (1 - p) * p * p * c2
        + p**3 * b[:, 1]
    ).astype("f4")
    cache[0], cache[-1] = points[0, 1], points[-1, 1]
    return np.interp(np.arange(width) / width, x, cache).astype("f4")


def sample_curve(renderer, props, name, default=1.0):
    ref = _property(props, name)
    if ref is None:
        return np.full((1, 256, 1), default, np.float32)
    attrs, values = renderer.loader.resource(ref)
    if attrs.get("type") == "CurveTexture":
        return curve_channel(renderer.loader, _property(values, "curve"), 256)[None, :, None]
    if attrs.get("type") == "CurveXYZTexture":
        return np.stack(
            [
                curve_channel(renderer.loader, _property(values, "curve_" + axis), 256)
                for axis in "xyz"
            ],
            axis=-1,
        )[None, :, :]
    return renderer._shader_texture(ref)


def sample(values, phase):
    return sample_texture(values, np.array((phase, 0), np.float32))


def normalized(value):
    length = float(np.linalg.norm(value))
    return value / length if length else np.zeros_like(value)


def texture_fields(bank, renderer, node, row):
    """Keep every source texel; extract only channels the source shader uses."""
    texture_ref = _property(node.props, "texture")
    image = renderer.texture(texture_ref).convert("RGBA")
    a = np.asarray(image, "u1")
    shader, material, blend, _ = renderer.material(node)
    renderer.remember(shader)
    row["texture_size"] = list(image.size)
    row["fields"] = [-1] * 4
    row["curves"] = [-1] * 2
    row["parameters"] = []
    if shader.endswith("entangled_leaf_shader.gdshader"):
        t.require(a.shape == (128, 128, 4), "Source leaf texture dimensions changed")
        row["kind"] = 8
        row["fields"][0] = bank.field(a[:, :, (0, 2, 3)])
        row["curves"][0] = bank.curve(
            renderer._shader_texture(_property(material, "shader_parameter/lut"))
        )
        t.require(
            np.allclose(_vector(_property(material, "shader_parameter/pivot_offset")), (0, -0.3))
            and np.allclose(
                _vector(_property(material, "shader_parameter/rotation_offset")), (0.5, 0.8)
            )
            and np.allclose(
                _vector(_property(material, "shader_parameter/rotation_st"), 4), (0, 0, 0, 0.1)
            )
            and abs(_property(material, "shader_parameter/rotation_range") - 0.35) < 1e-7,
            "Leaf source shader uniforms changed",
        )
        row["pivot"] = [0, -0.3]
    elif shader.endswith("vfx_flipbook_shader.gdshader"):
        t.require(a.shape == (256, 256, 4), "Source lightning texture dimensions changed")
        row["kind"] = 10
        row["fields"][0] = bank.field(a[:, :, (0, 3)])
        row["curves"][0] = bank.curve(
            renderer._shader_texture(_property(material, "shader_parameter/lut"))
        )
        t.require(
            _property(material, "shader_parameter/frame_count") == 4
            and np.allclose(_vector(_property(material, "shader_parameter/flipbook_size")), (2, 2))
            and not _property(material, "shader_parameter/use_erosion_texture")
            and not _property(material, "shader_parameter/use_hue_shift"),
            "Lightning source shader uniforms changed",
        )
        row["pivot"] = list(_vector(_property(material, "shader_parameter/pivot_offset")))
    else:
        t.require(
            shader.endswith("vfx_grayscale_particle_shader.gdshader") and a.shape == (128, 128, 4),
            "Unreviewed card particle shader",
        )
        row["kind"] = 9
        row["fields"][0] = bank.field(a[:, :, (0, 3)])
        row["curves"][0] = bank.curve(
            renderer._shader_texture(_property(material, "shader_parameter/lut"))
        )
        row["pivot"] = list(_vector(_property(material, "shader_parameter/pivot_offset", (0, 0))))
    row["blend"] = blend
    return row


def source_tracks(bank, renderer, node, row, cache):
    props = node.props
    _, process = renderer.loader.resource(_property(props, "process_material"))

    def get(name, default=None):
        return _property(process, name, default)

    t.require(
        get("particle_flag_disable_z", False)
        and not np.any(_vector(get("gravity", (0, -9.8, 0)), 3)),
        "Unhandled card particle dimension/gravity",
    )
    t.require(
        _property(props, "emitting", True)
        and not _property(props, "one_shot", False)
        and _property(props, "speed_scale", 1) == 1
        and not _property(props, "trail_enabled", False),
        "Unhandled card particle lifecycle",
    )
    for key in (
        "linear_accel_min",
        "linear_accel_max",
        "radial_accel_min",
        "radial_accel_max",
        "tangential_accel_min",
        "tangential_accel_max",
        "angular_velocity_min",
        "angular_velocity_max",
        "initial_angle_min",
        "initial_angle_max",
        "orbit_velocity_min",
        "orbit_velocity_max",
        "collision_mode",
    ):
        t.require(abs(get(key, 0)) < 0.0001, "Unhandled card particle force: " + key)
    curves = {
        name: sample_curve(renderer, process, name)
        for name in (
            "scale_curve",
            "scale_over_velocity_curve",
            "alpha_curve",
            "damping_curve",
            "radial_velocity_curve",
            "turbulence_influence_over_life",
        )
    }
    color_ramp = sample_curve(renderer, process, "color_ramp")
    initial_ramp = sample_curve(renderer, process, "color_initial_ramp")
    turbulence = bool(get("turbulence_enabled", False))
    for key in (
        "hue_variation_min",
        "hue_variation_max",
        "directional_velocity_min",
        "directional_velocity_max",
    ):
        t.require(abs(get(key, 0)) < 0.0001, "Unhandled card particle display/control: " + key)
    for key in ("animation_speed_curve", "animation_offset_curve", "velocity_limit_curve"):
        t.require(get(key) is None, "Unhandled card particle curve: " + key)
    if turbulence:
        t.require(
            max(
                abs(get(key, 0))
                for key in (
                    "turbulence_initial_displacement_min",
                    "turbulence_initial_displacement_max",
                )
            )
            < 0.000002,
            "Card initial turbulence displacement needs explicit sampling",
        )
    else:
        t.require(
            max(
                abs(get(key, 0))
                for key in (
                    "radial_velocity_min",
                    "radial_velocity_max",
                    "damping_min",
                    "damping_max",
                )
            )
            < 0.0001,
            "Unhandled non-turbulent particle force",
        )
    library = helper(cache) if turbulence else None
    life = float(row["lifetime"])
    fps = int(row["fixed_fps"]) or 30
    # The stationary leaves retain their shader's TIME/instance-offset motion.
    ticks = 0 if row["kind"] == 8 else math.ceil(life * fps)
    matrix = np.vstack((np.asarray(row["matrix"]).reshape(2, 3), (0, 0, 1)))
    screen = renderer.screen
    local = bool(_property(props, "local_coords", False))
    origin = matrix[:2, 2].copy()
    basis = matrix[:2, :2]
    records = []
    for index in range(row["amount"]):
        rng = Random(index + 1 + seed_hash(sum(row["node"].encode())))

        def mix(name, default=0.0):
            return float(get(name + "_min", default)) + (
                float(get(name + "_max", default)) - float(get(name + "_min", default))
            ) * float(rng())

        scale = mix("scale", 1.0)
        rng()
        anim_speed = mix("anim_speed")
        anim_offset = mix("anim_offset")
        fraction = 1 - float(get("lifetime_randomness", 0)) * float(rng())
        color = np.asarray(_vector(get("color", (1, 1, 1, 1)), 4), dtype="f4")
        if get("color_initial_ramp") is not None:
            color = color * sample(initial_ramp, float(rng()))
        rng()
        rng()
        initial = mix("initial_velocity")
        rng()
        radial = mix("radial_velocity")
        rng()
        influence = mix("turbulence_influence", 0.1) if turbulence else 0.0
        rng()
        rng()
        rng()
        damping = mix("damping")
        rng()
        shape = int(get("emission_shape", 0))
        if shape == 6:
            t.require(
                np.array_equal(_vector(get("emission_ring_axis"), 3), (0, 0, 1))
                and get("emission_ring_cone_angle", 90) == 90,
                "Card emission ring changed",
            )
            rng()
            angle = float(rng()) * 2 * 3.14159
            inner = float(get("emission_ring_inner_radius"))
            outer = float(get("emission_ring_radius"))
            radius = math.sqrt(float(rng()) * (outer * outer - inner * inner) + inner * inner)
            position = np.array(
                (math.sin(angle) * radius, math.cos(angle) * radius, 0.0), dtype="f4"
            )
        elif shape == 3:
            position = np.array([float(rng()) * 2 - 1 for _ in range(3)], dtype="f4") * _vector(
                get("emission_box_extents"), 3
            )
        else:
            t.require(shape == 0, "Unsupported card emission shape")
            position = np.zeros(3, dtype="f4")
        position = position * _vector(get("emission_shape_scale", (1, 1, 1)), 3) + _vector(
            get("emission_shape_offset", (0, 0, 0)), 3
        )
        if turbulence:
            rng()  # Pinned1.49e-6 displacement is below1/65536 LCD pixel.
        direction = _vector(get("direction", (1, 0, 0)), 3)
        angle = (
            math.atan2(direction[1], direction[0])
            + (float(rng()) * 2 - 1) * get("spread", 45) * 3.14159 / 180
        )
        velocity = np.array((math.cos(angle) * initial, math.sin(angle) * initial, 0.0), dtype="f4")
        if not local:
            position[:2] = basis @ position[:2] + origin
            velocity[:2] = basis @ velocity[:2]
        center = origin if not local else np.zeros(2)
        actual_life = life * fraction
        frames = []
        affine_error = 0.0
        # Source node randomness only changes birth placement within its slot.
        delay = (
            (index + float(_property(props, "randomness", 0)) * float(rng()))
            / row["amount"]
            * (1 - float(_property(props, "explosiveness", 0)))
            * life
        )
        for tick in range(ticks + 1):
            age = tick / fps
            phase = min(1.0, age / actual_life)
            final = velocity.copy()
            if tick and turbulence:
                radial_speed = radial * float(sample(curves["radial_velocity_curve"], phase)[0])
                delta = position[:2] - center
                control = np.zeros(3, dtype="f4")
                control[:2] = normalized(delta) * radial_speed
                if radial_speed < 0:
                    control[:2] = normalized(control[:2]) * min(
                        abs(radial_speed), float(np.linalg.norm(delta)) * fps
                    )
                speed = float(np.linalg.norm(velocity))
                damp = damping * float(sample(curves["damping_curve"], phase)[0])
                speed = max(
                    0.0,
                    speed
                    - (
                        speed * speed * damp * 0.05
                        if get("particle_flag_damping_as_friction", False)
                        else damp
                    )
                    / fps,
                )
                velocity = normalized(velocity) * speed
                noise_scale = float(get("turbulence_noise_scale", 9)) ** 0.25 * (4 / 10**0.25) - 4
                movement = _vector(get("turbulence_noise_speed", (0, 0, 0)), 3)
                at = np.array(
                    (
                        *((position * noise_scale) + (age + delay) * movement),
                        (age + delay) * get("turbulence_noise_speed_random", 0.2),
                    ),
                    dtype="f4",
                )
                noise = np.empty(3, dtype="f4")
                library.nspire_godot_curl4(
                    at.ctypes.data_as(FLOAT_P),
                    float(get("turbulence_noise_strength", 1)),
                    noise.ctypes.data_as(FLOAT_P),
                )
                amount = max(
                    0.0,
                    min(
                        1.0,
                        influence
                        * float(sample(curves["turbulence_influence_over_life"], phase)[0]),
                    ),
                )
                gain = 1 + (1 - amount) * 0.2
                velocity = (
                    velocity * (1 - amount)
                    + noise * float(np.linalg.norm(velocity)) * gain * amount
                )
                control = (
                    control * (1 - amount) + noise * float(np.linalg.norm(control)) * gain * amount
                )
                final = velocity + control
                final[2] = 0
                position += final / fps
            elif tick:
                position += velocity / fps
            scales = scale * sample(curves["scale_curve"], phase)
            if scales.size == 1:
                scales = np.repeat(scales, 3)
            if get("scale_over_velocity_curve") is not None:
                speed = float(np.linalg.norm(final))
                extent = float(
                    get("scale_over_velocity_max", 5) - get("scale_over_velocity_min", 0)
                )
                scales = scales * sample(
                    curves["scale_over_velocity_curve"], min(1.0, speed / extent)
                )
            scales = np.maximum(0.001, scales)
            rotation = np.eye(2)
            if get("particle_flag_align_y", False) and np.linalg.norm(final[:2]) > 0:
                axis = normalized(final[:2])
                rotation = np.array(((axis[1], axis[0]), (-axis[0], axis[1])))
            transform = np.eye(3)
            transform[:2, :2] = rotation @ np.diag(scales[:2])
            transform[:2, 2] = position[:2]
            if local:
                transform = matrix @ transform
            # Particle VERTEX covers the complete source texture, including
            # the original custom vertex pivot. No sprite dimensions shrink.
            w, h = row["texture_size"]
            pivot = np.array(row["pivot"])
            quad = np.array(
                ((w, 0, (pivot[0] - 0.5) * w), (0, h, (pivot[1] - 0.5) * h), (0, 0, 1.0))
            )
            projected = screen @ transform @ quad
            corners = projected @ np.array(((0, 1, 1, 0), (0, 0, 1, 1), (1, 1, 1, 1)))
            lo = np.floor(corners[:2].min(axis=1)).astype(int)
            hi = np.ceil(corners[:2].max(axis=1)).astype(int)
            inverse = np.linalg.inv(projected) @ np.diag((156, 218, 1.0))
            # Q12 inverse coefficients stay in the Q20 wire representation.
            # Across normalized viewport[0,1]^2 each UV coordinate error is
            # bounded by1.5/4096. The forward quad bounds the corresponding
            # LCD displacement; every retained visible frame must stay<.05px.
            error = float(np.max(np.abs(projected[:2, :2]).sum(axis=1))) * 1.5 / 4096
            t.require(error < 0.05, "Card particle transform exceeds the subpixel precision bound")
            affine_error = max(affine_error, error)
            q = np.rint(inverse[:2].ravel() * 4096).astype("i8") * 256
            # At vanishing source scale no raster sample can contribute.
            active = age <= actual_life and np.all(abs(q) < 2147483647)
            tint = color * sample(color_ramp, phase)
            tint[3] *= float(sample(curves["alpha_curve"], phase)[0])
            tint *= row["tint"]
            rgba = np.rint(np.clip(tint, 0, 1) * 255).astype("u1")
            if not active:
                rgba[3] = 0
                q[:] = 0
            custom = (anim_offset + phase * anim_speed) % 1.0
            frames.append(
                struct.pack(
                    "<4h6i4BH",
                    *map(int, np.clip([*lo, *hi], -32768, 32767)),
                    *map(int, q),
                    *map(int, rgba),
                    round(custom * 65535),
                )
            )
        records.append(
            dict(
                lifetime_ms=round(life * 1000),
                delay_ms=round(delay * 1000),
                fps=fps,
                frames=b"".join(frames),
                count=len(frames),
                maximum_affine_error_pixels=affine_error,
            )
        )
    return records
