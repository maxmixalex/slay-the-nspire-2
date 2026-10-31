"""Pixel/registration checks with generated geometry, never original artwork."""
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
import unittest
import numpy as np
from PIL import Image
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import framed_animation_ui20_assets as f
from world_raster import raster


def quad(x,y,w,h):
    texture=np.array([[[1.,.1,.3,1.],[.3,1.,.1,.6]],[[.1,.3,1.,.4],[1.,1.,1.,1.]]])
    return (np.array([[x,y],[x+w,y],[x+w,y+h],[x,y+h]],dtype=float),
            np.array([[0,0],[1,0],[1,1],[0,1]],dtype=float),
            np.array([[0,1,2],[0,2,3]]),texture,np.ones(4),0)


class FrameRegistrationTests(unittest.TestCase):
    def test_motion_never_reduces_resolution(self):
        first=f.frame_parts([quad(0,0,180,220)],f.NATIVE_DENSITY)
        moved=f.frame_parts([quad(2100,0,180,220)],f.NATIVE_DENSITY)
        self.assertEqual(first[0][0].size,moved[0][0].size)
        self.assertEqual(moved[0][1]-first[0][1],400)
        np.testing.assert_array_equal(np.asarray(first[0][0]),np.asarray(moved[0][0]))

    def test_oversized_frame_parts_reassemble_exact_source_pixels(self):
        draw=[quad(-100,-80,410,270),quad(120,15,160,100)]
        parts=f.frame_parts(draw,1)
        self.assertGreater(len(parts),1)
        expected=raster(draw,1,(102,192),(414,274))
        actual=Image.new('RGBA',expected.size)
        for image,x,y in parts:
            self.assertLessEqual(image.width,320);self.assertLessEqual(image.height,240)
            actual.paste(image,(x+102,y+192))
            # A single whole-frame solve keeps every source alpha8/RGB byte
            # exact across partition boundaries without a quantization map.
            oracle=expected.crop((x+102,y+192,x+102+image.width,y+192+image.height))
            actual_raw=f.sparse_encode(image)
            expected_raw=f.sparse_encode(oracle)
            self.assertEqual(actual_raw,expected_raw)
        np.testing.assert_array_equal(np.asarray(actual)[:,:,3],np.asarray(expected)[:,:,3])

    def test_active_writer_rejects_retired_lossy_profiles(self):
        with TemporaryDirectory() as temporary:
            picture=Image.new('RGBA',(4,3),(19,47,113,17))
            for profile in (1,2,4):
                with self.assertRaises(ValueError):f.pixel_row(Path(temporary),'retired',picture,profile)

    def test_empty_frame_retains_its_clock_without_allocating_a_canvas(self):
        parts=f.frame_parts([],f.NATIVE_DENSITY)
        self.assertEqual([(p.size,x,y) for p,x,y in parts],[((1,1),0,0)])
        self.assertEqual(parts[0][0].getchannel('A').getextrema(),(0,0))

    def test_table_records_signed_world_coordinates_and_exact_clocks(self):
        frames=[[(50000,-312,199)],[(50001,1111,-217),(50002,1431,-217)]]
        with TemporaryDirectory() as temporary:
            cache=Path(temporary);row=f.word_row(cache,'test',frames,79,5376,16149)
            raw=(cache/row['file']).read_bytes();words=struct.unpack('<'+'H'*(len(raw)//2),raw)
            self.assertEqual(words[:10],(f.MAGIC,1,2,79,5376,0,16149,0,23,3))
            self.assertEqual(words[10:14],(14,1,17,2))
            self.assertEqual(struct.unpack_from('<Hhh',raw,28),(50000,-312,199))
            self.assertEqual(struct.unpack_from('<Hhh',raw,34),(50001,1111,-217))
            self.assertEqual(struct.unpack_from('<Hhh',raw,40),(50002,1431,-217))


if __name__=='__main__':unittest.main()
