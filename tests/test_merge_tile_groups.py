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
from test_repack_retained import stored


def mixed_fixture():
    """Four source groups, one protected image and a later unprotected pair."""
    original=fixture();source=coalesce.view(original);reader=t.TileReader(source)
    base=source.base_end;table=base+64;costs=table+5*8;descriptors=costs+2
    out=bytearray(original[:base])+bytearray(descriptors+2*16-base)
    records=[reader.group(0)[0],reader.group(1)[0]]*2
    index=bytearray(reader.group(2)[0])
    starts=t.index_records(index,2,4)
    struct.pack_into('<I',index,list(starts)[1]+12,6)
    for group,(raw,count) in enumerate([*( (raw,2) for raw in records),(bytes(index),2)]):
        out.extend(bytes(len(out)&1));at=len(out);encoded=stored(raw)
        block=t.GROUP.pack(b'TGR1',1 if group<4 else 2,group,len(raw),len(encoded),0,count,0)+encoded
        out.extend(block);struct.pack_into('<II',out,table+group*8,at,len(block))
    for image in range(2):
        t.IMAGE.pack_into(out,40+image*16,descriptors+image*16,32,16,6,16)
        _global,local,at,size=t.DESCRIPTOR.unpack_from(original,source.images[image][0])
        t.DESCRIPTOR.pack_into(out,descriptors+image*16,base,local,at,size)
    out.extend(bytes(len(out)&1));glyph=len(out);out.extend(b'\xa6')
    t.GLYPH.pack_into(out,source.glyph_at,65,glyph,8,1,1,1,0)
    out.extend(bytes(len(out)&1));string=len(out);out.extend(b'eng\0')
    struct.pack_into('<II',out,source.string_at,string,3)
    t.CLIP.pack_into(out,source.string_at+8,0,1,100,0,0,0)
    t.GLOBAL.pack_into(out,base,b'TIL1',1,16,14,8,4,1,2,descriptors,table,
                       770,2,t.GROUP_LIMIT,t.ENCODED_LIMIT,costs,2)
    out[costs:costs+2]=bytes((1,1));struct.pack_into('<I',out,12,len(out))
    return bytes(out)


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

    def test_protected_semantic_clip_creates_mixed_groups_and_new_slot_limit(self):
        original=mixed_fixture();source=coalesce.view(original);before=t.TileReader(source)
        with tempfile.TemporaryDirectory() as folder:
            plan,mapping,report=merge.make_plan(source,Path(folder),workers=1,
                                               protected_clips={'enemy.synthetic.idle':0})
            output,relocate,_padding=merge.rewrite(original,plan)
            merge.verify(original,output,plan,mapping,relocate)
            after=t.TileReader(coalesce.view(output))
            self.assertEqual([row['old'] for row in plan['pixels']],[[0],[1],[2,3]])
            self.assertEqual(after.tile_groups,3);self.assertEqual(after.per_group,4)
            self.assertEqual(after.tile_count,12)
            self.assertEqual(mapping.tolist(),[0,1,4,5,8,9,10,11])
            self.assertEqual(report['protection']['protected_pairs'],1)
            self.assertEqual(report['protection']['selected_active_groups'],[0])
            self.assertTrue(report['protection']['selected_track_group_pages_do_not_increase'])
            self.assertEqual([row[5] for row in after.groups[:3]],[2,2,4])
            for group in range(2):
                old_at,old_size=before.groups[group][:2];new_at,new_size=after.groups[group][:2]
                self.assertEqual(original[old_at:old_at+old_size],output[new_at:new_at+new_size])
            for image in range(2):self.assertEqual(before.image_raw(image),after.image_raw(image))
            raw,_starts=after.group(after.tile_groups)
            self.assertIn(10,struct.unpack_from('<II',raw,list(t.index_records(raw,2,12))[1]+12))

    def test_protected_compacted_groups_keep_all_inactive_index_slots_valid(self):
        original=fixture()
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            plan,_mapping,_keep,_slots,_report=compact.make_plan(coalesce.view(original),root/'compact',workers=1)
            compacted,_=coalesce.rewrite(original,plan,version=42)
            before=t.TileReader(coalesce.view(compacted))
            plan,mapping,report=merge.make_plan(coalesce.view(compacted),root/'pair',workers=1,
                                               protected_clips={'character.synthetic.idle':0})
            output,relocate,_padding=merge.rewrite(compacted,plan)
            merge.verify(compacted,output,plan,mapping,relocate)
            after=t.TileReader(coalesce.view(output))
            self.assertGreater(after.tile_count,before.tile_count)
            self.assertEqual(after.tile_groups,2)
            for image in range(2):self.assertEqual(before.image_raw(image),after.image_raw(image))
            for selection in ({'bad':-1},{'missing':1},{'wrong-type':'0'}):
                with self.assertRaisesRegex(ValueError,'protected animation'):
                    merge.make_plan(coalesce.view(original),root/'bad',workers=1,protected_clips=selection)

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
