"""Tutorial appends keep old grouped text, glyphs and artwork addressable."""
from pathlib import Path
import sys
import struct
import tempfile
import unittest

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as tiles
import group_strings
import group_glyphs
import ftue_ui9_assets as ftue
from patch_feedback_assets import write_image
from patch_frontend_assets import patch_pack
from test_group_strings import independent_texts
from test_card_resource_assets import original
import split_content
import ui7_profile


def fixture(language):
    packed, _ = group_glyphs.repack(original())
    source = group_strings.view(packed)
    end = group_strings.nonstring_end(source, split_content.layout(packed, 'eng'))
    out = bytearray(packed[:end])
    values = ['']*len(source.strings)
    for i in range(380):
        values[i] = ('Independent preserved rich text '+str(i)+' [gold]value[/gold].\n')*5
    values[2] = ''
    values[6110] = language
    stored = {}
    for i, value in enumerate(values):
        raw = value.encode()+b'\0'
        if raw not in stored:
            out.extend(bytes(len(out)&1))
            stored[raw] = len(out)
            out.extend(raw)
        struct.pack_into('<II', out, source.string_at+i*8, stored[raw], len(raw)-1)
    struct.pack_into('<I', out, 12, len(out))
    return bytes(out)


class FtueAssetsTest(unittest.TestCase):
    def test_reserved_grouped_empty_slot_can_be_filled_without_changing_aliases(self):
        for language in ('eng', 'zhs'):
            with self.subTest(language=language), tempfile.TemporaryDirectory() as directory:
                cache = Path(directory)
                before, _ = group_strings.repack(fixture(language))
                source = group_strings.view(before)
                self.assertEqual(source.string_payload(2), b'\0')
                self.assertTrue(source.strings[2][1] & group_strings.FLAG)
                row = write_image(cache, 'independent-tutorial-fixture',
                                  Image.new('RGBA', (5, 7), (31, 96, 177, 192)))
                value = 'Tutorial test: [gold]play[/gold]\n独立测试' if language == 'zhs' else 'Tutorial test: [gold]play[/gold].'
                manifest = dict(images=[row], localized_text={language:{'test.TUTORIAL':value}})
                after, proof = patch_pack(before, manifest, cache,
                    base_aux=source.counts[5], auxiliary=source.counts[5]+1,
                    input_version=33, version=33, texts=((2, 'test.TUTORIAL'),),
                    base_strings=len(source.strings), base_clips=len(source.clips))
                target = tiles.PackView(after, source.counts[5]+1, versions=(33,))
                expected = independent_texts(before)
                expected[2] = value.encode()+b'\0'
                self.assertEqual(independent_texts(after), expected)
                self.assertTrue(proof['unchanged_old_payload'])
                self.assertEqual(source.cards, target.cards)
                self.assertEqual(source.clips, target.clips)
                self.assertEqual(source.aux, target.aux[:-1])
                for i in range(len(source.glyphs)):
                    self.assertEqual(source.glyph_payload(i), target.glyph_payload(i))
                    self.assertEqual(source.glyph_alpha(i), target.glyph_alpha(i))
                old_tiles, new_tiles = tiles.TileReader(source), tiles.TileReader(target)
                for i, entry in enumerate(source.images):
                    old = old_tiles.image_raw(i) if entry[3] == 6 else source.image_raw(i)
                    new = new_tiles.image_raw(i) if entry[3] == 6 else target.image_raw(i)
                    self.assertEqual(old, new)
                # An occupied slot remains rejected even when its storage is
                # grouped; appending tutorials must not replace live text.
                with self.assertRaises(ValueError):
                    patch_pack(before, manifest, cache,
                        base_aux=source.counts[5], auxiliary=source.counts[5]+1,
                        input_version=33, version=33, texts=((0, 'test.TUTORIAL'),),
                        base_strings=len(source.strings), base_clips=len(source.clips))

    def test_append_only_catalog_does_not_cross_profile_slots(self):
        selected = tuple((slot, key) for slot, key in ui7_profile.TEXTS if key in ftue.TEXT_KEYS)
        self.assertEqual(selected, ftue.TEXTS)
        self.assertEqual((ftue.TEXTS[0][0], ftue.TEXTS[-1][0]), (8347, 8377))
        self.assertLess(ftue.TEXTS[-1][0], 8400)
        self.assertEqual(len(ftue.SPECS), ftue.AUXILIARY-ftue.BASE_AUX)
        self.assertEqual(ftue.AUXILIARY, 2058)
        self.assertNotIn('main_menu_ui.GENERIC_POPUP.confirm', ftue.TEXT_KEYS)
        self.assertFalse(any('MULTIPLAYER' in key for key in ftue.TEXT_KEYS))


if __name__ == '__main__':
    unittest.main()
