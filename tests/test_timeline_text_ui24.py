"""Locale-tail relocation preserves compressed artwork and existing text/font data."""
import json
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch
from types import SimpleNamespace
from PIL import ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import timeline_text_ui24 as timeline
import split_content as split
from test_ftue_assets import fixture
import group_strings


def current(code='eng'):
    return group_strings.repack(fixture(code))[0]


class TimelineTextTest(unittest.TestCase):
    def test_real_contract_counts_and_order(self):
        spec, _ = timeline.contract()
        self.assertEqual(len(spec['strings']), 257)
        self.assertEqual(spec['strings'][0]['key'], spec['epoch_order'][0]+'.title')
        self.assertEqual(spec['strings'][228]['key'], 'STORY_DEFECT')
        wanted = timeline.glyph_requirements(['']*257, 'zhs')
        self.assertIn((ord('获'), 10, 0), wanted)
        self.assertIn((ord('线'), 12, 0), wanted)

    def test_new_font_glyph_keeps_the_native_rle_payload_contract(self):
        font = SimpleNamespace(fonts={10: ImageFont.load_default(size=10)}, coverage={ord('A')})
        cp, band, width, height, advance, encoded, raw = timeline.original_glyph(font, ord('A'), 'A', 10, 0)
        self.assertEqual((cp, band), (ord('A'), 10))
        self.assertIsInstance(raw, bytes)
        self.assertIsInstance(encoded, int)
        alpha = bytes(byte for i in range(0, len(raw), 2) for byte in [raw[i+1]]*raw[i]) if encoded else raw
        self.assertEqual(len(alpha), width*height)
        self.assertEqual(advance & 4095, round(font.fonts[10].getlength('A')*64))

    def test_grouped_tail_old_payloads_and_common_survive_added_directory(self):
        common = None
        for code in ('eng', 'zhs'):
            data = bytearray(current(code)); struct.pack_into('<I', data, 4, 46)
            data[36:40] = b'NAF1'; data = bytes(data)
            texts = [('Timeline\n原文 [gold]'+str(i)+'[/gold] ')*4 for i in range(257)]
            additions = [(0x10e000, 10, 2, 2, 0x8000 | 128, 0, b'\x00\x40\x80\xff')]
            with patch.object(timeline, 'BASE_STRINGS', 8192):
                output, report = timeline.append_records(data, texts, additions)
                proof = timeline.verify(data, output, texts, {(0x10e000, 10, 0)})
            target = timeline.view(output)
            self.assertEqual(len(target.strings), 8192+257)
            self.assertEqual(output[36:40], b'NAF1')
            self.assertGreater(report['groups'], 0)
            self.assertEqual(proof['existing_strings_exact'], 8192)
            before = timeline.view(data)
            old_tiles, new_tiles = timeline.t.TileReader(before), timeline.t.TileReader(target)
            for i, row in enumerate(before.images):
                self.assertEqual(old_tiles.image_raw(i) if row[3] == 6 else before.image_raw(i),
                                 new_tiles.image_raw(i) if row[3] == 6 else target.image_raw(i))
            bounds = split.layout(output, code)
            actual = split.common_file(output[bounds['prefix']:bounds['prefix']+bounds['artwork']])
            if common is None:
                common = actual
            self.assertEqual(common, actual)
            leaf = split.compress_language_directory(split.language_file(output, bounds, code, common[32:64]))
            self.assertEqual(split.reconstruct(common, leaf, code), output)

    def test_source_locale_fallback_and_optional_empty_are_explicit(self):
        tables = {
            'localization/eng/epochs.json': {'a': 'English', 'b': 'Fallback'},
            'localization/zhs/epochs.json': {'a': '中文'},
        }
        class Pack:
            def read(self, name):
                return json.dumps(tables[name]).encode()
        rows = [dict(table='epochs', key=key, optional=key == 'c') for key in ('a', 'b', 'c')]
        values, proof = timeline.original_texts(Pack(), 'zhs', rows)
        self.assertEqual(values, ['中文', 'Fallback', ''])
        self.assertEqual(proof['english_fallback_slots'], [8705])
        self.assertEqual(proof['optional_absent_slots'], [8706])
        with self.assertRaisesRegex(ValueError, 'Missing or invalid'):
            timeline.original_texts(Pack(), 'zhs', [dict(table='epochs', key='missing')])

    def test_combining_sequences_preserve_exact_original_utf8_and_keys(self):
        original = ['e\u0301 [gold]กำลัง[/gold]', 'e\u0301']
        shaped, inverse = timeline.shape_texts(original, {0xf0000, 0x10f000})
        self.assertTrue(inverse)
        self.assertEqual([''.join(inverse.get(ord(c), c) for c in text) for text in shaped], original)
        self.assertEqual(shaped[0][0], shaped[1])
        self.assertTrue(all(0x10e000 <= cp < 0x10f000 for cp in inverse))
        with self.assertRaisesRegex(ValueError, 'collides'):
            timeline.shape_texts(original, {0x10e000})

    def test_reject_duplicate_glyph_and_repeated_append(self):
        data = bytearray(current()); struct.pack_into('<I', data, 4, 46); data = bytes(data)
        source = timeline.view(data); cp, _, band, w, h, advance, encoded = source.glyphs[0]
        existing = (cp, band, w, h, advance, encoded & 0x7fff, bytes(source.glyph_payload(0)))
        with patch.object(timeline, 'BASE_STRINGS', 8192):
            with self.assertRaisesRegex(ValueError, 'new and unique'):
                timeline.append_records(data, ['']*257, [existing])
            output, _ = timeline.append_records(data, ['']*257, [])
            with self.assertRaisesRegex(ValueError, 'unchanged'):
                timeline.append_records(output, ['']*257, [])


if __name__ == '__main__':
    unittest.main()
