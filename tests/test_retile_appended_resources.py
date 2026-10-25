"""Old tiles, partial last groups and appended frame/locale relocation."""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import repack_retained
import retile_appended_resources as retile
import split_content
from test_repack_retained import fixture


def source(language='eng', glyphs=1):
    data = bytearray(fixture(language, glyphs))
    view = repack_retained.view(data)
    # Clip four is a retained sparse picture and five is the original tiled
    # image. The tile catalog ends in a partial 680-record group.
    tiles.CLIP.pack_into(data, view.clip_at, 4, 2, 100, 0, 0, 0)
    struct.pack_into('<I', data, view.base_end + 44, 680)
    output, _, _ = repack_retained.repack(bytes(data), fast_images='none')
    data = bytearray(output)
    struct.pack_into('<I', data, 4, 39)
    # Harmless old tail padding ensures this deliberately tiny fixture tests
    # relocation independently of the production pack's storage economics.
    data.extend(bytes(8192))
    struct.pack_into('<I', data, 12, len(data))
    return bytes(data)


class RetileAppendedTest(unittest.TestCase):
    def test_partial_tile_group_and_shared_retained_slices_are_exact(self):
        with tempfile.TemporaryDirectory() as temporary, redirect_stdout(StringIO()):
            root = Path(temporary)
            english = retile.view(source())
            plan = retile.make_plan(english, root)
            self.assertEqual(plan['selected'], [4])
            self.assertEqual(plan['first_changed'], 0)
            encoded = retile.compress_plan(plan, root, root / 'cache', 1)
            output, receipt = retile.rewrite(english, plan, encoded)
            retile.verify(english, output, plan)
            target = retile.view(output)
            self.assertEqual(tiles.TileReader(english).image_raw(5), tiles.TileReader(target).image_raw(5))
            self.assertEqual(english.image_raw(4), tiles.TileReader(target).image_raw(4))
            self.assertEqual(english.image_raw(3), target.image_raw(3))
            self.assertGreater(receipt['saved_bytes'], 0)
            chinese = retile.view(source('zhs', glyphs=3))
            other, _ = retile.rewrite(chinese, plan, encoded)
            retile.verify(chinese, other, plan)
            a, b = split_content.layout(output, 'eng'), split_content.layout(other, 'zhs')
            self.assertEqual(output[a['prefix']:a['prefix']+a['artwork']],
                             other[b['prefix']:b['prefix']+b['artwork']])
            # Warm offline cache bytes must be proven and reused exactly.
            self.assertEqual(encoded, retile.compress_plan(plan, root, root / 'cache', 1))


if __name__ == '__main__':
    unittest.main()
