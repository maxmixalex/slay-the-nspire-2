"""Public synthetic checks for property-directed fonts and styled pack tails."""
from pathlib import Path
import struct
import sys
import unittest
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import extract_original_fonts as extract
import replace_original_glyphs as fonts
import repack_retained as retained
from test_repack_retained import fixture


def text(value):
    raw = value.encode()+b'\0'
    return struct.pack('<I', len(raw))+raw


def resource(payload, *, compressed=False, data_property=True):
    # A decoy SFNT signature in an unrelated stored property must not be
    # mistaken for the main FontFile.data. No real font bytes are embedded.
    raw = bytearray(b'' if compressed else b'RSRC')
    raw.extend(struct.pack('<5I', 0, 0, 4, 5, 6)); raw.extend(text('FontFile'))
    raw.extend(bytes(8)); raw.extend(struct.pack('<I', 0)); raw.extend(bytes(8+44))
    raw.extend(struct.pack('<I', 2)); raw.extend(text('cache')); raw.extend(text('data' if data_property else 'unrelated'))
    raw.extend(struct.pack('<II', 0, 1)); raw.extend(text('local://main'))
    offset = len(raw); raw.extend(bytes(8)); struct.pack_into('<Q', raw, offset, len(raw))
    raw.extend(text('FontFile')); raw.extend(struct.pack('<I', 2))
    decoy = b'OTTOwrong cache'
    raw.extend(struct.pack('<III', 0, 31, len(decoy))); raw.extend(decoy); raw.extend(bytes(-len(decoy) % 4))
    raw.extend(struct.pack('<III', 1, 31, len(payload))); raw.extend(payload); raw.extend(bytes(-len(payload) % 4))
    if not compressed: return bytes(raw)
    import zstandard
    size = 64
    blocks = [zstandard.ZstdCompressor().compress(bytes(raw[at:at+size]))
              for at in range(0, len(raw)+1, size)]
    return b'RSCC'+struct.pack('<III', 2, size, len(raw))+struct.pack('<'+'I'*len(blocks), *(len(b) for b in blocks))+b''.join(blocks)


class OriginalFontTest(unittest.TestCase):
    def test_reads_main_data_property_across_compressed_blocks(self):
        expected = b'\0\1\0\0'+bytes(range(255))*3
        for compressed in (False, True):
            got, report = extract.font_payload(resource(expected, compressed=compressed))
            self.assertEqual(got, expected)
            self.assertEqual(report['godot'], [4, 5])
            self.assertEqual(report['binary_format'], 6)

    def test_never_carves_decoy_fonts_and_rejects_truncation(self):
        with self.assertRaisesRegex(extract.ResourceFormatError, 'no stored data'):
            extract.font_payload(resource(b'OTTOactual', data_property=False))
        with self.assertRaises(extract.ResourceFormatError):
            extract.font_payload(resource(b'OTTOactual')[:-8])
        with self.assertRaisesRegex(extract.ResourceFormatError, 'supported SFNT'):
            extract.font_payload(resource(b'wrong font'))

    def test_nested_styles_only_collect_referenced_faces(self):
        faces = fonts.styled_alphabet(['A[b]甲[i]乙[/i]丙[/b][i]丁[/i]戊'])
        self.assertEqual(faces[0], {ord('A'), ord('戊')})
        self.assertEqual(faces[1]-set(range(32, 127)), {ord('甲'), ord('丙')})
        self.assertEqual(faces[2]-set(range(32, 127)), {ord('丁')})
        self.assertEqual(faces[3]-set(range(32, 127)), {ord('乙')})

    def test_appended_locale_gets_regular_and_styled_glyphs(self):
        class SyntheticFace:
            coverage = {65, ord('甲')}
            def glyph(self, cp, px, style=0):
                return (cp, px | style << 6, 1, 1, 1, 0, b'\xff')
            def metadata(self): return dict(family='synthetic')
        source = SimpleNamespace(strings=[(0, len('A[b]甲[/b]'.encode()))],
            glyphs=[(65, 0, 8, 1, 1, 1, 0)], block=lambda off, n:'A[b]甲[/b]'.encode()[:n])
        face = SyntheticFace()
        rows, report = fonts.generate(source, [face, face, None, None])
        keys = {(row[0], row[1]) for row in rows}
        self.assertTrue({(ord('甲'), px) for px in fonts.SIZES} <= keys)
        self.assertTrue({(ord('甲'), px | 64) for px in fonts.SIZES} <= keys)
        self.assertEqual(report['appended_regular_codepoints'], [ord('甲')])

    def test_replacement_preserves_shared_art_and_exact_locale(self):
        for language in ('eng', 'zhs'):
            original, _, _ = retained.repack(fixture(language, glyphs=3), fast_images='none')
            original = bytearray(original); struct.pack_into('<I', original, 4, 24)
            before = tiles.PackView(original, 1, versions=(24,))
            rows = [(cp, px, w, h, advance, enc, bytes(before.block(off, enc or w*h)))
                    for cp, off, px, w, h, advance, enc in before.glyphs]
            rows[0] = (65, 8, 2, 1, 3, 0, b'\xff\x00')
            rows += [(65, 8 | 64, 3, 1, 7, 0, b'\xff\x00\xff'),
                     (66, 8 | 128, 2, 1, 4, 0, b'\x00\xff')]
            result, report = fonts.replace(bytes(original), language, rows)
            after = tiles.PackView(result, 1, versions=(24,))
            self.assertEqual(after.counts[1], 5)
            self.assertEqual(report['directory_growth_bytes'], 32)
            self.assertTrue(report['artwork_bytes_exact'] and report['strings_exact'])
            old_tiles, new_tiles = tiles.TileReader(before), tiles.TileReader(after)
            self.assertEqual([old_tiles.image_raw(i) if row[3] == 6 else before.image_raw(i)
                              for i, row in enumerate(before.images)],
                             [new_tiles.image_raw(i) if row[3] == 6 else after.image_raw(i)
                              for i, row in enumerate(after.images)])
            self.assertEqual(after.glyphs[3][2:6], (72, 3, 1, 7))
            self.assertEqual(bytes(after.block(after.glyphs[3][1], 3)), b'\xff\x00\xff')
            # Regenerating the same faces does not accumulate old font tails.
            again, receipt = fonts.replace(result, language, rows)
            self.assertEqual(result, again)
            self.assertEqual(receipt['directory_growth_bytes'], 0)

    def test_styles_are_version_gated_and_existing_codepoints_are_required(self):
        original, _, _ = retained.repack(fixture(), fast_images='none')
        with self.assertRaisesRegex(ValueError, 'UI7'):
            fonts.replace(original, 'eng', [(65, 8, 1, 1, 1, 0, b'\xff')])
        original = bytearray(original); struct.pack_into('<I', original, 4, 24)
        with self.assertRaisesRegex(ValueError, 'dropped'):
            fonts.replace(bytes(original), 'eng', [(66, 8, 1, 1, 1, 0, b'\xff')])

    def test_scene_face_family_survives_split_pack_validation(self):
        import split_content
        original, _, _ = retained.repack(fixture('eng', glyphs=1), fast_images='none')
        original = bytearray(original); struct.pack_into('<I', original, 4, 28)
        before = tiles.PackView(original, 1, versions=(28,))
        cp, off, px, w, h, advance, enc = before.glyphs[0]
        regular = (cp, px, w, h, advance, enc, bytes(before.block(off, enc or w*h)))
        rows = [regular, (cp, 5, 1, 1, 1, 0, b'\xff'), (cp, 5 | 32 | 64, 1, 1, 1, 0, b'\x80'),
                (cp, 6 | 32, 2, 1, 2, 0, b'\xff\x00'),
                (cp, 14 | 32 | 64, 2, 1, 2, 0, b'\x00\xff')]
        result, _ = fonts.replace(bytes(original), 'eng', rows)
        profile = split_content.layout(result, 'eng')
        self.assertEqual(profile['glyphs'], 5)
        decoded = tiles.PackView(result, 1, versions=(28,))
        self.assertEqual({row[2] for row in decoded.glyphs}, {px, 5, 101, 38, 110})

    def test_complete_article_alphabet_uses_its_actual_reader_sizes(self):
        class SyntheticFace:
            coverage = set(range(32,127)) | {ord('甲')}
            def glyph(self, cp, px, style=0):
                return (cp, px | style << 6, 1, 1, 1, 0, b'\xff')
            def metadata(self): return dict(family='synthetic')
        texts=['']*8704;texts[0]='A';texts[8519]='[b]甲[/b]'
        source=SimpleNamespace(version=28, strings=[(i,len(t.encode())) for i,t in enumerate(texts)],
            glyphs=[(cp,0,8,1,1,1,0) for cp in range(32,127)],block=lambda off,n:texts[off].encode()[:n])
        rows,report=fonts.generate(source,[SyntheticFace(),SyntheticFace(),None,None])
        article={(row[1]&63,row[1]>>6) for row in rows if row[0]==ord('甲')}
        self.assertEqual(article,{(8,0),(10,0),(8,1),(10,1)})
        self.assertEqual(report['missing_regular_codepoints'],[])
        self.assertEqual(report['auxiliary_only_codepoints'],[ord('甲')])
        self.assertIn((65,5),{(row[0],row[1]) for row in rows})

    def test_ordered_fallbacks_cover_each_real_character_without_format_glyphs(self):
        class SyntheticFace:
            def __init__(self, coverage, alpha): self.coverage,self.alpha=set(coverage),alpha
            def glyph(self, cp, px, style=0):
                return (cp,px|((style&3)<<6)|((style&4)<<3),1,1,1,0,bytes([self.alpha]))
            def metadata(self): return dict(family='synthetic',alpha=self.alpha)
        texts=['']*8704;texts[0]='A';texts[8519]='ğ🙂\ufe0f➝'
        source=SimpleNamespace(version=28,strings=[(i,len(t.encode())) for i,t in enumerate(texts)],
            glyphs=[(65,0,8,1,1,1,0)],block=lambda off,n:texts[off].encode()[:n])
        primary=SyntheticFace([65],10);latin=SyntheticFace([65,287],20)
        emoji=SyntheticFace([287,0x1f642,0xfe0f],30);symbols=SyntheticFace([0x279d],40)
        rows,receipt=fonts.generate(source,[primary,None,None,None],[latin,emoji,symbols])
        by_cp={cp:{(size,raw) for glyph_cp,size,_w,_h,_advance,_encoded,raw in rows if glyph_cp==cp}
               for cp in (65,287,0x1f642,0x279d,0xfe0f)}
        self.assertEqual(by_cp[65],{(5,b'\x0a'),(6,b'\x0a'),(8,b'\x0a')})
        self.assertEqual(by_cp[287],{(8,b'\x14'),(10,b'\x14')})
        self.assertEqual(by_cp[0x1f642],{(8,b'\x1e'),(10,b'\x1e')})
        self.assertEqual(by_cp[0x279d],{(8,b'\x28'),(10,b'\x28')})
        self.assertFalse(by_cp[0xfe0f]);self.assertEqual(receipt['zero_width_format_controls'],[0xfe0f])
        self.assertEqual(receipt['missing_regular_codepoints'],[])
        self.assertEqual(receipt['fallback_codepoints_by_font'],{'0':[287],'1':[0x1f642],'2':[0x279d]})
        self.assertEqual(len(receipt['fallback_fonts']),3)

    def test_tooltip_small_bold_is_title_scoped_and_append_only(self):
        import append_ui9_tooltip_glyphs as tooltip
        texts=['']*8704;texts[20]='新';texts[21]='黜';texts[2230]='疲';texts[6135]='血'
        source=SimpleNamespace(version=31,strings=[(i,len(t.encode())) for i,t in enumerate(texts)],
            cards=[(0,20,21,21)],glyphs=[(cp,0,101,1,1,1,0) for cp in range(32,127)]+[(ord('疲'),0,101,1,1,1,0)],
            block=lambda off,n:texts[off].encode()[:n])
        self.assertEqual(fonts.tooltip_small_bold_alphabet(source)-set(range(32,127)),set(map(ord,'新疲血')))
        class SyntheticFace:
            coverage=set(map(ord,'新血'))
            def glyph(self,cp,px,style):
                self.asserted=(px,style)
                return cp,px|((style&3)<<6)|((style&4)<<3),1,1,1,0,b'\xff'
        face=SyntheticFace();rows=tooltip.missing_glyphs(source,face)
        self.assertEqual({(row[0],row[1]) for row in rows},{(ord('新'),101),(ord('血'),101)})
        self.assertEqual(face.asserted,(5,5))
        source.glyphs += [(cp,0,px,w,h,advance,encoded) for cp,px,w,h,advance,encoded,_ in rows]
        self.assertEqual(tooltip.missing_glyphs(source,face),[])
        texts[2291]='铁';source.strings[2291]=(2291,len('铁'.encode()))
        display=SyntheticFace();display.coverage={ord('铁')}
        rows=tooltip.missing_glyphs(source,face,display)
        self.assertEqual({(row[0],row[1]) for row in rows},{(ord('铁'),px|32) for px in (10,12,14)})
        self.assertEqual(display.asserted,(14,4))


if __name__ == '__main__': unittest.main()
