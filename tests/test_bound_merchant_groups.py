"""Restore bounded exact groups without importing another content generation."""
from contextlib import redirect_stdout
from dataclasses import replace
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import bound_merchant_groups as bound
import compact_retained_colors as colors
import content_tiles as t
import merge_retained_resources as merge
import split_content as split
import storage_parts as storage
from complete_retained_resources_ui17 import decoded_group
from retained_filter import group_layout,transform_group
from test_merge_retained_resources import source,installation
from test_repack_retained import stored


def stages(root,code='eng'):
    original=bytearray(source(code));view=colors.view(original)
    # One synthetic semantic clip includes all retained images and an alias.
    t.CLIP.pack_into(original,view.string_at+len(view.strings)*8,0,5,100,0,0,0)
    original=bytes(original);view=colors.view(original)
    bounds=split.layout(original,code)
    common=split.common_file(original[bounds['prefix']:bounds['prefix']+bounds['artwork']])
    index,_parts,_report=storage.partition(common,original)
    plan,_count=merge.make_plan(view,index,root/'encoded',workers=1,minimum_saved=1,minimum_percent=0)
    current,_mapping=merge.rewrite(view,plan)
    assert len(colors.groups(colors.view(current)))<len(colors.groups(view))
    return original,current


class BoundMerchantGroupsTest(unittest.TestCase):
    def test_planar_medium_uses_rgr2_and_rejects_wrong_cached_codec(self):
        raw=struct.pack('<HHHB',1,0,2,1)+bytes.fromhex('1122003344ff')
        slices=((0,(4<<24)|len(raw)),);table=struct.pack('<II',*slices[0])
        stream=stored(transform_group(raw,slices,8))
        original=t.GROUP.pack(b'RGR3',len(raw),len(stream),14,1,0,8,0)+table+stream
        with tempfile.TemporaryDirectory() as folder:
            proposed=bound.medium_stream(original,raw,slices,44,folder)
            self.assertEqual(group_layout(proposed,44)[2:],(2,slices))
            self.assertEqual(decoded_group(proposed,44),raw)
            with mock.patch.object(bound.lzma,'compress',side_effect=AssertionError('Unexpected encoding')):
                self.assertEqual(bound.medium_stream(original,raw,slices,44,folder),proposed)
            # Equal pixels do not make the slower wrong cached codec eligible.
            next(Path(folder).glob('*.bin')).write_bytes(original)
            with self.assertRaisesRegex(ValueError,'RGR2 mode2'):
                bound.medium_stream(original,raw,slices,44,folder)

    def test_exact_reference_blocks_slices_aliases_and_namespace(self):
        with tempfile.TemporaryDirectory() as folder:
            original,current=stages(Path(folder));old=colors.view(original);source=colors.view(current)
            for reference in ('small','medium'):
                plan,report=bound.make_plan(source,{'small':old,'medium':old},
                    [dict(key='merchant.synthetic',clip=0,reference=reference)])
                output,relocate,descriptors=bound.rewrite(source,plan)
                proof=bound.verify(source,output,plan,relocate,descriptors);target=colors.view(output)
                self.assertEqual(len(colors.groups(target)),len(colors.groups(old)))
                self.assertEqual(target.images[0],target.images[1])
                self.assertGreater(report['raw_bytes_compared'],0)
                self.assertEqual((len(output)-len(current))%4096,0)
                self.assertTrue(proof['exact_other_streams_aliases_and_catalog'])
                before,after=t.TileReader(source),t.TileReader(target)
                for image,row in enumerate(source.images):
                    a,b=(before,after) if row[3]==6 else (source,target)
                    self.assertEqual(a.image_raw(image),b.image_raw(image))
                for group in colors.groups(old):
                    self.assertIn(bytes(old.block(group.offset,group.size)),output)

    def test_foreign_reference_plan_and_missing_semantic_track_fail_closed(self):
        with tempfile.TemporaryDirectory() as folder:
            original,current=stages(Path(folder));old=colors.view(original);source=colors.view(current)
            selection=[dict(key='merchant.synthetic',clip=0,reference='small')]
            plan,_report=bound.make_plan(source,{'small':old,'medium':old},selection)
            broken=list(plan);broken[0]=replace(plan[0],sha256='0'*64)
            with self.assertRaisesRegex(ValueError,'shared merchant plan'):bound.rewrite(source,broken)
            # A valid but different original RGB bit cannot enter this build.
            foreign=bytearray(original);group=colors.groups(old)[0];foreign[group.offset+35]^=1
            with self.assertRaisesRegex(ValueError,'complete retained raw bytes'):
                bound.make_plan(source,{'small':colors.view(foreign),'medium':old},selection)
            no_track=bytearray(original)
            t.CLIP.pack_into(no_track,old.string_at+len(old.strings)*8,5,1,100,0,0,0)
            missing=colors.view(no_track)
            with self.assertRaisesRegex(ValueError,'no retained frames'):
                bound.make_plan(missing,{'small':missing,'medium':missing},selection)

    def test_complete_bilingual_nsm1_roundtrip(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);pairs={code:stages(root/code,code) for code in ('eng','zhs')}
            _common,index,parts,locales=installation({c:p[1] for c,p in pairs.items()})
            src=root/'input';src.mkdir();(src/'nSpire2.dat.tns').write_bytes(index)
            for name,payload in parts.items():(src/name).write_bytes(payload)
            for code,payload in locales.items():(src/f'nSpire2.{code}.dat.tns').write_bytes(payload)
            reference=root/'original.nsp';reference.write_bytes(pairs['eng'][0])
            argv=['bound_merchant_groups','--input',str(src),'--output',str(root/'output'),
                  '--work',str(root/'work'),'--small-reference',str(reference),
                  '--medium-reference',str(reference),'--locales','eng','zhs']
            with mock.patch.object(sys,'argv',argv),mock.patch.object(bound,'merchant_selection',
                    return_value=[dict(key='merchant.synthetic',clip=0,reference='small')]),redirect_stdout(io.StringIO()):
                bound.main()
            target=root/'output';report=json.loads((target/'merchant-bounds-receipt.json').read_text())
            self.assertTrue(report['complete']);self.assertEqual(report['runtime_bytes_added'],0)
            common=storage.join_parts((target/'nSpire2.dat.tns').read_bytes(),
                                      {n:(target/n).read_bytes() for n in storage.PART_FILES})
            for code in ('eng','zhs'):
                logical=split.reconstruct(common,(target/f'nSpire2.{code}.dat.tns').read_bytes(),code)
                after=colors.view(logical);before=colors.view(pairs[code][0])
                self.assertEqual(after.clips,before.clips)
                for image in range(5):self.assertEqual(after.image_raw(image),before.image_raw(image))
            self.assertEqual(split.reconstruct(common,(target/'nSpire2.eng.dat.tns').read_bytes(),'eng'),
                             (root/'work/eng-bounded44.nsp').read_bytes())

    def test_concern_boundary_cannot_split_a_replaced_group(self):
        with tempfile.TemporaryDirectory() as folder:
            original,current=stages(Path(folder));old=colors.view(original);source=colors.view(current)
            common,index,_parts,_locales=installation({'eng':current})
            plan,_report=bound.make_plan(source,{'small':old,'medium':old},
                [dict(key='merchant.synthetic',clip=0,reference='small')])
            output,relocate,_descriptors=bound.rewrite(source,plan)
            group=colors.groups(source)[plan[0].group]
            prefix=split.layout(current,'eng')['prefix'];cut=64+group.offset+1-prefix
            bad=bytearray(index[:96]);bad.extend(struct.pack('<4I',64,cut,32,0))
            bad.extend(struct.pack('<4I',cut,len(common),32,1))
            struct.pack_into('<I',bad,16,len(bad));struct.pack_into('<I',bad,24,2)
            with self.assertRaisesRegex(ValueError,'interior'):
                bound.partition_with_relocation(bytes(bad),source,output,relocate)


if __name__=='__main__':unittest.main()
