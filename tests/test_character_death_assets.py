"""Public synthetic checks for death timing, registration and additive planes."""
from pathlib import Path
import sys
import unittest
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import character_death_assets as death
import character_death_particles as particles

class CharacterDeathTest(unittest.TestCase):
    def test_every_authored_frame_including_exact_terminal_pose(self):
        # Binary32 source values are slightly above exact frame boundaries.
        durations=(2.3333335,1.8666668,2.2333335,4.166667,2.3333335)
        for duration,count,ms in zip(durations,death.COUNTS,death.DURATIONS):
            values=death.times(duration)
            self.assertEqual(len(values),count)
            self.assertEqual(values[0],0)
            self.assertEqual(values[-1],duration)
            self.assertEqual(round(duration*1000),ms)
            self.assertTrue(all(a<b for a,b in zip(values,values[1:])))
        self.assertEqual(len(death.times(5,60)),301)

    def test_particle_tail_keeps_global_clock_across_adapter_limit(self):
        class Sampler:
            def __init__(self):self.calls=[]
            def sample(self,resource,clip,values,*,loop):
                self.calls.append((resource,clip,list(values),loop))
                self_test.assertLessEqual(len(values),256)
                return [dict(time=value) for value in values]
        self_test=self;sampler=Sampler();values=death.times(5,60)
        actual=death.sample_bounded(sampler,'regent','die',values)
        self.assertEqual([pose['time'] for pose in actual],values)
        self.assertEqual([len(call[2]) for call in sampler.calls],[256,45])
        self.assertGreater(sampler.calls[1][2][0],sampler.calls[0][2][-1])
        self.assertEqual(actual[-1]['time'],5.)
        self.assertTrue(all(not call[3] for call in sampler.calls))

    def test_registered_sampling_changes_neither_origin_nor_scene_extent(self):
        points=np.array([[-6000.,-1500.],[1800.,4500.]])
        box=death.layout(points,.065)
        self.assertTrue(box['sampling_scale_limited'])
        self.assertLessEqual(box['size'][0],320);self.assertLessEqual(box['size'][1],240)
        row=death.clip_row(3,71,2.3333335,box,.29,'synthetic')
        self.assertGreater(row['sprite_units'],0)
        restored=box['scale']*row['sprite_units']/1024
        self.assertAlmostEqual(restored,.29,delta=box['scale']/2048)
        self.assertEqual(row['first'],3);self.assertEqual(row['count'],71)
        with self.assertRaises(ValueError):death.layout([[float('nan'),1]],1)

    def test_additive_planes_retain_emission_instead_of_alpha_blending(self):
        texture=np.array([[[.6,.3,.1,.5]]])
        sample=(np.array([0.,0.]),np.eye(2),np.ones(4),texture)
        one=np.asarray(particles.render_particles([sample],1.,np.array([.5,.5]),[1,1]))
        twice=np.asarray(particles.render_particles([sample,sample],1.,np.array([.5,.5]),[1,1]))
        self.assertEqual(one.tolist(),[[[76,38,12,255]]])
        self.assertEqual(twice.tolist(),[[[153,76,25,255]]])
        self.assertEqual(np.asarray(particles.render_particles([],1.,np.zeros(2),[2,2])).sum(),0)
        self.assertEqual(particles.particle_bounds([[sample]]).shape,(4,2))

if __name__=='__main__':unittest.main()
