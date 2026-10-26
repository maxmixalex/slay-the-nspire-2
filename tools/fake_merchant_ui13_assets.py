"""Original Fake Merchant custom scene: complete, registered room animation.

The separately supplied PCK/runtime remain private. No ordinary-merchant
animation or shop atlas substitutes for this event's source scene. Static
pixel factoring is exact after RGB565 quantization, before lossless packing.
"""
from __future__ import annotations

import json
import hashlib
import os
from pathlib import Path

import numpy as np
from PIL import Image

import content_tiles as tiles
from pck import Pack
from patch_feedback_assets import write_image
from random_character_ui13_assets import godot_noise_image
from world_enemy_assets import SpineSampler
from world_raster import raster
from world_scene_assets import load_scene_texture
from world_rest_actor_assets import _FrameSampler
from world_room_shaders import RoomRenderer

BASE_AUXILIARY, AUXILIARY_COUNT = 3343, 5011
NO_RUG_AUXILIARY_COUNT = 5156
SCENE = 'scenes/events/custom/fake_merchant.tscn'
SCALE = 218/1080
LEFT = (320-1920*SCALE)/2
FPS = 24
REGENT_BODY = (0, 0, 160, 190)
REGENT_EYE = (92, 77, 100, 83)
REGENT_ATLAS = (160, 196)
# source resource, exact full period expressed in 24Hz samples, world origin,
# world scale, skin, fixed native crop. The converter rejects escaped ink.
SEQUENCES = (
 ('bottom', 'animations/backgrounds/fake_merchant_room/bottom/shop_fake_merchant_bottom.tres',
  144, (-10, 20), .505, None, (0, 0, 320, 218)),
 ('foreground', 'animations/backgrounds/fake_merchant_room/bottom/shop_fake_merchant_bottom_cutter.tres',
  144, (-10, 20), .505, None, (0, 0, 320, 218)),
 ('merchant', 'animations/backgrounds/fake_merchant_room/top/fake_merchant_top.tres',
  80, (1303, 761), .470095, None, (185, 60, 90, 120)),
 ('merchant-outline', 'animations/backgrounds/fake_merchant_room/top/fake_merchant_top.tres',
  80, (1303, 761), .470095, ('outline',), (185, 60, 90, 120)),
 ('ironclad', 'animations/characters/ironclad/ironclad_skel_data.tres',
  288, (570.75, 782.75), .49, None, (0, 0, 160, 190)),
 ('silent', 'animations/characters/silent/silent_skel_data.tres',
  168, (590, 779.25), .5075, None, (0, 0, 160, 190)),
 ('defect', 'animations/characters/defect/defect_skel_data.tres',
  288, (569, 768.75), .21875, None, (0, 0, 160, 190)),
 ('regent', 'animations/characters/regent/regent_skel_data.tres',
  320, (565.5, 781), .5075, None, (0, 0, 160, 190)),
 ('necrobinder', 'animations/characters/necrobinder/necrobinder_skel_data.tres',
  144, (558.5, 805.5), .4725, None, (0, 0, 160, 190)),
)


def quantized(pixels):
    v = np.asarray(pixels, dtype=np.uint32)
    rgb = ((v[:, :, 0] >> 3) << 11) | ((v[:, :, 1] >> 2) << 5) | (v[:, :, 2] >> 3)
    return np.where(v[:, :, 3], rgb | (v[:, :, 3] << 16), 0)


def invariant_planes(frames):
    """Disjoint pixels preserve source-over, including partial edge alpha."""
    reference = quantized(frames[0])
    fixed = np.ones(reference.shape, dtype=bool)
    for frame in frames[1:]:
        fixed &= quantized(frame) == reference
    static = np.array(frames[0], dtype=np.uint8, copy=True)
    static[~fixed] = 0
    moving = []
    for frame in frames:
        pixels = np.array(frame, dtype=np.uint8, copy=True)
        pixels[fixed] = 0
        if not np.array_equal(quantized(static) | quantized(pixels), quantized(frame)):
            raise ValueError('Fake Merchant disjoint factoring changed source pixels')
        moving.append(Image.fromarray(pixels))
    return Image.fromarray(static), moving, int(np.count_nonzero(fixed & (reference != 0)))


def source_checks(pack):
    room = pack.read(SCENE).decode()
    for token in ('offset_left = -970.0', 'offset_top = -520.0',
                  'scale = Vector2(1.01, 1.01)', 'offset_left = -398.0',
                  'offset_top = 276.0', 'scale = Vector2(1.75, 1.75)',
                  'amount = 300', 'amount = 30', 'lifetime = 14.0',
                  'preprocess = 15.0', 'shader_parameter/speed = 0.388',
                  'shader_parameter/strength = 0.058'):
        tiles.require(token in room, 'Fake Merchant scene changed: '+token)
    button = pack.read('scenes/events/custom/fake_merchant_button.tscn').decode()
    for token in ('offset_left = 174.0', 'offset_top = -195.0',
                  'position = Vector2(169, 416)', 'scale = Vector2(0.470095, 0.470095)'):
        tiles.require(token in button, 'Fake Merchant button registration changed: '+token)


def _image(draw, scale, origin):
    # In these relaxed clips the additive eye/orb fragments are enclosed by
    # opaque earlier body pixels. Verify that for EVERY frame before baking
    # them into a normal plane; otherwise the background blend would change.
    # This avoids extra planes only where source coverage proves it safe.
    indices = [i for i, row in enumerate(draw) if row[-1] != 0]
    if indices:
        tiles.require(all(draw[i][-1] == 1 for i in indices),
                      'Fake Merchant player has unreviewed multiply/screen material')
        for index in indices:
            before = np.asarray(raster(draw[:index], scale, origin, (320, 218)))
            emission = np.asarray(raster([draw[index]], scale, origin, (320, 218)))
            lit = np.any(emission[:, :, :3] != 0, axis=2) & (emission[:, :, 3] != 0)
            tiles.require(not np.any(lit & (before[:, :, 3] != 255)),
                          'Fake Merchant additive fragments need a separate ordered plane')
    return raster(draw, scale, origin, (320, 218))


def regent_material_order(frame):
    # Visible source attachments are not a fixed-length list: the minion's
    # Normal sweat appears for62 of the320 samples. Keep that actual order.
    # An invisible eye also must remain invisible, never synthesized.
    materials=[(row['slot'],row['blend']) for row in frame['draw'] if row['blend']]
    tiles.require(not materials or materials==[('eye add',1)],
                  'Regent relaxed source contains an unreviewed emissive slot or material')
    return bool(materials)


def regent_atlas(draw, scale, origin):
    """One cache entry, original Normal/Add/Normal order and straight alpha.

    The complete320-sample eye union is only8x6 pixels. Outside it all source
    fragments are Normal, so keep their exact normal composite as one body.
    Inside its hole store Before/Add/After separately. This avoids retaining
    duplicate large occluded body planes and stays within320x240.
    """
    additive = [i for i, row in enumerate(draw) if row[-1] != 0]
    tiles.require(not additive or len(additive) == 1 and draw[additive[0]][-1] == 1,
                  'Regent relaxed material ordering changed')
    if not additive:
        body=raster(draw,scale,origin,(320,218));outside=np.array(body)[:,:,3]
        outside[:190,:160]=0
        tiles.require(not np.any(outside),'Regent source body escaped its registered atlas crop')
        atlas=Image.new('RGBA',REGENT_ATLAS);atlas.paste(body.crop(REGENT_BODY),(0,0))
        return atlas
    index = additive[0]
    # Raster this ONE emissive slot as Normal against transparency so RGB
    # stays straight. Raster(Add) against black would premultiply RGB, then
    # runtime Add would multiply by alpha again and visibly dim the edge.
    eye = [draw[index][:-1] + (0,)]
    body = raster(draw[:index]+draw[index+1:], scale, origin, (320, 218))
    emission = raster(eye, scale, origin, (320, 218))
    outside = np.array(body)[:, :, 3]; x, y, right, bottom = REGENT_BODY
    outside[y:bottom,x:right] = 0
    tiles.require(not np.any(outside), 'Regent source body escaped its registered atlas crop')
    outside = np.array(emission)[:, :, 3]; x, y, right, bottom = REGENT_EYE
    outside[y:bottom,x:right] = 0
    tiles.require(not np.any(outside), 'Regent source band escaped its registered atlas crop')
    atlas = Image.new('RGBA', REGENT_ATLAS)
    body = body.crop(REGENT_BODY);body.paste((0,0,0,0),REGENT_EYE);atlas.paste(body,(0,0))
    planes = [raster(draw[:index], scale, origin, (320, 218)),emission,
              raster(draw[index+1:], scale, origin, (320, 218))]
    for plane,target in zip(planes,((0,190),(8,190),(16,190))):
        atlas.paste(plane.crop(REGENT_EYE),target)
    return atlas


class NecrobinderActor:
    """CreateVisuals keeps its flameattach shader alive during relaxed_loop.

    NNecrobinderVfx._Ready disables scythe/hurt/low-health emitters; none of
    their starting events occurs in this clip. UpdateFlameVisibility hides
    HeadBoneNode only for die. Follow the real slot, not its editor pose.
    """
    def __init__(self, pack, sampler, resource, metadata):
        self.proxy = _FrameSampler(sampler, resource, metadata)
        self.renderer = RoomRenderer(pack, 320, 218, (1920, 1080),
                                     load_scene_texture, self.proxy, 0., 1)
        self.root = self.renderer.loader.instantiate('scenes/creature_visuals/necrobinder.tscn')
        spine = next(child for child in self.root.children if child.name == 'Visuals')
        head = next(child for child in spine.children if child.name == 'HeadBoneNode')
        flame = next(child for child in head.children if child.name == 'SteppedFireMix_dark')
        head.children, head.child_ops = [flame], [(flame, -1)]
        spine.children, spine.child_ops = [head], [(head, -1)]
        self.root.children, self.root.child_ops = [spine], [(spine, -1)]
        self.matrix = np.array([[1.75, 0., 562.], [0., 1.75, 816.], [0., 0., 1.]])

    def image(self, frame):
        renderer = self.renderer
        self.proxy.frame = frame; self.proxy.time = renderer.time = frame['time']
        renderer.commands = []
        renderer.diagnostics['room_shaders'] = []
        renderer.visit(self.root, self.matrix, np.array([1920., 1080.]),
                       np.ones(4), 0, self.root.name)
        image, diagnostics = renderer.finish()
        shaders = diagnostics['room_shaders']
        tiles.require(len(shaders) == 1 and shaders[0]['kind'] == 'dark_fire',
                      'Necrobinder relaxed head flame was not rendered')
        return image


def _sequence(pack, sampler, cache, spec, first, exclude_slots=()):
    key, resource, count, position, scale, skin, crop = spec
    # Explicit sequence names avoid relying on the numeric append directory.
    clip = 'idle_loop' if key in ('bottom', 'bottom-no-rug', 'foreground', 'merchant', 'merchant-outline') else 'relaxed_loop'
    # A failed later actor must not invalidate already complete sequences.
    # This build-time receipt checks source/code identities and every retained
    # payload. It adds no integrity pass to the calculator runtime.
    files = ('fake_merchant_ui13_assets.py','world_enemy_assets.py','SpineSampleAdapter.cs',
             'world_raster.py','animation.py','world_scene_assets.py','world_room_shaders.py',
             'world_rest_actor_assets.py','sparse_image.py','patch_feedback_assets.py')
    identity = hashlib.sha256(b''.join((Path(__file__).parent/name).read_bytes() for name in files)+
        json.dumps([spec,first,list(exclude_slots)]).encode()+sampler.runtime_identity.encode()).hexdigest()
    receipt = cache/('fake-'+key+'-receipt.json')
    if receipt.is_file():
        previous = json.loads(receipt.read_text())
        if previous.get('source_identity') == identity:
            for row in previous['images']:
                payload = (cache/row['file']).read_bytes()
                tiles.require(len(payload)==row['bytes'] and tiles.sha256(payload)==row['sha256'],
                              'Incomplete Fake Merchant sequence payload: '+row['file'])
            return previous['images'],previous['sequence']
    metadata = sampler.metadata(resource)
    source = next(row for row in metadata['clips'] if row['name'] == clip)
    tiles.require(abs(source['duration']-count/FPS) < .00001,
                  'Fake Merchant original animation duration changed: '+key)
    times = [i/FPS for i in range(count)]
    sampled = sampler.sample(resource, clip, times, skins=skin, loop=True, physics_step=1/60)
    origin = np.array([LEFT+position[0]*SCALE, position[1]*SCALE])
    attached = NecrobinderActor(pack, sampler, resource, metadata) if key == 'necrobinder' else None
    x, y, w, h = crop
    frames = [];visible_draw_counts={};visible_eye_frames=0
    for frame in sampled:
        drawing = sampler.draw(resource, frame)
        if exclude_slots:
            found = {row['slot'] for row in frame['draw'] if row['slot'] in exclude_slots}
            tiles.require(found==set(exclude_slots), 'Fake Merchant hidden rug slot changed')
            drawing = [geometry for row, geometry in zip(frame['draw'], drawing) if row['slot'] not in exclude_slots]
        if key == 'regent':
            visible_eye_frames+=int(regent_material_order(frame))
            visible_draw_counts[len(frame['draw'])]=visible_draw_counts.get(len(frame['draw']),0)+1
            frames.append(regent_atlas(drawing, scale*SCALE, origin))
        else:
            full = attached.image(frame) if attached else _image(drawing, scale*SCALE, origin)
            pixels = np.asarray(full)
            outside = pixels[:, :, 3].copy(); outside[y:y+h, x:x+w] = 0
            tiles.require(not np.any(outside), 'Fake Merchant frame escaped declared crop: '+key)
            frames.append(full.crop((x, y, x+w, y+h)))
    static, moving, stable_pixels = invariant_planes(frames)
    rows = []
    for index, image in enumerate([static]+moving):
        row = write_image(cache, f'fake-{key}-{index:03d}', image, sparse=True)
        row.update(auxiliary=first+index, source=resource, clip=clip,
                   frame=index-1, static=index == 0, frames=count, nominal_fps=FPS,
                   original_duration_seconds=source['duration'], crop=list(crop),
                   source_position=list(position), source_scale=scale,
                   skin=list(skin) if skin else ['default'])
        if key == 'regent':row['ordered_bands'] = [0, 0, 1, 0]
        rows.append(row)
    for index in (0, count//3, count*2//3):
        frames[index].save(cache/f'fake-{key}-{"atlas" if key=="regent" else "source"}-{index:03d}.png')
    static.save(cache/f'fake-{key}-static.png')
    summary = dict(key=key, first=first, frames=count, crop=list(crop),
                   invariant_pixels=stable_pixels, source_version=metadata['source_version'],
                   original_duration_seconds=source['duration'],
                   constraints=metadata.get('constraints'), skin=skin,
                   stored_bytes=sum(row['bytes'] for row in rows),
                   exact_factoring_frames=count, additive_background_coverage_verified=key != 'regent')
    if key == 'regent':
        summary['ordered_bands'] = [dict(source=[0,0,160,190],destination=[0,0],blend='Normal'),
            dict(source=[0,190,8,6],destination=[92,77],blend='Normal'),
            dict(source=[8,190,8,6],destination=[92,77],blend='Add',alpha='straight source sample'),
            dict(source=[16,190,8,6],destination=[92,77],blend='Normal')]
        summary['body_hole'] = list(REGENT_EYE)
        summary['packed_dimensions'] = list(REGENT_ATLAS)
        summary['visible_draw_counts'] = visible_draw_counts
        summary['visible_eye_add_frames'] = visible_eye_frames
    if attached:
        summary['attached_shader'] = 'shaders/vfx/vfx_stepped_shader_fire_dark.tres'
        summary['attached_slot'] = 'flameattach'
        summary['shader_sampling'] = 'same24Hz original source times as the complete6s relaxed body period'
    if exclude_slots:summary['hidden_source_slots'] = list(exclude_slots)
    tiles.atomic_bytes(receipt,json.dumps(dict(source_identity=identity,images=rows,sequence=summary),indent=2).encode()+b'\n')
    return rows, summary


def _resume_prefix(path, cache, runtime_identity):
    """Retain only the seven immutable, successfully sampled cp16 sequences.

    This is an explicit artifact input with its own source provenance, not a
    relaxed code-cache signature. A fresh invocation still samples all nine.
    """
    raw = Path(path).read_bytes(); retained = json.loads(raw)
    tiles.require(retained.get('schema')==1 and
        retained.get('source_checkpoint')=='e5cdd7f28098a12bfd01d257584d157d12035498' and
        retained.get('reference')=='v0.111.0 / 41cef1ea' and
        retained.get('auxiliary_start')==BASE_AUXILIARY and retained.get('auxiliary_end')==4542 and
        retained.get('runtime_sha256')==runtime_identity,
        'Unknown Fake Merchant retained prefix provenance')
    rows, sequences = retained.get('images', []), retained.get('sequences', [])
    tiles.require(len(rows)==1199 and len(sequences)==7, 'Incomplete Fake Merchant retained prefix')
    offset = 0; root = cache.resolve()
    for spec, sequence in zip(SEQUENCES[:7], sequences):
        key, source, count, position, scale, skin, crop = spec
        clip = 'idle_loop' if key in ('bottom','foreground','merchant','merchant-outline') else 'relaxed_loop'
        tiles.require(sequence.get('key')==key and sequence.get('first')==BASE_AUXILIARY+offset and
            sequence.get('frames')==count and sequence.get('crop')==list(crop) and
            sequence.get('exact_factoring_frames')==count and
            sequence.get('additive_background_coverage_verified') is True,
            'Fake Merchant retained sequence recipe differs: '+key)
        for frame, row in enumerate(rows[offset:offset+count+1]):
            tiles.require(row.get('auxiliary')==BASE_AUXILIARY+offset+frame and row.get('source')==source and
                row.get('clip')==clip and row.get('frame')==frame-1 and row.get('static')==(frame==0) and
                row.get('frames')==count and row.get('nominal_fps')==FPS and
                row.get('crop')==list(crop) and row.get('source_position')==list(position) and
                row.get('source_scale')==scale and row.get('skin')==(list(skin) if skin else ['default']) and
                row.get('width')==crop[2] and row.get('height')==crop[3] and
                abs(row.get('original_duration_seconds',0)-count/FPS)<.00001,
                'Fake Merchant retained frame recipe differs: '+key)
            payload_path=(cache/row['file']).resolve()
            tiles.require(payload_path.is_relative_to(root) and payload_path.name==f'fake-{key}-{frame:03d}.bin',
                          'Unexpected Fake Merchant retained payload path')
            payload=payload_path.read_bytes()
            tiles.require(len(payload)==row.get('bytes') and tiles.sha256(payload)==row.get('sha256'),
                          'Changed Fake Merchant retained payload: '+row['file'])
        offset += count+1
    tiles.require(offset==len(rows), 'Retained Fake Merchant prefix has surplus frames')
    return rows, sequences, dict(source_checkpoint=retained['source_checkpoint'],
        prefix_sha256=tiles.sha256(raw), images=len(rows), auxiliary_start=BASE_AUXILIARY, auxiliary_end=4542)


def source_assets(pck, cache, *, runtime=None, dotnet=None, fastnoise_header=None, resume_prefix=None):
    cache = Path(cache); cache.mkdir(parents=True, exist_ok=True)
    pack, rows, sequences = Pack(pck), [], []
    retained = None
    try:
        source_checks(pack)
        with SpineSampler(pack, runtime=runtime, dotnet=dotnet,
                          work_dir=cache/'adapter') as sampler:
            if resume_prefix:
                rows, sequences, retained = _resume_prefix(resume_prefix, cache, sampler.runtime_identity)
            for spec in SEQUENCES[len(sequences):]:
                frames, metadata = _sequence(pack, sampler, cache, spec, BASE_AUXILIARY+len(rows))
                rows.extend(frames); sequences.append(metadata)
                tiles.atomic_bytes(cache/'fake-merchant-progress.json',
                                   json.dumps(dict(completed=sequences), indent=2).encode()+b'\n')
        header = fastnoise_header or os.environ.get('NSPIRE_FASTNOISE_HEADER')
        images = (
            ('noise', 'NoiseTexture2D_b02o1', godot_noise_image(50, 50, .0259, .093, header, cache/'noise')),
            ('glitter', 'animations/backgrounds/fake_merchant_room/bottom/fake_merchant_glitter.png', None),
            ('square-speck', 'images/packed/vfx/generic/square_speck.png', None),
        )
        for key, source, image in images:
            if image is None:image = load_scene_texture(pack, source).convert('RGBA')
            row = write_image(cache, 'fake-'+key, image)
            row.update(auxiliary=BASE_AUXILIARY+len(rows), source=source)
            rows.append(row)
        tiles.require(BASE_AUXILIARY+len(rows) == AUXILIARY_COUNT, 'Fake Merchant append directory changed')
        evidence = dict(schema=1, source_scene=SCENE, sequences=sequences,
            auxiliary_start=BASE_AUXILIARY, auxiliary_end=AUXILIARY_COUNT,
            source_animation_frames=sum(row[2] for row in SEQUENCES),
            source_camera=[1920,1080], destination_camera=[320,218],
            source_reference='v0.111.0 / 41cef1ea', fps=FPS, physics_fps=60,
            static_factoring='exact disjoint RGB565+alpha invariants across every frame',
            particle_texture_references=dict(light=3341, dot=3342),
            runtime_particles=dict(lights=5, stars=300, square_specks=[30,30],
                adaptation='bounded 60Hz fixed-point CPU curl field in place of GPU turbulence'),
            images=rows)
        if retained:evidence['retained_prefix']=retained
        tiles.atomic_bytes(cache/'fake-merchant-assets.json', json.dumps(evidence, indent=2).encode()+b'\n')
    finally:
        pack.close()
    return rows


def source_no_rug_assets(pck, cache, *, runtime=None, dotnet=None, base=5011, end=5156):
    """Only the post-fight acquired-rug variant; never resample the prior1668.

    NFakeMerchant hides the source rug bone after the fight if the genuine
    FakeMerchantsRug relic was obtained. The remaining animation keeps all
   144 source samples, rather than painting over its moving highlights.
    """
    tiles.require(base==AUXILIARY_COUNT and end==NO_RUG_AUXILIARY_COUNT,'Changed no-rug append range')
    cache=Path(cache);cache.mkdir(parents=True,exist_ok=True)
    pack=Pack(pck)
    try:
        source_checks(pack)
        spec=('bottom-no-rug',)+SEQUENCES[0][1:]
        with SpineSampler(pack,runtime=runtime,dotnet=dotnet,work_dir=cache/'adapter') as sampler:
            rows,sequence=_sequence(pack,sampler,cache,spec,base,exclude_slots=('rug',))
        tiles.require(len(rows)==end-base,'Changed no-rug source sample count')
        evidence=dict(schema=1,source_scene=SCENE,auxiliary_start=base,auxiliary_end=end,
                      condition='StartedFight && player.HasRelic<FakeMerchantsRug>()',
                      source_animation_frames=144,sequence=sequence,images=rows)
        tiles.atomic_bytes(cache/'fake-merchant-no-rug-assets.json',json.dumps(evidence,indent=2).encode()+b'\n')
        return rows
    finally:pack.close()
