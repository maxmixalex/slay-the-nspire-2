import copy
import struct
import types
import unittest

import numpy as np
from nspire_converter import ancient_fire as f


class FireSourceContract(unittest.TestCase):
    @staticmethod
    def source():
        lights = [
            dict(
                path=f"Tezcatara/SpineSprite/SpineBoneNode{i}/light",
                matrix=[0.05, 0, 120, 0, 0.05, 80],
            )
            for i in range(90)
        ]
        frames = []
        for frame in range(f.FRAMES):
            current = copy.deepcopy(lights)
            for i in range(f.DYNAMIC):
                current[i]["matrix"][2] += frame / 256
            frames.append(current)
        rows = [dict(kind="light") for _ in lights]
        rows += [dict(kind="cinder") for _ in range(74)]
        uniforms = dict(
            InvertNoiseMask=True,
            GlobalOffset=[0, 0],
            InnerColor=[0.4, 0.2, 0.1, 1],
            OuterColor=[0.8, 0.3, 0, 1],
            InnerColorStep=[0.2, 0.8],
            OuterStep=[0.1, 0.3],
        )
        for name in ("Noise1", "Noise2", "Distortion1", "Distortion2"):
            uniforms[name + ("Scaling" if name.startswith("Noise") else "Scale")] = [1, 1]
            uniforms[name + "Panning"] = [0.05, -0.1]
            uniforms[name + "Strength"] = 0.5
        for name in ("NoiseMask", "DistortionMask"):
            uniforms[name + "Scale"] = [1, 1]
            uniforms[name + "Offset"] = [0, 0]
        shader = next(
            k for k, value in f.shader.SHADERS.items() if value[0] == "fire" and value[2] == 1
        )
        for i in range(f.NODES):
            values = copy.deepcopy(uniforms)
            values["InnerColorStep"][0] += (i % 3) * 0.01
            rows.append(
                dict(
                    kind="fire",
                    path=(
                        f"Tezcatara/SpineSprite/SpineBoneNode{i}/fire"
                        if i < f.DYNAMIC
                        else f"Tezcatara/fire{i}"
                    ),
                    matrix=[0.05, 0, 120, 0, 0.05, 80],
                    rect=[0, 0, 64, 64],
                    texture=f.TEXTURES[2][0],
                    textures=f.EXPECTED_TEXTURES,
                    tint=[1, 1, 1, 1],
                    shader=shader,
                    blend=0,
                    uniforms=values,
                )
            )
        return rows, frames

    def test_actual_shader_blend_and_bone_sampling_contract(self):
        rows, frames = self.source()
        raw = f.pack_parameters(rows, frames)
        self.assertEqual(f.parameters(raw)["source_shader_period_ms"], 20000)
        self.assertEqual(raw[f.NODES_AT + 37], 1)  # shader mode wins over default material mode
        self.assertEqual(struct.unpack_from("<h", raw, f.DELTAS_AT + 505 * 12)[0], 505)
        self.assertEqual(len(raw), 138880)
        for offset in (0, 4, 10, 16, 36, 40, 44, 52, f.NODES_AT + 38, f.ORDER_AT + 2, f.LENGTH):
            bad = bytearray(raw)
            bad[offset] ^= 1
            with self.assertRaises(ValueError):
                f.parameters(bad)
        bad = bytearray(raw)
        struct.pack_into(
            "<i", bad, f.MODELS_AT + 116, struct.unpack_from("<i", bad, f.MODELS_AT + 112)[0] + 1
        )
        with self.assertRaisesRegex(ValueError, "arithmetic bounds"):
            f.parameters(bad)

    def test_explicit_lossless_prediction_and_raw_fallback(self):
        rows, frames = self.source()
        raw = f.pack_parameters(rows, frames)
        self.assertEqual(f.parameters(raw)["noise_prediction"], "raw")
        tagged = bytearray(raw)
        struct.pack_into("<I", tagged, 48, 1)
        self.assertEqual(f.parameters(tagged)["noise_prediction"], "up")
        struct.pack_into("<I", tagged, 48, 2)
        with self.assertRaises(ValueError):
            f.parameters(tagged)
        for used, stride in f.noise_pages():
            original = bytes((i * 73 + (i // stride) * 19) & 255 for i in range(used)) + bytes(
                (-used) % 640
            )
            encoded = f.predict_page(original, used, stride)
            self.assertEqual(encoded[:stride], original[:stride])
            self.assertEqual(f.predict_page(encoded, used, stride, True), original)
            self.assertFalse(any(encoded[used:]))
        with self.assertRaises(ValueError):
            f.predict_page(bytes(640), 641, 20)

    def test_source_changes_cannot_be_silently_reinterpreted(self):
        rows, frames = self.source()
        rows[164]["uniforms"]["InvertNoiseMask"] = False
        with self.assertRaisesRegex(ValueError, "mask/offset"):
            f.pack_parameters(rows, frames)
        rows[164]["uniforms"]["InvertNoiseMask"] = True
        frames[-1][0]["matrix"][2] += 1000
        with self.assertRaisesRegex(ValueError, "delta exceeds"):
            f.pack_parameters(rows, frames)

    def test_only_two_noise_textures_are_half_sampled_and_rounding_is_exact(self):
        original = list(f.TEXTURES)
        sampled = list(f.sampled_textures())
        self.assertEqual(
            [(r[1], r[2]) for r in original],
            [(1024, 1024), (500, 500), (500, 500), (50, 500), (20, 500)],
        )
        self.assertEqual(
            [(r[1], r[2]) for r in sampled],
            [(512, 512), (250, 250), (500, 500), (50, 500), (20, 500)],
        )
        self.assertEqual(sampled[2:], original[2:])
        for a, b in zip(original, sampled):
            self.assertEqual((a[0], a[3], a[4]), (b[0], b[3], b[4]))
        rgba = np.array(
            [[[0, 255, 1, 0], [0, 255, 2, 1]], [[0, 255, 3, 2], [2, 255, 4, 3]]], dtype="u1"
        )
        before = rgba.copy()
        out = f.half_noise(rgba)
        np.testing.assert_array_equal(out, np.array([[[1, 255, 3, 2]]], dtype="u1"))
        np.testing.assert_array_equal(rgba, before)
        for bad in (rgba.astype("f4"), rgba[:1], rgba[:, :, :3]):
            with self.assertRaises(ValueError):
                f.half_noise(bad)

    def test_real_byte_pages_use_supported_geometry_and_exact_endpoint(self):
        rows, frames = self.source()
        images = []
        for _, width, height, channels, _ in f.sampled_textures():
            count = width * height * len(channels)
            for start in range(0, count, f.PAGE_BYTES):
                used = min(f.PAGE_BYTES, count - start)
                images.append((320, (used + 639) // 640, 0, bytes((used + 639) // 640 * 640)))
        raw = f.pack_parameters(rows, frames)
        images.append((320, len(raw) // 640, 0, raw))
        source = types.SimpleNamespace(aux=[0] * f.AUXILIARY, images=[None] * f.IMAGES)
        source.aux[f.BASE_AUX] = f.IMAGES - 1

        def image(index):
            return images[index - f.BASE_IMAGES]

        f.validate(source, image)
        self.assertEqual(len(images), 9)
        self.assertEqual((f.BASE_IMAGES, f.IMAGES), (59732, 59741))
        old = bytearray(raw)
        struct.pack_into("<H", old, 38, 18)
        with self.assertRaises(ValueError):
            f.parameters(old)
        self.assertTrue(all(row[0] <= 320 and row[1] <= 240 for row in images))
        first = images[0]
        images[0] = (256, 256, 0, bytes(131072))
        with self.assertRaisesRegex(ValueError, "page range"):
            f.validate(source, image)
        images[0] = first
        dirty = bytearray(first[3])
        dirty[f.PAGE_BYTES] = 1
        images[0] = (*first[:3], bytes(dirty))
        with self.assertRaisesRegex(ValueError, "padding"):
            f.validate(source, image)
        images[0] = first
        source.images.append(None)
        with self.assertRaisesRegex(ValueError, "endpoint"):
            f.validate(source, image)


if __name__ == "__main__":
    unittest.main()
