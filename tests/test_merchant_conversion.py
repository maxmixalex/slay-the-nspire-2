"""Synthetic skin/deform and material tests; no original assets are bundled."""
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from animation import geometry
from merchant_effects import sample_texture, shade


def mesh(path):
    return {"kind": 2, "path": path, "color": 0xffffffff,
            "vertices": [[(-1, x, y, 1.)] for x, y in ((0, 0), (2, 0), (0, 2))],
            "uv": np.array([[0., 0.], [1., 0.], [0., 1.]]),
            "triangles": np.array([[0, 1, 2]])}


class SkinResolutionTest(unittest.TestCase):
    def test_partial_skin_keeps_default_and_uses_owning_deform(self):
        sk = SimpleNamespace(skins=[("default", {(0, "body"): mesh("body"), (1, "ear"): mesh("ear")}),
                                    ("outline", {(0, "body"): mesh("selected"), (2, "stroke"): mesh("stroke")})])
        slots = [{"attachment": name, "bone": 0, "color": np.ones(4), "blend": 0}
                 for name in ("body", "ear", "stroke")]
        textures = {name: np.zeros((1, 1, 4)) for name in ("body", "ear", "selected", "stroke")}
        world = [np.array([[1., 0., 10.], [0., 1., 20.], [0., 0., 1.]])]
        deformation = {(0, 0, "body"): np.array([1., 2.]*3),
                       (1, 0, "body"): np.array([3., 4.]*3),
                       (0, 1, "ear"): np.array([-1., -2.]*3),
                       (1, 1, "ear"): np.array([500., 500.]*3)}
        base = geometry(sk, world, slots, deformation, [2, 0, 1], textures)
        selected = geometry(sk, world, slots, deformation, [2, 0, 1], textures, skin=1)
        self.assertEqual(len(base), 2)
        self.assertEqual(len(selected), 3)
        self.assertIs(selected[0][3], textures["stroke"])
        self.assertIs(selected[1][3], textures["selected"])
        self.assertIs(selected[2][3], textures["ear"])
        np.testing.assert_array_equal(base[0][0], [[11, 22], [13, 22], [11, 24]])
        np.testing.assert_array_equal(selected[1][0], [[13, 24], [15, 24], [13, 26]])
        np.testing.assert_array_equal(selected[2][0], [[9, 18], [11, 18], [9, 20]])
        np.testing.assert_array_equal(base[1][0], selected[2][0])
        with self.assertRaises(ValueError):
            geometry(sk, world, slots, deformation, [0], textures, skin=2)


class FireMaterialTest(unittest.TestCase):
    def test_linear_texel_centres_and_authored_wrap(self):
        image = np.array([[[0.], [1.]], [[2.], [3.]]])
        uv = np.array([[.25, .25], [.75, .75], [.5, .5], [-.25, .25], [1.25, .25]])
        np.testing.assert_allclose(sample_texture(image, uv, True).ravel(), [0, 3, 1.5, 1, 0])
        np.testing.assert_allclose(sample_texture(image, uv, False).ravel(), [0, 3, 1.5, 0, 1])

    def test_independent_constant_material_values(self):
        uniforms = {"Noise1Strength": 1., "Noise2Strength": 1., "InvertNoiseMask": True,
                    "NoiseMaskScale": np.ones(2), "NoiseMaskOffset": np.zeros(2),
                    "DistortionMaskScale": np.ones(2), "DistortionMaskOffset": np.zeros(2),
                    "OuterColor": np.array([.1, .2, .3, 1.]),
                    "InnerColor": np.array([.5, .4, .3, 1.]),
                    "InnerColorStep": np.array([0., 1.]), "OuterStep": np.array([0., 1.])}
        for prefix in ("Noise1", "Noise2"):
            uniforms[prefix+"Scaling"] = np.ones(2)
            uniforms[prefix+"Panning"] = np.array([.37, -.51])
        for prefix in ("Distortion1", "Distortion2"):
            uniforms[prefix+"Scale"] = np.ones(2)
            uniforms[prefix+"Panning"] = np.array([.41, .19])
            uniforms[prefix+"Strength"] = 0.
        values = {"Noise1Texture": .5, "Noise2Texture": .8, "NoiseMask": .2,
                  "Distortion1Texture": .9, "Distortion2Texture": .4, "DistortionMask": .8, "TEXTURE": .5}
        images = {name: np.full((1, 1, 4), value) for name, value in values.items()}
        uv = np.array([[.3, .4], [.7, .8]])
        np.testing.assert_allclose(shade(uv, 1.17, uniforms, images),
                                   [[.424, .4592, .4944, .648]]*2, atol=1e-12)
        uniforms["InvertNoiseMask"] = False
        np.testing.assert_allclose(shade(uv, 2.53, uniforms, images),
                                   [[.208, .2864, .3648, .216]]*2, atol=1e-12)


if __name__ == "__main__":
    unittest.main()
