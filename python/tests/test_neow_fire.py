import struct
import unittest

import numpy as np
from nspire_converter import neow_fire as n


class NeowFireTest(unittest.TestCase):
    def test_patch_keys_and_sparse_literals(self):
        rng = np.random.default_rng(31)
        previous = bytes(n.PIXELS)
        for frame in range(80):
            row = bytearray(previous)
            for at in rng.integers(0, n.PIXELS, 50):
                row[int(at)] = int(rng.integers(256))
            row[100:200] = bytes([frame]) * 100
            ref = bytes(n.PIXELS) if frame % 32 == 0 else previous
            raw = n.encode(bytes(row), ref, frame)
            self.assertEqual(n.decode(raw, ref, frame), row)
            previous = bytes(row)
            with self.assertRaises(ValueError):
                n.decode(raw[:-1], ref, frame)
            bad = bytearray(raw)
            struct.pack_into("<H", bad, 4, frame + 1)
            with self.assertRaises(ValueError):
                n.decode(bad, ref, frame)

    def test_cyclic_hold_bound_and_endpoints(self):
        source = np.empty((n.FRAMES, n.PIXELS), "u1")
        for i in range(n.FRAMES):
            source[i] = np.uint8((np.arange(n.PIXELS, dtype="u4") + i // 5) % 256)
        selected, _ = n.hold(source)
        self.assertLessEqual(int(np.max(abs(selected.astype("i2") - source.astype("i2")))), 8)
        self.assertTrue(np.all(selected[source == 0] == 0))
        self.assertTrue(np.all(selected[source == 255] == 255))

    def test_integer_zero_bounds_and_parameter_v2(self):
        raw = bytearray(4608)
        raw[:4] = b"NWF1"
        struct.pack_into(
            "<16H",
            raw,
            4,
            1,
            1,
            4,
            n.FRAMES,
            32,
            n.PIXELS,
            n.BASE_IMAGES,
            6,
            n.FIRST_MASK,
            n.PARAMETER,
            n.NODES_AT,
            n.MODELS_AT,
            n.ROWS_AT,
            n.SPANS_AT,
            n.SPANS,
            n.LENGTH,
        )
        struct.pack_into("<2I", raw, 36, 200000, 18000)
        span = pixel = 0
        for y in range(240):
            struct.pack_into("<2H", raw, n.ROWS_AT + y * 4, span, pixel)
            for index in range(3 if y < 225 else 2):
                count = 25 if span < 565 else 24
                struct.pack_into("<2H", raw, n.SPANS_AT + span * 4, index * 100, count)
                span += 1
                pixel += count
        struct.pack_into("<2H", raw, n.ROWS_AT + 240 * 4, span, pixel)
        for model in range(2):
            for index in (4, 9, 31):
                struct.pack_into("<i", raw, n.MODELS_AT + model * 128 + index * 4, 1 << 24)
        fields = [
            np.full((636, 600, 1), 255, "u1"),
            np.zeros((250, 250, 3), "u1"),
            np.full((256, 256, 3), 255, "u1"),
            np.zeros((100, 100, 2), "u1"),
        ]
        result = n.with_integer_bounds(bytes(raw), fields)
        self.assertEqual(len(result), 13312)
        self.assertTrue(n.parameters(result)["integer_bounds"])
        self.assertEqual(result[n.LENGTH : n.LENGTH + 8742], bytes([255]) * 8742)
        self.assertEqual(result[n.LENGTH + 8742], 15)
        bad = bytearray(result)
        bad[n.LENGTH] = 0
        with self.assertRaises(ValueError):
            n.parameters(bad)
        bad = bytearray(result)
        bad[n.LENGTH + 8742] |= 16
        with self.assertRaises(ValueError):
            n.parameters(bad)
        bad = bytearray(result)
        struct.pack_into("<H", bad, 4, 3)
        with self.assertRaises(ValueError):
            n.parameters(bad)

    def test_reduced_payload_alias(self):
        from pathlib import Path
        from tempfile import TemporaryDirectory

        from nspire_converter import content_tiles as t
        from nspire_converter.tanx_scene import group_images

        raw = n.encode(bytes([127]) * n.PIXELS, bytes(n.PIXELS), 0)
        with TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "mask.bin").write_bytes(raw)
            row = dict(
                width=256,
                height=len(raw) // 512,
                format=0,
                file="mask.bin",
                bytes=len(raw),
                sha256=t.sha256(raw),
                raw_sha256=t.sha256(raw),
            )
            art = group_images([row] * n.FRAMES, root, group_bytes=256 * 1024)
            self.assertEqual(len(set(art["entries"])), 1)
            self.assertLess(len(art["payload"]), len(raw))
            self.assertLessEqual(art["report"]["maximum_group_bytes"], len(raw))


if __name__ == "__main__":
    unittest.main()
