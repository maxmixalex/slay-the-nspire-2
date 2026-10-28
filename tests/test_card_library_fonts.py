"""Source label coverage and exact relocation, including grouped old payloads."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import card_library_fonts as library
import content_tiles as tiles
import group_strings
from patch_ui7_assets import append_glyphs
from test_ftue_assets import fixture


class Labels:
    def __init__(self):
        self.index = library.library_text_index()
        self.values = {slot: '[red]中文[/red]' for key, slot in self.index.items()
                       if key.endswith('.flavor')}
        for key in library.BOLD_LABEL_KEYS:
            self.values[self.index[key]] = '牌'
        self.values.update({self.index['card_library.CARD_COUNT']: '{Amount}张牌',
                            self.index['card_library.SEARCH_PLACEHOLDER']: '搜索',
                            self.index['card_library.NO_RESULTS']: '没有牌',
                            self.index['card_library.VICTORIES']: '[gold]胜场：[/gold]{Victories}局'})
        self.glyphs = [(cp, 0, band, 1, 1, 1, 0)
                       for cp, band in library.RETIRED_FLAVOR_KEYS]

    def string_payload(self, slot):
        return self.values[slot].encode()+b'\0'


def original_with_flavor_keys():
    packed, _ = group_strings.repack(fixture('zhs'))
    source = group_strings.view(packed)
    # The actual bug affected only directory keys. This fixture gives their
    # payloads distinctive alpha so their exact survival is independently
    # observable even after the keys are retired.
    additions = [(cp, band, 2, 4, 2, 0, bytes((cp & 255, 0, 64, 255))*2)
                 for cp, band in sorted(library.RETIRED_FLAVOR_KEYS)]
    additions.append((ord('甲'), library.FLAVOR_BAND, 2, 4, 2, 0, bytes(range(8))))
    result, _ = append_glyphs(packed, source.counts[5], len(source.clips), additions,
        version=33, profile=(len(source.cards), len(source.strings), len(source.clips), source.counts[5]))
    result = bytearray(result)
    struct.pack_into('<I', result, 4, library.VERSION)
    return bytes(result), source.counts[5]


class CardLibraryFontsTest(unittest.TestCase):
    def test_labels_use_source_bands_and_full_counter_value_alphabet(self):
        required = library.glyph_requirements(Labels())
        self.assertIn((ord('牌'), 4, 5), required)
        self.assertIn((ord('张'), 4, 0), required)
        self.assertIn((ord('搜'), 5, 0), required)
        self.assertIn((ord('没'), 6, 0), required)
        for digit in '0123456789':
            for size in (4, 5, 6):
                self.assertIn((ord(digit), size, 0), required)
        for char in '0123+X':
            self.assertIn((ord(char), 5, 5), required)
        # Placeholder names and color markup are not display text.
        for char in '{}[]/AmountVictoriesgold':
            self.assertNotIn((ord(char), 4, 0), required)
            if char not in '0123+X':
                self.assertNotIn((ord(char), 5, 0), required)
        labels = Labels()
        labels.values[labels.index['card_library.CARD_COUNT']] = '{Unexpected} cards'
        with self.assertRaisesRegex(ValueError, 'Unrecognized Card Library substitution'):
            library.glyph_requirements(labels)

    def test_retirement_is_locale_and_reachable_text_sensitive(self):
        labels = Labels()
        self.assertFalse(library.retireable_flavor_keys(labels, 'eng'))
        self.assertEqual(library.retireable_flavor_keys(labels, 'zhs'), library.RETIRED_FLAVOR_KEYS)
        first = next(slot for key, slot in labels.index.items() if key.endswith('.flavor'))
        for text in ('中文 / 中文', 'literal red', '[unknown]中文[/unknown]', '[lb]字[rb]'):
            labels.values[first] = text
            with self.subTest(text=text), self.assertRaisesRegex(ValueError, 'visible in current text'):
                library.retireable_flavor_keys(labels, 'zhs')

    def test_shrink_equal_and_grow_keep_grouped_glyphs_strings_and_art_exact(self):
        before, auxiliary = original_with_flavor_keys()
        source = tiles.PackView(before, auxiliary, versions=(34,))
        self.assertTrue(any(row[6] & tiles.GLYPH_GROUP_FLAG for row in source.glyphs))
        self.assertTrue(any(row[1] & tiles.STRING_GROUP_FLAG for row in source.strings))
        old_reader = tiles.TileReader(source)
        for count in (0, 9, 12):
            with self.subTest(added=count):
                additions = [(0x4e10+i, 100, 2, 4, 2, 0, bytes((i, 85, 170, 255))*2)
                             for i in range(count)]
                after = library.rewrite_directory(before, auxiliary, library.RETIRED_FLAVOR_KEYS, additions)
                proof = library.verify_preserved(before, after, auxiliary, library.RETIRED_FLAVOR_KEYS)
                self.assertTrue(proof['exact_complete_old_payload'])
                self.assertTrue(proof['retired_payloads_retained'])
                target = tiles.PackView(after, auxiliary, versions=(34,))
                self.assertEqual(len(target.glyphs), len(source.glyphs)-9+count)
                keys = {(row[0], row[2]): i for i, row in enumerate(target.glyphs)}
                self.assertFalse(set(keys) & library.RETIRED_FLAVOR_KEYS)
                self.assertEqual(target.glyph_alpha(keys[ord('甲'), library.FLAVOR_BAND]), bytes(range(8)))
                for row in additions:
                    self.assertEqual(target.glyph_alpha(keys[row[0], row[1]]), row[-1])
                new_reader = tiles.TileReader(target)
                for image, row in enumerate(source.images):
                    old = old_reader.image_raw(image) if row[3] == 6 else source.image_raw(image)
                    new = new_reader.image_raw(image) if row[3] == 6 else target.image_raw(image)
                    self.assertEqual(old, new)

    def test_retirement_and_existing_directory_bound_are_not_generalized(self):
        before, auxiliary = original_with_flavor_keys()
        source = tiles.PackView(before, auxiliary, versions=(34,))
        with self.assertRaisesRegex(ValueError, 'restricted'):
            library.rewrite_directory(before, auxiliary, {(ord('甲'), library.FLAVOR_BAND)}, [])
        additions = [(0x1000+i, 8, 1, 1, 1, 0, b'\xff')
                     for i in range(library.GLYPH_LIMIT-len(source.glyphs)+1)]
        with self.assertRaisesRegex(ValueError, 'exceeds32768'):
            library.rewrite_directory(before, auxiliary, set(), additions)


if __name__ == '__main__':
    unittest.main()
