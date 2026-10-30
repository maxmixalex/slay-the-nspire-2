"""Exact tile aliasing, active-group constraints, relocation and cache bounds."""
from pathlib import Path
import random
import struct
import sys
import tempfile
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as t
import coalesce_tile_resources as coalesce
from test_repack_retained import stored


def fixture():
    ni, ng, ns, nc, nk, nu = 2, 1, 1, 1, 1, 1
    base = 40 + ni*16 + ng*16 + ns*8 + nc*16 + nk*16 + nu*4
    table = base + 64
    costs = table + 3*8
    descriptors = costs + 2
    out = bytearray(descriptors + 2*16)
    randomizer = random.Random(14203)
    a = bytearray(b'\x10\x10' + randomizer.randbytes(768))
    a[514] = 0  # Nonzero hidden RGB under zero alpha must stay exact.
    b = b'\x10\x10' + randomizer.randbytes(768)
    c = b'\x10\x10' + randomizer.randbytes(768)
    geometry = struct.pack('<HHHB', 1, 0, 32, 1)*16
    frames = [t.FRAME.pack(16*(2+5+32*3), 8, len(geometry)) +
              struct.pack('<II', left, 1) + geometry for left in (0,2)]
    raw_groups = [(1, bytes(a)+b, 2), (1, bytes(a)+c, 2), (2,b''.join(frames),2)]
    for group,(kind,raw,count) in enumerate(raw_groups):
        out.extend(bytes(len(out)&1)); at=len(out)
        encoded=stored(raw)
        payload=t.GROUP.pack(b'TGR1',kind,group,len(raw),len(encoded),0,count,0)+encoded
        out.extend(payload); struct.pack_into('<II',out,table+group*8,at,len(payload))
    for i in range(ni):
        t.IMAGE.pack_into(out,40+i*16,descriptors+i*16,32,16,6,16)
        t.DESCRIPTOR.pack_into(out,descriptors+i*16,base,0,i*len(frames[0]),len(frames[i]))
    out.extend(bytes(len(out)&1)); glyph=len(out); out.extend(b'\xa6')
    t.GLYPH.pack_into(out,40+ni*16,65,glyph,8,1,1,1,0)
    out.extend(bytes(len(out)&1)); string=len(out); out.extend(b'eng\0')
    string_at=40+ni*16+ng*16
    struct.pack_into('<II',out,string_at,string,3)
    t.CLIP.pack_into(out,string_at+ns*8,0,2,100,0,0,0)
    t.GLOBAL.pack_into(out,base,b'TIL1',1,16,14,4,2,1,2,descriptors,table,
                       770,2,t.GROUP_LIMIT,t.ENCODED_LIMIT,costs,ni)
    out[costs:costs+ni]=bytes((1,1))
    t.HEADER.pack_into(out,0,b'NSP2',41,t.REFERENCE,len(out),ni,ng,ns,nc,nk,0)
    return bytes(out)


class CoalesceTilesTest(unittest.TestCase):
    def test_active_geometry_matches_runtime_tiles(self):
        geometry=b''.join(struct.pack('<HHHB',1,15,2,mode) for mode in (0,1))
        self.assertEqual(coalesce.active_tiles(geometry,32,2).tolist(),[True,True])
        self.assertEqual(coalesce.active_tiles(bytes(34),32,17).tolist(),[False]*4)
        last=bytes(32)+struct.pack('<HHHB',1,31,1,1)
        self.assertEqual(coalesce.active_tiles(last,32,17).tolist(),[False,False,False,True])

    def test_bad_geometry_is_rejected(self):
        for raw in (b'',b'\x00',b'\x00\x00x',struct.pack('<HHHB',1,0,33,1),
                    struct.pack('<HHHB',1,0,0,1),struct.pack('<HHHB',1,0,1,2)):
            with self.assertRaises(ValueError): coalesce.active_tiles(raw,32,1)

    def test_every_active_use_constrains_the_destination(self):
        canonical=np.array((0,1,0,3,4,1),dtype='<u4')
        frames=[(0,0,np.array((2,1),dtype='<u4')),(0,1,np.array((5,3),dtype='<u4'))]
        mapping,proof=coalesce.constrained_mapping(canonical,frames,2)
        self.assertEqual(int(mapping[2]),0)
        self.assertEqual(int(mapping[5]),5)
        self.assertEqual(proof['frame_group_occurrences_before'],4)
        self.assertEqual(proof['frame_group_occurrences_after'],3)
        self.assertTrue(proof['no_active_frame_adds_a_group'])
        frames.append((0,2,np.array((2,3),dtype='<u4')))
        mapping,_=coalesce.constrained_mapping(canonical,frames,2)
        self.assertEqual(int(mapping[2]),2)

    def test_inactive_only_records_can_alias_without_visible_reads(self):
        mapping,proof=coalesce.constrained_mapping(np.array((0,1,0,3),dtype='<u4'),[],2)
        self.assertEqual(mapping.tolist(),[0,1,0,3])
        self.assertEqual(proof['active_frames'],0)

    def test_invalid_or_chained_canonical_maps(self):
        for values in ([],[1],[1,2,2],[1,0]):
            with self.assertRaises(ValueError):
                coalesce.constrained_mapping(np.array(values,dtype='<u4'),[],2)

    def test_full_conversion_preserves_pixels_hidden_rgb_aliases_and_metadata(self):
        original=fixture(); source=coalesce.view(original); before=t.TileReader(source)
        with tempfile.TemporaryDirectory() as folder:
            plan,mapping,keep,raw,report=coalesce.make_plan(source,Path(folder),workers=1)
            self.assertEqual(int(mapping[2]),0)
            self.assertGreater(report['aligned_bytes_saved'],0)
            output,_=coalesce.rewrite(original,plan)
            proof=coalesce.verify(original,output,plan,mapping,keep,raw)
            after=t.TileReader(coalesce.view(output))
            for i in range(len(source.images)):
                self.assertEqual(before.image_raw(i),after.image_raw(i))
            self.assertEqual(after.group(0)[0],before.group(0)[0])
            self.assertEqual(proof['images'],2)
            self.assertEqual(proof['glyphs'],1)
            self.assertEqual(proof['strings'],1)
            self.assertEqual(proof['saved_bytes'],report['aligned_bytes_saved'])
            self.assertEqual(report['resident_bytes_added'],0)
            del raw

    def test_foreign_artwork_plan_is_rejected(self):
        original=fixture()
        with tempfile.TemporaryDirectory() as folder:
            plan,_map,_keep,raw,_report=coalesce.make_plan(coalesce.view(original),Path(folder),workers=1)
            wrong=dict(plan); a,b,c=wrong[0]; wrong[0]=(b'x'+a[1:],b,c)
            with self.assertRaisesRegex(ValueError,'artwork differs'): coalesce.rewrite(original,wrong)
            del raw

    def test_corrupt_encoder_cache_cannot_change_output(self):
        original=fixture()
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)
            _plan,_mapping,_keep,raw,_report=coalesce.make_plan(coalesce.view(original),path,workers=1)
            del raw
            cached=list((path/'encoded').glob('*.lzma2')); self.assertTrue(cached)
            cached[0].write_bytes(b'invalid')
            with self.assertRaises(ValueError): coalesce.make_plan(coalesce.view(original),path,workers=1)


if __name__=='__main__': unittest.main()
