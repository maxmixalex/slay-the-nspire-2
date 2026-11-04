#!/usr/bin/env python3
"""Populate verified original-enemy frame caches using bounded worker processes.

This produces no pack and does not change the serial directory assembly order.
The normal build_world converter remains responsible for complete validation,
capacity checks and final publication. Inputs and outputs must remain private.
"""

from __future__ import annotations

import argparse
import atexit
import multiprocessing
import os
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

import lz4
import numpy as np
from PIL import Image

from nspire_converter.build_content import texture
from nspire_converter.build_world import (
    CACHE_SCHEMA,
    CANONICAL_PLACEMENT,
    KAISER_LAYOUT,
    ConversionCache,
    cached_enemy_clip,
    canonical,
    choose_layout,
    encode_image,
    encoded_frames,
    job_state,
    private_path,
    render_scope,
    sample_enemy_batched,
    source_fingerprint,
)
from nspire_converter.converter_workspace import WorkLock
from nspire_converter.pck import Pack
from nspire_converter.reference_inventory import sha256, verify_release
from nspire_converter.world_enemy_assets import SpineSampler, manifest
from nspire_converter.world_ui_profile import ENEMY_CLIP_ROWS, validate_headers


def unique_atomic_bytes(path: Path, data: bytes) -> None:
    """A producer never shares its temporary filename with another process."""
    path.parent.mkdir(parents=True, exist_ok=True)
    name = None
    try:
        with tempfile.NamedTemporaryFile(
            dir=path.parent, prefix=path.name + ".", suffix=".tmp", delete=False
        ) as stream:
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
    def put(self, kind, settings, metadata, payload=b"", *, scope=None, reused_from=None):
        # Exact schema/key/receipt protocol of the serial converter, with
        # independent temporary files when its forward scan meets our tail.
        scope = scope or {}
        key = self.key(kind, settings, scope)
        receipt = self.directory / kind / (key + ".json")
        unique_atomic_bytes(receipt.with_suffix(".bin"), payload)
        unique_atomic_bytes(
            receipt,
            canonical(
                dict(
                    schema=CACHE_SCHEMA,
                    key=key,
                    bytes=len(payload),
                    metadata=metadata,
                    scope=scope,
                    reused_from=reused_from,
                )
            )
            + b"\n",
        )
        return metadata, payload


def open_cache(directory, dependencies, sources):
    directory = private_path(Path(directory), "Cache")
    directory.mkdir(parents=True, exist_ok=True)
    # ConversionCache's common dependency receipt is initialized once at a
    # time. flock is released by the OS even if a worker is interrupted.
    with WorkLock(directory / "prefetch-init", blocking=True):
        return PrefetchCache(directory, dependencies, source_files=sources)


_worker = None


def initialize_worker(game, directory, dependencies, sources, runtime, dotnet, fastnoise):
    global _worker
    cache = open_cache(directory, dependencies, sources)
    pck = Pack(Path(game) / "SlayTheSpire2.pck")
    sampler = SpineSampler(
        pck,
        runtime=runtime,
        dotnet=dotnet,
        texture=texture,
        fastnoise_header=fastnoise,
        work_dir=cache.directory / ("prefetch-runtime-" + str(os.getpid())),
    )
    atexit.register(pck.file.close)
    atexit.register(sampler.close)
    models = {row["model"]: row for row in manifest()["models"]}
    scopes = {name: render_scope("clip", sources, model=model) for name, model in models.items()}
    _worker = cache, pck, sampler, models, scopes, sources


def prefetch_job(item):
    number, job, layout = item
    cache, pck, sampler, models, scopes, sources = _worker
    settings = dict(job=job, visual_state=job_state(job, layout))
    scope = scopes[job["model"]]
    cached = cached_enemy_clip(cache, settings, sources, models[job["model"]])
    if cached is not None:
        return number, job["key"], "reused", len(cached[1])
    images, metadata = sample_enemy_batched(pck, models[job["model"]], job, sampler, layout=layout)
    if (
        metadata["frames"] != job["frames"]
        or metadata["size"] != layout["size"]
        or not 0 < metadata["frame_ms"] <= 1000
        or abs(metadata["duration_ms"] - round(job["duration"] * 1000)) > 1
    ):
        raise ValueError("Prefetched clip differs from the original directory: " + job["key"])
    payload = encoded_frames([encode_image(image, animation=True) for image in images])
    # Concurrent production must agree exactly; never replace divergent art
    # merely because it has the same settings digest.
    other = cache.get("clip", settings, scope=scope)
    if other is not None:
        if canonical(other[0]) != canonical(metadata) or other[1] != payload:
            raise ValueError("Concurrent original-frame producers disagree: " + job["key"])
        return number, job["key"], "matched", len(payload)
    cache.put("clip", settings, metadata, payload, scope=scope)
    return number, job["key"], "baked", len(payload)


def clip_group_key(job, model, layout):
    """Group Kaiser jobs whose selected track replaces the same setup pose.

    The adapter sets setup tracks, then replaces the selected track before its
    first Apply. These shared models have no phase/bone/child drivers that use
    facts separately. Keep all other job and visual inputs in the comparison.
    """
    if (
        model.get("shared_group") != "KaiserCrab"
        or model.get("phase_tracks")
        or model.get("scene_controls")
        or any("skeleton" in layer for layer in model.get("layers", []))
    ):
        return job["key"]
    state = job_state(job, layout)
    setup = state.get("setup_tracks", [])
    if (
        "track" not in state
        or "setup_tracks" not in state
        or any(track.get("queued") for track in setup)
    ):
        return job["key"]
    state.pop("facts", None)
    state["setup_tracks"] = [track for track in setup if track["track"] != state["track"]]
    inputs = {
        key: value for key, value in job.items() if key not in ("key", "id", "when", "visual_state")
    }
    return canonical(dict(job=inputs, visual_state=state))


def prefetch_group(group):
    source, targets = group
    source_result = prefetch_job(source)
    cache, _pck, _sampler, _models, scopes, _sources = _worker
    source_job, source_layout = source[1:]
    scope = scopes[source_job["model"]]
    settings = dict(job=source_job, visual_state=job_state(source_job, source_layout))
    metadata, payload = cache.get("clip", settings, scope=scope)
    source_key = cache.key("clip", settings, scope)
    results = []
    for number, job, layout in targets:
        if number == source[0]:
            results.append(source_result)
            continue
        settings = dict(job=job, visual_state=job_state(job, layout))
        own_metadata = dict(metadata, visual_state=settings["visual_state"])
        other = cache.get("clip", settings, scope=scope)
        if other is not None:
            if canonical(other[0]) != canonical(own_metadata) or other[1] != payload:
                raise ValueError("Equivalent original-frame producers disagree: " + job["key"])
        else:
            cache.put(
                "clip",
                settings,
                own_metadata,
                payload,
                scope=scope,
                reused_from=dict(pipeline="equivalent-kaiser-tracks", key=source_key),
            )
        results.append((number, job["key"], "reused", len(payload)))
    return results


def measured_layouts(cache, sources, models):
    layouts = {}
    for name, model in models.items():
        jobs = [job for job in ENEMY_CLIP_ROWS if job["model"] == name]
        if not jobs:
            continue
        if model.get("shared_group"):
            layouts[name] = dict(KAISER_LAYOUT)
            continue
        scope = render_scope("bounds", sources, model=model)
        bounds = []
        for job in jobs:
            record = cache.get("bounds", dict(job=job, placement=CANONICAL_PLACEMENT), scope=scope)
            if record is None:
                raise ValueError("Run the verified serial measurement stage first: " + job["key"])
            bounds.append(record[0])
        layouts[name] = choose_layout(bounds)
    return layouts


def prefetch(cache, game, runtime, dotnet, fastnoise, layouts, workers=2, *, first=1, last=None):
    """Fill missing complete jobs before the ordered world-pack assembly."""
    last = last or len(ENEMY_CLIP_ROWS)
    sources = cache.source_files
    models = {row["model"]: row for row in manifest()["models"]}
    groups = {}
    for number in range(last - 1, first - 2, -1):
        job = ENEMY_CLIP_ROWS[number]
        layout = layouts[job["model"]]
        item = number, job, layout
        key = clip_group_key(job, models[job["model"]], layout)
        group = groups.setdefault(key, dict(source=None, targets=[]))
        settings = dict(job=job, visual_state=job_state(job, layout))
        if cached_enemy_clip(cache, settings, sources, models[job["model"]]) is None:
            group["targets"].append(item)
        elif group["source"] is None:
            group["source"] = item
    pending = [
        (group["source"] or group["targets"][0], group["targets"])
        for group in groups.values()
        if group["targets"]
    ]
    missing = sum(len(targets) for _source, targets in pending)
    print(
        f"Prefetch: {missing} missing clips in {len(pending)} render groups in rows {first}..{last}; {workers} workers; cache {cache.identity}",
        flush=True,
    )
    report = dict(
        workers=workers,
        requested_jobs=last - first + 1,
        missing_jobs=missing,
        render_groups=len(pending),
        baked=0,
        reused=0,
        matched=0,
    )
    if pending:
        with ProcessPoolExecutor(
            max_workers=workers,
            mp_context=multiprocessing.get_context("spawn"),
            initializer=initialize_worker,
            initargs=(
                str(game),
                str(cache.directory.parent),
                cache.dependencies,
                sources,
                str(runtime),
                dotnet,
                str(fastnoise),
            ),
        ) as pool:
            completed = 0
            for results in pool.map(prefetch_group, pending):
                for number, key, status, size in results:
                    completed += 1
                    report[status] += 1
                    print(
                        f"Prefetch {completed}/{missing} clip {number + 1}: {key}: {status}, {size} bytes",
                        flush=True,
                    )
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game", type=Path)
    parser.add_argument("--cache", type=Path, required=True)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--dotnet", required=True)
    parser.add_argument("--fastnoise-header", type=Path, required=True)
    parser.add_argument("--workers", type=int, choices=range(1, 5), default=2)
    parser.add_argument(
        "--clip-range",
        type=int,
        nargs=2,
        metavar=("FIRST", "LAST"),
        help="Populate only these inclusive one-based enemy job rows; renderer/layout identities stay unchanged",
    )
    args = parser.parse_args()
    first, last = args.clip_range or (1, len(ENEMY_CLIP_ROWS))
    if not 1 <= first <= last <= len(ENEMY_CLIP_ROWS):
        parser.error(
            "--clip-range must be ordered enemy job rows within 1.." + str(len(ENEMY_CLIP_ROWS))
        )
    game = private_path(args.game, "Original game")
    runtime = private_path(args.runtime, "Spine runtime")
    fastnoise = args.fastnoise_header.resolve(strict=True)
    validate_headers()
    _lock, _dll, hashes = verify_release(game, cache_identity=True)
    sources = source_fingerprint()
    dependencies = dict(
        schema=CACHE_SCHEMA,
        pck=hashes["pck"],
        spine_runtime=sha256(runtime),
        fastnoise_header=sha256(fastnoise),
        python=sys.version,
        numpy=np.__version__,
        pillow=Image.__version__,
        lz4=lz4.__version__,
    )
    cache = open_cache(args.cache, dependencies, sources)
    models = {row["model"]: row for row in manifest()["models"]}
    layouts = measured_layouts(cache, sources, models)
    prefetch(
        cache, game, runtime, args.dotnet, fastnoise, layouts, args.workers, first=first, last=last
    )
    print(
        "Complete: cache population only; final pack validation still belongs to build_world.",
        flush=True,
    )


if __name__ == "__main__":
    main()
