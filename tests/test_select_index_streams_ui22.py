from pathlib import Path
import lzma
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import coalesce_tile_resources as coalesce
import content_tiles as t
import select_index_streams_ui22 as select
from test_coalesce_tile_resources import fixture


class ExactIndexSelection(unittest.TestCase):
    def test_full_pack_preserves_pixels_metadata_and_old_decoder_bounds(self):
        before=fixture();source=coalesce.view(before);a=t.TileReader(source)
        with tempfile.TemporaryDirectory() as folder:
            after,proof=select.convert_bytes(before,Path(folder),workers=1,align_suffix=False)
        b=t.TileReader(coalesce.view(after))
        self.assertTrue(proof['complete']);self.assertGreater(proof['verification']['saved_bytes'],0)
        self.assertEqual(proof['index_groups_changed'],1)
        self.assertTrue(b.predictors[-1])
        for image in range(len(source.images)):
            self.assertEqual(a.image_raw(image),b.image_raw(image))
        for group in range(a.tile_groups):
            at,n,*_=a.groups[group];bt,m,*_=b.groups[group]
            self.assertEqual(before[at-32:at+n],after[bt-32:bt+m])
        self.assertEqual(a.group(a.tile_groups)[0],b.group(b.tile_groups)[0])

    def test_unprofitable_or_unselected_candidates_keep_original_bytes(self):
        before=fixture()
        with tempfile.TemporaryDirectory() as folder:
            after,proof=select.convert_bytes(before,Path(folder),workers=1,minimum_percent=100)
            self.assertEqual(after,before);self.assertEqual(proof['index_groups_changed'],0)
            after,proof=select.convert_bytes(before,Path(folder),workers=1,groups=[])
            self.assertEqual(after,before)
            with self.assertRaisesRegex(ValueError,'index-group'):
                select.convert_bytes(before,Path(folder),workers=1,groups=[0])

    def test_corrupt_cache_cannot_change_a_tile_reference(self):
        before=fixture()
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)
            select.convert_bytes(before,path,workers=1,align_suffix=False)
            cached=list(path.glob('*.bin'));self.assertEqual(len(cached),1)
            raw=bytearray(cached[0].read_bytes());raw[-2]^=0x7f;cached[0].write_bytes(raw)
            with self.assertRaises((ValueError,lzma.LZMAError)):
                select.convert_bytes(before,path,workers=1,align_suffix=False)


if __name__=='__main__':
    unittest.main()
