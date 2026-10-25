"""Speech and popup images must preserve filtered art and grouped locale tails."""
from pathlib import Path
import random
import struct
import sys
import tempfile
import unittest

from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import compact_retained_colors as colors
import content_tiles as tiles
import fake_merchant_ui10_assets as speech
import history_header_ui10_assets as history
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
import repack_retained as retained
import split_content as split
from test_compact_tile_resources import grouped_text
from test_repack_retained import fixture


class FakeMerchantSpeechAssetsTest(unittest.TestCase):
    def test_append_preserves_rgr2_images_and_complete_grouped_tails(self):
        raw = bytearray(fixture(glyphs=3, world_profile=True))
        offset = tiles.IMAGE.unpack_from(raw, 40)[0]
        rng = random.Random(3235)
        for pixel in range(4096):
            green = rng.randrange(64)
            red = (green//2+rng.randrange(3))%32
            blue = (green//2+rng.randrange(2))%32
            struct.pack_into('<H', raw, offset+pixel*2, red*2048+green*32+blue)
        packed, _, _ = retained.repack(bytes(raw), fast_images='none')
        packed = bytearray(packed);struct.pack_into('<I', packed, 4, 23)
        packed = bytearray(grouped_text(bytes(packed)))
        struct.pack_into('<I', packed, 4, 35)
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            old = colors.view(packed)
            choices = colors.plan(old, cache, jobs=1)
            self.assertTrue(any(choice.payload for choice in choices))
            before = colors.rewrite(old, choices)
            source = colors.view(before)
            self.assertTrue(any(before[g.offset:g.offset+4] == b'RGR2' for g in colors.groups(source)))
            image = Image.new('RGBA', (60, 41), (42, 73, 168, 0))
            for y in range(2, 39):
                for x in range(3, 57):
                    image.putpixel((x, y), (42, 73, 168, (x*29+y*17)%256))
            rows = [write_image(cache, 'synthetic-blue-speech', image)]
            for i, size in enumerate(((109, 126), (37, 18), (37, 18), (37, 18), (37, 18))):
                rows.append(write_image(cache, 'synthetic-popup-'+str(i), image.resize(size)))
            ni, ng, ns, nc, nk, na = source.counts
            after, proof = patch_pack(before, dict(images=rows, localized_text={'eng': {}}), cache,
                base_aux=na, auxiliary=na+6, input_version=36, version=speech.VERSION,
                texts=(), base_clips=nc, base_strings=ns)
            target = tiles.PackView(after, na+6, versions=(37,))
            self.assertEqual(target.counts, (ni+6, ng, ns, nc, nk, na+6))
            self.assertEqual(source.clips, target.clips)
            self.assertEqual(source.cards, target.cards)
            self.assertEqual(source.aux, target.aux[:-6])
            self.assertEqual(target.aux[-6:], list(range(ni, ni+6)))
            a, b = tiles.TileReader(source), tiles.TileReader(target)
            for i, entry in enumerate(source.images):
                self.assertEqual(a.image_raw(i) if entry[3] == 6 else source.image_raw(i),
                                 b.image_raw(i) if entry[3] == 6 else target.image_raw(i))
            for i in range(ng):
                self.assertEqual(source.glyph_payload(i), target.glyph_payload(i))
                self.assertEqual(source.glyph_alpha(i), target.glyph_alpha(i))
            for i in range(ns):
                self.assertEqual(source.string_payload(i), target.string_payload(i))
            for name in ('unchanged_old_payload', 'unchanged_old_directory_indices',
                         'unchanged_preexisting_strings', 'verified_all_offset_relocations'):
                self.assertTrue(proof[name])
            self.assertEqual(split.layout(after, 'eng')['version'], 37)
            # The later History icon append retains the complete UI37 image
            # catalogue, filtered groups and already grouped locale tails.
            lock = write_image(cache, 'synthetic-history-lock', image.resize((6, 6)))
            final, lock_proof = patch_pack(after, dict(images=[lock], localized_text={'eng': {}}), cache,
                base_aux=na+6, auxiliary=na+7, input_version=37, version=history.VERSION,
                texts=(), base_clips=nc, base_strings=ns)
            current = tiles.PackView(final, na+7, versions=(38,))
            self.assertEqual(current.counts, (ni+7, ng, ns, nc, nk, na+7))
            self.assertEqual(current.aux[:-1], target.aux)
            self.assertEqual(current.images[-1][1:3], (6, 6))
            tile_reader = tiles.TileReader(current)
            for i, entry in enumerate(target.images):
                self.assertEqual(b.image_raw(i) if entry[3] == 6 else target.image_raw(i),
                                 tile_reader.image_raw(i) if entry[3] == 6 else current.image_raw(i))
            for i in range(ng):
                self.assertEqual(target.glyph_payload(i), current.glyph_payload(i))
                self.assertEqual(target.glyph_alpha(i), current.glyph_alpha(i))
            for i in range(ns):
                self.assertEqual(target.string_payload(i), current.string_payload(i))
            for name in ('unchanged_old_payload', 'unchanged_old_directory_indices',
                         'unchanged_preexisting_strings', 'verified_all_offset_relocations'):
                self.assertTrue(lock_proof[name])
            self.assertEqual(split.layout(final, 'eng')['version'], 38)


if __name__ == '__main__':
    unittest.main()
