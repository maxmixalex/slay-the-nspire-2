"""Source-free tests for the bounded ordered Regent atlas, not original art."""
import sys
import unittest
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from fake_merchant_ui13_assets import regent_atlas, invariant_planes, quantized, _image


def quad(color, blend=0, x=92, y=77):
    vertices = np.array([[x,-y],[x+8,-y],[x+8,-y-6],[x,-y-6]],dtype=float)
    uv = np.array([[0,0],[1,0],[1,1],[0,1]],dtype=float)
    texture = np.empty((2,2,4));texture[:]=color
    return vertices,uv,np.array([[0,1,2],[0,2,3]]),texture,np.ones(4),blend


class FakeMerchantOrderedAtlasTests(unittest.TestCase):
    def test_partial_body_keeps_straight_add_alpha_and_original_order(self):
        draws=[quad((1,0,.25,.5)),quad((.25,1,.5,.5),1),quad((0,.25,1,.25))]
        # This is exactly the class of source edge that cannot be flattened
        # into one normal plane over an arbitrary room background.
        with self.assertRaisesRegex(ValueError,'separate ordered plane'):
            _image(draws,1,np.zeros(2))
        atlas=np.asarray(regent_atlas(draws,1,np.zeros(2)))
        self.assertEqual(atlas.shape,(196,160,4))
        expected=[np.array([255,0,63,127]),np.array([63,255,127,127]),np.array([0,63,255,63])]
        self.assertFalse(np.any(atlas[77:83,92:100]))
        bands=[atlas[190:196,0:8],atlas[190:196,8:16],atlas[190:196,16:24]]
        for actual,value in zip(bands,expected):
            np.testing.assert_array_equal(actual,np.broadcast_to(value,actual.shape))
        # In particular the stored green remains255, not127 from baking Add
        # against transparent black and inadvertently multiplying it twice.
        for background in (np.zeros(3),np.ones(3),np.full(3,.5)):
            result=background.copy()
            for value,mode in zip(expected,(0,1,0)):
                color,alpha=value[:3]/255,value[3]/255
                result=np.minimum(1,result+color*alpha) if mode else color*alpha+result*(1-alpha)
            before=expected[0][:3]/255*(127/255)+background*(128/255)
            lit=np.minimum(1,before+expected[1][:3]/255*(127/255))
            reference=expected[2][:3]/255*(63/255)+lit*(192/255)
            np.testing.assert_allclose(result,reference,atol=1e-15)
            wrong=np.minimum(1,before+expected[1][:3]/255*(127/255)**2)
            wrong=expected[2][:3]/255*(63/255)+wrong*(192/255)
            self.assertGreater(float(np.max(np.abs(reference-wrong))),.1)

    def test_factoring_preserves_each_ordered_band_without_overlap(self):
        frames=[]
        for alpha in (.25,.5,.75):
            frames.append(regent_atlas([quad((1,0,0,1)),quad((0,1,0,alpha),1),quad((0,0,1,.25))],1,np.zeros(2)))
        fixed,moving,_=invariant_planes(frames);a=quantized(fixed)
        for source,dynamic in zip(frames,moving):
            b=quantized(dynamic)
            self.assertFalse(np.any((a>>16!=0)&(b>>16!=0)))
            np.testing.assert_array_equal(a|b,quantized(source))

    def test_changed_eye_registration_is_rejected(self):
        with self.assertRaisesRegex(ValueError,'escaped'):
            regent_atlas([quad((1,1,1,1)),quad((1,1,1,.5),1,x=91)],1,np.zeros(2))


if __name__=='__main__':unittest.main()
