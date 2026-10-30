"""Exact TGI2 indices, immutable pixel streams, relocation and version gates."""
from pathlib import Path
import random
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as t
import predict_index_resources as prediction
import repack_retained as retained
import split_content as split
from tile_prediction import index_transform
from test_repack_retained import fixture
from test_compact_tile_resources import grouped_text


class IndexPredictionTest(unittest.TestCase):
    def test_frame_changes_aliases_unaligned_geometry_and_large_ids(self):
        randomizer = random.Random(61781)
        for counts in ((1,), (300,)*64, (3,3,7,1,300,300,2)):
            raw = bytearray()
            for frame, count in enumerate(counts):
                ids = [randomizer.randrange(65535*300) for _ in range(count)]
                geometry = randomizer.randbytes(frame % 17 + 1)
                raw.extend(t.FRAME.pack(600000, count*4, len(geometry)))
                raw.extend(struct.pack('<'+'I'*count, *ids));raw.extend(geometry)
            encoded = index_transform(raw, len(counts), 3)
            self.assertEqual(len(encoded), len(raw))
            self.assertEqual(index_transform(encoded, len(counts), 3, True), raw)

    def convert(self, raw, cache=None):
        plan = prediction.make_plan(prediction.view(raw), workers=1, cache=cache)
        return prediction.rewrite(raw, plan)

    def assert_same(self, original, converted):
        before, after = prediction.view(original), prediction.view(converted)
        old, new = t.TileReader(before), t.TileReader(after)
        self.assertEqual(after.version, 41)
        for i, row in enumerate(before.images):
            self.assertEqual(old.image_raw(i) if row[3] == 6 else before.image_raw(i),
                             new.image_raw(i) if row[3] == 6 else after.image_raw(i))
        for i in range(len(before.glyphs)):
            self.assertEqual(before.glyph_alpha(i), after.glyph_alpha(i))
        for i in range(len(before.strings)):
            self.assertEqual(before.string_payload(i), after.string_payload(i))
        self.assertEqual(before.clips, after.clips)

    def test_full_converter_and_idempotency(self):
        raw, _, _ = retained.repack(fixture(), fast_images='none')
        result, receipt = self.convert(raw)
        self.assertGreater(receipt['saved_bytes'], 0)
        self.assertEqual(receipt['changed_index_groups'], 1)
        self.assertEqual(receipt['resident_bytes_added'], 0)
        self.assert_same(raw, result)
        self.assertEqual(self.convert(result)[0], result)

    def test_grouped_glyph_text_and_bilingual_split(self):
        outputs = {}
        for code, glyphs in (('eng',1), ('zhs',3)):
            raw, _, _ = retained.repack(fixture(code, glyphs=glyphs), fast_images='none')
            raw = grouped_text(raw)
            result, _ = self.convert(raw)
            self.assert_same(raw, result); outputs[code] = result
        common, locales, _ = split.split(outputs, compress_directory=True)
        for code in outputs:self.assertEqual(split.reconstruct(common, locales[code], code), outputs[code])

    def test_corrupt_cached_stream_is_not_accepted(self):
        raw, _, _ = retained.repack(fixture(), fast_images='none')
        with tempfile.TemporaryDirectory() as folder:
            self.convert(raw, folder)
            paths = list(Path(folder).glob('*.lzma2')); self.assertTrue(paths)
            paths[0].write_bytes(b'invalid')
            with self.assertRaises(ValueError):self.convert(raw, folder)

    def test_version_kind_mode_and_reconstructed_tile_bounds(self):
        raw, _, _ = retained.repack(fixture(), fast_images='none')
        result, _ = self.convert(raw)
        source = prediction.view(result); reader = t.TileReader(source)
        at = reader.groups[reader.tile_groups][0]-32
        for field, value in ((4,40),(at+4,1),(at+28,2),(at+24,0),(at+8,0)):
            bad = bytearray(result); struct.pack_into('<I', bad, field, value)
            with self.assertRaises(ValueError):t.TileReader(prediction.view(bad))
        bad = bytearray(result)
        first = reader.groups[0][0]-32
        struct.pack_into('<I', bad, first, int.from_bytes(b'TGI2','little'))
        struct.pack_into('<I', bad, first+28, 1)
        with self.assertRaises(ValueError):t.TileReader(prediction.view(bad))

    def test_invalid_record_lengths_and_trailing_bytes(self):
        raw = t.FRAME.pack(2,4,2)+bytes(6)
        for count in (0,65):
            with self.assertRaises(ValueError):index_transform(raw,count,3)
        for field, value in ((0,0),(0,614881),(4,0),(4,1204),(4,3),(8,0),(8,3)):
            bad = bytearray(raw);struct.pack_into('<I',bad,field,value)
            with self.assertRaises(ValueError):index_transform(bad,1,3)
        with self.assertRaises(ValueError):index_transform(raw+b'x',1,3)
        with self.assertRaises(ValueError):index_transform(raw[:-1],1,3)


if __name__ == '__main__':unittest.main()
