import unittest

import numpy as np
from nspire_converter import ancient_gpu_masks as a
from nspire_converter.world_scene_assets import _sections


class NativeMaskTest(unittest.TestCase):
    def test_typed_mask_bounds_and_exact_references(self):
        bindings = [dict(x=0, y=0, w=31, h=23, stride=32) for _ in range(8)]
        # Layout checks happen before the independent source-ramp hash gate.
        raw = bytearray(a.table(a.BASE_IMAGES, bindings, bytes(1280)))
        with self.assertRaisesRegex(ValueError, "source gradient"):
            a.validate_parameter(raw, a.IMAGES)
        raw[16] = 0
        with self.assertRaisesRegex(ValueError, "mask bounds"):
            a.validate_parameter(raw, a.IMAGES)

    def test_alpha_projection_keeps_only_native_samples(self):
        props = _sections(
            '[node name="x" type="GPUParticles2D"]\nposition = Vector2(900, 600)\n', "test"
        )[0][2]
        alpha = np.full((20, 20), 73, dtype="u1")
        (x, y, crop), error = a.projected_alpha(alpha, props)
        self.assertTrue(0 <= x < x + crop.shape[1] <= 320 and 0 <= y < y + crop.shape[0] <= 240)
        self.assertTrue(np.all(crop == 73))
        self.assertLessEqual(error, 0.5000001)

    def test_bilinear_source_clamp_and_texel_centers(self):
        alpha = np.array([[0, 64], [128, 255]], dtype="u1")
        u = np.array([0.25, 0.75, 0.25, 0.75, 0, 1])
        v = np.array([0.25, 0.25, 0.75, 0.75, 0, 1])
        self.assertTrue(np.array_equal(a.bilinear_alpha(alpha, u, v), [0, 64, 128, 255, 0, 255]))


if __name__ == "__main__":
    unittest.main()
