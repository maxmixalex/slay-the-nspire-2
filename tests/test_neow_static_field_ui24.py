"""NWF3 storage conversion preserves the runtime's animation-profile contract.

This small public fixture exercises the real replacement/group/compaction
writers. Only the original-art shader input and its already-oracled field
are substituted, so the test needs no original game assets.
"""
from pathlib import Path
import random
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import compact_retained_slices as compact
from coalesce_tile_resources import view
import content_tiles as t
import neow_static_field_ui24 as baked
from test_coalesce_tile_resources import fixture


def source_pack(profile):
    data = bytearray(fixture())
    struct.pack_into('<I', data, 4, 46)
    source = view(data)
    # An obsolete large raw material and a parameter image replace the two
    # test images, leaving real tile tables, fonts, text and clip references.
    for image, width, height, raw in (
            (0, 250, 100, random.Random(7142).randbytes(50000)),
            (1, 256, 9, bytes(4608))):
        data.extend(bytes(len(data) & 1))
        at = len(data)
        data.extend(raw)
        t.IMAGE.pack_into(data, 40 + image * 16, at, width, height, 0, len(raw))
    struct.pack_into('<I', data, source.aux_at, 1)
    struct.pack_into('<I', data, 12, len(data))
    data[36:40] = profile
    return bytes(data)


class NeowAnimationProfile(unittest.TestCase):
    def test_real_writers_preserve_full_and_reduced_profiles_at_both_boundaries(self):
        results = []
        radiance = bytes((31, 47)) * 256
        for profile in (bytes(4), b'NAF1'):
            with tempfile.TemporaryDirectory() as folder:
                original = source_pack(profile)
                original_hash = t.sha256(original)
                with patch.multiple(baked.fire, BASE_AUX=0, BASE_IMAGES=0,
                                    PARAMETER=1, IMAGES=2, FIRST_MASK=1, TEXTURE_PAGES=1), \
                        patch.object(baked.fire, 'parameters', side_effect=lambda raw: {
                            'baked_light': struct.unpack_from('<H', raw, 4)[0] == 3}), \
                        patch.object(baked, 'source_fields', return_value=[]), \
                        patch.object(baked, 'bake', return_value=radiance), \
                        patch.object(compact, 'make_plan', wraps=compact.make_plan) as plan:
                    output, proof = baked.convert(original, folder, workers=1)
                    self.assertEqual(plan.call_args.args[0].data[36:40], profile)
                    self.assertTrue(plan.call_args.kwargs['preserve_profile'])
                    self.assertEqual(output[36:40], profile)
                    self.assertEqual(view(output).image_raw(0)[3], radiance)
                    self.assertLess(len(output), len(original))
                    self.assertEqual(proof['output_sha256'], t.sha256(output))
                    self.assertEqual(t.sha256(original), original_hash)
                    repeated, again = baked.convert(output, Path(folder) / 'again', workers=1)
                    self.assertEqual(repeated, output)
                    self.assertTrue(again['already_baked'])
                results.append(output)
        # The profile is metadata; it cannot alter the new radiance, codec,
        # source clocks or any other output byte in otherwise identical packs.
        self.assertEqual(results[0][:36], results[1][:36])
        self.assertEqual(results[0][40:], results[1][40:])


if __name__ == '__main__':
    unittest.main()
