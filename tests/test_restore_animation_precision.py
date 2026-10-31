"""Source provenance must precede restoration of original color samples."""
from pathlib import Path
import sys
import unittest
from types import SimpleNamespace

import numpy as np
from PIL import Image

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from restore_animation_precision import matching_precision,changed_owners
from quantize_animation_resources import quantize_image
from sparse_image import encode


class PrecisionProvenance(unittest.TestCase):
    def source(self):
        y,x=np.indices((7,19));rgba=np.stack(((x*13+y*7)%256,(x*17+y*19)%256,
            (x*23+y*41)%256,(x*29+y*47)%256),axis=2).astype('u1')
        return 19,7,4,encode(Image.fromarray(rgba,'RGBA'))

    def test_all_historical_maps_require_complete_source_equivalence(self):
        original=self.source()
        for profile in (1,2,4):
            current=(*original[:3],quantize_image(original[3],*original[:3],profile))
            self.assertEqual(matching_precision(original,current),profile)
        self.assertEqual(matching_precision(original,original),8)

    def test_new_source_or_geometry_is_never_assumed_to_be_quantization(self):
        original=self.source();current=bytearray(original[3]);current[-1]^=8
        self.assertIsNone(matching_precision(original,(*original[:3],bytes(current))))
        self.assertIsNone(matching_precision(original,(20,*original[1:])))

    def test_later_registration_and_clock_changes_exclude_the_whole_clip(self):
        a=SimpleNamespace(clips=[(100,3,42,12,13,5120),(103,2,35,1,2,0)])
        for field in range(6):
            changed=list(a.clips[0]);changed[field]+=1
            b=SimpleNamespace(clips=[tuple(changed),a.clips[1]])
            self.assertEqual(changed_owners(a,b),{0})
        self.assertEqual(changed_owners(a,a),set())


if __name__=='__main__':unittest.main()
