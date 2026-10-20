"""Asset-free tests of authored UI tint and nine-patch composition."""
from pathlib import Path
import sys
import unittest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from world_ui_assets import component


class Components(unittest.TestCase):
    def test_nine_patch_keeps_corners_and_extends_original_center(self):
        image = Image.new('RGBA', (3, 3))
        colors = [(x*80, y*80, 40, 255) for y in range(3) for x in range(3)]
        image.putdata(colors)
        result = component(image, dict(key='test', source_size=[3,3], size=[7,9],
            nine_patch=dict(margins=[1,1,1,1], scale=1)))
        self.assertEqual(result.getpixel((0,0)), colors[0])
        self.assertEqual(result.getpixel((6,0)), colors[2])
        self.assertEqual(result.getpixel((0,8)), colors[6])
        self.assertEqual(result.getpixel((6,8)), colors[8])
        self.assertEqual(result.getpixel((3,4)), colors[4])

    def test_authored_black_panel_changes_rgb_and_alpha(self):
        image = Image.new('RGBA', (2, 2), (255,255,255,128))
        result = component(image, dict(key='panel', source_size=[2,2], size=[2,2],
            fit='stretch', modulate=[0,0,0,.5]))
        self.assertEqual(result.getpixel((0,0)), (0,0,0,64))

    def test_non_patch_preserves_empty_atlas_margin(self):
        image = Image.new('RGBA', (6,4))
        image.paste((255,0,0,255), (2,1,4,3))
        result = component(image, dict(key='button', source_size=[6,4], size=[6,4], fit='stretch'))
        self.assertEqual(result.getbbox(), (2,1,4,3))

    def test_source_drift_and_unrepresentable_borders_are_errors(self):
        image = Image.new('RGBA', (4,4))
        with self.assertRaises(ValueError):
            component(image, dict(key='drift', source_size=[8,8], size=[4,4], fit='stretch'))
        with self.assertRaises(ValueError):
            component(image, dict(key='small', source_size=[4,4], size=[2,2],
                nine_patch=dict(margins=[1,1,1,1], scale=1)))
        with self.assertRaises(ValueError):
            component(image, dict(key='invalid', source_size=[4,4], size=[8,8],
                nine_patch=dict(margins=[2,1,2,1], scale=1)))


if __name__ == '__main__':
    unittest.main()
