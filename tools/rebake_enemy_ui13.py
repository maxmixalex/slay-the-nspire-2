#!/usr/bin/env python3
"""Resample original enemy clips at LCD density with independent registration.

The original skeleton, skin, conditional tracks, physics stepping, frame count
and clock remain authoritative. Each clip gets its own bounded canvas; a distant
tail in one action can no longer shrink the pixels of every idle/hurt state.
All output stays private. Replaced sparse tiles are recycled, and unselected
images, old index groups, localized text and fonts are independently compared.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import time

import numpy as np
import content_tiles as tiles
from pck import Pack
from retile_appended_resources import view, emit, compress_plan, rewrite
from retile_character_ui12 import make_plan, verify
from world_enemy_assets import SpineSampler, _compose
from world_enemy_profile import plan as enemy_plan
from world_raster import raster

ROOT = Path(__file__).resolve().parents[1]
LCD_DENSITY = 4 / 21
SOURCE_FILES = ('tools/rebake_enemy_ui13.py', 'tools/world_enemy_assets.py',
                'tools/world_raster.py', 'tools/SpineSampleAdapter.cs',
                'tools/animation.py', 'tools/build_content.py', 'tools/build_world.py',
                'config/enemy-visuals-0.17.json')


def clip_layout(bounds, density=LCD_DENSITY):
    """Keep source registration exact while fitting a single full clip.

    The LCD camera projects four pixels per 21 source units. Sample up to
    that density, subject to the existing 320x218 image and pivot limits.
    Integer units and integer pivots remove per-clip inverse-scale rounding
    from the runtime. Every sampled source vertex retains two pixels of pad.
    """
    points = np.asarray(bounds, dtype=float)
    tiles.require(points.shape == (2, 2) and np.all(np.isfinite(points)),
                  'Clip bounds must be finite two-dimensional endpoints')
    low, high = points
    tiles.require(np.all(high >= low) and math.isfinite(density) and density > 0,
                  'Invalid clip bounds or source density')
    span = np.maximum(high-low, 1)
    scale = min(density, 315/span[0], 213/span[1])
    if low[0]:scale = min(scale, 638/abs(low[0]))
    if high[1]:scale = min(scale, 478/abs(high[1]))
    units = math.ceil(1024/scale)
    tiles.require(0 < units < 1000000, 'Clip scale exceeds runtime units bound')
    scale = 1024/units
    origin = [math.ceil(2-low[0]*scale), math.ceil(2+high[1]*scale)]
    size = [math.ceil(high[0]*scale+origin[0])+2,
            math.ceil(origin[1]-low[1]*scale)+2]
    tiles.require(0 < size[0] <= 320 and 0 < size[1] <= 218 and
                  -640 <= origin[0] <= 640 and -480 <= origin[1] <= 480,
                  'Clip layout exceeds the bounded runtime canvas')
    return dict(scale=scale, sprite_units=units, origin=origin, size=size,
                bounds=points.tolist(), crop=False)


def bake_job(pack, sampler, model, job):
    """One source sampling pass supplies both bounds and final raster pixels."""
    from build_world import job_state
    times = [i*job['duration']/job['frames'] for i in range(job['frames'])]
    state = job_state(job)
    samples = _compose(sampler, model, job['clip'], times, job['skins'], state)
    vertices = [layer[0] for draw in samples for layer in draw if len(layer[0])]
    tiles.require(bool(vertices), 'Original clip has no skeletal geometry')
    low = np.asarray([row.min(axis=0) for row in vertices]).min(axis=0)
    high = np.asarray([row.max(axis=0) for row in vertices]).max(axis=0)
    layout = clip_layout([low, high])
    images = [raster(draw, layout['scale'], layout['origin'], tuple(layout['size']))
              for draw in samples]
    metadata = dict(key=job['key'], clip=job['id'], model=job['model'],
                    frames=job['frames'], frame_ms=max(1, round(job['duration']*1000/job['frames'])),
                    duration_ms=max(1, round(job['duration']*1000)),
                    original_ms=max(1, round(job['original_duration']*1000)),
                    layout=layout, skins=job['skins'], visual_state=state,
                    physics_step=1/60, runtime_sha256=sampler.runtime_identity)
    return images, metadata


def cache_identity(pack, runtime):
    digest = hashlib.sha256()
    for name in SOURCE_FILES:digest.update((ROOT/name).read_bytes())
    digest.update(Path(runtime).read_bytes())
    for key, entry in sorted(pack.entries.items()):
        digest.update(key.encode());digest.update(entry.md5)
    return digest.hexdigest()


def source_frames(source, pack, runtime, dotnet, work, models):
    catalog = enemy_plan()
    by_name = {row['model']: row for row in catalog['models']}
    tiles.require(set(models) <= set(by_name), 'Unknown enemy model requested')
    tiles.require(all(not by_name[name].get('shared_group') for name in models),
                  'Shared scene actors must keep their scene-space registration')
    jobs = [row for row in catalog['jobs'] if row['model'] in models]
    identity = cache_identity(pack, runtime)
    replacements, clip_overrides, receipts = {}, {}, []
    adapter = work/'adapter';cache = work/'sampled';cache.mkdir(parents=True, exist_ok=True)
    sampler = None
    try:
        for index, job in enumerate(jobs):
            directory = job['id'];original = source.clips[directory]
            tiles.require(original[1] == job['frames'] and
                          original[2] == max(1, round(job['duration']*1000/job['frames'])),
                          'Pinned source frame count or timing changed')
            signature = tiles.sha256((identity+json.dumps(job, sort_keys=True)).encode())
            folder = cache/f'{directory:04d}-{signature[:16]}'
            saved = folder/'metadata.json'
            metadata = json.loads(saved.read_text()) if saved.is_file() else None
            paths = [folder/f'{frame:03d}.png' for frame in range(job['frames'])]
            if metadata is None or metadata.get('signature') != signature or not all(p.is_file() for p in paths):
                if sampler is None:
                    sampler = SpineSampler(pack, runtime=runtime, dotnet=dotnet, work_dir=adapter)
                images, metadata = bake_job(pack, sampler, by_name[job['model']], job)
                folder.mkdir(parents=True, exist_ok=True)
                for picture, path in zip(images, paths):picture.save(path)
                metadata['signature'] = signature
                tiles.atomic_bytes(saved, json.dumps(metadata, indent=2).encode()+b'\n')
            layout = metadata['layout']
            for frame, path in enumerate(paths):replacements[original[0]+frame] = path
            clip_overrides[directory] = (*original[:3], *layout['origin'], layout['sprite_units'])
            receipts.append(dict(clip=directory, key=job['key'], frames=original[1],
                                 frame_ms=original[2], old_size=source.images[original[0]][1:3],
                                 new_size=layout['size'], pivot=layout['origin'],
                                 sprite_units=layout['sprite_units'], scale=layout['scale']))
            emit('enemy_source_sample', complete=index+1, total=len(jobs), **receipts[-1])
    finally:
        if sampler is not None:sampler.close()
    return replacements, clip_overrides, receipts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, action='append', required=True)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--dotnet', default='dotnet')
    parser.add_argument('--model', action='append')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=range(1, 5), default=3)
    args = parser.parse_args()
    for path in [*args.input, args.pck, args.runtime, args.output, args.work]:
        tiles.private_path(path, 'Private source-resampled enemy path')
    tiles.require(all(path.resolve().parent != args.output.resolve() for path in args.input),
                  'Output must not overwrite source packs')
    args.output.mkdir(parents=True, exist_ok=True);args.work.mkdir(parents=True, exist_ok=True)
    started = time.monotonic();source = view(args.input[0].read_bytes());pack = Pack(args.pck)
    try:
        replacements, clips, receipts = source_frames(source, pack, args.runtime, args.dotnet,
                                                      args.work, args.model or ['Vantom'])
    finally:pack.close()
    tiles.atomic_bytes(args.work/'source-frames.json', json.dumps(dict(
        replacements=[dict(image=image, png=str(path)) for image, path in replacements.items()],
        clips=clips, receipt=receipts), indent=2).encode()+b'\n')
    plan = make_plan(source, args.work, replacements, replacements, allow_geometry=True,
                     clip_overrides=clips)
    tiles.atomic_bytes(args.work/'plan.json', json.dumps(plan, indent=2).encode()+b'\n')
    encoded = compress_plan(plan, args.work, args.work/'encoded', args.workers)
    reports = []
    for path in args.input:
        original = source if path == args.input[0] else view(path.read_bytes())
        output, size = rewrite(original, plan, encoded)
        checked = verify(original, output, plan, args.work)
        target = args.output/path.name
        tiles.atomic_bytes(target, output)
        reports.append(dict(path=str(target), sha256=tiles.sha256(output), **size, **checked))
        emit('enemy_pack_verified', **reports[-1])
    result = dict(models=args.model or ['Vantom'], clips=len(clips), frames=len(replacements),
                  elapsed_seconds=round(time.monotonic()-started, 3),
                  source_samples=receipts, outputs=reports)
    tiles.atomic_bytes(args.output/'enemy-rebake-receipt.json', json.dumps(result, indent=2).encode()+b'\n')
    emit('enemy_rebake_complete', clips=len(clips), frames=len(replacements),
         elapsed_seconds=result['elapsed_seconds'])


if __name__ == '__main__':main()
