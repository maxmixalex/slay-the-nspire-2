"""Synthetic native glyph pixels, aliasing, group bounds and split proof."""
from pathlib import Path
import lzma
import random
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import group_glyphs as grouped
import split_content as split


def fixture(language='eng', count=400, random_pixels=False, version=31):
    ni, ng, ns, nc, nk, aux = 2, count, 6111, 1, 1, 1
    glyph_at = 40 + ni * 16
    string_at = glyph_at + ng * 16
    base = string_at + ns * 8 + nc * 16 + nk * 16 + aux * 4
    table, descriptors = base + 64, base + 64 + 8 + 2
    out = bytearray(descriptors + 16)

    def append(payload):
        out.extend(bytes(len(out) & 1))
        at = len(out)
        out.extend(payload)
        return at

    # Only artwork identity is relevant to font conversion; this exact TGR1
    # header is also used by the independent split-container fixture.
    art = tiles.GROUP.pack(b'TGR1', 1, 0, 4, 4, 0, 1, 0) + b'tile'
    art_at = append(art)
    image = bytes.fromhex('1234abcd')
    image_at = append(image)
    tiles.IMAGE.pack_into(out, 40, image_at, 2, 1, 0, len(image))
    tiles.IMAGE.pack_into(out, 56, descriptors, 2, 1, 6, 16)
    struct.pack_into('<II', out, table, art_at, len(art))
    tiles.GLOBAL.pack_into(out, base, b'TIL1', 1, 16, 14, 1, 1, 0, 1,
        descriptors, table, 770, 1, 512 * 1024, tiles.ENCODED_LIMIT, table + 8, ni)
    tiles.DESCRIPTOR.pack_into(out, descriptors, base, 0, 0, 4)
    rng = random.Random(77291)
    records = []
    for i in range(count):
        px = (5, 37, 101, 165)[i % 4] if version >= 28 else 8
        cp = 0x4e00 + i
        width, height, advance = (16, 25, 17) if i % 23 == 0 else (20, 20, 21)
        if random_pixels:
            payload, encoded = rng.randbytes(width * height), 0
        elif i % 3 == 0:
            payload, encoded = bytes((200, (i * 17) & 255, 200, 255)), 4
        else:
            payload = i.to_bytes(2, 'little') + bytes((0, 63, 127, 191, 255)) * 79 + b'\x00\x80\xff'
            encoded = 0
        records.append((cp, px, width, height, advance, encoded, payload))
    records.sort(key=lambda row: (row[1], row[0]))
    # Deliberately write in a stable frequency-like order, not key order.
    order = list(range(count));rng.shuffle(order)
    locations, shared = {}, {}
    for i in order:
        payload = records[i][-1]
        if payload not in shared:
            shared[payload] = append(payload)
        locations[i] = shared[payload]
    for i, (cp, px, width, height, advance, encoded, _payload) in enumerate(records):
        tiles.GLYPH.pack_into(out, glyph_at + i * 16, cp, locations[i], px, width, height, advance, encoded)
    append(b'obsolete-unreferenced-font-tail' * 7)
    at = append(language.encode() + b'\0')
    for i in range(ns):
        struct.pack_into('<II', out, string_at + i * 8, at, 3)
    tiles.CLIP.pack_into(out, string_at + ns * 8, 0, 1, 33, 0, 0, 0)
    tiles.HEADER.pack_into(out, 0, b'NSP2', version, tiles.REFERENCE, len(out), ni, ng, ns, nc, nk, 0)
    return bytes(out)


def independent_pixels(data):
    ni, ng = struct.unpack_from('<II', data, 16)
    result, groups = [], {}
    for index in range(ng):
        cp, at, px, width, height, advance, encoded = struct.unpack_from('<IIHBBHH', data, 40 + ni * 16 + index * 16)
        if encoded & 0x8000:
            relative, group_bytes, raw_at, raw_count = struct.unpack_from('<iIII', data, at)
            start = at + relative
            if start not in groups:
                tag, size, encoded_bytes, prop, count, a, b, c = struct.unpack_from('<4s7I', data, start)
                assert tag == b'RGR1' and size <= 65536 and prop == 14 and (a, b, c) == (0, 0, 0)
                raw = lzma.decompress(data[start + 32:start + group_bytes], format=lzma.FORMAT_RAW,
                    filters=[dict(id=lzma.FILTER_LZMA2, dict_size=512 * 1024)])
                assert len(raw) == size and group_bytes == encoded_bytes + 32 and count > 0
                groups[start] = raw
            payload = groups[start][raw_at:raw_at + raw_count]
            encoded &= 0x7fff
        else:
            payload = data[at:at + (encoded or width * height)]
        if encoded:
            alpha = bytearray()
            for pos in range(0, encoded, 2):
                alpha.extend(bytes((payload[pos + 1],)) * payload[pos])
            alpha = bytes(alpha)
        else:
            alpha = bytes(payload)
        assert len(alpha) == width * height
        result.append(((cp, px, width, height, advance, encoded), alpha))
    return result


class GroupedGlyphTest(unittest.TestCase):
    def test_all_native_faces_raw_and_rle_pixels_exact_with_both_bounds(self):
        original = fixture()
        expected = independent_pixels(original)
        for size in grouped.GROUP_CHOICES:
            with self.subTest(size=size):
                result, report = grouped.repack(original, size)
                self.assertEqual(independent_pixels(result), expected)
                self.assertGreater(report['saved_bytes'], 0)
                self.assertGreater(report['groups'], 1)
                self.assertGreater(report['grouped_glyph_entries'], 0)
                self.assertLessEqual(report['maximum_group_raw_bytes'], size)
                self.assertEqual(report['resident_cache_growth_bytes'], 0)
                self.assertTrue(report['verification']['exact_payload_and_alpha'])
                self.assertNotIn(b'obsolete-unreferenced-font-tail', result)
                self.assertEqual(grouped.repack(result, size)[0], result)

    def test_incompressible_glyphs_keep_their_old_encoding(self):
        original = fixture(count=60, random_pixels=True)
        result, report = grouped.repack(original)
        self.assertEqual(report['groups'], 0)
        self.assertEqual(independent_pixels(original), independent_pixels(result))
        self.assertTrue(all(not row[6] & 0x8000 for row in grouped.view(result).glyphs))

    def test_previous_raw_versions_remain_supported(self):
        for version in (20, 23, 24, 27, 28, 29, 30, 31, 32):
            original = fixture(count=10, version=version)
            result, _ = grouped.repack(original)
            self.assertEqual(independent_pixels(original), independent_pixels(result))

    def test_exact_physical_order_and_aliases(self):
        original = fixture(count=800)
        source = grouped.view(original)
        batches = grouped.plan(source)
        expected, seen = [], set()
        for index in sorted(range(len(source.glyphs)), key=lambda i: (source.glyphs[i][1], i)):
            payload = bytes(source.glyph_payload(index))
            if payload not in seen:
                seen.add(payload);expected.append(payload)
        self.assertEqual([r.payload for b in batches for r in b.records], expected)
        result, _ = grouped.repack(original)
        target = grouped.view(result)
        locations = {}
        for i, row in enumerate(source.glyphs):
            payload = bytes(source.glyph_payload(i))
            locations.setdefault(payload, set()).add(target.glyphs[i][1])
        self.assertTrue(all(len(offsets) == 1 for offsets in locations.values()))
        self.assertLess(len(locations), len(source.glyphs))

    def test_bilingual_split_has_identical_art_and_exact_roundtrip(self):
        originals = {lang: fixture(lang, count=350 if lang == 'eng' else 450) for lang in ('eng', 'zhs')}
        packed = {lang: grouped.repack(data)[0] for lang, data in originals.items()}
        common, localized, receipt = split.split(packed)
        self.assertEqual(receipt['languages']['eng']['version'], 32)
        for lang in packed:
            self.assertEqual(split.reconstruct(common, localized[lang], lang), packed[lang])
            old, new = split.layout(originals[lang], lang), split.layout(packed[lang], lang)
            self.assertEqual(originals[lang][old['prefix']:old['prefix'] + old['artwork']],
                packed[lang][new['prefix']:new['prefix'] + new['artwork']])

    def test_malformed_group_descriptors_headers_and_streams_rejected(self):
        output, _ = grouped.repack(fixture())
        target = grouped.view(output)
        index = next(i for i, row in enumerate(target.glyphs) if row[6] & 0x8000)
        row = target.glyphs[index]
        at = row[1]
        relative, total, raw_at, raw_bytes = grouped.DESCRIPTOR.unpack_from(output, at)
        group = at + relative
        mutations = (
            (4, 31),
            (at, 0x7fffffff), (at + 4, 32), (at + 8, 65536), (at + 12, raw_bytes + 1),
            (group, 0), (group + 4, 65537), (group + 8, total - 31),
            (group + 12, 13), (group + 16, 0), (group + 20, 1), (group + 28, 1),
        )
        for offset, value in mutations:
            with self.subTest(offset=offset, value=value):
                broken = bytearray(output);struct.pack_into('<I', broken, offset, value)
                with self.assertRaises(ValueError):grouped.view(broken)
                with self.assertRaises(ValueError):split.layout(broken, 'eng')
        # Valid bounds but invalid compressed contents must fail the full
        # glyph readback, even though the split-only checker does not decode.
        broken = bytearray(output);broken[group + 32] ^= 255
        with self.assertRaises(ValueError):grouped.verify(fixture(), broken)
        broken = bytearray(output);struct.pack_into('<I', broken, at + 4, total + 1);struct.pack_into('<I', broken, group + 8, total - 31)
        with self.assertRaises(ValueError):grouped.view(broken)

    def test_key_geometry_art_and_locale_corruption_cannot_pass_proof(self):
        original = fixture();output, _ = grouped.repack(original)
        target = grouped.view(output);bounds = split.layout(output, 'eng')
        for offset in (target.glyph_at, target.glyph_at + 12, bounds['prefix'] + 33, target.strings[0][0]):
            with self.subTest(offset=offset):
                broken = bytearray(output);broken[offset] ^= 1
                with self.assertRaises(ValueError):grouped.verify(original, broken)

    def test_bad_limits_and_old_group_flag_fail_explicitly(self):
        for size in (0, 1024, 32767, 65537, 512 * 1024):
            with self.assertRaises(ValueError):grouped.repack(fixture(count=1), size)
        original = bytearray(fixture(count=1));at = grouped.view(original).glyph_at
        encoded = struct.unpack_from('<H', original, at + 14)[0]
        struct.pack_into('<H', original, at + 14, encoded | 0x8000)
        with self.assertRaises(ValueError):grouped.view(original)
        with self.assertRaises(ValueError):split.layout(original, 'eng')


if __name__ == '__main__':
    unittest.main()
