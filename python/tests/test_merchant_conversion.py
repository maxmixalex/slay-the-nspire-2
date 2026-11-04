"""Synthetic skin/deform and material tests; no original assets are bundled."""

import unittest
from types import SimpleNamespace

import numpy as np
from nspire_converter.animation import geometry
from nspire_converter.merchant_effects import sample_texture, shade


def mesh(path):
    return {
        "kind": 2,
        "path": path,
        "color": 0xFFFFFFFF,
        "vertices": [[(-1, x, y, 1.0)] for x, y in ((0, 0), (2, 0), (0, 2))],
        "uv": np.array([[0.0, 0.0], [1.0, 0.0], [0.0, 1.0]]),
        "triangles": np.array([[0, 1, 2]]),
    }


class SkinResolutionTest(unittest.TestCase):
    def test_partial_skin_keeps_default_and_uses_owning_deform(self):
        sk = SimpleNamespace(
            skins=[
                ("default", {(0, "body"): mesh("body"), (1, "ear"): mesh("ear")}),
                ("outline", {(0, "body"): mesh("selected"), (2, "stroke"): mesh("stroke")}),
            ]
        )
        slots = [
            {"attachment": name, "bone": 0, "color": np.ones(4), "blend": 0}
            for name in ("body", "ear", "stroke")
        ]
        textures = {name: np.zeros((1, 1, 4)) for name in ("body", "ear", "selected", "stroke")}
        world = [np.array([[1.0, 0.0, 10.0], [0.0, 1.0, 20.0], [0.0, 0.0, 1.0]])]
        deformation = {
            (0, 0, "body"): np.array([1.0, 2.0] * 3),
            (1, 0, "body"): np.array([3.0, 4.0] * 3),
            (0, 1, "ear"): np.array([-1.0, -2.0] * 3),
            (1, 1, "ear"): np.array([500.0, 500.0] * 3),
        }
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
        image = np.array([[[0.0], [1.0]], [[2.0], [3.0]]])
        uv = np.array([[0.25, 0.25], [0.75, 0.75], [0.5, 0.5], [-0.25, 0.25], [1.25, 0.25]])
        np.testing.assert_allclose(sample_texture(image, uv, True).ravel(), [0, 3, 1.5, 1, 0])
        np.testing.assert_allclose(sample_texture(image, uv, False).ravel(), [0, 3, 1.5, 0, 1])

    def test_independent_constant_material_values(self):
        uniforms = {
            "Noise1Strength": 1.0,
            "Noise2Strength": 1.0,
            "InvertNoiseMask": True,
            "NoiseMaskScale": np.ones(2),
            "NoiseMaskOffset": np.zeros(2),
            "DistortionMaskScale": np.ones(2),
            "DistortionMaskOffset": np.zeros(2),
            "OuterColor": np.array([0.1, 0.2, 0.3, 1.0]),
            "InnerColor": np.array([0.5, 0.4, 0.3, 1.0]),
            "InnerColorStep": np.array([0.0, 1.0]),
            "OuterStep": np.array([0.0, 1.0]),
        }
        for prefix in ("Noise1", "Noise2"):
            uniforms[prefix + "Scaling"] = np.ones(2)
            uniforms[prefix + "Panning"] = np.array([0.37, -0.51])
        for prefix in ("Distortion1", "Distortion2"):
            uniforms[prefix + "Scale"] = np.ones(2)
            uniforms[prefix + "Panning"] = np.array([0.41, 0.19])
            uniforms[prefix + "Strength"] = 0.0
        values = {
            "Noise1Texture": 0.5,
            "Noise2Texture": 0.8,
            "NoiseMask": 0.2,
            "Distortion1Texture": 0.9,
            "Distortion2Texture": 0.4,
            "DistortionMask": 0.8,
            "TEXTURE": 0.5,
        }
        images = {name: np.full((1, 1, 4), value) for name, value in values.items()}
        uv = np.array([[0.3, 0.4], [0.7, 0.8]])
        np.testing.assert_allclose(
            shade(uv, 1.17, uniforms, images), [[0.424, 0.4592, 0.4944, 0.648]] * 2, atol=1e-12
        )
        uniforms["InvertNoiseMask"] = False
        np.testing.assert_allclose(
            shade(uv, 2.53, uniforms, images), [[0.208, 0.2864, 0.3648, 0.216]] * 2, atol=1e-12
        )


if __name__ == "__main__":
    unittest.main()
