#!/usr/bin/env python3
"""Sample the two authored Ancient skeleton loops without adding a framebuffer.

The complete original scene camera, draw order and bone followers are rendered
at 320x240. Exact opaque RGB565 key/copy patches share the existing scene raster.
Source particle systems remain explicitly listed as omitted; no substitute
particles are invented. The six painted Ancient scenes have no Spine loop.
"""
from __future__ import annotations

import argparse
from functools import lru_cache
import hashlib
import json
import math
from pathlib import Path
import struct
import time

import numpy as np
from PIL import Image

import content_tiles as t

ROOT = Path(__file__).resolve().parents[1]
BASE_AUX, AUXILIARY = 7212, 7214
MAGIC, STEP_MS, KEY_INTERVAL = 0x414e, 33, 8
WIDTH, HEIGHT = 320, 240
SCENES = (
    dict(key='NEOW', path='scenes/events/background_scenes/neow.tscn',
         resource='animations/backgrounds/neow_room/neow.tres', clip='idle_loop', duration_ms=18000),
    dict(key='TEZCATARA', path='scenes/events/background_scenes/tezcatara.tscn',
         resource='animations/backgrounds/tezcatara/tezcatara_skel_data_resource.tres',
         clip='animation', duration_ms=16667),
)


def emit(stage, **values):
    print(json.dumps(dict(stage=stage, **values)), flush=True)


def parameters(raw, image_count):
    t.require(len(raw) == 16, 'Ancient scene parameters must contain eight words')
    magic, version, first, count, step, low, high, reserved = struct.unpack('<8H', raw)
    duration = low | high << 16
    t.require((magic, version, step, reserved) == (MAGIC, 1, STEP_MS, 0) and
              0 < duration <= 120000 and count == (duration + step - 1) // step and
              count and first + count <= image_count, 'Invalid Ancient scene timeline')
    return dict(first=first, frames=count, frame_ms=step, duration_ms=duration)


def rgb565(image):
    # The ordinary room is drawn over black. Flatten with the same integer
    # alpha convention before computing patches, never by dropping alpha.
    rgba = np.asarray(image.convert('RGBA'), dtype=np.uint32)
    colors = ((rgba[:, :, 0] >> 3) << 11) | ((rgba[:, :, 1] >> 2) << 5) | (rgba[:, :, 2] >> 3)
    alpha = rgba[:, :, 3]
    if np.any(alpha != 255):
        channels = []
        for shift, mask in ((11, 31), (5, 63), (0, 31)):
            value = ((colors >> shift) & mask) * alpha
            value += 128
            channels.append(((value + (value >> 8)) >> 8) << shift)
        colors = channels[0] | channels[1] | channels[2]
    return colors.astype('<u2')


def encode_patch(current, reference=None):
    """Opaque exact copy spans, merging only byte-cheaper tiny gaps."""
    mask = np.ones((HEIGHT, WIDTH), dtype=bool) if reference is None else current != reference
    out = bytearray()
    for y in range(HEIGHT):
        edges = np.diff(np.pad(mask[y].astype('i1'), (1, 1)))
        starts, ends = np.flatnonzero(edges == 1), np.flatnonzero(edges == -1)
        if len(starts) > 1:
            separate = starts[1:] - ends[:-1] > 2
            starts, ends = starts[np.r_[True, separate]], ends[np.r_[separate, True]]
        out.extend(struct.pack('<H', len(starts)))
        for left, right in zip(starts, ends):
            out.extend(struct.pack('<HHB', int(left), int(right-left), 0))
            out.extend(current[y, left:right].astype('<u2').tobytes())
    return bytes(out)


def signature(pack, runtime, scene):
    h = hashlib.sha256(json.dumps(scene, sort_keys=True).encode())
    h.update(Path(runtime).read_bytes())
    for name in ('ancient_scene_ui23.py', 'world_scene_assets.py', 'world_enemy_assets.py',
                 'world_raster.py', 'animation.py', 'SpineSampleAdapter.cs'):
        h.update((ROOT/'tools'/name).read_bytes())
    for name, entry in sorted(pack.entries.items()):
        h.update(name.encode()); h.update(entry.md5)
    return h.hexdigest()


class SampledLoop:
    def __init__(self, sampler, scene, times):
        from character_death_assets import sample_bounded
        self.sampler, self.scene = sampler, scene
        self.frames = sample_bounded(sampler,scene['resource'],scene['clip'],times,loop=True)

    def metadata(self, resource):
        return self.sampler.metadata(resource)

    def sample(self, resource, clip, times, **kwargs):
        t.require(resource == self.scene['resource'] and clip == self.scene['clip'],
                  'Unexpected Ancient scene animation')
        return [self.frames[round(value*1000)//STEP_MS] for value in times]

    def draw(self, resource, frame):
        return self.sampler.draw(resource, frame)


def sample_scene(pack, sampler, runtime, scene, work):
    from build_content import texture
    import animation
    from world_raster import raster
    from world_scene_assets import render_scene
    key = signature(pack, runtime, scene)
    directory = Path(work)/(scene['key'].lower()+'-'+key[:16]); directory.mkdir(parents=True, exist_ok=True)
    count = math.ceil(scene['duration_ms']/STEP_MS)
    metadata = sampler.metadata(scene['resource'])
    t.require([(clip['name'], round(clip['duration']*1000)) for clip in metadata['clips']] ==
              [(scene['clip'], scene['duration_ms'])], 'Authored Ancient loop changed')
    paths = [directory/f'{frame:04d}.rgb565' for frame in range(count)]
    receipt = directory/'frames.json'
    if receipt.exists() and all(path.is_file() and path.stat().st_size == WIDTH*HEIGHT*2 for path in paths):
        return directory, json.loads(receipt.read_text())
    times = [frame*STEP_MS/1000 for frame in range(count)]
    loop = SampledLoop(sampler, scene, times)
    load_texture = lru_cache(maxsize=32)(texture)
    hashes, diagnostics = [], None
    previous_raster = animation.raster
    try:
        animation.raster = raster
        for frame, (seconds, path) in enumerate(zip(times, paths)):
            if path.is_file() and path.stat().st_size == WIDTH*HEIGHT*2:
                raw = path.read_bytes()
            else:
                picture, details = render_scene(pack, scene['path'], WIDTH, HEIGHT,
                    viewport=(1680,1260), mode='scene', sampler=loop, sample_time=seconds,
                    texture_loader=load_texture)
                raw = rgb565(picture).tobytes(); t.atomic_bytes(path, raw)
                if diagnostics is None: diagnostics = details
                if frame in (0, count//2, count-1): picture.save(directory/f'{frame:04d}.png')
            hashes.append(t.sha256(raw))
            if frame % 20 == 0: emit('ancient_sample', key=scene['key'], frame=frame, frames=count)
    finally:
        animation.raster = previous_raster
        load_texture.cache_clear()
    report = dict(scene, signature=key, frames=count, frame_ms=STEP_MS, size=[WIDTH,HEIGHT],
        viewport=[1680,1260], source_rgb565_sha256=hashes, diagnostics=diagnostics,
        exact_source_cadence_at_calculator_tick=True, particle_systems_implemented=False)
    t.atomic_bytes(receipt, (json.dumps(report,indent=2)+'\n').encode())
    return directory, report


def append(data, sampled, work, workers):
    from coalesce_tile_resources import view
    from patch_frontend_assets import patch_pack
    from split_content import LANGUAGES
    from lz4_block import encode
    import retile_character_ui12 as dice
    import retile_appended_resources as retile
    source = view(data)
    t.require(source.version in (45,46) and len(source.aux) == BASE_AUX and len(source.clips) == 1783,
              'Ancient extension requires complete post-retirement UI22 artwork')
    work = Path(work); cache = work/'patches'; cache.mkdir(parents=True,exist_ok=True)
    images, tables, selected, reports = [], [], set(), []
    for scene_index, (directory, record) in enumerate(sampled):
        first = len(source.images)+len(images)
        t.require(first+record['frames'] < 65536, 'Ancient frame image ids exceed compact words')
        base = previous = None
        for frame in range(record['frames']):
            colors = np.frombuffer((directory/f'{frame:04d}.rgb565').read_bytes(),dtype='<u2').reshape(HEIGHT,WIDTH)
            raw = encode_patch(colors, None if frame == 0 else base if frame % KEY_INTERVAL == 0 else previous)
            packed = struct.pack('<BI',4,len(raw))+encode(raw)
            payload, fmt = (packed,5) if len(packed) < len(raw) else (raw,4)
            filename = f'{scene_index}-{frame:04d}.bin'; t.atomic_bytes(cache/filename,payload)
            selected.add(first+frame)
            images.append(dict(width=WIDTH,height=HEIGHT,format=fmt,file=filename,bytes=len(payload),
                               sha256=t.sha256(payload),raw_sha256=t.sha256(raw)))
            if frame == 0: base = colors
            previous = colors
        raw = struct.pack('<8H',MAGIC,1,first,record['frames'],STEP_MS,
                          record['duration_ms']&65535,record['duration_ms']>>16,0)
        filename = f'parameters-{scene_index}.bin'; t.atomic_bytes(cache/filename,raw)
        tables.append(len(source.images)+len(images))
        images.append(dict(width=8,height=1,format=0,file=filename,bytes=len(raw),
                           sha256=t.sha256(raw),raw_sha256=t.sha256(raw)))
        reports.append(dict(key=record['key'],first=first,frames=record['frames'],
            duration_ms=record['duration_ms'],frame_ms=STEP_MS,source=record['path'],cache=str(directory)))
    manifest = dict(images=images,auxiliary_image_ids=tables,localized_text={code:{} for code in LANGUAGES})
    added, details = patch_pack(data,manifest,cache,base_aux=BASE_AUX,auxiliary=AUXILIARY,
        input_version=source.version,version=source.version,texts=(),base_clips=len(source.clips),
        base_strings=len(source.strings))
    spool = work/'retile'; spool.mkdir(parents=True,exist_ok=True)
    current = view(added); plan = dice.make_plan(current,spool,selected,precision=8)
    plan['compact_replaced_directory'] = True
    encoded = retile.compress_plan(plan,spool,spool/'encoded',workers)
    output, compression = retile.rewrite(current,plan,encoded)
    proof = dice.verify(current,output,plan,spool)
    target = view(output)
    t.require(target.clips == source.clips and target.aux[:BASE_AUX] == source.aux,
              'Ancient extension moved existing clip or auxiliary identities')
    from animation_color_candidates_ui23 import protected_images
    t.require(selected <= protected_images(target), 'Ancient source colors must remain exact')
    return output, dict(complete=True,scenes=reports,appended_images=len(images),
        auxiliary_count=AUXILIARY,input_bytes=len(data),output_bytes=len(output),
        added_bytes=len(output)-len(data),exact_rgb565=True,framebuffer_growth=0,
        append=details,compression=compression,verification=proof)


def main():
    from pck import Pack
    from world_enemy_assets import SpineSampler
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--pck',required=True,type=Path)
    parser.add_argument('--runtime',required=True,type=Path)
    parser.add_argument('--dotnet',default='dotnet')
    parser.add_argument('--work',required=True,type=Path)
    parser.add_argument('--workers',type=int,default=2)
    parser.add_argument('--sample-only',action='store_true')
    args = parser.parse_args()
    t.require(args.sample_only or args.input and args.output,'Supply input/output or --sample-only')
    for path in (args.input,args.output,args.pck,args.runtime,args.work):
        if path is not None:t.private_path(path,'Private Ancient conversion input/output')
    args.work.mkdir(parents=True,exist_ok=True)
    pack = Pack(args.pck); started=time.monotonic()
    try:
        with SpineSampler(pack,runtime=args.runtime,dotnet=args.dotnet,work_dir=args.work/'adapter') as sampler:
            sampled=[sample_scene(pack,sampler,args.runtime,scene,args.work/'sampled') for scene in SCENES]
        if not args.sample_only:
            output,report=append(args.input.read_bytes(),sampled,args.work,args.workers)
            report.update(output_sha256=t.sha256(output),seconds=time.monotonic()-started)
            t.atomic_bytes(args.output,output)
            t.atomic_bytes(args.output.with_suffix('.json'),(json.dumps(report,indent=2)+'\n').encode())
            emit('ancient_complete',added_bytes=report['added_bytes'],output_bytes=len(output))
    finally: pack.close()


if __name__ == '__main__':
    main()
