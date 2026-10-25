"""Exact retained color transforms, without source artwork or pack mutation."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from retained_filter import MAX_IMAGE_BYTES, transform


def scalar(color, mode):
    red, green, blue = color >> 11, (color >> 5) & 63, color & 31
    differences = [(channel - green // 2) % 32 for channel in (red, blue)]
    if mode == 2:
        differences = [2 * d if d < 16 else 2 * (32 - d) - 1 for d in differences]
    return green + 64 * differences[0] + 2048 * differences[1]


def sparse():
    # Empty row, mixed disjoint opaque/alpha spans, and a fully opaque row.
    raw = bytearray(struct.pack('<H', 0))
    raw.extend(struct.pack('<H', 2))
    raw.extend(struct.pack('<HHBHH', 0, 2, 0, 0xffff, 0x781f))
    raw.extend(struct.pack('<HHB', 3, 2, 1))
    raw.extend(struct.pack('<HBHB', 0x1234, 0, 0xf81f, 137))
    raw.extend(struct.pack('<HHHB', 1, 0, 5, 0))
    raw.extend(struct.pack('<5H', 0, 1, 0x7e0, 0xffdf, 0xffff))
    return bytes(raw), ((9, 2, 2), (18, 2, 3), (31, 5, 2))


class RetainedFilterTest(unittest.TestCase):
    def test_every_rgb565_color_matches_independent_oracle(self):
        raw = struct.pack('<65536H', *range(65536))
        for mode in (1, 2):
            encoded = transform(raw, 0, mode)
            values = struct.unpack('<65536H', encoded)
            self.assertEqual(values, tuple(scalar(c, mode) for c in range(65536)))
            self.assertEqual(len(set(values)), 65536)
            self.assertEqual(transform(encoded, 0, mode, True), raw)
            self.assertEqual(transform(transform(raw, 0, mode, True), 0, mode), raw)

    def test_rgb_alpha_interleaving_and_readonly_input(self):
        raw = b''.join(struct.pack('<HB', color, (color * 37) & 255) for color in range(65536))
        for mode in (1, 2):
            source = bytearray(raw)
            encoded = transform(memoryview(source), 1, mode)
            self.assertEqual(source, raw)
            self.assertEqual(encoded[2::3], raw[2::3])
            for at in range(0, len(raw), 3):
                self.assertEqual(struct.unpack_from('<H', encoded, at)[0], scalar(at // 3, mode))
            self.assertEqual(transform(encoded, 1, mode, True), raw)

    def test_palette_only_and_all_palette_colors(self):
        for fmt in (2, 3):
            indices = bytes(range(256)) if fmt == 2 else b''.join(bytes((n, 255 - n)) for n in range(256))
            for page in range(256):
                palette = struct.pack('<256H', *(page * 256 + i for i in range(256)))
                raw = palette + indices
                for mode in (1, 2):
                    encoded = transform(raw, fmt, mode, width=16, height=16)
                    self.assertEqual(encoded[512:], indices)
                    self.assertEqual(encoded[:512], struct.pack('<256H', *(scalar(page * 256 + i, mode) for i in range(256))))
                    self.assertEqual(transform(encoded, fmt, mode, True, width=16, height=16), raw)

    def test_sparse_geometry_alpha_and_empty_rows_stay_exact(self):
        raw, ranges = sparse()
        color_bytes = {at + i * stride + byte for at, count, stride in ranges for i in range(count) for byte in (0, 1)}
        for mode in (1, 2):
            encoded = transform(raw, 4, mode, width=5, height=3)
            self.assertEqual(len(encoded), len(raw))
            for i in range(len(raw)):
                if i not in color_bytes:
                    self.assertEqual(encoded[i], raw[i])
            for at, count, stride in ranges:
                for i in range(count):
                    original, = struct.unpack_from('<H', raw, at + i * stride)
                    filtered, = struct.unpack_from('<H', encoded, at + i * stride)
                    self.assertEqual(filtered, scalar(original, mode))
            self.assertEqual(transform(encoded, 4, mode, True, width=5, height=3), raw)
            self.assertEqual(transform(raw, 4, mode), encoded)
            self.assertEqual(transform(bytes(480), 4, mode, width=320, height=240), bytes(480))

    def test_shape_mode_and_dimensions_rejected_both_directions(self):
        cases = [(b'', 0, 1, {}), (bytes(MAX_IMAGE_BYTES + 1), 0, 1, {}),
                 (b'abc', 0, 1, {}), (b'ab', 1, 1, {}), (bytes(512), 2, 1, {}),
                 (bytes(513), 3, 1, {}), (b'ab', 5, 1, {}), (b'ab', 0, 0, {}),
                 (b'ab', 0, 3, {}), (b'ab', 0.0, 1, {}), (b'ab', 0, 1.0, {}),
                 (b'ab', 0, 1, dict(width=1)),
                 (b'ab', 0, 1, dict(width=1, height=0)),
                 (b'ab', 0, 1, dict(width=321, height=1)),
                 (b'ab', 0, 1, dict(width=1, height=241)),
                 (b'ab', 0, 1, dict(width=1.0, height=1)),
                 (b'ab', 0, 1, dict(width=2, height=1))]
        for data, fmt, mode, dimensions in cases:
            for inverse in (False, True):
                with self.subTest(fmt=fmt, mode=mode, length=len(data), dimensions=dimensions, inverse=inverse):
                    source = bytearray(data)
                    with self.assertRaises(ValueError):
                        transform(source, fmt, mode, inverse, **dimensions)
                    self.assertEqual(source, data)

    def test_malformed_sparse_is_rejected_without_touching_input(self):
        row = struct.pack('<HHHBH', 1, 0, 1, 0, 0x1234)
        cases = [b'\0', struct.pack('<H', 4), row[:6], row[:-1],
                 struct.pack('<HHHB', 1, 0, 0, 0),
                 struct.pack('<HHHBH', 1, 0, 1, 2, 0),
                 struct.pack('<HHHBH', 1, 3, 1, 0, 0),
                 struct.pack('<HHHBHHHBH', 2, 1, 1, 0, 0, 0, 1, 0, 0),
                 row + b'\0', row + b'\0\0']
        for raw in cases:
            for inverse in (False, True):
                source = bytearray(raw)
                with self.subTest(raw=raw, inverse=inverse):
                    with self.assertRaises(ValueError):
                        transform(source, 4, 1, inverse, width=3, height=1)
                    self.assertEqual(source, raw)
        with self.assertRaises(ValueError):
            transform(row, 4, 1, width=3, height=2)
        with self.assertRaises(ValueError):
            transform(bytes(482), 4, 1)


if __name__ == '__main__':
    unittest.main()
