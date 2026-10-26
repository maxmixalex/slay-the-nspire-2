"""Public pixel fixtures for exact regular-merchant frame factoring."""
from pathlib import Path
import sys
import tempfile
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from merchant_ui14_assets import (decoded_row, rgba_image, separated_planes,
                                 verify_recomposition)
from patch_feedback_assets import write_image


class MerchantSplitTests(unittest.TestCase):
    def test_all_frames_recompose_after_independent_sparse_lz4_decoding(self):
        # A black opaque invariant must not be confused with transparency;
        # partial-alpha invariants also move without a second blend operation.
        colors = np.array([[0, 0xffff, 0x8421, 0], [0xf800, 0x07e0, 0x001f, 0]], dtype='<u2')
        alpha = np.array([[255, 128, 255, 0], [255, 255, 33, 0]], dtype=np.uint8)
        frames = [(colors.copy(), alpha.copy()) for _ in range(3)]
        frames[1][0][1, 0] = 0xf810
        frames[1][1][1, 1] = 254
        frames[2][1][0, 2] = 0;frames[2][0][0, 2] = 0
        static, moving, count = separated_planes(frames)
        self.assertEqual(count, 3)
        self.assertEqual(int(static[1][0, 0]), 255)
        self.assertEqual(int(static[1][0, 1]), 128)
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            row = write_image(cache, 'fixed', rgba_image(*static), sparse=True)
            decoded_static, _ = decoded_row(row, cache)
            for index, (original, plane) in enumerate(zip(frames, moving)):
                row = write_image(cache, 'moving-'+str(index), rgba_image(*plane), sparse=True)
                decoded_moving, _ = decoded_row(row, cache)
                verify_recomposition(original, decoded_static, decoded_moving)
                self.assertFalse(np.any((decoded_static[1] != 0) & (decoded_moving[1] != 0)))

    def test_changing_alpha_remains_moving_and_overlap_fails(self):
        frames = [(np.array([[0xf81f]], dtype='<u2'), np.array([[value]], dtype=np.uint8))
                  for value in (1, 2, 1)]
        static, moving, count = separated_planes(frames)
        self.assertEqual(count, 0)
        for original, plane in zip(frames, moving):verify_recomposition(original, static, plane)
        with self.assertRaisesRegex(ValueError, 'overlaps'):
            verify_recomposition(frames[0], frames[0], frames[0])

    def test_shape_changes_fail_instead_of_changing_registration(self):
        frames = [(np.zeros((1, 2), dtype='<u2'), np.zeros((1, 2), dtype=np.uint8)),
                  (np.zeros((2, 1), dtype='<u2'), np.zeros((2, 1), dtype=np.uint8))]
        with self.assertRaisesRegex(ValueError, 'dimensions'):separated_planes(frames)


if __name__ == '__main__':unittest.main()
