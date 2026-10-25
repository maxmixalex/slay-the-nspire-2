"""Precision boundaries, shared static protection and real NSP relocation."""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import struct
import sys
import tempfile
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import quantize_animation_resources as precision
import retile_appended_resources as retile
import split_content
from test_retile_appended_resources import source


class AnimationPrecisionTest(unittest.TestCase):
    def test_complete_color_domain_and_alpha_error_bounds(self):
        original = np.arange(65536, dtype=np.int32)
        alpha = np.arange(256, dtype=np.int32)
        for profile, expected in ((1, (1, 2, 1)), (2, (1, 1, 1)), (4, (0, 1, 0))):
            colors, opacity = precision.lookup(profile)
            colors = colors.astype(np.int32)
            errors = (np.max(np.abs((colors >> 11)-(original >> 11))),
                      np.max(np.abs(((colors >> 5) & 63)-((original >> 5) & 63))),
                      np.max(np.abs((colors & 31)-(original & 31))))
            self.assertEqual(errors, expected)
            self.assertLessEqual(np.max(np.abs(opacity.astype(np.int32)-alpha)), 2)
            self.assertEqual((int(opacity[0]), int(opacity[255])), (0, 255))
            self.assertEqual((int(colors[0]), int(colors[-1])), (0, 65535))

    def test_protected_tile_and_incompatible_shared_pixels(self):
        raw = bytearray(bytes((1, 1)) + bytes(768))
        struct.pack_into('<H', raw, 2, 0x18e3)
        raw[514] = 255
        self.assertEqual(precision.quantize_tiles(raw, np.asarray([8], dtype='u1')), raw)
        with self.assertRaisesRegex(ValueError, 'conflicting color'):
            precision.quantize_tiles(raw, np.asarray([1 | 4], dtype='u1'))
        struct.pack_into('<H', raw, 2, 65535)
        self.assertEqual(precision.quantize_tiles(raw, np.asarray([1 | 4], dtype='u1')), raw)

    def test_bilingual_pack_preserves_static_design_and_sparse_geometry(self):
        with tempfile.TemporaryDirectory() as temporary, redirect_stdout(StringIO()):
            work = Path(temporary)
            original = retile.view(source())
            diced = retile.make_plan(original, work)
            encoded = retile.compress_plan(diced, work, work/'retile-cache', 1)
            english, _ = retile.rewrite(original, diced, encoded)
            chinese, _ = retile.rewrite(retile.view(source('zhs', 3)), diced, encoded)
            before = precision.view(english)
            plan = precision.make_plan(before, work, '444a6')
            converted = precision.compress_plan(before, plan, work, work/'precision-cache', 1)
            a = precision.rewrite(before, plan, converted)
            b = precision.rewrite(precision.view(chinese), plan, converted)
            precision.verify(before, a, plan, work)
            precision.verify(precision.view(chinese), b, plan, work)
            after = precision.view(a)
            width, height, fmt, raw = tiles.TileReader(before).image_raw(4)
            result = tiles.TileReader(after).image_raw(4)
            self.assertEqual(result, (width, height, fmt,
                precision.quantize_image(raw, width, height, fmt, 1)))
            self.assertEqual(tiles.validate_sparse(raw, width, height),
                             tiles.validate_sparse(result[3], width, height))
            for image in (0, 1, 2, 3):
                self.assertEqual(before.image_raw(image), after.image_raw(image))
            la, lb = split_content.layout(a, 'eng'), split_content.layout(b, 'zhs')
            self.assertEqual(a[la['prefix']:la['prefix']+la['artwork']],
                             b[lb['prefix']:lb['prefix']+lb['artwork']])


if __name__ == '__main__':
    unittest.main()
