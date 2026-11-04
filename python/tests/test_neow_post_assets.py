import struct
import unittest
from types import SimpleNamespace

from nspire_converter import neow_post_assets as n


class NeowPostTests(unittest.TestCase):
    def test_exact_endpoint_and_bounded_single_view(self):
        self.assertEqual(
            (n.BASE_AUX, n.AUXILIARY, n.BASE_IMAGES, n.IMAGES), (7224, 7225, 59729, 59730)
        )
        self.assertEqual(n.STATIC_COUNT, 19392)
        self.assertLessEqual(
            n.STATIC_AT + n.STATIC_COUNT * 2 + 10000 + 2050, 256 * n.STATIC_HEIGHT * 2
        )

    def test_geometry_clock_lod_and_profile_are_explicit(self):
        for static in (False, True):
            field = bytes(n.STATIC_COUNT * 2) if static else None
            raw = n.parameter_bytes(bytes(16384), bytes(10000), field)
            self.assertEqual(len(raw), 28672 if not static else 51200)
            self.assertEqual(struct.unpack_from("<4i", raw, 32), n.UV_WORDS)
            self.assertEqual(struct.unpack_from("<3H", raw, 22), (1000, 15, int(static)))
            mask, lod = struct.unpack_from("<2H", raw, 18)
            self.assertEqual(lod, mask + 10000)
            self.assertEqual(raw[lod : lod + 2050], n.lod_table())

    def test_unknown_or_relabelled_source_payload_is_rejected(self):
        source = SimpleNamespace(aux=[0] * n.AUXILIARY, images=[0] * n.IMAGES, data=bytes(40))
        source.aux[n.BASE_AUX] = n.BASE_IMAGES
        raw = n.parameter_bytes(bytes(16384), bytes(10000))
        with self.assertRaisesRegex(ValueError, "original mask/noise"):
            n.validate(source, lambda i: (256, n.FULL_HEIGHT, 0, raw))
        with self.assertRaisesRegex(ValueError, "clock0"):
            n.require_reduced_source(source, lambda i: (256, n.FULL_HEIGHT, 0, raw))

    def test_mip_table_is_bounded_and_monotonic(self):
        table = struct.unpack("<1025H", n.lod_table())
        self.assertTrue(all(x == 0 for x in table[:257]))
        self.assertEqual(table[-1], 256)
        self.assertTrue(all(a <= b for a, b in zip(table, table[1:])))


if __name__ == "__main__":
    unittest.main()
