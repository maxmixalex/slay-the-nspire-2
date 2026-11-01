#!/usr/bin/env python3
"""Source-bound Neow/Tez CPU lights and Neow's deferred original vignette.

This renderer specializes only the Ancient sampler. It leaves the shared world
renderer and its source-cache identity unchanged. Existing Neow frame IDs are
rebuilt without the one intervening vignette; no second scene family is added.
"""
from __future__ import annotations

from contextlib import contextmanager
from functools import lru_cache
import hashlib
import json
import math
from pathlib import Path
import struct

import numpy as np
from PIL import Image

import content_tiles as t
import world_scene_assets as world

BASE_AUX, AUXILIARY, BASE_IMAGES, IMAGES = 7215, 7217, 59674, 59677
MAGIC, VERSION, STEP_MS, RECORD_BYTES = 0x4c41, 1, 33, 32
LIGHT = 'images/vfx/light.png'
SCRIPT = 'scenes/backgrounds/little_light_script.gd'
VIGNETTE = 'animations/backgrounds/neow_room/neow_vignette.png'
LIGHT_SHA = '3658f5fa81e9e00238496f5b3968da250a4ebc9dee31e567dbb45b36b3c3b62e'
VIGNETTE_SHA = '4930de546d84d3b421e6fa22a90cde6c84a296baa187f1ff4e67f2364c82c236'
SCALE_POINTS = ((.321019, .507939, .36242), (.417635, .507939, .423164))
EXPECTED = ((10, 3, 546, 18000, 20), (90, 17, 506, 16667, 179))


def round_signed(value):
    return np.where(value >= 0, np.floor(value + .5), np.ceil(value - .5)).astype(np.int64)


def geometry(matrix, scale):
    """Native quad coordinates: Q8 center/extent, turn4096 and Q16 shear.

    Perpendicular height, rather than the second column's norm, retains the
    complete affine transform when a source bone or node introduces shear.
    """
    a, b, x, c, d, y = np.asarray(matrix)[:2].ravel()
    norm = math.hypot(a, c)
    determinant = a*d-b*c
    t.require(norm > 0 and determinant > 0, 'Light transform must be finite and non-reflecting')
    values = np.array([x*256, y*256, norm*256*scale*256,
                       determinant/norm*256*scale*256,
                       math.atan2(c, a)*4096/(2*math.pi),
                       (a*b+c*d)/determinant*65536])
    t.require(np.all(np.isfinite(values)), 'Nonfinite light transform')
    result = round_signed(values)
    t.require(np.max(np.abs(result)) < 2**31, 'Light projection exceeds signed words')
    return result


def curve_variant(loader, ref):
    attrs, props = loader.resource(ref)
    values = world._property(props, '_data')
    t.require(attrs.get('type') == 'Curve' and len(values) == 15 and
              world._property(props, 'point_count') == 3, 'Unexpected light scale curve')
    points = []
    for i in range(3):
        point = world._vector(values[i*5])
        t.require(tuple(values[i*5+1:i*5+5]) == (0., 0., 0, 0), 'Light curve tangents changed')
        points.append(tuple(point))
    for index, heights in enumerate(SCALE_POINTS):
        if points == list(zip((0., .502521, 1.), heights)):
            return index
    raise ValueError('Unreviewed Ancient light scale curve')


def node_record(renderer, node, path, parent_matrix, parent_tint, parent_z, clips):
    p = node.props
    value = lambda name, default=None: world._property(p, name, default)
    t.require(not clips and not node.children, 'Light has an unhandled clip or child')
    t.require(renderer.loader.path(value('script')) == SCRIPT, 'Light script identity changed')
    shader, material, blend, _ = renderer.material(node)
    t.require(shader is None and blend == 1, 'Light requires the original additive material')
    t.require(value('lifetime', 1) == 3 and value('amount', 8) in (1, 2) and
              value('explosiveness', 0) == 0 and value('randomness', 0) == 0 and
              value('initial_velocity_min', 0) == value('initial_velocity_max', 0) == 0 and
              np.array_equal(world._vector(value('gravity', (0, 980))), [0, 0]) and
              value('emission_shape', 0) == 0,
              'Light lifetime/emission/motion differs from the reviewed stationary source')
    # No angular, orbit or acceleration animation may silently disappear.
    for prefix in ('angular_velocity', 'orbit_velocity', 'linear_accel', 'radial_accel', 'tangential_accel', 'damping', 'angle'):
        t.require(value(prefix+'_min', 0) == value(prefix+'_max', 0) == 0,
                  'Unhandled light particle motion: '+prefix)
    scale = value('scale_amount_min', 1)
    t.require(scale > 0 and scale == value('scale_amount_max', 1), 'Random light size is unsupported')
    tint = parent_tint*world._vector(value('modulate', (1, 1, 1, 1)), 4)*world._vector(value('self_modulate', (1, 1, 1, 1)), 4)
    t.require(np.array_equal(tint, np.ones(4)), 'Light inherited tint changed')
    attrs, ramp = renderer.loader.resource(value('color_ramp'))
    offsets = world._property(ramp, 'offsets').args
    colors = world._property(ramp, 'colors').args
    t.require(attrs.get('type') == 'Gradient' and tuple(offsets) == (0, .411546, .994413) and
              tuple(colors) == (.34902, .34902, .34902, 1, 1, 1, 1, 1, .34902, .34902, .34902, 1),
              'Light color ramp changed')
    color = world._vector(value('color', (1, 1, 1, 1)), 4)
    t.require(np.all((color >= 0) & (color <= 1)), 'Light color outside normalized range')
    z = int(value('z_index', 0)) + (parent_z if value('z_as_relative', True) else 0)
    matrix = renderer.screen @ parent_matrix @ world._node_matrix(p)
    return dict(path=path, matrix=matrix[:2].ravel().tolist(), geometry=geometry(matrix, scale).tolist(),
                color=round_signed(color*255).tolist(), amount=int(value('amount', 8)),
                local_coords=bool(value('local_coords', False)), curve=curve_variant(renderer.loader, value('scale_amount_curve')),
                scale=float(scale), z=z)


class Marker:
    def __init__(self, kind, record=None, layer=None):
        self.kind, self.record, self.layer = kind, record, layer


class LightRenderer(world._Renderer):
    def visit(self, node, parent_matrix, parent_size, parent_tint, parent_z, path, clips=(), bones=None):
        visible = world._property(node.props, 'visible', True)
        ref = world._property(node.props, 'texture')
        if visible and node.kind == 'CPUParticles2D' and ref and self.loader.path(ref) == LIGHT:
            record = node_record(self, node, path, parent_matrix, parent_tint, parent_z, clips)
            self.commands.append((record['z'], Marker('light', record), 1, ()))
            return
        if visible and path == 'Neow/TextureRect2':
            t.require(node.kind == 'TextureRect' and not node.children and not clips and
                      ref and self.loader.path(ref) == VIGNETTE and self.material(node)[0] is None,
                      'Neow deferred vignette node changed')
            begin = len(self.commands)
            super().visit(node, parent_matrix, parent_size, parent_tint, parent_z, path, clips, bones)
            t.require(len(self.commands) == begin+1, 'Vignette must produce exactly one draw')
            z, image, blend, masks = self.commands[-1]
            t.require(blend == 0 and not masks, 'Vignette is not an ordinary unclipped layer')
            self.commands[-1] = (z, Marker('vignette', layer=image), blend, masks)
            return
        return super().visit(node, parent_matrix, parent_size, parent_tint, parent_z, path, clips, bones)

    def finish(self, plane='all'):
        t.require(plane == 'all' and self.canvas == (320, 240), 'Ancient light camera must remain native320x240')
        commands = sorted(self.commands, key=lambda item: item[0])
        light_positions = [i for i, row in enumerate(commands) if isinstance(row[1], Marker) and row[1].kind == 'light']
        vignette_positions = [i for i, row in enumerate(commands) if isinstance(row[1], Marker) and row[1].kind == 'vignette']
        scene = 0 if self.diagnostics['path'].endswith('/neow.tscn') else 1
        t.require(len(light_positions) == EXPECTED[scene][0] and
                  len(vignette_positions) == (1 if scene == 0 else 0), 'Ancient light/vignette source count changed')
        other = [i for i, row in enumerate(commands) if row[1] is not None and not isinstance(row[1], Marker)]
        t.require(other and max(other) < min(light_positions), 'A light interleaves unsupported source mesh layers')
        lights = [commands[i][1].record for i in light_positions]
        if scene == 0:
            split = vignette_positions[0]
            t.require(sum(i < split for i in light_positions) == 7, 'Neow must draw7lights/vignette/3lights')
            vignette = commands[split][1].layer
            for position, row in zip(light_positions, lights): row['layer'] = int(position > split)
        else:
            vignette = None
            for row in lights: row['layer'] = 1
        # The ordinary source oracle contains no CPU particles, matching the
        # old renderer. It is separate from the intentionally deferred plate.
        self.commands = [(z, image.layer, blend, clips) if isinstance(image, Marker) and image.kind == 'vignette'
                         else (z, image, blend, clips) for z, image, blend, clips in commands
                         if not isinstance(image, Marker) or image.kind == 'vignette']
        ordinary, _ = super().finish(plane)
        if scene == 0:
            self.commands = [row for row in commands if not isinstance(row[1], Marker)]
            background, diagnostics = super().finish(plane)
        else:
            background, diagnostics = ordinary, self.diagnostics
        from ancient_scene_ui23 import rgb565
        diagnostics['authored_cpu_lights'] = dict(nodes=lights, order='7lights/vignette/3lights' if scene == 0 else 'scene/90lights',
            ordinary_source_rgb565_sha256=t.sha256(rgb565(ordinary).tobytes()),
            ordinary_source_rgba_sha256=t.sha256(ordinary.tobytes()),
            deferred_vignette_rgba=vignette.tobytes() if vignette else None)
        return background, diagnostics


@contextmanager
def specialized_renderer():
    previous = world._Renderer
    t.require(previous is LightRenderer.__bases__[0], 'Concurrent Ancient renderer specialization is not allowed')
    world._Renderer = LightRenderer
    try: yield
    finally: world._Renderer = previous


def pack_parameters(scene, frames):
    nodes, expected_dynamic, count, duration, particles = EXPECTED[scene]
    t.require(len(frames) == count and all(len(row) == nodes for row in frames), 'Light sample frame/node count changed')
    baseline = frames[0]
    coordinates = np.asarray([[row['geometry'] for row in frame] for frame in frames], dtype=np.int64)
    dynamic = [i for i in range(nodes) if np.any(coordinates[:, i] != coordinates[0, i])]
    t.require(len(dynamic) == expected_dynamic and sum(row['amount'] for row in baseline) == particles,
              'Light moving-node/particle count differs from the authored scene')
    records, deltas = bytearray(), bytearray()
    for i, row in enumerate(baseline):
        for frame in frames:
            t.require(all(frame[i][key] == row[key] for key in ('path', 'color', 'amount', 'local_coords', 'curve', 'scale', 'z', 'layer')),
                      'Light definition/order changes within its source loop')
        t.require(row['local_coords'] or i not in dynamic, 'Global particle emission moves with an unhandled bone')
        if not row['local_coords']:
            t.require(all(frame[i].get('matrix') == row.get('matrix') for frame in frames),
                      'Global-coordinate light parent is not source-static')
        slot = dynamic.index(i) if i in dynamic else 255
        records.extend(struct.pack('<6i4B4B', *row['geometry'], *row['color'], row['amount'], slot, row['layer'], row['curve']))
        if slot != 255:
            delta = coordinates[:, i]-coordinates[0, i]
            t.require(np.all((delta >= -32768) & (delta <= 32767)), 'Light bone delta exceeds signed16')
            deltas.extend(delta.astype('<i2').tobytes())
    raw_length = 32+len(records)+len(deltas)
    header = (MAGIC, VERSION, scene, nodes, len(dynamic), count, STEP_MS, duration & 65535,
              duration >> 16, RECORD_BYTES, 32, 32+len(records), raw_length & 65535, raw_length >> 16,
              BASE_IMAGES if scene == 0 else 65535, 0)
    raw = struct.pack('<16H', *header)+records+deltas
    raw += bytes((-len(raw)) % 512)
    parameters(raw, scene)
    return raw, dict(nodes=nodes, particles=particles, moving_nodes=dynamic, frames=count, duration_ms=duration,
                     bytes=len(raw), raw_bytes=raw_length, paths=[row['path'] for row in baseline])


def parameters(raw, scene):
    t.require(scene in (0, 1) and len(raw) >= 32 and len(raw) % 512 == 0, 'Invalid light parameter extent')
    header = struct.unpack_from('<16H', raw)
    nodes, dynamic, count, duration, particles = EXPECTED[scene]
    length = 32+nodes*RECORD_BYTES+dynamic*count*12
    t.require(header == (MAGIC, VERSION, scene, nodes, dynamic, count, STEP_MS, duration & 65535,
                        duration >> 16, RECORD_BYTES, 32, 32+nodes*RECORD_BYTES,
                        length & 65535, length >> 16, BASE_IMAGES if scene == 0 else 65535, 0) and
              len(raw) == (length+511)//512*512 and not any(raw[length:]), 'Light header/offset/padding differs')
    rows = [struct.unpack_from('<6i4B4B', raw, 32+i*RECORD_BYTES) for i in range(nodes)]
    slots = [row[11] for row in rows if row[11] != 255]
    t.require(slots == list(range(dynamic)) and sum(row[10] for row in rows) == particles and
              all(row[2] > 0 and row[3] > 0 and row[10] in (1, 2) and row[12] in (0, 1) and row[13] in (0, 1) for row in rows),
              'Light node amount/geometry/curve/dynamic mapping invalid')
    t.require([row[12] for row in rows] == ([0]*7+[1]*3 if scene == 0 else [1]*nodes), 'Light layer order changed')
    t.require(sum(row[13] for row in rows) == scene, 'Alternate Tez curve must occur once')
    for i, row in enumerate(rows):
        if row[11] == 255:
            coordinates = np.asarray(row[:6], dtype=np.int64).reshape(1, 6)
        else:
            start = 32+nodes*RECORD_BYTES+row[11]*count*12
            delta = np.frombuffer(raw, '<i2', count*6, start).reshape(count, 6).astype(np.int64)
            t.require(not np.any(delta[0]), 'Light delta starts nonzero')
            coordinates = delta+np.asarray(row[:6])
        t.require(np.all(np.abs(coordinates) < 8388608) and np.all(coordinates[:, 2:4] > 0), 'Light coordinate exceeds bounded native arithmetic')
        # Largest source scale is.507939; use the looser.51 bound. This keeps
        # the existing quad's signed32 clipped bounding-box numerator safe.
        w, h = (coordinates[:, axis]*51//100+1 for axis in (2, 3))
        t.require(np.all(w+h+(np.abs(coordinates[:, 5])*h+65535)//65536 < 2097152),
                  'Light quad exceeds native bounding-box arithmetic')
    return dict(scene=scene, nodes=nodes, frames=count, dynamic=dynamic, duration_ms=duration, rows=rows)


def sample_scene(pack, sampler, runtime, scene, work):
    import animation
    from ancient_scene_ui23 import SampledLoop, emit, rgb565, signature
    from build_content import texture
    from world_raster import raster
    scene_index = 0 if scene['key'] == 'NEOW' else 1
    t.require(scene['key'] in ('NEOW', 'TEZCATARA'), 'Unrecognized animated Ancient scene')
    identity = hashlib.sha256((signature(pack, runtime, scene)+t.sha256(Path(__file__).read_bytes())).encode()).hexdigest()
    directory = Path(work)/(scene['key'].lower()+'-lights-'+identity[:16])
    directory.mkdir(parents=True, exist_ok=True)
    count = EXPECTED[scene_index][2]
    metadata = sampler.metadata(scene['resource'])
    t.require([(clip['name'], round(clip['duration']*1000)) for clip in metadata['clips']] ==
              [(scene['clip'], scene['duration_ms'])], 'Authored Ancient loop changed')
    light = texture(pack, LIGHT).convert('RGBA')
    t.require(light.size == (256, 256) and t.sha256(light.tobytes()) == LIGHT_SHA, 'Original light texture changed')
    if scene_index == 0:
        image = texture(pack, VIGNETTE).convert('RGBA')
        t.require(image.size == (2560, 1200) and t.sha256(image.tobytes()) == VIGNETTE_SHA,
                  'Original Neow vignette pixels changed')
    paths = [directory/f'{frame:04d}.rgb565' for frame in range(count)]
    records = [directory/f'{frame:04d}.json' for frame in range(count)]
    times = [frame*STEP_MS/1000 for frame in range(count)]
    pending = []
    for frame, (path, receipt) in enumerate(zip(paths, records)):
        if path.is_file() and receipt.is_file():
            row = json.loads(receipt.read_text(encoding='utf-8'))
            t.require(row.get('signature') == identity and path.stat().st_size == 320*240*2 and
                      t.sha256(path.read_bytes()) == row['source_rgb565_sha256'], 'Ancient light frame cache is corrupt')
        else:
            t.require(not receipt.exists(), 'Ancient light frame receipt has no matching pixels')
            pending.append(frame)
    loop = SampledLoop(sampler, scene, times) if pending else None
    load_texture = lru_cache(maxsize=32)(texture)
    previous_raster = animation.raster
    try:
        animation.raster = raster
        with specialized_renderer():
            for frame in pending:
                picture, details = world.render_scene(pack, scene['path'], 320, 240,
                    viewport=(1680, 1260), mode='scene', sampler=loop, sample_time=times[frame], texture_loader=load_texture)
                lights = details.pop('authored_cpu_lights')
                vignette = lights.pop('deferred_vignette_rgba')
                if vignette is not None:
                    target = directory/'vignette.rgba'
                    if target.exists(): t.require(target.read_bytes() == vignette, 'Deferred vignette moves within source loop')
                    else: t.atomic_bytes(target, vignette)
                    lights['vignette_rgba_sha256'] = t.sha256(vignette)
                raw = rgb565(picture).tobytes()
                t.atomic_bytes(paths[frame], raw)
                row = dict(signature=identity, frame=frame, milliseconds=frame*STEP_MS,
                    source_rgb565_sha256=t.sha256(raw), lights=lights,
                    diagnostics=details if frame == 0 else None)
                t.atomic_bytes(records[frame], (json.dumps(row, separators=(',', ':'))+'\n').encode())
                if frame in (0, count//2, count-1): picture.save(directory/f'{frame:04d}.png')
                if frame % 20 == 0: emit('ancient_lights_sample', key=scene['key'], frame=frame, frames=count)
    finally:
        animation.raster = previous_raster
        load_texture.cache_clear()
    rows = [json.loads(path.read_text(encoding='utf-8')) for path in records]
    raw, typed = pack_parameters(scene_index, [row['lights']['nodes'] for row in rows])
    t.atomic_bytes(directory/'light-parameters.bin', raw)
    if scene_index == 0:
        vignette = (directory/'vignette.rgba').read_bytes()
        t.require(len(vignette) == 320*240*4 and all(row['lights']['vignette_rgba_sha256'] == t.sha256(vignette) for row in rows),
                  'Deferred vignette cache identity changed')
    report = dict(scene, signature=identity, frames=count, frame_ms=STEP_MS, size=[320, 240], viewport=[1680, 1260],
        source_rgb565_sha256=[row['source_rgb565_sha256'] for row in rows],
        original_unlit_source_rgb565_sha256=[row['lights']['ordinary_source_rgb565_sha256'] for row in rows],
        diagnostics=rows[0]['diagnostics'], exact_source_cadence_at_calculator_tick=True,
        authored_lights=typed, light_parameters_sha256=t.sha256(raw),
        source_order=rows[0]['lights']['order'], particle_systems_implemented=False,
        source_family='original-scene-with-only-Neow-vignette-deferred',
        source_script_sha256=t.sha256(pack.read(SCRIPT+'c')),
        cosmetic_boundary='stable native preprocessing seed; original random seed/process epoch is unavailable')
    t.atomic_bytes(directory/'frames.json', (json.dumps(report, indent=2)+'\n').encode())
    return directory, report


def validate(source, image):
    t.require((len(source.aux), len(source.images)) == (AUXILIARY, IMAGES) and
              source.aux[BASE_AUX:] == [BASE_IMAGES+1, BASE_IMAGES+2], 'Ancient light extension catalog differs')
    w, h, fmt, raw = image(BASE_IMAGES)
    t.require((w, h, fmt) == (320, 240, 3) and len(raw) == 512+320*240*2,
              'Deferred Neow vignette geometry/alpha changed')
    rows = []
    for scene in range(2):
        w, h, fmt, raw = image(BASE_IMAGES+1+scene)
        t.require(w == 256 and h*512 == len(raw) and fmt == 0, 'Light parameter image geometry/format changed')
        rows.append(parameters(raw, scene))
    return rows


def vignette_image(rgba):
    rgba=np.asarray(rgba,dtype='u1')
    t.require(rgba.shape==(240,320,4),'Vignette must retain the native scene extent')
    rgb=rgba.astype('u2')
    colors=((rgb[:,:,0]>>3)<<11)|((rgb[:,:,1]>>2)<<5)|(rgb[:,:,2]>>3)
    palette,indices=np.unique(colors,return_inverse=True)
    t.require(len(palette)<=256,'Original vignette no longer fits the exact existing palette')
    padded=np.zeros(256,dtype='<u2');padded[:len(palette)]=palette
    raw=padded.tobytes()+indices.astype('u1').tobytes()+rgba[:,:,3].tobytes()
    t.require(np.array_equal(padded[indices].reshape(240,320),colors),'Vignette palette changes RGB565 pixels')
    return 320,240,3,raw


def verify_scene_family(image, timeline, directory, record):
    """Reject lights appended to old baked-vignette or unrelated scene pixels.

    Both existing approved source policies are accepted: exact RGB565, or the
    complete cyclic bounded hold. Mixed policies cannot pass frame by frame.
    """
    from ancient_scene_ui23 import held_colors
    originals=[]
    for frame in range(timeline['frames']):
        raw=(Path(directory)/f'{frame:04d}.rgb565').read_bytes()
        t.require(len(raw)==320*240*2 and t.sha256(raw)==record['source_rgb565_sha256'][frame],
                  'Deferred Ancient source-frame cache changed')
        originals.append(np.frombuffer(raw,'<u2').reshape(240,320))
    chosen,_=held_colors(originals)
    exact_ok=held_ok=True;base=previous=None
    for frame in range(timeline['frames']):
        w,h,fmt,raw=image(timeline['first']+frame)
        t.require((w,h,fmt)==(320,240,4),'Ancient light source family must use opaque copy patches')
        _,colors,alpha=t.validate_sparse(raw,w,h,pixels=True)
        t.require(np.all((alpha==0)|(alpha==255)), 'Ancient copy patch contains partial alpha')
        reference=None if not frame else base if frame%timeline['key_interval']==0 else previous
        restored=np.zeros_like(colors) if reference is None else reference.copy()
        restored[alpha!=0]=colors[alpha!=0]
        if not frame:base=restored.copy()
        previous=restored
        exact_ok=exact_ok and np.array_equal(restored,originals[frame])
        held_ok=held_ok and np.array_equal(restored,chosen[frame])
        t.require(exact_ok or held_ok,'Ancient lights cannot overlay an old or mismatched baked scene family')
    return dict(frames=timeline['frames'],matched_policy='exact' if exact_ok else 'cyclic-hold-r1-g2-b1-exact-fallback')


def append(data, sampled, work):
    from coalesce_tile_resources import view
    from patch_frontend_assets import patch_pack
    from split_content import LANGUAGES
    from no_animations import reduced, mark
    from tanx_scene_ui23 import group_images
    source = view(data)
    t.require(source.version == 46 and (len(source.aux), len(source.images), len(source.clips)) == (BASE_AUX, BASE_IMAGES, 1783),
              'Ancient light append requires the exact Tanx endpoint')
    t.require(len(sampled) == 2 and [row['key'] for _, row in sampled] == ['NEOW', 'TEZCATARA'], 'Missing sampled light family')
    from no_animations import ancient_rows
    reader=t.TileReader(source)
    image=lambda index: reader.image_raw(index) if source.images[index][3]==6 else source.image_raw(index)
    timelines=ancient_rows(source,image)
    t.require(not reduced(data),'Append authored lights before reducing the complete source scene family')
    binding=[verify_scene_family(image,timeline,directory,record)
             for timeline,(directory,record) in zip(timelines,sampled)]
    work = Path(work); work.mkdir(parents=True, exist_ok=True)
    rgba = np.frombuffer((Path(sampled[0][0])/'vignette.rgba').read_bytes(), 'u1').reshape(240, 320, 4)
    images = [vignette_image(rgba)]
    for scene, (directory, record) in enumerate(sampled):
        raw = (Path(directory)/'light-parameters.bin').read_bytes()
        t.require(t.sha256(raw) == record['light_parameters_sha256'], 'Light source parameter cache changed')
        parameters(raw, scene)
        images.append((256, len(raw)//512, 0, raw))
    rows = []
    for i, (w, h, fmt, raw) in enumerate(images):
        filename = f'{i}.bin'; t.atomic_bytes(work/filename, raw)
        rows.append(dict(width=w, height=h, format=fmt, file=filename, bytes=len(raw), sha256=t.sha256(raw), raw_sha256=t.sha256(raw)))
    # The154,112-byte vignette is one existing palette/alpha8 image, below the
    # established512KiB decoder bound. Tanx's own default remains128KiB.
    art = group_images(rows, work, group_bytes=256*1024)
    manifest = dict(images=rows, auxiliary_image_ids=[BASE_IMAGES+1, BASE_IMAGES+2], localized_text={code:{} for code in LANGUAGES})
    output, detail = patch_pack(data, manifest, work, base_aux=BASE_AUX, auxiliary=AUXILIARY, input_version=46, version=46,
        texts=(), base_clips=len(source.clips), base_strings=len(source.strings), appended_art=art)
    if reduced(data): output = mark(output)
    target = view(output); validate(target, target.image_raw)
    t.require(target.clips == source.clips and target.cards == source.cards and target.aux[:BASE_AUX] == source.aux and
              reduced(output) == reduced(data), 'Ancient lights changed old identities/profile')
    for i, row in enumerate(images): t.require(target.image_raw(BASE_IMAGES+i) == row, 'Light typed pixels differ after encode')
    return output, dict(complete=True, input_sha256=t.sha256(data), output_sha256=t.sha256(output),
        input_bytes=len(data), output_bytes=len(output), added_bytes=len(output)-len(data),
        grouping=art['report'], append=detail, raw_sha256=[t.sha256(row[3]) for row in images],
        decoded_bytes=sum(len(row[3]) for row in images), appended_images=3,
        duplicated_scene_frames=0, source_families=[row['signature'] for _, row in sampled],
        source_frame_binding=binding,
        framebuffer_growth=0, particle_pool_growth=0, profile_preserved=True)


def main():
    import argparse
    from pck import Pack
    from world_enemy_assets import SpineSampler
    from ancient_scene_ui23 import SCENES
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('pck', 'runtime', 'work'): parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--input', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--sample-only', action='store_true')
    args = parser.parse_args()
    t.require(args.sample_only or args.input and args.output, 'Supply input/output or sample-only')
    for path in (args.pck, args.runtime, args.work, args.input, args.output):
        if path is not None: t.private_path(path, 'Private original Ancient light source/output')
    pack = Pack(args.pck)
    try:
        with SpineSampler(pack, runtime=args.runtime, work_dir=args.work/'adapter') as sampler:
            sampled = [sample_scene(pack, sampler, args.runtime, scene, args.work/'sampled') for scene in SCENES]
        if not args.sample_only:
            output, proof = append(args.input.read_bytes(), sampled, args.work/'append')
            t.atomic_bytes(args.output, output)
            t.atomic_bytes(args.output.with_suffix('.json'), (json.dumps(proof, indent=2)+'\n').encode())
    finally: pack.close()


if __name__ == '__main__': main()
