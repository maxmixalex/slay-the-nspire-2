"""Independent byte/pixel contracts for the private glyph-locality converter."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import reorder_glyph_payloads as locality


def fixture():
    sizes, chars = (8, 10, 12, 16), (32, 65, 66, 0x4e2d, 0x5b57)
    ni, ng, ns = 2, len(sizes) * len(chars), 3
    glyph_at = 40 + ni * 16
    string_at = glyph_at + ng * 16
    out = bytearray(string_at + ns * 8 + 24)

    def append(payload):
        out.extend(bytes(len(out) & 1))
        offset = len(out); out.extend(payload)
        return offset

    image = bytes.fromhex('1234abcd5678ff00')
    image_at = append(image)
    for index in range(ni):
        struct.pack_into('<IHHII', out, 40 + index * 16, image_at, 2, 2, 0, len(image))
    # Transparent raw glyphs have an odd length and alias across sizes. Other
    # characters alternate raw and run encoding, with distinct coverage bytes.
    shared = None
    for index, (size, cp) in enumerate((size, cp) for size in sizes for cp in chars):
        if cp == 32:
            raw, encoded, width, height = b'\0\0\0', 0, 1, 3
            if shared is None: shared = append(raw)
            offset = shared
        else:
            coverage = (size * 13 + cp * 7) & 255
            width, height = 2, 3
            raw, encoded = (bytes((2, coverage, 4, 255 - coverage)), 4) if index & 1 else (bytes((coverage,)) * 6, 0)
            offset = append(raw)
        struct.pack_into('<IIHBBHH', out, glyph_at + index * 16,
                         cp, offset, size, width, height, width + 1, encoded)
    # Chinese character frequency must count codepoints, not each UTF-8 byte.
    texts = ('字' * 18 + '中中AB', '字' * 9 + '中', 'eng')
    for index, text in enumerate(texts):
        raw = text.encode(); offset = append(raw + b'\0')
        struct.pack_into('<II', out, string_at + index * 8, offset, len(raw))
    struct.pack_into('<4s9I', out, 0, b'NSP2', 22, 0x41cef1ea, len(out), ni, ng, ns, 1, 1, 0)
    return bytes(out)


def pixels(data, row):
    cp, offset, size, width, height, advance, encoded = row
    if not encoded:
        coverage = data[offset:offset + width * height]
    else:
        coverage = bytearray()
        for at in range(offset, offset + encoded, 2):
            coverage.extend(bytes((data[at + 1],)) * data[at])
        coverage = bytes(coverage)
    if len(coverage) != width * height:
        raise AssertionError('Malformed synthetic glyph')
    # Independently composite antialias coverage onto several RGB565 colors.
    composited = []
    for paper, ink in ((0, 0xffff), (0x31a6, 0xd6ba), (0xffff, 0)):
        for alpha in coverage:
            r = (((paper >> 11) * (255 - alpha) + (ink >> 11) * alpha + 127) // 255)
            g = ((((paper >> 5) & 63) * (255 - alpha) + ((ink >> 5) & 63) * alpha + 127) // 255)
            b = (((paper & 31) * (255 - alpha) + (ink & 31) * alpha + 127) // 255)
            composited.append((r << 11) | (g << 5) | b)
    return cp, size, width, height, advance, composited


class GlyphLocalityTest(unittest.TestCase):
    def test_exact_geometry_pixels_and_non_glyph_sections(self):
        source = fixture()
        for order in locality.ORDERS:
            with self.subTest(order=order):
                result, receipt = locality.reorder(source, order)
                before, after = locality.layout(source), locality.layout(result)
                self.assertEqual(len(result), len(source))
                self.assertEqual(source[:before.glyph_at], result[:after.glyph_at])
                self.assertEqual(source[before.glyph_at + len(before.glyphs) * 16:before.first],
                                 result[after.glyph_at + len(after.glyphs) * 16:after.first])
                self.assertEqual(source[before.last:], result[after.last:])
                self.assertEqual([pixels(source, row) for row in before.glyphs],
                                 [pixels(result, row) for row in after.glyphs])
                self.assertTrue(receipt['verification']['non_glyph_bytes_exact'])
                self.assertEqual(receipt['installation_growth_bytes'], 0)
                self.assertEqual(receipt['runtime_buffer_growth_bytes'], 0)
                self.assertGreater(receipt['relocated_glyph_entries'], 0)

    def test_frequency_keeps_native_sizes_together_and_aliases_shared(self):
        source = fixture(); result, _ = locality.reorder(source)
        after = locality.layout(result)
        common = sorted((row[1], row[2]) for row in after.glyphs if row[0] == 0x5b57)
        self.assertEqual(common[0][0], after.first)
        self.assertEqual([size for _, size in common], [8, 10, 12, 16])
        common_end = max(row[1] + (row[6] or row[3] * row[4]) for row in after.glyphs if row[0] == 0x5b57)
        self.assertTrue(all(row[1] >= common_end for row in after.glyphs if row[0] != 0x5b57))
        self.assertEqual(len({row[1] for row in after.glyphs if row[0] == 32}), 1)
        self.assertEqual(locality.reorder(result)[0], result)

    def test_size_frequency_does_not_interleave_other_sizes(self):
        source = fixture(); result, _ = locality.reorder(source, 'size-frequency')
        after = locality.layout(result)
        # The shared transparent space deliberately stays one alias. Every
        # other physical unit remains in its native-size frequency cohort.
        physical = sorted((row[1], row[2], row[0]) for row in after.glyphs if row[0] != 32)
        self.assertEqual([size for _, size, _ in physical], sorted(size for _, size, _ in physical))
        for size in (8, 10, 12, 16):
            self.assertEqual([cp for _, px, cp in physical if px == size], [0x5b57, 0x4e2d, 65, 66])
        self.assertEqual(len({row[1] for row in after.glyphs if row[0] == 32}), 1)
        self.assertEqual(locality.reorder(result, 'size-frequency')[0], result)
        self.assertNotEqual(locality.reorder(source)[0], result)

    def test_rejects_unknown_layout_and_detects_pixel_or_art_corruption(self):
        source = fixture(); before = locality.layout(source)
        broken = bytearray(source)
        struct.pack_into('<I', broken, before.glyph_at + 4, before.first + 2)
        with self.assertRaises(ValueError): locality.reorder(broken)
        result, _ = locality.reorder(source); after = locality.layout(result)
        for position in (after.first, 40, after.last):
            broken = bytearray(result); broken[position] ^= 1
            with self.assertRaises(ValueError): locality.verify(source, broken)
        broken = bytearray(source); struct.pack_into('<I', broken, 4, 19)
        with self.assertRaises(ValueError): locality.reorder(broken)


if __name__ == '__main__':
    unittest.main()
