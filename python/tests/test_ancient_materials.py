import struct
import unittest
from types import SimpleNamespace

import numpy as np
from nspire_converter import ancient_materials as a


class MaterialTests(unittest.TestCase):
    def test_catalog_and_exact_pinned_source_blend(self):
        self.assertEqual(
            (a.BASE_AUX, a.AUXILIARY, a.BASE_IMAGES, a.IMAGES), (7217, 7220, 59677, 59700)
        )
        self.assertTrue(all(row[-1] == 1 for row in a.LIGHTS))
        self.assertEqual(a.DRAWS[2][4], 1)

    def test_parameter_mapping_and_base_reference_bound(self):
        for scene in (1, 2):
            raw = a.parameters(
                scene,
                a.DRAWS,
                bytes(2500) if scene == 1 else b"",
                2726 if scene == 1 else 2745,
                bytes(32),
            )
            self.assertEqual(struct.unpack_from("<H", raw, 12)[0], 9 if scene == 1 else 5)
            self.assertEqual(
                struct.unpack_from("<H", raw, 442)[0], 0x3152 if scene == 1 else 0x3252
            )
            self.assertEqual(struct.unpack_from("<4i", raw, 424), a.residual_mapping(scene - 1))
        with self.assertRaisesRegex(ValueError, "base identity"):
            a.parameters(2, a.DRAWS)

    def test_modular_clamping_preserves_bounded_colors(self):
        # Opposing channel extremes exercise negative residuals and clamping before
        # modulo encoding, which previously could wrap a clamped zero to31/63.
        base = np.full((240, 320), 0xFFFF, dtype="u2")
        pages = []
        for i in range(9):
            x = np.full((240, 320), 0 if i & 1 else 0xFFFF, dtype="<u2")
            pages.append((320, 240, 0, x.tobytes()))
        rows, proof = a.residual_pages(pages, base, 0)
        self.assertEqual(len(rows), 9)
        self.assertTrue(proof["reconstruction_exact"])
        self.assertTrue(
            all(x <= bound for x, bound in zip(proof["maximum_sampler_channel_error"], (1, 2, 1)))
        )

    def test_native_response_cardinality_and_opaque_constant(self):
        texture = np.full((8, 8, 4), 255, dtype="u1")
        base = np.full((240, 320), 65535, dtype="u2")
        rows, proof = a.response_pages(texture, base)
        self.assertEqual(len(rows), 5)
        self.assertEqual([r["index"] for r in proof["responses"]], [0, 4, 2, 1, 3])
        self.assertTrue(all((w, h, f) == (320, 240, 0) and not any(raw) for w, h, f, raw in rows))

    def test_reduced_source_entry_has_no_color_quantization(self):
        texture = np.full((2, 2, 4), 255, dtype="u1")
        texture[:, :, 0] = 57
        texture[:, :, 1] = 113
        texture[:, :, 2] = 201
        noise = np.full((2, 2), 127, dtype="u1")
        row = a.exact_entry_pose(texture, noise, 1)
        colors = np.frombuffer(row[3], dtype="<u2", count=320 * 240)
        alpha = np.frombuffer(row[3], dtype="u1", offset=320 * 240 * 2)
        expected = (57 >> 3) << 11 | (113 >> 2) << 5 | (201 >> 3)
        self.assertTrue(np.all(colors[alpha != 0] == expected))
        self.assertTrue(np.all(colors[alpha == 0] == 0))
        raw = a.parameters(2, a.DRAWS, b"", 2745, bytes(32), static=True)
        self.assertEqual(struct.unpack_from("<H", raw, 442)[0], 0x5350)

    def test_reduction_rejects_relabeling_animated_materials(self):
        source = SimpleNamespace(aux=[0] * (a.AUXILIARY))
        rows = {}
        for scene in (1, 2):
            key = a.BASE_IMAGES + 20 + scene
            source.aux[a.BASE_AUX + scene] = key
            raw = a.parameters(
                scene,
                a.DRAWS,
                bytes(2500) if scene == 1 else b"",
                2726 if scene == 1 else 2745,
                bytes(32),
            )
            rows[key] = (*a.PARAMETER_SHAPES[scene], 0, raw)
        with self.assertRaisesRegex(ValueError, "original-source"):
            a.require_reduced_source(source, rows.__getitem__)
        for key, row in rows.items():
            raw = bytearray(row[3])
            struct.pack_into("<H", raw, 442, 0x5350)
            rows[key] = (*row[:3], bytes(raw))
        a.require_reduced_source(source, rows.__getitem__)


if __name__ == "__main__":
    unittest.main()
