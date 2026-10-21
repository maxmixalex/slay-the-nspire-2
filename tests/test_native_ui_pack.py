"""Relocation, source-payload immutability and UI shader boundary checks."""
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
import unittest

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
from append_native_ui import append_skins, reference_hsv
from test_content_tiles import fixture


class NativeUiPack(unittest.TestCase):
    def test_offset_relocation_preserves_every_existing_frame_and_text(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            source, tiled = root / 'source', root / 'tiled'
            original = fixture(extra=7, shared_retained=True)
            source.write_bytes(original)
            tiles.repack(source, tiled, root / 'cache', auxiliary_count=1,
                         first_clip=1, tiles_per_group=1, workers=1)
            baseline = tiled.read_bytes()
            skins = [(2, 2, 1, bytes(range(12))), (1, 1, 0, b'\x34\x12')]
            result, receipt = append_skins(baseline, 1, skins)
            old = tiles.PackView(baseline, 1, versions=(15,))
            new = tiles.PackView(result, 3, versions=(16,))
            old_reader, new_reader = tiles.TileReader(old), tiles.TileReader(new)
            for i in range(len(old.images)):
                a = old_reader.image_raw(i) if old.images[i][3] == 6 else old.image_raw(i)
                b = new_reader.image_raw(i) if new.images[i][3] == 6 else new.image_raw(i)
                self.assertEqual(a, b)
            self.assertEqual(receipt['added_bytes'], len(result) - len(baseline))
            self.assertEqual(tiled.read_bytes(), baseline)
            self.assertEqual(new.clips, old.clips)
            self.assertEqual(new.aux, [0, len(old.images), len(old.images) + 1])
            for (off, length), (old_off, _) in zip(new.strings, old.strings):
                self.assertEqual(result[off:off + length + 1], baseline[old_off:old_off + length + 1])
            for bad in ([], [(321, 1, 0, b'x')], [(1, 1, 4, b'\0\0')], [(2, 2, 1, b'x')]):
                with self.assertRaises(ValueError):
                    append_skins(baseline, 1, bad)
            with self.assertRaises(ValueError):
                append_skins(result, 3, skins)

    def test_yiq_identity_alpha_and_saturation(self):
        rgba = np.array([[[255, 0, 0, 7], [0, 255, 0, 127], [3, 79, 212, 0]]], dtype=np.uint8)
        image = Image.fromarray(rgba, 'RGBA')
        self.assertTrue(np.array_equal(np.asarray(reference_hsv(image, 1, 1, 1)), rgba))
        grey = np.asarray(reference_hsv(image, 1, 0, 1))
        self.assertTrue(np.array_equal(grey[:, :, 3], rgba[:, :, 3]))
        self.assertTrue(np.max(np.abs(grey[:, :, 0].astype(int) - grey[:, :, 1])) <= 1)
        self.assertTrue(np.max(np.abs(grey[:, :, 1].astype(int) - grey[:, :, 2])) <= 1)
        self.assertTrue(np.all(np.asarray(reference_hsv(image, 1, 1, 0))[:, :, :3] == 0))


if __name__ == '__main__':
    unittest.main()
