"""Synthetic lossless compaction, locale relocation and grouping boundaries."""
from pathlib import Path
import lzma
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import compact_retained_resources as compact
import repack_retained as retained
import content_tiles as tiles
import split_content as split
from test_repack_retained import fixture


class CompactRetainedTest(unittest.TestCase):
    def test_existing_groups_literal_images_and_localized_offsets(self):
        eng, _, original_plan = retained.repack(fixture('eng', tail_image=True))
        zhs, _, _ = retained.repack(fixture('zhs', glyphs=3, tail_image=True), batches=original_plan)
        sources = {'eng': compact.view(eng), 'zhs': compact.view(zhs)}
        self.assertNotEqual(sources['eng'].images[66][0], sources['zhs'].images[66][0])
        with tempfile.TemporaryDirectory() as directory:
            for preserve in (True, False):
                records = compact.records(sources['eng'])
                plan = compact.batches(records, preserve)
                plan = [compact.encode(batch, sources['eng'], Path(directory), preserve) for batch in plan]
                results = {}
                for language, source in sources.items():
                    output, counts = compact.write(source, plan)
                    proof = compact.verify(source, output)
                    self.assertEqual(proof['retained_images_exact'], 66)
                    self.assertEqual(proof['tiled_images_exact'], 1)
                    self.assertLess(len(output), len(source.data))
                    target = compact.view(output)
                    cost = struct.unpack_from('<I', output, target.base_end + 56)[0]
                    old = struct.unpack_from('<I', source.data, source.base_end + 56)[0]
                    self.assertEqual(output[cost:cost + 67], source.data[old:old + 67])
                    self.assertLessEqual(counts['largest_group'], tiles.GROUP_LIMIT)
                    results[language] = output
                common, languages, receipt = split.split(results)
                self.assertTrue(receipt['byte_identical_roundtrip'])
                for language in results:
                    self.assertEqual(split.reconstruct(common, languages[language], language), results[language])

    def test_cache_corruption_fails_and_same_source_is_deterministic(self):
        original, _, _ = retained.repack(fixture())
        source = compact.view(original)
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            def convert():
                plan = [compact.encode(batch, source, cache, False)
                        for batch in compact.batches(compact.records(source), False)]
                return compact.write(source, plan)[0]
            first = convert(); self.assertEqual(convert(), first)
            self.assertTrue(list(cache.glob('*.lzma2')))
            for path in cache.glob('*.lzma2'):
                path.write_bytes(b'broken')
            with self.assertRaises((ValueError, lzma.LZMAError)):
                convert()

    def test_purpose_boundaries_and_no_oversize_batch(self):
        records = [compact.Record(0, bytes(40000), i, (1, i // 3), 65536, [i]) for i in range(6)]
        plan = compact.batches(records, False)
        self.assertEqual(len(plan), 6)
        for batch in plan:
            self.assertEqual(len({r.category for r in batch.records}), 1)
            self.assertLessEqual(sum(len(r.raw) for r in batch.records), 65536)
        records = [compact.Record(0, bytes(10000), i, (2, i // 2), 524288, [i]) for i in range(4)]
        self.assertEqual([len(b.records) for b in compact.batches(records, False)], [2, 2])


if __name__ == '__main__':
    unittest.main()
