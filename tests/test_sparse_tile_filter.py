"""Lossless sparse tile fields, live package images and corruption boundaries."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import coalesce_tile_resources as coalesce
import compact_sparse_tiles as compact
import content_tiles as t
import sparse_tile_filter as sparse
import split_content as split
import storage_parts as storage
from test_coalesce_tile_resources import fixture


def sparse_fixture():
    result=bytearray(fixture(strings=6111))
    reader=t.TileReader(coalesce.view(result))
    for at,_encoded,_raw,_crc,_kind,items in reader.groups[:reader.tile_groups]:
        for tile in range(items):
            for pixel in range(1,256,2):
                begin=at+3+tile*770
                result[begin+2+pixel*2:begin+4+pixel*2]=b'\0\0'
                result[begin+514+pixel]=0
    return bytes(result)


class SparseTileFilterTest(unittest.TestCase):
    def test_every_rgb565_word_hidden_color_alpha_and_partial_zero_mask(self):
        records=np.zeros((512,770),dtype='u1');records[:,:2]=16
        values=np.arange(65536,dtype='<u2').reshape(512,128)
        colors=np.zeros((512,256),dtype='<u2');colors[:,::2]=values
        alpha=np.zeros((512,256),dtype='u1');alpha[:,::2]=np.arange(65536,dtype='u1').reshape(512,128)
        records[:,2:514]=colors.view('u1');records[:,514:]=alpha
        raw=records.tobytes()
        for mode in (1,2):
            filtered=sparse.transform(raw,mode)
            self.assertLess(len(filtered),len(raw))
            self.assertEqual(sparse.restore(filtered,512,mode),raw)
            self.assertLessEqual(512*34+sparse.pixel_count(512,len(filtered)),512*258)

    def test_mask_population_length_dimensions_and_scratch_limits_are_checked(self):
        raw=b'\x10\x10'+bytes(768)
        filtered=sparse.transform(raw,1)
        bad=bytearray(filtered);bad[2]=1
        with self.assertRaisesRegex(ValueError,'mask population'):sparse.restore(bad,1,1)
        bad=bytearray(filtered);bad[0]=0
        with self.assertRaises(ValueError):sparse.restore(bad,1,1)
        for items,size in ((0,34),(1361,1361*34),(1,33),(1,35),(1,34+225*3)):
            with self.assertRaises(ValueError):sparse.pixel_count(items,size)
        with self.assertRaisesRegex(ValueError,'scratch bound'):
            sparse.transform(b'\x10\x10'+b'\xff'*768,1)

    def test_complete_conversion_preserves_all_images_and_index_geometry(self):
        before=sparse_fixture();source=coalesce.view(before)
        with tempfile.TemporaryDirectory() as folder:
            plan,report=compact.make_plan(source,Path(folder),workers=2)
            after,_=coalesce.rewrite(before,plan,version=45)
            proof=compact.verify(before,after,plan)
            old,new=t.TileReader(source),t.TileReader(coalesce.view(after))
            self.assertEqual(report['changed_groups'],2)
            self.assertTrue(all(new.sparse[:new.tile_groups]))
            self.assertGreater(proof['compared_live_tile_bytes'],0)
            for image in range(len(source.images)):
                self.assertEqual(old.image_raw(image),new.image_raw(image))
            # Exercise the independent physical packer too: it validates
            # group headers without going through the host TileReader.
            bounds=split.layout(after,'eng')
            common=split.common_file(after[bounds['prefix']:bounds['prefix']+bounds['artwork']])
            index,parts,_details=storage.partition(common,after)
            self.assertEqual(storage.join_parts(index,parts),common)
            language=split.compress_language_directory(split.language_file(after,bounds,'eng',bytes(32)))
            language,_details=storage.compress_blocks(language)
            self.assertEqual(split.reconstruct(common,language,'eng'),after)
            repeated,again=compact.make_plan(source,Path(folder),workers=1)
            self.assertEqual(repeated,plan);self.assertEqual(again,report)
            bad=bytearray(after);struct.pack_into('<I',bad,4,44)
            with self.assertRaises(ValueError):t.TileReader(coalesce.view(bad))
            with self.assertRaises(ValueError):split.layout(bad,'eng')
            cached=next((Path(folder)/'sparse-encoded').glob('*.lzma2'));cached.write_bytes(b'bad')
            with self.assertRaises(ValueError):compact.make_plan(source,Path(folder),workers=1)


if __name__=='__main__':unittest.main()
