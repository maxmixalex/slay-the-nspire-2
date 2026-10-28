"""Append native title glyphs without changing any existing compressed stream."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import append_ui9_tooltip_glyphs as append
import compact_tile_resources as compact
import content_tiles as tiles
import repack_retained as retained
import split_content as split
from patch_ui7_assets import append_glyphs
from test_compact_tile_resources import grouped_text
from test_repack_retained import fixture


def samples():
    raw,_,_=retained.repack(fixture(),fast_images='none')
    raw=bytearray(raw);struct.pack_into('<I',raw,4,31);raw=bytes(raw)
    grouped=grouped_text(raw)
    glyph_only=bytearray(grouped)
    old=tiles.PackView(raw,1,versions=(31,));mixed=tiles.PackView(grouped,1,versions=(33,))
    for i,(offset,length) in enumerate(old.strings):
        struct.pack_into('<II',glyph_only,mixed.string_at+i*8,offset,length)
    struct.pack_into('<I',glyph_only,4,32)
    transformed,_=compact.repack(grouped,workers=1)
    view=tiles.PackView(transformed,1,versions=(33,));reader=tiles.TileReader(view)
    assert any(transformed[row[0]-32:row[0]-28]==b'TGR2' for row in reader.groups)
    return (raw,bytes(glyph_only),grouped,transformed)


class AppendTooltipGlyphsTest(unittest.TestCase):
    def test_old_raw_and_grouped_keys_alpha_strings_and_streams_are_exact(self):
        additions=[(ord('二'),101,3,5,4,0,bytes(range(15))),
                   (ord('右'),101,3,5,4,2,b'\x0f\xff')]
        for before in samples():
            version=struct.unpack_from('<I',before,4)[0]
            with self.subTest(version=version,bytes=len(before)):
                after,count=append_glyphs(before,1,1,additions,version=version,profile=(1,6111,1,1))
                self.assertEqual(count,2)
                proof=append.verify_append(before,after,1,version)
                self.assertEqual(proof['old_glyphs_verified'],1)
                self.assertEqual(proof['strings_verified'],6111)
                self.assertTrue(proof['exact_old_compressed_groups_and_descriptors'])
                source=tiles.PackView(before,1,versions=(version,));target=tiles.PackView(after,1,versions=(version,))
                old,new=tiles.TileReader(source),tiles.TileReader(target)
                for i,entry in enumerate(source.images):
                    self.assertEqual(old.image_raw(i) if entry[3]==6 else source.image_raw(i),
                        new.image_raw(i) if entry[3]==6 else target.image_raw(i))
                by_key={(r[0],r[2]):i for i,r in enumerate(target.glyphs)}
                self.assertEqual(target.glyph_alpha(by_key[(ord('二'),101)]),bytes(range(15)))
                self.assertEqual(target.glyph_alpha(by_key[(ord('右'),101)]),b'\xff'*15)
                self.assertEqual(append_glyphs(after,1,1,additions,version=version,profile=(1,6111,1,1)),(after,0))
                self.assertEqual(split.layout(before,'eng')['artwork'],split.layout(after,'eng')['artwork'])

    def test_proof_rejects_changed_old_alpha_and_text(self):
        before=samples()[-1]
        after,_=append_glyphs(before,1,1,[(ord('二'),101,1,1,1,0,b'\xff')],version=33,profile=(1,6111,1,1))
        target=tiles.PackView(after,1,versions=(33,))
        for position in (target.glyph_at+12,target.strings[0][0]+8):
            broken=bytearray(after);broken[position]^=1
            with self.assertRaises(ValueError):append.verify_append(before,broken,1,33)


if __name__=='__main__':unittest.main()
