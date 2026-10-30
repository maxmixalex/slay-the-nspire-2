"""Read-page phase repair preserves encoded bytes, pixels and all metadata."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import align_resource_suffix as align
import coalesce_tile_resources as coalesce
import compact_tile_slots as compact
import content_tiles as t
import split_content as split
import storage_parts as storage
from test_coalesce_tile_resources import fixture


def identity_plan(data):
    reader=t.TileReader(coalesce.view(data));plan={}
    for group,row in enumerate(reader.groups):
        at,length=row[:2];payload=data[at-32:at+length];plan[group]=payload,payload,payload
    return plan


def proof(before,after,plan):
    return coalesce.verify(before,after,plan,None,None,None,
                           pixel_groups=False,encoded_groups_only=True)


class ResourceSuffixTest(unittest.TestCase):
    def test_padding_only_plan_keeps_all_encoded_bytes_and_maximum_even_phase(self):
        original=fixture();plan=identity_plan(original);last=max(plan)
        a,b,c=plan[last];plan[last]=a,b,c+bytes(2)
        shifted,_=coalesce.rewrite(original,plan)
        self.assertEqual(len(shifted),len(original)+2)
        self.assertTrue(proof(original,shifted,plan)['encoded_groups_only'])
        plan,receipt=align.make_plan(shifted,original)
        self.assertEqual(receipt['padding_bytes'],4094)
        self.assertEqual(receipt['input_suffix_phase'],(receipt['reference_suffix_phase']+2)%4096)
        self.assertEqual(receipt['output_suffix_phase'],receipt['reference_suffix_phase'])
        output,_=coalesce.rewrite(shifted,plan)
        proof(shifted,output,plan)
        self.assertEqual(len(output),len(original)+4096)
        a,b=t.TileReader(coalesce.view(original)),t.TileReader(coalesce.view(output))
        for image in range(2):self.assertEqual(a.image_raw(image),b.image_raw(image))

    def test_already_aligned_input_is_idempotent(self):
        original=fixture();plan,receipt=align.make_plan(original,original)
        self.assertEqual(receipt['padding_bytes'],0)
        self.assertEqual(coalesce.rewrite(original,plan)[0],original)
        plan=identity_plan(original);last=max(plan);a,b,c=plan[last];plan[last]=a,b,c+bytes(4096)
        shifted,_=coalesce.rewrite(original,plan)
        plan,receipt=align.make_plan(shifted,original)
        self.assertEqual(receipt['padding_bytes'],0)
        self.assertEqual(coalesce.rewrite(shifted,plan)[0],shifted)

    def test_compacted_resource_preserves_reference_concern_spans(self):
        original=fixture(strings=6111);bounds=split.layout(original,'eng')
        common=split.common_file(original[bounds['prefix']:bounds['prefix']+bounds['artwork']])
        index,_parts,_=storage.partition(common,original)
        with tempfile.TemporaryDirectory() as folder:
            plan,_,_,_,_=compact.make_plan(coalesce.view(original),Path(folder),workers=1)
            compacted,_=coalesce.rewrite(original,plan,version=42)
            plan,receipt=align.make_plan(compacted,original)
            aligned,_=coalesce.rewrite(compacted,plan,version=42)
            self.assertEqual(receipt['reference_suffix_phase'],receipt['output_suffix_phase'])
            proof(compacted,aligned,plan)
            common,new_index,parts,partition=compact.partition_like(index,original,aligned)
            self.assertEqual(partition['spans'],struct.unpack_from('<I',index,24)[0])
            self.assertEqual(storage.join_parts(new_index,parts),common)
            reader=t.TileReader(coalesce.view(aligned))
            for i in range(2):self.assertEqual(reader.image_raw(i),t.TileReader(coalesce.view(original)).image_raw(i))

    def test_changed_reference_suffix_is_rejected(self):
        original=fixture();changed=bytearray(original)
        glyph=coalesce.view(original).glyphs[0][1];changed[glyph]^=1
        with self.assertRaisesRegex(ValueError,'suffix differs'):
            align.make_plan(original,bytes(changed))

    def test_encoded_identity_proof_cannot_hide_codec_changes(self):
        original=fixture();plan=identity_plan(original);a,b,c=plan[0]
        changed=b[:-1]+bytes((b[-1]^1,));plan[0]=a,changed,changed
        output,_=coalesce.rewrite(original,plan)
        with self.assertRaisesRegex(ValueError,'changed an encoded group'):
            proof(original,output,plan)


if __name__=='__main__':unittest.main()
