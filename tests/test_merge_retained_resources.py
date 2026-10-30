"""Exact multi-group descriptor remapping and complete merged installation."""
from contextlib import redirect_stdout
from dataclasses import replace
import io
import json
import lzma
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import compact_retained_colors as colors
import content_tiles as t
import merge_retained_resources as merge
import repack_retained as retained
import split_content as split
import storage_parts as storage
from test_repack_retained import fixture, stored


def source(code='eng'):
    old=retained.view(fixture(code))
    records=[r for b in retained.plan_images(old,fast_images='none') for r in b.records]
    # One legal retained stream per unique image, including odd-sized dense
    # alpha and a sparse image. Aliased image entries keep one descriptor.
    batches=[retained.Batch([r],stored(r.raw),len(r.raw),len(r.payload)) for r in records]
    return retained.write_pack(old,batches,output_version=43)[0]


def installation(original):
    common,locales,_=split.split(original,compress_directory=True)
    index,parts,_=storage.partition(common,original['eng'])
    return common,index,parts,{c:storage.compress_blocks(v)[0] for c,v in locales.items()}


class MergeRetainedTest(unittest.TestCase):
    def test_pair_exact_raw_images_aliases_and_descriptor_relocation(self):
        original=source();old=colors.view(original)
        _common,index,_parts,_locales=installation({'eng':original,'zhs':source('zhs')})
        with tempfile.TemporaryDirectory() as folder:
            plan,count=merge.make_plan(old,index,Path(folder),workers=1,minimum_saved=1,minimum_percent=0)
            self.assertGreater(sum(p.payload is not None for p in plan),0)
            result,mapping=merge.rewrite(old,plan)
            proof=merge.verify(old,result,plan,mapping,decode_groups=True)
            after=colors.view(result)
            self.assertEqual(proof['complete_raw_groups_compared'],count)
            self.assertLess(proof['output_retained_groups'],count)
            self.assertGreater(proof['saved_bytes'],0)
            self.assertEqual(after.images[0],after.images[1])
            a,b=t.TileReader(old),t.TileReader(after)
            for i,row in enumerate(old.images):
                x,y=(a,b) if row[3]==6 else (old,after)
                self.assertEqual(x.image_raw(i),y.image_raw(i))
            # The cached encoder has to pass the same exact-group verification.
            with mock.patch.object(merge.lzma,'compress',side_effect=AssertionError('Unexpected encoding')):
                again,_=merge.make_plan(old,index,Path(folder),workers=1,minimum_saved=1,minimum_percent=0)
            self.assertEqual(merge.rewrite(old,again)[0],result)
            # Every selected right-hand image receives its full left raw size.
            selected=next(p for p in plan if p.payload is not None)
            right=colors.groups(old)[selected.first+1]
            image=right.first_image;off=after.images[image][0]
            invalid=bytearray(result)
            shift=struct.unpack_from('<I',invalid,off+8)[0]
            self.assertGreater(shift,0);struct.pack_into('<I',invalid,off+8,shift-1)
            with self.assertRaises(ValueError):merge.verify(old,bytes(invalid),plan,mapping,decode_groups=True)
            # A foreign plan never overwrites metadata or silently merges it.
            broken=replace(selected,hashes=('0'*64,selected.hashes[1]))
            with self.assertRaisesRegex(ValueError,'differs from the shared plan'):
                merge.rewrite(old,[broken])

    def test_same_part_in_two_distinct_spans_does_not_cross_boundary(self):
        original=source();old=colors.view(original)
        _common,index,_parts,_locales=installation({'eng':original,'zhs':source('zhs')})
        groups,pairs=merge.eligible_pairs(old,index);self.assertTrue(pairs)
        first,left,right,*_=pairs[0]
        prefix=split.layout(original,'eng')['prefix'];boundary=64+right.offset-prefix
        n=struct.unpack_from('<I',index,24)[0]
        rows=[struct.unpack_from('<4I',index,96+i*16) for i in range(n)]
        altered=[]
        for start,end,physical,part in rows:
            if start<boundary<end:
                altered.extend(((start,boundary,physical,part),(boundary,end,physical+boundary-start,part)))
            else:altered.append((start,end,physical,part))
        fake=bytearray(index[:96]);struct.pack_into('<I',fake,24,len(altered))
        fake.extend(b''.join(struct.pack('<4I',*r) for r in altered));struct.pack_into('<I',fake,16,len(fake))
        _,remaining=merge.eligible_pairs(old,bytes(fake))
        self.assertFalse(any(row[0]==first for row in remaining))

    def test_minimum_gain_keeps_original_group_boundaries(self):
        original=source();old=colors.view(original)
        _common,index,_parts,_locales=installation({'eng':original,'zhs':source('zhs')})
        with tempfile.TemporaryDirectory() as folder:
            plan,_=merge.make_plan(old,index,Path(folder),workers=1,minimum_saved=10**9,minimum_percent=0)
            self.assertTrue(all(p.payload is None for p in plan))
            output,mapping=merge.rewrite(old,plan)
            self.assertEqual(output,original)
            merge.verify(old,output,plan,mapping,decode_groups=True)

    def test_complete_bilingual_split_and_readback(self):
        original={c:source(c) for c in ('eng','zhs')}
        _common,index,parts,locales=installation(original)
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);src=root/'input';src.mkdir()
            (src/'nSpire2.dat.tns').write_bytes(index)
            for name,payload in parts.items():(src/name).write_bytes(payload)
            for code,payload in locales.items():(src/f'nSpire2.{code}.dat.tns').write_bytes(payload)
            output,work=root/'output',root/'work'
            argv=['merge_retained_resources','--input',str(src),'--output',str(output),'--work',str(work),
                  '--workers','1','--locales','eng','zhs','--minimum-saved','1','--minimum-percent','0']
            with mock.patch.object(sys,'argv',argv),redirect_stdout(io.StringIO()):merge.main()
            receipt=json.loads((output/'retained-merge-receipt.json').read_text())
            self.assertTrue(receipt['complete']);self.assertGreater(receipt['saved_installed_bytes'],0)
            common=storage.join_parts((output/'nSpire2.dat.tns').read_bytes(),
                                      {n:(output/n).read_bytes() for n in storage.PART_FILES})
            for code in ('eng','zhs'):
                logical=split.reconstruct(common,(output/f'nSpire2.{code}.dat.tns').read_bytes(),code)
                self.assertEqual(logical,(work/f'{code}-merged.nsp').read_bytes())
                self.assertEqual(struct.unpack_from('<I',logical,4)[0],43)
                self.assertEqual(len(colors.view(logical).images),len(colors.view(original[code]).images))


    def test_alignment_padding_cannot_cross_a_concern(self):
        original=source();old=colors.view(original)
        common,index,_parts,_locales=installation({'eng':original,'zhs':source('zhs')})
        groups=colors.groups(old);last=groups[-1]
        self.assertEqual(last.size&1,1)
        prefix=split.layout(original,'eng')['prefix'];boundary=64+last.offset+last.size-prefix
        fake=bytearray(index[:96]);fake.extend(struct.pack('<4I',64,boundary,32,0))
        fake.extend(struct.pack('<4I',boundary,len(common),32,1))
        struct.pack_into('<I',fake,16,len(fake));struct.pack_into('<I',fake,24,2)
        _,pairs=merge.eligible_pairs(old,bytes(fake))
        self.assertFalse(any(entry[0]+1==len(groups)-1 for entry in pairs))

    def test_cached_proposal_cannot_bypass_complete_raw_proof(self):
        original=source();old=colors.view(original)
        _common,index,_parts,_locales=installation({'eng':original,'zhs':source('zhs')})
        with tempfile.TemporaryDirectory() as folder:
            cache=Path(folder)
            plan,_=merge.make_plan(old,index,cache,workers=1,minimum_saved=1,minimum_percent=0)
            pair=next(p for p in plan if p.payload is not None)
            path=next(p for p in cache.glob('*.bin') if p.read_bytes()==pair.payload)
            raw=bytearray(merge.decoded_group(pair.payload,43));raw[0]^=1
            packed=lzma.compress(raw,format=lzma.FORMAT_RAW,filters=[dict(id=lzma.FILTER_LZMA2,dict_size=524288)])
            bad=t.GROUP.pack(b'RGR1',len(raw),len(packed),14,1,0,0,0)+packed
            path.write_bytes(bad)
            again,_=merge.make_plan(old,index,cache,workers=1,minimum_saved=1,minimum_percent=0)
            result,mapping=merge.rewrite(old,again)
            with self.assertRaisesRegex(ValueError,'Complete retained raw bytes changed'):
                merge.verify(old,result,again,mapping,decode_groups=True)


if __name__=='__main__':unittest.main()
