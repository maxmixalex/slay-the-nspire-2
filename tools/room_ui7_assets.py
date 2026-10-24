"""Pinned room UI7 artwork; exact official Spine constraints stay offline.

Hands are sampled at five-degree local rotate_me intervals, including both
weighted meshes and the original transform constraints. The runtime selects
the nearest pose. No whole-image rotation or independent constraint substitute
is used. The common pivot/scale keep scene registration stable across poses.
"""
from __future__ import annotations
import json
import hashlib
from pathlib import Path
import shutil
import numpy as np
from PIL import Image

from animation import raster
from build_content import texture
from card_banner import material_tint
from content_tiles import require
from patch_feedback_assets import write_image
from world_enemy_assets import SpineSampler

BASE_AUX, COUNT = 1320, 108
HAND_SAMPLES = 47
HAND_SIZE, HAND_PIVOT = (320, 240), (160, 230)
HAND_SCALE = .4 * 2 / 9
ANCIENTS = ('neow', 'darv', 'nonupeipe', 'tanx', 'orobas', 'pael', 'tezcatara', 'vakuu')
HANDS = (
    ('merchant', 'animations/backgrounds/merchant_room/hand/merchant_hand_skel_data.tres'),
    ('fake-merchant', 'animations/backgrounds/fake_merchant_room/hand/fake_merchant_hand_skel_data.tres'),
)


def image_row(cache, key, image, *, sparse=False):
    row = write_image(cache, 'room-ui7-' + key, image, sparse=sparse)
    row['key'] = 'room_ui7.' + key
    return row


def source_checks(pack):
    inventory = pack.read('scenes/merchant/merchant_inventory.tscn').decode()
    fake = pack.read('scenes/events/custom/fake_merchant_inventory.tscn').decode()
    for scene in (inventory, fake):
        for expected in ('offset_left = -842.0', 'offset_right = 905.0',
                         'position = Vector2(234, -54)', 'scale = Vector2(0.4, 0.4)'):
            require(expected in scene, 'Changed original merchant hand transform: ' + expected)
    dialogue = pack.read('scenes/merchant/merchant_rug_dialogue.tscn').decode()
    for expected in ('shader_parameter/h = 1.0', 'shader_parameter/s = 1.2',
                     'shader_parameter/v = 0.4', 'rotation = -3.14159'):
        require(expected in dialogue, 'Changed original merchant dialogue: ' + expected)


def cached_hands(source, target, sampler):
    """Reuse the completed private hand stage without accepting stale poses.

    The separately retained skeleton/atlas inputs are compared with the exact
    source resources before reusing geometry. The source PCK itself remains the
    caller's pinned v0.111.0 input, as for all other UI asset stages.
    """
    source = Path(source)
    manifest = json.loads((source / 'room-ui7-sampling.json').read_text())
    rows_path = source / 'room-ui7-rows.json'
    if not rows_path.is_file():
        rows_path = source / 'rows-stage2.json'
    rows = json.loads(rows_path.read_text())[:2 * HAND_SAMPLES]
    geometry = manifest['hand_samples']
    require(manifest['runtime_sha256'] == sampler.runtime_identity,
            'Merchant hand cache uses a different Spine runtime')
    require(tuple(manifest['hand_size']) == HAND_SIZE and
            tuple(manifest['hand_pivot']) == HAND_PIVOT and
            manifest['hand_scale'] == HAND_SCALE and
            len(rows) == len(geometry) == 2 * HAND_SAMPLES,
            'Merchant hand cache framing/count changed')
    for hand, (name, resource) in enumerate(HANDS):
        resolved, metadata = sampler.resolve(resource), sampler.metadata(resource)
        for field in ('local_skeleton', 'local_atlas'):
            current = Path(resolved[field])
            previous = source / 'spine-adapter' / 'inputs' / current.name
            require(previous.read_bytes() == current.read_bytes(),
                    'Merchant hand cache original skeleton/atlas changed')
        for index in range(HAND_SAMPLES):
            at = hand * HAND_SAMPLES + index
            row, bounds = rows[at], geometry[at]
            key = f'{name}-hand-{index:02d}'
            require(row['key'] == 'room_ui7.' + key and bounds['key'] == key and
                    bounds['degrees'] == -132 + index * 5 and bounds['source'] == resource and
                    bounds['source_version'] == metadata['source_version'] and
                    (row['width'], row['height'], row['format'], row['decoded_format']) == (*HAND_SIZE, 5, 4),
                    'Merchant hand cache order/source/pose changed')
            raw = (source / row['file']).read_bytes()
            require(len(raw) == row['bytes'] and hashlib.sha256(raw).hexdigest() == row['sha256'],
                    'Merchant hand cache payload changed')
            destination = target / row['file']
            if destination.resolve() != (source / row['file']).resolve():
                destination.write_bytes(raw)
            png = source / (key + '.png')
            if png.is_file() and png.resolve() != (target / png.name).resolve():
                shutil.copyfile(png, target / png.name)
    return rows, geometry


def build_images(pack, cache, *, runtime=None, dotnet=None, work_dir=None, hand_cache=None):
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    source_checks(pack)
    rows, geometry = [], []
    with SpineSampler(pack, runtime=runtime, dotnet=dotnet,
                      work_dir=work_dir or cache / 'spine-adapter') as sampler:
        if hand_cache is not None:
            rows, geometry = cached_hands(hand_cache, cache, sampler)
        for name, resource in (() if hand_cache is not None else HANDS):
            metadata = sampler.metadata(resource)
            require(metadata['source_version'].startswith('4.2.'), 'Unexpected merchant skeleton version')
            for index in range(HAND_SAMPLES):
                angle = -132 + index * 5
                frame = sampler.sample(resource, 'default', [0],
                                       bone_drives=[dict(bone='rotate_me', local_rotation=angle)])[0]
                draw = sampler.draw(resource, frame)
                vertices = np.concatenate([layer[0] for layer in draw])
                pixels = np.column_stack((vertices[:, 0] * HAND_SCALE + HAND_PIVOT[0],
                                          HAND_PIVOT[1] - vertices[:, 1] * HAND_SCALE))
                lo, hi = pixels.min(axis=0), pixels.max(axis=0)
                # Cropping a moving weighted arm silently is not an acceptable
                # optimization. Any changed bound requires reviewed framing.
                require(np.all(lo >= 0) and np.all(hi < HAND_SIZE),
                        f'{name} angle {angle} exceeds common hand canvas: {lo}..{hi}')
                image = raster(draw, HAND_SCALE, np.asarray(HAND_PIVOT), HAND_SIZE)
                key = f'{name}-hand-{index:02d}'
                rows.append(image_row(cache, key, image, sparse=True))
                geometry.append(dict(key=key, degrees=angle, source=resource,
                                     source_version=metadata['source_version'],
                                     pixel_bounds=[*lo.tolist(), *hi.tolist()],
                                     pixel_alpha_bounds=image.getbbox()))
                if index in (0, 12, 24, 36, 46):
                    image.save(cache / (key + '.png'))
        runtime_identity = sampler.runtime_identity
    bubble = material_tint(pack, texture(pack, 'images/packed/vfx/speech_bubble3.png'),
                           'scenes/merchant/merchant_rug_dialogue.tscn')
    bubble = bubble.transpose(Image.Transpose.ROTATE_180).resize((70, 48), Image.Resampling.LANCZOS)
    rows.append(image_row(cache, 'merchant-dialogue', bubble))
    for frame in ('01', '02', '04'):
        image = texture(pack, 'images/rooms/merchant_room/card_removal_' + frame + '.png')
        rows.append(image_row(cache, 'merchant-removal-' + frame,
                              image.resize((60, 60), Image.Resampling.LANCZOS)))
    rows.append(image_row(cache, 'ancient-dialogue', texture(pack, 'images/ui/dialogue_nine_patch.png').resize((26, 19), Image.Resampling.LANCZOS)))
    rows.append(image_row(cache, 'ancient-dialogue-tail', texture(pack, 'images/ui/dialogue_tail.png').resize((8, 9), Image.Resampling.LANCZOS)))
    for name in ANCIENTS:
        icon = texture(pack, f'images/ui/run_history/{name}.png').convert('RGBA')
        outline = texture(pack, f'images/ui/run_history/{name}_outline.png').convert('RGBA')
        require(icon.size == outline.size, 'Mismatched ancient speaker icon/outline')
        shadow = Image.new('RGBA', icon.size)
        shadow.putalpha(outline.getchannel('A').point(lambda a: round(a * .5019608)))
        shadow.alpha_composite(icon)
        rows.append(image_row(cache, 'ancient-speaker-' + name,
                              shadow.resize((20, 20), Image.Resampling.LANCZOS)))
    require(len(rows) == COUNT, 'Room UI7 auxiliary order/count mismatch')
    (cache / 'room-ui7-sampling.json').write_text(json.dumps(dict(
        hand_size=HAND_SIZE, hand_pivot=HAND_PIVOT, hand_scale=HAND_SCALE,
        runtime_sha256=runtime_identity, hand_samples=geometry,
        reused_hand_cache=str(Path(hand_cache).resolve()) if hand_cache is not None else None,
        auxiliary_first=BASE_AUX, auxiliary_count=BASE_AUX + COUNT), indent=2) + '\n')
    (cache / 'room-ui7-rows.json').write_text(json.dumps(rows, indent=2) + '\n')
    return rows
