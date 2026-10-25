"""History art uses its own source icons, preserves alpha and appends stable pairs."""
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import history_body_ui10_assets as history


class HistoryBodyAssets(unittest.TestCase):
    def test_pairs_preserve_aspect_and_source_alpha(self):
        requested, captured = [], []

        def load(_pack, source):
            requested.append(source)
            # A non-square translucent icon exposes accidental stretch,
            # early black tint or removal of the original alpha channel.
            return Image.new('RGBA', (100, 50), (80, 160, 240, 128))

        def write(_cache, key, image):
            captured.append(image.copy())
            return {'file': key+'.rgba'}

        with tempfile.TemporaryDirectory() as directory:
            with patch.object(history, 'load_scene_texture', load), patch.object(history, 'write_image', write):
                rows = history.build_images(object(), Path(directory))
        self.assertEqual(len(rows), 64)
        self.assertEqual([row['auxiliary'] for row in rows], list(range(2068, 2132)))
        self.assertEqual(len(set(requested)), 64)
        self.assertIn('images/ui/run_history/unknown_monster.png', requested)
        self.assertIn('images/ui/run_history/queen_boss_outline.png', requested)
        for icon, outline in zip(rows[::2], rows[1::2]):
            self.assertEqual(outline['source'], icon['source'].replace('.png', '_outline.png'))
            self.assertEqual(icon['source_control_size'], [64, 64])
            self.assertEqual(outline['source_control_size'], [90, 90])
        for image in captured:
            self.assertEqual(image.size, (13, 13))
            self.assertEqual(image.getpixel((6, 0))[3], 0)
            pixel = image.getpixel((6, 6))
            self.assertEqual(pixel[3], 128)
            self.assertTrue(all(abs(actual-expected) <= 1 for actual, expected in zip(pixel[:3], (80, 160, 240))))
            self.assertEqual(image.getbbox()[2]-image.getbbox()[0], 13)
            self.assertLess(image.getbbox()[3]-image.getbbox()[1], 13)


if __name__ == '__main__':
    unittest.main()
