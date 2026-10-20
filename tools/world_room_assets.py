"""Compose original rest scenes and sample the original treasure queue.

The scene recipes contain only public resource paths and numeric transforms.
Pixels, skeletons, shader source, and the separately licensed Spine runtime are
read from the owner's private inputs. No substitute room artwork is generated.
"""
from __future__ import annotations
import math
import numpy as np
from animation import raster
from world_enemy_assets import RUNTIME_SHA256
from world_room_profile import room_profile
from world_scene_assets import _Renderer, Property, SceneRenderError, _property


def _child(node, name):
    matches = [child for child in node.children if child.name == name]
    if len(matches) != 1:
        raise SceneRenderError('Original room must contain one direct ' + name)
    return matches[0]


def render_rest(pack, row, *, sampler=None, texture_loader=None):
    """Render the authored act scene inside its actual BgContainer."""
    profile = room_profile()
    rest = profile['rest']
    scene = next((value for value in rest['scenes'] if value['path'] == row['path']), None)
    if scene is None:
        raise ValueError('Unreviewed original rest scene: ' + row['path'])
    if texture_loader is None:
        from build_content import texture as texture_loader
    renderer = _Renderer(pack, *row['size'], tuple(row['viewport']),
                         texture_loader, sampler, 0., 1)
    room = renderer.loader.instantiate(rest['source_scene'])
    parent = _child(room, rest['parent']['node'])
    actual_origin, actual_size = renderer.control_size(parent, np.array(row['viewport'], float), None)
    if (not np.allclose(actual_origin, rest['parent']['evaluated_origin'], atol=0.00001) or
            not np.allclose(actual_size, rest['parent']['evaluated_size'], atol=0.00001)):
        raise SceneRenderError('Original rest parent transform differs from the reviewed recipe')
    background = renderer.loader.instantiate(row['path'])
    if background.name != scene['root']:
        raise SceneRenderError('Original rest scene root differs from the reviewed recipe')
    if row['dark']:
        lighting = _child(background, 'RestSiteLighting')
        lighting.props['visible'] = Property('false', lighting.scope)
    # NRestSiteRoom inserts this zero-size Control at index zero. Character
    # containers are a separate presentation layer, not combat-idle substitutes.
    parent.children = [background]
    origin, size = renderer.control_size(background, actual_size, None)
    if not np.allclose(size, rest['instancing']['root_size'], atol=0.00001):
        raise SceneRenderError('Original rest background is no longer a zero-size Control')
    renderer.diagnostics.update(path=row['path'], source_scene=rest['source_scene'],
        mode='rest-scene', screen_origin=row['screen_origin'], dark=row['dark'],
        room_parent_origin=list(actual_origin), room_parent_size=list(actual_size),
        background_origin=list(origin), background_size=list(size),
        character_layer='separate_original_rest_character_not_yet_converted')
    try:
        renderer.visit(parent, np.eye(3), np.asarray(row['viewport'], float),
                       np.ones(4), 0, room.name + '/' + parent.name)
        return renderer.finish()
    finally:
        if renderer.owns_sampler:
            renderer.sampler.close()


def _treasure_metadata(sampler, resource, skins, profile):
    metadata = sampler.metadata(resource)
    clips = {row['name']: row for row in metadata['clips']}
    for item in profile['treasure']['animation_queue']:
        actual = clips.get(item['clip'])
        if actual is None or abs(actual['duration'] - item['source_duration_seconds']) > 0.00001:
            raise ValueError('Original treasure clip name/duration drift: ' + resource)
        if actual.get('events', []) != item['source_events']:
            raise ValueError('Original treasure event timeline drift: ' + resource)
    if any(skin not in metadata['available_skins'] for skin in skins):
        raise ValueError('Original treasure skin is missing: ' + resource)
    return metadata


def _treasure_transform(profile, size, viewport):
    scale = max(size[0] / viewport[0], size[1] / viewport[1])
    screen = np.array([[scale, 0., (size[0] - viewport[0] * scale) / 2],
                       [0., scale, (size[1] - viewport[1] * scale) / 2], [0., 0., 1.]])
    origin = profile['treasure']['spine_node']['evaluated_origin']
    sx, sy = profile['treasure']['spine_node']['scale']
    # Sampler points are y up; the source Godot scene is y down.
    return screen @ np.array([[sx, 0., origin[0]], [0., -sy, origin[1]], [0., 0., 1.]])


def _clear_layer(size, color):
    width, height = size
    return (np.array([[0., 0.], [width, 0.], [width, -height], [0., -height]]),
            np.array([[0., 0.], [1., 0.], [1., 1.], [0., 1.]]),
            np.array([[0, 1, 2], [2, 3, 0]]),
            np.asarray(color, float).reshape(1, 1, 4), np.ones(4), 0)


def sample_treasure(pack, row, sampler):
    """Sample a segment of one continuous opening -> shine_fade track.

    The SHINE and OPEN requests replay the opening first on the same skeleton.
    Sampling shine_fade alone would reset unkeyed chest bones to setup pose.
    The full source room intentionally clips to its viewport; its registration
    never changes with an animation frame or outline skin.
    """
    profile = room_profile()
    family = next((value for value in profile['treasure']['families']
                   if value['key'] == row['family'] and value['path'] == row['resource']), None)
    if family is None:
        raise ValueError('Unreviewed original treasure family: ' + row['family'])
    metadata = _treasure_metadata(sampler, row['resource'], row['skins'], profile)
    queue = profile['treasure']['animation_queue']
    if row['kind'] == 'treasure-still':
        times = [row['time']]
    elif row['kind'] == 'treasure-clip':
        times = [row['start'] + index * row['duration'] / row['frames']
                 for index in range(row['frames'])]
    else:
        raise ValueError('Unknown treasure presentation request')
    sampled = sampler.sample(row['resource'], queue[0]['clip'], times,
        skins=row['skins'], loop=False,
        tracks=[dict(track=0, clip=queue[1]['clip'], queued=True, loop=False, delay=0)])
    if len(sampled) != len(times):
        raise ValueError('Treasure sampler did not return its full requested period')
    matrix = _treasure_transform(profile, row['size'], row['viewport'])
    low, high = np.full(2, np.inf), np.full(2, -np.inf)
    images = []
    for frame in sampled:
        layers = [_clear_layer(row['size'], profile['treasure']['room_clear_color'])]
        for vertices, uvs, triangles, pixels, color, blend in sampler.draw(row['resource'], frame):
            points = (matrix @ np.column_stack((vertices, np.ones(len(vertices)))).T).T[:, :2]
            if len(points):
                if not np.all(np.isfinite(points)):
                    raise ValueError('Nonfinite original treasure geometry')
                low, high = np.minimum(low, points.min(axis=0)), np.maximum(high, points.max(axis=0))
            layers.append((points * [1, -1], uvs, triangles, pixels, color, blend))
        if len(layers) == 1:
            raise ValueError('Original treasure skeleton has no visible attachments')
        images.append(raster(layers, 1., np.zeros(2), tuple(row['size'])))
    result = dict(resource=row['resource'], source_version=metadata.get('source_version'),
        source_scene=profile['treasure']['source_scene'], skins=row['skins'],
        animation_queue=queue, times=times, frames=len(images), size=row['size'],
        screen_origin=row['screen_origin'], pivot=row['pivot'], loop=False,
        transform=matrix[:2].ravel().tolist(), bounds=[low.tolist(), high.tolist()],
        clipping='authored_room_viewport', runtime_sha256=RUNTIME_SHA256,
        physics_step=1/60, clear_color=profile['treasure']['room_clear_color'])
    if row['kind'] == 'treasure-clip':
        result.update(clip=row['clip'], duration_ms=row['duration_ms'], frame_ms=row['frame_ms'])
    return images, result


def render_room_image(pack, row, sampler):
    if row['kind'] == 'rest-scene':
        return render_rest(pack, row, sampler=sampler)
    if row['kind'] == 'treasure-still':
        images, metadata = sample_treasure(pack, row, sampler)
        return images[0], metadata
    raise ValueError('Unknown original room image kind')
