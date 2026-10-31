"""Copy-span merging preserves every changed pixel and its exact page budget."""
from pathlib import Path
import struct
import sys
import unittest
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from character_select_delta_ui22 import runs, geometry


class SelectionGeometryTest(unittest.TestCase):
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
