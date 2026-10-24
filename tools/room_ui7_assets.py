"""Pinned room UI7 artwork; exact official Spine constraints stay offline.

Hands are sampled at five-degree local rotate_me intervals, including both
weighted meshes and the original transform constraints. The runtime selects
the nearest pose. No whole-image rotation or independent constraint substitute
is used. The common pivot/scale keep scene registration stable across poses.
"""
from __future__ import annotations
import json
from pathlib import Path
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


def build_images(pack, cache, *, runtime=None, dotnet=None, work_dir=None):
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    source_checks(pack)
    rows, geometry = [], []
    with SpineSampler(pack, runtime=runtime, dotnet=dotnet,
                      work_dir=work_dir or cache / 'spine-adapter') as sampler:
        for name, resource in HANDS:
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
    rows.append(image_row(cache, 'ancient-dialogue', texture(pack, 'images/ui/dialogue_nine_patch.png')))
    rows.append(image_row(cache, 'ancient-dialogue-tail', texture(pack, 'images/ui/dialogue_tail.png')))
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
        auxiliary_first=BASE_AUX, auxiliary_count=BASE_AUX + COUNT), indent=2) + '\n')
    return rows
