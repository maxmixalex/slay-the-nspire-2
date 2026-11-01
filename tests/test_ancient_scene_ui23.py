"""Exact scene patches, loop boundaries and compact metadata rejection."""
from pathlib import Path
import struct
import sys
import unittest

import numpy as np
from PIL import Image

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import ancient_scene_ui23 as ancient
from content_tiles import validate_sparse


class AncientScene(unittest.TestCase):
    def test_long_loop_batches_keep_absolute_source_time(self):
        class Sampler:
            def __init__(self):self.calls=[]
            def sample(self,resource,clip,times,**kwargs):
                self.calls.append(times)
                self.assertion=len(times)<=256 and kwargs['loop']
                if not self.assertion:raise ValueError('Invalid batch')
                return list(times)
        sampler=Sampler();scene=ancient.SCENES[0]
        times=[frame*33/1000 for frame in range(546)]
        loop=ancient.SampledLoop(sampler,scene,times)
        self.assertEqual([len(call) for call in sampler.calls],[256,256,34])
        self.assertEqual(loop.frames,times)
        self.assertEqual(loop.sample(scene['resource'],scene['clip'],[times[545]]),[times[545]])

    def test_original_loop_duration_and_tail_tick(self):
        for duration, count in ((18000,546),(16667,506)):
            raw=struct.pack('<8H',ancient.MAGIC,1,58613,count,33,duration,0,0)
            row=ancient.parameters(raw,60000)
            self.assertEqual(row['frames'],count)
            self.assertEqual(row['key_interval'],8)
            words=list(struct.unpack('<8H',raw));words[7]=32
            self.assertEqual(ancient.parameters(struct.pack('<8H',*words),60000)['key_interval'],32)
            words[7]=16
            with self.assertRaises(ValueError):ancient.parameters(struct.pack('<8H',*words),60000)
            self.assertEqual((duration-1)//33,count-1)
            with self.assertRaises(ValueError):ancient.parameters(raw,58613+count-1)
        with self.assertRaises(ValueError):
            ancient.parameters(struct.pack('<8H',ancient.MAGIC,1,58613,505,33,16667,0,0),60000)

    def test_random_seek_and_sequential_patches_are_exact(self):
        rng=np.random.default_rng(41)
        frame=rng.integers(0,65536,size=(240,320),dtype=np.uint16)
        full=[];patches=[]
        for index in range(27):
            frame=frame.copy();frame[20:26,15+index:23+index]^=12345
            reference=None if index==0 else full[0] if index%8==0 else full[-1]
            patches.append(ancient.encode_patch(frame,reference));full.append(frame)
        def apply(destination,index):
            _,color,alpha=validate_sparse(patches[index],320,240,pixels=True)
            self.assertTrue(np.all((alpha==0)|(alpha==255)))
            destination[alpha!=0]=color[alpha!=0]
        for target in (0,26,15,16,7,8,24,9,1):
            restored=np.zeros((240,320),dtype=np.uint16);apply(restored,0)
            key=target-target%8
            if key:apply(restored,key)
            for index in range(key+1,target+1):apply(restored,index)
            np.testing.assert_array_equal(restored,full[target])

    def test_black_backing_uses_integer_rgb565_alpha(self):
        image=Image.fromarray(np.array([[[255,255,255,255],[255,255,255,0],
                                         [255,0,0,128],[0,255,0,128]]],dtype='u1'))
        np.testing.assert_array_equal(ancient.rgb565(image),[[65535,0,16<<11,32<<5]])

    def test_cyclic_hold_bounds_and_exact_fallback(self):
        rng=np.random.default_rng(41)
        channels=rng.integers(0,5,size=(30,3,7,3),dtype=np.int16)+np.array([10,25,10])
        frames=[((c[:,:,0]<<11)|(c[:,:,1]<<5)|c[:,:,2]).astype('<u2') for c in channels]
        chosen,report=ancient.held_colors(frames)
        self.assertTrue(report['cyclic_seam_verified'])
        self.assertLessEqual(report['warm_cycles'],4)
        self.assertTrue(np.all(np.array(report['maximum_rgb565_channel_error'])<=[1,2,1]))
        again,other=ancient.held_colors(frames)
        self.assertEqual(report,other)
        for source,output,repeated in zip(frames,chosen,again):
            np.testing.assert_array_equal(output,repeated)
            self.assertEqual(source.shape,output.shape)
        unchanged=[np.full((2,3),0x1234,dtype='<u2') for _ in range(4)]
        output,proof=ancient.held_colors(unchanged)
        self.assertEqual(proof['warm_cycles'],1);self.assertEqual(proof['exact_fallback_pixels'],0)
        for frame in output:np.testing.assert_array_equal(frame,unchanged[0])


if __name__=='__main__':unittest.main()
