"""Appending an icon must preserve complete grouped glyph payloads exactly."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as tiles
import group_glyphs as grouped
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
import repack_retained as retained
from test_repack_retained import fixture


def original():
    data, _receipt, _batches = retained.repack(fixture(glyphs=180, world_profile=True), fast_images='none')
    source = tiles.PackView(data, 1262, versions=(20,))
    out = bytearray(data)
    for i, row in enumerate(source.glyphs):
        out.extend(bytes(len(out) & 1))
        at = len(out)
        out.extend(i.to_bytes(2, 'little')+bytes((0, 63, 127, 191, 255))*79+b'\x00\x80\xff')
        tiles.GLYPH.pack_into(out, source.glyph_at+i*16, row[0], at, 8, 20, 20, 21, 0)
    struct.pack_into('<I', out, 12, len(out))
    return bytes(out)


class CardResourceAssetsTest(unittest.TestCase):
    def test_insert_before_all_groups_and_descriptors(self):
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            image = Image.new('RGBA', (16, 16), (197, 44, 39, 0))
            for y in range(16):
                for x in range(16):
                    image.putpixel((x, y), (197, 44, 39, (x*29+y*17) % 256))
            row = write_image(cache, 'synthetic-independent-icon', image)
            for group_bytes in grouped.GROUP_CHOICES:
                with self.subTest(group_bytes=group_bytes):
                    before, report = grouped.repack(original(), group_bytes)
                    self.assertGreater(report['groups'], 0)
                    source = tiles.PackView(before, 1262, versions=(32,))
                    after, proof = patch_pack(before, dict(images=[row], localized_text={'eng': {}}), cache,
                        base_aux=1262, auxiliary=1263, input_version=32, version=32, texts=())
                    target = tiles.PackView(after, 1263, versions=(32,))
                    self.assertTrue(proof['unchanged_old_payload'])
                    self.assertEqual(source.clips, target.clips)
                    self.assertEqual(source.cards, target.cards)
                    self.assertEqual(source.aux, target.aux[:-1])
                    self.assertEqual(target.aux[-1], len(source.images))
                    old_reader, new_reader = tiles.TileReader(source), tiles.TileReader(target)
                    for i, entry in enumerate(source.images):
                        old = old_reader.image_raw(i) if entry[3] == 6 else source.image_raw(i)
                        new = new_reader.image_raw(i) if entry[3] == 6 else target.image_raw(i)
                        self.assertEqual(old, new)
                    for i, (old, new) in enumerate(zip(source.glyphs, target.glyphs)):
                        self.assertEqual(old[:1]+old[2:], new[:1]+new[2:])
                        self.assertEqual(source.glyph_payload(i), target.glyph_payload(i))
                        self.assertEqual(source.glyph_alpha(i), target.glyph_alpha(i))
                        if old[6] & grouped.FLAG:
                            self.assertEqual(before[old[1]:old[1]+16], after[new[1]:new[1]+16])
                    for old, new in zip(source.strings, target.strings):
                        self.assertEqual(old[1], new[1])
                        self.assertEqual(before[old[0]:old[0]+old[1]+1], after[new[0]:new[0]+new[1]+1])


if __name__ == '__main__':
    unittest.main()
