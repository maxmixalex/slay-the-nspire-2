"""Only new scene-font tails may change; old groups and every alpha stay exact."""
from pathlib import Path
import random
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import compact_appended_glyphs as compact
import content_tiles as tiles
import card_library_fonts as library
import split_content as split
import compact_retained_colors as colors
from test_card_library_fonts import original_with_flavor_keys
from test_group_glyphs import independent_pixels
from test_compact_retained_colors import source as retained_source
from test_repack_retained import stored


def baseline(language='zhs'):
    data, auxiliary = original_with_flavor_keys()
    source = compact.view(data)
    marker, size = source.strings[split.MARKER]
    assert size == 3
    result = bytearray(data)
    result[marker:marker + 3] = language.encode()
    return bytes(result), auxiliary


def append(before, auxiliary, count=400, random_pixels=False):
    rng = random.Random(0x353632)
    rows = []
    for i in range(count):
        raw = (rng.randbytes(400) if random_pixels else
               i.to_bytes(2, 'little') + bytes((0, 63, 127, 191, 255)) * 79 + b'\x00\x80\xff')
        rows.append((0x10000 + i, (7, 39, 103, 167)[i % 4], 20, 20, 21, 0, raw))
    if not random_pixels:
        old = compact.view(before)
        index = next(i for i, row in enumerate(old.glyphs) if row[6] & tiles.GLYPH_GROUP_FLAG)
        cp, _off, band, width, height, advance, encoded = old.glyphs[index]
        rows.extend([
            (0x11000, 7, width, height, advance, encoded & 0x7fff, bytes(old.glyph_payload(index))),
            (0x11001, 7, 1, 2, 1, 0, b'\x01\x83'),
            (0x11002, 7, 1, 2, 1, 0, b'\x01\x83'),
            (0x11003, 7, 20, 20, 20, 4, b'\xc8\0\xc8\xff'),
        ])
    return library.rewrite_directory(before, auxiliary, set(), rows, output_version=35)


class AppendedGlyphTest(unittest.TestCase):
    def test_same_version_rgr2_art_and_old_grouped_tails_remain_exact(self):
        with tempfile.TemporaryDirectory() as directory:
            old = colors.view(retained_source())
            choices = colors.plan(old, Path(directory), jobs=1)
            packed = colors.rewrite(old, choices)
            self.assertTrue(any(choice.payload for choice in choices))
            for version in (36, 37, 38):
                data = bytearray(packed)
                source = colors.view(packed)

                def grouped(raw):
                    data.extend(bytes(len(data) & 1))
                    offset = len(data)
                    stream = stored(raw)
                    payload = tiles.GROUP.pack(b'RGR1', len(raw), len(stream), 14, 1, 0, 0, 0) + stream
                    data.extend(payload)
                    data.extend(bytes(len(data) & 1))
                    descriptor = len(data)
                    data.extend(struct.pack('<iIII', offset - descriptor, len(payload), 0, len(raw)))
                    return descriptor

                glyph = source.glyphs[0]
                at = grouped(bytes(source.glyph_payload(0)))
                tiles.GLYPH.pack_into(data, source.glyph_at, glyph[0], at, *glyph[2:6], glyph[6] | 0x8000)
                text = bytes(source.string_payload(0))
                at = grouped(text)
                struct.pack_into('<II', data, source.string_at, at, (len(text) - 1) | tiles.STRING_GROUP_FLAG)
                struct.pack_into('<I', data, 4, version)
                struct.pack_into('<I', data, 12, len(data))
                before = bytes(data)
                rows = [(0x12000 + i, 7, 20, 20, 20, 0,
                         i.to_bytes(2, 'little') + bytes((0, 63, 127, 191, 255)) * 79 + b'\0\x80\xff')
                        for i in range(80)]
                original = library.rewrite_directory(before, source.counts[5], set(), rows, output_version=version)
                result, report = compact.compact(original, before)
                self.assertGreater(report['saved_bytes'], 0)
                self.assertEqual(compact.view(result).version, version)
                self.assertEqual(independent_pixels(original), independent_pixels(result))
                self.assertTrue(library.verify_preserved(before, result, source.counts[5], set())['exact_complete_old_payload'])
                candidate = compact.view(result)
                baseline_view = compact.view(before)
                old_tiles, new_tiles = tiles.TileReader(baseline_view), tiles.TileReader(candidate)
                for image, row in enumerate(baseline_view.images):
                    old_image = old_tiles.image_raw(image) if row[3] == 6 else baseline_view.image_raw(image)
                    new_image = new_tiles.image_raw(image) if row[3] == 6 else candidate.image_raw(image)
                    self.assertEqual(old_image, new_image)
                # A version switch is never a shortcut around the baseline.
                changed = bytearray(original)
                struct.pack_into('<I', changed, 4, 37 if version == 36 else 36)
                with self.assertRaisesRegex(ValueError, 'upgrade or downgrade'):
                    compact.compact(bytes(changed), before)

    def test_existing_groups_and_complete_old_payload_stay_exact(self):
        before, auxiliary = baseline()
        original = append(before, auxiliary)
        source = compact.view(original)
        result, report = compact.compact(original, before)
        target = compact.view(result)
        self.assertGreater(report['saved_bytes'], 0)
        self.assertGreater(report['groups'], 1)
        self.assertLessEqual(report['maximum_group_raw_bytes'], 32768)
        self.assertEqual(report['new_runtime_pools'], 0)
        self.assertEqual(target.version, 35)
        self.assertEqual(independent_pixels(original), independent_pixels(result))
        self.assertEqual(source.images, target.images)
        self.assertEqual(source.strings, target.strings)
        self.assertTrue(report['verification']['exact_complete_old_payload'])
        self.assertTrue(library.verify_preserved(before, result, auxiliary, set())['exact_complete_old_payload'])
        known = {(row[0], row[2]) for row in compact.view(before).glyphs}
        for old, new in zip(source.glyphs, target.glyphs):
            if (old[0], old[2]) in known:
                self.assertEqual(old, new)
        keys = {row[0]: i for i, row in enumerate(target.glyphs) if row[2] == 7}
        self.assertEqual(target.glyphs[keys[0x11001]][1], target.glyphs[keys[0x11002]][1])
        self.assertTrue(target.glyphs[keys[0x11000]][6] & tiles.GLYPH_GROUP_FLAG)
        # Already grouped appends are not recompressed or split on rerun.
        again, second = compact.compact(result, before)
        self.assertEqual(again, result)
        self.assertEqual(second['preserved_reason'], 'existing_appended_groups')

    def test_incompressible_tail_is_not_made_larger(self):
        before, auxiliary = baseline()
        original = append(before, auxiliary, count=70, random_pixels=True)
        result, report = compact.compact(original, before)
        self.assertEqual(result, original)
        self.assertEqual(report['saved_bytes'], 0)
        self.assertEqual(report['groups'], 0)
        self.assertEqual(report['preserved_reason'], 'no_strict_net_saving')

    def test_empty_append_and_old_versions_are_explicit(self):
        before, auxiliary = baseline()
        current = library.rewrite_directory(before, auxiliary, set(), [], output_version=35)
        result, report = compact.compact(current, before)
        self.assertEqual(result, current)
        self.assertEqual(report['preserved_reason'], 'no_new_literal_tail')
        with self.assertRaisesRegex(ValueError, 'NSP2/35'):
            compact.compact(before, before)

    def test_baseline_mismatch_overlap_and_unknown_tail_are_rejected(self):
        before, auxiliary = baseline()
        original = append(before, auxiliary, count=20)
        source = compact.view(original)
        known = {(row[0], row[2]) for row in compact.view(before).glyphs}
        old_id = next(i for i, row in enumerate(source.glyphs) if (row[0], row[2]) in known)
        new_ids = [i for i, row in enumerate(source.glyphs) if (row[0], row[2]) not in known]
        broken = bytearray(original)
        broken[source.glyph_at + old_id * 16 + 12] ^= 1
        with self.assertRaises(ValueError):
            compact.compact(bytes(broken), before)
        broken = bytearray(original)
        first = new_ids[0]
        struct.pack_into('<I', broken, source.glyph_at + first * 16 + 4, source.glyphs[first][1] + 1)
        with self.assertRaises(ValueError):
            compact.compact(bytes(broken), before)
        broken = bytearray(original) + b'unknown payload'
        struct.pack_into('<I', broken, 12, len(broken))
        with self.assertRaisesRegex(ValueError, 'Unknown bytes'):
            compact.compact(bytes(broken), before)
        other = bytearray(before)
        at = compact.view(before).glyph_at
        other[at + 12] ^= 1
        with self.assertRaises(ValueError):
            compact.compact(original, bytes(other))

    def test_bilingual_split_and_every_image_remain_exact(self):
        outputs = {}
        for language, count in (('eng', 80), ('zhs', 120)):
            before, auxiliary = baseline(language)
            original = append(before, auxiliary, count=count)
            output, _ = compact.compact(original, before)
            source, target = compact.view(original), compact.view(output)
            old_tiles, new_tiles = tiles.TileReader(source), tiles.TileReader(target)
            for image, row in enumerate(source.images):
                old = old_tiles.image_raw(image) if row[3] == 6 else source.image_raw(image)
                new = new_tiles.image_raw(image) if row[3] == 6 else target.image_raw(image)
                self.assertEqual(old, new)
            outputs[language] = output
        common, locales, _receipt = split.split(outputs)
        for language, output in outputs.items():
            self.assertEqual(split.reconstruct(common, locales[language], language), output)


if __name__ == '__main__':
    unittest.main()
