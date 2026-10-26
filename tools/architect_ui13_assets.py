"""Original Architect reading tracks and source pen-trail planes.

The caller supplies the private game PCK and reviewed Spine 4.2 runtime.
Returned rows append auxiliary images; no original clip IDs are renumbered.
Reading is composed with the actual simultaneous body track, including the
Silent's four hurt interruptions. Pen positions are sampled from trail_slot
at 60 Hz, retaining the source's last thirty points and both blend planes.
"""
from __future__ import annotations

import json
from pathlib import Path

import numpy as np
from PIL import Image

from pck import Pack
from patch_feedback_assets import write_image
from world_enemy_assets import SpineSampler, _compose, manifest
from world_raster import raster
from world_room_shaders import curve_channel, sample_texture
from world_scene_assets import Call, SceneLoader, _property, load_scene_texture

BASE_AUXILIARY = 2228
READING_COUNTS = (45, 79, 75)
TRAIL_COUNT = 40
AUXILIARY_COUNT = BASE_AUXILIARY + sum(READING_COUNTS) + 2*TRAIL_COUNT
ORIGIN = (251, 136)  # source (1320,830), projected by 4/21, after the top bar
DENSITY = 4/21


def reading_tracks(character: str, fast=False):
    """TheArchitect.PlayCurrentLine's actual two-track command schedule."""
    tracks = []
    stop = 1.75
    if character == 'silent':
        hit = .8 if fast else .85
        step = .15 if fast else .2
        tracks.append(dict(track=0, clip='hurt', loop=False, queued=True, delay=hit))
        tracks.extend(dict(track=0, clip='hurt', loop=False, queued=True, delay=step)
                      for _ in range(3))
        tracks.append(dict(track=0, clip='idle_loop', loop=True, queued=True, delay=.73333335))
        stop = hit+3*step+2
    tracks.extend((dict(track=1, clip='_tracks/head_stop_reading', loop=False,
                        queued=True, delay=stop),
                   dict(track=1, clip='_tracks/head_normal', loop=True,
                        queued=True, delay=.5)))
    return dict(setup_tracks=[dict(track=1, clip='_tracks/head_reading', loop=True)], tracks=tracks)


def _sequence(value):
    return value.args if isinstance(value, Call) else value


def _nodes(root):
    result = {}
    def walk(node, path=''):
        result[path] = node
        for child in node.children:
            walk(child, (path+'/' if path else '')+child.name)
    walk(root)
    return result


class PenTrail:
    def __init__(self, pack, loader, node):
        props = node.props
        self.width = float(_property(props, 'width'))*DENSITY
        self.curve = curve_channel(loader, _property(props, 'width_curve'), 1024)
        self.texture = np.asarray(load_scene_texture(pack, loader.path(_property(props, 'texture'))).convert('RGBA'), dtype=float)/255
        self.tile = int(_property(props, 'texture_mode', 0)) == 1
        _, gradient = loader.resource(_property(props, 'gradient'))
        self.offsets = np.asarray(_sequence(_property(gradient, 'offsets')), dtype=float)
        self.colors = np.asarray(_sequence(_property(gradient, 'colors')), dtype=float).reshape(-1, 4)
        self.outer = node.name == 'TrailOuter'
        self.noise = None
        if self.outer:
            _, material = loader.resource(_property(props, 'material'))
            path = loader.path(_property(material, 'shader_parameter/NoiseTexture'))
            self.noise = np.asarray(load_scene_texture(pack, path).convert('RGBA'), dtype=float)/255
            self.panning = np.asarray(_sequence(_property(material, 'shader_parameter/NoisePanning')), dtype=float)
            self.step = float(_property(material, 'shader_parameter/step'))
            self.spread = float(_property(material, 'shader_parameter/step_spread'))

    def image(self, points, time):
        image = Image.new('RGBA', (320, 218))
        if len(points) < 2:return image
        points = np.asarray(points, dtype=float)
        # Consecutive identical slot poses do not define an extra segment.
        points = points[np.r_[True, np.linalg.norm(np.diff(points, axis=0), axis=1)>1e-6]]
        if len(points) < 2:return image
        low = np.maximum((0, 0), np.floor(points.min(axis=0)-self.width/2-1).astype(int))
        high = np.minimum((320, 218), np.ceil(points.max(axis=0)+self.width/2+1).astype(int))
        if np.any(high <= low):return image
        # Two subpixels in each axis preserve the authored antialiased edge.
        y, x = np.mgrid[low[1]*2:high[1]*2, low[0]*2:high[0]*2]
        pixels = np.stack(((x+.5)/2, (y+.5)/2), axis=-1)
        lengths = np.linalg.norm(np.diff(points, axis=0), axis=1)
        walked = np.r_[0., np.cumsum(lengths)]
        distance = np.full(x.shape, np.inf)
        progress = np.zeros(x.shape)
        side = np.zeros(x.shape)
        for i, length in enumerate(lengths):
            delta = points[i+1]-points[i]
            q = np.clip(((pixels-points[i])*delta).sum(axis=-1)/(length*length), 0, 1)
            residual = pixels-(points[i]+q[..., None]*delta)
            d = np.linalg.norm(residual, axis=-1)
            take = d < distance
            distance[take] = d[take]
            progress[take] = (walked[i]+q[take]*length)/walked[-1]
            side[take] = ((residual[..., 0]*-delta[1]+residual[..., 1]*delta[0])/length)[take]
        radius = np.interp(progress, np.arange(1024)/1024, self.curve)*self.width/2
        coverage = distance <= radius
        v = np.clip(side/np.maximum(radius*2, 1e-8)+.5, 0, 1)
        u = progress
        if self.tile:
            # Line2D.TILE repeats at width * source aspect ratio.
            u = progress*walked[-1]/(self.width*self.texture.shape[1]/self.texture.shape[0])
        uv = np.stack((u, v), axis=-1)
        tex = sample_texture(self.texture, uv, repeat=self.tile)
        color = np.stack([np.interp(progress, self.offsets, self.colors[:, k]) for k in range(4)], axis=-1)
        rgba = tex*color
        if self.outer:
            noise = sample_texture(self.noise, uv+self.panning*time, repeat=True)[..., 0]
            q = np.clip((noise-self.step)/self.spread, 0, 1)
            # The source shader overwrites COLOR.a, rather than multiplying
            # it by the line gradient. Preserve its exact dissolve contract.
            rgba[..., 3] = q*q*(3-2*q)*tex[..., 3]
        rgba[..., 3] *= coverage
        # Downsample premultiplied RGBA so transparent edges stay clean.
        rgba[..., :3] *= rgba[..., 3:4]
        h, w = high[1]-low[1], high[0]-low[0]
        rgba = rgba.reshape(h, 2, w, 2, 4).mean(axis=(1, 3))
        rgba[..., :3] = np.divide(rgba[..., :3], rgba[..., 3:4],
            out=np.zeros_like(rgba[..., :3]), where=rgba[..., 3:4]>0)
        image.paste(Image.fromarray(np.uint8(np.clip(rgba*255, 0, 255))), tuple(low))
        return image


def source_assets(pck: str | Path, cache: str | Path, *, runtime, dotnet='dotnet') -> list[dict]:
    cache = Path(cache);cache.mkdir(parents=True, exist_ok=True)
    rows = [];pack = Pack(pck)
    model = next(row for row in manifest()['models'] if row['model'] == 'Architect')
    def add(key, image, **metadata):
        image.save(cache/(key+'.png'))
        row = write_image(cache, key, image, sparse=True)
        row.update(auxiliary=BASE_AUXILIARY+len(rows), **metadata);rows.append(row)
    try:
        with SpineSampler(pack, runtime=runtime, dotnet=dotnet, work_dir=cache/'adapter') as sampler:
            for branch, character, fast, count in ((0, 'other', False, 45),
                    (1, 'silent', False, 79), (2, 'silent', True, 75)):
                state = reading_tracks(character, fast)
                times = [frame*.05 for frame in range(count)]
                for frame, draw in enumerate(_compose(sampler, model, 'idle_loop', times, ['default'], state)):
                    add(f'architect-reading-{branch}-{frame:03d}', raster(draw, DENSITY, ORIGIN, (320, 218)),
                        source=model['skeleton_path'], state=branch, frame=frame, frame_ms=50,
                        source_tracks=state, source_origin=list(ORIGIN), blend=0)
            # attack emits trail_start at .2s; idle_loop emits trail_end at
            # 2/3s after its 1.5s predecessor. Capture all 60-Hz slot poses.
            times = [i/60 for i in range(131)]
            frames = sampler.sample(model, 'attack', times, skins=['default'],
                setup_tracks=[dict(track=1, clip='_tracks/head_normal', loop=True)],
                tracks=[dict(track=0, clip='idle_loop', loop=True, queued=True, delay=1.5)])
            points = []
            for frame in frames:
                slot = next(row for row in frame['slots'] if row['slot'] == 'trail_slot')
                m = slot['node_matrix']
                points.append(np.array(ORIGIN)+np.array((m[2], m[5]))*.35*DENSITY)
            loader = SceneLoader(pack)
            nodes = _nodes(loader.instantiate('scenes/creature_visuals/architect.tscn'))
            for name, blend in (('TrailOuter', 0), ('TrailInner', 1)):
                trail = PenTrail(pack, loader, nodes['Visuals/TrailSlot/'+name])
                for index in range(TRAIL_COUNT):
                    tick = 12+index*3
                    first = max(12, tick-29)
                    add(f'architect-{name.lower()}-{index:03d}', trail.image(points[first:tick+1], tick/60),
                        source=model['scene_path'], node='Visuals/TrailSlot/'+name, frame=index,
                        start_ms=200, frame_ms=50, end_ms=2167, max_segments=30,
                        source_step_hz=60, blend=blend)
        if len(rows) != AUXILIARY_COUNT-BASE_AUXILIARY:
            raise ValueError('Architect auxiliary row count changed')
        (cache/'assets.json').write_text(json.dumps(rows, indent=2)+'\n')
        return rows
    finally:
        pack.close()
