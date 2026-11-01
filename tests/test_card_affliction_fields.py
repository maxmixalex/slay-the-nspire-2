import sys
from pathlib import Path
import struct
import unittest
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import card_affliction_fields as fields

class FieldTests(unittest.TestCase):
    def test_exact_odd_shape_channels_and_zero_tiles(self):
        rng=np.random.default_rng(731)
        for channels in range(1,5):
            a=rng.integers(0,256,(65,97,channels),dtype='u1');a[:32,:32]=0
            raw,proof=fields.encode(a)
            self.assertTrue(np.array_equal(a,fields.decode(raw)))
            self.assertLess(proof['tiles'],proof['directory_tiles'])

    def test_distortion_support_preserves_every_reachable_texel(self):
        alpha=np.zeros((73,91),'u1');alpha[0,0]=255;alpha[36,45]=1;alpha[-1,-1]=255
        used=fields.support(alpha,20,1)
        for y,x in np.argwhere(alpha):
            self.assertTrue(used[max(0,y-1):y+2,max(0,x-20):x+21].all())
        a=np.arange(73*91*3,dtype='u1').reshape(73,91,3)
        raw,_=fields.encode(a,used)
        self.assertTrue(np.array_equal(fields.decode(raw)[used],a[used]))

    def test_invalid_directory_and_padding_rejected(self):
        raw,_=fields.encode(np.ones((33,33,1),'u1'))
        for at,value in ((4,1),(16,33),(24,0),(28,len(raw)+1)):
            bad=bytearray(raw);struct.pack_into('<H' if at==4 else '<I',bad,at,value)
            with self.assertRaises(ValueError):fields.validate(bad)
        bad=bytearray(raw);struct.pack_into('<H',bad,32,65535)
        with self.assertRaises(ValueError):fields.validate(bad)

if __name__=='__main__':unittest.main()
