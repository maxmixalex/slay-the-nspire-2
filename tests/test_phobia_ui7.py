"""Public geometry/codec cases for the explicit original alternate mode."""
from collections import OrderedDict
from pathlib import Path
import re
import sys
from tempfile import TemporaryDirectory
import unittest

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as tiles
import patch_phobia_assets as phobia
from patch_feedback_assets import write_image
from world_scene_assets import render_scene


class Pack:
    def __init__(self, scenes, images):
        self.scenes, self.images = scenes, images
        self.entries = dict.fromkeys(scenes)

    def read(self, path):
        return self.scenes[path.removeprefix('res://')].encode()


def texture(pack, path):
    return pack.images[path]


class ImageView:
    image_raw = tiles.PackView.image_raw

    def __init__(self, rows, grouped):
        self.version, self.data = 25, grouped['payload']
        self.images = [(off, row['width'], row['height'], fmt, count)
                       for row, (off, fmt, count) in zip(rows, grouped['entries'])]
        self.retained_cache = OrderedDict()

    def block(self, offset, count):
        tiles.require(0 <= offset <= len(self.data)-count, 'Synthetic image outside payload')
        return self.data[offset:offset+count]


class PhobiaUI7(unittest.TestCase):
    def test_explicit_background_mode_never_changes_normal_branch(self):
        scene = '''[gd_scene format=3]
[ext_resource type="Texture2D" path="res://red.png" id="red"]
[ext_resource type="Texture2D" path="res://blue.png" id="blue"]
[node name="Root" type="Node2D"]
[node name="Visual" type="Sprite2D" parent="."]
centered = false
texture = ExtResource("red")
[node name="PhobiaModeVisual" type="Sprite2D" parent="."]
visible = false
centered = false
texture = ExtResource("blue")
'''
        pack = Pack({'main.tscn': scene}, {'red.png': Image.new('RGBA', (8, 8), 'red'),
                                         'blue.png': Image.new('RGBA', (8, 8), 'blue')})
        def draw(enabled=False):
            return render_scene(pack, 'main.tscn', 8, 8, viewport=(8, 8),
                                texture_loader=texture, phobia_mode=enabled)
        normal, before = draw()
        alternate, report = draw(True)
        restored, after = draw()
        self.assertEqual(normal.getpixel((4, 4)), (255, 0, 0, 255))
        self.assertEqual(alternate.getpixel((4, 4)), (0, 0, 255, 255))
        self.assertEqual(report['phobia_layers'], 1)
        self.assertEqual(normal.tobytes(), restored.tobytes())
        self.assertEqual(before, after)

    def test_static_body_keeps_original_child_registration_and_dead_center(self):
        scene = '''[gd_scene format=3]
[ext_resource type="Texture2D" path="res://body.png" id="body"]
[node name="Creature" type="Node2D"]
position = Vector2(700, 600)
[node name="PhobiaModeVisuals" type="Sprite2D" parent="."]
position = Vector2(2, -3)
scale = Vector2(0.5, 0.25)
texture = ExtResource("body")
'''
        pack = Pack({'body.tscn': scene}, {'body.png': Image.new('RGBA', (8, 12), 'red'),
                                         'dead.png': Image.new('RGBA', (4, 8), 'blue')})
        spec = dict(scene_path='body.tscn', source='body.png', dead_source='dead.png',
                    position=[2, -3], scale=[.5, .25])
        draw, bounds = phobia.static_geometry(pack, spec, texture_loader=texture)
        dead, dead_bounds = phobia.static_geometry(pack, spec, dead=True, texture_loader=texture)
        self.assertEqual(bounds, [[0, 1.5], [4, 4.5]])
        self.assertEqual(dead_bounds, [[1, 2], [3, 4]])
        layout = phobia.anchored_layout([bounds, dead_bounds], 2)
        self.assertEqual(layout['scale'], 2)
        self.assertEqual(layout['origin'], [2, 11])
        self.assertEqual(layout['size'], [12, 10])
        normal = phobia.raster(draw, layout['scale'], layout['origin'], layout['size'])
        alternate = phobia.raster(dead, layout['scale'], layout['origin'], layout['size'])
        self.assertEqual(normal.getbbox(), (2, 2, 10, 8))
        self.assertEqual(alternate.getbbox(), (4, 3, 8, 7))
        with self.assertRaisesRegex(ValueError, 'registration'):
            phobia.static_geometry(pack, dict(spec, position=[0, 0]), texture_loader=texture)

    def test_alternate_fit_preserves_verified_scale_or_rejects_overflow(self):
        layout = phobia.anchored_layout([[[-5, -8], [9, 12]], [[-7, -2], [20, 9]]], .5)
        self.assertEqual(layout['scale'], .5)
        self.assertEqual(layout['origin'], [5.5, 8])
        self.assertEqual(layout['size'], [18, 14])
        with self.assertRaisesRegex(ValueError, 'normal scale'):
            phobia.anchored_layout([[[0, 0], [1000, 10]]], 1)
        class Normal:
            clips = [(0, 2, 33, 4, 7, 0)]
            images = [(0, 12, 15), (0, 12, 15)]
        job = dict(id=0, frames=2, model='Synthetic')
        phobia.validate_normal_layout(Normal(), [job], dict(size=[12, 15], origin=[4.1, 7.2]))
        with self.assertRaisesRegex(ValueError, 'normal layout'):
            phobia.validate_normal_layout(Normal(), [job], dict(size=[12, 15], origin=[4.6, 7.2]))

    def test_grouped_new_art_is_lossless_and_reuses_exact_frames(self):
        with TemporaryDirectory() as directory:
            cache = Path(directory)
            image = Image.fromarray(np.tile(np.array([[255, 0, 0, 255], [0, 0, 255, 255]],
                                                     dtype=np.uint8), (32, 16, 1)))
            rows = [write_image(cache, 'frame-'+str(i), image, sparse=True) for i in range(3)]
            grouped = phobia.grouped_art(rows, cache)
            decoded = ImageView(rows, grouped)
            original = phobia.RowImages(rows, [(cache/row['file']).read_bytes() for row in rows])
            for i in range(3):
                self.assertEqual(decoded.image_raw(i), original.image_raw(i))
            self.assertEqual(grouped['report']['exact_frame_aliases'], 2)
            self.assertLess(grouped['report']['stored_payload_bytes'], grouped['report']['original_payload_bytes'])
            self.assertEqual(grouped['report']['runtime_buffer_growth'], 0)

    def test_complete_numeric_directory_matches_runtime_lookup(self):
        rows = phobia.original_jobs()
        keys = [row['key'] for row in rows]
        for spec in phobia.PROFILE['static_bodies']:
            keys.append(spec['key'])
            if 'dead_key' in spec:
                keys.append(spec['dead_key'])
        index = (phobia.ROOT/'include/phobia_clip_index.inc').read_text()
        actual = {key: int(value) for key, value in re.findall(r'\{"([^"]+)",(\d+)\}', index)}
        expected = {key: phobia.BASE_CLIPS+i for i, key in enumerate(keys)}
        self.assertEqual(actual, expected)
        self.assertEqual(len(rows), 32)
        self.assertEqual(len(expected), 43)
        self.assertEqual(phobia.CLIPS, max(expected.values())+1)
        self.assertEqual(phobia.AUXILIARY-phobia.BASE_AUX, 4)


if __name__ == '__main__':
    unittest.main()
