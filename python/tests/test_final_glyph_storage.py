"""Raw glyph representation keeps native coverage and cold-read bounds exact."""

import struct
import tempfile
import unittest
from pathlib import Path

from nspire_converter import final_glyph_storage as final
from test_glyph_groups import current


class FinalGlyphStorageTest(unittest.TestCase):
    def source(self, language="eng", reduced=False):
        raw = bytearray(current(language))
        struct.pack_into("<I", raw, 4, 46)
        if reduced:
            raw[36:40] = b"NAF1"
        return bytes(raw)

    def test_actual_rle_raw_aliases_and_fractional_metrics_survive(self):
        for language in ("eng", "zhs"):
            data = self.source(language)
            source = final.view(data)
            with tempfile.TemporaryDirectory() as cache:
                output, proof = final.convert(data, cache, 1)
                again, repeated = final.convert(data, cache, 2)
            self.assertEqual(output, again)
            self.assertEqual(proof["verification"], repeated["verification"])
            self.assertTrue(proof["verification"]["glyph_keys_metrics_coverage_exact"])
            self.assertEqual(final.view(output).glyphs[0][5], source.glyphs[0][5])
            self.assertLessEqual(len(output), len(data))

    def test_every_alias_uses_tightest_small_label_bound(self):
        data = self.source()
        source = final.view(data)
        with tempfile.TemporaryDirectory() as cache:
            batches, proof = final.plan(source, cache, 1)
        for batch, limit in batches:
            small = any((source.glyphs[i][2] & 31) <= 10 for r in batch.records for i in r.ids)
            self.assertEqual(limit, 4096 if small else 32768)
            self.assertLessEqual(batch.raw_size, limit)
        self.assertLessEqual(
            max(sum(p[:2]) for p in proof["profiles"]), proof["previous_max_lc_plus_lp"]
        )

    def test_reduced_marker_and_all_artwork_survive(self):
        data = self.source(reduced=True)
        with tempfile.TemporaryDirectory() as cache:
            output, proof = final.convert(data, cache, 1)
        self.assertEqual(output[36:40], b"NAF1")
        self.assertEqual(final.view(data).images, final.view(output).images)
        self.assertTrue(proof["verification"]["artwork_clocks_profile_exact"])

    def test_corrupt_cached_stream_is_rejected(self):
        with tempfile.TemporaryDirectory() as cache:
            final.convert(self.source(), cache, 1)
            paths = list(Path(cache).glob("*.lzma2"))
            self.assertTrue(paths)
            paths[0].write_bytes(b"\x00")
            with self.assertRaises((ValueError, RuntimeError)):
                final.convert(self.source(), cache, 1)


if __name__ == "__main__":
    unittest.main()
