"""Exact field reorder and malformed-input coverage without original assets."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from retained_planes import transform
from retained_filter import group_layout, transform_group, transform as color_transform


class RetainedPlanesTest(unittest.TestCase):
    def test_independent_layout_hidden_colors_and_empty_rows(self):
        raw = bytes.fromhex('0000 0200 0000 0200 00 ffff 1f78 0300 0200 01 3412 00 1ff8 89 0000')
        expected = bytes.fromhex('0000 0200 0000 0200 00 0300 0200 01 0000 ffff 1f78 3412 1ff8 00 89')
        self.assertEqual(transform(raw), expected)
        self.assertEqual(transform(expected, True), raw)
        for mode in (8, 9, 10):
            slices = ((0, (4 << 24) | len(raw)),)
            value = transform_group(raw, slices, mode)
            self.assertEqual(len(value), len(raw))
            self.assertEqual(transform_group(value, slices, mode, True), raw)

    def test_all_rgb565_and_alpha_values(self):
        source = bytearray()
        for y in range(240):
            source.extend(struct.pack('<HHHB', 1, 0, 320, y & 1))
            for x in range(320):
                value = (y * 320 + x) & 65535
                source.extend(struct.pack('<H', value))
                if y & 1:source.append((value * 37) & 255)
        raw = bytes(source); slices = ((0, (4 << 24) | len(raw)),)
        for mode in (8, 9, 10):
            encoded = transform_group(raw, slices, mode)
            # Independent old operation order: per-span color first, then
            # field separation. New contiguous-plane filtering must emit the
            # same bytes so existing content-addressed caches remain valid.
            expected = transform(color_transform(raw, 4, mode-8)) if mode != 8 else transform(raw)
            self.assertEqual(encoded, expected)
            self.assertEqual(transform_group(encoded, slices, mode, True), raw)
            self.assertEqual(source, raw)

    def test_group_header_version_and_modes(self):
        table = struct.pack('<II', 0, (4 << 24) | 2)
        for mode in range(3):
            data = struct.pack('<4s7I', b'RGR3', 2, 1, 14, 1, mode, 8, 0) + table + b'\0'
            self.assertEqual(group_layout(data, 43), (2, 40, 8 + mode, ((0, (4 << 24) | 2),)))
            with self.assertRaises(ValueError):group_layout(data, 42)
        data = struct.pack('<4s7I', b'RGR3', 2, 1, 14, 1, 3, 8, 0) + table + b'\0'
        with self.assertRaises(ValueError):group_layout(data, 43)

    def test_malformed_geometry(self):
        cases = [b'', b'\0', bytes(482), struct.pack('<H', 321),
                 struct.pack('<HHHB', 1, 0, 0, 0),
                 struct.pack('<HHHBH', 1, 0, 1, 2, 0),
                 struct.pack('<HHHBH', 1, 320, 1, 0, 0),
                 struct.pack('<HHHBH', 1, 0, 2, 0, 0),
                 struct.pack('<H', 2)+struct.pack('<HHBH', 1, 1, 0, 0)+struct.pack('<HHBH', 0, 1, 0, 0)]
        for raw in cases:
            for inverse in (False, True):
                with self.subTest(raw=raw, inverse=inverse):
                    with self.assertRaises(ValueError):transform(raw, inverse)


if __name__ == '__main__':unittest.main()
