"""Synthetic live/shared/forward-pointer coverage for merchant compaction."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import compact_ui6_merchant as compact
import content_tiles as tiles
import patch_ui6_merchant as merchant
import repack_retained as retained
from test_repack_retained import fixture


def source_pack(forward=False):
    original = bytearray(fixture(world_profile=True))
    catalog = retained.view(original)
    for slot in range(5):
        struct.pack_into('<I', original, catalog.aux_at + slot * 4, slot)
    data, _, _ = retained.repack(bytes(original), group_bytes=4096, fast_images='none')
    data = bytearray(data)
    struct.pack_into('<I', data, 4, 23)
    if not forward:
        return bytes(data)
    source = tiles.PackView(data, 1262, versions=(23,))
    offset, _width, _height, fmt, _length = source.images[3]
    if fmt != 7:
        raise AssertionError('Fixture requires a retained descriptor')
    relative, size, raw_at, shape = retained.DESCRIPTOR.unpack_from(data, offset)
    end = compact.art_end(source)
    copy = bytes(data[offset + relative:offset + relative + size])
    inserted = len(copy) + (len(copy) & 1)
    out = data[:end] + copy + bytes(len(copy) & 1) + data[end:]
    retained.DESCRIPTOR.pack_into(out, offset, end - offset, size, raw_at, shape)
    for index, row in enumerate(source.glyphs):
        tiles.GLYPH.pack_into(out, source.glyph_at + index * tiles.GLYPH.size,
                             row[0], row[1] + inserted, *row[2:])
    for index, (at, length) in enumerate(source.strings):
        struct.pack_into('<II', out, source.string_at + index * 8, at + inserted, length)
    struct.pack_into('<I', out, 12, len(out))
    return bytes(out)


def corrected_pack(original, ids, cache):
    source = tiles.PackView(original, 1262, versions=(23,))
    candidates = {
        0: (64, 64, 0, b'\x67\x89' * 4096),
        1: (64, 64, 0, b'\x67\x89' * 4096),
        2: (2, 1, 1, bytes.fromhex('99aabbccddff')),
        4: (2, 1, 4, struct.pack('<HHHB', 1, 0, 2, 1) + bytes.fromhex('99aa00bbccff')),
    }
    rows = []
    for image_id in (*ids, 3):
        width, height, fmt, raw = candidates[image_id] if image_id != 3 else source.image_raw(3)
        name = f'correction-{image_id}.bin'
        (cache / name).write_bytes(raw)
        rows.append(dict(file=name, auxiliary=image_id, width=width, height=height,
                         format=fmt, decoded_format=fmt, bytes=len(raw),
                         sha256=tiles.sha256(raw), raw_sha256=tiles.sha256(raw)))
    data, report = merchant.patch_pack(original, dict(images=rows, clips=[]), cache, auxiliary=1262)
    if report['replaced_images'] != len(ids) or report['identical_images_skipped'] != 1:
        raise AssertionError('Fixture did not replace only the intended images')
    return data


class MerchantCompactionTests(unittest.TestCase):
    def check_case(self, ids, forward=False):
        original = source_pack(forward)
        with tempfile.TemporaryDirectory() as directory:
            corrected = corrected_pack(original, ids, Path(directory))
            output, receipt = compact.compact(original, corrected, auxiliary=1262)
        before = tiles.PackView(corrected, 1262, versions=(23,))
        after = tiles.PackView(output, 1262, versions=(23,))
        self.assertGreater(receipt['removed_bytes'], 0)
        self.assertEqual(receipt['image_entries'], len(before.images))
        self.assertTrue(receipt['all_images_decoded_equal'])
        self.assertTrue(receipt['all_current_encoded_payloads_equal'])
        self.assertEqual(before.clips, after.clips)
        self.assertEqual(before.cards, after.cards)
        self.assertEqual(before.aux, after.aux)
        self.assertEqual(before.counts, after.counts)
        self.assertEqual(tiles.TileReader(before).image_raw(5), tiles.TileReader(after).image_raw(5))
        for index in range(5):
            self.assertEqual(before.image_raw(index), after.image_raw(index))
        return original, corrected, output, receipt

    def test_shared_old_group_and_aliased_descriptor_are_retained(self):
        original, _corrected, output, receipt = self.check_case((0, 2))
        before = tiles.PackView(original, 1262, versions=(23,))
        after = tiles.PackView(output, 1262, versions=(23,))
        self.assertEqual(before.image_raw(0), after.image_raw(1))
        self.assertEqual(receipt['preserved_unreachable_decoded_slices_in_shared_groups'], 6)
        self.assertEqual(receipt['removed_bytes'], 16)

    def test_fully_unreferenced_group_and_descriptor_ranges_are_removed(self):
        original, corrected, _output, receipt = self.check_case((0, 1, 2))
        self.assertGreater(receipt['removed_bytes'], 32)
        self.assertEqual(receipt['preserved_unreachable_decoded_slices_in_shared_groups'], 6)
        # A different historical payload cannot authorize removing these bytes.
        bad = bytearray(original)
        source = tiles.PackView(original, 1262, versions=(23,))
        bad[source.images[2][0]] ^= 1
        with self.assertRaisesRegex(ValueError, 'historical artwork'):
            compact.compact(bytes(bad), corrected, auxiliary=1262)

    def test_positive_and_negative_retained_links_survive_intervening_removal(self):
        _original, corrected, output, receipt = self.check_case((0, 1, 2, 4), forward=True)
        before = tiles.PackView(corrected, 1262, versions=(23,))
        after = tiles.PackView(output, 1262, versions=(23,))
        previous = retained.DESCRIPTOR.unpack_from(corrected, before.images[3][0])[0]
        following = retained.DESCRIPTOR.unpack_from(output, after.images[3][0])[0]
        self.assertGreater(previous, 0)
        self.assertGreater(following, 0)
        self.assertLess(following, previous)
        self.assertLess(retained.DESCRIPTOR.unpack_from(output, after.images[0][0])[0], 0)
        self.assertEqual(receipt['preserved_unreachable_decoded_slices_in_shared_groups'], 0)


if __name__ == '__main__':
    unittest.main()
