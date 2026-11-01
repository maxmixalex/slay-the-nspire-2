"""Exact public fixtures for the current locale graft and stream tightening."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as tiles
import group_glyphs
import group_strings
import repack_retained
import split_content as split
from ui12_resources import replace_images, view
from ui14_resources import graft_locale
from tighten_ui14_resources import tighten
from reclaim_unused_tiles_ui14 import reclaim
from test_repack_retained import fixture


def current(language='eng', glyphs=1):
    data, _, _ = repack_retained.repack(fixture(language, glyphs), fast_images='none')
    def tag(raw):
        raw = bytearray(raw);struct.pack_into('<I', raw, 4, 40);return bytes(raw)
    data = tag(data);source = view(data)
    data, _ = group_glyphs.write(source, group_glyphs.plan(source), split.layout(data, language))
    data = tag(data);source = view(data)
    data, _ = group_strings.write(source, group_strings.plan(source),
        group_strings.nonstring_end(source, split.layout(data, language)))
    return tag(data)


class Ui14ResourceTests(unittest.TestCase):
    def test_graft_preserves_reduced_profile_for_each_locale(self):
        from no_animations import mark, reduced
        english=bytearray(current());struct.pack_into('<I',english,4,46)
        artwork=mark(english)
        for code,glyphs in (('eng',1),('zhs',3),('jpn',2)):
            previous=bytearray(current(code,glyphs));struct.pack_into('<I',previous,4,46)
            localized,_=graft_locale(artwork,bytes(previous),code)
            self.assertTrue(reduced(localized))
            before,after=view(previous),view(localized)
            for i in range(len(before.strings)):
                self.assertEqual(before.string_payload(i),after.string_payload(i))
        # A historical checksum must not become a functional presentation mode.
        struct.pack_into('<I',english,36,0x12345678)
        full,_=graft_locale(bytes(english),bytes(english),'eng')
        self.assertEqual(struct.unpack_from('<I',full,36)[0],0)

    def test_replaced_frames_reclaim_only_unreferenced_tiles(self):
        from test_retile_appended_resources import source
        import retile_appended_resources as retile
        with tempfile.TemporaryDirectory() as directory:
            work=Path(directory);original=retile.view(source())
            plan=retile.make_plan(original,work)
            tiled,_=retile.rewrite(original,plan,retile.compress_plan(plan,work,work/'tiles',1))
            replacements={4:Image.new('RGBA',original.images[4][1:3],(24,128,96,198))}
            replaced,_=replace_images(tiled,replacements)
            output,report=reclaim(replaced,work/'reclaim',1)
        self.assertGreater(report['unreferenced_tiles'],0)
        self.assertGreater(report['reclaimed_groups'],0)
        self.assertTrue(report['all_live_tile_pixels_unchanged'])
        before,after=view(replaced),view(output)
        a,b=tiles.TileReader(before),tiles.TileReader(after)
        for i,row in enumerate(before.images):
            old=a.image_raw(i) if row[3]==6 else before.image_raw(i)
            new=b.image_raw(i) if row[3]==6 else after.image_raw(i)
            self.assertEqual(old,new)

    def test_graft_preserves_grouped_fonts_and_text_after_art_and_directory_growth(self):
        english = current();chinese = current('zhs', 3)
        artwork, _ = replace_images(english, {0: Image.new('RGBA', (80, 96), (39, 116, 215, 173))})
        localized, report = graft_locale(artwork, chinese, 'zhs')
        old, new, art = view(chinese), view(localized), view(artwork)
        self.assertEqual(new.image_raw(0), art.image_raw(0))
        self.assertTrue(report['compressed_locale_tail_unchanged'])
        for i in range(len(old.glyphs)):self.assertEqual(old.glyph_payload(i), new.glyph_payload(i))
        for i in range(len(old.strings)):self.assertEqual(old.string_payload(i), new.string_payload(i))
        en, _ = graft_locale(artwork, english, 'eng')
        common, languages, _ = split.split({'eng':en, 'zhs':localized})
        self.assertEqual(split.reconstruct(common, languages['zhs'], 'zhs'), localized)

    def test_tightening_preserves_all_pixels_and_grouped_locale_slices(self):
        before = current('eng', 3)
        with tempfile.TemporaryDirectory() as directory:
            after, report = tighten(before, directory, workers=1)
            cached, again = tighten(before, directory, workers=2)
        self.assertEqual(cached, after);self.assertLessEqual(len(after), len(before))
        source, target = view(before), view(after)
        a, b = tiles.TileReader(source), tiles.TileReader(target)
        for i, entry in enumerate(source.images):
            old = a.image_raw(i) if entry[3] == 6 else source.image_raw(i)
            new = b.image_raw(i) if entry[3] == 6 else target.image_raw(i)
            self.assertEqual(old, new)
        self.assertEqual(report['runtime_memory_added'], 0)


if __name__ == '__main__':unittest.main()
