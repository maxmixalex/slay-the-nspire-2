#!/usr/bin/env python3
"""Prefill private original rest-actor caches without assembling a world pack.

The serial converter's complete jobs, renderer, dependency scopes, encoding and
cache protocol remain unchanged. Only one or two worker processes are allowed.
Run from a frozen, verified source checkpoint; final pack validation still
belongs to build_world. No original inputs or generated art belong in Git.
"""
from __future__ import annotations

import argparse
import atexit
from concurrent.futures import ProcessPoolExecutor
from contextlib import contextmanager
import fcntl
import hashlib
import multiprocessing
import os
from pathlib import Path
import sys

import lz4
import numpy as np
from PIL import Image

from build_content import texture
from build_world import (CACHE_SCHEMA, canonical, decoded_frames, encode_image,
    encoded_frames, private_path, render_scope, sample_rest_actor, source_fingerprint)
from pck import Pack
from prefetch_world_clips import open_cache
from reference_inventory import sha256, verify_release
from world_enemy_assets import RUNTIME_SHA256, SpineSampler
from world_ui_profile import REST_ACTOR_CLIP_ROWS, validate_headers


# Includes the serial converter's seven profile checks, plus the source/job
# identity fields already emitted by the unchanged rest sampler.
PROFILE_FIELDS = ('frames', 'duration_ms', 'frame_ms', 'size', 'pivot', 'blend',
    'plane_encoding', 'resource', 'actor', 'node', 'clip', 'act_index', 'dark',
    'plane', 'viewport', 'screen_origin', 'background', 'loop', 'shader_time', 'phase')


def unchanged_sources(sources):
    if source_fingerprint() != sources:
        raise ValueError('Public source changed during rest prefetch; success was not published')


def validate_record(job, record):
    metadata, payload = record
    if (any(metadata.get(key) != job[key] for key in PROFILE_FIELDS) or
            metadata.get('source_duration_seconds') != job['duration'] or
            metadata.get('actor_scene') != job['path'] or
            metadata.get('runtime_sha256') != RUNTIME_SHA256):
        raise ValueError('Cached rest actor plane differs from its original profile: ' + job['key'])
    frames = decoded_frames(payload, job['frames'])
    if any((width, height) != tuple(job['size']) for width, height, _fmt, _raw in frames):
        raise ValueError('Cached rest actor frame dimensions differ from its profile: ' + job['key'])
    return metadata, payload


def cached_rest_clip(cache, job, scope):
    record = cache.get('clip', job, scope=scope)
    return None if record is None else validate_record(job, record)


def record_signature(record):
    metadata, payload = record
    return (hashlib.sha256(canonical(metadata)).hexdigest(),
            hashlib.sha256(payload).hexdigest())


@contextmanager
def job_lock(cache, job, scope):
    """Coordinate this prefetcher's workers without changing the cache schema.

    The existing serial converter does not take this lock. Its possible race
    is checked before/after publication and in the final scan; this is not a
    transactional guarantee against a noncooperating external writer. Every
    observed producer must agree on the complete metadata and payload bytes.
    """
    directory = cache.directory / 'prefetch-rest-locks'
    directory.mkdir(parents=True, exist_ok=True)
    with (directory / (cache.key('clip', job, scope) + '.lock')).open('a') as guard:
        fcntl.flock(guard.fileno(), fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(guard.fileno(), fcntl.LOCK_UN)


_worker = None


def initialize_worker(game, directory, dependencies, sources, runtime, dotnet, fastnoise):
    global _worker
    unchanged_sources(sources)
    cache = open_cache(directory, dependencies, sources)
    pck = Pack(Path(game) / 'SlayTheSpire2.pck')
    try:
        sampler = SpineSampler(pck, runtime=runtime, dotnet=dotnet, texture=texture,
            fastnoise_header=fastnoise,
            work_dir=cache.directory / ('prefetch-rest-runtime-' + str(os.getpid())))
    except BaseException:
        pck.close()
        raise
    atexit.register(pck.close)
    atexit.register(sampler.close)
    scope = render_scope('rest-actor-clip', sources)
    _worker = cache, pck, sampler, sources, scope


def prefetch_job(item):
    number, job = item
    cache, pck, sampler, sources, scope = _worker
    with job_lock(cache, job, scope):
        unchanged_sources(sources)
        cached = cached_rest_clip(cache, job, scope)
        if cached is not None:
            unchanged_sources(sources)
            return number, job['key'], 'reused', len(cached[1]), record_signature(cached)
        images, metadata = sample_rest_actor(pck, job, sampler)
        payload = encoded_frames([encode_image(image, animation=True) for image in images])
        validate_record(job, (metadata, payload))
        unchanged_sources(sources)
        other = cached_rest_clip(cache, job, scope)
        if other is not None:
            if canonical(other[0]) != canonical(metadata) or other[1] != payload:
                raise ValueError('Concurrent rest-frame producers disagree: ' + job['key'])
            status = 'matched'
        else:
            cache.put('clip', job, metadata, payload, scope=scope)
            status = 'baked'
        # A reader verifies the actual completed record, not just the value
        # returned by put. In particular no temporary/partial entry is a hit.
        written = cached_rest_clip(cache, job, scope)
        if written is None or canonical(written[0]) != canonical(metadata) or written[1] != payload:
            raise ValueError('Rest-frame cache changed during publication: ' + job['key'])
        unchanged_sources(sources)
        return number, job['key'], status, len(payload), record_signature(written)


def verify_complete(cache, jobs, sources, scope, expected):
    """Require all receipts and unchanged source before reporting cache success."""
    unchanged_sources(sources)
    for number, job in enumerate(jobs):
        record = cached_rest_clip(cache, job, scope)
        if record is None:
            raise ValueError('Rest prefetch finished without a complete clip: ' + job['key'])
        if number not in expected or record_signature(record) != expected[number]:
            raise ValueError('Rest-frame cache changed before final verification: ' + job['key'])
    unchanged_sources(sources)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--dotnet', required=True)
    parser.add_argument('--fastnoise-header', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=(1, 2), default=1)
    args = parser.parse_args(argv)
    game = private_path(args.game, 'Original game')
    runtime = private_path(args.runtime, 'Spine runtime')
    fastnoise = args.fastnoise_header.resolve(strict=True)
    if sha256(runtime) != RUNTIME_SHA256:
        raise ValueError('Unexpected separately supplied Spine runtime')
    validate_headers()
    _lock, _dll, hashes = verify_release(game)
    sources = source_fingerprint()
    dependencies = dict(schema=CACHE_SCHEMA, pck=hashes['pck'],
        spine_runtime=RUNTIME_SHA256, fastnoise_header=sha256(fastnoise),
        python=sys.version, numpy=np.__version__, pillow=Image.__version__, lz4=lz4.__version__)
    cache = open_cache(args.cache, dependencies, sources)
    scope = render_scope('rest-actor-clip', sources)
    pending, expected = [], {}
    # Reverse the serial converter's order so the two producers usually work
    # on different jobs. A future profile cannot silently add another kind.
    for number in range(len(REST_ACTOR_CLIP_ROWS) - 1, -1, -1):
        job = REST_ACTOR_CLIP_ROWS[number]
        if job['kind'] != 'rest-actor-clip':
            raise ValueError('Rest prefetch received a non-rest job')
        unchanged_sources(sources)
        cached = cached_rest_clip(cache, job, scope)
        if cached is None:
            pending.append((number, job))
        else:
            expected[number] = record_signature(cached)
    print(f'Rest prefetch: {len(pending)} missing of {len(REST_ACTOR_CLIP_ROWS)} clips; '
          f'{args.workers} workers; cache {cache.identity}', flush=True)
    if pending:
        with ProcessPoolExecutor(max_workers=args.workers,
                mp_context=multiprocessing.get_context('spawn'), initializer=initialize_worker,
                initargs=(str(game), str(args.cache), dependencies, sources,
                          str(runtime), args.dotnet, str(fastnoise))) as pool:
            for completed, (number, key, status, size, signature) in enumerate(pool.map(prefetch_job, pending), 1):
                expected[number] = signature
                print(f'Rest prefetch {completed}/{len(pending)} clip {number + 1}: '
                      f'{key}: {status}, {size} bytes', flush=True)
    verify_complete(cache, REST_ACTOR_CLIP_ROWS, sources, scope, expected)
    print('Complete: rest cache population only; final pack validation still belongs to build_world.', flush=True)


if __name__ == '__main__':
    main()
