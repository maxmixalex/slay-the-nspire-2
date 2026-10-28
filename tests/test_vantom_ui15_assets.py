"""Independent synthetic geometry checks; no original assets are embedded."""
import copy
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch
from types import SimpleNamespace

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from lz4_block import decode
import vantom_ui15_assets as v


def mesh(index, points):
    return dict(slot=str(index), index=index, attachment='triangle', region='triangle', page='fixture',
                premultiplied_alpha=False, vertices=np.asarray(points).reshape(-1).tolist(),
                uvs=[0, 0, 1, 0, 0, 1], triangles=[0, 1, 2], tint=[1, 1, 1, 1], dark=None, blend=0)


class VantomFactoringTests(unittest.TestCase):
    def test_source_topology_and_runtime_scratch_bounds_match(self):
        header=(Path(__file__).resolve().parents[1]/'include/vantom_ui15_resources.hpp').read_text()
        self.assertIn(f'VantomUi15MaxMeshVertices={v.MAX_MESH_VERTICES}',header)
        self.assertIn(f'VantomUi15MaxMeshTriangles={v.MAX_MESH_TRIANGLES}',header)
        for vertices,triangles in [(v.MAX_MESH_VERTICES+1,1),(3,v.MAX_MESH_TRIANGLES+1)]:
            points=np.zeros((vertices,2))
            draw=(points,np.zeros((vertices,2)),np.array([[0,1,2]]*triangles),np.ones((2,2,4)),np.ones(4),0)
            with TemporaryDirectory() as temporary:
                with self.assertRaisesRegex(ValueError,'mesh bound'):
                    v.mesh_asset(draw,[points,points],[512,1024],Path(temporary),'bound')

    def test_frame_reuse_requires_exclusive_vantom_ownership(self):
        source = SimpleNamespace(clips=[(10, 2), (12, 3), (20, 2)], cards=[(3,)], aux=[4])
        self.assertEqual(v.reusable_image_slots(source, [{'id': 0}, {'id': 1}]), list(range(10, 15)))
        source.clips[2] = (14, 2)
        with self.assertRaises(ValueError):v.reusable_image_slots(source, [{'id': 0}, {'id': 1}])
        source.clips[2] = (20, 2);source.aux.append(12)
        with self.assertRaises(ValueError):v.reusable_image_slots(source, [{'id': 0}, {'id': 1}])
        source.aux.pop();source.cards.append((11,))
        with self.assertRaises(ValueError):v.reusable_image_slots(source, [{'id': 0}, {'id': 1}])

    def test_layers_fill_old_frame_slots_before_growing_directory(self):
        self.assertEqual([v.layer_image_slot([32167, 32168], 54350, i) for i in range(4)],
                         [32167, 32168, 54350+v.PARAMETER_COUNT, 54351+v.PARAMETER_COUNT])
        with self.assertRaises(ValueError):v.layer_image_slot([], 65535, 0)

    def frames(self):
        body = np.array([[10., 12.], [30., 12.], [10., 32.]])
        tail = np.array([[100., 200.], [110., 200.], [100., 210.]])
        blade = np.array([[-100., 150.], [-90., 150.], [-100., 160.]])
        return [dict(draw=[mesh(0, body*s), mesh(1, tail+(1-s)*np.array([-20., -30.])), mesh(2, blade)])
                for s in v.SCALES]

    def test_every_intermediate_scale_must_obey_the_source_dependency(self):
        frames = self.frames()
        kinds, tail, error, vertices = v.classify_frame(frames)
        self.assertEqual(kinds, [0, 1, 2])
        np.testing.assert_equal(tail, [-20, -30])
        self.assertEqual(error, 0)
        self.assertEqual(vertices, 9)
        # Matching both endpoints alone must not admit a distorted midpoint.
        frames[2]['draw'][0]['vertices'][0] += 2
        with self.assertRaises(ValueError):
            v.classify_frame(frames)

    def test_scale_dependent_material_or_topology_is_rejected(self):
        for field, changed in (('tint', [1, 1, 1, .5]), ('triangles', [0, 2, 1])):
            frames = self.frames()
            frames[1]['draw'][0][field] = changed
            with self.assertRaises(ValueError):
                v.classify_frame(frames)

    def test_invisible_particle_slot_splits_composites_at_actual_draw_rank(self):
        draw = [(np.zeros((3, 2)), None, None, None, None, 0)]*2
        raw = dict(draw=[dict(slot='body'), dict(slot='back')], draw_order=['body', 'emitter', 'back'])
        groups = v.ordered_groups(draw, [0, 0], raw, [2])
        self.assertEqual(len(groups), 2)
        self.assertEqual([g[2] for g in groups], [0, 2])

    def test_dynamic_draw_order_and_shader_boundaries_are_preserved(self):
        draw = [(np.zeros((3, 2)), None, None, None, None, 0)]*4
        raw = dict(draw=[dict(slot=name) for name in ('back', 'body', 'tail', 'tail_hook')],
                   draw_order=['back', 'body', 'tail', 'tail_hook'])
        groups = v.ordered_groups(draw, [0]*4, raw, [])
        self.assertEqual([len(group[1]) for group in groups], [2, 1, 1])
        self.assertEqual([group[2] for group in groups], [0, 2, 3])
        raw['draw_order'] = ['body', 'back', 'tail', 'tail_hook']
        with self.assertRaises(ValueError):v.ordered_groups(draw, [0]*4, raw, [])

    def test_nonlinear_source_mesh_uses_verified_midpoints(self):
        def source(sampler, model, job, scales=v.SCALES):
            result=[]
            for scale in scales:
                points=np.array([[10.,12.],[30.,12.],[10.,32.]])*scale
                points[1,1]+=40*(scale-.5)**2
                result.append([dict(draw=[mesh(0,points)],draw_order=['0'])])
            return result
        anchors=[dict(matrix=[1048576,0,0,1048576,0,0],depth=0,flags=2)]*4
        with patch.object(v,'raw_samples',source),patch.object(v,'emitter_samples',return_value=anchors):
            raw,classes,selected,emitter_keys,proof=v.sampled_geometry(None,{},dict(id=1,frames=1),np.eye(3),1.)
        self.assertEqual(classes[0][0],[3])
        self.assertGreater(len(selected[0,0]),2)
        self.assertLessEqual(proof['maximum_lcd_interpolation_error'],v.LCD_GEOMETRY_TOLERANCE)
        # Each source midpoint remains independent from the saved knots.
        self.assertGreater(len(raw),len(selected[0,0]))

    def test_discontinuous_source_retains_adjacent_q10_states(self):
        def source(sampler,model,job,scales=v.SCALES):
            result=[]
            for scale in scales:
                points=np.array([[10.,12.],[30.,12.],[10.,32.]])*scale
                if round(scale*1024)<820:points[1,1]+=12
                result.append([dict(draw=[mesh(0,points)],draw_order=['0'])])
            return result
        anchors=[dict(matrix=[1048576,0,0,1048576,0,0],depth=0,flags=2)]*4
        with patch.object(v,'raw_samples',source),patch.object(v,'emitter_samples',return_value=anchors):
            raw,classes,selected,emitter_keys,proof=v.sampled_geometry(None,{},dict(id=1,frames=1),np.eye(3),1.)
        self.assertIn(819,selected[0,0]);self.assertIn(820,selected[0,0])
        self.assertLessEqual(proof['maximum_lcd_interpolation_error'],v.LCD_GEOMETRY_TOLERANCE)
        self.assertLess(len(raw),50)

    def test_mesh_words_preserve_uv_topology_and_source_positions(self):
        uv=np.array([[0.,0.],[1.,0.],[0.,1.]])
        points=np.array([[1.,2.],[17.,2.],[1.,18.]])
        draw=(points,uv,np.array([[0,1,2]]),np.ones((5,7,4)),np.ones(4),0)
        with TemporaryDirectory() as temporary:
            words,picture,mask=v.mesh_asset(draw,[points*.5,points],[512,1024],Path(temporary),'mesh')
            self.assertEqual(words[:5],[v.MESH_MAGIC,1,3,1,2])
            self.assertEqual(words[words[7]:words[7]+3],[0,1,2])
            self.assertEqual(words[words[6]:words[6]+6],[0,0,65535,0,0,65535])
            self.assertEqual(words[words[8]:words[8]+6],np.rint(points*.5*64).astype(int).reshape(-1).tolist())
            self.assertEqual(words[9],len(words));self.assertTrue((Path(temporary)/picture).is_file());self.assertIsNone(mask)

    def test_private_parameter_words_survive_the_existing_image_codec(self):
        sample = dict(frames=1, frame_ms=77, sprite_units=22598, root_q8=[0, -1250],
            records=[dict(tail_q8=[-5000, 1600], layers=[dict(file='body.png', x=-20, y=-50, mode=0, depth=2),
                dict(file='blade.png', x=40, y=-90, mode=2, depth=9, mask='noise.png')],
                emitter_keys=[512,1024], emitters=[[dict(matrix=[1048576, -1234, 4321, 1048576, -192000, 62500], depth=i, flags=2) for i in range(4)] for q in range(2)])])
        with TemporaryDirectory() as temporary:
            path = Path(temporary)
            row = v.parameter_image(path, 'parameters', sample, {'body.png': 32167, 'blade.png': 36246, 'noise.png': 36247})
            payload = (path/row['file']).read_bytes()
            if row['format'] == 5:
                self.assertEqual(payload[0], 0)
                payload = decode(payload[5:], struct.unpack_from('<I', payload, 1)[0])
            self.assertEqual(len(payload), row['width']*row['height']*2)
            words = struct.unpack('<'+'H'*(len(payload)//2), payload)
            self.assertEqual(words[:6], (v.MAGIC, 2, 1, 77, 22598, 0))
            self.assertEqual(words[12:17], (17, 2, (-5000)&65535, 1600, 29))
            self.assertEqual(words[17:29], (32167, (-20)&65535, (-50)&65535, 0, 2, 65535,
                                          36246, 40, (-90)&65535, 2, 9, 36247))
            self.assertEqual(struct.unpack_from('<6i', payload, 32*2), (1048576, -1234, 4321, 1048576, -192000, 62500))
            # Image indices are unsigned; transform positions are signed.
            # A positive overflow must not silently become a negative offset.
            sample['records'][0]['tail_q8'][0] = 32768
            with self.assertRaises(ValueError):
                v.parameter_image(path, 'bad-position', sample, {'body.png': 32167, 'blade.png': 36246, 'noise.png': 36247})


if __name__ == '__main__':
    unittest.main()
