"""Color candidates must keep source alpha, pixels' positions and typed words."""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import struct
import sys
import tempfile
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import animation_color_candidates_ui23 as candidate
import content_tiles as tiles
import retile_appended_resources as retile
from test_retile_appended_resources import source


class AnimationColorCandidateTest(unittest.TestCase):
    def test_all_rgb565_words_have_declared_bounds(self):
        original = np.arange(65536, dtype=np.int32)
        for policy, expected in [('555a8', (0, 1, 0)), ('454a8', (1, 1, 1)),
                                 ('444a8', (1, 2, 1))]:
            mapped = candidate.color_lut(policy).astype(np.int32)
            actual = tuple(int(np.abs(((mapped >> shift) & mask) -
                                     ((original >> shift) & mask)).max())
                           for shift, mask in ((11, 31), (5, 63), (0, 31)))
            self.assertEqual(actual, expected)
            self.assertEqual((mapped[0], mapped[-1]), (0, 65535))

    def test_every_opacity_class_and_tile_edge_survive(self):
        raw = bytes((16, 16)) + np.arange(256, dtype='<u2').tobytes() + bytes(range(256))
        for policy in candidate.LEVELS:
            output = candidate.color_tiles(raw, policy)
            self.assertEqual(output[:2], raw[:2])
            self.assertEqual(output[514:], candidate.alpha_lut(policy).tobytes())
            before = np.arange(256, dtype=np.int32)
            after = np.frombuffer(output[514:], dtype='u1').astype(np.int32)
            self.assertLessEqual(int(np.max(np.abs(after-before))), candidate.alpha_error(policy))
            self.assertTrue(np.array_equal(before == 0, after == 0))
            self.assertTrue(np.array_equal(before == 255, after == 255))
            self.assertEqual(len(output), len(raw))

    def test_adaptive_colors_have_same_premultiplied_error_bound(self):
        colors = np.arange(65536, dtype='<u2')
        for opacity in (0, 1, 16, 17, 51, 52, 127, 128, 254, 255):
            alpha = np.full(65536, opacity, dtype='u1')
            mapped = candidate.mapped_colors(colors, alpha, '444a8-adaptive').astype(np.int32)
            for shift, mask, bound in ((11, 31, 1), (5, 63, 2), (0, 31, 1)):
                delta = np.abs(((mapped >> shift) & mask) -
                               ((colors.astype(np.int32) >> shift) & mask))
                self.assertLessEqual(int(delta.max()) * opacity, bound * 255)

    def test_alpha_quantization_keeps_implicit_geometry_byte_exact(self):
        import implicit_geometry
        alpha = np.array([list(range(256))], dtype='u1')
        original = implicit_geometry.alpha_geometry(alpha)
        self.assertEqual(original, implicit_geometry.alpha_geometry(candidate.alpha_lut('444a6')[alpha]))

    def test_complete_pack_keeps_geometry_and_opaque_parameter_words(self):
        with tempfile.TemporaryDirectory() as tmp, redirect_stdout(StringIO()):
            root = Path(tmp)
            original = retile.view(source())
            plan = retile.make_plan(original, root)
            encoded = retile.compress_plan(plan, root, root/'cache', 1)
            data, _ = retile.rewrite(original, plan, encoded)
            before = retile.view(data)
            for policy in candidate.LEVELS:
                results = candidate.convert(before, policy, workers=1)
                output = candidate.rewrite(before, {}, results)
                candidate.verify(before, output, results, policy)
                after = retile.view(output)
                for image in range(len(before.images)):
                    if before.images[image][3] == 6:
                        w, h, fmt, raw = tiles.TileReader(before).image_raw(image)
                        actual = tiles.TileReader(after).image_raw(image)
                        self.assertEqual(actual, (w, h, fmt, candidate.color_image(raw, fmt, policy, w, h)))
                        self.assertEqual(tiles.validate_sparse(raw, w, h),
                                         tiles.validate_sparse(actual[3], w, h))
                    else:
                        w, h, fmt, raw = before.image_raw(image)
                        if fmt == 0:
                            self.assertEqual(after.image_raw(image), (w, h, fmt, raw))

    def test_experiment_is_never_silent_default(self):
        with redirect_stdout(StringIO()), self.assertRaises(SystemExit):
            candidate.main(['--input', '/tmp/source.nsp', '--output', '/tmp/out.nsp',
                            '--policy', '444a8'])


if __name__ == '__main__':
    unittest.main()
