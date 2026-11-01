import sys
import unittest
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from actor_slot_materials import NecrobinderSlash, apply_slot_materials
from world_raster import raster


class SlotMaterials(unittest.TestCase):
    def fixture(self, noise=.5):
        shader = NecrobinderSlash.__new__(NecrobinderSlash)
        shader.parameters = dict(ColorParameter=(.1, .6, .9, 1), step=(.2, .21),
                                 noise_1_strength=1, noise_2_strength=.1,
                                 master_step=.085, opacity=.25)
        shader.textures = dict(shape_mask_tex=np.ones((1, 1, 4), np.float32),
                              noise_1_tex=np.full((2, 2, 4), noise, np.float32),
                              noise_2_tex=np.ones((2, 2, 4), np.float32))
        shader.started = 1 / 6
        shader.time = 0
        return shader

    def test_event_clock_and_exact_fade_endpoint(self):
        self.assertEqual(NecrobinderSlash.tween(0), (.04, .4))
        self.assertAlmostEqual(NecrobinderSlash.tween(.2)[0], .08875)
        self.assertAlmostEqual(NecrobinderSlash.tween(.225)[1], .2)
        self.assertEqual(NecrobinderSlash.tween(.45), (.235, 0))
        self.assertEqual(NecrobinderSlash.tween(10), (.235, 0))
        shader = self.fixture()
        uv = np.array([[.15, .75], [.3, .1]], np.float32)
        self.assertTrue(np.allclose(shader.at(0)(uv)[:, 3], .25))
        self.assertTrue(np.allclose(shader.at(shader.started)(uv)[:, 3], .4))
        self.assertTrue(np.allclose(shader.at(shader.started + .225)(uv)[:, 3], .2))
        self.assertTrue(np.all(shader.at(shader.started + .45)(uv)[:, 3] == 0))

    def test_source_mask_blue_and_noise_red_channels(self):
        shader = self.fixture(.1)
        uv = np.array([[.5, .5]], np.float32)
        # At the event, edges are .008/.0084. Red .1*.1 is above both.
        self.assertAlmostEqual(float(shader.at(shader.started)(uv)[0, 3]), .4, places=6)
        shader.textures['noise_1_tex'][..., 0] = 0
        self.assertEqual(float(shader(uv)[0, 3]), 0)
        shader.textures['noise_1_tex'][..., 0] = 1
        shader.textures['shape_mask_tex'][..., 2] = 0
        self.assertEqual(float(shader(uv)[0, 3]), 0)

    def test_slot_override_ignores_placeholder_and_keeps_geometry(self):
        shader = self.fixture()
        points = np.array([[0, 0], [8, 0], [8, -8], [0, -8]], float)
        uv = np.array([[0, 0], [1, 0], [1, 1], [0, 1]], float)
        triangles = np.array([[0, 1, 2], [0, 2, 3]])
        printed_placeholder = np.ones((2, 2, 4))
        layer = (points, uv, triangles, printed_placeholder, np.ones(4), 0)
        frame = dict(time=shader.started, draw=[dict(slot='slash', blend=0)])
        changed = apply_slot_materials(frame, [layer], shader)
        self.assertIs(changed[0][0], points)
        self.assertIs(changed[0][1], uv)
        self.assertIs(changed[0][2], triangles)
        image = np.array(raster(changed, 1, (0, 0), (8, 8)))
        self.assertTrue(np.all(image[..., :3] == [25, 153, 229]))
        self.assertTrue(np.all(image[..., 3] == 102))
        frame['draw'][0]['slot'] = 'body'
        self.assertIs(apply_slot_materials(frame, [layer], shader)[0], layer)

    def test_fragment_receives_interpolated_atlas_uv(self):
        seen = []
        def fragment(uv):
            seen.append(uv.copy())
            return np.concatenate((uv, np.zeros((*uv.shape[:-1], 1)), np.ones((*uv.shape[:-1], 1))), axis=-1)
        points = np.array([[0, 0], [4, 0], [0, -4]], float)
        uv = np.array([[.25, .1], [.75, .1], [.25, .9]], float)
        image = np.array(raster([(points, uv, np.array([[0, 1, 2]]), fragment, np.ones(4), 0)], 1, (0, 0), (4, 4)))
        self.assertTrue(seen)
        self.assertEqual(list(image[0, 0]), [79, 51, 0, 255])
        self.assertEqual(list(image[3, 3]), [0, 0, 0, 0])


if __name__ == '__main__':
    unittest.main()
