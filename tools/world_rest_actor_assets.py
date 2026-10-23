"""Sample the pinned rest-site actor scenes in their common room camera.

The plan module has no heavy or private-input dependencies. This converter is
loaded only for actual art work; its output and diagnostics remain private.
"""
from __future__ import annotations

from copy import deepcopy
import numpy as np
from PIL import Image, ImageChops

from world_enemy_assets import RUNTIME_SHA256
from world_rest_actors import ACT_CLIPS, LIGHT_OFF_TRACK, REST_ACTORS, ROOT_SCALE
from world_room_profile import room_profile
from world_room_shaders import RoomRenderer
from world_scene_assets import Property, SceneRenderError, _property, _vector


# Godot 4.5.1-stable Colors.cs defines DarkGray as 0xA9A9A9FF;
# Color(uint) divides each byte by 255.0f. Keep that binary32 value, not a
# rounded decimal or a linear-light conversion of these LDR numeric channels.
# https://github.com/godotengine/godot/blob/4.5.1-stable/modules/mono/glue/GodotSharp/GodotSharp/Core/Colors.cs
# https://github.com/godotengine/godot/blob/4.5.1-stable/modules/mono/glue/GodotSharp/GodotSharp/Core/Color.cs
DARK_GRAY_MODULATE = (float(np.float32(169) / np.float32(255)),) * 3 + (1.,)


def _child(node, name):
    matches = [child for child in node.children if child.name == name]
    if len(matches) != 1:
        raise SceneRenderError('Original rest scene must contain one direct ' + name)
    return matches[0]


def _same(actual, expected, label):
    if not np.allclose(actual, expected, atol=.00001, rtol=0):
        raise SceneRenderError('Original rest ' + label + ' differs from the reviewed recipe')


def _strip_runtime_meshes(node):
    """Remove serialized Godot SpineMesh2D placeholders, not authored art.

    Silent and Defect serialize the runtime's material-only mesh children.
    Sampler draw slots already recreate each of these meshes, in draw order.
    Reject real geometry or indexed authored siblings instead of duplicating
    or silently losing it under an assumed placeholder interpretation.
    """
    removed = [child for child in node.children if child.kind == 'SpineMesh2D']
    for child in removed:
        if child.children or set(child.props) - {'show_behind_parent', 'material'}:
            raise SceneRenderError('Unreviewed authored rest SpineMesh2D: ' + child.name)
    removed_ids = {id(child) for child in removed}
    if removed and any(index >= 0 and id(child) not in removed_ids
                       for child, index in node.child_ops):
        raise SceneRenderError('Indexed rest child depends on serialized runtime meshes')
    node.children = [child for child in node.children if id(child) not in removed_ids]
    node.child_ops = [(child, index) for child, index in node.child_ops if id(child) not in removed_ids]
    return len(removed) + sum(_strip_runtime_meshes(child) for child in node.children)


def _material_overrides(renderer, spine, recipe):
    """Resolve the actual scene material before trusting a Spine slot enum.

    SpineSprite.cpp's material dispatch first uses a slot-node override, then
    the corresponding SpineSprite material and only then a default. The pinned
    Regent and Necro each override multiply with an empty CanvasItemMaterial:
    Godot's default is MIX. No new/custom shader or material mode is assumed.
    Primary API source: EsotericSoftware/spine-runtimes, 4.2, SpineSprite.cpp
    and Godot 4.5.1-stable scene/resources/canvas_item_material.h.
    """
    names = ('normal_material', 'additive_material', 'multiply_material', 'screen_material')
    actual = {}
    for blend, name in enumerate(names):
        ref = _property(spine.props, name)
        if ref is None:
            continue
        attrs, props = renderer.loader.resource(ref)
        if attrs.get('type') != 'CanvasItemMaterial' or props:
            raise SceneRenderError('Unreviewed rest Spine material override: ' + name)
        actual[blend] = 0
    if actual != recipe.get('material_overrides', {}):
        raise SceneRenderError('Original rest Spine material override drift')
    pending = list(spine.children)
    while pending:
        node = pending.pop()
        if node.kind == 'SpineSlotNode' and any(_property(node.props, name) is not None for name in names):
            raise SceneRenderError('Unreviewed rest slot-node material override')
        pending.extend(node.children)
    return actual


def _actor_scene(renderer, recipe, row, profile):
    rest = profile['rest']
    single = rest['singleplayer_actor']
    room = renderer.loader.instantiate(rest['source_scene'])
    parent = _child(room, rest['parent']['node'])
    parent_origin, parent_size = renderer.control_size(parent, np.asarray(row['viewport']), None)
    _same(parent_origin, rest['parent']['evaluated_origin'], 'room parent origin')
    _same(parent_size, rest['parent']['evaluated_size'], 'room parent size')
    container_name = single['node'].split('/')[-1]
    container = _child(parent, container_name)
    container_origin, container_size = renderer.control_size(container, parent_size, None)
    _same(np.asarray(parent_origin) + container_origin, single['evaluated_origin'], 'single-player origin')
    _same(_vector(_property(container.props, 'scale', (1, 1))), single['scale'], 'single-player scale')
    _same(container_size, [0., 0.], 'single-player container size')
    if not np.array_equal(_vector(_property(container.props, 'modulate', (1, 1, 1, 1)), 4), np.ones(4)):
        raise SceneRenderError('Original rest single-player authored modulate drift')
    if row['dark']:
        # NRestSiteRoom.ExtinguishFireIfAble sets Character_1..4.Modulate
        # after HideFlameGlow. Inherit this before individual draw/plane
        # blending; post-darkening a complete additive plane loses saturated
        # emission. The reviewed attached fire shaders replace COLOR and
        # therefore intentionally ignore this inherited input themselves.
        container.props['modulate'] = Property(
            'Color(' + ', '.join(map(repr, DARK_GRAY_MODULATE)) + ')', container.scope)
    actor = renderer.loader.instantiate(row['path'])
    _same(_vector(_property(actor.props, 'position')), recipe['root_position'], 'authored actor position')
    _same(_vector(_property(actor.props, 'scale', (1, 1))), ROOT_SCALE, 'actor root scale')
    if float(_property(actor.props, 'rotation', 0)) != 0:
        raise SceneRenderError('Unreviewed original rest actor root rotation')
    direct = [child.name for child in actor.children if child.kind == 'SpineSprite']
    expected = ['Necro', 'Osty'] if recipe['character'] == 4 else [recipe['node']]
    if direct != expected:
        raise SceneRenderError('Original rest direct Spine children or companion order drift')
    spine = _child(actor, recipe['node'])
    _same(_vector(_property(spine.props, 'position')), recipe['position'], 'Spine child position')
    _same(_vector(_property(spine.props, 'scale', (1, 1))), recipe['scale'], 'Spine child scale')
    if renderer.loader.path(_property(spine.props, 'skeleton_data_res')) != recipe['resource']:
        raise SceneRenderError('Original rest Spine resource drift')
    materials = _material_overrides(renderer, spine, recipe)
    # NRestSiteRoom overrides this position after instantiation. Keeping the
    # authored (-2,42) would displace four characters in every act.
    actor.props['position'] = Property('Vector2(0, 0)', actor.scope)
    actor.children, actor.child_ops = [spine], [(spine, -1)]
    removed = _strip_runtime_meshes(spine)
    container.children, container.child_ops = [actor], [(actor, -1)]
    parent.children, parent.child_ops = [container], [(container, -1)]
    renderer.diagnostics.update(source_scene=rest['source_scene'], actor_scene=row['path'],
        actor=row['actor'], actor_node=row['node'], room_parent_origin=list(parent_origin),
        room_parent_size=list(parent_size), singleplayer_origin=list(single['evaluated_origin']),
        singleplayer_scale=list(single['scale']), actor_root_position=[0., 0.],
        singleplayer_container=room.name + '/' + parent.name + '/' + container.name,
        singleplayer_modulate=_vector(_property(container.props, 'modulate', (1, 1, 1, 1)), 4).tolist(),
        modulation_policy='inherited_before_raster_and_ordered_plane_composition',
        attached_fire_modulation='reviewed_fragment_replaces_COLOR_without_inherited_modulate'
            if recipe['character'] == 4 else None,
        actor_root_scale=list(ROOT_SCALE), spine_position=list(recipe['position']),
        spine_scale=list(recipe['scale']), removed_runtime_meshes=removed,
        material_overrides=[dict(source_slot_blend=source, effective_blend=effective,
                                 material='authored_empty_CanvasItemMaterial_default_MIX')
                            for source, effective in materials.items()],
        excluded_controls='ControlRoot (hitbox, focus reticle and thought anchors)',
        companion_condition='original_singleplayer_scene_always_present' if recipe['key'] == 'OSTY' else None)
    return room, parent


def _metadata(sampler, recipe):
    metadata = sampler.metadata(recipe['resource'])
    if metadata.get('source_version') != recipe['source_version']:
        raise SceneRenderError('Original rest Spine source version drift')
    if metadata.get('available_skins') != ['default']:
        raise SceneRenderError('Original rest skin directory drift')
    clips = {clip['name']: clip for clip in metadata['clips']}
    for name in ACT_CLIPS:
        clip = clips.get(name)
        if clip is None or abs(clip['duration'] - recipe['duration']) > .00001:
            raise SceneRenderError('Original rest animation name/duration drift')
        if clip.get('events', []):
            raise SceneRenderError('Unreviewed original rest animation event')
        if any('DrawOrder' in timeline for timeline in clip.get('timeline_types', [])):
            raise SceneRenderError('Rest blend planes require the reviewed stable draw order')
    off = clips.get('_tracks/light_off')
    if off is None or off['duration'] != 0 or off.get('events', []):
        raise SceneRenderError('Original rest light-off track drift')
    return metadata


class _FrameSampler:
    """Feed one batch-sampled original state to the shared scene renderer."""
    def __init__(self, sampler, resource, metadata):
        self.sampler, self.resource, self.info = sampler, resource, metadata
        self.frame, self.time = None, None

    def metadata(self, resource):
        if resource != self.resource:
            raise SceneRenderError('Unexpected additional rest skeleton')
        return self.info

    def sample(self, resource, clip, times, **settings):
        if resource != self.resource or times != [self.time] or self.frame is None:
            raise SceneRenderError('Rest scene requested a different sampled state')
        return [self.frame]

    def draw(self, resource, frame):
        return self.sampler.draw(resource, frame)


class _ActorRenderer(RoomRenderer):
    def spine(self, node, path, matrix, tint, clips, z):
        bones, commands, order = super().spine(node, path, matrix, tint, clips, z)
        recipe = self.actor_recipe
        expected = recipe.get('draw_order')
        if expected is not None and order != expected:
            raise SceneRenderError('Original rest blend-plane draw order drift')
        boundaries = [order.index(slot) for slot in recipe.get('plane_boundaries', [])]
        source_blends, effective_blends = {}, {}
        overrides = recipe.get('material_overrides', {})
        for slot, draws in commands.items():
            at = order.index(slot)
            plane = sum(at >= boundary for boundary in boundaries)
            effective_draws = []
            for z_index, image, source_blend, masks in draws:
                effective = overrides.get(source_blend, source_blend)
                source_blends[slot], effective_blends[slot] = source_blend, effective
                if effective != recipe['blends'][plane]:
                    raise SceneRenderError('Original rest slot blend differs from its ordered plane: ' + slot)
                self.command_planes[id(image)] = plane
                effective_draws.append((z_index, image, effective, masks))
            commands[slot] = effective_draws
        self.diagnostics['spines'][-1].update(clip=self.actor_row['clip'],
            loop=True, track=0, extra_tracks=[dict(LIGHT_OFF_TRACK)] if self.actor_row['dark'] else [],
            source_slot_blends=source_blends, effective_slot_blends=effective_blends)
        return bones, commands, order


def _plane_image(commands, size, blend):
    """Encode a consecutive blend group without mixing in the room background.

    Normal layers use ordinary straight RGBA. Additive layers store the sum of
    their premultiplied emission in RGB with alpha 255 (zero emission stays
    transparent). Runtime additive composition then adds that emission once.
    Unreviewed multiply/screen modes are rejected, including an unexpected
    removal of Regent's authored normal-material override.
    """
    layers = []
    for _, image, actual_blend, clips in commands:
        if image is None or actual_blend != blend:
            raise SceneRenderError('Rest actor plane contains a different blend or shared scene')
        if clips:
            image = image.copy()
            alpha = image.getchannel('A')
            for clip in clips:
                alpha = ImageChops.multiply(alpha, clip)
            image.putalpha(alpha)
        layers.append(image)
    if blend not in (0, 1):
        raise SceneRenderError('Unreviewed rest actor blend mode')
    out = np.zeros((size[1], size[0], 4), dtype=float)
    for image in layers:
        src = np.asarray(image, dtype=float) / 255.
        alpha = src[:, :, 3:4]
        if blend == 1:
            out[:, :, :3] += src[:, :, :3] * alpha
        elif blend == 0:
            out[:, :, :3] = src[:, :, :3] * alpha + out[:, :, :3] * (1 - alpha)
            out[:, :, 3:4] = alpha + out[:, :, 3:4] * (1 - alpha)
        else:
            raise SceneRenderError('Unreviewed rest actor blend mode')
    if blend == 1:
        out[:, :, 3] = np.any(out[:, :, :3] > 0, axis=2)
    else:
        np.divide(out[:, :, :3], out[:, :, 3:4], out=out[:, :, :3], where=out[:, :, 3:4] > 0)
    return Image.fromarray(np.uint8(np.clip(np.rint(out * 255), 0, 255)))


def sample_rest_actor(pack, row, sampler, *, texture_loader=None):
    """Sample one actor/act/lighting/ordered-plane job at its complete period."""
    profile = room_profile()
    recipe = next((actor for actor in REST_ACTORS if actor['key'] == row['actor']), None)
    if recipe is None:
        raise SceneRenderError('Unreviewed rest actor')
    act, plane = row['act_index'], row['plane']
    if (row['kind'] != 'rest-actor-clip' or not 0 <= act < len(ACT_CLIPS) or
            not 0 <= plane < len(recipe['blends']) or row['blend'] != recipe['blends'][plane] or
            row['clip'] != ACT_CLIPS[act] or row['resource'] != recipe['resource'] or
            row['path'] != profile['rest']['singleplayer_actor']['scenes'][recipe['scene_character']] or
            row['duration'] != recipe['duration'] or row['duration_ms'] != round(recipe['duration'] * 1000) or
            row['size'] != profile['output']['size'] or row['viewport'] != profile['conversion_viewport'] or
            row['screen_origin'] != profile['output']['screen_origin'] or row['pivot'] != [0, 0] or
            row['background'] != 'transparent' or
            row['shader_time'] != 0 or row['phase'] != 0 or not row['loop'] or
            not isinstance(row['frames'], int) or not 1 <= row['frames'] <= 256):
        raise SceneRenderError('Original rest actor request differs from its reviewed recipe')
    if texture_loader is None:
        from build_content import texture as texture_loader
    metadata = _metadata(sampler, recipe)
    times = [index * row['duration'] / row['frames'] for index in range(row['frames'])]
    sampled = sampler.sample(row['resource'], row['clip'], times, loop=True,
        tracks=[dict(LIGHT_OFF_TRACK)] if row['dark'] else [])
    if len(sampled) != len(times):
        raise SceneRenderError('Rest sampler did not return the full requested period')
    proxy = _FrameSampler(sampler, row['resource'], metadata)
    renderer = _ActorRenderer(pack, *row['size'], tuple(row['viewport']), texture_loader, proxy, 0., 1)
    renderer.actor_recipe, renderer.actor_row = recipe, row
    # Godot shader TIME is independent of animation track time. A skeleton
    # loop cannot supply an exact shader loop: freeze the reviewed noise phase
    # explicitly while preserving the animated bone/slot fire transforms.
    renderer.room_shader_time = row['shader_time']
    room, parent = _actor_scene(renderer, recipe, row, profile)
    baseline = deepcopy(renderer.diagnostics)
    images, first_diagnostics, visible_bounds = [], None, None
    for time, frame in zip(times, sampled):
        proxy.time, proxy.frame, renderer.time = time, frame, time
        renderer.commands, renderer.command_planes = [], {}
        renderer.diagnostics = deepcopy(baseline)
        renderer.diagnostics['sample_time'] = time
        renderer.visit(parent, np.eye(3), np.asarray(row['viewport'], float),
                       np.ones(4), 0, room.name + '/' + parent.name)
        unsupported = [item for item in renderer.diagnostics['omitted'] if item.get('reason') != 'authored_hidden']
        if unsupported:
            raise SceneRenderError('Unsupported original rest actor layer: ' + str(unsupported))
        commands = sorted(renderer.commands, key=lambda command: command[0])
        # The independent jobs must remain complete contiguous groups. An
        # authored shader with a new blend cannot silently enter a normal group.
        groups = [renderer.command_planes.get(id(command[1]), 0) for command in commands]
        if groups != sorted(groups):
            raise SceneRenderError('Original rest actor blend planes are no longer consecutive')
        for group, command in zip(groups, commands):
            if command[2] != recipe['blends'][group]:
                raise SceneRenderError('Unreviewed rest shader or mesh blend in actor plane')
        selected = [command for group, command in zip(groups, commands) if group == plane]
        image = _plane_image(selected, row['size'], row['blend'])
        images.append(image)
        box = image.getbbox()
        if box is not None:
            visible_bounds = list(box) if visible_bounds is None else [
                min(visible_bounds[0], box[0]), min(visible_bounds[1], box[1]),
                max(visible_bounds[2], box[2]), max(visible_bounds[3], box[3])]
        if first_diagnostics is None:
            first_diagnostics = renderer.diagnostics
            first_diagnostics.update(draw_commands=len(commands), plane_draw_commands=len(selected),
                screen_transform=renderer.screen[:2].ravel().tolist(),
                ordered_plane_blends=list(recipe['blends']), plane_boundaries=recipe.get('plane_boundaries', []),
                composition='normal_straight_rgba_or_additive_premultiplied_emission',
                quantization='shared_sampler_layers_RGBA8_then_plane_RGBA8_then_runtime_RGB565',
                validation='every_sample_order_material_and_effective_blend_checked')
    result = dict(resource=row['resource'], source_version=metadata['source_version'],
        actor=row['actor'], node=row['node'], clip=row['clip'], act_index=act,
        dark=row['dark'], plane=plane, blend=row['blend'], plane_encoding=row['plane_encoding'],
        source_scene=profile['rest']['source_scene'], actor_scene=row['path'],
        size=row['size'], viewport=row['viewport'], screen_origin=row['screen_origin'], pivot=row['pivot'],
        background=row['background'],
        frames=len(images), frame_ms=row['frame_ms'], duration_ms=row['duration_ms'],
        source_duration_seconds=row['duration'], times=times, loop=True, track=0,
        extra_tracks=[dict(LIGHT_OFF_TRACK)] if row['dark'] else [],
        clipping='authored_room_viewport', camera='complete_common_room_uniform_cover',
        runtime_sha256=getattr(sampler, 'runtime_identity', None), physics_step=1/60, diagnostics=first_diagnostics,
        shader_time=row['shader_time'], phase=row['phase'],
        phase_policy='sample_origin_zero_runtime_presentation_clock_no_run_rng',
        sampling_policy=dict(nominal_fps=20, frame_limit=256,
            sampled_fps=row['frames']/row['duration'], complete_period_seconds=row['duration'],
            reduced_by_frame_limit=row['frames'] == 256 and row['duration'] * 20 > 256),
        visible_pixel_bounds=visible_bounds,
        presentation_differences=['presentation_clock_instead_of_per_entry_Chaotic_animation_phase'] +
            (['fire_noise_shader_time_zero_instead_of_independent_global_time',
              'authored_fire_noise_parameters_instead_of_Chaotic_randomization']
             if recipe['character'] == 4 else []))
    return images, result
