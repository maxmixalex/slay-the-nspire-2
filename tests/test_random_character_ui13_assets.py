"""Public, source-free cases for Godot's seamless grayscale transport."""
import sys
from pathlib import Path
import unittest
from unittest.mock import patch

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import random_character_ui13_assets as random_assets


class SeamlessNoise(unittest.TestCase):
    def test_particle_linear_view_preserves_every_original_texel(self):
        original = np.arange(256*256*4, dtype=np.uint32).reshape(256, 256, 4)
        original = np.uint8(original ^ (original >> 8) ^ (original >> 16))
        source = Image.fromarray(original, 'RGBA')
        physical = random_assets.linear_texture_image(source)
        self.assertEqual(physical.size, (320, 205))
        self.assertEqual(physical.tobytes()[:256*256*4], source.tobytes())
        self.assertEqual(physical.tobytes()[256*256*4:], bytes(64*4))

    def test_small_original_corner_and_skirts(self):
        # One-texel skirts copy the extended source edges at alpha255.
        # This literal 3x3 image makes all four source quadrants distinct.
        values = np.arange(9, dtype=np.uint8).reshape(3, 3)*10
        with patch.object(random_assets, '_noise_values', return_value=values) as source:
            image = random_assets.godot_noise_image(2, 2, .0259, .093, 'header', 'cache')
        source.assert_called_once_with(3, 3, .0259, 'header', 'cache')
        self.assertEqual(list(image.getchannel('A').getdata()), [40, 50, 70, 80])
        self.assertTrue(np.all(np.asarray(image)[:, :, :3] == 255))

    def test_shifted_quadrants_remain_seamless(self):
        values = np.arange(25, dtype=np.uint8).reshape(5, 5)*10
        with patch.object(random_assets, '_noise_values', return_value=values):
            image = random_assets.godot_noise_image(4, 4, .0259, .093, 'header', 'cache')
        self.assertEqual(np.asarray(image)[:, :, 3].tolist(),
                         [[120, 130, 140, 110], [170, 180, 190, 160],
                          [220, 230, 240, 210], [70, 80, 90, 60]])

    def test_source_r8_is_not_rgb565_quantized(self):
        values = np.array([[0, 1, 2], [3, 4, 5], [6, 7, 8]], dtype=np.uint8)
        with patch.object(random_assets, '_noise_values', return_value=values):
            image = random_assets.godot_noise_image(2, 2, .0259, .093, 'header', 'cache')
        self.assertEqual(list(image.getchannel('A').getdata()), [4, 5, 7, 8])
        self.assertEqual(image.mode, 'RGBA')


if __name__ == '__main__':
    unittest.main()
