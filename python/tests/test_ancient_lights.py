import struct
import tempfile
import unittest
from copy import deepcopy
from pathlib import Path

import numpy as np
from nspire_converter import ancient_lights as lights


def frames(scene):
    nodes, dynamic, count, _duration, _particles = lights.EXPECTED[scene]
    source = []
    for i in range(nodes):
        source.append(
            dict(
                path=str(i),
                geometry=[i * 256, 240, 800, 900, 0, 0],
                color=[180, 190, 200, 100],
                amount=1 if scene and i == nodes - 1 else 2,
                local_coords=i < dynamic,
                curve=1 if scene and i == nodes - 1 else 0,
                scale=2.0,
                z=0,
                layer=int(scene or i >= 7),
            )
        )
    output = []
    for frame in range(count):
        row = deepcopy(source)
        for i in range(dynamic):
            row[i]["geometry"][0] += frame
        output.append(row)
    return output


class AncientLightTest(unittest.TestCase):
    def test_vignette_palette_retains_every_rgb565_and_alpha_value(self):
        rgba = np.zeros((240, 320, 4), dtype="u1")
        rgba[:, :, 0] = np.arange(320, dtype="u2") % 32 * 8
        rgba[:, :, 3] = np.arange(240, dtype="u1")[:, None]
        w, h, fmt, raw = lights.vignette_image(rgba)
        self.assertEqual((w, h, fmt, len(raw)), (320, 240, 3, 154112))
        palette = np.frombuffer(raw[:512], "<u2")
        indices = np.frombuffer(raw[512 : 512 + w * h], "u1")
        self.assertTrue(
            np.array_equal(palette[indices].reshape(h, w), (rgba[:, :, 0].astype("u2") >> 3) << 11)
        )
        self.assertEqual(raw[512 + w * h :], rgba[:, :, 3].tobytes())

    def test_typed_bone_roundtrip_and_one_particle_curve(self):
        for scene in (0, 1):
            source = frames(scene)
            raw, proof = lights.pack_parameters(scene, source)
            decoded = lights.parameters(raw, scene)
            self.assertEqual(proof["particles"], 20 if scene == 0 else 179)
            self.assertEqual(decoded["frames"], len(source))
            for i, row in enumerate(decoded["rows"]):
                for frame in (0, 1, len(source) - 1):
                    coords = list(row[:6])
                    if row[11] != 255:
                        at = 32 + decoded["nodes"] * 32 + (row[11] * decoded["frames"] + frame) * 12
                        coords = [a + b for a, b in zip(coords, struct.unpack_from("<6h", raw, at))]
                    self.assertEqual(coords, source[frame][i]["geometry"])

    def test_reject_corrupt_typed_offsets_mapping_and_padding(self):
        raw, _ = lights.pack_parameters(0, frames(0))
        for offset in (0, 2, 4, 10, 18, 20, 22, 28, 30, 32 + 29, 32 + 30, len(raw) - 1):
            changed = bytearray(raw)
            changed[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                lights.parameters(changed, 0)

    def test_global_following_random_geometry_and_source_order_rejected(self):
        for kind in ("global", "overflow", "layer", "definition"):
            source = frames(0)
            if kind == "global":
                for frame in source:
                    frame[0]["local_coords"] = False
            elif kind == "overflow":
                source[1][0]["geometry"][0] += 32768
            elif kind == "layer":
                for frame in source:
                    frame[0]["layer"] = 1
            else:
                source[1][0]["amount"] = 1
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                lights.pack_parameters(0, source)

    def test_affine_perpendicular_height_and_signed_rounding(self):
        matrix = np.array([[2.0, 0.5, -0.25], [0.0, 3.0, 0.125], [0, 0, 1]])
        result = lights.geometry(matrix, 1)
        self.assertEqual(result[:5].tolist(), [-64, 32, 131072, 196608, 0])
        self.assertEqual(result[5], 10923)
        self.assertEqual(
            lights.round_signed(np.array([-1.5, -0.5, 0.5, 1.5])).tolist(), [-2, -1, 1, 2]
        )

    def test_existing_scene_family_must_match_deferred_source(self):
        from nspire_converter.ancient_scene import encode_patch

        with tempfile.TemporaryDirectory() as work:
            frames = [np.full((240, 320), 0x4123, dtype="<u2") for _ in range(2)]
            frames[1][10, 20] = 0xFFFF
            hashes = []
            for i, frame in enumerate(frames):
                raw = frame.tobytes()
                (Path(work) / f"{i:04d}.rgb565").write_bytes(raw)
                hashes.append(lights.t.sha256(raw))
            images = [
                (320, 240, 4, encode_patch(frame, None if i == 0 else frames[i - 1]))
                for i, frame in enumerate(frames)
            ]
            timeline = dict(first=0, frames=2, key_interval=8)
            self.assertEqual(
                lights.verify_scene_family(
                    images.__getitem__, timeline, work, dict(source_rgb565_sha256=hashes)
                )["matched_policy"],
                "exact",
            )
            wrong = frames[0].copy()
            wrong[0, 0] = 0
            images[0] = (320, 240, 4, encode_patch(wrong))
            with self.assertRaises(ValueError):
                lights.verify_scene_family(
                    images.__getitem__, timeline, work, dict(source_rgb565_sha256=hashes)
                )


if __name__ == "__main__":
    unittest.main()
