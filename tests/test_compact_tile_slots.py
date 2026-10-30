"""NSP42 smaller nonempty groups with legacy strictness and exact pixels."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import coalesce_tile_resources as coalesce
import compact_tile_slots as compact
import content_tiles as t
import split_content as split
import storage_parts as storage
from test_coalesce_tile_resources import fixture


class CompactTileSlotsTest(unittest.TestCase):
    def test_repeated_padding_preserves_existing_concern_boundaries(self):
        original=fixture(strings=6111)
        bounds=split.layout(original,'eng')
        common=split.common_file(original[bounds['prefix']:bounds['prefix']+bounds['artwork']])
        index,_parts,_report=storage.partition(common,original)
        def replace(data,chunks):
            reader=t.TileReader(coalesce.view(data));plan={}
            for group,row in enumerate(reader.groups):
                at,length=row[:2];old=data[at-32:at+length];candidate=old
                if group==0:
                    raw=reader.group(group)[0];encoded=bytearray()
                    previous=0
                    for part in range(chunks):
                        end=len(raw)*(part+1)//chunks;size=end-previous
                        encoded.extend(bytes((1 if part==0 else 2,(size-1)>>8,(size-1)&255)))
                        encoded.extend(raw[previous:end]);previous=end
                    encoded.append(0);header=list(t.GROUP.unpack_from(old));header[4]=len(encoded)
                    candidate=t.GROUP.pack(*header)+encoded
                plan[group]=old,candidate,candidate+bytes((len(old)-len(candidate))&1)
            return coalesce.rewrite(data,plan)[0]
        converted=replace(replace(original,2),3)
        common,new_index,parts,receipt=compact.partition_like(index,original,converted)
        self.assertEqual(storage.join_parts(new_index,parts),common)
        self.assertEqual(receipt['spans'],struct.unpack_from('<I',index,24)[0])
        reader=t.TileReader(coalesce.view(converted))
        for i in range(2):self.assertEqual(reader.image_raw(i),t.TileReader(coalesce.view(original)).image_raw(i))

    def test_full_compaction_and_legacy_version_rejection(self):
        original=fixture();before=t.TileReader(coalesce.view(original))
        with tempfile.TemporaryDirectory() as folder:
            plan,mapping,keep,slots,report=compact.make_plan(coalesce.view(original),Path(folder),workers=1)
            self.assertGreater(report['aligned_bytes_saved'],0)
            self.assertGreater(report['raw_tile_bytes_removed'],0)
            self.assertEqual(slots[1].tolist(),[0])
            output,_=coalesce.rewrite(original,plan,version=42)
            proof=coalesce.verify(original,output,plan,mapping,keep,None,compact_slots=slots)
            after=t.TileReader(coalesce.view(output))
            self.assertEqual(after.groups[1][5],1)
            self.assertEqual(after.groups[1][2],770)
            for i in range(2):self.assertEqual(before.image_raw(i),after.image_raw(i))
            self.assertTrue(proof['exact_mapped_tile_records'])
            legacy=bytearray(output);struct.pack_into('<I',legacy,4,41)
            with self.assertRaisesRegex(ValueError,'group shape'):t.TileReader(coalesce.view(legacy))

    def test_empty_groups_keep_one_unreferenced_dummy(self):
        original=fixture()
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)
            plan,mapping,keep,raw,_report=coalesce.make_plan(coalesce.view(original),path/'alias',workers=1)
            aliased,_=coalesce.rewrite(original,plan);del raw
            plan,mapping,keep,slots,report=compact.make_plan(coalesce.view(aliased),path/'compact',workers=1)
            self.assertEqual(slots[1].tolist(),[])
            self.assertEqual(report['dummy_groups'],1)
            output,_=coalesce.rewrite(aliased,plan,version=42)
            coalesce.verify(aliased,output,plan,mapping,keep,None,compact_slots=slots)
            reader=t.TileReader(coalesce.view(output))
            self.assertEqual(reader.group(1)[0],b'\x01\x01'+bytes(768))
            for i in range(2):self.assertEqual(reader.image_raw(i),t.TileReader(coalesce.view(original)).image_raw(i))

    def test_zero_oversized_items_and_mismatched_raw_size_rejected(self):
        original=fixture()
        with tempfile.TemporaryDirectory() as folder:
            plan,mapping,keep,slots,_report=compact.make_plan(coalesce.view(original),Path(folder),workers=1)
            output,_=coalesce.rewrite(original,plan,version=42)
            group=t.TileReader(coalesce.view(output)).groups[1][0]-32
            for at,value in ((group+24,0),(group+24,3),(group+12,771)):
                bad=bytearray(output);struct.pack_into('<I',bad,at,value)
                with self.assertRaisesRegex(ValueError,'group shape'):t.TileReader(coalesce.view(bad))

    def test_indices_cannot_name_removed_slots(self):
        original=fixture()
        with tempfile.TemporaryDirectory() as folder:
            plan,mapping,keep,slots,_report=compact.make_plan(coalesce.view(original),Path(folder),workers=1)
            output,_=coalesce.rewrite(original,plan,version=42)
            reader=t.TileReader(coalesce.view(output))
            # This input's group1 slot1 is deliberately unreachable. Its
            # direct host decoder must reject a crafted reference to it.
            raw=bytearray(reader.group(reader.tile_groups)[0])
            first=next(iter(t.index_records(raw,2,reader.tile_count)))
            struct.pack_into('<I',raw,first+12,3)
            reader.cache[reader.tile_groups]=(bytes(raw),t.index_records(raw,2,reader.tile_count))
            with self.assertRaisesRegex(ValueError,'edge dimensions'):reader.image_raw(0)

    def test_weighted_sample_is_bounded_deterministic_subset(self):
        reader=t.TileReader(coalesce.view(fixture()))
        self.assertEqual(compact.weighted_sample(reader,[0,1],1),[0])
        self.assertEqual(compact.weighted_sample(reader,[0,1],2),[0,1])
        self.assertEqual(compact.weighted_sample(reader,[0,1],None),[0,1])


if __name__=='__main__':unittest.main()
