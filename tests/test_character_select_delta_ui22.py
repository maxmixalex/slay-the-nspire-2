"""Copy-span merging preserves every changed pixel and its exact page budget."""
from pathlib import Path
import struct
import sys
import unittest
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from character_select_delta_ui22 import runs, geometry, merge_same_tile_spans
from coalesce_tile_resources import active_tiles


class SelectionGeometryTest(unittest.TestCase):
    def test_same_tile_merges_preserve_pixels_tiles_and_each_frame_page_count(self):
        mask = np.zeros((240,320), dtype=bool)
        # Include same-tile gaps, a tiny gap across the tile boundary, a long
        # cross-tile gap, full rows, and enough spans to constrain page slack.
        mask[:, ::3] = True
        mask[0] = False; mask[0, [1, 5, 14, 16, 48, 80]] = True
        mask[1] = True; mask[2] = False
        raw, decoded = geometry(runs(mask)[0], 0)
        for maximum in (0, 1, 8, 15):
            merged, actual_size, stats = merge_same_tile_spans(raw, decoded, maximum)
            self.assertEqual((actual_size+4095)//4096, (decoded+4095)//4096)
            self.assertTrue(np.array_equal(active_tiles(raw,320,240), active_tiles(merged,320,240)))
            actual = np.zeros_like(mask); at = counted = 0
            for y in range(240):
                count, = struct.unpack_from('<H', merged, at); at += 2; counted += 2
                for _ in range(count):
                    x, length, mode = struct.unpack_from('<HHB', merged, at); at += 5
                    self.assertEqual(mode, 0)
                    actual[y, x:x+length] = True; counted += 5+length*2
            self.assertEqual(actual_size, counted)
            self.assertEqual(at, len(merged))
            self.assertTrue(np.all(actual[mask]))
            self.assertFalse(actual[0,15], 'A gap crossing physical tiles must stay split')
            self.assertFalse(np.any(actual[2]))
            self.assertEqual(len(raw)-len(merged), 5*stats['merged_spans'])
            self.assertEqual(actual_size-decoded, stats['extra_decoded_bytes'])
            self.assertEqual(np.count_nonzero(actual & ~mask), stats['copied_unchanged_pixels'])
        self.assertGreater(stats['merged_spans'], 0)

    def test_cost_model_matches_empty_sparse_dense_and_merged_rows(self):
        mask = np.zeros((240,320), dtype=bool)
        mask[1, [0,1,4,8,19,110,319]] = True
        mask[70, :] = True; mask[101, 40:51] = True
        rows, costs = runs(mask)
        for gap in (0,1,2,4,8,16,32,64,128,320):
            raw, decoded = geometry(rows, gap)
            actual = np.zeros_like(mask); at = count = 0
            for y in range(240):
                spans, = struct.unpack_from('<H', raw, at); at += 2; count += 2
                for _ in range(spans):
                    x, size, mode = struct.unpack_from('<HHB', raw, at); at += 5
                    self.assertEqual(mode, 0); actual[y,x:x+size] = True; count += 5+size*2
            self.assertEqual(at, len(raw)); self.assertEqual(decoded, count)
            self.assertEqual(decoded, int(costs[gap]))
            self.assertTrue(np.all(actual[mask]))
            self.assertFalse(np.any(actual[0])); self.assertTrue(np.all(actual[70]))


if __name__ == '__main__':
    unittest.main()
