from pathlib import Path
import struct,sys,unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import neow_particles_ui23 as p

def parameter():
    raw=bytearray(p.BYTES)
    struct.pack_into('<4s6H8I8B8x',raw,0,b'NNP1',1,p.ATLAS_FIRST,4,32,32,3,
                     64,1088,2112,2624,2880,2910,3422,4446,145,185,194,255,226,187,71,113)
    for i,offset in enumerate((0,16963,65535)):
        struct.pack_into('<5H',raw,2880+i*10,offset,65535,65535,65535,65535)
    raw[2910:3422]=b'\xff'*512
    return raw

class NeowParticles(unittest.TestCase):
    def test_shared_mask_reference_and_native_grain_regions(self):
        self.assertEqual(p.validate_parameter(parameter(),p.IMAGES),tuple(range(59712,59716)))
        self.assertEqual(p.BYTES,4608)

    def test_corrupt_regions_padding_tints_and_gradient_fail(self):
        for offset in (0,4,6,8,10,12,14,16,20,24,28,32,36,40,44,48,52,56,2880,2910,4446,4607):
            raw=parameter();raw[offset]^=1
            with self.subTest(offset=offset),self.assertRaises(Exception):p.validate_parameter(raw,p.IMAGES)
        for value in (0,65535):
            raw=parameter();struct.pack_into('<H',raw,2890,value)
            with self.assertRaises(Exception):p.validate_parameter(raw,p.IMAGES)
        with self.assertRaises(Exception):p.validate_parameter(parameter(),59715)

    def test_only_exact_catalog_is_accepted(self):
        from types import SimpleNamespace
        source=SimpleNamespace(aux=[0]*p.AUXILIARY,images=[0]*p.IMAGES)
        source.aux[p.BASE_AUX]=p.BASE_IMAGES
        self.assertEqual(p.validate(source,lambda i:(256,9,0,parameter()))['auxiliary'],(7226,))
        for name in ('aux','images'):
            getattr(source,name).append(0)
            with self.assertRaises(Exception):p.validate(source,lambda i:(256,9,0,parameter()))
            getattr(source,name).pop()

if __name__=='__main__':unittest.main()
