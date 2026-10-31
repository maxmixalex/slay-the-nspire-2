from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import coalesce_tile_resources as coalesce
import content_tiles as t
import prune_orphan_index_records_ui22 as prune
from test_coalesce_tile_resources import fixture
from test_repack_retained import stored
from tile_prediction import index_transform


class MeasuredFixture:
    elf_sha256 = t.sha256(b'synthetic test-only measurement')
    def __init__(self, slower=False): self.slower=slower; self.calls=0
    def index(self, payload, raw, *_):
        self.calls += 1
        work = 200 if self.slower and self.calls % 2 == 0 else len(raw)
        if self.slower and self.calls % 2: work=100
        return dict(lzma_instructions=work, inverse_instructions=0, total_instructions=work,
                    stored_bytes=len(payload), decoded_bytes=len(raw), restored_sha256=t.sha256(raw),
                    exact_records_guards_aapcs=True)


def with_middle_orphan(predicted=False):
    data=fixture(); source=coalesce.view(data); reader=t.TileReader(source); group=reader.tile_groups
    raw, starts=reader.group(group); first, second=list(starts)
    orphan=bytearray(raw[first:first+starts[first]]); struct.pack_into('<II',orphan,12,3,0)
    combined=raw[:second]+orphan+raw[second:]
    payload=stored(index_transform(combined,3,3,tile_count=reader.tile_count) if predicted else combined)
    plan={}
    for i,(at,size,*_)in enumerate(reader.groups):
        original=data[at-32:at+size]; candidate=original
        if i==group:
            header=list(t.GROUP.unpack_from(original));header[3],header[4],header[6]=len(combined),len(payload),3
            if predicted: header[0],header[7]=b'TGI2',1
            candidate=t.GROUP.pack(*header)+payload
        plan[i]=(original,candidate,candidate+bytes((len(original)-len(candidate))&1))
    result,relocate=coalesce.rewrite(data,plan,version=46);result=bytearray(result)
    old=reader.descriptors+16;g,local,offset,length=t.DESCRIPTOR.unpack_from(data,old)
    t.DESCRIPTOR.pack_into(result,relocate(old),relocate(g),local,offset+len(orphan),length)
    return bytes(result)


class OrphanRecords(unittest.TestCase):
    def test_real_removal_preserves_live_pixels_offsets_and_catalog(self):
        data=with_middle_orphan(); old=coalesce.view(data)
        output,proof=prune.convert_bytes(data,MeasuredFixture(),align_suffix=False)
        new=coalesce.view(output);reader=t.TileReader(new)
        self.assertEqual(proof['orphan_records_removed'],1)
        self.assertEqual(reader.groups[-1][5],2)
        self.assertLess(len(output),len(data))
        for i in range(len(old.images)): self.assertEqual(t.TileReader(old).image_raw(i),reader.image_raw(i))
        self.assertEqual(old.clips,new.clips);self.assertEqual(old.aux,new.aux)
        self.assertEqual(old.glyph_payload(0),new.glyph_payload(0))
        self.assertEqual(old.string_payload(0),new.string_payload(0))

    def test_complete_decoder_regression_keeps_entire_original_group(self):
        data=with_middle_orphan();output,proof=prune.convert_bytes(data,MeasuredFixture(True))
        self.assertEqual(output,data);self.assertEqual(proof['orphan_records_removed'],0)
        self.assertEqual(proof['groups'][0]['rejection'],'complete_linked_decoder_instructions_increased')

    def test_predicted_indices_are_rebuilt_without_the_removed_parent(self):
        data=with_middle_orphan(True);old=coalesce.view(data)
        output,proof=prune.convert_bytes(data,MeasuredFixture(),align_suffix=False)
        new=coalesce.view(output);a,b=t.TileReader(old),t.TileReader(new)
        self.assertEqual(proof['orphan_records_removed'],1);self.assertTrue(b.predictors[-1])
        for i in range(len(old.images)):self.assertEqual(a.image_raw(i),b.image_raw(i))

    def test_no_orphan_is_identity_and_default_requires_measurement(self):
        data=fixture();output,proof=prune.convert_bytes(data,MeasuredFixture())
        self.assertEqual(output,data);self.assertEqual(proof['groups_considered'],0)
        with self.assertRaisesRegex(ValueError,'ARM measurement'):
            prune.convert_bytes(data,None)

    def test_interior_descriptor_is_rejected_before_removal(self):
        data=bytearray(with_middle_orphan());source=coalesce.view(data);reader=t.TileReader(source)
        at=reader.descriptors+16;struct.pack_into('<I',data,at+8,1)
        with self.assertRaisesRegex(ValueError,'inside/misses'):
            prune.convert_bytes(bytes(data),MeasuredFixture())


if __name__=='__main__': unittest.main()
