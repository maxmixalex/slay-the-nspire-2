"""Synthetic lossless UI7 hand grouping and insertion boundaries."""
from pathlib import Path
import random
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as tiles
import group_ui7_hands as hands
import patch_frontend_assets as frontend
import repack_retained as retained
from lz4_block import encode as lz4_encode
from test_repack_retained import fixture


def synthetic_rows(cache):
    rows = []
    colors = random.Random(8321).randbytes(24*3)
    for index in range(95):
        if index < 94:
            raw = bytearray()
            for y in range(240):
                if 40 <= y < 100:
                    # Non-symmetric, moving geometry with exact fractional
                    # alpha and mostly shared RGB565 bytes across poses.
                    raw.extend(struct.pack('<HHHB', 1, 37+index%47, 24, 1))
                    raw.extend(colors)
                    raw[-3] ^= (index*7+y) & 255
                else:
                    raw.extend(b'\0\0')
            width, height, fmt = 320, 240, 4
            key = f"room_ui7.{'merchant' if index < 47 else 'fake-merchant'}-hand-{index%47:02d}"
        else:
            raw, width, height, fmt = bytearray(b'\x12\x34'*4), 2, 2, 0
            key = 'synthetic-static-icon'
        raw = bytes(raw)
        payload = struct.pack('<BI', fmt, len(raw))+lz4_encode(raw)
        filename = f'pose-{index}.bin'
        (cache/filename).write_bytes(payload)
        rows.append(dict(file=filename, key=key, width=width, height=height, format=5,
                         decoded_format=fmt, bytes=len(payload), sha256=tiles.sha256(payload),
                         raw_sha256=tiles.sha256(raw)))
    return rows


class HandGroupingTest(unittest.TestCase):
    def test_exact_sparse_pixels_catalog_and_bilingual_insertion(self):
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            rows = synthetic_rows(cache)
            plan = hands.build_payload(rows, cache)
            self.assertGreater(plan['report']['grouped_images'], 0)
            self.assertGreater(plan['report']['saved_common_art_bytes'], 0)
            self.assertEqual(plan['report']['unreachable_payload_bytes'], 0)
            self.assertLessEqual(plan['report']['max_group_raw_bytes'], 128*1024)
            hashes = []
            for language, glyph_count in (('eng', 1), ('zhs', 3)):
                original, _report, _batches = retained.repack(
                    fixture(language, glyphs=glyph_count, world_profile=True), fast_images='none')
                source = tiles.PackView(original, 1262, versions=(20,))
                manifest = dict(images=rows, localized_text={language: {}})
                output, report = frontend.patch_pack(original, manifest, cache,
                    base_aux=1262, auxiliary=1262+len(rows), input_version=20, version=24,
                    texts=(), appended_art=plan)
                target = tiles.PackView(output, 1262+len(rows), versions=(24,))
                old_reader, new_reader = tiles.TileReader(source), tiles.TileReader(target)
                self.assertEqual(target.clips, source.clips)
                self.assertEqual(target.cards, source.cards)
                self.assertEqual(target.aux[:1262], source.aux)
                self.assertEqual(target.aux[1262:], list(range(len(source.images), len(target.images))))
                self.assertTrue(report['unchanged_old_payload'])
                self.assertTrue(report['unchanged_old_decoded_costs'])
                self.assertEqual(report['added_common_art_bytes'], len(plan['payload']))
                for image, row in enumerate(source.images):
                    self.assertEqual(new_reader.image_raw(image) if row[3] == 6 else target.image_raw(image),
                                     old_reader.image_raw(image) if row[3] == 6 else source.image_raw(image))
                for image, row in enumerate(rows, len(source.images)):
                    w, h, fmt, raw = target.image_raw(image)
                    self.assertEqual((w, h, fmt), (row['width'], row['height'], row['decoded_format']))
                    self.assertEqual(tiles.sha256(raw), row['raw_sha256'])
                self.assertEqual(target.images[-1][3], 5)
                for (before, size), (after, new_size) in zip(source.strings, target.strings):
                    self.assertEqual(size, new_size)
                    self.assertEqual(original[before:before+size+1], output[after:after+size+1])
                hashes.append(report['common_art_sha256'])
            self.assertEqual(hashes[0], hashes[1])

    def test_changed_cached_payload_or_pose_order_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            rows = synthetic_rows(cache)
            rows[0]['key'], rows[1]['key'] = rows[1]['key'], rows[0]['key']
            with self.assertRaisesRegex(ValueError, 'catalog order'):
                hands.build_payload(rows, cache)
            rows[0]['key'], rows[1]['key'] = rows[1]['key'], rows[0]['key']
            (cache/rows[0]['file']).write_bytes(b'corrupt')
            with self.assertRaisesRegex(ValueError, 'changed before grouping'):
                hands.build_payload(rows, cache)

    def test_animation_groups_and_noncontiguous_auxiliary_mapping(self):
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            rows = synthetic_rows(cache)
            for index in range(3):
                rows.append(dict(rows[index], key=f'synthetic-character.{index}'))
            plan = hands.build_payload(rows, cache, extra_ranges=[(95, 3)])
            self.assertEqual(plan['report']['extra_grouped_images'], 3)
            self.assertLessEqual(plan['report']['max_group_raw_bytes'], tiles.GROUP_LIMIT)
            original, _report, _batches = retained.repack(
                fixture(world_profile=True), fast_images='none')
            source = tiles.PackView(original, 1262, versions=(20,))
            mapping = [95, 94, 97]
            manifest = dict(images=rows, localized_text={'eng': {}}, auxiliary_rows=mapping,
                            clips=[dict(first=95, count=3, frame_ms=83, pivot=[0, 0])])
            output, report = frontend.patch_pack(original, manifest, cache,
                base_aux=1262, auxiliary=1265, input_version=20, version=24,
                texts=(), appended_art=plan)
            target = tiles.PackView(output, 1265, versions=(24,))
            self.assertEqual(target.aux[:1262], source.aux)
            self.assertEqual(target.aux[1262:], [len(source.images)+i for i in mapping])
            self.assertEqual(target.clips[:-1], source.clips)
            self.assertEqual(target.clips[-1], (len(source.images)+95, 3, 83, 0, 0, 0))
            self.assertEqual(report['appended_clips'], 1)
            for index in range(95, 98):
                image = len(source.images)+index
                self.assertEqual(target.images[image][3], 7)
                self.assertEqual(tiles.sha256(target.image_raw(image)[3]), rows[index]['raw_sha256'])
            manifest['auxiliary_rows'] = [95, 94, len(rows)]
            with self.assertRaisesRegex(ValueError, 'auxiliary image references'):
                frontend.patch_pack(original, manifest, cache, base_aux=1262,
                    auxiliary=1265, input_version=20, version=24, texts=(), appended_art=plan)
            with self.assertRaisesRegex(ValueError, 'Overlapping appended image groups'):
                hands.build_payload(rows, cache, extra_ranges=[(95, 3), (96, 2)])


if __name__ == '__main__':
    unittest.main()
