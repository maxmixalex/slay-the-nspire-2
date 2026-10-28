"""Asset-free tests of authored UI tint and nine-patch composition."""
from pathlib import Path
import sys
import unittest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from world_ui_assets import component
from world_ui_profile import AUXILIARY_ROWS


class Components(unittest.TestCase):
    def test_reticle_native_corner_keeps_source_colors_and_alpha(self):
        # Asymmetric colors, fractional alpha and hidden RGB distinguish
        # source preservation from accidental tint, crop or mirroring. The
        # runtime owns those transforms and independently draws its shadows.
        row = next(row for row in AUXILIARY_ROWS if row['key'] == 'map.SELECTION_RETICLE')
        image = Image.new('RGBA', (48, 48))
        image.putdata([((x * 17) % 256, (y * 19) % 256, (x + y * 3) % 256,
                        (0, 1, 127, 128, 254, 255)[(x + y) % 6])
                       for y in range(48) for x in range(48)])
        self.assertEqual(component(image, row).tobytes(), image.tobytes())

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

    def test_ancient_rows_keep_original_patch_parameters(self):
        body = next(row for row in AUXILIARY_ROWS if row['key'] == 'events.ANCIENT_OPTION')
        outline = next(row for row in AUXILIARY_ROWS if row['key'] == 'events.ANCIENT_OPTION_OUTLINE')
        self.assertEqual(body['kind'], 'component')
        self.assertEqual(body['source_size'], [100, 100])
        self.assertEqual(body['nine_patch']['margins'], [34] * 4)
        self.assertEqual(body['size'], [292, 40])
        self.assertEqual(outline['source_size'], [120, 120])
        self.assertEqual(outline['nine_patch']['margins'], [44] * 4)
        self.assertEqual(outline['size'], [301, 52])
        # The original white source is a mask. A model-specific runtime
        # modulation must not be baked into a shared ancient's texture.
        self.assertNotIn('modulate', body)
        self.assertNotIn('modulate', outline)

    def test_nonuniform_target_scale_preserves_separate_border_widths(self):
        image = Image.new('RGBA', (12, 12), (0, 0, 200, 255))
        image.paste((200, 0, 0, 255), (0, 0, 4, 4))
        image.paste((0, 200, 0, 255), (4, 4, 8, 8))
        result = component(image, dict(key='two-axis', source_size=[12,12], size=[40,20],
            nine_patch=dict(margins=[4]*4, scale=[.5, 1.])))
        self.assertEqual(result.getpixel((1, 3)), (200, 0, 0, 255))
        self.assertEqual(result.getpixel((2, 3)), (0, 0, 200, 255))
        self.assertEqual(result.getpixel((2, 4)), (0, 200, 0, 255))
        self.assertEqual(result.getpixel((37, 15)), (0, 200, 0, 255))
        self.assertEqual(result.getpixel((38, 15)), (0, 0, 200, 255))

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
        for scale in ([1], [1, 0], [1, 2], [1, float('nan')], [1, 'invalid']):
            with self.subTest(scale=scale), self.assertRaises(ValueError):
                component(image, dict(key='bad-scale', source_size=[4,4], size=[8,8],
                    nine_patch=dict(margins=[1]*4, scale=scale)))


if __name__ == '__main__':
    unittest.main()
