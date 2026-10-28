#!/usr/bin/env python3
"""Independent geometric fixtures for new Spine 4.2 profile branches."""
from pathlib import Path
from types import SimpleNamespace
import math
import json
import sys
import unittest
import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from animation import pose, relative_world, clip_mesh, geometry, load_textures


def skeleton(values, parents, inherits=None, tracks=(), iks=(), transforms=()):
    inherits = inherits or [0] * len(values)
    bones = [dict(values=np.array(value, dtype=float), parent=parent,
                  inherit=inherit, length=2.)
             for value, parent, inherit in zip(values, parents, inherits)]
    children = [[] for _ in bones]
    for i, parent in enumerate(parents):
        if parent >= 0: children[parent].append(i)
    return SimpleNamespace(bones=bones, children=children, slots=[], iks=list(iks),
        transforms=list(transforms), animations={'sample': {'tracks': list(tracks)}})


UNIT = [0, 0, 0, 1, 1, 0, 0]


class AnimationTests(unittest.TestCase):
    def test_atlas_rotation_restores_spine_uv_orientation_and_trim(self):
        # Independent asymmetric texel fixtures for the coordinate mapping
        # in Spine 4.2 MeshAttachment.updateRegion. No game art is bundled.
        # A 90-degree packed region has atlas(x,y)=(v,1-u): C/F are the
        # first packed row, A/D the last. Mirroring the actor cannot fix it.
        packed = {
            0: [[1, 2, 3], [4, 5, 6]],
            90: [[3, 6], [2, 5], [1, 4]],
            180: [[6, 5, 4], [3, 2, 1]],
            270: [[4, 1], [5, 2], [6, 3]],
        }
        for degrees, labels in packed.items():
            with self.subTest(degrees=degrees):
                pixels = np.zeros((8, 9, 4), dtype=np.uint8)
                block = np.asarray(labels, dtype=np.uint8)
                h, w = block.shape
                pixels[2:2+h, 3:3+w, 0] = block
                pixels[2:2+h, 3:3+w, 1] = block * 17
                pixels[2:2+h, 3:3+w, 3] = 255
                page = Image.fromarray(pixels)
                atlas = ('page.png\nsize:9,8\npart\nbounds:3,2,3,2\n'
                         f'offsets:1,1,6,5\nrotate:{degrees}\n')
                pack = SimpleNamespace(imported=lambda *_: 'atlas',
                    read=lambda _: json.dumps({'atlas_data': atlas}).encode())
                result = load_textures(pack, 'fixture', lambda *_: page)['part']
                expected = np.zeros((5, 6, 4), dtype=np.uint8)
                expected[2:4, 1:4, 0] = [[1, 2, 3], [4, 5, 6]]
                expected[2:4, 1:4, 1] = np.array([[1, 2, 3], [4, 5, 6]]) * 17
                expected[2:4, 1:4, 3] = 255
                np.testing.assert_array_equal(result, expected / 255.)

    def test_no_rotation_or_reflection_keeps_stretch_and_world_position(self):
        fixtures = [
            ([90, 5, 7, 2, 3, 0, 0], [0, 1, 2, 1, 1, 0, 0],
             [[3, 0, -1], [0, 2, 9], [0, 0, 1]]),
            ([0, 5, 7, -2, 3, 0, 0], [0, 1, 2, 1, 1, 0, 0],
             [[2, 0, 3], [0, 3, 13], [0, 0, 1]]),
            ([0, 5, 7, 0, 3, 0, 0], [0, 1, 2, 1, 1, 0, 0],
             [[0, 0, 5], [0, 3, 13], [0, 0, 1]]),
            ([90, 5, 7, 0, 3, 0, 0], [0, 1, 2, 1, 1, 0, 0],
             [[3, 0, -1], [0, 0, 7], [0, 0, 1]]),
            ([90, 5, 7, 2, 3, 0, 0], [30, 1, 2, 2, 1, 0, 0],
             [[3 * math.sqrt(3), -1.5, -1], [2, math.sqrt(3), 9], [0, 0, 1]]),
            ([0, 5, 7, -2, 3, 0, 0], [0, 1, 2, -1, 1, 0, 0],
             [[-2, 0, 3], [0, 3, 13], [0, 0, 1]]),
        ]
        for parent, child, expected in fixtures:
            with self.subTest(parent=parent, child=child):
                sk = skeleton([parent, child], [-1, 0], [0, 2])
                world, *_ = pose(sk, 'sample', 0)
                np.testing.assert_allclose(world[1], expected, atol=1e-12)

    def test_relative_world_adds_target_and_updates_children(self):
        constraint = dict(order=0, bones=[1], target=2, local=False, relative=True,
            offset=np.array([0., 1, 2, 0, 0, 0]), mix=np.array([.5, .5, .25, .5, .25, 0]))
        sk = skeleton([UNIT, [0, 4, 6, 2, 3, 0, 0], [90, 10, 20, 2, 4, 0, 0],
                       [0, 1, 0, 1, 1, 0, 0]], [-1, 0, 0, 1], transforms=[constraint])
        world, *_ = pose(sk, 'sample', 0)
        root2 = math.sqrt(2)
        expected = np.array([[1.5 * root2, -5.25 / root2, 5],
                             [1.5 * root2, 5.25 / root2, 11.5], [0, 0, 1]])
        np.testing.assert_allclose(world[1], expected, atol=1e-12)
        np.testing.assert_allclose(world[3][:2, 2], [5 + 1.5 * root2, 11.5 + 1.5 * root2], atol=1e-12)
        bone = np.eye(3); target = np.diag([-1., 1., 1.])
        relative_world(bone, target, np.array([30., 0, 0, 0, 0, 0]), np.array([1., 0, 0, 0, 0, 0]))
        np.testing.assert_allclose(bone[:2, :2], [[-math.sqrt(3) / 2, -.5], [.5, -math.sqrt(3) / 2]], atol=1e-12)

    def test_one_bone_stretch_uses_local_reach_and_animated_switch(self):
        ik = dict(order=0, bones=[1], target=2, mix=1., soft=0., bend=1,
                  compress=False, stretch=False, uniform=False)
        track = ('ik', 0, 0, [[0, np.array([1., 0]), 1, 1, False, False],
                              [.5, np.array([1., 0]), 1, 1, False, True]])
        sk = skeleton([[90, 5, 7, 2, 3, 0, 0], [0, 1, 2, 1, 1, 0, 0],
                       [0, 1, 6, 1, 1, 0, 0]], [-1, 0, 0], tracks=[track], iks=[ik])
        before, *_ = pose(sk, 'sample', 0)
        self.assertAlmostEqual(float(np.linalg.norm(before[1][:2, 0])), 2.)
        after, *_ = pose(sk, 'sample', .5)
        np.testing.assert_allclose(after[1], [[-6, 0, -1], [0, -2, 9], [0, 0, 1]], atol=1e-12)
        np.testing.assert_allclose((after[1] @ [2, 0, 1])[:2], after[2][:2, 2], atol=1e-12)
        ik.update(compress=True, stretch=False, uniform=True)
        sk = skeleton([[90, 5, 7, 2, 3, 0, 0], [0, 1, 2, 1, 1, 0, 0],
                       [0, 1, 2.5, 1, 1, 0, 0]], [-1, 0, 0], iks=[ik])
        compressed, *_ = pose(sk, 'sample', 0)
        np.testing.assert_allclose(compressed[1], [[-.75, 0, -1], [0, -.5, 9], [0, 0, 1]], atol=1e-12)

    def test_concave_clipping_retains_area_and_interpolated_texture(self):
        vertices = np.array([[-1., -1], [3, -1], [3, 3], [-1, 3]])
        texcoords = (vertices + 1) / 4
        triangles = np.array([[0, 1, 2], [0, 2, 3]])
        contour = np.array([[0., 0], [2, 0], [2, 1], [1, 1], [1, 2], [0, 2]])
        for polygon in (contour, contour[::-1]):
            with self.subTest(clockwise=polygon is not contour):
                clipped, uv, tris = clip_mesh(vertices, texcoords, triangles, polygon)
                area = 0.
                for a, b, c in clipped[tris]:
                    area += abs((b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])) / 2
                    center = (a + b + c) / 3
                    self.assertTrue(center[0] <= 1 + 1e-8 or center[1] <= 1 + 1e-8)
                self.assertAlmostEqual(area, 3.)
                np.testing.assert_allclose(uv, (clipped + 1) / 4, atol=1e-12)
                self.assertTrue(np.all(clipped >= -1e-8) and np.all(clipped <= 2 + 1e-8))

    def test_clipping_end_slot_is_inclusive_and_can_be_empty(self):
        contour = [[(-1, x, y, 1.)] for x, y in ((0, 0), (1, 0), (1, 1), (0, 1))]
        clip = dict(kind=6, vertices=contour, end_slot=2)
        mesh = dict(kind=2, path='image', color=0xffffffff,
            vertices=[[(-1, x, y, 1.)] for x, y in ((-1, -1), (2, -1), (2, 2), (-1, 2))],
            uv=np.array([[0., 0], [1, 0], [1, 1], [0, 1]]), triangles=np.array([[0, 1, 2], [0, 2, 3]]))
        sk = SimpleNamespace(skins=[('default', {(0, 'clip'): clip, **{(i, 'image'): mesh for i in (1, 2, 3)}})])
        for end_attachment in ('', 'image'):
            with self.subTest(end_attachment=end_attachment):
                slots = [dict(bone=0, attachment=name, color=np.ones(4), blend=0)
                         for name in ('clip', 'image', end_attachment, 'image')]
                draw = geometry(sk, [np.eye(3)], slots, {}, list(range(4)), {'image': np.ones((2, 2, 4))})
                self.assertEqual(len(draw), 2 if not end_attachment else 3)
                for vertices, *_ in draw[:-1]:
                    np.testing.assert_allclose(vertices.min(axis=0), [0, 0], atol=1e-12)
                    np.testing.assert_allclose(vertices.max(axis=0), [1, 1], atol=1e-12)
                np.testing.assert_allclose(draw[-1][0].min(axis=0), [-1, -1], atol=1e-12)
                np.testing.assert_allclose(draw[-1][0].max(axis=0), [2, 2], atol=1e-12)


if __name__ == '__main__': unittest.main()
