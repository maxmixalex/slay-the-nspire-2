"""Public synthetic RGR2 conversion, relocation and bilingual readback."""
from pathlib import Path
import random
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import compact_retained_colors as colors
import content_tiles as tiles
import repack_retained
import split_content
from retained_filter import group_layout, transform_group
from test_repack_retained import fixture, stored


def source(language='eng', glyphs=1):
    data = bytearray(fixture(language, glyphs))
    offset = tiles.IMAGE.unpack_from(data, 40)[0]
    rng = random.Random(3235)
    for pixel in range(4096):
        green = rng.randrange(64)
        red = (green // 2 + rng.randrange(3)) % 32
        blue = (green // 2 + rng.randrange(2)) % 32
        struct.pack_into('<H', data, offset + pixel * 2, red * 2048 + green * 32 + blue)
    packed, _receipt, _plan = repack_retained.repack(bytes(data), fast_images='none')
    data = bytearray(packed)
    struct.pack_into('<I', data, 4, 35)
    return bytes(data)


class RetainedColorPackTest(unittest.TestCase):
    def test_exact_conversion_relocation_and_shared_art(self):
        with tempfile.TemporaryDirectory() as directory:
            before = {language: colors.view(source(language, count))
                      for language, count in (('eng', 1), ('zhs', 3))}
            choices = colors.plan(before['eng'], Path(directory), jobs=1)
            self.assertTrue(any(choice.payload for choice in choices))
            outputs = {}
            for language, old in before.items():
                result = colors.rewrite(old, choices)
                outputs[language] = result
                proof = colors.verify(old, result)
                self.assertEqual(proof['decoded_images'], len(old.images))
                self.assertTrue(proof['all_images_decoded'])
                self.assertLess(len(result), len(old.data))
                target = colors.view(result)
                self.assertEqual(target.version, 36)
                self.assertEqual(target.images[0], target.images[1])
                self.assertNotEqual(target.glyphs[0][1], old.glyphs[0][1])
                self.assertNotEqual(target.strings[0][0], old.strings[0][0])
                for group in colors.groups(target):
                    payload = target.block(group.offset, group.size)
                    raw_size, start, mode, slices = group_layout(payload, 36)
                    if mode:
                        filtered = tiles.decode_lzma2(payload[start:], raw_size)
                        raw = transform_group(filtered, slices, mode, True)
                        self.assertEqual(transform_group(raw, slices, mode), filtered)
                # A second conversion is byte-identical and does not add a
                # second filter or a redundant table to a previously saved group.
                again = colors.plan(target, Path(directory), jobs=1)
                self.assertTrue(all(choice.payload is None for choice in again))
                self.assertEqual(colors.rewrite(target, again), result)
            common, locales, _receipt = split_content.split(outputs)
            for language in outputs:
                self.assertEqual(split_content.reconstruct(common, locales[language], language), outputs[language])
            # Cache reuse must prove decoded filtered bytes, not trust a name.
            cached = sorted(Path(directory).glob('*.lzma2'))[0]
            cached.write_bytes(b'\0')
            with self.assertRaises(ValueError):
                colors.plan(before['eng'], Path(directory), jobs=1)

    def test_requested_group_version_table_and_stream_bounds(self):
        with tempfile.TemporaryDirectory() as directory:
            old = colors.view(source())
            choices = colors.plan(old, Path(directory), jobs=1)
            output = colors.rewrite(old, choices)
            target = colors.view(output)
            group = next(group for group in colors.groups(target)
                         if target.data[group.offset:group.offset + 4] == b'RGR2')
            cases = ((4, 0), (4, tiles.GROUP_LIMIT + 1), (8, 0), (12, 13),
                     (16, 0), (16, 0x20000000), (20, 0), (20, 3),
                     (24, 0), (28, 1), (32, 1), (36, 0), (36, (5 << 24) | 8192))
            for field, value in cases:
                changed = bytearray(output)
                struct.pack_into('<I', changed, group.offset + field, value)
                with self.subTest(field=field, value=value):
                    candidate = colors.view(changed)
                    with self.assertRaises(ValueError):
                        candidate.image_raw(group.first_image)
                    with self.assertRaises(ValueError):
                        split_content.layout(changed, 'eng')
            changed = bytearray(output)
            struct.pack_into('<I', changed, 4, 35)
            with self.assertRaises(ValueError):
                colors.view(changed).image_raw(group.first_image)
            with self.assertRaises(ValueError):
                split_content.layout(changed, 'eng')

    def test_catalog_mismatch_and_relocation_into_replaced_bytes_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            old = colors.view(source())
            choices = colors.plan(old, Path(directory), jobs=1)
            changed = bytearray(old.data)
            group = colors.groups(old)[0]
            changed[group.offset + 20] ^= 1
            with self.assertRaisesRegex(ValueError, 'Bilingual retained group'):
                colors.rewrite(colors.view(changed), choices)
            relocate = colors.Relocation([(10, 30, bytes(4)), (50, 70, bytes(6))])
            self.assertEqual([relocate(value) for value in (0, 10, 30, 49, 50, 70)],
                             [0, 10, 14, 33, 34, 40])
            for value in (11, 29, 51, 69):
                with self.assertRaises(ValueError):
                    relocate(value)

    def test_grouped_glyph_and_text_relative_links_remain_exact(self):
        old = colors.view(source())
        data = bytearray(old.data)

        def append(payload):
            data.extend(bytes(len(data) & 1))
            offset = len(data)
            data.extend(payload)
            return offset

        def group(raw):
            stream = stored(raw)
            payload = tiles.GROUP.pack(b'RGR1', len(raw), len(stream), 14, 1, 0, 0, 0) + stream
            at = append(payload)
            descriptor = colors.aligned(len(data))
            return append(struct.pack('<iIII', at - descriptor, len(payload), 0, len(raw)))

        glyph = old.glyph_payload(0)
        descriptor = group(glyph)
        row = old.glyphs[0]
        tiles.GLYPH.pack_into(data, old.glyph_at, row[0], descriptor, *row[2:6], row[6] | tiles.GLYPH_GROUP_FLAG)
        text = b'Exact grouped string: RGB565 alpha and all links stay intact.\0'
        descriptor = group(text)
        struct.pack_into('<II', data, old.string_at, descriptor, (len(text) - 1) | tiles.STRING_GROUP_FLAG)
        struct.pack_into('<I', data, 12, len(data))
        source_view = colors.view(data)
        with tempfile.TemporaryDirectory() as directory:
            choices = colors.plan(source_view, Path(directory), jobs=1)
            output = colors.rewrite(source_view, choices)
            colors.verify(source_view, output)
            target = colors.view(output)
            self.assertEqual(target.glyph_payload(0), glyph)
            self.assertEqual(target.string_payload(0), text)
            self.assertTrue(target.glyphs[0][6] & tiles.GLYPH_GROUP_FLAG)
            self.assertTrue(target.strings[0][1] & tiles.STRING_GROUP_FLAG)
            split_content.layout(output, 'eng')


if __name__ == '__main__':
    unittest.main()
