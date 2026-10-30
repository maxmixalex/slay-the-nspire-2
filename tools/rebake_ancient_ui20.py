#!/usr/bin/env python3
"""Reproject the eight original Ancient scenes onto the full calculator view.

These replacements retain the established auxiliary and image IDs. The source
viewport matches the original 4:3 layout; the runtime composites its top bar
afterwards, without squeezing an already cropped 320x218 room image to fit.
Only private input/output paths are accepted. No artwork is supplied here.
"""
from __future__ import annotations

import argparse
import json
import mmap
from pathlib import Path

from content_tiles import atomic_bytes, require
from patch_feedback_assets import write_image
from pck import Pack
from retile_appended_resources import view
from world_enemy_assets import SpineSampler
from world_scene_assets import render_scene
from world_ui_profile import AUXILIARY_ROWS, BASE_AUXILIARY


ROOT = Path(__file__).resolve().parents[1]
ANCIENTS = frozenset(('DARV', 'NEOW', 'NONUPEIPE', 'OROBAS', 'PAEL', 'TANX',
                      'TEZCATARA', 'VAKUU'))
SIZE = (320, 240)
VIEWPORT = (1680, 1260)


def private_path(path, name):
    path = Path(path).expanduser().resolve()
    require(path != ROOT and ROOT not in path.parents,
            name + ' must be outside the public source repository')
    return path


def source_assets(pck_path, source_path, cache, *, runtime, dotnet='dotnet'):
    pck_path = private_path(pck_path, 'PCK')
    source_path = private_path(source_path, 'Source pack')
    cache = private_path(cache, 'Output')
    runtime = private_path(runtime, 'Spine runtime')
    cache.mkdir(parents=True, exist_ok=True)
    selected = [(BASE_AUXILIARY + i, row) for i, row in enumerate(AUXILIARY_ROWS)
                if row['key'].startswith('events.') and
                row['key'].split('.', 1)[1] in ANCIENTS]
    require(len(selected) == len(ANCIENTS), 'Incomplete Ancient directory')
    require(all(tuple(row['size']) == SIZE and tuple(row.get('viewport', ())) == VIEWPORT
                for _, row in selected), 'Ancient profile must use the full 4:3 viewport')
    with source_path.open('rb') as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
        source = view(data)
        image_ids = {auxiliary: source.aux[auxiliary] for auxiliary, _ in selected}
    images = []
    pack = Pack(pck_path)
    try:
        with SpineSampler(pack, runtime=runtime, dotnet=dotnet,
                          work_dir=cache/'adapter') as sampler:
            for auxiliary, spec in selected:
                key = spec['key'].split('.', 1)[1].lower()
                image, diagnostics = render_scene(pack, spec['path'], *SIZE,
                    viewport=VIEWPORT, mode='scene', sampler=sampler)
                image.save(cache/(key+'.png'))
                row = write_image(cache, key, image)
                row.update(key=spec['key'], auxiliary=auxiliary,
                           image_id=image_ids[auxiliary], source=spec['path'],
                           viewport=list(VIEWPORT), png=key+'.png',
                           diagnostics=diagnostics)
                images.append(row)
                print(json.dumps(dict(stage='ancient', key=spec['key'],
                    size=list(image.size), bytes=row['bytes'])), flush=True)
        result = dict(schema=1, reference='v0.111.0 / 41cef1ea',
                      viewport=list(VIEWPORT), size=list(SIZE), images=images)
        atomic_bytes(cache/'ancient-ui20.json', json.dumps(result, indent=2).encode()+b'\n')
        return result
    finally:
        pack.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', required=True, type=Path)
    parser.add_argument('--source', required=True, type=Path,
                        help='Existing private NSP2/39 or /40 monolithic pack')
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--runtime', required=True, type=Path)
    parser.add_argument('--dotnet', default='dotnet')
    args = parser.parse_args()
    manifest = source_assets(args.pck, args.source, args.output,
                             runtime=args.runtime, dotnet=args.dotnet)
    print(json.dumps(dict(stage='complete', images=len(manifest['images']),
        bytes=sum(row['bytes'] for row in manifest['images']),
        manifest=str(args.output/'ancient-ui20.json'))), flush=True)


if __name__ == '__main__':
    main()
