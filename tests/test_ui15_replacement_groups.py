"""Lossless native word/UV payloads grouped into reused image identities."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
from patch_phobia_assets import grouped_art
from ui12_resources import replace_images, view
from test_ui14_resources import current


class ReplacementGroupTests(unittest.TestCase):
    def make_rows(self, directory):
        # Arbitrary words must survive even when they resemble RGB565 colors.
        words = struct.pack('<512H', *((0x8100 + i * 97) & 65535 for i in range(512)))
        pixels = struct.pack('<512H', *([0xf800] * 512))
        alpha = bytes((i % 19) * 13 for i in range(512))
        rows = []
        for i, (fmt, payload) in enumerate(((0, words), (1, pixels + alpha))):
            path = directory / ('native-%d.bin' % i)
            path.write_bytes(payload)
            rows.append(dict(file=str(path), width=32, height=16, format=fmt,
                bytes=len(payload), sha256=tiles.sha256(payload),
                raw_sha256=tiles.sha256(payload), preserve_payload=True))
        return rows

    def test_reused_slots_preserve_words_alpha_and_every_locale_slice(self):
        original = current('eng', 3)
        with tempfile.TemporaryDirectory() as folder:
            work = Path(folder)
            rows = self.make_rows(work)
            art = grouped_art(rows, work)
            art['image_ids'] = [0, 1]
            result, report = replace_images(original, {},
                raw_replacements=dict(enumerate(rows)), replacement_art=art)
            source, target = view(original), view(result)
            self.assertGreater(art['report']['retained_groups'], 0)
            self.assertGreater(report['replacement_grouping']['saved_common_art_bytes'], 0)
            for i, row in enumerate(rows):
                self.assertEqual(target.image_raw(i),
                    (32, 16, row['format'], Path(row['file']).read_bytes()))
            old_tiles, new_tiles = tiles.TileReader(source), tiles.TileReader(target)
            for i in range(2, len(source.images)):
                before = old_tiles.image_raw(i) if source.images[i][3] == 6 else source.image_raw(i)
                after = new_tiles.image_raw(i) if target.images[i][3] == 6 else target.image_raw(i)
                self.assertEqual(before, after)
            self.assertEqual(source.clips, target.clips)
            self.assertEqual(source.cards, target.cards)
            self.assertEqual(source.aux, target.aux)
            for i in range(len(source.glyphs)):
                self.assertEqual(source.glyph_payload(i), target.glyph_payload(i))
            for i in range(len(source.strings)):
                self.assertEqual(source.string_payload(i), target.string_payload(i))

    def test_rejects_duplicate_or_mismatched_image_identity(self):
        with tempfile.TemporaryDirectory() as folder:
            rows = self.make_rows(Path(folder))
            art = grouped_art(rows, Path(folder))
            art['image_ids'] = [0, 0]
            with self.assertRaisesRegex(ValueError, 'image identities'):
                replace_images(current(), {}, raw_replacements=dict(enumerate(rows)),
                    replacement_art=art)

    def test_rejects_descriptor_outside_group(self):
        with tempfile.TemporaryDirectory() as folder:
            rows = self.make_rows(Path(folder))
            art = grouped_art(rows, Path(folder))
            art['image_ids'] = [0, 1]
            art['entries'][0] = (len(art['payload']) + 2, 7, 16)
            with self.assertRaisesRegex(ValueError, 'descriptor'):
                replace_images(current(), {}, raw_replacements=dict(enumerate(rows)),
                    replacement_art=art)


if __name__ == '__main__':
    unittest.main()
