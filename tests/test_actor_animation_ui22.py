"""Exact registered actor pieces and sparse auxiliary references; no source art."""
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
import unittest

import numpy as np
from PIL import Image

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import actor_animation_ui22 as actor
import content_tiles as tiles
from lz4_block import decode as lz4_decode
from patch_frontend_assets import auxiliary_image_ids
from sparse_image import encode as sparse_encode


class ExactActorPieces(unittest.TestCase):
    def test_split_preserves_every_visible_rgba_byte(self):
        y,x=np.indices((319,681))
        rgba=np.stack((x%256,y%256,(x*17+y*29)%256,(x+y)%256),axis=2).astype('u1')
        image=Image.fromarray(rgba,'RGBA');parts=actor.exact_parts(image,-451,-113)
        self.assertGreater(len(parts),1)
        rebuilt=Image.new('RGBA',image.size)
        for picture,left,top in parts:
            self.assertLessEqual(picture.width,320);self.assertLessEqual(picture.height,240)
            rebuilt.paste(picture,(left+451,top+113))
        actual=np.asarray(rebuilt);visible=rgba[:,:,3]>0
        np.testing.assert_array_equal(actual[visible],rgba[visible])
        np.testing.assert_array_equal(actual[:,:,3],rgba[:,:,3])

    def test_sparse_storage_does_not_reduce_color_or_alpha(self):
        rgba=np.array([[[29,37,43,1],[57,78,99,2],[111,143,173,254],
                        [217,231,249,255]]],dtype='u1')
        picture=Image.fromarray(rgba,'RGBA')
        with TemporaryDirectory() as temporary:
            cache=Path(temporary);row=actor.pixel_row(cache,'exact',picture,8)
            raw=(cache/row['file']).read_bytes()
            if row['format']==5:
                fmt,size=struct.unpack_from('<BI',raw)
                self.assertEqual(fmt,4);raw=lz4_decode(raw[5:],size)
            self.assertEqual(raw,sparse_encode(picture))
            _,colors,alpha=tiles.validate_sparse(raw,4,1,pixels=True)
            original=rgba.astype('u2')
            expected=((original[:,:,0]>>3)<<11)|((original[:,:,1]>>2)<<5)|(original[:,:,2]>>3)
            np.testing.assert_array_equal(colors,expected)
            np.testing.assert_array_equal(alpha,rgba[:,:,3])

    def test_travel_changes_registration_without_shrinking_picture(self):
        picture=Image.new('RGBA',(95,134),(13,99,211,173))
        first=actor.exact_parts(picture,-20,-101)
        later=actor.exact_parts(picture,1740,-101)
        self.assertEqual(first[0][0].tobytes(),later[0][0].tobytes())
        self.assertEqual(first[0][0].size,later[0][0].size)
        self.assertEqual(later[0][1]-first[0][1],1760)

    def test_empty_frame_keeps_one_zero_alpha_sample(self):
        parts=actor.exact_parts(Image.new('RGBA',(701,305)),-88,-97)
        self.assertEqual([(p.size,x,y) for p,x,y in parts],[((1,1),0,0)])
        self.assertEqual(parts[0][0].getchannel('A').getextrema(),(0,0))


class AuxiliaryReferences(unittest.TestCase):
    def test_legacy_relative_rows_unchanged(self):
        self.assertEqual(auxiliary_image_ids({'auxiliary_rows':[2,0]},100,3,2),[102,100])

    def test_existing_image_references_do_not_require_dummy_images(self):
        self.assertEqual(auxiliary_image_ids({'auxiliary_image_ids':[0,103,0,2]},100,4,4),[0,103,0,2])
        self.assertEqual(auxiliary_image_ids({'auxiliary_image_ids':[2,4,6]},100,0,3),[2,4,6])

    def test_absolute_references_keep_bounds_and_reject_ambiguity(self):
        for manifest in ({'auxiliary_image_ids':[104]},
                         {'auxiliary_image_ids':[-1]},
                         {'auxiliary_image_ids':[2],'auxiliary_rows':[0]}):
            with self.assertRaises(ValueError):auxiliary_image_ids(manifest,100,4,1)


if __name__=='__main__':unittest.main()
