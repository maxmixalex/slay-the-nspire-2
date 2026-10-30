"""Complete direct tile pairing preserves all records and relocated catalogs."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import coalesce_tile_resources as coalesce
import compact_tile_slots as compact
import content_tiles as t
import merge_tile_groups as merge
import split_content as split
import storage_parts as storage
from test_coalesce_tile_resources import fixture


class MergeTileGroupsTest(unittest.TestCase):
    def test_complete_indices_pixels_locale_and_concerns(self):
        original=fixture(strings=6111);before=t.TileReader(coalesce.view(original))
        with tempfile.TemporaryDirectory() as folder:
            plan,mapping,report=merge.make_plan(coalesce.view(original),Path(folder),workers=1)
            output,relocate,padding=merge.rewrite(original,plan)
            proof=merge.verify(original,output,plan,mapping,relocate)
            after=t.TileReader(coalesce.view(output))
            self.assertEqual(after.per_group,4);self.assertEqual(after.tile_groups,1)
            self.assertEqual(after.group(0)[0],before.group(0)[0]+before.group(1)[0])
            self.assertEqual(after.dictionary,t.LARGE_GROUP_LIMIT)
            self.assertLessEqual(padding,4095)
            self.assertTrue(proof['exact_tiles_and_indices'])
            for i in range(2):self.assertEqual(before.image_raw(i),after.image_raw(i))
            bounds=split.layout(output,'eng')
            common=split.common_file(output[bounds['prefix']:bounds['prefix']+bounds['artwork']])
            index,parts,_details=storage.partition(common,output)
            self.assertEqual(storage.join_parts(index,parts),common)
            language=split.compress_language_directory(split.language_file(output,bounds,'eng',bytes(32)))
            language,_details=storage.compress_blocks(language)
            self.assertEqual(split.reconstruct(common,language,'eng'),output)
            self.assertLess(report['pixel_stream_bytes_saved']*-1,len(original))
            for version in (43,45):
                bad=bytearray(output);struct.pack_into('<I',bad,4,version)
                with self.assertRaises(ValueError):t.TileReader(coalesce.view(bad))

    def test_compacted_slots_shift_odd_group_without_changing_pixels(self):
        original=fixture()
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            plan,mapping,keep,slots,_report=compact.make_plan(coalesce.view(original),root/'compact',workers=1)
            compacted,_=coalesce.rewrite(original,plan,version=42)
            before=t.TileReader(coalesce.view(compacted))
            plan,mapping,_report=merge.make_plan(coalesce.view(compacted),root/'pair',workers=1)
            output,relocate,_padding=merge.rewrite(compacted,plan)
            merge.verify(compacted,output,plan,mapping,relocate)
            after=t.TileReader(coalesce.view(output))
            self.assertEqual(after.groups[0][5],sum(g[5] for g in before.groups[:2]))
            for i in range(2):self.assertEqual(before.image_raw(i),after.image_raw(i))

    def test_foreign_plan_and_corrupt_cached_bytes_are_rejected(self):
        original=fixture()
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);plan,_mapping,_report=merge.make_plan(coalesce.view(original),root,workers=1)
            bad=bytearray(original);offset=t.TileReader(coalesce.view(original)).groups[0][0]
            bad[offset+4]^=1
            with self.assertRaisesRegex(ValueError,'artwork differs'):merge.rewrite(bytes(bad),plan)
            path=next((root/'paired-encoded').glob('*.lzma2'));path.write_bytes(b'bad')
            with self.assertRaises(ValueError):merge.make_plan(coalesce.view(original),root,workers=1)


if __name__=='__main__':unittest.main()
