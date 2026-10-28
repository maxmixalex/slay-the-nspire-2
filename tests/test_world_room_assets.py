"""Asset-free checks for original room registration and queued animation."""
from copy import deepcopy
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from world_room_assets import render_rest, sample_treasure
from world_room_profile import room_plan, room_profile
from world_ui_profile import BASE_CLIPS, ENEMY_CLIP_ROWS, MAP_CLIP_ROWS, ROOM_CLIP_ROWS


class Pack:
    def __init__(self, documents):
        self.documents = documents
        self.entries = dict.fromkeys(documents)

    def read(self, path):
        return self.documents[path.removeprefix('res://')].encode()


def no_texture(pack, path):
    raise AssertionError('This synthetic room has no texture resources')


class Sampler:
    def metadata(self, resource):
        return dict(source_version='4.2', available_skins=['normal', 'outline'],
            clips=[dict(name='animation', duration=2., events=[]),
                   dict(name='shine_fade', duration=1., events=[])])

    def sample(self, resource, clip, times, **settings):
        self.request = dict(resource=resource, clip=clip, times=times, **settings)
        return [dict(time=value) for value in times]

    def draw(self, resource, frame):
        return [(np.array([[0., 0.], [2., 0.], [0., -2.]]),
                 np.array([[0., 0.], [1., 0.], [0., 1.]]),
                 np.array([[0, 1, 2]]), np.array([[[1., 0., 0., 1.]]]), np.ones(4), 0)]


def treasure_fixture():
    profile = deepcopy(room_profile())
    profile['treasure']['spine_node']['evaluated_origin'] = [1., 2.]
    profile['treasure']['spine_node']['scale'] = [1., 1.]
    profile['treasure']['families'] = [dict(key='treasure.TEST', path='private.tres',
                                           skin='normal', outline_skin='outline')]
    for item, duration in zip(profile['treasure']['animation_queue'], [2., 1.]):
        item['source_duration_seconds'] = duration
    row = dict(resource='private.tres', family='treasure.TEST', kind='treasure-clip',
               skins=['normal'], size=[8, 8], viewport=[8, 8], screen_origin=[0, 22],
               pivot=[0, 0], start=2., duration=1., frames=2,
               duration_ms=1000, frame_ms=500, clip='shine_fade')
    return profile, row


class OriginalRooms(unittest.TestCase):
    def test_room_catalog_appends_after_every_existing_enemy_and_map_clip(self):
        plan = room_plan()
        self.assertEqual(len(plan['images']), 17)
        self.assertEqual(len(plan['clips']), 6)
        start = BASE_CLIPS + len(ENEMY_CLIP_ROWS) + len(MAP_CLIP_ROWS)
        self.assertEqual(start, 1641)
        self.assertEqual([row['id'] for row in ROOM_CLIP_ROWS], list(range(start, start + 6)))
        self.assertEqual(sum(row['frames'] for row in plan['clips']), 99)
        for row in plan['clips']:
            self.assertFalse(row['loop'])
            self.assertEqual(row['size'], [320, 218])
            self.assertEqual(row['pivot'], [0, 0])

    def test_rest_uses_real_parent_and_zero_size_background_anchors(self):
        room = '''[gd_scene format=3]
[node name="Room" type="Control"]
[node name="BgContainer" type="Control" parent="."]
anchor_left = 0.5
anchor_top = 0.5
anchor_right = 0.5
anchor_bottom = 0.5
offset_left = -18.0
offset_top = -7.0
offset_right = -17.5
offset_bottom = -6.5
[node name="Character" type="ColorRect" parent="BgContainer"]
offset_right = 40.0
offset_bottom = 20.0
color = Color(0, 1, 0, 1)
'''
        background = '''[gd_scene format=3]
[node name="Background" type="Control"]
[node name="Base" type="ColorRect" parent="."]
anchor_left = 0.5
anchor_top = 0.5
anchor_right = 0.5
anchor_bottom = 0.5
offset_right = 4.0
offset_bottom = 3.0
color = Color(0, 0, 1, 1)
[node name="RestSiteLighting" type="Control" parent="."]
[node name="Glow" type="ColorRect" parent="RestSiteLighting"]
offset_right = 2.0
offset_bottom = 1.0
color = Color(1, 0, 0, 1)
'''
        profile = deepcopy(room_profile())
        profile['rest'].update(source_scene='room.tscn', scenes=[dict(path='bg.tscn', root='Background')])
        profile['rest']['parent'].update(evaluated_origin=[2., 3.], evaluated_size=[.5, .5])
        row = dict(path='bg.tscn', size=[40, 20], viewport=[40, 20], dark=False, screen_origin=[0, 22])
        pack = Pack({'room.tscn': room, 'bg.tscn': background})
        with patch('world_room_assets.room_profile', return_value=profile):
            light, metadata = render_rest(pack, row, texture_loader=no_texture)
            dark, dark_metadata = render_rest(pack, dict(row, dark=True), texture_loader=no_texture)
        self.assertEqual(metadata['room_parent_origin'], [2., 3.])
        self.assertEqual(metadata['background_size'], [0., 0.])
        self.assertEqual(light.getpixel((2, 3)), (255, 0, 0, 255))
        self.assertEqual(dark.getpixel((2, 3)), (0, 0, 255, 255))
        self.assertEqual(light.getbbox(), (2, 3, 6, 6))
        self.assertTrue(dark_metadata['dark'])
        self.assertFalse(any(row['node'].endswith('/Character') for row in metadata['drawn']))

    def test_shine_replays_the_opening_on_the_same_skeleton(self):
        profile, row = treasure_fixture()
        sampler = Sampler()
        with patch('world_room_assets.room_profile', return_value=profile):
            images, metadata = sample_treasure(None, row, sampler)
        self.assertEqual(sampler.request['clip'], 'animation')
        self.assertEqual(sampler.request['times'], [2., 2.5])
        self.assertEqual(sampler.request['tracks'],
            [dict(track=0, clip='shine_fade', queued=True, loop=False, delay=0)])
        self.assertFalse(sampler.request['loop'])
        self.assertEqual(metadata['bounds'], [[1., 2.], [3., 4.]])
        self.assertEqual(metadata['pivot'], [0, 0])
        self.assertEqual(len(images), 2)
        self.assertEqual(images[0].getpixel((0, 0)), (0, 0, 0, 255))
        self.assertEqual(images[0].getpixel((1, 2)), (255, 0, 0, 255))

    def test_final_open_pose_preserves_queue_and_outline_registration(self):
        profile, row = treasure_fixture()
        with patch('world_room_assets.room_profile', return_value=profile):
            sampler = Sampler()
            _, opened = sample_treasure(None, dict(row, kind='treasure-still', time=3.), sampler)
            self.assertEqual(sampler.request['times'], [3.])
            _, outlined = sample_treasure(None, dict(row, kind='treasure-still', time=0., skins=['outline']), sampler)
        self.assertEqual(opened['transform'], outlined['transform'])
        self.assertEqual(opened['bounds'], outlined['bounds'])
        self.assertEqual(sampler.request['skins'], ['outline'])

    def test_original_clip_duration_or_events_cannot_silently_drift(self):
        profile, row = treasure_fixture()
        profile['treasure']['animation_queue'][0]['source_duration_seconds'] = 2.25
        with patch('world_room_assets.room_profile', return_value=profile):
            with self.assertRaisesRegex(ValueError, 'duration drift'):
                sample_treasure(None, row, Sampler())
        profile['treasure']['animation_queue'][0]['source_duration_seconds'] = 2.
        profile['treasure']['animation_queue'][1]['source_events'] = [{'name': 'unexpected', 'time': .5}]
        with patch('world_room_assets.room_profile', return_value=profile):
            with self.assertRaisesRegex(ValueError, 'event timeline drift'):
                sample_treasure(None, row, Sampler())


if __name__ == '__main__':
    unittest.main()
