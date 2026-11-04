import unittest

from nspire_converter import nonu_fog as f


class FogParameters(unittest.TestCase):
    def test_rejects_arbitrary_and_truncated_tables(self):
        for raw in (b"", bytes(2048), b"NFG1" + bytes(2044)):
            with self.assertRaises(Exception):
                f.validate_parameter(raw, 59717)

    def test_exact_catalog_contract(self):
        self.assertEqual(
            (f.BASE_AUX, f.AUXILIARY, f.BASE_IMAGES, f.IMAGES), (7222, 7223, 59712, 59717)
        )
        self.assertEqual(f.NODES, ("fog3", "fog4", "fog5", "fog6", "fog7", "fog8"))


if __name__ == "__main__":
    unittest.main()
