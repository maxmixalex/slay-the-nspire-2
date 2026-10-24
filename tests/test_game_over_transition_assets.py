"""Projection keeps the authored shader scalar, including its half steps."""
from pathlib import Path
import sys
import unittest
import numpy as np
from PIL import Image
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import patch_game_over_transition_assets as transition


class GameOverTransitionAssetsTest(unittest.TestCase):
    def test_linear_centers_preserve_both_red_taps_without_rgb565_rounding(self):
        y,x=np.indices((1200,2560));red=((x*17+y*29)%256).astype(np.uint8)
        source=Image.fromarray(red,'L')
        actual=np.frombuffer(transition.sample_mask(source),dtype='<u2').reshape(240,320)
        for yy in range(240):
            for xx in range(320):
                want=int(red[yy*5+2,xx*8+3])+int(red[yy*5+2,xx*8+4])
                self.assertEqual(int(actual[yy,xx]),want)
        self.assertTrue(np.any(actual%2))
        self.assertEqual(actual.nbytes,153600)
        with self.assertRaises(ValueError):transition.sample_mask(Image.new('RGB',(320,240)))


if __name__=='__main__':unittest.main()
