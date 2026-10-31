"""Exact replacements retain compacted logical slots and the 1 MiB contract."""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as t
import retile_appended_resources as retile
import retile_character_ui12 as character
from test_retile_appended_resources import source


def tile(color):
    return b'\x10\x10'+struct.pack('<H', color)*256+bytes([255])*256


class Compacted:
    tile_count, tile_groups, per_group = 6, 2, 4
    cache = {}
    def group(self, group):
        return (tile(0x1234) if group == 0 else tile(0x5678)+tile(0x9abc), None)


class ExactRetileTest(unittest.TestCase):
    def test_bulk_replacement_prunes_before_descriptor_capacity_is_exceeded(self):
        with tempfile.TemporaryDirectory() as temporary, redirect_stdout(StringIO()):
            root = Path(temporary); original = retile.view(source())
            for step in range(2):
                work = root/str(step); work.mkdir()
                plan = character.make_plan(original, work, [4,5])
                encoded = retile.compress_plan(plan, work, work/'encoded', 1)
                data, _ = retile.rewrite(original, plan, encoded)
                original = retile.view(data)
            self.assertEqual(t.TileReader(original).frame_count, 5)
            for selected in ([4,5], [4]):
                work = root/('final-'+str(len(selected))); work.mkdir()
                plan = character.make_plan(original, work, selected)
                encoded = retile.compress_plan(plan, work, work/'encoded', 1)
                if len(selected) == 2:
                    with self.assertRaisesRegex(ValueError, 'needs compact_replaced_directory'):
                        retile.rewrite(original, plan, encoded)
                plan['compact_replaced_directory'] = True
                output, receipt = retile.rewrite(original, plan, encoded)
                character.verify(original, output, plan, work)
                after = t.TileReader(retile.view(output)); before = t.TileReader(original)
                self.assertEqual(after.frame_count, 2)
                self.assertGreater(receipt['removed_old_descriptors'], 0)
                self.assertGreater(receipt['removed_old_index_groups'], 0)
                for image in (4,5):
                    self.assertEqual(before.image_raw(image), after.image_raw(image))

    def test_compacted_slots_do_not_shift_following_groups(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'tiles.raw'
            used = np.array([1,0,0,0,1,1], dtype=bool)
            catalog = character.RecycledCatalog(Compacted(), used, path)
            self.assertEqual(path.stat().st_size, 6*t.TILE_BYTES)
            self.assertEqual(catalog.add(tile(0x5678)), 4)
            self.assertEqual(catalog.add(tile(0x1111)), 1)
            self.assertEqual(catalog.add(tile(0x2222)), 2)
            self.assertEqual(catalog.add(tile(0x3333)), 3)
            self.assertEqual(catalog.add(tile(0x4444)), 6)
            catalog.close()
            data = path.read_bytes()
            for number, color in ((0,0x1234),(1,0x1111),(2,0x2222),(3,0x3333),
                                  (4,0x5678),(5,0x9abc),(6,0x4444)):
                self.assertEqual(data[number*t.TILE_BYTES:(number+1)*t.TILE_BYTES], tile(color))

    def test_modern_dictionary_and_default_pixel_precision_are_preserved(self):
        with tempfile.TemporaryDirectory() as temporary, redirect_stdout(StringIO()):
            work = Path(temporary); data = bytearray(source()); before = retile.view(data)
            header = list(t.GLOBAL.unpack_from(data, before.base_end))
            header[3], header[11], header[12] = 16, 1360, t.LARGE_GROUP_LIMIT
            t.GLOBAL.pack_into(data, before.base_end, *header); struct.pack_into('<I', data, 4, 45)
            original = retile.view(data)
            with patch.object(character, 'quantize_image', side_effect=AssertionError('Lossy stage called')):
                plan = character.make_plan(original, work, [4,5])
            self.assertEqual(plan['precision'], 'exact')
            self.assertEqual(plan['dictionary'], t.LARGE_GROUP_LIMIT)
            encoded = retile.compress_plan(plan, work, work/'encoded', 1)
            output, _ = retile.rewrite(original, plan, encoded)
            character.verify(original, output, plan, work)
            after = retile.view(output); reader = t.TileReader(after)
            self.assertEqual(reader.dictionary, t.LARGE_GROUP_LIMIT)
            self.assertEqual(reader.per_group, 1360)
            self.assertEqual(original.image_raw(4), reader.image_raw(4))
            self.assertEqual(t.TileReader(original).image_raw(5), reader.image_raw(5))


if __name__ == '__main__':
    unittest.main()
