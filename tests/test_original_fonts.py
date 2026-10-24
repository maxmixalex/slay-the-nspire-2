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


if __name__ == '__main__': unittest.main()
