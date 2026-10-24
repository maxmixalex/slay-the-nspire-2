"""Independent shader-data and reserved-slot contracts; no private assets."""
from pathlib import Path
import re
import struct
import sys
import tempfile
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import selection_ui9_assets as selector
from lz4_block import decode


class SelectionAssetsTest(unittest.TestCase):
    def test_half_texture_validates_layout_and_finite_data(self):
        header = bytearray(52)
        header[:4] = b'GST2'
        struct.pack_into('<III', header, 4, 1, 512, 512)
        struct.pack_into('<HHII', header, 40, 512, 512, 0, 15)
        original = np.zeros((512, 512, 4), dtype='<f2')
        original[:, :, :3] = .5
        original[:, :, 3] = 1
        data = bytes(header)+original.tobytes()
        self.assertTrue(np.array_equal(selector.half_texture(data), original))
        for offset, value in ((4, 2), (8, 511), (44, 1), (48, 14)):
            broken = bytearray(data)
            struct.pack_into('<I', broken, offset, value)
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                selector.half_texture(broken)
        for bad in (float('nan'), float('inf'), -1., 2.):
            broken = bytearray(data)
            struct.pack_into('<e', broken, 52+6, bad)
            with self.subTest(value=bad), self.assertRaises(ValueError):
                selector.half_texture(broken)
        with self.assertRaises(ValueError):
            selector.half_texture(data[:-1])

    def test_sampling_preserves_both_shader_alpha_and_source_rgb(self):
        rgba = np.array([[[1., 0., 0., 0.], [0., 1., 0., 1.]],
                         [[0., 0., 1., .5], [1., 1., 1., .25]]])
        # Identity-sized projection does not blur a texel; each pair is
        # alpha then RGB, not the reverse or an ordinary 8-bit alpha plane.
        words = np.frombuffer(selector.project_shader_input(rgba, (2, 2)), dtype='<u2')
        self.assertEqual(words.tolist(), [0, 0xf800, 65535, 0x07e0, 32768, 0x001f, 16384, 0xffff])
        # A single output sample lies at the center of the four source
        # texels. Quantization follows interpolation, never precedes it.
        center = struct.unpack('<HH', selector.project_shader_input(rgba, (1, 1)))
        self.assertEqual(center, (round(.4375*65535), (16 << 11) | (32 << 5) | 16))
        bigger = np.frombuffer(selector.project_shader_input(rgba, (4, 4)), dtype='<u2').reshape(4, 4, 2)
        self.assertEqual(bigger[0, 0].tolist(), [0, 0xf800])
        self.assertEqual(bigger[-1, -1].tolist(), [16384, 0xffff])

    def test_shader_payload_is_exact_after_independent_decode(self):
        rgba = np.zeros((3, 5, 4), dtype=np.float64)
        rgba[:, :, 0] = .25
        rgba[:, :, 1] = .75
        rgba[:, :, 3] = np.linspace(.2, 1., 15).reshape(3, 5)
        raw = selector.project_shader_input(rgba)
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            row = selector.write_shader_input(cache, raw)
            payload = (cache/row['file']).read_bytes()
            if row['format'] == 5:
                self.assertEqual(struct.unpack_from('<BI', payload), (0, 104980))
                actual = decode(payload[5:], 104980)
            else:
                actual = payload
            self.assertEqual(actual, raw)
            self.assertEqual((row['width'], row['height'], row['decoded_format']), (290, 181, 0))

    def test_text_slots_do_not_alias_live_catalogs(self):
        root = Path(__file__).resolve().parents[1]/'src'
        rows = re.findall(r'\{"([^"]+)",\s*(\d+)\}', (root/'selection_text_index.inc').read_text())
        self.assertEqual([(int(slot), key) for key, slot in rows], list(selector.TEXTS))
        self.assertEqual(len(set(selector.TEXT_KEYS)), 32)
        self.assertEqual(list(selector.TEXT_KEYS), sorted(selector.TEXT_KEYS))
        slots = set(selector.TEXT_SLOTS)
        for name in ('run_modes_text_index.inc', 'auxiliary_text_index.inc', 'profile_text_index.inc',
                     'ui7_text_index.inc', 'world_text_index.inc', 'locale_text_index.inc'):
            existing = {int(slot) for slot in re.findall(r'\{"[^"]+",\s*(\d+)\}', (root/name).read_text())}
            self.assertFalse(existing & slots, name)


if __name__ == '__main__':
    unittest.main()
