"""Relocation, source-payload immutability and UI shader boundary checks."""
from pathlib import Path
import struct
import re
import sys
from tempfile import TemporaryDirectory
import unittest

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
from append_native_ui import append_skins, reference_hsv, missing_text_glyphs
from native_ui_profile import TEXTS, BASE_STRING_COUNT, STRING_COUNT
from test_content_tiles import fixture


class NativeUiPack(unittest.TestCase):
    def test_offset_relocation_preserves_every_existing_frame_and_text(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            source, tiled = root / 'source', root / 'tiled'
            original = fixture(extra=7, shared_retained=True)
            source.write_bytes(original)
            tiles.repack(source, tiled, root / 'cache', auxiliary_count=1,
                         first_clip=1, tiles_per_group=1, workers=1)
            baseline = tiled.read_bytes()
            skins = [(2, 2, 1, bytes(range(12))), (1, 1, 0, b'\x34\x12')]
            texts = ['Synthetic orb description.\nSecond line.', 'Synthetic \u03a9 title']
            result, receipt = append_skins(baseline, 1, skins, texts=texts)
            old = tiles.PackView(baseline, 1, versions=(15,))
            new = tiles.PackView(result, 3, versions=(16,))
            old_reader, new_reader = tiles.TileReader(old), tiles.TileReader(new)
            for i in range(len(old.images)):
                a = old_reader.image_raw(i) if old.images[i][3] == 6 else old.image_raw(i)
                b = new_reader.image_raw(i) if new.images[i][3] == 6 else new.image_raw(i)
                self.assertEqual(a, b)
            self.assertEqual(receipt['added_bytes'], len(result) - len(baseline))
            self.assertEqual(tiled.read_bytes(), baseline)
            self.assertEqual(new.clips, old.clips)
            self.assertEqual(new.aux, [0, len(old.images), len(old.images) + 1])
            self.assertEqual(new.counts[2], old.counts[2] + len(texts))
            self.assertEqual(receipt['appended_strings'], len(texts))
            for (off, length), (old_off, _) in zip(new.strings, old.strings):
                self.assertEqual(result[off:off + length + 1], baseline[old_off:old_off + length + 1])
            for text, (off, length) in zip(texts, new.strings[len(old.strings):]):
                self.assertEqual(result[off:off + length + 1], text.encode('utf-8') + b'\0')
            image_only, image_receipt = append_skins(baseline, 1, skins)
            previous = tiles.PackView(image_only, 3, versions=(16,))
            self.assertEqual(previous.counts[2], old.counts[2])
            self.assertEqual(image_receipt['appended_strings'], 0)
            # New directory entries relocate every old pointer by exactly
            # eight bytes each, independent of text payload byte length.
            for image, old_image in zip(new.images[:len(old.images)], previous.images[:len(old.images)]):
                self.assertEqual(image[0] - old_image[0], len(texts) * 8)
                self.assertEqual(image[1:], old_image[1:])
            for bad in ([], [(321, 1, 0, b'x')], [(1, 1, 4, b'\0\0')], [(2, 2, 1, b'x')]):
                with self.assertRaises(ValueError):
                    append_skins(baseline, 1, bad)
            with self.assertRaises(ValueError):
                append_skins(result, 3, skins)
            for bad_texts in ([''], ['a\0b'], ['x' * tiles.STRING_LIMIT], [b'not text'], ['x'] * 257):
                with self.assertRaises(ValueError):
                    append_skins(baseline, 1, skins, texts=bad_texts)

    def test_appended_text_key_order_matches_runtime(self):
        source = (Path(__file__).resolve().parents[1] / 'src/native_ui_text_index.inc').read_text()
        entries = re.findall(r'\{"([^"]+)",WorldStringCount\+(\d+)\}', source)
        keys = [table + '.' + key for table, key in TEXTS]
        self.assertEqual(entries, [(key, str(i)) for i, key in enumerate(keys)])
        self.assertEqual(keys, sorted(set(keys)))
        self.assertEqual(STRING_COUNT, BASE_STRING_COUNT + len(keys))

    def test_glyph_coverage_reports_each_missing_size(self):
        data = fixture()
        rows = missing_text_glyphs(data, ['\n\t ', '\U0001f9ee'], sizes=(8, 10, 12, 16))
        self.assertEqual(rows, [('U+1F9EE', size) for size in (8, 10, 12, 16)])

    def test_yiq_identity_alpha_and_saturation(self):
        rgba = np.array([[[255, 0, 0, 7], [0, 255, 0, 127], [3, 79, 212, 0]]], dtype=np.uint8)
        image = Image.fromarray(rgba, 'RGBA')
        self.assertTrue(np.array_equal(np.asarray(reference_hsv(image, 1, 1, 1)), rgba))
        grey = np.asarray(reference_hsv(image, 1, 0, 1))
        self.assertTrue(np.array_equal(grey[:, :, 3], rgba[:, :, 3]))
        self.assertTrue(np.max(np.abs(grey[:, :, 0].astype(int) - grey[:, :, 1])) <= 1)
        self.assertTrue(np.max(np.abs(grey[:, :, 1].astype(int) - grey[:, :, 2])) <= 1)
        self.assertTrue(np.all(np.asarray(reference_hsv(image, 1, 1, 0))[:, :, :3] == 0))


if __name__ == '__main__':
    unittest.main()
