"""Asset-free checks of scene geometry and runtime layer construction."""
import sys
import hashlib
from unittest.mock import patch
import unittest
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import world_scene_assets
from world_scene_assets import SceneLoader, SceneRenderError, background_layers, load_scene_texture, parse_value, render_scene


class Pack:
    def __init__(self, scenes, images=None):
        self.scenes = scenes
        self.images = images or {}
        self.entries = {name: None for name in scenes}

    def read(self, path):
        return self.scenes[path.removeprefix('res://')].encode()


def texture(pack, path):
    return pack.images[path]


def render(pack, path='main.tscn', size=(16, 16), **kwargs):
    return render_scene(pack, path, *size, viewport=size, texture_loader=texture, **kwargs)


class SceneGeometry(unittest.TestCase):
    def test_full_path_atlas_resource_preserves_transparent_margin(self):
        resource = '''[gd_resource type="AtlasTexture" format=3]
[ext_resource type="Texture2D" path="res://atlas.png" id="a"]
[resource]
atlas = ExtResource("a")
region = Rect2(2, 1, 2, 3)
margin = Rect2(1, 2, 3, 4)
'''
        p = Pack({'compressed.sprites/icon.tres': resource},
                 {'atlas.png': Image.new('RGBA', (8, 8), 'blue')})
        image = load_scene_texture(p, 'res://compressed.sprites/icon.tres', texture_loader=texture)
        self.assertEqual(image.size, (5, 7))
        self.assertEqual(image.getbbox(), (1, 2, 3, 5))
        self.assertEqual(image.getpixel((2, 4)), (0, 0, 255, 255))

    def test_inline_shader_scope_multiline_and_explicit_static_policy(self):
        # This synthetic shader is deliberately unrelated to the private game
        # source. Its assignment and header-looking line must remain one value.
        code = 'shader_type canvas_item;\n[not_a_resource]\nvoid fragment() {\n    COLOR = vec4(1.0);\n}\n'
        scene = '''[gd_scene format=3]
[ext_resource type="Texture2D" path="res://red.png" id="red"]
[sub_resource type="Shader" id="Inline"]
code = "''' + code + '''"
[sub_resource type="ShaderMaterial" id="Material"]
shader = SubResource("Inline")
shader_parameter/speed = 0.388
shader_parameter/strength = 0.058
[node name="Root" type="Node2D"]
[node name="Backdrop" type="Sprite2D" parent="."]
centered = false
texture = ExtResource("red")
[node name="Effect" type="Sprite2D" parent="."]
position = Vector2(3, 0)
centered = false
texture = ExtResource("red")
material = SubResource("Material")
'''
        p = Pack({'main.tscn': scene}, {'red.png': Image.new('RGBA', (2, 2), 'red')})
        doc = SceneLoader(p).document('main.tscn')
        self.assertEqual(list(doc.internal['Inline'][1]), ['code'])
        self.assertEqual(parse_value(doc.internal['Inline'][1]['code'].text), code)
        digest = hashlib.sha256(code.encode()).hexdigest()
        image, diagnostics = render(p)
        self.assertEqual(image.getbbox(), (0, 0, 2, 2))
        omitted = next(x for x in diagnostics['omitted'] if x.get('reason') == 'dynamic_shader')
        self.assertEqual(omitted['shader'], 'main.tscn::Inline')
        self.assertEqual(omitted['shader_sha256'], digest)
        with patch.dict(world_scene_assets._STATIC_INLINE_SHADERS,
                        {digest: 'synthetic_explicit_static_policy'}):
            image, diagnostics = render(p, sample_time=1.25)
        self.assertEqual(image.getbbox(), (0, 0, 5, 2))
        policy = diagnostics['approximations'][0]
        self.assertEqual(policy['reason'], 'synthetic_explicit_static_policy')
        self.assertEqual(policy['shader_sha256'], digest)
        self.assertEqual((policy['speed'], policy['strength'], policy['sample_time']), (.388, .058, 1.25))

    def test_nested_rotation_scale_and_sprite_origin(self):
        strip = Image.new('RGBA', (2, 1))
        strip.putdata([(255, 0, 0, 255), (0, 0, 255, 255)])
        scene = '''[gd_scene format=3]
[ext_resource type="Texture2D" path="res://strip.png" id="t"]
[node name="Root" type="Node2D"]
position = Vector2(2, 3)
scale = Vector2(2, 2)
[node name="Turn" type="Node2D" parent="."]
rotation = 1.5707963267948966
[node name="Strip" type="Sprite2D" parent="Turn"]
position = Vector2(1, 0)
centered = false
texture = ExtResource("t")
'''
        image, _ = render(Pack({'main.tscn': scene}, {'strip.png': strip}), size=(8, 12))
        self.assertEqual(image.getbbox(), (0, 5, 2, 9))
        self.assertGreater(image.getpixel((1, 5))[0], image.getpixel((1, 5))[2])
        self.assertGreater(image.getpixel((1, 8))[2], image.getpixel((1, 8))[0])

    def test_instance_child_override_retains_resource_scope_and_anchor_size(self):
        child = '''[gd_scene format=3]
[ext_resource type="Texture2D" path="res://red.png" id="same_id"]
[node name="Child" type="Control"]
anchor_right = 1.0
anchor_bottom = 1.0
[node name="Center" type="TextureRect" parent="."]
anchor_left = 0.5
anchor_top = 0.5
anchor_right = 0.5
anchor_bottom = 0.5
offset_left = -1
offset_top = -1
offset_right = 1
offset_bottom = 1
texture = ExtResource("same_id")
expand_mode = 1
'''
        main = '''[gd_scene format=3]
[ext_resource type="PackedScene" path="res://child.tscn" id="child"]
[ext_resource type="Texture2D" path="res://green.png" id="same_id"]
[node name="Root" type="Control"]
anchor_right = 1.0
anchor_bottom = 1.0
[node name="Instance" parent="." instance=ExtResource("child")]
position = Vector2(1, 0)
[node name="Center" parent="Instance"]
texture = ExtResource("same_id")
'''
        p = Pack({'main.tscn': main, 'child.tscn': child},
                 {'red.png': Image.new('RGBA', (2, 2), 'red'),
                  'green.png': Image.new('RGBA', (2, 2), (0, 255, 0, 255))})
        image, _ = render(p, size=(8, 8))
        self.assertEqual(image.getbbox(), (4, 3, 6, 5))
        self.assertEqual(image.getpixel((4, 3)), (0, 255, 0, 255))

    def test_transform2d_clip_mask_and_show_behind(self):
        scene = '''[gd_scene format=3]
[ext_resource type="Texture2D" path="res://mask.png" id="mask"]
[ext_resource type="Texture2D" path="res://blue.png" id="blue"]
[node name="Root" type="Node2D"]
transform = Transform2D(1, 0, 0, 1, 4, 3)
[node name="Mask" type="Sprite2D" parent="."]
centered = false
texture = ExtResource("mask")
clip_children = 1
[node name="Fill" type="Sprite2D" parent="Mask"]
centered = false
texture = ExtResource("blue")
show_behind_parent = true
'''
        mask = Image.new('RGBA', (4, 4))
        mask.paste((255, 255, 255, 255), (1, 1, 3, 3))
        p = Pack({'main.tscn': scene}, {'mask.png': mask,
                 'blue.png': Image.new('RGBA', (4, 4), 'blue')})
        image, _ = render(p)
        self.assertEqual(image.getbbox(), (5, 4, 7, 6))
        self.assertEqual(image.getpixel((5, 4)), (0, 0, 255, 255))
        self.assertEqual(image.getpixel((4, 3))[3], 0)

    def test_runtime_layer_selection_ignores_editor_hint_and_instantiates_hidden_root(self):
        prefix = 'scenes/backgrounds/example/'
        root = '''[gd_scene format=3]
[node name="Root" type="Control"]
[node name="Layer_00" type="Control" parent="."]
VisibleLayer = 2
'''
        def layer(color):
            return '''[gd_scene format=3]
[ext_resource type="Texture2D" path="res://%s.png" id="t"]
[node name="A" type="TextureRect"]
visible = false
offset_left = -4
offset_top = -4
offset_right = 4
offset_bottom = 4
texture = ExtResource("t")
expand_mode = 1
''' % color
        p = Pack({prefix + 'example_background.tscn': root,
                  prefix + 'layers/example_bg_00_b.tscn': layer('blue'),
                  prefix + 'layers/example_bg_00_a.tscn': layer('red')},
                 {'red.png': Image.new('RGBA', (8, 8), 'red'),
                  'blue.png': Image.new('RGBA', (8, 8), 'blue')})
        image, meta = render(p, prefix + 'example_background.tscn', (8, 8),
                             mode='background', camera=(1, -23, 0))
        self.assertEqual(image.getpixel((2, 2)), (255, 0, 0, 255))
        self.assertTrue(meta['layers'][0]['path'].endswith('_a.tscn'))
        image, _ = render(p, prefix + 'example_background.tscn', (8, 8),
                          mode='background', camera=(1, -23, 0), variants={'00': 1})
        self.assertEqual(image.getpixel((2, 2)), (0, 0, 255, 255))
        with self.assertRaises(SceneRenderError):
            background_layers(p, prefix + 'example_background.tscn', {'00': 2})

    def test_shader_mask_is_not_drawn_as_plain_texture(self):
        scene = '''[gd_scene format=3]
[ext_resource type="Texture2D" path="res://white.png" id="t"]
[ext_resource type="Shader" path="res://screen_effect.gdshader" id="shader"]
[sub_resource type="ShaderMaterial" id="mat"]
shader = ExtResource("shader")
[node name="Root" type="Control"]
[node name="Background" type="ColorRect" parent="."]
offset_right = 8
offset_bottom = 8
color = Color(0, 0, 1, 1)
[node name="Mask" type="Sprite2D" parent="."]
centered = false
texture = ExtResource("t")
material = SubResource("mat")
'''
        image, meta = render(Pack({'main.tscn': scene},
                                 {'white.png': Image.new('RGBA', (8, 8), 'white')}), size=(8, 8))
        self.assertEqual(image.getpixel((4, 4)), (0, 0, 255, 255))
        self.assertEqual(meta['omitted'][0]['reason'], 'dynamic_shader')

    def test_global_z_and_inherited_alpha(self):
        scene = '''[gd_scene format=3]
[node name="Root" type="Control"]
[node name="High" type="ColorRect" parent="."]
offset_right = 8
offset_bottom = 8
color = Color(1, 0, 0, 1)
z_index = 2
[node name="LowParent" type="Control" parent="."]
modulate = Color(1, 1, 1, 0.5)
[node name="Front" type="ColorRect" parent="LowParent"]
offset_right = 8
offset_bottom = 8
color = Color(0, 0, 1, 1)
z_as_relative = false
z_index = 3
'''
        image, _ = render(Pack({'main.tscn': scene}), size=(8, 8))
        red, _, blue, alpha = image.getpixel((3, 3))
        self.assertEqual(alpha, 255)
        self.assertTrue(126 <= red <= 129 and 126 <= blue <= 129)

    def test_sampled_spine_bone_follows_behind_child(self):
        class Sampler:
            def metadata(self, resource):
                return {'clips': [{'name': 'idle'}], 'source_version': '4.2.fixture'}
            def sample(self, resource, clip, times):
                return [{'draw': [], 'bones': [{'name': 'tip', 'matrix': [1, 0, 5, 0, 1, -3]}]}]
            def draw(self, resource, frame):
                return []
        scene = '''[gd_scene format=3]
[ext_resource type="SpineSkeletonDataResource" path="res://fixture.skel" id="s"]
[ext_resource type="Texture2D" path="res://pixel.png" id="t"]
[node name="Root" type="Node2D"]
[node name="Spine" type="SpineSprite" parent="."]
skeleton_data_res = ExtResource("s")
position = Vector2(2, 1)
[node name="Bone" type="SpineBoneNode" parent="Spine"]
bone_name = "tip"
position = Vector2(1000, 1000)
show_behind_parent = true
[node name="Dot" type="Sprite2D" parent="Spine/Bone"]
centered = false
texture = ExtResource("t")
'''
        p = Pack({'main.tscn': scene}, {'pixel.png': Image.new('RGBA', (1, 1), 'red')})
        image, _ = render(p, sampler=Sampler())
        self.assertEqual(image.getbbox(), (7, 4, 8, 5))

    def test_godot_follow_matrix_overrides_sheared_bone_and_slot_preview(self):
        class Sampler:
            def metadata(self, resource):
                return {'clips': []}
            def sample(self, resource, clip, times):
                return [{'draw': [],
                         'bones': [{'name': 'tip', 'matrix': [1, 2, 50, 0, 1, -50],
                                    'node_matrix': [1, 0, 5, 0, 1, 3]}],
                         'slots': [{'slot': 'badge', 'matrix': [1, 2, 50, 0, 1, -50],
                                    'node_matrix': [1, 0, 8, 0, 1, 6]}]}]
            def draw(self, resource, frame):
                return []
        scene = '''[gd_scene format=3]
[ext_resource type="SpineSkeletonDataResource" path="res://fixture.skel" id="s"]
[ext_resource type="Texture2D" path="res://pixel.png" id="t"]
[node name="Root" type="Node2D"]
[node name="Spine" type="SpineSprite" parent="."]
skeleton_data_res = ExtResource("s")
position = Vector2(2, 1)
[node name="Bone" type="SpineBoneNode" parent="Spine"]
bone_name = "tip"
position = Vector2(1000, 1000)
[node name="Dot" type="Sprite2D" parent="Spine/Bone"]
centered = false
texture = ExtResource("t")
[node name="Slot" type="SpineSlotNode" parent="Spine"]
slot_name = "badge"
position = Vector2(1000, 1000)
[node name="Dot" type="Sprite2D" parent="Spine/Slot"]
centered = false
texture = ExtResource("t")
'''
        p = Pack({'main.tscn': scene}, {'pixel.png': Image.new('RGBA', (1, 1), 'red')})
        image, _ = render(p, sampler=Sampler())
        self.assertEqual(image.getbbox(), (7, 4, 11, 8))
        self.assertEqual(image.getpixel((7, 4)), (255, 0, 0, 255))
        self.assertEqual(image.getpixel((10, 7)), (255, 0, 0, 255))

    def test_drive_nodes_require_runtime_state_instead_of_preview_position(self):
        scene = '''[gd_scene format=3]
[node name="Root" type="Node2D"]
[node name="Target" type="SpineBoneNode" parent="."]
bone_mode = 1
bone_name = "aim"
position = Vector2(1000, 1000)
'''
        with self.assertRaisesRegex(SceneRenderError, 'runtime driver state: .*Target'):
            render(Pack({'main.tscn': scene}))

    def test_bone_follows_ancestor_global_transform_but_slot_requires_direct_parent(self):
        class Sampler:
            def metadata(self, resource):
                return {'clips': []}
            def sample(self, resource, clip, times):
                return [{'draw': [], 'bones': [{'name': 'tip', 'node_matrix': [1, 0, 3, 0, 1, 2]}],
                         'slots': [{'slot': 'tip', 'node_matrix': [1, 0, 10, 0, 1, 10]}]}]
            def draw(self, resource, frame):
                return []
        scene = '''[gd_scene format=3]
[ext_resource type="SpineSkeletonDataResource" path="res://fixture.skel" id="s"]
[ext_resource type="Texture2D" path="res://pixel.png" id="t"]
[node name="Root" type="SpineSprite"]
skeleton_data_res = ExtResource("s")
position = Vector2(1, 1)
[node name="Wrapper" type="Node2D" parent="."]
position = Vector2(5, 4)
[node name="Bone" type="SpineBoneNode" parent="Wrapper"]
bone_name = "tip"
position = Vector2(1000, 1000)
[node name="Dot" type="Sprite2D" parent="Wrapper/Bone"]
centered = false
texture = ExtResource("t")
[node name="Slot" type="SpineSlotNode" parent="Wrapper"]
slot_name = "tip"
position = Vector2(1, 1)
[node name="Dot" type="Sprite2D" parent="Wrapper/Slot"]
centered = false
texture = ExtResource("t")
'''
        p = Pack({'main.tscn': scene}, {'pixel.png': Image.new('RGBA', (1, 1), 'red')})
        image, _ = render(p, sampler=Sampler())
        self.assertEqual(image.getpixel((4, 3)), (255, 0, 0, 255))
        self.assertEqual(image.getpixel((7, 6)), (255, 0, 0, 255))
        self.assertEqual(image.getpixel((11, 11))[3], 0)

    def test_spine_slot_children_follow_full_draw_order_even_for_empty_inactive_slots(self):
        class Sampler:
            def metadata(self, resource):
                return {'clips': []}
            def sample(self, resource, clip, times):
                return [{'draw': [{'slot': 'front'}], 'draw_order': ['empty', 'front'],
                         'slots': [{'slot': name, 'node_matrix': [1, 0, 0, 0, 1, 0],
                                    'active': False, 'attachment': None}
                                   for name in ['empty', 'front']], 'bones': []}]
            def draw(self, resource, frame):
                return [(np.array([[0., 0.], [4., 0.], [4., -4.], [0., -4.]]),
                         np.array([[0., 0.], [1., 0.], [1., 1.], [0., 1.]]),
                         np.array([[0, 1, 2], [0, 2, 3]]),
                         np.array([[[0., 1., 0., 1.]]]), np.ones(4), 0)]
        scene = '''[gd_scene format=3]
[ext_resource type="SpineSkeletonDataResource" path="res://fixture.skel" id="s"]
[ext_resource type="Texture2D" path="res://red.png" id="red"]
[ext_resource type="Texture2D" path="res://blue.png" id="blue"]
[node name="Root" type="SpineSprite"]
skeleton_data_res = ExtResource("s")
[node name="EmptySlot" type="SpineSlotNode" parent="."]
slot_name = "empty"
[node name="Fill" type="Sprite2D" parent="EmptySlot"]
centered = false
texture = ExtResource("red")
[node name="FrontSlot" type="SpineSlotNode" parent="."]
slot_name = "front"
show_behind_parent = true
[node name="Dot" type="Sprite2D" parent="FrontSlot"]
position = Vector2(1, 1)
centered = false
texture = ExtResource("blue")
'''
        p = Pack({'main.tscn': scene}, {'red.png': Image.new('RGBA', (5, 5), 'red'),
                                       'blue.png': Image.new('RGBA', (1, 1), 'blue')})
        image, _ = render(p, sampler=Sampler(), size=(8, 8))
        self.assertEqual(image.getpixel((1, 1)), (0, 0, 255, 255))
        self.assertEqual(image.getpixel((2, 2)), (0, 255, 0, 255))
        self.assertEqual(image.getpixel((4, 4)), (255, 0, 0, 255))

    def test_spine_shared_slot_uses_fixed_mesh_index_and_explicit_child_index(self):
        class Sampler:
            def metadata(self, resource):
                return {'clips': []}
            def sample(self, resource, clip, times):
                return [{'draw': [], 'draw_order': ['empty'], 'bones': [],
                         'slots': [{'slot': 'empty', 'node_matrix': [1, 0, 0, 0, 1, 0]}]}]
            def draw(self, resource, frame):
                return []
        scene = '''[gd_scene format=3]
[ext_resource type="SpineSkeletonDataResource" path="res://fixture.skel" id="s"]
[ext_resource type="Texture2D" path="res://red.png" id="r"]
[ext_resource type="Texture2D" path="res://blue.png" id="b"]
[node name="Root" type="SpineSprite"]
skeleton_data_res = ExtResource("s")
[node name="First" type="SpineSlotNode" parent="."]
slot_name = "empty"
[node name="Dot" type="Sprite2D" parent="First"]
centered = false
texture = ExtResource("r")
[node name="Second" type="SpineSlotNode" parent="." index=0]
slot_name = "empty"
[node name="Dot" type="Sprite2D" parent="Second"]
centered = false
texture = ExtResource("b")
'''
        p = Pack({'main.tscn': scene}, {'red.png': Image.new('RGBA', (1, 1), 'red'),
                                       'blue.png': Image.new('RGBA', (1, 1), 'blue')})
        image, _ = render(p, sampler=Sampler())
        # Initial [Second, mesh, First] -> [mesh, Second, First]. The fixed
        # insertion index is captured before moving either slot follower.
        self.assertEqual(image.getpixel((0, 0)), (255, 0, 0, 255))

    def test_wind_shear_uses_quad_uv_without_moving_centered_sprite_origin(self):
        scene = '''[gd_scene format=3]
[ext_resource type="Texture2D" path="res://white.png" id="t"]
[ext_resource type="Shader" path="res://shaders/wind_sway.gdshader" id="shader"]
[sub_resource type="ShaderMaterial" id="mat"]
shader = ExtResource("shader")
shader_parameter/minStrength = 1.0
shader_parameter/maxStrength = 1.0
shader_parameter/strengthScale = 4.0
[node name="Root" type="Node2D"]
[node name="Leaf" type="Sprite2D" parent="."]
position = Vector2(8, 8)
texture = ExtResource("t")
material = SubResource("mat")
'''
        p = Pack({'main.tscn': scene}, {'white.png': Image.new('RGBA', (4, 4), 'white')})
        image, _ = render(p)
        self.assertGreater(image.getpixel((10, 6))[3], 0)
        self.assertEqual(image.getpixel((14, 6))[3], 0)

    def test_resource_expressions_do_not_execute_code(self):
        with self.assertRaises(SceneRenderError):
            parse_value('__import__("os").system("false")')
        with self.assertRaises(SceneRenderError):
            parse_value('1 + 2')

    def test_shared_background_preserves_foreground_occlusion(self):
        scene = '''[gd_scene format=3]
[ext_resource type="PackedScene" path="res://scenes/creature_visuals/kaiser_crab_boss_setup.tscn" id="shared"]
[node name="Root" type="Control"]
[node name="Back" type="ColorRect" parent="."]
offset_right = 8
offset_bottom = 8
color = Color(1, 0, 0, 1)
[node name="Shared" parent="." instance=ExtResource("shared")]
position = Vector2(3, 2)
[node name="Front" type="ColorRect" parent="."]
offset_right = 2
offset_bottom = 2
color = Color(0, 1, 0, 1)
'''
        p = Pack({'main.tscn': scene,
                  'scenes/creature_visuals/kaiser_crab_boss_setup.tscn':
                  '[gd_scene format=3]\n[node name="Actor" type="Node2D"]\n'})
        back, meta = render(p, size=(8, 8), plane='back')
        front, _ = render(p, size=(8, 8), plane='front')
        full, _ = render(p, size=(8, 8))
        self.assertEqual(back.getpixel((1, 1)), (255, 0, 0, 255))
        self.assertEqual(front.getbbox(), (0, 0, 2, 2))
        self.assertEqual(full.getpixel((1, 1)), (0, 255, 0, 255))
        self.assertEqual(meta['shared_background'][0]['transform'], [1, 0, 3, 0, 1, 2])


if __name__ == '__main__':
    unittest.main()
