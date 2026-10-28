"""Dead index records disappear while every live image/font/clock stays exact."""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import struct
import sys
import tempfile
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from retile_appended_resources import view, compress_plan, rewrite
from retile_character_ui12 import make_plan
from prune_tile_directory_ui13 import prune, verify
from test_retile_appended_resources import source
import content_tiles as t


class TileDirectoryPruneTest(unittest.TestCase):
    def test_replaced_descriptors_and_whole_unused_index_group_are_removed(self):
        with tempfile.TemporaryDirectory() as temporary, redirect_stdout(StringIO()):
            work = Path(temporary);original = view(source());replacements = {}
            for image in (4, 5):
                path = work/f'{image}.png';Image.new('RGBA', original.images[image][1:3], (48, 91, 210, 171)).save(path)
                replacements[image] = path
            plan = make_plan(original, work, replacements, replacements)
            replaced, _ = rewrite(original, plan, compress_plan(plan, work, work/'encoded', 1))
            before = view(replaced);after, receipt = prune(replaced);checked = verify(before, after)
            self.assertGreater(receipt['removed_descriptors'], 0)
            self.assertGreater(receipt['removed_index_groups'], 0)
            self.assertGreater(receipt['saved_bytes'], 0)
            self.assertTrue(checked['all_image_dimensions_pivots_samples_and_clocks_exact'])
            old, new = t.TileReader(before), t.TileReader(view(after))
            for image, row in enumerate(before.images):
                a = old.image_raw(image) if row[3] == 6 else before.image_raw(image)
                b = new.image_raw(image) if row[3] == 6 else view(after).image_raw(image)
                self.assertEqual(a, b)
            twice, second = prune(after)
            self.assertEqual(twice, after)
            self.assertEqual(second['saved_bytes'], 0)

    def test_invalid_live_descriptor_is_not_treated_as_discardable(self):
        damaged = bytearray(source());before = view(damaged)
        tiled = next(row for row in before.images if row[3] == 6)
        struct.pack_into('<I', damaged, tiled[0]+4, 65535)
        with self.assertRaisesRegex(ValueError, 'live descriptor'):
            prune(bytes(damaged))


if __name__ == '__main__':unittest.main()
