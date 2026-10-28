"""Byte-exact TGR2 coverage, relocation and compatibility with grouped text."""
from pathlib import Path
import lzma
import struct
import sys
import tempfile
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import compact_tile_resources as compact
import content_tiles as tiles
import repack_retained as retained
import split_content as split
from tile_filter import filter_tiles, restore_tiles
from test_repack_retained import fixture
from test_compact_ui6_merchant import source_pack


def grouped_text(data):
    source = compact.view(data)
    out = bytearray(data)
    def append(payload):
        out.extend(bytes(len(out) & 1));at = len(out);out.extend(payload);return at
    def group(payload, count):
        encoded = lzma.compress(payload, format=lzma.FORMAT_RAW,
            filters=[dict(id=lzma.FILTER_LZMA2, dict_size=524288)])
        block = tiles.GROUP.pack(b'RGR1', len(payload), len(encoded), 14, count, 0, 0, 0) + encoded
        return append(block), len(block)
    raw = b''.join(source.glyph_payload(i) for i in range(len(source.glyphs)))
    start, length = group(raw, len(source.glyphs))
    raw_at = 0
    for i, old in enumerate(source.glyphs):
        size = len(source.glyph_payload(i));at = (len(out) + 1) & ~1
        append(compact.SLICE.pack(start - at, length, raw_at, size));raw_at += size
        tiles.GLYPH.pack_into(out, source.glyph_at + i * 16, old[0], at, *old[2:6], old[6] | 0x8000)
    payload = source.string_payload(0)
    start, length = group(payload, 1)
    at = (len(out) + 1) & ~1
    append(compact.SLICE.pack(start - at, length, 0, len(payload)))
    marker = append(source.string_payload(split.MARKER))
    for i in range(len(source.strings)):
        struct.pack_into('<II', out, source.string_at + i * 8,
            marker if i == split.MARKER else at, len(payload) - 1 | (0 if i == split.MARKER else 0x80000000))
    struct.pack_into('<I', out, 4, 33);struct.pack_into('<I', out, 12, len(out))
    return bytes(out)


class CompactTilesTest(unittest.TestCase):
    def test_all_colors_and_full_group_restore_every_byte(self):
        for count in (1, 256, 680):
            raw = np.zeros((count, 770), dtype='u1')
            raw[:, 0] = np.arange(count) % 16 + 1
            raw[:, 1] = np.arange(count) // 16 % 16 + 1
            colors = np.arange(count * 256, dtype='u2').reshape(count, 256)
            raw[:, 2:514] = colors.astype('<u2').view('u1').reshape(count, 512)
            raw[:, 514:] = (np.arange(count * 256) * 13).astype('u1').reshape(count, 256)
            for mode in (1, 2):
                self.assertEqual(restore_tiles(filter_tiles(raw.tobytes(), mode), mode), raw.tobytes())
        for data, mode in ((b'', 1), (bytes(770), 1), (bytes(771), 1), (bytes(770 * 681), 1), (b'\x10\x10' + bytes(768), 3)):
            with self.assertRaises(ValueError):
                filter_tiles(data, mode)

    def exact(self, before, after):
        source, target = compact.view(before), compact.view(after)
        old, new = tiles.TileReader(source), tiles.TileReader(target)
        for i, image in enumerate(source.images):
            self.assertEqual(old.image_raw(i) if image[3] == 6 else source.image_raw(i),
                             new.image_raw(i) if image[3] == 6 else target.image_raw(i))
        for i in range(len(source.glyphs)):
            self.assertEqual(source.glyph_alpha(i), target.glyph_alpha(i))
        for i in range(len(source.strings)):
            self.assertEqual(source.string_payload(i), target.string_payload(i))

    def test_conversion_is_exact_and_repeatable(self):
        original, _, _ = retained.repack(fixture(), fast_images='none')
        result, receipt = compact.repack(original, workers=1)
        self.assertGreater(receipt['saved_bytes'], 0)
        self.assertEqual(receipt['improved_groups'], 1)
        self.assertTrue(receipt['no_resident_buffer_added'])
        self.exact(original, result)
        self.assertEqual(compact.repack(result, workers=1)[0], result)

    def test_grouped_glyph_string_and_forward_retained_relocation(self):
        original = grouped_text(source_pack(forward=True))
        result, receipt = compact.repack(original, workers=1)
        self.assertGreater(receipt['saved_bytes'], 0)
        self.exact(original, result)
        self.assertTrue(any(row[6] & 0x8000 for row in compact.view(result).glyphs))
        self.assertTrue(any(row[1] & 0x80000000 for row in compact.view(result).strings))

    def test_bilingual_split_and_corrupt_encoder_cache(self):
        outputs = {}
        with tempfile.TemporaryDirectory() as directory:
            for language, count in (('eng', 1), ('zhs', 3)):
                original, _, _ = retained.repack(fixture(language, glyphs=count), fast_images='none')
                result, _ = compact.repack(grouped_text(original), workers=1, cache_directory=directory)
                outputs[language] = result
            common, localized, _ = split.split(outputs)
            for language in outputs:
                self.assertEqual(split.reconstruct(common, localized[language], language), outputs[language])
            cache = list(Path(directory).glob('*.bin'));self.assertTrue(cache)
            for path in cache:path.write_bytes(b'broken')
            repeated, _ = compact.repack(grouped_text(original), workers=1, cache_directory=directory)
            self.assertEqual(repeated, outputs['zhs'])

    def test_old_version_unknown_mode_and_index_filter_are_rejected(self):
        original, _, _ = retained.repack(fixture(), fast_images='none')
        result, _ = compact.repack(original, workers=1)
        source = compact.view(result);reader = tiles.TileReader(source)
        offset = reader.groups[0][0] - 32
        for at, value in ((4, 32), (offset + 28, 3), (reader.groups[1][0] - 32, int.from_bytes(b'TGR2', 'little'))):
            invalid = bytearray(result);struct.pack_into('<I', invalid, at, value)
            with self.assertRaises(ValueError):
                tiles.TileReader(compact.view(invalid))


if __name__ == '__main__':
    unittest.main()
