#!/usr/bin/env python3
"""Append the pinned original world to the owner's private NSP2/13 pack.

The output is NSP2/14. Historical payloads and directory indexes are preserved.
Every original used enemy clip, skin and conditional track variant is sampled
through the separately supplied reviewed Spine runtime. No game assets, private
runtime, font, generated pack or cache may be written inside this repository.
"""
from __future__ import annotations

import argparse
import hashlib
import inspect
import json
import math
import os
from pathlib import Path
import struct
import sys
from collections import defaultdict, OrderedDict

import numpy as np
import lz4
from PIL import Image, ImageDraw, ImageFont

from build_characters import KEYWORD_KEYS
from build_content import ROOT, atlas, choose_font, image_data, prepared, texture
from build_expansion import rle
from build_tactics import compact_image
from world_raster import raster
from content_pack14 import PackData, GLYPH_LIMITS
from pck import Pack
from reference_inventory import sha256, verify_release
from sparse_image import encode as sparse_encode
from world_enemy_assets import SpineSampler, bake_enemy, manifest as enemy_manifest, RUNTIME_SHA256
from world_enemy_profile import plan as enemy_plan
from world_scene_assets import render_scene, load_scene_texture
from world_ui_assets import component as ui_component
from world_room_assets import render_room_image, sample_treasure
from world_rest_actors import sample_rest_actor
from world_ui_profile import (BASE_CARDS, BASE_STRINGS, BASE_AUXILIARY, BASE_CLIPS,
    CARDS, STRINGS, AUXILIARY, CLIPS, TEXT_BASE, TEXT_KEYS, AUXILIARY_ROWS, CLIP_ROWS,
    ENEMY_CLIP_ROWS, MAP_CLIP_ROWS, ROOM_CLIP_ROWS, REST_ACTOR_CLIP_ROWS,
    FILE_LIMIT, DIRECTORY_LIMIT, validate_headers)

SOURCE_SHA256 = '4b83320460798a5dd57d4faeee3cdf9613bef7413259ac57b61fb74d109a5917'
IMAGE_LIMIT = 65535
FONT_SIZES = (8, 10, 12, 16, 20)
CACHE_SCHEMA = 2
# Verified checkpoint 5f5f652f. This exact earlier converter is the only v1
# producer accepted for migration; each job's own renderer inputs are checked
# below before its unchanged key and payload digest are accepted.
LEGACY_BUILDER_SHA256 = '0c77076571131dedff2e15cb8edb5ab073da8ff008cd33506c4f2636fb5312aa'
LEGACY_FUNCTIONS = {
    'encode_image': 'b50ef3e45d6e8c7b4680e69a068ddb0f40d8ead738fef5f9f3709a10c7de594f',
    'job_state': '911e69fde9fa183587a737c9cf8216afe991ed7351df6136a1f9e7acb2c53a0d',
    # Exact extractions of the three rendering call sites in the pinned v1
    # producer. Any future change to these functions disables v1 reuse.
    'render_texture': '35b99cee67d672516f439a372575a519c28b96c654266aac9d6ab7aa26d9f6a5',
    'render_world_scene': '3675d1c2eef85357e2881bbe69212ff27f80a11a8f64026ed0722c72d4d80a92',
    'sample_enemy': '7ea8d198f1bd731275e31d8c8a2adca054d773ecd6e3620c653254c2c4c07977',
}
TEXTURE_SOURCES = ('tools/build_content.py', 'tools/setup_pose.py', 'tools/animation.py', 'tools/pck.py')
GEOMETRY_SOURCES = TEXTURE_SOURCES + ('tools/world_enemy_assets.py', 'tools/SpineSampleAdapter.cs')
ENCODER_SOURCES = ('tools/build_tactics.py', 'tools/lz4_block.py', 'tools/sparse_image.py')
REST_ACTOR_PROFILE_FIELDS = ('frames', 'duration_ms', 'frame_ms', 'size', 'pivot', 'blend',
    'plane_encoding', 'resource', 'actor', 'node', 'clip', 'act_index', 'dark',
    'plane', 'viewport', 'screen_origin', 'background', 'loop', 'shader_time', 'phase')
# These pinned cards consist solely of their original keyword text. Their
# localization table intentionally has no .description entry.
KEYWORD_ONLY_CARDS = frozenset(('CLUMSY', 'GREED', 'INJURY', 'POOR_SLEEP',
    'SPORE_MIND', 'SOOT', 'CURSE_OF_THE_BELL', 'FOLLY', 'ASCENDERS_BANE', 'WRITHE'))
# Exact original Kaiser scene/camera placement in the 320 x 218 content stage.
KAISER_LAYOUT = dict(scale=0.1513888888888889,
    origin=[7.702777777777754, 33.70925925925924], size=[320, 218], crop=True)
# Original single-player, single-enemy layout at camera scale 1. This supplies
# an explicit normalized target position to source-driven aim controllers.
CANONICAL_PLACEMENT = dict(creature_position=[1440., 740.],
    creature_local_position=[480., 200.], targets=[[480., 740.]])


def canonical(value) -> bytes:
    return json.dumps(value, ensure_ascii=True, sort_keys=True,
                      separators=(',', ':'), allow_nan=False).encode()


def private_path(path: Path, label: str) -> Path:
    path = path.expanduser().resolve()
    if path == ROOT or ROOT in path.parents:
        raise ValueError(label + ' must be outside the public source repository')
    return path


def atomic_bytes(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + f'.{os.getpid()}.tmp')
    temporary.write_bytes(data)
    temporary.replace(path)


def source_fingerprint() -> dict:
    """Include all public conversion code/config and the runtime glyph corpus."""
    files = {}
    for folder, suffixes in [('tools', {'.py', '.cs'}), ('config', {'.json'}),
                             ('src', {'.cpp', '.inc'}), ('include', {'.hpp', '.inc'})]:
        for path in sorted((ROOT / folder).rglob('*')):
            if path.is_file() and path.suffix in suffixes:
                files[path.relative_to(ROOT).as_posix()] = sha256(path)
    return files


class ConversionCache:
    """Checksummed, content-addressed private records with atomic completion.

    A record is reusable only if its dependency/settings digest and complete
    payload hash match. A process interruption cannot mark a partial job done.
    No pickle or executable serialized input is used.
    """
    def __init__(self, directory: Path, dependencies: dict, *, source_files=None):
        self.dependencies = dependencies
        self.identity = hashlib.sha256(canonical(dependencies)).hexdigest()
        root = private_path(directory, 'Cache')
        self.directory = root / self.identity
        self.directory.mkdir(parents=True, exist_ok=True)
        self.hits = self.misses = self.legacy_hits = 0
        self.source_files = source_files or {}
        self.legacy = []
        if source_files:
            for path in sorted(root.glob('*/dependencies.json')):
                previous = json.loads(path.read_bytes())
                identity = hashlib.sha256(canonical(previous)).hexdigest()
                if (previous.get('schema') != 1 or path.parent.name != identity or
                    previous.get('source', {}).get('tools/build_world.py') != LEGACY_BUILDER_SHA256):
                    continue
                # The common input identity remains exact. Catalog additions,
                # unrelated core files and font changes cannot affect images.
                if all(previous.get(name) == value for name, value in dependencies.items() if name != 'schema'):
                    self.legacy.append((path.parent, identity, previous['source']))
        atomic_bytes(self.directory / 'dependencies.json', canonical(dependencies) + b'\n')

    def key(self, kind: str, settings: dict, scope=None) -> str:
        return hashlib.sha256(canonical(dict(schema=CACHE_SCHEMA, kind=kind,
            dependency=self.identity, settings=settings, scope=scope or {}))).hexdigest()

    def _legacy_matches(self, scope, sources):
        if scope.get('pipeline') not in ('texture-rgba-1', 'scene-rgba-1', 'enemy-bounds-1', 'enemy-rgba-1'):
            return False
        if any(sources.get(name) != value for name, value in scope.get('files', {}).items()):
            return False
        if any(LEGACY_FUNCTIONS.get(name) != value for name, value in scope.get('functions', {}).items()):
            return False
        # V1 bound the whole manifest instead of a single model row. Exact old
        # manifest equality is required for this one-time migration. V2 jobs
        # thereafter bind their own complete numeric model specification.
        if 'model' in scope and sources.get('config/enemy-visuals-0.17.json') != self.source_files.get('config/enemy-visuals-0.17.json'):
            return False
        return True

    @staticmethod
    def _read(receipt, key, schema):
        row = json.loads(receipt.read_bytes())
        if row.get('key') != key or row.get('schema') != schema:
            raise ValueError('Private cache receipt identity mismatch: ' + key)
        payload = receipt.with_suffix('.bin').read_bytes()
        if len(payload) != row['bytes'] or hashlib.sha256(payload).hexdigest() != row['sha256']:
            raise ValueError('Private cache payload is incomplete or changed: ' + key)
        return row['metadata'], payload

    def get(self, kind: str, settings: dict, *, scope=None):
        scope = scope or {}
        key = self.key(kind, settings, scope)
        receipt = self.directory / kind / (key + '.json')
        if not receipt.is_file():
            for directory, identity, sources in self.legacy:
                if not self._legacy_matches(scope, sources):
                    continue
                old_key = hashlib.sha256(canonical(dict(schema=1, kind=kind,
                    dependency=identity, settings=settings))).hexdigest()
                old_receipt = directory / kind / (old_key + '.json')
                if not old_receipt.is_file():
                    continue
                metadata, payload = self._read(old_receipt, old_key, 1)
                self.put(kind, settings, metadata, payload, scope=scope,
                         reused_from=dict(identity=identity, key=old_key, schema=1))
                self.hits += 1
                self.legacy_hits += 1
                return metadata, payload
            self.misses += 1
            return None
        result = self._read(receipt, key, CACHE_SCHEMA)
        self.hits += 1
        return result

    def put(self, kind: str, settings: dict, metadata: dict, payload=b'', *, scope=None, reused_from=None):
        scope = scope or {}
        key = self.key(kind, settings, scope)
        receipt = self.directory / kind / (key + '.json')
        atomic_bytes(receipt.with_suffix('.bin'), payload)
        atomic_bytes(receipt, canonical(dict(schema=CACHE_SCHEMA, key=key,
            bytes=len(payload), sha256=hashlib.sha256(payload).hexdigest(), metadata=metadata,
            scope=scope, reused_from=reused_from)) + b'\n')
        return metadata, payload


def encoded_frames(images: list[tuple]) -> bytes:
    result = bytearray(struct.pack('<I', len(images)))
    for width, height, fmt, raw in images:
        if not (0 < width <= 320 and 0 < height <= 240 and 0 <= fmt <= 5):
            raise ValueError('Frame cache image outside reader bounds')
        result.extend(struct.pack('<HHII', width, height, fmt, len(raw)))
        result.extend(raw)
    return bytes(result)


def decoded_frames(raw: bytes, expected: int) -> list[tuple]:
    if len(raw) < 4 or struct.unpack_from('<I', raw)[0] != expected or not 1 <= expected <= 256:
        raise ValueError('Frame cache count mismatch')
    result, at = [], 4
    for _ in range(expected):
        if at + 12 > len(raw):
            raise ValueError('Truncated frame cache header')
        width, height, fmt, count = struct.unpack_from('<HHII', raw, at)
        at += 12
        if not (0 < width <= 320 and 0 < height <= 240 and fmt <= 5 and
                0 < count <= 320 * 240 * 4 and at + count <= len(raw)):
            raise ValueError('Frame cache image outside reader bounds')
        result.append((width, height, fmt, raw[at:at + count]))
        at += count
    if at != len(raw):
        raise ValueError('Unexpected frame cache suffix')
    return result


def encode_image(image: Image.Image, animation=False) -> tuple:
    image = image.convert('RGBA')
    if animation:
        fmt, raw = compact_image(4, sparse_encode(image))
    else:
        alpha = image.getchannel('A').getextrema()[0] < 255
        fmt, raw = compact_image(int(alpha), image_data(image, alpha))
    return (*image.size, fmt, raw)


def render_texture(pack, path, size, fit, kind, texture_cache):
    image = atlas(pack, path, texture_cache) if kind == 'atlas' else texture(pack, path)
    return prepared(image, tuple(size), fit)


def render_world_scene(pack, row, sampler):
    return render_scene(pack, row['path'], *row['size'], mode=row['kind'],
                        sampler=sampler, plane=row.get('plane', 'all'))


def render_atlas_resource(pack, row, texture_cache):
    image = load_scene_texture(pack, row['path'], cache=texture_cache)
    return prepared(image, tuple(row['size']), row['fit'])


def render_ui_component(pack, row, texture_cache):
    return ui_component(load_scene_texture(pack, row['path'], cache=texture_cache), row)


def sample_enemy(pack, model, job, sampler, *, layout=None, measure=False):
    return bake_enemy(pack, model, job['clip'], sampler=sampler,
        frames=job['frames'], skin_variant=job['skins'],
        visual_state=job_state(job, layout), measure_only=measure)


def sample_enemy_batched(pack, model, job, sampler, *, layout=None):
    """Use identical sampled geometry with bounded batched pixel operations.

    Each converter/prefetch worker executes one job at a time in its own
    process. Restore the reference binding even if rendering raises; bounds
    and independent scene sampling continue to use their original code path.
    """
    import world_enemy_assets as enemy_renderer
    previous = enemy_renderer.raster
    try:
        enemy_renderer.raster = raster
        return sample_enemy(pack, model, job, sampler, layout=layout)
    finally:
        enemy_renderer.raster = previous


def sample_rest_actor_batched(pack, job, sampler):
    """Opt only rest-actor jobs into the checked bounded triangle rasterizer.

    Serial conversion and each independent prefetch process sample one job at
    a time. Scene/treasure callers retain the reference binding, including
    after a failed rest job. This wrapper is not safe for concurrent threads.
    """
    import animation as scene_raster
    previous = scene_raster.raster
    try:
        scene_raster.raster = raster
        return sample_rest_actor(pack, job, sampler)
    finally:
        scene_raster.raster = previous


def sample_map_clip(pack, job, sampler):
    """Preserve the original boss-map RGB mask and the declared scene anchor.

    The original shader colors these channels at presentation time. Baking a
    specific act color here would prevent the same clip from being used on
    other maps or in its visited state. Bounds failures require an explicit
    canvas/anchor correction; no automatic resize or silent crop is allowed.
    """
    resource = job['resource']
    metadata = sampler.metadata(resource)
    clips = {row['name']: row for row in metadata['clips']}
    if job['clip'] not in clips or abs(clips[job['clip']]['duration'] - job['duration']) > 0.00001:
        raise ValueError('Original map animation duration/name drift: ' + job['key'])
    times = [index * job['duration'] / job['frames'] for index in range(job['frames'])]
    sampled = sampler.sample(resource, job['clip'], times, skins=job['skins'], loop=job['loop'])
    if len(sampled) != job['frames']:
        raise ValueError('Map sampler did not return its full declared period')
    matrix = np.asarray(job['transform'], dtype=float).reshape(2, 3)
    low, high = np.full(2, np.inf), np.full(2, -np.inf)
    images = []
    for frame in sampled:
        layers = []
        for vertices, uvs, triangles, pixels, color, blend in sampler.draw(resource, frame):
            points = (matrix @ np.column_stack((vertices, np.ones(len(vertices)))).T).T
            if len(points):
                low, high = np.minimum(low, points.min(axis=0)), np.maximum(high, points.max(axis=0))
            # The manifest transform yields Godot/downwards canvas points;
            # raster performs its own y flip from the geometry convention.
            layers.append((points * [1, -1], uvs, triangles, pixels, color, blend))
        if (not np.all(np.isfinite(low)) or np.any(low < -0.001) or
                np.any(high > np.asarray(job['size']) + 0.001)):
            raise ValueError('Original map animation exceeds its explicit canvas: ' +
                job['key'] + ' bounds=' + str([low.tolist(), high.tolist()]) +
                ' size=' + str(job['size']))
        images.append(raster(layers, 1., np.zeros(2), tuple(job['size'])))
    return images, dict(resource=resource, clip=job['clip'], skins=job['skins'],
        source_version=metadata.get('source_version'), frames=job['frames'],
        duration_ms=job['duration_ms'], frame_ms=job['frame_ms'],
        size=job['size'], pivot=job['pivot'], loop=job['loop'],
        shader=job['shader'], transform=job['transform'],
        bounds=[low.tolist(), high.tolist()], runtime_sha256=RUNTIME_SHA256,
        physics_step=1/60, original_channels=True)


def function_hash(function) -> str:
    return hashlib.sha256(inspect.getsource(function).encode()).hexdigest()


def render_scope(kind: str, source_files: dict, *, model=None) -> dict:
    """Hash the code/data that can actually change this category's pixels."""
    if kind == 'texture':
        paths, functions = TEXTURE_SOURCES + ENCODER_SOURCES, [render_texture, encode_image]
        pipeline = 'texture-rgba-1'
    elif kind == 'scene':
        paths = GEOMETRY_SOURCES + ENCODER_SOURCES + ('tools/world_scene_assets.py',)
        functions, pipeline = [render_world_scene, encode_image], 'scene-rgba-1'
    elif kind == 'atlas-tres':
        paths = TEXTURE_SOURCES + ENCODER_SOURCES + ('tools/world_scene_assets.py',)
        functions, pipeline = [render_atlas_resource, encode_image], 'atlas-tres-rgba-1'
    elif kind == 'ui-component':
        paths = TEXTURE_SOURCES + ENCODER_SOURCES + ('tools/world_scene_assets.py', 'tools/world_ui_assets.py')
        functions, pipeline = [render_ui_component, encode_image], 'ui-component-rgba-1'
    elif kind == 'map-clip':
        paths = GEOMETRY_SOURCES + ENCODER_SOURCES + ('tools/world_raster.py',)
        functions, pipeline = [sample_map_clip, encode_image], 'map-boss-batched-rgba-1'
    elif kind in ('room-image', 'room-clip', 'rest-actor-clip', 'rest-actor-clip-reference'):
        paths = GEOMETRY_SOURCES + ENCODER_SOURCES + (
            'tools/world_scene_assets.py', 'tools/world_room_assets.py',
            'tools/world_room_profile.py', 'config/world-room-ui-0.17.json')
        if kind in ('rest-actor-clip', 'rest-actor-clip-reference'):
            paths += ('tools/world_rest_actors.py', 'tools/world_rest_actor_assets.py',
                      'tools/world_room_shaders.py')
            functions, pipeline = [sample_rest_actor, encode_image], 'rest-actor-plane-rgba-1'
            if kind == 'rest-actor-clip':
                paths += ('tools/world_raster.py',)
                functions += [sample_rest_actor_batched]
                pipeline = 'rest-actor-plane-batched-rgba-1'
        else:
            if kind == 'room-image':
                paths += ('tools/world_room_shaders.py',)
            functions = [render_room_image if kind == 'room-image' else sample_treasure, encode_image]
            pipeline = 'room-image-rgba-1' if kind == 'room-image' else 'treasure-clip-rgba-1'
    elif kind in ('bounds', 'clip', 'clip-reference'):
        pixels = kind != 'bounds'
        paths = GEOMETRY_SOURCES + (ENCODER_SOURCES if pixels else ())
        functions = [sample_enemy, job_state] + ([encode_image] if pixels else [])
        pipeline = 'enemy-rgba-1' if pixels else 'enemy-bounds-1'
        if kind == 'clip':
            paths += ('tools/world_raster.py',)
            functions += [sample_enemy_batched]
            pipeline = 'enemy-batched-rgba-1'
        if model is None:
            raise ValueError('Enemy cache scope requires its entire original model specification')
    else:
        raise ValueError('Unknown rendering dependency scope: ' + kind)
    result = dict(pipeline=pipeline, files={name: source_files[name] for name in paths},
                  functions={function.__name__: function_hash(function) for function in functions})
    if model is not None:
        result['model'] = hashlib.sha256(canonical(model)).hexdigest()
    return result


def cached_enemy_clip(cache, settings, source_files, model):
    """Reuse checked reference pixels, while binding new pixels to new code.

    The reference implementation is retained unchanged. Its complete geometry,
    encoder, model and job identities must match before an old payload is
    imported. A changed batching implementation cannot reuse another batching
    version's output, and never weakens the checksum checks of either record.
    """
    scope = render_scope('clip', source_files, model=model)
    cached = cache.get('clip', settings, scope=scope)
    if cached is not None:
        return cached
    reference = render_scope('clip-reference', source_files, model=model)
    cached = cache.get('clip', settings, scope=reference)
    if cached is not None:
        return cache.put('clip', settings, cached[0], cached[1], scope=scope,
            reused_from=dict(pipeline=reference['pipeline'],
                             key=cache.key('clip', settings, reference)))
    return None


def validate_rest_actor_record(job, record):
    """Check complete original job identity and every encoded frame boundary."""
    metadata, payload = record
    if (any(metadata.get(key) != job[key] for key in REST_ACTOR_PROFILE_FIELDS) or
            metadata.get('source_duration_seconds') != job['duration'] or
            metadata.get('actor_scene') != job['path'] or
            metadata.get('runtime_sha256') != RUNTIME_SHA256):
        raise ValueError('Cached rest actor plane differs from its original profile: ' + job['key'])
    frames = decoded_frames(payload, job['frames'])
    if any((width, height) != tuple(job['size']) for width, height, _fmt, _raw in frames):
        raise ValueError('Cached rest actor frame dimensions differ from its profile: ' + job['key'])
    return metadata, payload


def _checked_rest_actor_record(cache, job, scope):
    record = cache.get('clip', job, scope=scope)
    if record is None:
        return None
    key = cache.key('clip', job, scope)
    receipt = json.loads((cache.directory / 'clip' / (key + '.json')).read_bytes())
    if (receipt.get('scope') != scope or receipt.get('metadata') != record[0] or
            receipt.get('sha256') != hashlib.sha256(record[1]).hexdigest() or
            receipt.get('bytes') != len(record[1])):
        raise ValueError('Rest-frame cache scope or receipt changed during read: ' + job['key'])
    return validate_rest_actor_record(job, record)


def cached_rest_actor_clip(cache, job, scope, *, reference_scope=None):
    """Import only an exact checked reference job, never another batched version.

    The old sampler and its dependency scope remain unchanged. Reference
    migration is explicit, so callers can also use this as a strict current-
    scope reader. As in the prefetch protocol, observed concurrent producers
    must agree; this is not a transaction against noncooperating writers.
    """
    cached = _checked_rest_actor_record(cache, job, scope)
    if cached is not None or reference_scope is None:
        return cached
    if (scope != render_scope('rest-actor-clip', cache.source_files) or
            reference_scope != render_scope('rest-actor-clip-reference', cache.source_files)):
        raise ValueError('Rest reference migration requires exact renderer dependency scopes')
    reference = _checked_rest_actor_record(cache, job, reference_scope)
    if reference is None:
        return None
    other = _checked_rest_actor_record(cache, job, scope)
    if other is not None:
        if canonical(other[0]) != canonical(reference[0]) or other[1] != reference[1]:
            raise ValueError('Concurrent rest-frame producers disagree: ' + job['key'])
    else:
        cache.put('clip', job, reference[0], reference[1], scope=scope,
            reused_from=dict(pipeline=reference_scope['pipeline'],
                             key=cache.key('clip', job, reference_scope)))
    written = _checked_rest_actor_record(cache, job, scope)
    if (written is None or canonical(written[0]) != canonical(reference[0]) or
            written[1] != reference[1]):
        raise ValueError('Rest-frame cache changed during migration: ' + job['key'])
    return written


def card_models() -> list[dict]:
    source = json.loads((ROOT / 'config/world-cards-0.17.json').read_text())
    if source['reference'] != 'v0.111.0 / 41cef1ea':
        raise ValueError('World card reference drift')
    rows = source['cards']
    if [row['id'] for row in rows] != list(range(BASE_CARDS, CARDS)):
        raise ValueError('World card IDs are missing, duplicated or reordered')
    return rows


def card_description(text: str, row: dict, upgraded: bool, keywords: dict) -> str:
    # C++ resolves canonical and live per-instance values. Flattening these
    # branches here would lose Mad Science, Wither levels and upgraded values.
    result = text.replace('\\n', '\n')
    for keyword in row['keywords']['upgraded' if upgraded else 'base']:
        if keyword not in KEYWORD_KEYS:
            raise ValueError('Unsupported original keyword: ' + keyword)
        key = KEYWORD_KEYS[keyword] + '.title'
        if key not in keywords:
            raise ValueError('Original keyword title is missing: ' + key)
        result += ('\n' if result else '') + '[gold]' + keywords[key] + '。[/gold]'
    return result


def retained_prefix(pack: PackData) -> tuple:
    if pack.version != 13 or (len(pack.cards), len(pack.strings), len(pack.aux), len(pack.clips)) != (
            BASE_CARDS, BASE_STRINGS, BASE_AUXILIARY, BASE_CLIPS * 16):
        raise ValueError('World conversion requires the complete historical NSP2/13 profile')
    return (pack.images[:], pack.cards[:], pack.strings[:], pack.aux[:],
            bytes(pack.clips), pack.glyphs.copy())


def assert_retained(pack: PackData, prefix: tuple) -> None:
    images, cards, strings, aux, clips, glyphs = prefix
    if (pack.images[:len(images)] != images or pack.cards[:len(cards)] != cards or
        pack.strings[:len(strings)] != strings or pack.aux[:len(aux)] != aux or
        pack.clips[:len(clips)] != clips or
        any(pack.glyphs.get(key) != value for key, value in glyphs.items())):
        raise ValueError('Historical payload or directory record changed')


def choose_layout(measurements: list[dict]) -> dict:
    """One pivot/scale for every clip, skin, phase and move of one model."""
    if not measurements:
        raise ValueError('A model has no measured original clips')
    bounds = np.asarray([m['bounds'] for m in measurements], dtype=float)
    references = np.asarray([m['reference_bounds'] for m in measurements], dtype=float)
    if not np.all(np.isfinite(bounds)) or not np.all(np.isfinite(references)):
        raise ValueError('Nonfinite original animation bounds')
    low, high = bounds[:, 0].min(axis=0), bounds[:, 1].max(axis=0)
    reference_span = np.maximum(references[:, 1] - references[:, 0], 1).max(axis=0)
    span = np.maximum(high - low, 1)
    natural_scale = min(156 / reference_span[0], 84 / reference_span[1])
    scale = min(natural_scale, 315 / span[0], 213 / span[1])
    size = (np.ceil(span * scale).astype(int) + 4).tolist()
    origin = [2 - low[0] * scale, 2 + high[1] * scale]
    if (size[0] > 320 or size[1] > 218 or not -640 <= round(origin[0]) <= 640 or
            not -480 <= round(origin[1]) <= 480):
        raise ValueError('Common model layout exceeds image or pivot bounds')
    return dict(scale=float(scale), origin=list(map(float, origin)), size=size,
        bounds=[low.tolist(), high.tolist()], crop=False,
        reference_scale=float(natural_scale), fit_ratio=float(scale / natural_scale))


def job_state(job: dict, layout=None) -> dict:
    state = dict(CANONICAL_PLACEMENT)
    state.update(job['visual_state'])
    if layout is not None:
        state['layout'] = layout
    return state


def glyphs(pack: PackData, font_path: Path) -> int:
    alphabet = set(''.join(pack.strings)) | {chr(n) for n in range(32, 127)}
    for folder in ('src', 'include'):
        for path in (ROOT / folder).rglob('*'):
            if path.suffix in ('.cpp', '.hpp', '.inc'):
                alphabet.update(path.read_text())
    alphabet = sorted((char for char in alphabet if ord(char) >= 32), key=ord)
    needed = {(ord(char), px) for char in alphabet for px in FONT_SIZES}
    if len(set(pack.glyphs) | needed) > GLYPH_LIMITS[14]:
        raise ValueError('Five-size glyph directory exceeds the NSP2/14 bound')
    before = len(pack.glyphs)
    for px in FONT_SIZES:
        font = ImageFont.truetype(str(font_path), px)
        for char in alphabet:
            key = (ord(char), px)
            if key in pack.glyphs:
                continue
            advance = max(1, math.ceil(font.getlength(char)))
            width, height = max(advance, font.getbbox(char)[2]), px + 5
            if not (0 < width <= 40 and 0 < height <= 40 and advance <= 40):
                raise ValueError('New glyph exceeds the runtime bitmap bound')
            bitmap = Image.new('L', (width, height))
            ImageDraw.Draw(bitmap).text((0, px + 1), char, font=font, fill=255, anchor='ls')
            raw = bitmap.tobytes()
            runs = rle(raw)
            pack.glyphs[key] = (width, height, advance, len(runs), runs or raw)
    if not needed <= set(pack.glyphs):
        raise ValueError('New original text or visible runtime text lacks glyph coverage')
    return len(pack.glyphs) - before


def directory_bytes(pack: PackData) -> int:
    return (40 + len(pack.images) * 16 + len(pack.glyphs) * 16 +
            len(pack.strings) * 8 + len(pack.clips) + len(pack.cards) * 16 + len(pack.aux) * 4)


def plan_summary() -> dict:
    plan = enemy_plan()
    images = 2638 + CARDS - BASE_CARDS + len(AUXILIARY_ROWS) + sum(j['frames'] for j in CLIP_ROWS)
    worst_directory = (40 + images * 16 + GLYPH_LIMITS[14] * 16 + STRINGS * 8 +
                       CLIPS * 16 + CARDS * 16 + AUXILIARY * 4)
    if images > IMAGE_LIMIT or worst_directory > DIRECTORY_LIMIT:
        raise ValueError('Planned world profile exceeds the image or directory reader bound')
    return dict(cards=CARDS, strings=STRINGS, auxiliary_images=AUXILIARY, clips=CLIPS,
        original_used_enemy_clips=plan['used_clips'], enemy_variants=len(ENEMY_CLIP_ROWS),
        enemy_frames=sum(j['frames'] for j in ENEMY_CLIP_ROWS), map_clips=len(MAP_CLIP_ROWS),
        map_frames=sum(j['frames'] for j in MAP_CLIP_ROWS),
        room_clips=len(ROOM_CLIP_ROWS), room_frames=sum(j['frames'] for j in ROOM_CLIP_ROWS),
        rest_actor_clips=len(REST_ACTOR_CLIP_ROWS),
        rest_actor_frames=sum(j['frames'] for j in REST_ACTOR_CLIP_ROWS),
        expected_images=images,
        maximum_directory_bytes=worst_directory, directory_limit=DIRECTORY_LIMIT,
        file_limit=FILE_LIMIT,
        payload_size='Unknown until all original frames are losslessly encoded')


def build(game: Path, source: Path, output: Path, font_path: Path, *,
          runtime=None, dotnet=None, fastnoise_header=None, cache_dir=None) -> dict:
    output = private_path(output, 'Output')
    cache_dir = private_path(cache_dir or output.parent / 'world-cache', 'Cache')
    source = source.resolve()
    if output == source:
        raise ValueError('Output must not overwrite the historical source pack')
    print('Verifying pinned input hashes and generated directory tables', flush=True)
    _lock, _dll, hashes = verify_release(game)
    if sha256(source) != SOURCE_SHA256:
        raise ValueError('Historical NSP2/13 SHA-256 does not match the completed 0.16 pack')
    validate_headers()
    rows = card_models()
    runtime = Path(runtime or os.environ.get('NSPIRE_SPINE_RUNTIME', ''))
    if not runtime.is_file() or sha256(runtime) != RUNTIME_SHA256:
        raise ValueError('Supply the separately licensed reviewed Spine runtime')
    fastnoise_header = fastnoise_header or os.environ.get('NSPIRE_FASTNOISE_HEADER')
    if not fastnoise_header or not Path(fastnoise_header).is_file():
        raise ValueError('Supply the reviewed Godot FastNoiseLite header for original segment motion')
    source_files = source_fingerprint()
    dependencies = dict(schema=CACHE_SCHEMA, reference=hashes, source_pack=SOURCE_SHA256,
        font=sha256(font_path), spine_runtime=RUNTIME_SHA256,
        fastnoise_header=sha256(Path(fastnoise_header)), source=source_files,
        python=sys.version, numpy=np.__version__, pillow=Image.__version__, lz4=lz4.__version__,
        canonical_placement=CANONICAL_PLACEMENT, shared_kaiser_layout=KAISER_LAYOUT)
    # A later combat/UI repair must not invalidate tens of thousands of
    # unchanged original frames. The final receipt/source guard still records
    # the entire source; cached image geometry depends on this explicit closure.
    cache_dependencies = dict(schema=CACHE_SCHEMA, pck=hashes['pck'],
        spine_runtime=RUNTIME_SHA256, fastnoise_header=dependencies['fastnoise_header'],
        python=dependencies['python'], numpy=dependencies['numpy'], pillow=dependencies['pillow'],
        lz4=dependencies['lz4'])
    cache = ConversionCache(cache_dir, cache_dependencies, source_files=source_files)
    pack = PackData(source.read_bytes())
    prefix = retained_prefix(pack)
    image_meta, animation_meta, layouts = [], [], {}
    unique_images = set(raw for _w, _h, _fmt, raw in pack.images)
    encoded_image_bytes = sum(map(len, unique_images))

    def append_image(row):
        nonlocal encoded_image_bytes
        if len(pack.images) >= IMAGE_LIMIT:
            raise ValueError('Image count exceeds the NSP2/14 reader bound')
        raw = row[3]
        if raw not in unique_images:
            unique_images.add(raw)
            encoded_image_bytes += len(raw)
        if encoded_image_bytes + directory_bytes(pack) > FILE_LIMIT:
            raise ValueError('Unique image payloads alone exceed the NSP2/14 file limit')
        pack.images.append(row)
        return len(pack.images) - 1

    pck = Pack(game / 'SlayTheSpire2.pck')
    try:
        with SpineSampler(pck, runtime=runtime, dotnet=dotnet,
                work_dir=cache.directory / 'runtime', texture=texture,
                fastnoise_header=fastnoise_header) as sampler:
            card_text = json.loads(pck.read('localization/zhs/cards.json'))
            keywords = json.loads(pck.read('localization/zhs/card_keywords.json'))
            texture_scope = render_scope('texture', source_files)
            scene_scope = render_scope('scene', source_files)
            atlas_scope = render_scope('atlas-tres', source_files)
            component_scope = render_scope('ui-component', source_files)
            map_scope = render_scope('map-clip', source_files)
            room_image_scope = render_scope('room-image', source_files)
            room_clip_scope = render_scope('room-clip', source_files)
            rest_actor_scope = render_scope('rest-actor-clip', source_files)
            rest_actor_reference = render_scope('rest-actor-clip-reference', source_files)
            for row in rows:
                key, path = row['key'], row['portrait']
                if key + '.title' not in card_text or (key + '.description' not in card_text and key not in KEYWORD_ONLY_CARDS):
                    raise ValueError('Original world card text is incomplete: ' + key)
                first = len(pack.strings)
                description = card_text.get(key + '.description', '')
                pack.strings.extend([card_text[key + '.title'].replace('\\n', '\n'),
                    card_description(description, row, False, keywords),
                    card_description(description, row, True, keywords)])
                settings = dict(path=path, size=[82, 60], fit='cover')
                cached = cache.get('image', settings, scope=texture_scope)
                if cached is None:
                    image = render_texture(pck, path, (82, 60), 'cover', 'texture', {})
                    cached = cache.put('image', settings, dict(path=path),
                        encoded_frames([encode_image(image)]), scope=texture_scope)
                index = append_image(decoded_frames(cached[1], 1)[0])
                if row['id'] != len(pack.cards):
                    raise ValueError('Appended card directory order changed')
                pack.cards.append((index, first, first + 1, first + 2))
                image_meta.append(dict(kind='portrait', model=row['model'], path=path, image=index))
            if len(pack.strings) != TEXT_BASE:
                raise ValueError('World text base differs from the runtime table')
            tables = {}
            for key in TEXT_KEYS:
                table, name = key.split('.', 1)
                if table not in tables:
                    tables[table] = json.loads(pck.read(f'localization/zhs/{table}.json'))
                if name not in tables[table] or not isinstance(tables[table][name], str):
                    raise ValueError('Original localization key is missing: ' + key)
                pack.strings.append(tables[table][name].replace('\\n', '\n'))
            texture_cache, atlas_texture_cache = {}, OrderedDict()
            for number, row in enumerate(AUXILIARY_ROWS):
                print(f'World image {number + 1}/{len(AUXILIARY_ROWS)} {row["key"]}', flush=True)
                is_tres = row['kind'] == 'atlas' and row['path'].endswith('.tres')
                scope = (room_image_scope if row['kind'] in ('rest-scene', 'treasure-still') else
                    scene_scope if row['kind'] in ('background', 'scene') else
                    component_scope if row['kind'] == 'component' else atlas_scope if is_tres else texture_scope)
                cached = cache.get('image', row, scope=scope)
                if cached is None:
                    if row['kind'] in ('background', 'scene'):
                        image, diagnostics = render_world_scene(pck, row, sampler)
                    elif row['kind'] in ('rest-scene', 'treasure-still'):
                        image, diagnostics = render_room_image(pck, row, sampler)
                    elif row['kind'] == 'component':
                        image, diagnostics = render_ui_component(pck, row, atlas_texture_cache), {}
                    elif is_tres:
                        image, diagnostics = render_atlas_resource(pck, row, atlas_texture_cache), {}
                    elif row['kind'] in ('texture', 'atlas'):
                        image = render_texture(pck, row['path'], row['size'], row['fit'], row['kind'], texture_cache)
                        diagnostics = {}
                    else:
                        raise ValueError('Unsupported world image kind: ' + row['kind'])
                    if row['key'].startswith(('backgrounds.KAISER_CRAB_BOSS', 'background_front.KAISER_CRAB_BOSS')):
                        shared = diagnostics.get('shared_background', [])
                        expected = [KAISER_LAYOUT['scale'], 0, KAISER_LAYOUT['origin'][0],
                                    0, KAISER_LAYOUT['scale'], KAISER_LAYOUT['origin'][1]]
                        if len(shared) != 1 or not np.allclose(shared[0]['screen_transform'], expected, atol=1e-9, rtol=0):
                            raise ValueError('Shared Kaiser original scene placement changed')
                    cached = cache.put('image', row, dict(diagnostics=diagnostics),
                        encoded_frames([encode_image(image)]), scope=scope)
                index = append_image(decoded_frames(cached[1], 1)[0])
                pack.aux.append(index)
                image_meta.append(dict(row, image=index, **cached[0]))

            models = {row['model']: row for row in enemy_manifest()['models']}
            by_model = defaultdict(list)
            for job in ENEMY_CLIP_ROWS:
                by_model[job['model']].append(job)
            # Measure every output frame before rasterizing any clip in a model.
            # This prevents clip, skin and conditional phase changes from moving
            # the actor's pivot or resizing its body.
            for name, jobs in by_model.items():
                model = models[name]
                if model.get('shared_group'):
                    layouts[name] = dict(KAISER_LAYOUT)
                    continue
                measured = []
                bounds_scope = render_scope('bounds', source_files, model=model)
                for index, job in enumerate(jobs):
                    settings = dict(job=job, placement=CANONICAL_PLACEMENT)
                    cached = cache.get('bounds', settings, scope=bounds_scope)
                    if cached is None:
                        print(f'Measure {name} {index + 1}/{len(jobs)} {job["key"]}', flush=True)
                        _, metadata = sample_enemy(pck, model, job, sampler, measure=True)
                        cached = cache.put('bounds', settings, metadata, scope=bounds_scope)
                    measured.append(cached[0])
                layouts[name] = choose_layout(measured)
                atomic_bytes(cache.directory / 'layouts.json', canonical(layouts) + b'\n')

            for number, job in enumerate(ENEMY_CLIP_ROWS):
                print(f'Enemy clip {number + 1}/{len(ENEMY_CLIP_ROWS)} {job["key"]}', flush=True)
                state = job_state(job, layouts[job['model']])
                settings = dict(job=job, visual_state=state)
                scope = render_scope('clip', source_files, model=models[job['model']])
                cached = cached_enemy_clip(cache, settings, source_files, models[job['model']])
                if cached is None:
                    images, metadata = sample_enemy_batched(pck, models[job['model']], job, sampler,
                                                           layout=layouts[job['model']])
                    cached = cache.put('clip', settings, metadata,
                        encoded_frames([encode_image(image, animation=True) for image in images]), scope=scope)
                metadata, payload = cached
                frames = decoded_frames(payload, job['frames'])
                if (metadata['frames'] != job['frames'] or metadata['size'] != layouts[job['model']]['size'] or
                    not 0 < metadata['frame_ms'] <= 1000 or
                    abs(metadata['duration_ms'] - round(job['duration'] * 1000)) > 1):
                    raise ValueError('Cached clip metadata differs from the generated profile')
                first = len(pack.images)
                for frame in frames:
                    append_image(frame)
                directory = BASE_CLIPS + number
                if job['id'] != directory:
                    raise ValueError('Generated animation directory order drift')
                pack.clips += struct.pack('<IHHhhI', first, len(frames), metadata['frame_ms'], *metadata['pivot'], 0)
                animation_meta.append(dict(id=directory, key=job['key'], first_image=first, **metadata))
            for number, job in enumerate(MAP_CLIP_ROWS):
                print(f'Map clip {number + 1}/{len(MAP_CLIP_ROWS)} {job["key"]}', flush=True)
                cached = cache.get('clip', job, scope=map_scope)
                if cached is None:
                    images, metadata = sample_map_clip(pck, job, sampler)
                    cached = cache.put('clip', job, metadata,
                        encoded_frames([encode_image(image, animation=True) for image in images]), scope=map_scope)
                metadata, payload = cached
                frames = decoded_frames(payload, job['frames'])
                if any(metadata[key] != job[key] for key in ('frames', 'duration_ms', 'frame_ms', 'size', 'pivot', 'shader')):
                    raise ValueError('Cached map clip differs from its original profile')
                directory, first = BASE_CLIPS + len(ENEMY_CLIP_ROWS) + number, len(pack.images)
                if job['id'] != directory:
                    raise ValueError('Generated map animation directory order drift')
                for frame in frames:
                    append_image(frame)
                pack.clips += struct.pack('<IHHhhI', first, len(frames), metadata['frame_ms'], *metadata['pivot'], 0)
                animation_meta.append(dict(id=directory, key=job['key'], first_image=first, **metadata))
            for number, job in enumerate(ROOM_CLIP_ROWS):
                print(f'Room clip {number + 1}/{len(ROOM_CLIP_ROWS)} {job["key"]}', flush=True)
                cached = cache.get('clip', job, scope=room_clip_scope)
                if cached is None:
                    images, metadata = sample_treasure(pck, job, sampler)
                    cached = cache.put('clip', job, metadata,
                        encoded_frames([encode_image(image, animation=True) for image in images]), scope=room_clip_scope)
                metadata, payload = cached
                frames = decoded_frames(payload, job['frames'])
                if any(metadata[key] != job[key] for key in ('frames', 'duration_ms', 'frame_ms', 'size', 'pivot')):
                    raise ValueError('Cached room clip differs from its original profile')
                directory = BASE_CLIPS + len(ENEMY_CLIP_ROWS) + len(MAP_CLIP_ROWS) + number
                if job['id'] != directory or len(pack.clips) // 16 != directory:
                    raise ValueError('Generated room animation directory order drift')
                first = len(pack.images)
                for frame in frames:
                    append_image(frame)
                pack.clips += struct.pack('<IHHhhI', first, len(frames), metadata['frame_ms'], *metadata['pivot'], 0)
                animation_meta.append(dict(id=directory, key=job['key'], first_image=first, **metadata))
            for number, job in enumerate(REST_ACTOR_CLIP_ROWS):
                print(f'Rest actor clip {number + 1}/{len(REST_ACTOR_CLIP_ROWS)} {job["key"]}', flush=True)
                cached = cached_rest_actor_clip(cache, job, rest_actor_scope,
                    reference_scope=rest_actor_reference)
                if cached is None:
                    images, metadata = sample_rest_actor_batched(pck, job, sampler)
                    payload = encoded_frames([encode_image(image, animation=True) for image in images])
                    validate_rest_actor_record(job, (metadata, payload))
                    other = cached_rest_actor_clip(cache, job, rest_actor_scope,
                        reference_scope=rest_actor_reference)
                    if other is not None:
                        if canonical(other[0]) != canonical(metadata) or other[1] != payload:
                            raise ValueError('Concurrent rest-frame producers disagree: ' + job['key'])
                    else:
                        cache.put('clip', job, metadata, payload, scope=rest_actor_scope)
                    cached = cached_rest_actor_clip(cache, job, rest_actor_scope)
                    if cached is None or canonical(cached[0]) != canonical(metadata) or cached[1] != payload:
                        raise ValueError('Rest-frame cache changed during publication: ' + job['key'])
                metadata, payload = cached
                frames = decoded_frames(payload, job['frames'])
                validate_rest_actor_record(job, cached)
                directory = BASE_CLIPS + len(ENEMY_CLIP_ROWS) + len(MAP_CLIP_ROWS) + len(ROOM_CLIP_ROWS) + number
                if job['id'] != directory or len(pack.clips) // 16 != directory:
                    raise ValueError('Generated rest actor animation directory order drift')
                first = len(pack.images)
                for frame in frames:
                    append_image(frame)
                pack.clips += struct.pack('<IHHhhI', first, len(frames), metadata['frame_ms'], *metadata['pivot'], 0)
                animation_meta.append(dict(id=directory, key=job['key'], first_image=first, **metadata))
    finally:
        pck.close()

    print('Appending complete five-size glyph coverage and validating retained payloads', flush=True)
    added_glyphs = glyphs(pack, font_path)
    if (len(pack.cards), len(pack.strings), len(pack.aux), len(pack.clips) // 16) != (CARDS, STRINGS, AUXILIARY, CLIPS):
        raise ValueError('World conversion did not fill the complete declared profile')
    if len(pack.images) != plan_summary()['expected_images'] or directory_bytes(pack) > DIRECTORY_LIMIT:
        raise ValueError('Output image/directory accounting differs from the complete plan')
    assert_retained(pack, prefix)
    if source_fingerprint() != source_files:
        raise ValueError('Public source changed during conversion; output was not published')
    data = pack.encode(14)
    decoded = PackData(data)
    assert_retained(decoded, prefix)
    if decoded.encode(14) != data:
        raise ValueError('World pack is not a canonical, fully decodable NSP2/14')
    result = dict(format='NSP2/14', reference='v0.111.0 / 41cef1ea',
        sha256=hashlib.sha256(data).hexdigest(), bytes=len(data),
        directory_bytes=directory_bytes(pack), file_limit=FILE_LIMIT, directory_limit=DIRECTORY_LIMIT,
        cards=len(pack.cards), strings=len(pack.strings), images=len(pack.images),
        auxiliary_images=len(pack.aux), clips=len(pack.clips) // 16, glyphs=len(pack.glyphs),
        new_glyphs=added_glyphs, retained_image_payloads=len(prefix[0]),
        retained_card_records=len(prefix[1]), retained_strings=len(prefix[2]),
        retained_auxiliary_records=len(prefix[3]), retained_animation_clips=BASE_CLIPS,
        retained_glyph_payloads=len(prefix[5]), dependencies=dependencies,
        cache_identity=cache.identity, cache_hits=cache.hits, cache_misses=cache.misses,
        cache_migrated_jobs=cache.legacy_hits,
        source_paths=image_meta, animations=animation_meta, layouts=layouts,
        animation_sampling='Full original selected periods; enemy idle 8 fps, actions 12 fps, map bosses and treasure 20 fps, rest actors up to 20 fps with a 256-frame period cap; physics 60 Hz; old clips unchanged',
        normalized_target_layout=CANONICAL_PLACEMENT,
        limitations=[
            'Offline Spine geometry/raster and scene diagnostics do not establish 95% visual similarity.',
            'Target controllers use the explicit single-player, single-enemy reference placement; calculator layout adapts other encounters.',
            'Original scene particles, unsupported shaders and dynamic nodes are listed in per-scene diagnostics.',
            'Finite conditional multi-track clips preserve original states; Godot phase-continuous rendering requires visual comparison.'],
        excluded=['audio', 'multiplayer', 'timeline'])
    atomic_bytes(output, data)
    atomic_bytes(output.with_name('content-manifest.json'),
                 json.dumps(result, ensure_ascii=False, indent=2, allow_nan=False).encode() + b'\n')
    print(json.dumps({key: result[key] for key in ('format', 'sha256', 'bytes', 'directory_bytes',
        'cards', 'strings', 'images', 'auxiliary_images', 'clips', 'glyphs', 'cache_hits')}, ensure_ascii=False), flush=True)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path, nargs='?')
    parser.add_argument('source', type=Path, nargs='?')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--font')
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--dotnet')
    parser.add_argument('--fastnoise-header', type=Path)
    parser.add_argument('--cache', type=Path)
    parser.add_argument('--plan', action='store_true', help='Print public directory counts without reading private inputs')
    args = parser.parse_args()
    if args.plan:
        print(json.dumps(plan_summary(), indent=2))
    else:
        if args.game is None or args.source is None or args.output is None:
            parser.error('game, source and --output are required unless --plan is selected')
        build(args.game, args.source, args.output, choose_font(args.font), runtime=args.runtime,
              dotnet=args.dotnet, fastnoise_header=args.fastnoise_header, cache_dir=args.cache)
