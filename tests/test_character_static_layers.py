"""Exact static-plane extraction from small synthetic character loops."""
from pathlib import Path
import sys
import tempfile
import unittest

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from character_static_layers import decoded, split_loop
from patch_feedback_assets import write_image


class StaticCharacterLayers(unittest.TestCase):
    def frame(self, directory, index, rgba):
        row = write_image(directory, f'frame-{index}', Image.fromarray(rgba, 'RGBA'), sparse=True)
        row.update(source='synthetic-character-scene', frame=index, sample_seconds=index/12)
        return row

    def test_disjoint_exact_opaque_alpha_and_edge_regions(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            rgba = np.zeros((17, 35, 4), dtype='u1')
            rgba[:, :16] = [157, 67, 232, 123]
            rgba[:, 16:32] = [83, 119, 17, 255]
            rgba[:, 32:] = [11, 193, 129, 255]
            rows = []
            for index in range(3):
                moving = rgba.copy()
                moving[0, 17] = [index*32, 201, 39, 255 if index != 1 else 0]
                rows.append(self.frame(directory, index, moving))
            base, deltas, report = split_loop(rows, directory, 'test-loop')
            self.assertEqual(len(deltas), len(rows))
            self.assertTrue(report['all_composited_rgb565_alpha_equal'])
            base_color, base_alpha = decoded(base, directory)
            self.assertTrue(np.all(base_alpha[:16, 16:32] == 0))
            self.assertTrue(np.all(base_alpha[:, :16] == 123))
            self.assertTrue(np.all(base_alpha[:, 32:] == 255))
            # The one-pixel bottom edge is an independently constant region.
            self.assertTrue(np.all(base_alpha[16, 16:32] == 255))
            for index, (original, delta) in enumerate(zip(rows, deltas)):
                colors, alpha = decoded(original, directory)
                delta_color, delta_alpha = decoded(delta, directory)
                self.assertFalse(np.any((base_alpha != 0) & (delta_alpha != 0)))
                np.testing.assert_array_equal(np.where(base_alpha, base_color, delta_color), colors)
                np.testing.assert_array_equal(np.where(base_alpha, base_alpha, delta_alpha), alpha)
                self.assertEqual(delta['frame'], index)
                self.assertEqual(delta['sample_seconds'], original['sample_seconds'])
                self.assertEqual(delta['original_raw_sha256'], original['raw_sha256'])

    def test_identical_frames_have_empty_dynamic_planes(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            rgba = np.full((16, 32, 4), 255, dtype='u1')
            rows = [self.frame(directory, index, rgba) for index in range(8)]
            base, deltas, report = split_loop(rows, directory, 'static-loop')
            self.assertEqual(report['static_visible_pixels'], 512)
            self.assertGreater(report['independent_bytes_saved'], 0)
            for delta in deltas:
                self.assertFalse(np.any(decoded(delta, directory)[1]))
            self.assertTrue(np.all(decoded(base, directory)[1] == 255))

    def test_corrupt_input_is_rejected_before_reuse(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            row = self.frame(directory, 0, np.full((8, 8, 4), 255, dtype='u1'))
            path = directory/row['file']
            raw = bytearray(path.read_bytes());raw[-1] ^= 1;path.write_bytes(raw)
            with self.assertRaisesRegex(ValueError, 'Changed character image'):
                split_loop([row], directory, 'corrupt-loop')


if __name__ == '__main__':
    unittest.main()
