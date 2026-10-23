import sys
import unittest
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import title_assets


class TitleParticles(unittest.TestCase):
    def test_seven_independent_fog_drawings_and_original_dot(self):
        # Deliberately different neighbors expose a crop/resampling seam or
        # incorrect vertical flipbook interpretation without private art.
        colors = [(31*i, 255-29*i, 17*i, 255) for i in range(7)]
        sheet = Image.new('RGBA', (252, 700))
        for i, color in enumerate(colors):
            sheet.paste(color, (0, i*100, 252, (i+1)*100))
        dot = Image.new('RGBA', (256, 256), (13, 47, 89, 255))
        requested = []

        def texture(_pack, name):
            requested.append(name)
            return sheet if name.endswith('fog_sheet_2.png') else dot

        result = title_assets.build_particle_atlas(None, texture)
        self.assertLessEqual(result.width, 320)
        self.assertLessEqual(result.height, 240)
        self.assertEqual(requested, ['images/vfx/fog_sheet_2.png', 'images/vfx/dot.png'])
        for i, color in enumerate(colors):
            x, y = (i % 2)*135, (i // 2)*54
            for dx, dy in ((0, 0), (67, 27), (134, 53)):
                self.assertEqual(result.getpixel((x+dx, y+dy)), color)
        self.assertEqual(result.getpixel((136, 163)), (13, 47, 89, 255))
        self.assertEqual(result.getpixel((140, 168)), (0, 0, 0, 0))
        self.assertEqual(sheet.getpixel((20, 600)), colors[6])

    def test_wrong_fog_grid_is_not_silently_reinterpreted(self):
        with self.assertRaisesRegex(ValueError, 'seven vertical frames'):
            title_assets.build_particle_atlas(None, lambda *_: Image.new('RGBA', (252, 699)))


if __name__ == '__main__':
    unittest.main()
