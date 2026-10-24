"""Exact synthetic pack coverage for the production group recompressor."""
from pathlib import Path
import sys
import tempfile
import unittest
import struct

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import recompress_resources as compact
import repack_retained as retained
import split_content as split
from test_repack_retained import fixture
from test_compact_ui6_merchant import source_pack


class ResourceRecompressionTests(unittest.TestCase):
    def test_latest_animation_and_journal_versions_keep_exact_rows(self):
        packed,_,_=retained.repack(fixture(),fast_images='none')
        for version in (29,30):
            source=bytearray(packed);struct.pack_into('<I',source,4,version)
            after,_=compact.recompress(bytes(source),workers=1)
            self.assertEqual(compact.view(after).version,version)
            self.exact_images(source,after)

    def exact_images(self, before, after):
        source, target = compact.view(before), compact.view(after)
        old_tiles, new_tiles = tiles.TileReader(source), tiles.TileReader(target)
        self.assertEqual(source.counts, target.counts)
        self.assertEqual(source.clips, target.clips)
        for index, entry in enumerate(source.images):
            old = old_tiles.image_raw(index) if entry[3] == 6 else source.image_raw(index)
            new = new_tiles.image_raw(index) if entry[3] == 6 else target.image_raw(index)
            self.assertEqual(old, new)

    def test_groups_aliases_and_idempotence(self):
        before, _, _ = retained.repack(fixture(), fast_images='none')
        after, report = compact.recompress(before, workers=1)
        self.assertGreater(report['saved_bytes'], 0)
        self.assertGreaterEqual(report['improved_groups'], 2)
        self.assertEqual(report['runtime_memory_added'], 0)
        self.exact_images(before, after)
        source = compact.view(after)
        self.assertEqual(source.images[0], source.images[1])
        repeated, second = compact.recompress(after, workers=1)
        self.assertEqual(repeated, after)
        self.assertEqual(second['saved_bytes'], 0)

    def test_forward_retained_pointers_and_uncompressible_literals(self):
        before = source_pack(forward=True)
        after, report = compact.recompress(before, workers=2)
        self.assertGreater(report['saved_bytes'], 0)
        self.exact_images(before, after)
        before, _, _ = retained.repack(fixture(incompressible=True), fast_images='none')
        after, _ = compact.recompress(before, workers=1)
        self.exact_images(before, after)

    def test_bilingual_offsets_keep_one_shared_artwork(self):
        eng, _, plan = retained.repack(fixture('eng', glyphs=1), fast_images='none')
        zhs, _, _ = retained.repack(fixture('zhs', glyphs=3), batches=plan, fast_images='none')
        cache = {}
        eng_new, e = compact.recompress(eng, workers=1, cache=cache)
        zhs_new, z = compact.recompress(zhs, workers=2, cache=cache)
        self.assertEqual(e['saved_bytes'], z['saved_bytes'])
        common, languages, _ = split.split(dict(eng=eng_new, zhs=zhs_new))
        self.assertEqual(split.reconstruct(common, languages['eng'], 'eng'), eng_new)
        self.assertEqual(split.reconstruct(common, languages['zhs'], 'zhs'), zhs_new)
        self.exact_images(eng, eng_new)
        self.exact_images(zhs, zhs_new)

    def test_persistent_cache_survives_relocation_and_recovers_corruption(self):
        eng, _, plan = retained.repack(fixture('eng', glyphs=1), fast_images='none')
        zhs, _, _ = retained.repack(fixture('zhs', glyphs=3), batches=plan, fast_images='none')
        with tempfile.TemporaryDirectory() as directory:
            first, _ = compact.recompress(eng, workers=1, cache_directory=directory)
            cached = sorted(Path(directory).glob('*.bin'))
            self.assertTrue(cached)
            expected, _ = compact.recompress(zhs, workers=1)
            relocated, _ = compact.recompress(zhs, workers=1, cache_directory=directory)
            self.assertEqual(expected, relocated)
            cached[0].write_bytes(b'not a complete encoder cache')
            recovered, _ = compact.recompress(eng, workers=1, cache_directory=directory)
            self.assertEqual(first, recovered)


if __name__ == '__main__':
    unittest.main()
