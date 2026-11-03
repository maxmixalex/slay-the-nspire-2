"""Authored slash samples keep exact pixels and their retained decoder path."""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import struct
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import actor_storage_policy as policy
import animation_color_candidates_ui23 as colors
import content_tiles as tiles
import converter_finalize as finalize
import retile_appended_resources as retile
from test_retile_appended_resources import source


def registered_source():
    clips = [(0, 1, 100, 0, 0, 0)]*29
    clips[28] = (2, 2, 83, 59, 87, 1024)
    aux = [0]*policy.ACTOR_AUXILIARY_END
    aux[policy.ACTOR_AUXILIARY_BASE+28] = 8
    # Two samples; the first also has an overflow piece outside the clip range.
    words = [0x4652, 1, 2, 83, 1024, 0, 1024, 0, 23, 3,
             14, 2, 20, 1, 2, 0, 0, 7, 12, 34, 3, 0, 0]
    table = struct.pack('<23H', *words)
    return SimpleNamespace(clips=clips, aux=aux, images=[(0, 1, 1, 7, 16)]*9,
                           image_raw=lambda image: (23, 1, 0, table))


class ActorStoragePolicyTest(unittest.TestCase):
    def test_registered_overflow_pieces_are_included_without_changing_clocks(self):
        before = registered_source()
        clocks = list(before.clips)
        self.assertEqual(policy.exact_material_images(before), {2, 3, 7})
        self.assertEqual(before.clips, clocks)
        before.aux[policy.ACTOR_AUXILIARY_BASE+28] = 0
        with self.assertRaisesRegex(ValueError, 'table is absent'):
            policy.exact_material_images(before)

    def test_earlier_catalog_has_no_native_material_exception(self):
        self.assertEqual(policy.exact_material_images(retile.view(source())), set())

    def test_actual_dicing_selection_keeps_material_retained(self):
        before = retile.view(source())
        self.assertIn(4, finalize.fresh_sparse_images(before))
        self.assertEqual(before.images[4][3], 7)
        with patch.object(policy, 'exact_material_images', return_value={4}):
            self.assertNotIn(4, finalize.fresh_sparse_images(before))
        self.assertEqual(before.images[4][3], 7)

    def test_real_color_writer_preserves_material_pixels_and_retained_group(self):
        with redirect_stdout(StringIO()):
            before = retile.view(source())
            # A live animation sample would otherwise receive the bounded LUT.
            ordinary = colors.convert(before, 'bounded-a5', workers=1, preserve_static=True)
            changed = retile.view(colors.rewrite(before, {}, ordinary))
            self.assertNotEqual(before.image_raw(4), changed.image_raw(4))
            with patch.object(policy, 'exact_material_images', return_value={4}):
                selected = colors.convert(before, 'bounded-a5', workers=1, preserve_static=True)
            output = colors.rewrite(before, {}, selected)
            colors.verify(before, output, selected, 'bounded-a5')
            after = retile.view(output)
            proof = policy.verify_exact_materials(before, after, {4})
            self.assertEqual(proof['images'], 1)
            self.assertEqual(after.images[4][3], 7)
            # Unprotected animated data still takes its existing policy.
            self.assertEqual(tiles.TileReader(after).image_raw(5), tiles.TileReader(changed).image_raw(5))

    def test_verification_rejects_a_changed_decoder_path(self):
        before = registered_source()
        after = registered_source()
        after.images = list(after.images)
        after.images[2] = (0, 1, 1, 6, 16)
        with self.assertRaisesRegex(ValueError, 'storage representation changed'):
            policy.verify_exact_materials(before, after, {2})


if __name__ == '__main__':
    unittest.main()
