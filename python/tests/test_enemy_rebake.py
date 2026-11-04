"""Independent source canvases and recycled-tile registration preservation."""

import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path

import numpy as np
from nspire_converter.rebake_enemy import LCD_DENSITY, clip_layout
from nspire_converter.retile_appended_resources import compress_plan, rewrite, view
from nspire_converter.retile_character import make_plan, verify
from PIL import Image
from test_retile_appended_resources import source


class EnemyRebakeTest(unittest.TestCase):
    def test_idle_no_longer_inherits_a_distant_attack_canvas(self):
        idle = clip_layout([[-300, -60], [260, 390]])
        attack = clip_layout([[-6000, -60], [260, 390]])
        self.assertGreater(idle["scale"], attack["scale"] * 3)
        self.assertLessEqual(idle["scale"], LCD_DENSITY)
        self.assertAlmostEqual(idle["scale"] * idle["sprite_units"], 1024)
        for layout in (idle, attack):
            lo, hi = np.asarray(layout["bounds"])
            x, y = layout["origin"]
            scale = layout["scale"]
            self.assertGreaterEqual(x + lo[0] * scale, 2)
            self.assertGreaterEqual(y - hi[1] * scale, 2)
            self.assertGreaterEqual(layout["size"][0] - (x + hi[0] * scale), 2)
            self.assertGreaterEqual(layout["size"][1] - (y - lo[1] * scale), 2)
            self.assertLessEqual(layout["size"][0], 320)
            self.assertLessEqual(layout["size"][1], 218)
            self.assertIsInstance(x, int)
            self.assertIsInstance(y, int)

    def test_geometric_replacement_preserves_other_pixels_and_all_clocks(self):
        with tempfile.TemporaryDirectory() as temporary, redirect_stdout(StringIO()):
            work = Path(temporary)
            original = view(source())
            replacements = {}
            for image, size in ((4, (27, 32)), (5, (31, 28))):
                path = work / f"{image}.png"
                Image.new("RGBA", size, (48, 91, 210, 171)).save(path)
                replacements[image] = path
            clips = {0: (4, 2, 100, 3, 21, 5376)}
            plan = make_plan(
                original,
                work,
                replacements,
                replacements,
                allow_geometry=True,
                clip_overrides=clips,
            )
            encoded = compress_plan(plan, work, work / "encoded", 1)
            for input_bytes in (source(), source("zhs", glyphs=3)):
                locale = view(input_bytes)
                output, _ = rewrite(locale, plan, encoded)
                receipt = verify(locale, output, plan, work)
                after = view(output)
                self.assertEqual(after.clips[0], clips[0])
                self.assertEqual(after.images[4][1:3], (27, 32))
                self.assertEqual(after.images[5][1:3], (31, 28))
                self.assertTrue(receipt["all_frames_clocks_preserved"])
                self.assertEqual(receipt["registered_clip_layouts"], 1)
                self.assertFalse(receipt["all_frames_dimensions_clocks_preserved"])
            with self.assertRaisesRegex(ValueError, "frame IDs, counts or clocks"):
                make_plan(
                    original,
                    work,
                    replacements,
                    replacements,
                    allow_geometry=True,
                    clip_overrides={0: (4, 2, 99, 3, 21, 5376)},
                )
            with self.assertRaisesRegex(ValueError, "every frame"):
                make_plan(
                    original,
                    work,
                    [4],
                    {4: replacements[4]},
                    allow_geometry=True,
                    clip_overrides=clips,
                )


if __name__ == "__main__":
    unittest.main()
