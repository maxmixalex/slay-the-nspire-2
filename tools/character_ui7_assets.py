"""Original character-selection Spine loops at the authored 4:3 camera.

The official separately licensed runtime supplies bone/mesh/constraint states.
The existing reviewed room compositor supplies node hierarchy, slot followers
and fire materials. All sampled pixels and receipts remain private. These are
12 fps LDR samples, not unmodified Godot screenshots. Independent procedural
particles and water distortion are recorded omissions; shader noise is held at
time zero rather than falsely assigned the skeleton's unrelated loop period.
"""
from __future__ import annotations

import json
from pathlib import Path
import numpy as np

import content_tiles as tiles
from build_content import texture
from patch_feedback_assets import write_image, SOURCE_PCK_SHA256
from world_enemy_assets import SpineSampler
from world_room_shaders import RoomRenderer
from world_scene_assets import Property, SceneRenderError, _node_matrix, _property, _vector

CHARACTERS = ('ironclad', 'silent', 'defect', 'regent', 'necrobinder')
DURATIONS = (5333, 11667, 13333, 6667, 8000)
COUNTS = (64, 140, 160, 80, 96)
SIZE, VIEWPORT = (320, 240), (1440, 1080)
CLIP_FIRST, FRAME_COUNT = 1705, 540


def converter_identity():
    names = ('character_ui7_assets.py', 'world_scene_assets.py', 'world_room_shaders.py',
             'world_enemy_assets.py', 'SpineSampleAdapter.cs', 'animation.py')
    return tiles.sha256(b''.join((Path(__file__).parent/name).read_bytes() for name in names))


class FrameSampler:
    def __init__(self, sampler, resource, metadata):
        self.sampler, self.resource, self.info = sampler, resource, metadata
        self.frame, self.time = None, 0

    def metadata(self, resource):
        tiles.require(resource == self.resource, 'Unexpected additional character skeleton')
        return self.info

    def sample(self, resource, clip, times, **settings):
        tiles.require(resource == self.resource and clip == 'animation' and times == [self.time] and
                      self.frame is not None, 'Character compositor requested another state')
        return [self.frame]

    def draw(self, resource, frame):
        return self.sampler.draw(resource, frame)


class CharacterRenderer(RoomRenderer):
    def spine(self, node, path, matrix, tint, clips, z):
        bones, commands, order = super().spine(node, path, matrix, tint, clips, z)
        overrides = {}
        for mode, key in enumerate(('normal_material', 'additive_material', 'multiply_material', 'screen_material')):
            ref = _property(node.props, key)
            if ref is None:
                continue
            attrs, properties = self.loader.resource(ref)
            if attrs.get('type') != 'CanvasItemMaterial':
                raise SceneRenderError('Unreviewed character Spine material: '+key)
            original = int(_property(properties, 'blend_mode', 0))
            overrides[mode] = {0: 0, 1: 1, 2: 4, 3: 2, 4: 5}[original]
        for slot, draws in commands.items():
            commands[slot] = [(level, image, overrides.get(blend, blend), masks)
                              for level, image, blend, masks in draws]
        self.diagnostics['spines'][-1]['authored_material_overrides'] = overrides
        return bones, commands, order


def build_art(pack, cache, *, runtime, dotnet=None):
    """Return contiguous images/clips and local first-frame auxiliary mapping.

    Append ``images`` after the caller's static prefix. Add that prefix length
    to each clip's ``first`` and to ``auxiliary_rows`` before pack insertion.
    This avoids five duplicate initial frames or duplicate image payloads.
    """
    cache = Path(cache);cache.mkdir(parents=True, exist_ok=True)
    path = cache/'character-ui7-assets.json'
    identity, runtime_identity = converter_identity(), tiles.sha256(Path(runtime).read_bytes())
    if path.is_file():
        previous = json.loads(path.read_text())
        if previous.get('converter_sha256') == identity and previous.get('runtime_sha256') == runtime_identity:
            tiles.require(len(previous['images']) == FRAME_COUNT and len(previous['clips']) == 5,
                          'Changed cached character catalog')
            for row in previous['images']:
                raw = (cache/row['file']).read_bytes()
                tiles.require(len(raw) == row['bytes'] and tiles.sha256(raw) == row['sha256'],
                              'Changed cached character frame')
            return previous

    rows, clips, auxiliary, reports = [], [], [], []
    with SpineSampler(pack, runtime=runtime, dotnet=dotnet, work_dir=cache/'character-spine-adapter') as sampler:
        for character, duration, count in zip(CHARACTERS, DURATIONS, COUNTS):
            resource = f'animations/character_select/{character}/characterselect_{character}_skel_data.tres'
            metadata = sampler.metadata(resource)
            tiles.require(len(metadata['clips']) == 1 and metadata['clips'][0]['name'] == 'animation' and
                          metadata['clips'][0]['duration_ms'] == duration,
                          'Original NSpineAutoPlayer loop changed: '+character)
            period = metadata['clips'][0]['duration']
            times = [period*index/count for index in range(count)]
            frames = sampler.sample(resource, 'animation', times)
            fixed = FrameSampler(sampler, resource, metadata)
            renderer = CharacterRenderer(pack, *SIZE, VIEWPORT, texture, fixed, 0., 1)
            root = renderer.loader.instantiate('scenes/screens/character_select_screen.tscn')
            container = next(node for node in root.children if node.name == 'AnimatedBg')
            origin, size = renderer.control_size(container, np.asarray(VIEWPORT, dtype=float))
            tiles.require(tuple(origin) == (-388., -80.) and tuple(size) == (2080., 1200.) and
                          tuple(_vector(_property(container.props, 'pivot_offset'))) == (1280., 600.),
                          'Original character parent framing changed')
            # NCharacterSelectScreenBg.OnWindowChange clamps aspect to 4:3;
            # at that endpoint CubicOut is zero and scale is 1.1*1.153.
            container.props['scale'] = Property('Vector2(1.2683, 1.2683)', container.scope)
            matrix = _node_matrix(container.props, origin)
            scene = f'scenes/screens/char_select/char_select_bg_{character}.tscn'
            node = renderer.loader.instantiate(scene)
            first = len(rows);auxiliary.append(first)
            first_report = None
            for index, (seconds, sample) in enumerate(zip(times, frames)):
                fixed.frame, fixed.time, renderer.time = sample, seconds, seconds
                renderer.room_shader_time = 0.
                renderer.commands = []
                for key in ('drawn', 'omitted', 'approximations', 'spines', 'conditions', 'room_shaders'):
                    renderer.diagnostics[key] = []
                renderer.visit(node, matrix, size, np.ones(4), 0, node.name)
                image, diagnostics = renderer.finish()
                row = write_image(cache, f'character-ui7-{character}-{index:03d}', image, sparse=True)
                row.update(key=f'character_ui7.{character}.{index:03d}', source=scene,
                           frame=index, sample_seconds=seconds)
                rows.append(row)
                if index in (0, count//2, count-1):
                    image.save(cache/f'character-ui7-{character}-{index:03d}.png')
                if first_report is None:
                    first_report = json.loads(json.dumps(diagnostics))
            clips.append(dict(first=first, count=count, frame_ms=round(duration/count), pivot=[0, 0],
                              key='character_ui7.'+character, duration_ms=duration))
            reports.append(dict(character=character, source=scene, source_skeleton=resource,
                                frames=count, duration_ms=duration, loop=True,
                                parent_matrix=matrix.tolist(), parent_size=size.tolist(),
                                sample_rate=count/period, initial_frame=first_report))
            print(json.dumps(dict(character=character, frames=count, duration_ms=duration,
                                  stored_bytes=sum(row['bytes'] for row in rows[first:]))), flush=True)
    tiles.require(len(rows) == FRAME_COUNT, 'Incorrect character animation image count')
    result = dict(schema=1, reference='v0.111.0 / 41cef1ea', source_pck_sha256=SOURCE_PCK_SHA256,
                  converter_sha256=identity, runtime_sha256=runtime_identity, images=rows, clips=clips,
                  auxiliary_rows=auxiliary, durations_ms=list(DURATIONS), clip_first=CLIP_FIRST,
                  viewport=list(VIEWPORT), output_size=list(SIZE), character_reports=reports,
                  limits=['12fps skeletal samples', 'independent particles omitted',
                          'water distortion frozen to authored texture', 'fire shader noise fixed at time zero',
                          'Regent constellation hover skins not sampled', 'no original Godot/GPU pixel oracle'])
    tiles.atomic_bytes(path, json.dumps(result, indent=2).encode()+b'\n')
    return result
