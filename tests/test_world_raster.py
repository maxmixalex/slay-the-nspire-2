"""Byte comparisons against the retained raster implementation, without assets."""
import unittest

import numpy as np

from animation import raster as reference
from world_raster import raster


class RasterBatchTests(unittest.TestCase):
    def compare(self, layers, scale=1.0, origin=(14.0, 17.0), canvas=(32, 29)):
        expected = np.asarray(reference(layers, scale, np.asarray(origin), canvas))
        actual = np.asarray(raster(layers, scale, np.asarray(origin), canvas))
        self.assertTrue(np.array_equal(expected, actual),
                        f'{np.count_nonzero(expected != actual)} RGBA bytes differ')

    def test_all_blends_and_ordered_overlap(self):
        rng = np.random.default_rng(81273)
        texture = rng.random((8, 11, 4), dtype=np.float32)
        for blend in range(4):
            layers = []
            for _ in range(4):
                vertices = rng.uniform(-16, 16, (9, 2))
                uv = rng.uniform(-0.3, 1.3, (9, 2))
                triangles = np.array([[0, 1, 2], [2, 1, 3], [4, 5, 6], [6, 7, 8], [2, 1, 0]])
                layers.append((vertices, uv, triangles, texture, rng.random(4), blend))
            self.compare(layers)
            self.compare(layers, 0.37, (1.25, -0.75), (41, 23))

    def test_clipped_degenerate_and_shared_pixel_centers(self):
        vertices = np.array([[0., 0.], [10., 0.], [0., 10.], [10., 10.],
                             [100., 100.], [100., 100.], [100., 100.],
                             [0., 0.], [0.000001, 0.], [0., 0.000001]])
        triangles = np.array([[0, 1, 2], [2, 1, 3], [4, 5, 6], [7, 8, 9]])
        uv = np.array([[0., 0.], [1., 0.], [0., 1.], [1., 1.]] + [[0., 0.]] * 6)
        texture = np.ones((3, 3, 4), dtype=np.float32)
        texture[:, :, 3] = np.array([[0., .5, 1.], [1., 0., .5], [.5, 1., 0.]])
        self.compare([(vertices, uv, triangles, texture, np.ones(4), 0)], origin=(0.5, 10.5))
        self.compare([(vertices, uv, triangles, texture, np.ones(4), 3)], scale=-1.)

    def test_multiple_bounded_batches(self):
        rng = np.random.default_rng(82519)
        vertices = rng.uniform(-35, 35, (135, 2))
        triangles = np.arange(135).reshape(-1, 3)
        texture = rng.random((17, 19, 4))
        self.compare([(vertices, rng.random((135, 2)), triangles, texture, np.ones(4), 2)],
                     origin=(32., 31.), canvas=(65, 64))


if __name__ == '__main__':
    unittest.main()
