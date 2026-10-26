"""Append the thirteen source Architect attack VFX and terminal FireBurst.

The separate original hit sparks/numbers and cue schedule remain runtime-owned.
This module samples the actual authored effects. Full conversion belongs to a
checkpointed frozen source archive; --max-frames is a bounded development probe.
No original image, skeleton, shader or game assembly is distributed here.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import math
from pathlib import Path
import re

import numpy as np

from architect_attack_particles import (DrawMaterial, Planes, Resources, particle_systems,
                                       sample_particle_frame, sequence)
from patch_feedback_assets import write_image
from pck import Pack
from world_enemy_assets import SpineSampler
from world_raster import raster
from world_scene_assets import SceneLoader, _node_matrix, _property, _vector, parse_value

BASE_AUXILIARY = 2507
DENSITY = 4/21
CANVAS = (320, 240)
ANCHOR = (160, 120)
WARP_ENCODING = 'RGB565 R5-16 = source dx, G6-32 = source dy; alpha marks nonzero displacement'


@dataclass(frozen=True)
class Effect:
    name: str
    scene: str
    frames: int
    lifetime_ms: int
    fps: int = 30
    frame_ms: int = 0
    planes: tuple = (0,)
    anchor: tuple = ANCHOR

    def time(self, frame):
        return frame*self.frame_ms/1000 if self.frame_ms else frame/self.fps


# Matches ArchitectPresentation::Vfx exactly, followed by the source death
# command's NFireBurstVfx. Ceil-only tail frames retain the actual free time.
EFFECTS = (
    Effect('blunt', 'attack_blunt', 38, 1250),
    Effect('heavy_blunt', 'heavy_blunt', 66, 2200, planes=(0, 2)),
    Effect('slash', 'attack_slash', 4, 268, fps=0, frame_ms=67),
    Effect('bloody_impact', 'bloody_impact', 90, 3000),
    Effect('rock_shatter', 'rock_shatter', 90, 3000),
    Effect('dagger_spray', 'dagger_spray', 30, 1000),
    # This clip enters from beyond -160 source-projected pixels. Its only
    # owner here is the Architect at x245, so keep its complete visible arrival.
    Effect('flying_slash', 'flying_slash', 10, 334, anchor=(245, 120)),
    Effect('dramatic_stab', 'dramatic_stab', 60, 2000),
    Effect('dagger_throw', 'dagger_throw', 60, 2000),
    Effect('lightning', 'attack_lightning', 5, 334, fps=15),
    Effect('scratch', 'scratch', 5, 167),
    Effect('starry_impact', 'starry_impact', 60, 2000, planes=(0, 2)),
    Effect('thrash', 'thrash', 60, 2000, planes=(0, 1)),
    Effect('fire_burst', 'fire_burst', 60, 2000),
)
AUXILIARY_COUNT = BASE_AUXILIARY+sum(effect.frames*len(effect.planes) for effect in EFFECTS)
assert AUXILIARY_COUNT == 3331


def directory():
    result = []; at = BASE_AUXILIARY
    for index, effect in enumerate(EFFECTS):
        result.append(dict(effect=index, name=effect.name, first=at,
            frames=effect.frames, lifetime_ms=effect.lifetime_ms, fps=effect.fps,
            frame_ms=effect.frame_ms, anchor=list(effect.anchor), canvas=list(CANVAS),
            planes=[dict(blend=plane, first=at+i*effect.frames) for i, plane in enumerate(effect.planes)]))
        at += effect.frames*len(effect.planes)
    return result


def sprite_frames(pack, effect):
    """Keep NSpriteAnimator's four textures and AnimationPlayer's five keys."""
    loader = SceneLoader(pack); scene = 'scenes/vfx/vfx_'+effect.scene+'.tscn'
    root = loader.instantiate(scene); resources = Resources(pack, loader)
    if effect.name == 'slash':
        raw = root.props['_frames']
        match = re.fullmatch(r'Array\[Texture2D\]\((.*)\)', raw.text, re.S)
        if not match:
            raise ValueError('Unreviewed NSpriteAnimator frame declaration')
        frames = parse_value(match[1], raw.scope)
        if len(frames) != effect.frames:
            raise ValueError('Slash source texture count changed')
        matrix = _node_matrix(root.props)
        # NSpriteAnimator selects an integer angle from [0,25) in _Ready.
        # Persist a separate cosmetic choice; the source four frames remain.
        rng = np.random.default_rng(0x41cef1ea+2*31337)
        limits = _vector(_property(root.props, '_rotationRange', (0, 0)))
        angle = math.radians(int(rng.integers(int(limits[0]), int(limits[1]))))
        sn, cs = math.sin(angle), math.cos(angle)
        matrix[:2, :2] = np.array([[cs, -sn], [sn, cs]])@matrix[:2, :2]
        return [dict(center=np.zeros(2), matrix=matrix[:2, :2], color=np.ones(4),
                     texture=resources.texture(ref), material=DrawMaterial(resources, root.props)) for ref in frames]
    animation = None
    for kind, attrs, props in loader.document(scene).sections:
        if attrs.get('type') == 'Animation' and _property(props, 'resource_name') == 'vfx_attack_lightning':
            animation = props; break
    if animation is None:
        raise ValueError('Original lightning AnimationPlayer clip is missing')
    textures = _property(animation, 'tracks/0/keys'); positions = _property(animation, 'tracks/1/keys')
    times = sequence(textures['times'])
    if len(times) != effect.frames or any(abs(float(time)-i/15)>1e-6 for i, time in enumerate(times)):
        raise ValueError('Original lightning timing changed')
    return [dict(center=_vector(position), matrix=np.eye(2), color=np.ones(4),
                 texture=resources.texture(ref), material=DrawMaterial(resources, {}))
            for ref, position in zip(textures['values'], positions['values'])]


def spine_frames(pack, sampler, effect, times):
    loader = SceneLoader(pack); scene = 'scenes/vfx/vfx_'+effect.scene+'.tscn'
    root = loader.instantiate(scene)
    node = next(child for child in root.children if child.kind == 'SpineSprite')
    resource = loader.path(_property(node.props, 'skeleton_data_res'))
    clip = _property(root.props, '_animation')
    metadata = sampler.metadata(resource)
    source = next(row for row in metadata['clips'] if row['name'] == clip)
    if abs(float(source['duration'])-effect.lifetime_ms/1000) > .0011:
        raise ValueError('Architect Spine VFX duration changed')
    flip = np.diag([1., -1., 1.])
    transform = flip@_node_matrix(root.props)@_node_matrix(node.props)@flip
    frames = sampler.sample(resource, clip, times, skins=['default'])
    for frame in frames:
        draw = sampler.draw(resource, frame, transform)
        if any(row[5] != 0 for row in draw):
            raise ValueError('Architect Spine effect acquired a separate blend plane')
        yield {0: raster(draw, DENSITY, effect.anchor, CANVAS)}


def emitter_report(systems):
    return [dict(node=key, source=emitter.node.scope, amount=emitter.amount,
        start_ms=round(emitter.start*1000), lifetime=emitter.lifetime, speed_scale=emitter.speed,
        fixed_fps=int(_property(emitter.node.props, 'fixed_fps', 30)),
        texture=emitter.resources.loader.path(_property(emitter.node.props, 'texture')),
        shader=emitter.draw.shader or 'CanvasItemMaterial', blend=emitter.draw.blend,
        flipbook=list(emitter.draw.flip), subemitter=emitter.child.item.path if emitter.child else None,
        subemitter_only=emitter.sub_only, spawned=emitter.spawned,
        visible=emitter.item.visible, turbulence=bool(emitter.prop('turbulence_enabled', False)),
        trail_sections=emitter.trail_sections if emitter.trails else 0,
        trail_lifetime=emitter.trail_lifetime if emitter.trails else 0.)
        for key, emitter in systems.items()]


def source_assets(pck: str | Path, cache: str | Path, *, runtime=None, dotnet='dotnet',
                  effects=None, max_frames=None) -> list[dict]:
    """Return stable auxiliary rows. Optional filters are development-only."""
    cache = Path(cache); cache.mkdir(parents=True, exist_ok=True)
    pack = Pack(pck); sampler = None; rows = []; reports = []
    definitions = directory()
    selected = set(effects) if effects else {effect.name for effect in EFFECTS}
    if selected-{effect.name for effect in EFFECTS}:
        raise ValueError('Unknown Architect effect')
    if max_frames is not None and max_frames < 1:
        raise ValueError('max_frames must be positive')
    try:
        for index, effect in enumerate(EFFECTS):
            if effect.name not in selected: continue
            definition = definitions[index]
            count = effect.frames if max_frames is None else min(max_frames, effect.frames)
            times = [effect.time(frame) for frame in range(count)]
            scene = 'scenes/vfx/vfx_'+effect.scene+'.tscn'
            systems = None
            if effect.name in ('flying_slash', 'scratch'):
                if sampler is None:
                    sampler = SpineSampler(pack, runtime=runtime, dotnet=dotnet, work_dir=cache/'adapter')
                frames = spine_frames(pack, sampler, effect, times)
            elif effect.name in ('slash', 'lightning'):
                sprites = sprite_frames(pack, effect)
                def render_sprites():
                    for frame in range(count):
                        out = Planes(CANVAS, effect.anchor, DENSITY); out.stamp(sprites[frame], times[frame])
                        yield out.images()
                frames = render_sprites()
            else:
                systems, ordered = particle_systems(pack, scene, effect_index=index)
                def render_particles():
                    previous = 0.
                    for time in times:
                        yield sample_particle_frame(systems, ordered, time, previous=previous)
                        previous = time
                frames = render_particles()
            bounds = None; warp_max = [0, 0]; visible_frames = {plane: 0 for plane in effect.planes}
            for frame, planes in enumerate(frames):
                for plane_index, blend in enumerate(effect.planes):
                    image = planes[blend]
                    if blend == 2:
                        array = np.asarray(image); visible = array[..., 3] > 0
                        x = np.rint(array[..., 0].astype(float)*31/255).astype(int)-16
                        y = np.rint(array[..., 1].astype(float)*63/255).astype(int)-32
                        if np.any(visible):
                            warp_max[0] = max(warp_max[0], int(np.abs(x[visible]).max()))
                            warp_max[1] = max(warp_max[1], int(np.abs(y[visible]).max()))
                        if warp_max[1]>15:
                            raise ValueError('Attack warp exceeds the runtime 32-row snapshot window')
                    box = image.getbbox()
                    if box:
                        visible_frames[blend] += 1
                        bounds = box if bounds is None else (min(bounds[0], box[0]), min(bounds[1], box[1]),
                                                             max(bounds[2], box[2]), max(bounds[3], box[3]))
                    key = f'architect-vfx-{effect.name}-{blend}-{frame:03d}'
                    image.save(cache/(key+'.png'))
                    row = write_image(cache, key, image, sparse=True)
                    row.update(auxiliary=definition['first']+plane_index*effect.frames+frame,
                        source=scene, effect=index, effect_name=effect.name, frame=frame,
                        frame_count=effect.frames, fps=effect.fps, frame_ms=effect.frame_ms,
                        lifetime_ms=effect.lifetime_ms, anchor=list(effect.anchor), density=DENSITY, blend=blend)
                    if blend == 2: row['warp_encoding'] = WARP_ENCODING
                    rows.append(row)
            reports.append(dict(**definition, sampled_frames=count, visible_frames=visible_frames,
                visible_bounds=bounds, warp_max_abs=warp_max,
                systems=emitter_report(systems) if systems else [],
                physics_hz=60 if systems else None,
                sampling_seed='fixed cosmetic seed, independent of gameplay RNG',
                turbulence='bounded CPU curl field; original strengths and life curves' if systems and
                    any(emitter.prop('turbulence_enabled', False) for emitter in systems.values()) else 'none'))
        rows.sort(key=lambda row: row['auxiliary'])
        full = effects is None and max_frames is None
        if full and [row['auxiliary'] for row in rows] != list(range(BASE_AUXILIARY, AUXILIARY_COUNT)):
            raise ValueError('Architect VFX auxiliary directory changed')
        (cache/'assets.json').write_text(json.dumps(rows, indent=2)+'\n')
        (cache/'source-report.json').write_text(json.dumps(dict(reference='v0.111.0 / 41cef1ea',
            full_sampling=full, auxiliary_first=BASE_AUXILIARY, auxiliary_count=AUXILIARY_COUNT,
            effects=reports, screen_displacement=WARP_ENCODING,
            adaptations=['60-Hz CPU integration with 30-Hz output, source sprite frame changes retained',
                         'fixed cosmetic seed replaces unrecorded GPU/System.Random states',
                         'GPU turbulence uses a bounded deterministic CPU curl field',
                         'screen displacement rounds to integer LCD pixels; no distortion mask is painted']),
            indent=2)+'\n')
        return rows
    finally:
        if sampler is not None: sampler.close()
        pack.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', required=True)
    parser.add_argument('--cache', required=True)
    parser.add_argument('--runtime')
    parser.add_argument('--dotnet', default='dotnet')
    parser.add_argument('--effect', action='append', choices=[effect.name for effect in EFFECTS])
    parser.add_argument('--max-frames', type=int)
    args = parser.parse_args()
    rows = source_assets(args.pck, args.cache, runtime=args.runtime, dotnet=args.dotnet,
                         effects=args.effect, max_frames=args.max_frames)
    print(json.dumps(dict(rows=len(rows), first=min(row['auxiliary'] for row in rows),
                         last=max(row['auxiliary'] for row in rows))))


if __name__ == '__main__':
    main()
