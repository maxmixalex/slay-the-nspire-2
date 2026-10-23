"""Private Spine 4.2 sampling and original enemy skin/layer composition.

This module contains an independent adapter, not the Spine runtime. The separately
licensed runtime and the player's PCK are read-only local inputs. Geometry stays
in Spine's x-right/y-up coordinates until a caller explicitly supplies a matrix.
"""
from __future__ import annotations
from collections import OrderedDict
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import subprocess
from tempfile import TemporaryDirectory
import numpy as np
from animation import raster

ROOT = Path(__file__).resolve().parents[1]
RUNTIME_SHA256 = '0e05031a0377bea228e6419572f171496cb77d58bfdc9129c4607d54601ba3ec'
FLIP_Y = np.diag([1.0, -1.0, 1.0])


def manifest() -> dict:
    return json.loads((ROOT / 'config/enemy-visuals-0.17.json').read_text())


def _version(value: str) -> tuple:
    return tuple(int(x) for x in re.findall(r'\d+', value))


def _scene_matrix(properties: dict) -> np.ndarray:
    """Godot local transform, including skew; no implicit coordinate flip."""
    x, y = properties.get('position', [0, 0])
    sx, sy = properties.get('scale', [1, 1])
    angle = properties.get('rotation', 0)
    skew = properties.get('skew', 0)
    return np.array([[math.cos(angle)*sx, -math.sin(angle+skew)*sy, x],
                     [math.sin(angle)*sx, math.cos(angle+skew)*sy, y],
                     [0, 0, 1]], dtype=float)


def _bone_matrix(values: list) -> np.ndarray:
    return np.array([values[:3], values[3:], [0, 0, 1]], dtype=float)


class SpineSampler:
    """Keep one reviewed runtime process alive across many original skeletons.

    ``resource`` accepts a .tres path, a .skel path, a manifest row, or a known
    skeleton basename. ``sample`` returns JSON-compatible frames; ``draw`` maps
    them to the six-tuples accepted by animation.raster without flattening blends.
    """
    def __init__(self, pack, *, runtime=None, dotnet=None, work_dir=None, texture=None, fastnoise_header=None):
        self.pack = pack
        self.runtime = Path(runtime or os.environ.get('NSPIRE_SPINE_RUNTIME', ''))
        if not self.runtime.is_file():
            raise ValueError('Supply the reviewed local Spine 4.2 runtime with runtime= or NSPIRE_SPINE_RUNTIME')
        # This identity keys compiled adapters and rendered-image caches; it is
        # not compared against a fixed allow-list or used as an integrity gate.
        self.runtime_identity = hashlib.sha256(self.runtime.read_bytes()).hexdigest()
        self.dotnet = str(dotnet or os.environ.get('NSPIRE_DOTNET', 'dotnet'))
        self._temporary = TemporaryDirectory(prefix='nspire-spine-') if work_dir is None else None
        self.work_dir = Path(self._temporary.name if self._temporary else work_dir)
        self.work_dir.mkdir(parents=True, exist_ok=True)
        self._resolved = {}
        self._metadata = {}
        self._pages = OrderedDict()
        self._texture = texture
        self._noise = None
        self._fastnoise_header = fastnoise_header
        self._process = None
        self._prepare()

    def _prepare(self):
        source = ROOT / 'tools/SpineSampleAdapter.cs'
        identity = hashlib.sha256(source.read_bytes() + self.runtime_identity.encode()).hexdigest()[:20]
        directory = self.work_dir / ('adapter-' + identity)
        assembly = directory / 'SpineSampleAdapter.dll'
        if not assembly.is_file():
            listing = subprocess.check_output([self.dotnet, '--list-sdks'], text=True)
            sdks = re.findall(r'^(\S+) \[([^\]]+)\]$', listing, re.M)
            if not sdks:
                raise ValueError('A local .NET SDK is required to compile the independent Spine adapter')
            sdk_version, sdk_root = max(sdks, key=lambda row: _version(row[0]))
            major = _version(sdk_version)[0]
            if major < 10:
                raise ValueError('The reviewed private runtime requires a .NET 10 or newer SDK')
            sdk_root = Path(sdk_root)
            refs_root = sdk_root.parent / 'packs/Microsoft.NETCore.App.Ref'
            refs_version = max((p for p in refs_root.iterdir() if p.is_dir()), key=lambda p: _version(p.name))
            tfm = f'net{_version(refs_version.name)[0]}.0'
            refs = sorted((refs_version / 'ref' / tfm).glob('*.dll'))
            if not refs:
                raise ValueError('No .NET reference assemblies found')
            directory.mkdir(parents=True, exist_ok=True)
            args = [self.dotnet, str(sdk_root / sdk_version / 'Roslyn/bincore/csc.dll'),
                    '-nologo', '-target:exe', '-optimize+', '-out:' + str(assembly),
                    '-r:' + str(self.runtime)]
            args.extend('-r:' + str(path) for path in refs)
            args.append(str(source))
            subprocess.run(args, check=True)
            (directory / 'SpineSampleAdapter.runtimeconfig.json').write_text(json.dumps({
                'runtimeOptions': {'tfm': tfm, 'framework': {
                    'name': 'Microsoft.NETCore.App', 'version': f'{_version(refs_version.name)[0]}.0.0'}}}))
            shutil.copyfile(self.runtime, directory / 'OurNotes.Spine.Runtime.dll')
        self._process = subprocess.Popen([self.dotnet, str(assembly)], stdin=subprocess.PIPE,
                                         stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                         bufsize=1)

    def close(self):
        if self._process is not None:
            self._process.stdin.close()
            try:
                code = self._process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self._process.kill()
                self._process.wait()
                code = -1
            error = self._process.stderr.read()
            self._process.stdout.close()
            self._process.stderr.close()
            self._process = None
            if code:
                raise RuntimeError('Spine adapter stopped unsuccessfully: ' + error.strip())
        if self._temporary:
            self._temporary.cleanup()
            self._temporary = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def _import(self, resource: str, extension: str) -> str:
        metadata = resource.removeprefix('res://') + '.import'
        if metadata in self.pack.entries:
            paths = re.findall(r'path(?:\.\w+)?="([^\"]+' + re.escape(extension) + ')"',
                               self.pack.read(metadata).decode())
            if len(paths) != 1:
                raise ValueError('Ambiguous imported resource: ' + resource)
            return paths[0]
        return self.pack.imported(Path(resource).name, extension)

    def resolve(self, resource) -> dict:
        if isinstance(resource, dict):
            skel, atlas = resource['skeleton_path'], resource['atlas_path']
        elif str(resource).endswith('.tres'):
            text = self.pack.read(str(resource)).decode()
            skels = re.findall(r'path="([^\"]+\.skel)"', text)
            atlases = re.findall(r'path="([^\"]+\.atlas)"', text)
            if len(skels) != 1 or len(atlases) != 1:
                raise ValueError('Expected one skeleton and one atlas in ' + str(resource))
            skel, atlas = skels[0], atlases[0]
        elif str(resource).endswith('.skel'):
            skel = str(resource)
            atlas = skel[:-5] + '.atlas'
        else:
            name = str(resource)
            candidates = [row for row in manifest()['models'] if row['skeleton'] == name]
            if not candidates:
                for row in manifest()['models']:
                    candidates.extend(layer for layer in row['layers'] if layer.get('skeleton') == name)
            if not candidates:
                originals = [path[:-7] for path in self.pack.entries
                             if path.endswith('/' + name + '.skel.import')]
                if len(originals) != 1:
                    raise ValueError('Use a .tres path for unknown or ambiguous skeleton ' + name)
                skel, atlas = 'res://' + originals[0], 'res://' + originals[0][:-5] + '.atlas'
            else:
                skel, atlas = candidates[0]['skeleton_path'], candidates[0]['atlas_path']
        key = skel + '\n' + atlas
        if key not in self._resolved:
            basename = hashlib.sha256(key.encode()).hexdigest()[:20]
            output = self.work_dir / 'inputs'
            output.mkdir(exist_ok=True)
            skel_file, atlas_file = output / (basename + '.skel'), output / (basename + '.atlas')
            skel_file.write_bytes(self.pack.read(self._import(skel, '.spskel')))
            raw = json.loads(self.pack.read(self._import(atlas, '.spatlas')))['atlas_data']
            atlas_file.write_text(raw)
            self._resolved[key] = dict(skeleton_path=skel, atlas_path=atlas,
                                       local_skeleton=str(skel_file), local_atlas=str(atlas_file))
        return self._resolved[key]

    def _request(self, resource, op: str, **values) -> dict:
        resolved = self.resolve(resource)
        request = dict(schema_version=1, op=op, skeleton_path=resolved['local_skeleton'],
                       atlas_path=resolved['local_atlas'], **values)
        process = self._process
        if process is None:
            raise RuntimeError('SpineSampler is closed')
        process.stdin.write(json.dumps(request, separators=(',', ':')) + '\n')
        process.stdin.flush()
        line = process.stdout.readline()
        if not line:
            raise RuntimeError('Spine adapter exited: ' + process.stderr.read().strip())
        response = json.loads(line)
        if not response.get('ok'):
            raise ValueError(response.get('type', 'Spine') + ': ' + response.get('error', 'unknown failure'))
        return response['result']

    def metadata(self, resource) -> dict:
        key = self.resolve(resource)['local_skeleton']
        if key not in self._metadata:
            self._metadata[key] = self._request(resource, 'metadata')
        return self._metadata[key]

    def decimillipede_noise(self, value):
        if self._noise is None:
            self._noise = _DecimillipedeNoise(self._fastnoise_header, self.work_dir)
        return self._noise(value)

    def sample(self, resource, clip, times, *, skins=None, setup_tracks=(), tracks=(),
               track=0, loop=False, physics_step=1/60, bone_drives=()) -> list:
        values = dict(clip=clip or '', times=list(times), setup_tracks=list(setup_tracks),
                      tracks=list(tracks), track=track, loop=loop, physics_step=physics_step,
                      bone_drives=list(bone_drives))
        if skins is not None:
            values['skins'] = list(skins)
        return self._request(resource, 'samples', **values)['frames']

    def _page(self, resource, page: str, pma: bool) -> np.ndarray:
        atlas = self.resolve(resource)['atlas_path']
        path = atlas.rsplit('/', 1)[0] + '/' + page
        key = (path, pma)
        if key not in self._pages:
            if self._texture is None:
                from build_content import texture
                self._texture = texture
            image = self._texture(self.pack, path)
            array = np.asarray(image.convert('RGBA'), dtype=np.float32) / 255
            if pma:
                array[:, :, :3] = np.divide(array[:, :, :3], array[:, :, 3:4],
                                            out=np.zeros_like(array[:, :, :3]), where=array[:, :, 3:4] > 0)
            self._pages[key] = array
            while len(self._pages) > 8:
                self._pages.popitem(last=False)
        else:
            self._pages.move_to_end(key)
        return self._pages[key]

    def draw(self, resource, frame: dict, transform=None) -> list:
        result = []
        matrix = None if transform is None else np.asarray(transform, dtype=float)
        for layer in frame['draw']:
            vertices = np.asarray(layer['vertices'], dtype=float).reshape(-1, 2)
            if matrix is not None:
                vertices = (matrix @ np.column_stack((vertices, np.ones(len(vertices)))).T).T[:, :2]
            uvs = np.asarray(layer['uvs'], dtype=float).reshape(-1, 2)
            triangles = np.asarray(layer['triangles'], dtype=int).reshape(-1, 3)
            page = self._page(resource, layer['page'], layer['premultiplied_alpha'])
            tint = np.asarray(layer['tint'], dtype=float)
            if layer['dark'] is not None and any(layer['dark']):
                # Spine's two-color shader after converting an atlas page to
                # straight alpha: light*tex + dark*(1-tex), with unchanged alpha.
                tinted = page.copy()
                tinted[:, :, :3] = page[:, :, :3]*tint[:3] + (1-page[:, :, :3])*layer['dark']
                page, tint = tinted, np.array([1, 1, 1, tint[3]])
            result.append((vertices, uvs, triangles, page, tint, layer['blend']))
        return result


def frame_layers(pack, resource, clip=None, time=0, *, skins=None, tracks=(), bone_drives=(), sampler=None):
    """One unflattened frame for a scene renderer, in Spine y-up coordinates."""
    owned = sampler is None
    sampler = sampler or SpineSampler(pack)
    try:
        frame = sampler.sample(resource, clip, [time], skins=skins, tracks=tracks, bone_drives=bone_drives)[0]
        return sampler.draw(resource, frame), dict(bones=frame['bones'], slots=frame['slots'], draw_order=frame['draw_order'],
                                                   source_version=sampler.metadata(resource)['source_version'])
    finally:
        if owned:
            sampler.close()


def _model(value) -> dict:
    if isinstance(value, dict):
        return value
    return next(row for row in manifest()['models'] if value in (row['id'], row['model'], row['key']))


def _tracks(model: dict, clip: str, visual_state: dict) -> tuple:
    primary = model['primary_node']
    setup = [t for t in model['tracks'] if t.get('scope', primary) == primary
             and t['method'] in ('SetupSkins', '_Ready')]
    facts = visual_state.get('facts', {})
    for phase in model.get('phase_tracks', []):
        if phase['fact'] not in facts:
            continue
        value = str(int(facts[phase['fact']]))
        if value not in phase['values']:
            raise ValueError('Invalid original animation phase: ' + phase['fact'])
        setup = [t for t in setup if t['track'] != phase['track']]
        setup.append(dict(track=phase['track'], **phase['values'][value]))
    setup = visual_state.get('setup_tracks', setup)
    post = []
    if 'move' in visual_state:
        move = next(m for m in model['moves'] if m['key'] == visual_state['move'])
        post.extend(t for t in model['tracks'] if t['method'] == move['method']
                    and t.get('scope', primary) == primary
                    and ('applies_to_clips' not in t or clip in t['applies_to_clips']))
    death = model['entries']['die']
    death_clips = {row['clip'] for row in death['variants']}
    if death['playback'] == 'trigger' and clip in death_clips:
        post.extend(t for t in model['tracks'] if t['method'] == 'AfterDeath'
                    and t.get('scope', primary) == primary)
    post.extend(visual_state.get('tracks', []))
    matches = [t for t in model['tracks'] if t['clip'] == clip and t.get('scope', primary) == primary]
    track = visual_state.get('track', matches[0]['track'] if matches else model.get('track', 0))
    if track:
        base = visual_state.get('base_clip', model['initial_clip'])
        if not model.get('shared_group'):
            setup = [dict(track=0, clip=base, loop=True)] + list(setup)
    return setup, post, track


# Independent coordinate, easing and dependency adapters. Original game/runtime
# implementations and generated geometry remain private inputs/outputs.
FASTNOISE_SHA256 = '38b24b9b04aa5e9f1e63336d4acbbbb73b11f15b697bb08a25f5ec0c6c274901'
_FASTNOISE_CALLER = r'''#include "FastNoiseLite.h"
static fastnoiselite::FastNoiseLite configure() {
    fastnoiselite::FastNoiseLite value(0);
    value.SetNoiseType(fastnoiselite::FastNoiseLite::NoiseType_Perlin);
    value.SetFrequency(1.0f);
    value.SetFractalType(fastnoiselite::FastNoiseLite::FractalType_FBm);
    value.SetFractalOctaves(5);
    value.SetFractalLacunarity(2.0f);
    value.SetFractalGain(0.5f);
    value.SetFractalWeightedStrength(0.0f);
    return value;
}
extern "C" float nspire_decim_noise(float x) {
    static const auto noise = configure();
    return noise.GetNoise(x, 0.0f);
}
'''


class _DecimillipedeNoise:
    """Call the reviewed public Godot dependency, without shipping its source."""
    def __init__(self, header, work_dir):
        import ctypes
        import sys
        header = Path(header or os.environ.get('NSPIRE_FASTNOISE_HEADER', ''))
        if not header.is_file():
            raise ValueError('Decimillipede sampling requires the reviewed Godot 4.5.1 FastNoiseLite header; set NSPIRE_FASTNOISE_HEADER')
        identity = hashlib.sha256(_FASTNOISE_CALLER.encode() + header.read_bytes()).hexdigest()[:20]
        directory = Path(work_dir) / ('noise-' + identity)
        extension = '.dylib' if sys.platform == 'darwin' else '.dll' if os.name == 'nt' else '.so'
        library = directory / ('nspire_noise' + extension)
        if not library.is_file():
            directory.mkdir(parents=True, exist_ok=True)
            source = directory / 'caller.cpp'
            source.write_text(_FASTNOISE_CALLER)
            shutil.copyfile(header, directory / 'FastNoiseLite.h')
            compiler = os.environ.get('CXX', 'c++')
            command = [compiler, '-std=c++17', '-O2', '-fwrapv', '-ffp-contract=off', '-fPIC',
                       '-dynamiclib' if sys.platform == 'darwin' else '-shared', str(source), '-o', str(library)]
            subprocess.run(command, check=True)
        self.library = ctypes.CDLL(str(library))
        self.call = self.library.nspire_decim_noise
        self.call.argtypes = [ctypes.c_float]
        self.call.restype = ctypes.c_float

    def __call__(self, value):
        return self.call(float(value))


def _primary_transform(model):
    matrix = np.eye(3)
    for properties in model.get('transform_chain', [model['transform']]):
        matrix = matrix @ _scene_matrix(properties)
    return matrix


def _drive_times(times, step=1/60):
    """Use the same bounded float steps as the Spine adapter, including outputs."""
    clock = np.float32(0)
    result = [float(clock)]
    for requested in times:
        target = np.float32(requested)
        if not np.isfinite(target) or target < clock or target > 120:
            raise ValueError('Driven samples require finite ordered times within [0, 120]')
        while float(target-clock) > .000001:
            clock = np.float32(clock + min(np.float32(step), np.float32(target-clock)))
            result.append(float(clock))
    return result


def _ease_sine(value):
    return (1-math.cos(math.pi * min(1, max(0, value)))) / 2


def _ease_elastic_out(value):
    if value <= 0:
        return 0
    if value >= 1:
        return 1
    return 1 + 2**(-10*value)*math.sin((value-.075)*math.tau/.3)


def _strike_offset(time, initial, stages):
    previous = np.asarray(initial, dtype=float)
    for stage in stages:
        target = np.asarray(stage['position'], dtype=float)
        if time <= stage['duration']:
            return previous + (target-previous)*_ease_sine(time/stage['duration'])
        time -= stage['duration']
        previous = target
    return previous


def bone_drives(model_row, clip, times, visual_state=None, *, noise=None, physics_step=1/60):
    """Godot Drive controls in SpineSprite-relative, y-down coordinates.

    Target positions and creature transforms use the original Godot scene units.
    No target is inferred from an intent. ``node_overrides`` and ``bone_drives``
    let callers carry a prior visual state, including an interrupted tween.
    """
    model = _model(model_row)
    visual_state = visual_state or {}
    controls = model.get('scene_controls', [])
    if not controls:
        return list(visual_state.get('bone_drives', []))
    primary = model['primary_node']
    properties = {row['node']: dict(row['properties']) for row in model['layers']}
    for control in controls:
        properties.setdefault(control['node'], dict(control['properties']))
    initial_properties = {key: dict(value) for key, value in properties.items()}
    for node, values in visual_state.get('node_overrides', {}).items():
        if node not in properties:
            raise ValueError('Unknown original scene node: ' + node)
        properties[node].update(values)
    body = _primary_transform(model)
    creature = _scene_matrix(visual_state.get('creature_transform', {
        'position': visual_state.get('creature_position', [0, 0])}))
    body_global = creature @ body
    inverse_global = np.linalg.inv(body_global)
    initial_creature = _scene_matrix(visual_state.get('initial_creature_transform',
                                                     visual_state.get('creature_transform', {
        'position': visual_state.get('creature_position', [0, 0])})))
    initial_global = initial_creature @ body
    grid = _drive_times(times, physics_step)
    catalog = {row['name']: row for row in model['clips']}
    events = catalog.get(clip, {}).get('events', [])
    targets = [np.asarray(t.get('position') if isinstance(t, dict) else t, dtype=float)
               for t in visual_state.get('targets', [])]
    if any(t.shape != (2,) or not np.all(np.isfinite(t)) for t in targets):
        raise ValueError('Target positions must each contain two finite Godot coordinates')

    def chain(node, changed):
        if node == primary:
            return np.eye(3)
        if not node.startswith(primary + '/'):
            raise ValueError('Drive control is outside its original SpineSprite: ' + node)
        result = np.eye(3)
        cursor = primary
        for part in node[len(primary)+1:].split('/'):
            cursor += '/' + part
            if cursor not in changed:
                raise ValueError('Missing original Drive ancestor: ' + cursor)
            result = result @ _scene_matrix(changed[cursor])
        return result

    result = []
    for control in controls:
        node = control['node']
        props = properties[node]
        if not props.get('enabled', props.get('bone_enabled', True)):
            continue
        path = node
        visible = True
        while path != primary:
            visible = visible and properties[path].get('visible', True)
            path = path.rsplit('/', 1)[0]
        if not visible:
            continue
        binding = control.get('binding', {'kind': 'scene_static'})
        kind = binding['kind']
        cues = [float(e['time']) for e in events if e['name'] == binding.get('event')]
        active = not binding.get('clips') or clip in binding['clips']
        target = min(targets, key=lambda t: t[0]) if targets and binding.get('selection') == 'leftmost' else targets[0] if targets else None
        original = chain(node, initial_properties)
        initial_position = initial_global @ np.array([original[0, 2], original[1, 2], 1])
        cached_position = visual_state.get('cached_global_positions', {}).get(node, initial_position[:2])
        shake = visual_state.get('decimillipede_strike', visual_state.get('move') in ('WRITHE_MOVE', 'BULK_MOVE', 'CONSTRICT_MOVE'))
        elapsed = float(visual_state.get('driver_elapsed', 0))
        initial_scale = float(visual_state.get('coral_scale', binding.get('initial_scale', [1])[0]))
        matrices = []
        for time in grid:
            changed = {key: dict(value) for key, value in properties.items()}
            current = changed[node]
            if kind == 'bot_fall':
                local_y = visual_state.get('creature_local_position', [0, 0])[1]
                sy = float(model['transform'].get('scale', [1, 1])[1])
                if sy == 0:
                    raise ValueError('Bot body has zero vertical scale')
                current['position'] = [0, (binding['ground_local_y']-local_y)/sy]
            elif kind == 'clam_growth':
                scale = initial_scale
                curves = []
                for cue in cues:
                    if time < cue:
                        break
                    at_cue = initial_scale
                    for start, old, target_scale in curves:
                        if cue >= start:
                            at_cue = old + (target_scale-old)*_ease_elastic_out((cue-start)/binding['duration'])
                    target_scale = min(at_cue+binding['step'], binding['maximum'])
                    curves.append((cue+binding['delay'], target_scale-binding['step'], target_scale))
                for start, old, target_scale in curves:
                    if time >= start:
                        scale = old + (target_scale-old)*_ease_elastic_out((time-start)/binding['duration'])
                current['scale'] = [scale, scale]
            elif kind == 'shrink_scale':
                desired = binding['shrunk'] if visual_state.get('shrink', False) else binding['normal']
                scale = desired
                if 'shrink_from' in visual_state:
                    start = float(visual_state.get('shrink_started', 0))
                    old = float(visual_state['shrink_from'])
                    scale = old + (desired-old)*_ease_sine((time-start)/binding['duration'])
                current['scale'] = [scale, scale]
            elif kind == 'target_local_delta_x' and active and target is not None:
                current['position'] = [float(target[0]-creature[0, 2]), binding['local_y']]
            elif kind == 'decimillipede_noise':
                if noise is None:
                    raise ValueError('Decimillipede requires the reviewed native noise sampler')
                driver = binding['driver_node']
                origin = np.asarray(properties[driver].get('position', [0, 0]), dtype=float)
                offset = np.zeros(2)
                if time > 0 or elapsed > 0:
                    u = np.float32(np.float32(time+elapsed)*np.float32(binding['speed'])+np.float32(binding['phase']))
                    offset = np.asarray([noise(u), noise(np.float32(u+np.float32(binding['axis_phase'])))])*binding['magnitude']
                strike = visual_state.get('strike_offset', [0, 0])
                if shake:
                    strike = _strike_offset(time, strike, binding['strike'])
                changed[driver]['position'] = (origin + offset + strike).tolist()
            elif kind not in ('scene_static', 'target_global_x', 'target_global_xy', 'target_local_delta_x', 'cached_global_position'):
                raise ValueError('Unknown original bone driver: ' + kind)
            matrix = chain(node, changed)
            if kind == 'target_global_x' and active:
                if target is not None:
                    world = body_global @ matrix
                    sx = model['transform'].get('scale', [1, 1])[0]
                    world[0, 2] = target[0] + binding['offset_times_visuals_scale_x']*sx
                    matrix = inverse_global @ world
                elif 'no_target_local_position' in binding:
                    current['position'] = binding['no_target_local_position']
                    matrix = chain(node, changed)
            elif kind == 'target_global_xy' and active and target is not None and cues and time >= cues[0]:
                world = body_global @ matrix
                world[:2, 2] = target + np.asarray(binding['offset'])
                matrix = inverse_global @ world
            elif kind == 'cached_global_position' and cues and time >= cues[0]:
                world = body_global @ matrix
                world[:2, 2] = cached_position
                matrix = inverse_global @ world
            if not np.all(np.isfinite(matrix)):
                raise ValueError('Non-finite Drive control: ' + node)
            matrices.append(matrix[:2, :].reshape(-1).tolist())
        item = dict(bone=props['bone_name'], matrix=matrices[0])
        if any(value != matrices[0] for value in matrices[1:]):
            item['keyframes'] = [dict(time=t, matrix=value) for t, value in zip(grid[1:], matrices[1:])]
        result.append(item)
    explicit = {row['bone']: row for row in visual_state.get('bone_drives', [])}
    result = [explicit.pop(row['bone'], row) for row in result]
    result.extend(explicit.values())
    return result


def _compose(sampler: SpineSampler, model: dict, clip: str, times, skins, visual_state) -> list:
    setup, tracks, track = _tracks(model, clip, visual_state)
    controls = bone_drives(model, clip, times, visual_state, noise=sampler.decimillipede_noise)
    frames = sampler.sample(model, clip, times, skins=skins, setup_tracks=setup,
                            tracks=tracks, track=track, bone_drives=controls,
                            loop=any(s['clip'] == clip and s['loop'] for s in model['states']))
    primary = np.eye(3)
    inherited_color = np.ones(4)
    for transform in model.get('transform_chain', [model['transform']]):
        primary = primary @ FLIP_Y @ _scene_matrix(transform) @ FLIP_Y
        inherited_color *= transform.get('modulate', [1, 1, 1, 1])
    self_color = model.get('transform_chain', [model['transform']])[-1].get('self_modulate', [1, 1, 1, 1])
    result = []
    children = [layer for layer in model['layers'] if 'skeleton' in layer]
    child_samples = {}
    child_starts = {}
    for child in children:
        child_clip = child.get('clip', 'empty')
        selected = visual_state.get('children', {}).get(child['node'], {})
        start = 0.0
        loop = child.get('loop', False)
        activation = child.get('activation')
        if activation and 'clip' not in selected:
            events = next(c for c in model['clips'] if c['name'] == clip).get('events', [])
            cues = [e['time'] for e in events if e['name'] == activation['event']]
            if cues:
                start, child_clip, loop = cues[0], activation['clip'], activation['loop']
        child_clip = selected.get('clip', child_clip)
        start = selected.get('start', start)
        child_starts[child['node']] = start
        child_samples[child['node']] = sampler.sample(child, child_clip, [max(0, t-start) for t in times],
            skins=selected.get('skins', child.get('skins', ['default'])), loop=loop)
    for frame_index, frame in enumerate(frames):
        draw = sampler.draw(model, frame, primary)
        draw = [(vs, uv, tri, tex, tint*inherited_color*self_color, blend)
                for vs, uv, tri, tex, tint, blend in draw]
        insertions = []
        for child in children:
            if times[frame_index] < child_starts[child['node']]:
                continue
            host_path = child['node'].rsplit('/', 1)[0]
            host = next((n for n in model['layers'] if n['node'] == host_path), None)
            if host is None or host['type'] not in ('SpineSlotNode', 'SpineBoneNode'):
                raise ValueError('Unresolved child skeleton attachment: ' + child['node'])
            props = host['properties']
            if host['type'] == 'SpineSlotNode':
                anchor = next(s for s in frame['slots'] if s['slot'] == props['slot_name'])
                preceding = sum(1 for d in frame['draw'] if next(i for i, s in enumerate(frame['slots']) if s['index'] == d['index']) < next(i for i, s in enumerate(frame['slots']) if s['index'] == anchor['index']))
                own_count = sum(d['index'] == anchor['index'] for d in frame['draw'])
                # The original sprite moves every SlotNode immediately after
                # this slot's mesh, overriding serialized behind-parent flags.
                at = preceding + own_count
            else:
                anchor = next(b for b in frame['bones'] if b['name'] == props['bone_name'])
                at = 0 if props.get('show_behind_parent', False) else len(draw)
            if not props.get('visible', True) or not child['properties'].get('visible', True):
                continue
            if 'node_matrix' in anchor:
                transform = primary @ FLIP_Y @ _bone_matrix(anchor['node_matrix']) @ _scene_matrix(child['properties']) @ FLIP_Y
            else:
                transform = primary @ _bone_matrix(anchor['matrix']) @ FLIP_Y @ _scene_matrix(child['properties']) @ FLIP_Y
            layers = sampler.draw(child, child_samples[child['node']][frame_index], transform)
            child_color = inherited_color * props.get('modulate', [1, 1, 1, 1])
            child_color *= child['properties'].get('modulate', [1, 1, 1, 1])
            child_color *= child['properties'].get('self_modulate', [1, 1, 1, 1])
            layers = [(vs, uv, tri, tex, tint*child_color, blend)
                      for vs, uv, tri, tex, tint, blend in layers]
            insertions.append((at, layers))
        for at, layers in sorted(insertions, key=lambda item: item[0], reverse=True):
            draw[at:at] = layers
        result.append(draw)
    return result


def bake_enemy(pack, model_row, clip, texture=None, *, frames=16, height=88, max_width=160,
               cache=None, sampler=None, skin_variant=None, visual_state=None, measure_only=False):
    """Bake one explicit original clip with its selected skin and skeletal layers.

    Shared KaiserCrab rows require visual_state['shared_background']=True; scene
    renderers normally use SpineSampler directly to render that one common actor.
    A caller's visual_state may select setup_tracks, tracks, move, base_clip,
    track and per-child clips. It is never inferred from an unrelated attack.
    """
    model = _model(model_row)
    visual_state = visual_state or {}
    if model.get('shared_group') and not visual_state.get('shared_background'):
        raise ValueError(model['model'] + ' renders through the shared KaiserCrab scene')
    if not 1 <= frames <= 256 or height < 5 or max_width < 5:
        raise ValueError('Invalid enemy frame dimensions/count')
    skins = model['skins']['default'] if skin_variant is None else ([skin_variant] if isinstance(skin_variant, str) else list(skin_variant))
    unavailable = model['skins'].get('unavailable', [])
    skins = [name for name in skins if name not in unavailable]
    catalog = {c['name']: c for c in model['clips']}
    if clip not in catalog:
        raise ValueError(f"{model['model']} has no original clip {clip}")
    original_duration = catalog[clip].get('duration', catalog[clip]['duration_ms'] / 1000)
    duration = original_duration
    if model['model'].startswith('DecimillipedeSegment') and visual_state.get('decimillipede_strike', visual_state.get('move') in ('WRITHE_MOVE', 'BULK_MOVE', 'CONSTRICT_MOVE')):
        duration = 1.25
    duration = float(visual_state.get('duration_seconds', duration))
    if not math.isfinite(duration) or not 0 <= duration <= 120:
        raise ValueError('Explicit animation duration must be finite and within [0, 120]')
    times = [index * duration / frames for index in range(frames)]
    owned = sampler is None
    sampler = sampler or SpineSampler(pack, texture=texture)
    cache = {} if cache is None else cache
    try:
        samples = _compose(sampler, model, clip, times, skins, visual_state)
        nonempty = [d[0] for ds in samples for d in ds if len(d[0])]
        if not nonempty:
            raise ValueError(f"{model['model']}/{clip} has no visible skeletal geometry")
        vertices = np.concatenate(nonempty)
        low, high = vertices.min(axis=0), vertices.max(axis=0)
        idle_clip = model['entries']['idle']['clip']
        idle = _compose(sampler, model, idle_clip, [0], skins, visual_state)
        reference = np.concatenate([d[0] for ds in idle for d in ds if len(d[0])])
        reference_low, reference_high = reference.min(axis=0), reference.max(axis=0)
        scale_key = ('enemy-scale', model['id'], tuple(skins), height, max_width)
        if scale_key not in cache:
            span = np.maximum(reference_high-reference_low, 1)
            cache[scale_key] = min((height - 4) / span[1], (max_width - 4) / span[0])
        layout = visual_state.get('layout')
        if layout:
            scale = float(layout['scale'])
            origin = np.asarray(layout['origin'], dtype=float)
            width, pixel_height = map(int, layout['size'])
            if not layout.get('crop', False):
                bounds_low, bounds_high = np.asarray(layout['bounds'])
                if np.any(low < bounds_low-.001) or np.any(high > bounds_high+.001):
                    raise ValueError('Geometry escaped its measured common model layout')
        else:
            scale = visual_state.get('scale', cache[scale_key])
            width, pixel_height = np.ceil((high-low)*scale).astype(int) + 4
            origin = np.array([2-low[0]*scale, 2+high[1]*scale])
        if not measure_only and (width > 320 or pixel_height > 240 or width<1 or pixel_height<1):
            raise ValueError(f"{model['model']}/{clip} exceeds the calculator stage at shared scale: {width}x{pixel_height}")
        images = [] if measure_only else [raster(draw, scale, origin, (int(width), int(pixel_height))) for draw in samples]
        return images, dict(name=model['skeleton'], model=model['model'], clip=clip,
            source_version=model['source_version'], runtime_sha256=getattr(sampler, 'runtime_identity', None),
            skins=skins, duration_ms=max(1, round(duration*1000)), original_duration_ms=max(1, round(original_duration*1000)),
            frame_ms=max(1, round(duration*1000/frames)), frames=frames,
            pivot=[round(origin[0]), round(origin[1])], size=[int(width), int(pixel_height)],
            bounds=[low.tolist(), high.tolist()], reference_bounds=[reference_low.tolist(), reference_high.tolist()],
            scale=scale, physics_step=1/60, godot_y_down=True,
            drive_nodes=len(model.get('scene_controls', [])),
            visual_state=visual_state)
    finally:
        if owned:
            sampler.close()
