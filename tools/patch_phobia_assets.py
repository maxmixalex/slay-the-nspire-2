#!/usr/bin/env python3
"""Append original phobia bodies, skins and background branches to private UI7.

All actor registrations are measured from the existing normal model pipeline
and checked against the supplied pack before alternate pixels are sampled.
The new images use the existing bounded retained-group codec; neither old art
nor gameplay state is changed. Original inputs and generated output are private.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import json
import mmap
from pathlib import Path

import numpy as np

import content_tiles as tiles
from build_world import choose_layout, sample_enemy, sample_enemy_batched
from group_ui7_hands import RowImages
from patch_feedback_assets import SOURCE_PCK_SHA256, write_image
from patch_frontend_assets import patch_pack as append_pack
from pck import Pack
from repack_retained import DESCRIPTOR, DEFAULT_GROUP_BYTES, plan_images
from world_enemy_assets import SpineSampler, manifest as enemy_manifest
from world_raster import raster
from world_scene_assets import (SceneLoader, _node_matrix, _property, _vector,
                                load_scene_texture, render_scene)
from world_ui_profile import ENEMY_CLIP_ROWS

ROOT = Path(__file__).resolve().parents[1]
VERSION, BASE_VERSION = 25, 24
BASE_AUX, AUXILIARY, BASE_CLIPS, CLIPS, STRINGS = 1877, 1881, 1710, 1753, 8704
PROFILE = json.loads((ROOT/'config/phobia-ui7-0.17.json').read_text())


def original_jobs():
    by_id = {row['id']: row for row in ENEMY_CLIP_ROWS}
    rows = []
    for spec in PROFILE['clips']:
        original = by_id[spec['source_directory']]
        tiles.require(original['key'] == spec['source_key'] and original['frames'] == spec['frames'],
                      'Original phobia animator directory changed')
        rows.append(dict(original, key='phobia.'+original['key'], skins=['phobia'],
                         id=BASE_CLIPS+len(rows)))
    tiles.require(len(rows) == 32, 'Original phobia skin catalog changed')
    return rows


def identity(runtime):
    names = ('patch_phobia_assets.py', 'build_world.py', 'world_enemy_assets.py',
        'world_scene_assets.py', 'world_raster.py', 'animation.py', 'SpineSampleAdapter.cs',
        'patch_frontend_assets.py', 'group_ui7_hands.py', 'repack_retained.py')
    return tiles.sha256(b''.join((ROOT/'tools'/name).read_bytes() for name in names)+
        (ROOT/'config/phobia-ui7-0.17.json').read_bytes()+Path(runtime).read_bytes())


def normal_geometry(source, jobs):
    result = []
    for job in jobs:
        first, count, ms, px, py, reserved = source.clips[job['id']]
        shapes = [(source.images[i][1], source.images[i][2]) for i in range(first, first+count)]
        tiles.require(count == job['frames'] and not reserved, 'Normal pack animator profile changed')
        result.append((job['id'], count, ms, px, py, shapes))
    return result


def validate_normal_layout(source, jobs, layout):
    """Do not fit a replacement independently of the normal actor's scale."""
    expected_size = tuple(layout['size'])
    expected_pivot = tuple(round(value) for value in layout['origin'])
    for _id, _count, _ms, x, y, shapes in normal_geometry(source, jobs):
        tiles.require((x, y) == expected_pivot and all(shape == expected_size for shape in shapes),
                      'Re-measured normal layout does not match the existing pack: '+jobs[0]['model'])


def anchored_layout(bounds, scale):
    bounds = np.asarray(bounds, dtype=float)
    low, high = bounds[:, 0].min(axis=0), bounds[:, 1].max(axis=0)
    tiles.require(np.isfinite(bounds).all() and scale > 0, 'Invalid alternate model bounds')
    width, height = np.ceil(np.maximum(high-low, 1)*scale).astype(int)+4
    origin = [2-low[0]*scale, 2+high[1]*scale]
    tiles.require(0 < width <= 320 and 0 < height <= 240 and
        -640 <= round(origin[0]) <= 640 and -480 <= round(origin[1]) <= 480,
        'Phobia body exceeds the renderer at its verified normal scale')
    return dict(scale=scale, origin=list(map(float, origin)), size=[int(width), int(height)],
                bounds=[low.tolist(), high.tolist()], crop=False)


def static_geometry(pack, spec, *, dead=False, texture_loader=None):
    """Original centered Sprite2D transform in the normal creature's space."""
    loader = SceneLoader(pack)
    root = loader.instantiate(spec['scene_path'])
    matches = []
    def walk(node, parent):
        matrix = parent @ _node_matrix(node.props)
        if node.name == 'PhobiaModeVisuals': matches.append((node, matrix))
        for child in node.children: walk(child, matrix)
    # NCreatureVisuals is itself positioned by the room; its child coordinates
    # are the registration space shared by the normal Spine actor and this body.
    for child in root.children: walk(child, np.eye(3))
    tiles.require(len(matches) == 1 and matches[0][0].kind == 'Sprite2D',
                  'Expected one original phobia Sprite2D body')
    node, matrix = matches[0]
    tiles.require(np.allclose(_vector(_property(node.props, 'position')), spec['position']) and
                  np.allclose(_vector(_property(node.props, 'scale')), spec['scale']),
                  'Original phobia registration changed')
    reference = loader.path(_property(node.props, 'texture'))
    tiles.require(reference.removeprefix('res://') == spec['source'], 'Original phobia body texture changed')
    path = spec['dead_source'] if dead else spec['source']
    image = load_scene_texture(pack, path, texture_loader=texture_loader).convert('RGBA')
    w, h = image.size
    center = np.array([w/2, h/2]) if _property(node.props, 'centered', True) else np.zeros(2)
    offset = _vector(_property(node.props, 'offset'))-center
    points = np.asarray([[0, 0], [w, 0], [w, h], [0, h]], dtype=float)+offset
    vertices = np.column_stack((points, np.ones(4))) @ matrix.T
    vertices = vertices[:, :2]; vertices[:, 1] *= -1  # Godot down -> raster up.
    uv = np.asarray([[0, 0], [1, 0], [1, 1], [0, 1]], dtype=float)
    if _property(node.props, 'flip_h', False): uv[:, 0] = 1-uv[:, 0]
    if _property(node.props, 'flip_v', False): uv[:, 1] = 1-uv[:, 1]
    color = _vector(_property(node.props, 'modulate', (1, 1, 1, 1)), 4)
    color *= _vector(_property(node.props, 'self_modulate', (1, 1, 1, 1)), 4)
    draw = [(vertices, uv, np.asarray([[0, 1, 2], [2, 3, 0]]),
             np.asarray(image, dtype=np.float32)/255, color, 0)]
    return draw, [vertices.min(axis=0).tolist(), vertices.max(axis=0).tolist()]


def grouped_art(rows, cache):
    """Alias exact frame payloads and compress neighboring sparse images once."""
    payloads = []
    for row in rows:
        raw = (cache/row['file']).read_bytes()
        tiles.require(len(raw) == row['bytes'] and tiles.sha256(raw) == row['sha256'], 'Phobia frame changed')
        payloads.append(raw)
    source = RowImages(rows, payloads)
    decoded_sizes = [len(source.image_raw(i)[3]) for i in range(len(rows))]
    batches = plan_images(source, group_bytes=DEFAULT_GROUP_BYTES, fast_images='none')
    payload, entries, groups, maximum = bytearray(), [None]*len(rows), 0, 0
    def append(raw):
        payload.extend(bytes(len(payload) & 1)); off = len(payload); payload.extend(raw); return off
    for batch in batches:
        if batch.encoded is None:
            for record in batch.records:
                at = append(record.payload)
                for image in record.ids: entries[image] = (at, record.stored_format, len(record.payload))
            continue
        group = tiles.GROUP.pack(b'RGR1', batch.raw_size, len(batch.encoded), 14,
                                 len(batch.records), 0, 0, 0)+batch.encoded
        group_at, raw_at = append(group), 0
        for record in batch.records:
            payload.extend(bytes(len(payload) & 1)); at = len(payload)
            payload.extend(DESCRIPTOR.pack(group_at-at, len(group), raw_at, (record.fmt << 24)|len(record.raw)))
            for image in record.ids: entries[image] = (at, 7, DESCRIPTOR.size)
            raw_at += len(record.raw)
        groups += 1; maximum = max(maximum, batch.raw_size)
    payload.extend(bytes(len(payload) & 1))
    old_bytes = sum((len(raw)+1)&~1 for raw in payloads)
    tiles.require(all(row is not None for row in entries) and len(payload) <= old_bytes,
                  'Phobia grouping enlarged or dropped image data')
    report = dict(retained_groups=groups, max_group_raw_bytes=maximum,
        target_group_raw_bytes=DEFAULT_GROUP_BYTES, exact_frame_aliases=len(rows)-sum(len(batch.records) for batch in batches),
        original_payload_bytes=old_bytes, stored_payload_bytes=len(payload),
        saved_common_art_bytes=old_bytes-len(payload), unchanged_decoded_pixels=True,
        unreachable_payload_bytes=0, runtime_buffer_growth=0)
    return dict(payload=bytes(payload), entries=entries, decoded_sizes=decoded_sizes, report=report)


def build_art(pck_path, normal, cache, *, runtime, dotnet=None):
    cache.mkdir(parents=True, exist_ok=True)
    selected = {row['model'] for row in PROFILE['skin_models']+PROFILE['static_bodies']}
    normal_jobs = defaultdict(list)
    for job in ENEMY_CLIP_ROWS:
        if job['model'] in selected: normal_jobs[job['model']].append(job)
    geometry = normal_geometry(normal, [job for rows in normal_jobs.values() for job in rows])
    geometry_hash = tiles.sha256(tiles.canonical(geometry))
    fingerprint = identity(runtime)
    path = cache/'phobia-assets.json'
    if path.is_file():
        saved = json.loads(path.read_text())
        if saved.get('converter_sha256') == fingerprint and saved.get('normal_geometry_sha256') == geometry_hash:
            tiles.require(len(saved['clips']) == CLIPS-BASE_CLIPS and len(saved['auxiliary_rows']) == 4,
                          'Changed cached phobia catalog')
            for row in saved['images']:
                raw = (cache/row['file']).read_bytes()
                tiles.require(len(raw) == row['bytes'] and tiles.sha256(raw) == row['sha256'], 'Changed cached phobia image')
            return saved
    models = {row['model']: row for row in enemy_manifest()['models']}
    layouts, images, clips, backgrounds, reports = {}, [], [], [], []
    pack = Pack(pck_path)
    try:
      with SpineSampler(pack, runtime=runtime, dotnet=dotnet, work_dir=cache/'spine-adapter') as sampler:
        for name, jobs in normal_jobs.items():
            stored = cache/('normal-layout-'+name+'.json')
            saved = json.loads(stored.read_text()) if stored.is_file() else {}
            if saved.get('converter_sha256') == fingerprint and saved.get('normal_geometry_sha256') == geometry_hash:
                layout = saved['layout']
            else:
                print('Measure normal registration: '+name, flush=True)
                measurements = [sample_enemy(pack, models[name], job, sampler, measure=True)[1] for job in jobs]
                layout = choose_layout(measurements)
            validate_normal_layout(normal, jobs, layout)
            layouts[name] = layout
            tiles.atomic_bytes(stored, json.dumps(dict(converter_sha256=fingerprint,
                normal_geometry_sha256=geometry_hash, layout=layout), indent=2).encode()+b'\n')
        jobs = original_jobs()
        skin_bounds = defaultdict(list)
        for job in jobs:
            print('Measure alternate: '+job['key'], flush=True)
            skin_bounds[job['model']].append(sample_enemy(pack, models[job['model']], job, sampler, measure=True)[1]['bounds'])
        alternate_layouts = {name: anchored_layout(bounds, layouts[name]['scale']) for name, bounds in skin_bounds.items()}
        for job in jobs:
            print('Sample alternate: '+job['key'], flush=True)
            frames, report = sample_enemy_batched(pack, models[job['model']], job, sampler, layout=alternate_layouts[job['model']])
            clips.append(dict(first=len(images), count=len(frames), frame_ms=report['frame_ms'],
                pivot=report['pivot'], key=job['key']))
            for i, image in enumerate(frames):
                row = write_image(cache, f'phobia-clip-{len(clips)-1:02d}-{i:03d}', image, sparse=True)
                row['key'] = job['key']; images.append(row)
            reports.append(report)
        for spec in PROFILE['static_bodies']:
            variants = [(False, *static_geometry(pack, spec))]
            if 'dead_source' in spec: variants.append((True, *static_geometry(pack, spec, dead=True)))
            layout = anchored_layout([bounds for _dead, _draw, bounds in variants], layouts[spec['model']]['scale'])
            for dead, draw, _bounds in variants:
                image = raster(draw, layout['scale'], layout['origin'], tuple(layout['size']))
                key = spec['dead_key'] if dead else spec['key']
                clips.append(dict(first=len(images), count=1, frame_ms=1000,
                    pivot=[round(value) for value in layout['origin']], key=key))
                row = write_image(cache, f'phobia-body-{len(clips)-1:02d}', image, sparse=True)
                row['key'] = key; images.append(row)
            reports.append(dict(model=spec['model'], normal_scale=layouts[spec['model']]['scale'], layout=layout,
                                source='PhobiaModeVisuals Sprite2D; original registration, no trim-to-fit'))
        for spec in PROFILE['backgrounds']:
            image, report = render_scene(pack, spec['path'], *spec['size'], mode='background', sampler=sampler, phobia_mode=True)
            tiles.require(report.get('phobia_layers', 0) > 0, 'Phobia background did not select an alternate layer')
            backgrounds.append(len(images))
            row = write_image(cache, spec['key'].replace('.', '-'), image)
            row['key'] = spec['key']; images.append(row); reports.append(report)
    finally: pack.close()
    expected = ['phobia.'+row['source_key'] for row in PROFILE['clips']]
    for spec in PROFILE['static_bodies']:
        expected.append(spec['key'])
        if 'dead_key' in spec: expected.append(spec['dead_key'])
    tiles.require([row['key'] for row in clips] == expected and len(clips) == CLIPS-BASE_CLIPS,
                  'Alternate clip directory differs from runtime indices')
    result = dict(schema=1, reference='v0.111.0 / 41cef1ea', source_pck_sha256=SOURCE_PCK_SHA256,
        converter_sha256=fingerprint, normal_geometry_sha256=geometry_hash, normal_registration_verified=True,
        output_version=VERSION, images=images, clips=clips, auxiliary_rows=backgrounds,
        auxiliary_count=AUXILIARY, animation_count=CLIPS, localized_text={'eng': {}, 'zhs': {}},
        layouts=layouts, diagnostics=reports, limitations=PROFILE['limitations'])
    tiles.atomic_bytes(path, json.dumps(result, indent=2).encode()+b'\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--input-eng', type=Path, required=True)
    parser.add_argument('--input-zhs', type=Path, required=True)
    parser.add_argument('--cache-dir', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--dotnet')
    args = parser.parse_args()
    sources = {lang: tiles.private_path(getattr(args, 'input_'+lang), 'Normal pack') for lang in ('eng', 'zhs')}
    output = tiles.private_path(args.output_dir, 'Alternate output directory')
    cache = tiles.private_path(args.cache_dir, 'Private alternate cache')
    pck = tiles.private_path(args.pck, 'Original PCK')
    runtime = tiles.private_path(args.runtime, 'Original sampling runtime')
    with sources['eng'].open('rb') as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
        normal = tiles.PackView(data, BASE_AUX, versions=(BASE_VERSION,), profile=(558, STRINGS, BASE_CLIPS, BASE_AUX))
        art = build_art(pck, normal, cache, runtime=runtime, dotnet=args.dotnet)
    grouped = grouped_art(art['images'], cache)
    receipts = {}
    for language, source in sources.items():
        target = output/('phobia-'+language+'.nsp')
        tiles.require(target.resolve() != source, 'Alternate output must not overwrite normal input')
        data, receipt = append_pack(source.read_bytes(), art, cache, base_aux=BASE_AUX,
            auxiliary=AUXILIARY, input_version=BASE_VERSION, version=VERSION, texts=(),
            base_clips=BASE_CLIPS, base_strings=STRINGS, appended_art=grouped)
        tiles.atomic_bytes(target, data); receipts[language] = receipt
    tiles.atomic_bytes(output/'phobia-conversion.json', json.dumps(receipts, indent=2).encode()+b'\n')
    print(json.dumps(dict(images=len(art['images']), clips=len(art['clips']),
        backgrounds=len(art['auxiliary_rows']), grouping=grouped['report'])))


if __name__ == '__main__': main()
