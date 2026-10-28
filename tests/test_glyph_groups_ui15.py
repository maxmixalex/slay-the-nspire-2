"""Current grouped locales retain native pixels/metrics/art while regrouping."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as tiles
import glyph_groups_ui15 as grouping
import group_glyphs
import group_strings
import split_content as split
from test_group_glyphs import fixture, independent_pixels
from ui12_resources import view


def current(language='eng'):
    raw = bytearray(fixture(language, count=800))
    struct.pack_into('<I', raw, 4, 40)
    source = view(raw)
    # Keep a real40-format fractional advance with a negative bearing.
    struct.pack_into('<H', raw, source.glyph_at+12, 0x8000|(4<<12)|1088)
    source = view(raw)
    raw, _ = group_glyphs.write(source, group_glyphs.plan(source), split.layout(raw, language))
    raw = bytearray(raw);struct.pack_into('<I', raw, 4, 40)
    source = view(raw)
    for index in range(600):
        text = ('[gold]'+('A native font face. ' if language=='eng' else '原有字体与像素。')+
                str(index)+'[/gold] ')*8
        payload = text.encode()+b'\0';raw.extend(bytes(len(raw)&1));offset=len(raw);raw.extend(payload)
        struct.pack_into('<II', raw, source.string_at+index*8, offset, len(payload)-1)
    struct.pack_into('<I', raw, 12, len(raw));source=view(raw)
    raw, _ = group_strings.write(source, group_strings.plan(source),
        group_strings.nonstring_end(source, split.layout(raw, language)))
    raw=bytearray(raw);struct.pack_into('<I', raw, 4, 40)
    return bytes(raw)


class CurrentGlyphGroupsTest(unittest.TestCase):
    def test_all_glyph_and_localized_bytes_survive_once_and_repeatedly(self):
        for language in ('eng','zhs'):
            original=current(language);source=view(original)
            self.assertTrue(any(row[1]&tiles.STRING_GROUP_FLAG for row in source.strings))
            result, receipt=grouping.repack(original);target=view(result)
            self.assertEqual(independent_pixels(original),independent_pixels(result))
            self.assertEqual(source.images,target.images)
            self.assertEqual(source.clips,target.clips)
            for index in range(len(source.strings)):
                self.assertEqual(source.string_payload(index),target.string_payload(index))
            self.assertEqual(receipt['input_version'],40)
            self.assertEqual(receipt['output_version'],40)
            self.assertEqual(receipt['runtime_memory_added'],0)
            self.assertEqual(receipt['runtime_preload_bytes'],0)
            self.assertTrue(receipt['verification']['exact_native_glyph_payloads_and_metrics'])
            self.assertEqual(grouping.repack(result)[0],result)

    def test_both_locale_editions_keep_identical_common_art(self):
        packed={lang:grouping.repack(current(lang))[0] for lang in ('eng','zhs')}
        common,languages,_=split.split(packed)
        for language in packed:
            self.assertEqual(split.reconstruct(common,languages[language],language),packed[language])

    def test_comparison_order_and_both_bounded_sizes_keep_pixels(self):
        original=current();expected=independent_pixels(original)
        for order in ('physical','band-frequency'):
            for size in group_glyphs.GROUP_CHOICES:
                result,receipt=grouping.repack(original,order=order,group_bytes=size)
                self.assertEqual(independent_pixels(result),expected)
                self.assertEqual(receipt['order'],order)
                self.assertLessEqual(receipt['glyphs']['maximum_group_raw_bytes'],size)


if __name__=='__main__':unittest.main()
