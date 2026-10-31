from pathlib import Path
import copy
import lzma
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import coalesce_tile_resources as coalesce
import content_tiles as t
import select_index_streams_ui22 as select
from test_coalesce_tile_resources import fixture


def gate_fixture(before, after, proof):
    row=proof['groups'][0];raw=row['raw_bytes'];digest=row['raw_sha256']
    def result(lzma_work,inverse,stored):
        return dict(lzma_instructions=lzma_work,inverse_instructions=inverse,
            total_instructions=lzma_work+inverse,stored_bytes=stored,decoded_bytes=raw,
            complete_bytes=raw,restored_sha256=digest)
    group=row['group'];elf_hashes=(t.sha256(b'baseline test ELF'),t.sha256(b'candidate test ELF'))
    gate=dict(schema=1,complete=True,source_sha256=t.sha256(before),candidate_sha256=t.sha256(after),
        old_elf_sha256=elf_hashes[0],new_elf_sha256=elf_hashes[1],
        maximum_instruction_regression_percent=0,total_candidates=1,
        accepted_groups=[group],rejected_groups=[],before_instructions=100,after_instructions=90,
        rows=[dict(group=group,records=row['records'],raw_bytes=raw,raw_sha256=digest,
            original_group_sha256=row['source_sha256'],candidate_group_sha256=row['candidate_sha256'],
            before=result(100,0,row['source_bytes']-32),after=result(80,10,row['candidate_bytes']-32),
            accepted=True,exact_rgb_index_geometry_guards_aapcs=True)])
    return gate,elf_hashes


class ExactIndexSelection(unittest.TestCase):
    def test_full_pack_preserves_pixels_metadata_and_old_decoder_bounds(self):
        before=fixture();source=coalesce.view(before);a=t.TileReader(source)
        with tempfile.TemporaryDirectory() as folder:
            after,proof=select.convert_bytes(before,Path(folder),workers=1,prepare_candidates=True,align_suffix=False)
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
            after,proof=select.convert_bytes(before,Path(folder),workers=1,prepare_candidates=True,minimum_percent=100)
            self.assertEqual(after,before);self.assertEqual(proof['index_groups_changed'],0)
            after,proof=select.convert_bytes(before,Path(folder),workers=1,prepare_candidates=True,groups=[])
            self.assertEqual(after,before)
            with self.assertRaisesRegex(ValueError,'index-group'):
                select.convert_bytes(before,Path(folder),workers=1,prepare_candidates=True,groups=[0])

    def test_corrupt_cache_cannot_change_a_tile_reference(self):
        before=fixture()
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)
            select.convert_bytes(before,path,workers=1,prepare_candidates=True,align_suffix=False)
            cached=list(path.glob('*.bin'));self.assertEqual(len(cached),1)
            raw=bytearray(cached[0].read_bytes());raw[-2]^=0x7f;cached[0].write_bytes(raw)
            with self.assertRaises((ValueError,lzma.LZMAError)):
                select.convert_bytes(before,path,workers=1,prepare_candidates=True,align_suffix=False)

    def test_default_cannot_silently_apply_unmeasured_candidates(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaisesRegex(ValueError,'ARM gate is required'):
                select.convert_bytes(fixture(),Path(folder),workers=1)

    def test_bound_gate_reproduces_candidate_and_rejects_slower_streams(self):
        before=fixture()
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)
            candidate,proof=select.convert_bytes(before,path,workers=1,prepare_candidates=True)
            self.assertTrue(proof['candidate_only']);self.assertFalse(proof['release_accepted'])
            gate,elf_hashes=gate_fixture(before,candidate,proof)
            actual,accepted=select.convert_bytes(before,path,workers=1,arm_gate=gate,elf_hashes=elf_hashes)
            self.assertEqual(actual,candidate);self.assertTrue(accepted['release_accepted'])
            self.assertEqual(accepted['arm_policy']['accepted_groups'],1)
            slower=copy.deepcopy(gate);row=slower['rows'][0]
            row['after']['lzma_instructions']=100;row['after']['total_instructions']=110;row['accepted']=False
            slower.update(accepted_groups=[],rejected_groups=[row['group']],before_instructions=0,after_instructions=0)
            actual,accepted=select.convert_bytes(before,path,workers=1,arm_gate=slower,elf_hashes=elf_hashes)
            self.assertEqual(actual,before);self.assertEqual(accepted['index_groups_changed'],0)
            self.assertEqual(accepted['arm_policy']['rejected_groups'],1)

    def test_stale_incomplete_tampered_and_contradictory_gates_fail_closed(self):
        before=fixture()
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)
            candidate,proof=select.convert_bytes(before,path,workers=1,prepare_candidates=True)
            gate,elf_hashes=gate_fixture(before,candidate,proof)
            cases=[]
            for key,value in (('complete',False),('source_sha256','0'*64),('candidate_sha256','0'*64),
                ('old_elf_sha256','0'*64),('new_elf_sha256','0'*64),('total_candidates',2),
                ('accepted_groups',[]),('maximum_instruction_regression_percent',1),('after_instructions',89)):
                bad=copy.deepcopy(gate);bad[key]=value;cases.append(bad)
            for key in ('original_group_sha256','candidate_group_sha256','raw_sha256'):
                bad=copy.deepcopy(gate);bad['rows'][0][key]='0'*64;cases.append(bad)
            bad=copy.deepcopy(gate);bad['rows'].append(copy.deepcopy(bad['rows'][0]));bad['total_candidates']=2;cases.append(bad)
            bad=copy.deepcopy(gate);bad['rows'][0]['after']['lzma_instructions']=101
            bad['rows'][0]['after']['total_instructions']=111;cases.append(bad)
            bad=copy.deepcopy(gate);bad['rows'][0]['before']['stored_bytes']+=1;cases.append(bad)
            bad=copy.deepcopy(gate);del bad['rows'][0]['after'];cases.append(bad)
            for bad in cases:
                with self.subTest(case=cases.index(bad)):
                    with self.assertRaises(ValueError):
                        select.convert_bytes(before,path,workers=1,arm_gate=bad,elf_hashes=elf_hashes)
            with self.assertRaisesRegex(ValueError,'ELF'):
                select.convert_bytes(before,path,workers=1,arm_gate=gate,elf_hashes=None)


if __name__=='__main__':
    unittest.main()
