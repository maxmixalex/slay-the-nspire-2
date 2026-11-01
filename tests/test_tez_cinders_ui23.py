import copy
import struct
import unittest

import numpy as np

import tez_cinders_ui23 as c


class CinderSourceContract(unittest.TestCase):
    @staticmethod
    def source():
        identity=[.05,0,120,0,.05,80]
        lights=[dict(path=(f'Tezcatara/SpineSprite/SpineBoneNode{i}/light' if i<2 else f'Tezcatara/light{i}'),
                     matrix=identity[:]) for i in range(90)]
        rows=[dict(kind='light',light=light) for light in lights]
        curves=dict(orbit_velocity_curve=[.02]*256,scale_curve=[1.]*256,alpha_curve=[1.]*256)
        for i in range(74):
            sphere=i<2
            rows.append(dict(kind='cinder',path=(f'Tezcatara/SpineSprite/SpineBoneNode{i}/cinder' if sphere else f'Tezcatara/cinder{i}'),
                             matrix=identity[:],process={**c.COMMON_PROCESS,**(c.SPHERE if sphere else c.BOX)},
                             properties=dict(lifetime=10 if sphere else 7,preprocess=6),tint=[1,1,1,1],blend=1,curves=curves))
        frames=[]
        for frame in range(506):
            current=copy.deepcopy(lights)
            for i in range(2):current[i]['matrix'][2]+=frame/256
            frames.append(current)
        return rows,frames,np.zeros((1000,2,2),dtype=np.int16)

    def test_source_roundtrip_and_strict_boundaries(self):
        rows,frames,velocity=self.source()
        raw,proof=c.make_parameters(rows,frames,velocity)
        self.assertEqual(len(raw),26112)
        self.assertEqual(proof['particles'],1776)
        self.assertEqual(struct.unpack_from('<h',raw,c.DELTA_OFFSET+505*12+8)[0],505)
        c.parameters(raw)
        for offset in (0,2,16,30,c.EMITTER_OFFSET+24,c.ORDER_OFFSET+2,c.LENGTH):
            bad=bytearray(raw);bad[offset]^=1
            with self.assertRaises(ValueError):c.parameters(bad)
        for offset,value in ((c.CURVE_OFFSET,1048576),(c.CURVE_OFFSET+1024,-1),(c.CURVE_OFFSET+2048,1048577)):
            bad=bytearray(raw);struct.pack_into('<i',bad,offset,value)
            with self.assertRaises(ValueError):c.parameters(bad)
        with self.assertRaises(ValueError):c.parameters(raw[:-1])
        with self.assertRaises(ValueError):c.parameters(raw+bytes(512))

    def test_changed_source_cannot_be_silently_accepted(self):
        rows,frames,velocity=self.source()
        rows[90]['process']=dict(rows[90]['process'],inherit_velocity_ratio=0.)
        with self.assertRaisesRegex(ValueError,'physics/tint'):c.make_parameters(rows,frames,velocity)
        rows[90]['process']={**c.COMMON_PROCESS,**c.SPHERE}
        frames[-1][0]['matrix'][2]+=1000
        with self.assertRaisesRegex(ValueError,'delta exceeds'):c.make_parameters(rows,frames,velocity)


if __name__=='__main__':unittest.main()
