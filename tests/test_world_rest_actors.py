"""Asset-free rest actor placement, track dispatch and blend-plane regressions."""
from copy import deepcopy
from pathlib import Path
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

import numpy as np
from PIL import Image

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
from world_rest_actors import ACT_CLIPS, REST_ACTORS, ROOT_SCALE, rest_actor_plan
from world_rest_actor_assets import (DARK_GRAY_MODULATE, _ActorRenderer, _actor_scene, _plane_image,
                                      sample_rest_actor)
from world_room_profile import room_profile
from world_room_shaders import RoomRenderer
from world_scene_assets import SceneRenderError, _property, _vector


class Pack:
    def __init__(self, documents):
        self.documents = documents
        self.entries = dict.fromkeys(documents)

    def read(self, path):
        return self.documents[path.removeprefix('res://')].encode()


def no_texture(pack, path):
    raise AssertionError('This synthetic rest scene has no texture resources')


class Sampler:
    def __init__(self, recipe=REST_ACTORS[0], layers=None):
        self.recipe, self.layers = recipe, layers
        self.bones, self.slots = [], []

    def frame_layers(self, dark):
        return self.layers if self.layers is not None else [
            ('body', [0., 0., 1., 1.] if dark else [0., 1., 0., 1.], 0)]

    def metadata(self, resource):
        return dict(source_version=self.recipe['source_version'], available_skins=['default'],
            clips=[dict(name=name, duration=self.recipe['duration'], events=[], timeline_types=[])
                   for name in ACT_CLIPS] + [dict(name='_tracks/light_off', duration=0.)])

    def sample(self, resource, clip, times, **settings):
        self.request = dict(resource=resource, clip=clip, times=times, **settings)
        dark = bool(settings['tracks'])
        draw = [dict(slot=slot) for slot, _, _ in self.frame_layers(dark)]
        return [dict(time=time, dark=dark, draw=draw, bones=self.bones, slots=self.slots,
                     draw_order=self.recipe.get('draw_order', [item['slot'] for item in draw]))
                for time in times]

    def draw(self, resource, frame):
        return [(np.array([[0., 0.], [10., 0.], [10., -10.], [0., -10.]]),
                 np.array([[0., 0.], [1., 0.], [1., 1.], [0., 1.]]),
                 np.array([[0, 1, 2], [2, 3, 0]]), np.array([[color]]), np.ones(4), blend)
                for _, color, blend in self.frame_layers(frame['dark'])]


def fixture(character='IRONCLAD'):
    profile = deepcopy(room_profile())
    profile.update(conversion_viewport=[64, 64])
    profile['output'].update(size=[64, 64])
    rest = profile['rest']
    rest['source_scene'] = 'room.tscn'
    rest['parent'].update(evaluated_origin=[2., 3.], evaluated_size=[.5, .5])
    rest['singleplayer_actor'].update(evaluated_origin=[10., 12.])
    rest['singleplayer_actor']['scenes'][character] = 'actor.tscn'
    room = '''[gd_scene format=3]
[node name="Room" type="Control"]
[node name="BgContainer" type="Control" parent="."]
offset_left = 2
offset_top = 3
offset_right = 2.5
offset_bottom = 3.5
[node name="Character_1" type="Control" parent="BgContainer"]
offset_left = 8
offset_top = 9
offset_right = 8
offset_bottom = 9
scale = Vector2(0.5, 0.5)
'''
    actors = [recipe for recipe in REST_ACTORS if recipe['scene_character'] == character]
    external = ''.join(f'[ext_resource type="SpineSkeletonDataResource" path="res://{actor["resource"]}" id="s{i}"]\n'
                       for i, actor in enumerate(actors))
    materials = ''.join(f'[sub_resource type="CanvasItemMaterial" id="m{i}"]\n'
                        for i, actor in enumerate(actors) if actor.get('material_overrides'))
    actor = '[gd_scene format=3]\n' + external + materials + f'''[node name="Actor" type="Node2D"]
position = Vector2({actors[0]['root_position'][0]}, {actors[0]['root_position'][1]})
scale = Vector2(0.760006, 0.760006)
'''
    for i, recipe in enumerate(actors):
        actor += f'''[node name="{recipe['node']}" type="SpineSprite" parent="."]
position = Vector2({recipe['position'][0]}, {recipe['position'][1]})
scale = Vector2({recipe['scale'][0]}, {recipe['scale'][1]})
skeleton_data_res = ExtResource("s{i}")
'''
        if recipe.get('material_overrides'):
            actor += f'multiply_material = SubResource("m{i}")\n'
    actor += '''[node name="ControlRoot" type="ColorRect" parent="."]
offset_right = 1000
offset_bottom = 1000
color = Color(1, 0, 0, 1)
'''
    documents = {'room.tscn': room, 'actor.tscn': actor}
    documents.update({actor['resource']: '[gd_resource type="SpineSkeletonDataResource" format=3]\n[resource]\n'
                      for actor in actors})
    row = next(row for row in rest_actor_plan(profile)['clips'] if row['actor'] == character)
    row.update(frames=2, frame_ms=round(row['duration_ms'] / 2))
    return profile, row, Pack(documents)


def center_actor(profile, pack, recipe):
    """Put synthetic geometry in view without changing any reviewed actor transform."""
    origin = np.array([20., 20.]) - .5 * ROOT_SCALE[0] * np.asarray(recipe['position'])
    profile['rest']['singleplayer_actor']['evaluated_origin'] = origin.tolist()
    x, y = origin - profile['rest']['parent']['evaluated_origin']
    for old, new in [('offset_left = 8', f'offset_left = {x}'),
                     ('offset_right = 8', f'offset_right = {x}'),
                     ('offset_top = 9', f'offset_top = {y}'),
                     ('offset_bottom = 9', f'offset_bottom = {y}')]:
        pack.documents['room.tscn'] = pack.documents['room.tscn'].replace(old, new)


def composite(background, rgba, blend):
    src = np.asarray(rgba, float) / 255
    alpha = src[3]
    if blend == 1:
        return np.minimum(1., background + src[:3] * alpha)
    if blend == 2:
        return background * (1 - alpha + src[:3] * alpha)
    return src[:3] * alpha + background * (1 - alpha)


class OriginalRestActors(unittest.TestCase):
    def test_plan_is_lightweight_and_covers_whole_independent_periods(self):
        code = ("import sys; sys.path.insert(0,sys.argv[1]); import world_rest_actors; "
                "world_rest_actors.rest_actor_plan(); "
                "assert not {'numpy','PIL','pck','build_content','world_enemy_assets'} & set(sys.modules)")
        subprocess.run([sys.executable, '-c', code, str(TOOLS)], check=True)
        jobs = rest_actor_plan()['clips']
        self.assertEqual((len(jobs), sum(job['frames'] for job in jobs)), (48, 8340))
        self.assertEqual(len({job['key'] for job in jobs}), 48)
        self.assertTrue(all(job['size'] == [320, 218] and job['pivot'] == [0, 0] and
                            job['viewport'] == [1920, 1080] and job['screen_origin'] == [0, 22]
                            for job in jobs))
        necro = next(job for job in jobs if job['actor'] == 'NECROBINDER')
        osty = next(job for job in jobs if job['actor'] == 'OSTY')
        self.assertEqual((necro['duration_ms'], necro['frames']), (21333, 256))
        self.assertEqual((osty['duration_ms'], osty['frames']), (10000, 200))
        self.assertGreater(necro['frames'] * necro['frame_ms'], 21000)
        self.assertEqual([row['key'] for row in REST_ACTORS if row['character'] == 4], ['NECROBINDER', 'OSTY'])

    def test_actor_selection_is_by_act_not_first_act_region(self):
        profile = room_profile()
        self.assertIn('rest_background.UNDERDOCKS', [row['key'] for row in profile['rest']['scenes']])
        jobs = rest_actor_plan(profile)['clips']
        self.assertEqual({job['clip'] for job in jobs if job['act_index'] == 0}, {'overgrowth_loop'})
        self.assertEqual({job['clip'] for job in jobs if job['act_index'] == 1}, {'hive_loop'})
        self.assertEqual({job['clip'] for job in jobs if job['act_index'] == 2}, {'glory_loop'})

    def test_scene_zeroes_authored_root_and_excludes_ui_then_replays_dark_track_one(self):
        profile, row, pack = fixture()
        sampler = Sampler()
        with patch('world_rest_actor_assets.room_profile', return_value=profile):
            lit, metadata = sample_rest_actor(pack, row, sampler, texture_loader=no_texture)
            self.assertEqual(sampler.request['times'], [0., 6.666667 / 2])
            self.assertEqual(sampler.request['tracks'], [])
            dark, dark_metadata = sample_rest_actor(pack, dict(row, dark=True), sampler, texture_loader=no_texture)
        self.assertEqual(sampler.request['tracks'], [dict(track=1, clip='_tracks/light_off', loop=True)])
        self.assertTrue(sampler.request['loop'])
        self.assertEqual(lit[0].getbbox(), (10, 12, 14, 16))
        self.assertEqual(lit[0].getpixel((10, 12)), (0, 255, 0, 255))
        self.assertEqual(dark[0].getpixel((10, 12)), (0, 0, 169, 255))
        self.assertEqual(metadata['diagnostics']['actor_root_position'], [0., 0.])
        self.assertEqual(metadata['diagnostics']['actor_root_scale'], [.760006, .760006])
        self.assertEqual(metadata['diagnostics']['singleplayer_origin'], [10., 12.])
        self.assertEqual(dark_metadata['shader_time'], 0.)
        self.assertEqual(metadata['diagnostics']['singleplayer_modulate'], [1., 1., 1., 1.])
        self.assertEqual(dark_metadata['diagnostics']['singleplayer_modulate'], list(DARK_GRAY_MODULATE))

    def test_dark_gray_is_the_exact_godot_byte_to_binary32_color(self):
        self.assertEqual([struct.unpack('<I', struct.pack('<f', channel))[0]
                          for channel in DARK_GRAY_MODULATE], [0x3f29a9aa] * 3 + [0x3f800000])
        self.assertEqual(DARK_GRAY_MODULATE[0], 0.6627451181411743)

    def test_all_six_actor_material_chains_inherit_rgb_without_changing_alpha(self):
        for recipe in REST_ACTORS:
            with self.subTest(actor=recipe['key']):
                profile, _, pack = fixture(recipe['scene_character'])
                center_actor(profile, pack, recipe)
                row = next(job for job in rest_actor_plan(profile)['clips'] if job['actor'] == recipe['key'])
                row.update(frames=1, frame_ms=row['duration_ms'])
                slot = 'orb' if recipe['key'] == 'DEFECT' else 'throne shadow' if recipe['key'] == 'REGENT' else 'body'
                blend = 2 if recipe.get('material_overrides') else 0
                sampler = Sampler(recipe, [(slot, [1., 1., 1., 127/255], blend)])
                with patch('world_rest_actor_assets.room_profile', return_value=profile):
                    lit, _ = sample_rest_actor(pack, row, sampler, texture_loader=no_texture)
                    dark, info = sample_rest_actor(pack, dict(row, dark=True), sampler, texture_loader=no_texture)
                self.assertEqual(lit[0].getpixel((22, 21)), (255, 255, 255, 127))
                self.assertEqual(dark[0].getpixel((22, 21)), (169, 169, 169, 127))
                self.assertEqual(dark[0].getpixel((0, 0)), (0, 0, 0, 0))
                self.assertEqual(lit[0].getbbox(), dark[0].getbbox())
                self.assertEqual(info['diagnostics']['singleplayer_container'], 'Room/BgContainer/Character_1')
                self.assertEqual(info['diagnostics']['spines'][0]['effective_slot_blends'], {slot: 0})

    def test_authored_parent_modulate_drift_is_rejected_for_both_lighting_states(self):
        for tint in ('Color(0.9999999, 1, 1, 1)', 'Color(1, 1, 1, .5)'):
            for dark in (False, True):
                with self.subTest(tint=tint, dark=dark):
                    profile, row, pack = fixture()
                    pack.documents['room.tscn'] += 'modulate = ' + tint + '\n'
                    renderer = _ActorRenderer(pack, 64, 64, (64, 64), no_texture, None, 0., 1)
                    with self.assertRaisesRegex(SceneRenderError, 'authored modulate drift'):
                        _actor_scene(renderer, REST_ACTORS[0], dict(row, dark=dark), profile)

    def test_defect_tints_each_emission_before_additive_plane_saturation(self):
        recipe = REST_ACTORS[2]
        profile, _, pack = fixture('DEFECT')
        center_actor(profile, pack, recipe)
        row = next(job for job in rest_actor_plan(profile)['clips']
                   if job['actor'] == 'DEFECT' and job['dark'] and job['plane'] == 1)
        row.update(frames=1, frame_ms=row['duration_ms'])
        for count, expected in ((1, 169), (2, 255)):
            with self.subTest(emitting_slots=count):
                sampler = Sampler(recipe, [(slot, [1., 0., 0., 1.], 1)
                                           for slot in ('glow4', 'glow3')[:count]])
                with patch('world_rest_actor_assets.room_profile', return_value=profile):
                    images, metadata = sample_rest_actor(pack, row, sampler, texture_loader=no_texture)
                self.assertEqual(images[0].getpixel((22, 21)), (expected, 0, 0, 255))
                self.assertEqual(metadata['diagnostics']['ordered_plane_blends'], [0, 1, 0])
                self.assertEqual(metadata['diagnostics']['plane_draw_commands'], count)
        # A naive post-darken of the saturated two-slot plane would give 169.
        self.assertNotEqual(round(255 * DARK_GRAY_MODULATE[0]), expected)

    def test_necro_and_osty_attached_fire_replace_color_after_parent_tint(self):
        shader = 'shaders/vfx/vfx_stepped_shader_fire_dark.tres'
        for recipe in REST_ACTORS[4:]:
            with self.subTest(actor=recipe['key']):
                profile, _, pack = fixture('NECROBINDER')
                center_actor(profile, pack, recipe)
                row = next(job for job in rest_actor_plan(profile)['clips'] if job['actor'] == recipe['key'])
                row.update(frames=1, frame_ms=row['duration_ms'])
                pack.documents[shader] = '[gd_resource type="VisualShader" format=3]\n[resource]\ncode = "synthetic shader fixture"\n'
                declarations = f'''[ext_resource type="Texture2D" path="res://fire.png" id="fire_texture"]
[ext_resource type="Shader" path="res://{shader}" id="fire_shader"]
[sub_resource type="ShaderMaterial" id="fire_material"]
shader = ExtResource("fire_shader")
'''
                pack.documents['actor.tscn'] = pack.documents['actor.tscn'].replace('[node name="Actor"', declarations + '[node name="Actor"')
                follower = 'SpineBoneNode' if recipe['key'] == 'NECROBINDER' else 'SpineSlotNode'
                reference = 'bone_name = "flame"' if recipe['key'] == 'NECROBINDER' else 'slot_name = "body"'
                pack.documents['actor.tscn'] += f'''[node name="Follower" type="{follower}" parent="{recipe['node']}"]
{reference}
[node name="Fire" type="Sprite2D" parent="{recipe['node']}/Follower"]
texture = ExtResource("fire_texture")
material = SubResource("fire_material")
centered = false
'''
                blend = 2 if recipe.get('material_overrides') else 0
                sampler = Sampler(recipe, [('body', [1., 1., 1., 127/255], blend)])
                sampler.bones = [dict(name='flame', node_matrix=[1., 0., 30., 0., 1., 0.])]
                sampler.slots = [dict(slot='body', node_matrix=[1., 0., 30., 0., 1., 0.])]
                texture = lambda pack, path: Image.new('RGBA', (10, 10), (0, 0, 255, 255))
                with patch('world_rest_actor_assets.room_profile', return_value=profile), \
                        patch.object(RoomRenderer, '_verify_shader'), \
                        patch.object(RoomRenderer, '_shader_texture', return_value=np.ones((1, 1, 4), np.float32)):
                    lit, _ = sample_rest_actor(pack, row, sampler, texture_loader=texture)
                    dark, info = sample_rest_actor(pack, dict(row, dark=True), sampler, texture_loader=texture)
                self.assertEqual(lit[0].getpixel((22, 21)), (255, 255, 255, 127))
                self.assertEqual(dark[0].getpixel((22, 21)), (169, 169, 169, 127))
                fire_x = int(np.ceil(20 + .5 * ROOT_SCALE[0] * recipe['scale'][0] * 30)) + 1
                self.assertEqual(lit[0].getpixel((fire_x, 21)), (255, 0, 0, 255))
                self.assertEqual(dark[0].getpixel((fire_x, 21)), lit[0].getpixel((fire_x, 21)))
                self.assertEqual(info['diagnostics']['room_shaders'][0]['kind'], 'dark_fire')
                self.assertEqual(info['diagnostics']['room_shaders'][0]['sample_time'], 0.)

    def test_dark_gray_renderer_change_invalidates_actor_cache_without_discarding_enemy_or_room_clips(self):
        from build_world import ConversionCache, render_scope, source_fingerprint
        from world_enemy_assets import manifest
        source = source_fingerprint()
        previous = dict(source, **{'tools/world_rest_actor_assets.py': 'previous renderer without parent tint'})
        for model in manifest()['models']:
            for kind in ('bounds', 'clip-reference', 'clip'):
                with self.subTest(model=model['key'], kind=kind):
                    self.assertEqual(render_scope(kind, source, model=model), render_scope(kind, previous, model=model))
        for kind in ('texture', 'scene', 'atlas-tres', 'ui-component', 'map-clip', 'room-image', 'room-clip'):
            self.assertEqual(render_scope(kind, source), render_scope(kind, previous))
        current_scope, old_scope = [render_scope('rest-actor-clip', version) for version in (source, previous)]
        self.assertNotEqual(current_scope, old_scope)
        jobs = rest_actor_plan()['clips']
        self.assertEqual((len(jobs), sum(job['frames'] for job in jobs)), (48, 8340))
        self.assertEqual((sum(job['dark'] for job in jobs), sum(job['frames'] for job in jobs if job['dark'])), (24, 4170))
        with TemporaryDirectory() as directory:
            cache = ConversionCache(Path(directory), {'schema': 2, 'input': 'synthetic'})
            settings, metadata, payload = jobs[1], {'frames': jobs[1]['frames']}, b'old actor pixels'
            cache.put('clip', settings, metadata, payload, scope=old_scope)
            self.assertEqual(cache.get('clip', settings, scope=old_scope), (metadata, payload))
            self.assertIsNone(cache.get('clip', settings, scope=current_scope))

    def test_necrobinder_keeps_authored_osty_position_and_separate_child(self):
        profile, row, pack = fixture('NECROBINDER')
        recipe = next(actor for actor in REST_ACTORS if actor['key'] == 'OSTY')
        osty = next(job for job in rest_actor_plan(profile)['clips'] if job['actor'] == 'OSTY')
        renderer = _ActorRenderer(pack, 64, 64, (64, 64), no_texture, None, 0., 1)
        _, parent = _actor_scene(renderer, recipe, osty, profile)
        root = parent.children[0].children[0]
        self.assertEqual([child.name for child in root.children], ['Osty'])
        self.assertEqual(_vector(_property(root.children[0].props, 'position')).tolist(), [-227.63, 352.629])
        self.assertEqual(renderer.diagnostics['companion_condition'], 'original_singleplayer_scene_always_present')

    def test_changed_authored_root_or_companion_order_is_rejected(self):
        profile, row, pack = fixture()
        pack.documents['actor.tscn'] = pack.documents['actor.tscn'].replace('Vector2(0.760006, 0.760006)', 'Vector2(1, 1)')
        with patch('world_rest_actor_assets.room_profile', return_value=profile):
            with self.assertRaisesRegex(SceneRenderError, 'root scale'):
                sample_rest_actor(pack, row, Sampler(), texture_loader=no_texture)
        profile, row, pack = fixture('NECROBINDER')
        pack.documents['actor.tscn'] = pack.documents['actor.tscn'].replace('name="Osty"', 'name="WrongCompanion"')
        renderer = _ActorRenderer(pack, 64, 64, (64, 64), no_texture, None, 0., 1)
        with self.assertRaisesRegex(SceneRenderError, 'companion order'):
            _actor_scene(renderer, REST_ACTORS[4], row, profile)

    def test_additive_emission_plane_matches_direct_draws_over_different_backgrounds(self):
        layers = [Image.new('RGBA', (1, 1), color) for color in [(20, 200, 50, 80), (140, 80, 180, 160)]]
        encoded = _plane_image([(0, image, 1, ()) for image in layers], [1, 1], 1).getpixel((0, 0))
        self.assertEqual(encoded[3], 255)
        for background in ([0., 0., 0.], [.2, .4, .1], [.9, .8, .95]):
            expected = np.asarray(background)
            for layer in layers:
                expected = composite(expected, layer.getpixel((0, 0)), 1)
            actual = composite(np.asarray(background), encoded, 1)
            np.testing.assert_allclose(actual, expected, atol=1/255)
        empty = _plane_image([], [1, 1], 1)
        self.assertEqual(empty.getpixel((0, 0)), (0, 0, 0, 0))

    def test_unreviewed_multiply_is_rejected_and_normal_groups_keep_transparency(self):
        shadow = Image.new('RGBA', (1, 1), (60, 80, 100, 127))
        with self.assertRaisesRegex(SceneRenderError, 'Unreviewed rest actor blend'):
            _plane_image([(0, shadow, 2, ())], [1, 1], 2)
        layers = [Image.new('RGBA', (1, 1), color) for color in [(20, 200, 50, 80), (140, 80, 180, 160)]]
        normal = _plane_image([(0, image, 0, ()) for image in layers], [1, 1], 0).getpixel((0, 0))
        self.assertLess(normal[3], 255)
        for background in ([0., 0., 0.], [.2, .4, .1], [.9, .8, .95]):
            expected = np.asarray(background)
            for layer in layers:
                expected = composite(expected, layer.getpixel((0, 0)), 0)
            np.testing.assert_allclose(composite(np.asarray(background), normal, 0), expected, atol=1/255)

    def test_regent_material_override_replaces_the_source_multiply_slot(self):
        profile, row, pack = fixture('REGENT')
        recipe = REST_ACTORS[3]
        renderer = _ActorRenderer(pack, 64, 64, (64, 64), no_texture, None, 0., 1)
        _actor_scene(renderer, recipe, row, profile)
        self.assertEqual(renderer.diagnostics['material_overrides'],
            [dict(source_slot_blend=2, effective_blend=0,
                  material='authored_empty_CanvasItemMaterial_default_MIX')])
        renderer.actor_recipe, renderer.actor_row, renderer.command_planes = recipe, row, {}
        shadow = Image.new('RGBA', (1, 1), (60, 80, 100, 127))
        def base(instance, *args):
            instance.diagnostics['spines'].append({})
            return {}, {'throne shadow': [(0, shadow, 2, ())]}, recipe['draw_order']
        with patch.object(RoomRenderer, 'spine', base):
            _, commands, _ = renderer.spine(None, '', None, None, (), 0)
        self.assertEqual(commands['throne shadow'][0][2], 0)
        self.assertEqual(renderer.diagnostics['spines'][-1]['source_slot_blends'], {'throne shadow': 2})
        self.assertEqual(renderer.diagnostics['spines'][-1]['effective_slot_blends'], {'throne shadow': 0})
        pack.documents['actor.tscn'] = pack.documents['actor.tscn'].replace('multiply_material = SubResource("m0")', '')
        renderer = _ActorRenderer(pack, 64, 64, (64, 64), no_texture, None, 0., 1)
        with self.assertRaisesRegex(SceneRenderError, 'material override drift'):
            _actor_scene(renderer, recipe, row, profile)

    def test_missing_dark_glows_do_not_merge_the_front_and_back_planes(self):
        recipe = REST_ACTORS[2]
        renderer = _ActorRenderer(None, 1, 1, (1, 1), no_texture, None, 0., 1)
        renderer.actor_recipe, renderer.actor_row = recipe, dict(clip='hive_loop', dark=True)
        renderer.command_planes = {}
        back, front = Image.new('RGBA', (1, 1), 'red'), Image.new('RGBA', (1, 1), 'green')
        def base(instance, *args):
            instance.diagnostics['spines'].append({})
            return {}, {'orb': [(0, back, 0, ())], 'head': [(0, front, 0, ())]}, recipe['draw_order']
        with patch.object(RoomRenderer, 'spine', base):
            renderer.spine(None, '', None, None, (), 0)
        self.assertEqual(renderer.command_planes[id(back)], 0)
        self.assertEqual(renderer.command_planes[id(front)], 2)
        changed = deepcopy(recipe)
        changed['draw_order'][9], changed['draw_order'][10] = changed['draw_order'][10], changed['draw_order'][9]
        renderer.actor_recipe = changed
        with patch.object(RoomRenderer, 'spine', base):
            with self.assertRaisesRegex(SceneRenderError, 'draw order drift'):
                renderer.spine(None, '', None, None, (), 0)


if __name__ == '__main__':
    unittest.main()
