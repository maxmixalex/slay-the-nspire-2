"""Transparent tile alignment preserves source-world pixels and clocks."""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import sys
import tempfile
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from align_enemy_tiles_ui13 import padded_sparse, world_pixel_signature, padding_layouts, verify_world_samples
from retile_character_ui12 import make_plan, verify
from retile_appended_resources import view, compress_plan, rewrite
from sparse_image import encode
from test_retile_appended_resources import source


class EnemyTileAlignmentTest(unittest.TestCase):
    def test_padding_retains_every_alpha_sample_and_world_coordinate(self):
        image = Image.new('RGBA', (19, 21))
        for x in range(4, 15):
            for y in range(3, 19):image.putpixel((x, y), (8*x, 4*y, 80, x*y % 255+1))
        raw = encode(image);padded = padded_sparse(raw, 19, 21, 7, 11, 32, 32)
        self.assertEqual(world_pixel_signature(raw, 21, 25, 37, 5376),
                         world_pixel_signature(padded[3], 32, 32, 48, 5376))
        with self.assertRaisesRegex(ValueError, 'crop or rescale'):
            padded_sparse(raw, 19, 21, 7, 11, 25, 32)
        self.assertNotEqual(world_pixel_signature(raw, 21, 25, 37, 5376),
                            world_pixel_signature(padded[3], 32, 31, 48, 5376))

    def test_retile_preserves_source_frames_fonts_and_world_identity(self):
        with tempfile.TemporaryDirectory() as temporary, redirect_stdout(StringIO()):
            work = Path(temporary);original = view(source());replacements = {}
            for image in (4, 5):
                path = work/f'{image}.png';Image.new('RGBA', (19, 21), (48, 91, 210, 171)).save(path);replacements[image] = path
            clips = {0: (4, 2, 100, 25, 37, 5376)}
            first = make_plan(original, work, replacements, replacements, allow_geometry=True, clip_overrides=clips)
            baked, _ = rewrite(original, first, compress_plan(first, work, work/'encoded', 1));registered = view(baked)
            layouts = padding_layouts(registered, [dict(clip=0, key='Fixture.idle_loop')])
            layout = layouts[0];dx, dy = layout['offset'];clip = registered.clips[0]
            overrides = {0: (*clip[:3], clip[3]+dx, clip[4]+dy, clip[5])}
            def transform(image, row):return padded_sparse(row[3], row[0], row[1], dx, dy, *layout['size'])
            second = make_plan(registered, work, [4, 5], precision=8, allow_geometry=True,
                               clip_overrides=overrides, raw_transform=transform)
            output, _ = rewrite(registered, second, compress_plan(second, work, work/'encoded', 1))
            checked = verify(registered, output, second, work)
            self.assertTrue(checked['all_frames_clocks_preserved'])
            exact = verify_world_samples(registered, view(output), layouts)
            self.assertEqual(exact['world_frames_exact'], 2)
            self.assertTrue(exact['source_density_unchanged'])


if __name__ == '__main__':unittest.main()
