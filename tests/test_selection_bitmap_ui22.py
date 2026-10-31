import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import selection_bitmap as bitmap


def geometry(rows):
    output = bytearray()
    for spans in rows:
        output.extend(struct.pack('<H', len(spans)))
        for row in spans:
            output.extend(struct.pack('<HHB', *row))
    return bytes(output)


class SelectionBitmapTests(unittest.TestCase):
    def test_exact_sparse_coverage_active_tiles_and_size(self):
        rows = [[(x, 1, 0) for x in range(16, 304, 3)] if 16 <= y < 224 else []
                for y in range(240)]
        source = geometry(rows); encoded = bitmap.encode_geometry(320, 240, source)
        self.assertIsNotNone(encoded)
        self.assertEqual(len(source), len(encoded))
        self.assertEqual(bitmap.restore_geometry(320, 240, encoded), source)
        active = int.from_bytes(bitmap.active_mask(320, 240, encoded), 'little')
        for y in range(15):
            for x in range(20):
                self.assertEqual(bool(active & (1 << (y * 20 + x))),
                                 1 <= y < 14 and 1 <= x < 19)
        self.assertIsNone(bitmap.encode_geometry(320, 240, encoded))

    def test_unsuitable_geometry_keeps_original_representation(self):
        self.assertIsNone(bitmap.encode_geometry(320, 240, geometry([[(0, 320, 0)]] * 240)))
        self.assertIsNone(bitmap.encode_geometry(320, 240, geometry(
            [[(x, 1, 1) for x in range(0, 320, 2)]] * 240)))
        self.assertIsNone(bitmap.encode_geometry(320, 240, geometry(
            [[(x, 1, 0) for x in range(320)]] * 240)))

    def test_malformed_masks_fail_closed(self):
        source = geometry([[(x, 1, 0) for x in range(0, 320, 2)]] * 240)
        encoded = bitmap.encode_geometry(320, 240, source)
        for at, value in ((2, encoded[2] ^ 1), (39, encoded[39] | 0x80),
                          (len(encoded) - 1, 1)):
            bad = bytearray(encoded); bad[at] = value
            with self.assertRaises(ValueError):
                bitmap.restore_geometry(320, 240, bad)
        with self.assertRaises(ValueError):
            bitmap.restore_geometry(320, 240, encoded[:9000])
        with self.assertRaises(ValueError):
            bitmap.restore_geometry(319, 240, encoded)


if __name__ == '__main__':
    unittest.main()
