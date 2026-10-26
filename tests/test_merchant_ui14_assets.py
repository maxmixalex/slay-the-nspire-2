"""Public pixel fixtures for exact regular-merchant frame factoring."""
from pathlib import Path
import sys
import tempfile
import unittest
import random

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from merchant_ui14_assets import (decoded_row, rgba_image, separated_planes,
                                 verify_recomposition)
from patch_feedback_assets import write_image
from group_merchant_ui14 import build_payload, regroup_existing
from ui12_resources import replace_images, view
import content_tiles as tiles
from test_ui14_resources import current


class MerchantSplitTests(unittest.TestCase):
    def test_all_frames_recompose_after_independent_sparse_lz4_decoding(self):
        # A black opaque invariant must not be confused with transparency;
        # partial-alpha invariants also move without a second blend operation.
        colors = np.array([[0, 0xffff, 0x8421, 0], [0xf800, 0x07e0, 0x001f, 0]], dtype='<u2')
        alpha = np.array([[255, 128, 255, 0], [255, 255, 33, 0]], dtype=np.uint8)
        frames = [(colors.copy(), alpha.copy()) for _ in range(3)]
        frames[1][0][1, 0] = 0xf810
        frames[1][1][1, 1] = 254
        frames[2][1][0, 2] = 0;frames[2][0][0, 2] = 0
        static, moving, count = separated_planes(frames)
        self.assertEqual(count, 3)
        self.assertEqual(int(static[1][0, 0]), 255)
        self.assertEqual(int(static[1][0, 1]), 128)
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            row = write_image(cache, 'fixed', rgba_image(*static), sparse=True)
            decoded_static, _ = decoded_row(row, cache)
            for index, (original, plane) in enumerate(zip(frames, moving)):
                row = write_image(cache, 'moving-'+str(index), rgba_image(*plane), sparse=True)
                decoded_moving, _ = decoded_row(row, cache)
                verify_recomposition(original, decoded_static, decoded_moving)
                self.assertFalse(np.any((decoded_static[1] != 0) & (decoded_moving[1] != 0)))

    def test_changing_alpha_remains_moving_and_overlap_fails(self):
        frames = [(np.array([[0xf81f]], dtype='<u2'), np.array([[value]], dtype=np.uint8))
                  for value in (1, 2, 1)]
        static, moving, count = separated_planes(frames)
        self.assertEqual(count, 0)
        for original, plane in zip(frames, moving):verify_recomposition(original, static, plane)
        with self.assertRaisesRegex(ValueError, 'overlaps'):
            verify_recomposition(frames[0], frames[0], frames[0])

    def test_shape_changes_fail_instead_of_changing_registration(self):
        frames = [(np.zeros((1, 2), dtype='<u2'), np.zeros((1, 2), dtype=np.uint8)),
                  (np.zeros((2, 1), dtype='<u2'), np.zeros((2, 1), dtype=np.uint8))]
        with self.assertRaisesRegex(ValueError, 'dimensions'):separated_planes(frames)


class MerchantRetainedGroupTests(unittest.TestCase):
    @staticmethod
    def frames():
        image = Image.frombytes('RGB', (64, 24), random.Random(813).randbytes(64*24*3)).convert('RGBA')
        for y in range(image.height):image.putpixel((63, y), (248, 136, 32, 93))
        frames = {}
        for index in range(5):
            frame = image.copy();frame.putpixel((index, 3), (index*32, 255, 0, 127));frames[index] = frame
        return frames

    def test_bounded_neighbor_groups_preserve_all_bytes_and_reclaim_old_copies(self):
        packed, _ = replace_images(current('eng', 3), self.frames())
        source = view(packed)
        output, report = regroup_existing(packed, ranges=((0, 3), (3, 2)), group_bytes=8192, max_frames=2)
        target = view(output)
        self.assertLess(len(output), len(packed))
        self.assertEqual(report['saved_pack_bytes'], len(packed)-len(output))
        self.assertEqual(report['unreachable_replacement_bytes'], 0)
        self.assertEqual(report['runtime_memory_added'], 0)
        self.assertGreater(report['retained_groups'], 0)
        for group in report['groups']:
            self.assertLessEqual(group['frames'], 2)
            self.assertLessEqual(group['raw_bytes'], 8192)
            self.assertFalse(group['first'] < 3 < group['first']+group['frames'])
        for image in range(5):self.assertEqual(source.image_raw(image), target.image_raw(image))
        self.assertEqual(tiles.TileReader(source).image_raw(5), tiles.TileReader(target).image_raw(5))
        for i in range(len(source.glyphs)):self.assertEqual(source.glyph_payload(i), target.glyph_payload(i))
        for i in range(len(source.strings)):self.assertEqual(source.string_payload(i), target.string_payload(i))
        self.assertEqual(source.clips, target.clips)
        self.assertEqual(source.cards, target.cards)
        self.assertEqual(source.aux, target.aux)

    def test_tiny_frames_keep_their_cheaper_independent_encoding(self):
        packed, _ = replace_images(current(), {0:Image.new('RGBA', (1, 1))})
        source = view(packed)
        payload, entries, decoded, report = build_payload(source, ((0, 1),))
        self.assertEqual(report['retained_groups'], 0)
        self.assertEqual(entries[0][3], source.images[0][3])
        self.assertEqual(decoded[0], source.image_raw(0))
        self.assertEqual(len(payload), report['previous_frame_payload_bytes'])

    def test_rejects_overlap_and_oversized_cold_blocks(self):
        packed, _ = replace_images(current(), self.frames())
        source = view(packed)
        with self.assertRaisesRegex(ValueError, 'overlap'):build_payload(source, ((0, 2), (1, 2)))
        with self.assertRaisesRegex(ValueError, 'bounded sparse'):build_payload(source, ((0, 1),), group_bytes=2)
        with self.assertRaisesRegex(ValueError, 'measured bound'):build_payload(source, ((0, 1),), group_bytes=128*1024)


if __name__ == '__main__':unittest.main()
