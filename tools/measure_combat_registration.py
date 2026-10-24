#!/usr/bin/env python3
"""Recover exact actor sampling scales from the owner's pinned resource pack.

Only numeric registration metadata is emitted. The model's existing common
layout is independently remeasured and must match every stored clip's extent
and pivot. No pixel resampling, new pack, or runtime validation is introduced.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import json
import math
import mmap
from pathlib import Path

import numpy as np
import animation
import content_tiles as tiles
from all_character_ui_profile import NEW_CLIPS
from build_content import texture
from build_silent import NEW_CLIPS as SILENT_CLIPS
from build_world import choose_layout, sample_enemy
from patch_phobia_assets import normal_geometry, validate_normal_layout
from pck import Pack
from world_enemy_assets import SpineSampler, manifest
from world_scene_assets import SceneLoader, _node_matrix
from world_ui_profile import ENEMY_CLIP_ROWS


def measure_enemies(pack, source, cache, sampler, prior=None, models=None):
    jobs = defaultdict(list)
    for job in ENEMY_CLIP_ROWS:
        if not job.get('shared_group'):
            jobs[job['model']].append(job)
    result = []
    for model in manifest()['models']:
        name = model['model']
        if models and name not in models:
            continue
        if model.get('shared_group') or name not in jobs:
            result.append(dict(id=model['id'], model=name, sprite_units=0, shared=bool(model.get('shared_group'))))
            continue
        rows = jobs[name]
        geometry_hash = tiles.sha256(json.dumps(normal_geometry(source, rows), separators=(',', ':')).encode())
        stored = cache/('normal-layout-'+name+'.json')
        layout = None
        for candidate in (stored, prior/('normal-layout-'+name+'.json') if prior else stored):
            if not candidate.is_file():
                continue
            saved = json.loads(candidate.read_text())
            if candidate == stored and saved.get('geometry_sha256') != geometry_hash:
                continue
            trial = saved['layout']
            try:
                validate_normal_layout(source, rows, trial)
            except ValueError:
                continue
            # A previous original-geometry measurement can be reused only
            # with exact stored dimensions/pivots for every corresponding clip.
            layout = trial
            break
        if layout is None:
            print('Measure original registration: '+name, flush=True)
            measured = [sample_enemy(pack, model, job, sampler, measure=True)[1] for job in rows]
            layout = choose_layout(measured)
        validate_normal_layout(source, rows, layout)
        saved = dict(geometry_sha256=geometry_hash, layout=layout)
        tiles.atomic_bytes(stored, json.dumps(saved, indent=2).encode()+b'\n')
        # Q10 source units per one baked pixel. The renderer applies its
        # ordinary original-scene projection to this number once per binding.
        units = round(1024/layout['scale'])
        tiles.require(0 < units < 1000000, 'Unbounded sprite registration')
        result.append(dict(id=model['id'], model=name, sprite_units=units, layout=layout, checked_clips=len(rows)))
        print(json.dumps(dict(model=name, sprite_units=units, checked_clips=len(rows))), flush=True)
    return result


def player_transform(pack, name):
    root = SceneLoader(pack).instantiate('scenes/creature_visuals/'+name+'.tscn')
    matches = []
    def walk(node, transform):
        current = transform @ _node_matrix(node.props)
        if node.kind == 'SpineSprite' and node.name == 'Visuals':
            matches.append(current)
        for child in node.children:
            walk(child, current)
    walk(root, np.eye(3))
    tiles.require(len(matches) == 1, 'Ambiguous player skeleton: '+name)
    matrix = matches[0]
    tiles.require(abs(matrix[0, 1]) < .00001 and abs(matrix[1, 0]) < .00001 and
                  abs(matrix[0, 0]-matrix[1, 1]) < .00001 and matrix[0, 0] > 0,
                  'Player needs a nonuniform or rotated runtime transform: '+name)
    return matrix


def measure_players(pack, source):
    # These are the exact first cache-populating calls in build_content,
    # build_silent and build_all_characters. Later animations inherit them.
    definitions = [('ironclad', 0, 86, 160, 1, None),
                   ('silent', 11, SILENT_CLIPS[0][2], SILENT_CLIPS[0][3], None, SILENT_CLIPS[0][4])]
    for name in ('defect', 'regent', 'necrobinder'):
        index, row = next((i, row) for i, row in enumerate(NEW_CLIPS) if row[1] == name)
        definitions.append((name, 17+index, row[3], row[4], None, row[5]))
    definitions.append(('osty', 32, 55, 100, 1, None))
    result = []
    previous = animation.raster
    animation.raster = lambda *unused: None
    try:
        for name, clip, height, width, count, fps in definitions:
            sk = animation.Skeleton(pack.read(pack.imported(name+'.skel', '.spskel')))
            cache = {name: (sk, animation.load_textures(pack, name, texture))}
            count = count or max(1, math.ceil(sk.animations['idle_loop']['duration']*fps))
            _, info = animation.bake(pack, name, 'idle_loop', texture, frames=count, height=height, max_width=width, cache=cache)
            scale = cache[(name+'-scale', ())]
            # Reproduce the actual idle clip after its scalar was cached by a
            # still image (Ironclad/Osty); it must match the existing resource.
            first, stored_count, _ms, px, py, _reserved = source.clips[clip]
            _, actual = animation.bake(pack, name, 'idle_loop', texture, frames=stored_count, height=height, max_width=width, cache=cache)
            shapes = {(source.images[i][1], source.images[i][2]) for i in range(first, first+stored_count)}
            tiles.require(shapes == {tuple(actual['size'])} and (px, py) == tuple(actual['pivot']),
                          'Player registration differs from stored pack: '+name)
            matrix = player_transform(pack, name)
            result.append(dict(model=name, clip=clip, sprite_units=round(1024*matrix[0, 0]/scale),
                offset=[float(matrix[0, 2]), float(matrix[1, 2])], source_scale=float(matrix[0, 0]), baked_scale=scale,
                checked_frames=stored_count, stored_size=actual['size'], stored_pivot=actual['pivot']))
    finally:
        animation.raster = previous
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', required=True, type=Path)
    parser.add_argument('--content', required=True, type=Path)
    parser.add_argument('--auxiliary', type=int, default=1945)
    parser.add_argument('--runtime', required=True, type=Path)
    parser.add_argument('--dotnet', default='dotnet')
    parser.add_argument('--cache', required=True, type=Path)
    parser.add_argument('--prior-layouts', type=Path)
    parser.add_argument('--models', nargs='*')
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.cache.mkdir(parents=True, exist_ok=True)
    with args.content.open('rb') as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
        source = tiles.PackView(data, args.auxiliary, versions=tuple(range(14, 29)))
        pack = Pack(args.pck)
        try:
            with SpineSampler(pack, runtime=args.runtime, dotnet=args.dotnet, work_dir=args.cache/'spine-adapter') as sampler:
                enemies = measure_enemies(pack, source, args.cache, sampler, args.prior_layouts, set(args.models or []))
            players = [] if args.models else measure_players(pack, source)
        finally:
            pack.close()
        report = dict(reference='v0.111.0/41cef1ea', content_version=source.version,
                      enemies=enemies, players=players, pixel_payloads_changed=False)
        tiles.atomic_bytes(args.output, json.dumps(report, indent=2).encode()+b'\n')


if __name__ == '__main__':
    main()
