#!/usr/bin/env python3
"""Populate verified original-enemy frame caches using bounded worker processes.

This produces no pack and does not change the serial directory assembly order.
The normal build_world converter remains responsible for complete validation,
capacity checks and final publication. Inputs and outputs must remain private.
"""
from __future__ import annotations

import argparse
import atexit
from concurrent.futures import ProcessPoolExecutor
import fcntl
import hashlib
import json
import multiprocessing
import os
from pathlib import Path
import sys
import tempfile

import lz4
import numpy as np
from PIL import Image

from build_world import (CACHE_SCHEMA, CANONICAL_PLACEMENT, KAISER_LAYOUT,
    ConversionCache, canonical, choose_layout, encode_image, encoded_frames,
    job_state, private_path, render_scope, sample_enemy_batched, source_fingerprint,
    cached_enemy_clip)
from build_content import texture
from pck import Pack
from reference_inventory import sha256, verify_release
from world_enemy_assets import SpineSampler, RUNTIME_SHA256, manifest
from world_ui_profile import ENEMY_CLIP_ROWS, validate_headers


def unique_atomic_bytes(path: Path, data: bytes) -> None:
    """A producer never shares its temporary filename with another process."""
    path.parent.mkdir(parents=True, exist_ok=True)
    name = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name + '.',
                                         suffix='.tmp', delete=False) as stream:
            name = Path(stream.name)
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
        name = None
    finally:
        if name is not None:
            name.unlink(missing_ok=True)


class PrefetchCache(ConversionCache):
    def put(self, kind, settings, metadata, payload=b'', *, scope=None, reused_from=None):
        # Exact schema/key/receipt protocol of the serial converter, with
        # independent temporary files when its forward scan meets our tail.
        scope = scope or {}
        key = self.key(kind, settings, scope)
        receipt = self.directory / kind / (key + '.json')
        unique_atomic_bytes(receipt.with_suffix('.bin'), payload)
        unique_atomic_bytes(receipt, canonical(dict(schema=CACHE_SCHEMA,
            key=key, bytes=len(payload),
            metadata=metadata, scope=scope, reused_from=reused_from)) + b'\n')
        return metadata, payload


def open_cache(directory, dependencies, sources):
    directory = private_path(Path(directory), 'Cache')
    directory.mkdir(parents=True, exist_ok=True)
    # ConversionCache's common dependency receipt is initialized once at a
    # time. flock is released by the OS even if a worker is interrupted.
    with (directory / 'prefetch-init.lock').open('a') as guard:
        fcntl.flock(guard.fileno(), fcntl.LOCK_EX)
        return PrefetchCache(directory, dependencies, source_files=sources)


_worker = None


def initialize_worker(game, directory, dependencies, sources, runtime, dotnet, fastnoise):
    global _worker
    cache = open_cache(directory, dependencies, sources)
    pck = Pack(Path(game) / 'SlayTheSpire2.pck')
    sampler = SpineSampler(pck, runtime=runtime, dotnet=dotnet, texture=texture,
        fastnoise_header=fastnoise,
        work_dir=cache.directory / ('prefetch-runtime-' + str(os.getpid())))
    atexit.register(pck.file.close)
    atexit.register(sampler.close)
    models = {row['model']: row for row in manifest()['models']}
    scopes = {name: render_scope('clip', sources, model=model) for name, model in models.items()}
    _worker = cache, pck, sampler, models, scopes, sources


def prefetch_job(item):
    number, job, layout = item
    cache, pck, sampler, models, scopes, sources = _worker
    settings = dict(job=job, visual_state=job_state(job, layout))
    scope = scopes[job['model']]
    cached = cached_enemy_clip(cache, settings, sources, models[job['model']])
    if cached is not None:
        return number, job['key'], 'reused', len(cached[1])
    images, metadata = sample_enemy_batched(pck, models[job['model']], job, sampler, layout=layout)
    if (metadata['frames'] != job['frames'] or metadata['size'] != layout['size'] or
            not 0 < metadata['frame_ms'] <= 1000 or
            abs(metadata['duration_ms'] - round(job['duration'] * 1000)) > 1):
        raise ValueError('Prefetched clip differs from the original directory: ' + job['key'])
    payload = encoded_frames([encode_image(image, animation=True) for image in images])
    # Concurrent production must agree exactly; never replace divergent art
    # merely because it has the same settings digest.
    other = cache.get('clip', settings, scope=scope)
    if other is not None:
        if canonical(other[0]) != canonical(metadata) or other[1] != payload:
            raise ValueError('Concurrent original-frame producers disagree: ' + job['key'])
        return number, job['key'], 'matched', len(payload)
    cache.put('clip', settings, metadata, payload, scope=scope)
    return number, job['key'], 'baked', len(payload)


def measured_layouts(cache, sources, models):
    layouts = {}
    for name, model in models.items():
        jobs = [job for job in ENEMY_CLIP_ROWS if job['model'] == name]
        if not jobs:
            continue
        if model.get('shared_group'):
            layouts[name] = dict(KAISER_LAYOUT)
            continue
        scope = render_scope('bounds', sources, model=model)
        bounds = []
        for job in jobs:
            record = cache.get('bounds', dict(job=job, placement=CANONICAL_PLACEMENT), scope=scope)
            if record is None:
                raise ValueError('Run the verified serial measurement stage first: ' + job['key'])
            bounds.append(record[0])
        layouts[name] = choose_layout(bounds)
    return layouts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--dotnet', required=True)
    parser.add_argument('--fastnoise-header', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=range(1, 5), default=2)
    parser.add_argument('--clip-range', type=int, nargs=2, metavar=('FIRST','LAST'),
                        help='Populate only these inclusive one-based enemy job rows; renderer/layout identities stay unchanged')
    args = parser.parse_args()
    first,last=args.clip_range or (1,len(ENEMY_CLIP_ROWS))
    if not 1<=first<=last<=len(ENEMY_CLIP_ROWS):
        parser.error('--clip-range must be ordered enemy job rows within 1..'+str(len(ENEMY_CLIP_ROWS)))
    game = private_path(args.game, 'Original game')
    runtime = private_path(args.runtime, 'Spine runtime')
    fastnoise = args.fastnoise_header.resolve(strict=True)
    validate_headers()
    _lock, _dll, hashes = verify_release(game, cache_identity=True)
    sources = source_fingerprint()
    dependencies = dict(schema=CACHE_SCHEMA, pck=hashes['pck'],
        spine_runtime=sha256(runtime), fastnoise_header=sha256(fastnoise),
        python=sys.version, numpy=np.__version__, pillow=Image.__version__, lz4=lz4.__version__)
    cache = open_cache(args.cache, dependencies, sources)
    models = {row['model']: row for row in manifest()['models']}
    layouts = measured_layouts(cache, sources, models)
    pending = []
    for number in range(len(ENEMY_CLIP_ROWS) - 1, -1, -1):
        if not first<=number+1<=last:
            continue
        job = ENEMY_CLIP_ROWS[number]
        scope = render_scope('clip', sources, model=models[job['model']])
        settings = dict(job=job, visual_state=job_state(job, layouts[job['model']]))
        if cached_enemy_clip(cache, settings, sources, models[job['model']]) is None:
            pending.append((number, job, layouts[job['model']]))
    print(f'Prefetch: {len(pending)} missing clips in rows {first}..{last}; {args.workers} workers; cache {cache.identity}', flush=True)
    with ProcessPoolExecutor(max_workers=args.workers,
            mp_context=multiprocessing.get_context('spawn'), initializer=initialize_worker,
            initargs=(str(game), str(args.cache), dependencies, sources,
                      str(runtime), args.dotnet, str(fastnoise))) as pool:
        for completed, (number, key, status, size) in enumerate(pool.map(prefetch_job, pending), 1):
            print(f'Prefetch {completed}/{len(pending)} clip {number + 1}: {key}: {status}, {size} bytes', flush=True)
    print('Complete: cache population only; final pack validation still belongs to build_world.', flush=True)


if __name__ == '__main__':
    main()
