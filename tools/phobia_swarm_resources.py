#!/usr/bin/env python3
"""Append owner-provided swarm and sleeping particles to UI40 startup art.

The source scene supplies the emitter and three lifetime curves. Its numeric
curve samples are public metadata; texture pixels stay in private resources.
Only one shared-art file and the existing sixteen compressed NSL2 directories
are written, with no duplicate logical packs.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from PIL import Image

import content_tiles as tiles
import split_content as split
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
from pck import Pack
from world_scene_assets import SceneLoader, _property, _vector, load_scene_texture

ROOT = Path(__file__).resolve().parents[1]
SCENE = 'scenes/creature_visuals/entomancer.tscn'
TEXTURE = 'images/vfx/monsters/entomancer/entomancer_bug_particle.png'
BASE_AUX, AUXILIARY, CLIPS = 2198, 2201, 1783
SAMPLES = 128


def curve_points(loader, material, name):
    texture = loader.resource(_property(material, name))[1]
    curve = loader.resource(_property(texture, 'curve_x' if name == 'directional_velocity_curve' else 'curve'))[1]
    data = _property(curve, '_data')
    tiles.require(len(data) % 5 == 0, 'Malformed original particle curve')
    return [(float(_vector(data[i])[0]), float(_vector(data[i])[1]), float(data[i+1]), float(data[i+2]))
            for i in range(0, len(data), 5)]


def curve_value(points, at):
    if at <= points[0][0]:return points[0][1]
    if at >= points[-1][0]:return points[-1][1]
    for a, b in zip(points, points[1:]):
        if a[0] <= at <= b[0]:
            span = b[0]-a[0];t = (at-a[0])/span
            return ((2*t**3-3*t**2+1)*a[1]+(t**3-2*t**2+t)*span*a[3]+
                    (-2*t**3+3*t**2)*b[1]+(t**3-t**2)*span*b[2])
    raise ValueError('Unordered original particle curve')


def source_metadata(pack):
    loader = SceneLoader(pack);root = loader.instantiate(SCENE)
    body = next(node for node in root.children if node.name == 'PhobiaModeVisuals')
    emitter = next(node for node in body.children if node.name == 'SwarmParticles')
    expected = dict(amount=100, lifetime=2.0, preprocess=2.0, local_coords=True)
    for key, value in expected.items():tiles.require(_property(emitter.props, key) == value, 'Original swarm '+key+' changed')
    tiles.require(loader.path(_property(emitter.props, 'texture')) == TEXTURE, 'Original swarm texture changed')
    material = loader.resource(_property(emitter.props, 'process_material'))[1]
    for key, value in dict(emission_shape=6, emission_ring_radius=400.0,
                          emission_ring_inner_radius=50.0, spread=180.0,
                          initial_velocity_min=100.0, initial_velocity_max=200.0,
                          orbit_velocity_min=-1.0, orbit_velocity_max=1.0,
                          scale_max=1.5).items():
        tiles.require(_property(material, key) == value, 'Original swarm material '+key+' changed')
    points = {name:curve_points(loader, material, name) for name in
              ('scale_curve', 'orbit_velocity_curve', 'directional_velocity_curve')}
    scale = [round(curve_value(points['scale_curve'], i/SAMPLES)*1024) for i in range(SAMPLES+1)]
    orbit = [0];total = 0.0
    # Integrate the authored angular speed at 1024 intervals. The runtime
    # samples its integral, so it need not march an emitter from room entry.
    for i in range(SAMPLES):
        for sub in range(8):total += curve_value(points['orbit_velocity_curve'], (i+(sub+.5)/8)/SAMPLES)*2/(SAMPLES*8)
        orbit.append(round(total*4096))
    directional = [round(curve_value(points['directional_velocity_curve'], i/SAMPLES)*1024) for i in range(SAMPLES+1)]
    translation = _vector(_property(body.props, 'position')) + _vector(_property(body.props, 'scale'))*_vector(_property(emitter.props, 'position'))
    tiles.require(abs(translation[0]) < .001 and abs(translation[1]+175) < .001,
                  'Original emitter registration changed')
    return dict(scene=SCENE, texture=TEXTURE, emitter='PhobiaModeVisuals/SwarmParticles',
                **expected, ring=[50, 400], speed=[100, 200], scale=[1, 1.5],
                parent_scale=list(map(float, _vector(_property(body.props, 'scale')))),
                source_origin=list(map(float, translation)), curve_points=points,
                tables=dict(Scale=scale, Orbit=orbit, Directional=directional),
                runtime_particle_buffers=0, runtime_count=100)


def curve_header(metadata):
    lines = ['// Numeric lifetime metadata from Entomancer PhobiaModeVisuals; no artwork.',
             '// Scale/Directional use Q10. Orbit integrates two seconds in 1/4096 turns.']
    for name, values in metadata['tables'].items():
        lines.append('static constexpr int16_t '+name+'[129]={')
        lines.extend(' '+','.join(map(str, values[i:i+16]))+',' for i in range(0, len(values), 16))
        lines.append('};')
    return '\n'.join(lines)+'\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--input-dir', type=Path)
    parser.add_argument('--output-dir', type=Path)
    parser.add_argument('--cache-dir', type=Path)
    parser.add_argument('--write-curves', action='store_true')
    args = parser.parse_args()
    pack = Pack(tiles.private_path(args.pck, 'Owner PCK'))
    try:
        metadata = source_metadata(pack)
        header = ROOT/'include/phobia_swarm_curves.inc'
        if args.write_curves:header.write_text(curve_header(metadata))
        tiles.require(header.read_text() == curve_header(metadata), 'Checked-in swarm curves differ from original scene')
        if not args.input_dir:
            print(json.dumps(metadata, ensure_ascii=False));return
        source_dir = tiles.private_path(args.input_dir, 'Private startup input')
        output = tiles.private_path(args.output_dir, 'Private final resources')
        cache = tiles.private_path(args.cache_dir, 'Private particle sample')
        tiles.require(source_dir != output and output != cache, 'Input/output/cache must differ')
        cache.mkdir(parents=True, exist_ok=True)
        image = load_scene_texture(pack, TEXTURE).convert('RGBA')
        tiles.require(image.size == (15, 15), 'Original swarm particle dimensions changed')
        row = write_image(cache, 'entomancer-swarm', image)
        row.update(auxiliary=BASE_AUX, source=TEXTURE)
        rows=[row];sleeping=[]
        for index, language in enumerate(('eng', 'jpn')):
            path='images/vfx/sleeping/sleeping_z_'+language+'.png'
            original=load_scene_texture(pack, path).convert('RGBA')
            tiles.require(original.size == (128, 128), 'Original sleeping texture registration changed')
            image=original.resize((32,32),Image.Resampling.LANCZOS)
            item=write_image(cache,'sleeping-z-'+language,image)
            item.update(auxiliary=BASE_AUX+index+1,source=path)
            rows.append(item);sleeping.append(dict(source=path,source_size=[128,128],size=[32,32],
                auxiliary=BASE_AUX+index+1,pivot_preserved=True))
    finally:pack.close()
    common = (source_dir/'nSpire2.common.tns').read_bytes();new_common = None
    manifest = dict(images=rows, clips=[], localized_text={language:{} for language in split.LANGUAGES})
    report = dict(reference='v0.111.0 / 41cef1ea', source=metadata, sleeping=sleeping, locales={},
                  old_auxiliary=BASE_AUX, auxiliary=AUXILIARY, logical_files_written=0,
                  input_common_sha256=tiles.sha256(common))
    for language in split.LANGUAGES:
        data = split.reconstruct(common, (source_dir/('nSpire2.'+language+'.tns')).read_bytes(), language)
        result, receipt = patch_pack(data, manifest, cache, base_aux=BASE_AUX,
            auxiliary=AUXILIARY, input_version=40, version=40,
            texts=(), base_clips=CLIPS, base_strings=8704)
        bounds = split.layout(result, language);art = result[bounds['prefix']:bounds['prefix']+bounds['artwork']]
        if new_common is None:
            new_common = split.common_file(art);tiles.atomic_bytes(output/'nSpire2.common.tns', new_common)
        else:tiles.require(art == new_common[64:], 'Swarm artwork differs across locales')
        directory = split.compress_language_directory(split.language_file(result, bounds, language, new_common[32:64]))
        tiles.require(split.reconstruct(new_common, directory, language) == result, 'Swarm split roundtrip differs')
        tiles.atomic_bytes(output/('nSpire2.'+language+'.tns'), directory)
        report['locales'][language] = dict(patch=receipt, logical_sha256=tiles.sha256(result),
            nsl2_sha256=tiles.sha256(directory), localized_bytes=len(directory))
        print(json.dumps(dict(language=language, common_bytes=len(new_common),
            added_common_bytes=len(new_common)-len(common), existing_art_unchanged=True)), flush=True)
    report.update(common_bytes=len(new_common), common_sha256=tiles.sha256(new_common),
                  added_common_bytes=len(new_common)-len(common))
    tiles.atomic_bytes(output/'phobia-swarm-receipt.json', json.dumps(report, ensure_ascii=False, indent=2).encode()+b'\n')


if __name__ == '__main__':main()
