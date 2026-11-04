import struct
import unittest

from nspire_converter import vakuu_cinders as v


def parameter():
    return struct.pack(
        "<4s6H6i",
        b"VKC1",
        1,
        24,
        59710,
        59711,
        7221,
        0,
        350802,
        229028,
        -228827,
        350854,
        -9313,
        51931,
    ) + bytes(24)


class VakuuCinders(unittest.TestCase):
    def test_source_geometry_and_shared_references(self):
        r = v.validate_parameter(parameter(), 59731)
        self.assertEqual(r, dict(images=(59710, 59711), table=59730))

    def test_rejects_changed_geometry(self):
        raw = bytearray(parameter())
        raw[16] ^= 1
        with self.assertRaises(Exception):
            v.validate_parameter(raw, 59731)

    def test_rejects_missing_shared_leaf_and_padding(self):
        with self.assertRaises(Exception):
            v.validate_parameter(parameter(), 59711)
        raw = bytearray(parameter())
        raw[-1] = 1
        with self.assertRaises(Exception):
            v.validate_parameter(raw, 59731)


if __name__ == "__main__":
    unittest.main()
