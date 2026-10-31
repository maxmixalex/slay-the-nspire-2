"""Exact old sparse bytes, format boundary, literal fallback and bitmap bounds."""
from itertools import product
from pathlib import Path
import lzma
import random
import struct
import sys
import tempfile
import unittest

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as t
import coalesce_tile_resources as coalesce
import implicit_geometry as implicit
import implicit_tile_geometry_ui22 as convert
from sparse_image import encode as original_sparse
from tile_prediction import index_transform
from test_coalesce_tile_resources import fixture as old_fixture
from test_repack_retained import stored


def fixture(*, predicted=False, hidden_zero=False, alias=False):
    data = bytearray(old_fixture()); struct.pack_into('<I', data, 4, 45)
    source = coalesce.view(data); reader = t.TileReader(source); plan = {}
    for group, (at, size, _raw, _crc, kind, count) in enumerate(reader.groups):
        original = bytes(data[at - 32:at + size]); header = list(t.GROUP.unpack_from(original))
        raw = bytearray(reader.group(group)[0])
        if kind == 1:
            for start in range(0, len(raw), 770):
                raw[start + 514:start + 770] = bytes([91]) * 256
                if hidden_zero:
                    raw[start + 514] = 0
        elif predicted:
            raw = index_transform(raw, count, 3, tile_count=reader.tile_count)
            header[0], header[7] = b'TGI2', 1
        payload = stored(bytes(raw)); header[4] = len(payload)
        encoded = t.GROUP.pack(*header) + payload
        plan[group] = original, encoded, encoded + bytes((len(original) - len(encoded)) & 1)
    output, _ = coalesce.rewrite(bytes(data), plan, version=45)
    if alias:
        output = bytearray(output); parsed = coalesce.view(output)
        first = t.DESCRIPTOR.unpack_from(output, parsed.images[0][0])
        t.DESCRIPTOR.pack_into(output, parsed.images[1][0], *first)
        output = bytes(output)
    return output


class ImplicitGeometryTest(unittest.TestCase):
    def compare_alpha(self, values):
        alpha = np.asarray(values, dtype='u1').reshape(1, -1)
        rgba = np.empty((1, alpha.shape[1], 4), dtype='u1')
        rgba[:, :, :3] = np.array((233, 149, 57), dtype='u1')
        rgba[:, :, 3] = alpha
        raw = original_sparse(Image.fromarray(rgba))
        geometry, colors, decoded_alpha = t.validate_sparse(raw, alpha.shape[1], 1, pixels=True)
        self.assertEqual(implicit.alpha_geometry(alpha), geometry)
        self.assertEqual(implicit.sparse_bytes(geometry, colors, decoded_alpha, len(raw)), raw)

    def test_fast_alpha_layout_matches_independent_original_dynamic_program(self):
        for size in range(1, 8):
            for values in product((0, 91, 255), repeat=size):
                self.compare_alpha(values)
        for leading in range(0, 18):
            for middle in range(0, 18):
                for trailing in (0, 1, 4, 5, 6, 10, 11, 17):
                    self.compare_alpha([255] * leading + [91] + [255] * middle + [37] + [255] * trailing)
        generator = random.Random(42246)
        for _ in range(128):
            self.compare_alpha([generator.choice((0, 1, 91, 254, 255, 255, 255)) for _ in range(320)])

    def test_mask_tail_padding_and_bounds_are_strict(self):
        active = np.arange(300) % 3 == 1
        encoded = implicit.encode_mask(active, 320, 240, 480)
        np.testing.assert_array_equal(implicit.active_mask(encoded, 320, 240), active)
        self.assertEqual(coalesce.active_tiles(encoded, 320, 240).tolist(), active.tolist())
        cases = [encoded[:39], encoded[:-1] + b'\1', b'\xfd\xff\x80']
        for raw in cases:
            with self.assertRaises(ValueError):
                implicit.active_mask(raw, 320 if len(raw) > 3 else 1, 240 if len(raw) > 3 else 1)
        with self.assertRaises(ValueError): implicit.encode_mask(active, 320, 240, 39)
        with self.assertRaises(ValueError): implicit.alpha_geometry(np.zeros((241, 1), dtype='u1'))
        with self.assertRaises(ValueError): implicit.alpha_geometry(np.zeros((1, 321), dtype='u1'))

    def test_plain_and_predicted_groups_keep_all_old_sparse_bytes_and_clocks(self):
        for predicted in (False, True):
            before = fixture(predicted=predicted)
            source = coalesce.view(before); old = t.TileReader(source)
            with tempfile.TemporaryDirectory() as directory:
                after, proof = convert.convert_bytes(before, Path(directory), workers=1, align_suffix=False)
            target = coalesce.view(after); new = t.TileReader(target)
            self.assertEqual(target.version, 46)
            self.assertEqual(proof['converted_records'], 2)
            self.assertGreater(proof['saved_bytes'], 0)
            self.assertEqual(old.predictors, new.predictors)
            for image in range(len(source.images)):
                self.assertEqual(old.image_raw(image), new.image_raw(image))
            self.assertEqual(source.clips, target.clips)
            self.assertEqual(source.cards, target.cards)
            self.assertEqual(source.aux, target.aux)
            self.assertEqual(source.string_payload(0), target.string_payload(0))
            old_raw, old_starts = old.group(old.tile_groups)
            new_raw, new_starts = new.group(new.tile_groups)
            self.assertEqual(old_starts, new_starts)
            for at, length in old_starts.items():
                header = t.FRAME.unpack_from(old_raw, at)
                self.assertEqual(header, t.FRAME.unpack_from(new_raw, at))
                self.assertEqual(old_raw[at:at + 12 + header[1]], new_raw[at:at + 12 + header[1]])
                self.assertTrue(implicit.marked(new_raw[at + 12 + header[1]:at + length]))
            downgraded = bytearray(after); struct.pack_into('<I', downgraded, 4, 45)
            with self.assertRaisesRegex(ValueError, 'NSP2/46'):
                t.TileReader(coalesce.view(downgraded)).image_raw(0)

    def test_alpha_zero_payload_and_protected_or_unknown_records_stay_literal(self):
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            before = fixture(hidden_zero=True)
            after, proof = convert.convert_bytes(before, cache, workers=1, align_suffix=False)
            self.assertEqual(proof['converted_records'], 0)
            self.assertEqual(proof['groups'][0]['noncanonical_frames'], 2)
            old, new = t.TileReader(coalesce.view(before)), t.TileReader(coalesce.view(after))
            for image in (0, 1): self.assertEqual(old.image_raw(image), new.image_raw(image))
            before = fixture()
            after, proof = convert.convert_bytes(before, cache, workers=1, protected_images=(0,), align_suffix=False)
            self.assertEqual(proof['converted_records'], 1)
            self.assertEqual(proof['groups'][0]['protected_frames'], 1)
            before = fixture(alias=True)
            after, proof = convert.convert_bytes(before, cache, workers=1, align_suffix=False)
            self.assertEqual(proof['converted_records'], 1)
            self.assertEqual(proof['groups'][0]['unknown_frames'], 1)
            self.assertEqual(proof['groups'][0]['converted_records'][0]['images'], [0, 1])
            self.assertEqual(t.TileReader(coalesce.view(after)).image_raw(0),
                             t.TileReader(coalesce.view(after)).image_raw(1))

    def test_cached_payload_is_rechecked_and_conversion_is_idempotent(self):
        before = fixture()
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            after, _ = convert.convert_bytes(before, cache, workers=1, align_suffix=False)
            again, proof = convert.convert_bytes(after, cache, workers=1, align_suffix=False)
            self.assertEqual(after, again)
            self.assertEqual(proof['converted_records'], 0)
            files = list(cache.glob('*.lzma2')); self.assertEqual(len(files), 1)
            files[0].write_bytes(b'broken')
            with self.assertRaises((ValueError, lzma.LZMAError)):
                convert.convert_bytes(before, cache, workers=1, align_suffix=False)


if __name__ == '__main__':
    unittest.main()
