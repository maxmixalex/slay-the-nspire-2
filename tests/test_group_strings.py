"""Exact UTF-8/index, boot-marker, font preservation and bounded string groups."""
from pathlib import Path
import lzma
import random
import struct
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import group_glyphs
import group_strings as strings
import split_content as split
from test_group_glyphs import fixture as glyph_fixture


def fixture(language='eng', grouped_fonts=False, short_random=False):
    old = glyph_fixture(language, count=80)
    if grouped_fonts:
        old = group_glyphs.repack(old)[0]
    source = strings.view(old)
    end = strings.nonstring_end(source, split.layout(old, language))
    out = bytearray(old[:end]);out.extend(b'obsolete-unreferenced-text' * 9)
    values = [''] * len(source.strings)
    rng = random.Random(9333)
    for i in range(380):
        values[i] = (''.join(chr(33+rng.randrange(94)) for _ in range(8)) if short_random else
            f'Card {i}: [gold]原版文字[/gold] — retain exact UTF-8, markup, newlines.\n' * 4)
    if not short_random:
        values[1] = values[0];values[2] = '';values[3] = 'A' * 16383;values[5] = 'B' * 16383
    values[7] = values[6110] = language
    seen = {}
    for i, value in enumerate(values):
        raw = value.encode('utf-8') + b'\0'
        if raw not in seen:
            out.extend(bytes(len(out)&1));seen[raw] = len(out);out.extend(raw)
        struct.pack_into('<II', out, source.string_at+i*8, seen[raw], len(raw)-1)
    struct.pack_into('<I', out, 12, len(out))
    return bytes(out)


def independent_texts(data):
    ni, ng, ns = struct.unpack_from('<3I', data, 16)
    at = 40 + ni*16 + ng*16;result = [];cache = {}
    for i in range(ns):
        offset, tagged = struct.unpack_from('<II', data, at+i*8);length = tagged & 0x7fffffff
        if tagged & 0x80000000:
            relative, count, raw_at, raw_count = struct.unpack_from('<iIII', data, offset)
            group_at = offset+relative
            if group_at not in cache:
                h = struct.unpack_from('<4s7I', data, group_at)
                assert h[0] == b'RGR1' and h[1] <= 32768 and h[2] == count-32 and h[3] == 14
                raw = lzma.decompress(data[group_at+32:group_at+count], format=lzma.FORMAT_RAW,
                    filters=[dict(id=lzma.FILTER_LZMA2, dict_size=512*1024)])
                assert len(raw) == h[1];cache[group_at] = raw
            assert raw_count == length+1
            raw = cache[group_at][raw_at:raw_at+raw_count]
        else:
            raw = data[offset:offset+length+1]
        assert len(raw) == length+1 and raw[-1:] == b'\0' and b'\0' not in raw[:-1]
        raw[:-1].decode('utf-8');result.append(bytes(raw))
    return result


class GroupedStringTest(unittest.TestCase):
    def test_exact_text_indices_markup_maxlength_and_literal_marker(self):
        for grouped_fonts in (False, True):
            original = fixture(grouped_fonts=grouped_fonts)
            result, report = strings.repack(original)
            source, target = strings.view(original), strings.view(result)
            self.assertEqual(independent_texts(original), independent_texts(result))
            self.assertEqual(source.glyphs, target.glyphs)
            for i in range(len(source.glyphs)):
                self.assertEqual(source.glyph_payload(i), target.glyph_payload(i))
            self.assertGreater(report['saved_bytes'], 0)
            self.assertGreater(report['groups'], 1)
            self.assertLessEqual(report['maximum_group_raw_bytes'], 32768)
            self.assertEqual(target.string_length(3), 16383)
            self.assertEqual(target.strings[0], target.strings[1])
            for marker_alias in (7, 6110):
                self.assertFalse(target.strings[marker_alias][1] & strings.FLAG)
                self.assertEqual(target.string_payload(marker_alias), b'eng\0')
            self.assertNotIn(b'obsolete-unreferenced-text', result)
            self.assertEqual(strings.repack(result)[0], result)
            self.assertEqual(report['resident_cache_growth_bytes'], 0)
            self.assertFalse(report['startup_string_decompression'])

    def test_short_incompressible_strings_preserve_literal_storage(self):
        original = fixture(short_random=True)
        result, report = strings.repack(original)
        self.assertEqual(report['groups'], 0)
        self.assertEqual(independent_texts(original), independent_texts(result))

    def test_exact_bilingual_split_and_existing_grouped_fonts(self):
        original = {language:fixture(language, grouped_fonts=True) for language in ('eng','zhs')}
        packed = {language:strings.repack(data)[0] for language,data in original.items()}
        common, localized, receipt = split.split(packed)
        for language in packed:
            self.assertEqual(split.reconstruct(common, localized[language], language), packed[language])
            self.assertEqual(receipt['languages'][language]['version'], 33)
            old, new = split.layout(original[language],language), split.layout(packed[language],language)
            self.assertEqual(original[language][old['prefix']:old['prefix']+old['artwork']],
                packed[language][new['prefix']:new['prefix']+new['artwork']])

    def test_bad_version_marker_bounds_group_header_and_utf8_rejected(self):
        original = fixture();packed, _ = strings.repack(original);target = strings.view(packed)
        index = next(i for i,row in enumerate(target.strings) if row[1]&strings.FLAG)
        offset, tagged = target.strings[index]
        relative, count, raw_at, raw_count = strings.DESCRIPTOR.unpack_from(packed, offset)
        group_at = offset+relative
        cases=((4,32),(offset,0x7fffffff),(offset+4,32),(offset+8,32768),(offset+12,raw_count+1),
            (group_at,0),(group_at+4,32769),(group_at+8,count-31),(group_at+12,13),
            (group_at+16,0),(group_at+20,1),(target.string_at+index*8+4,strings.FLAG|16384),
            (target.string_at+6110*8+4,strings.FLAG|3))
        for at,value in cases:
            with self.subTest(at=at,value=value):
                broken=bytearray(packed);struct.pack_into('<I',broken,at,value)
                with self.assertRaises(ValueError):strings.view(broken)
                with self.assertRaises(ValueError):split.layout(broken,'eng')
        broken=bytearray(packed);broken[group_at+32]^=255
        with self.assertRaises(ValueError):strings.verify(original,broken)
        broken=bytearray(packed);struct.pack_into('<I',broken,offset+4,count+1);struct.pack_into('<I',broken,group_at+8,count-31)
        with self.assertRaises(ValueError):strings.view(broken)

    def test_changed_string_order_text_or_art_cannot_pass_proof(self):
        original=fixture();packed,_=strings.repack(original);target=strings.view(packed);bounds=split.layout(packed,'eng')
        broken=bytearray(packed)
        a=target.string_at+8*8;b=target.string_at+9*8
        broken[a:a+8],broken[b:b+8]=broken[b:b+8],broken[a:a+8]
        with self.assertRaises(ValueError):strings.verify(original,broken)
        broken=bytearray(packed);broken[bounds['prefix']+33]^=1
        with self.assertRaises(ValueError):strings.verify(original,broken)
        # A raw locale string must still be NUL-terminated before conversion.
        broken=bytearray(original);source=strings.view(original);off,length=source.strings[7];broken[off+length]=33
        with self.assertRaises(ValueError):strings.repack(broken)


if __name__=='__main__':unittest.main()
