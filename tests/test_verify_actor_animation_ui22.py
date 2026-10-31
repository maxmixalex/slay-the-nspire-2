from pathlib import Path
import sys
import unittest
from PIL import Image
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from verify_actor_animation_ui22 import exact_source


class WholeFrameSourceProof(unittest.TestCase):
    def test_empty_source_frame_has_no_visible_registration(self):
        exact_source((Image.new('RGBA',(16,12),(221,117,53,0)),-49,-75),
                     (Image.new('RGBA',(1,1)),0,0))

    def test_empty_sentinel_never_hides_visible_original_pixels(self):
        with self.assertRaises(ValueError):
            exact_source((Image.new('RGBA',(16,12),(29,47,113,1)),-49,-75),
                         (Image.new('RGBA',(1,1)),0,0))

    def test_visible_alpha_and_registration_remain_exact(self):
        expected=Image.new('RGBA',(9,8));expected.paste((29,47,113,1),(2,3,5,6))
        picture=expected.crop((2,3,5,6))
        exact_source((expected,-10,-20),(picture,-8,-17))
        with self.assertRaises(ValueError):exact_source((expected,-10,-20),(picture,-7,-17))


if __name__=='__main__':unittest.main()
