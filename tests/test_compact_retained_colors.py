"""Public synthetic RGR2 conversion, relocation and bilingual readback."""
from pathlib import Path
import json
import random
import struct
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import compact_retained_colors as colors
import content_tiles as tiles
import repack_retained
import split_content
import predict_index_resources as indices
from complete_retained_resources_ui17 import verify as verify_completed
from retained_filter import group_layout, transform_group
from test_repack_retained import fixture, stored


def source(language='eng', glyphs=1, group_bytes=128 * 1024):
    data = bytearray(fixture(language, glyphs))
    offset = tiles.IMAGE.unpack_from(data, 40)[0]
    rng = random.Random(3235)
    for pixel in range(4096):
        green = rng.randrange(64)
        red = (green // 2 + rng.randrange(3)) % 32
        blue = (green // 2 + rng.randrange(2)) % 32
        struct.pack_into('<H', data, offset + pixel * 2, red * 2048 + green * 32 + blue)
    packed, _receipt, _plan = repack_retained.repack(bytes(data), fast_images='none', group_bytes=group_bytes)
    data = bytearray(packed)
    struct.pack_into('<I', data, 4, 35)
    return bytes(data)


class RetainedColorPackTest(unittest.TestCase):
    def test_appended_groups_preserve_v41_predictive_indices_and_exact_pixels(self):
        original=source()
        indexed,_=indices.rewrite(original,indices.make_plan(indices.view(original),workers=1))
        old=colors.view(indexed);reader=tiles.TileReader(old)
        self.assertTrue(any(reader.predictors))
        with tempfile.TemporaryDirectory() as folder:
            choices=colors.plan(old,Path(folder),jobs=1)
            self.assertTrue(any(choice.payload is not None for choice in choices))
            result=colors.rewrite(old,choices)
            proof=verify_completed(old,result,choices,decode_groups=True)
            self.assertEqual(proof['original_version'],41)
            self.assertEqual(proof['output_version'],41)
            self.assertEqual(proof['complete_raw_retained_groups_compared'],len(choices))
            self.assertGreater(proof['complete_raw_retained_bytes_compared'],0)
            self.assertGreater(proof['saved_bytes'],0)
            exact=colors.verify(old,result)
            self.assertTrue(exact['all_images_decoded'])
            after=colors.view(result);new_reader=tiles.TileReader(after)
            self.assertEqual(new_reader.predictors,reader.predictors)
            # No second filtering, raw page change, or downgrade on repeated conversion.
            again=colors.plan(after,Path(folder),jobs=1)
            self.assertEqual(colors.rewrite(after,again),result)

    def test_own_receipt_reuses_exact_output_with_an_empty_cache(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            old = colors.view(source(group_bytes=4096))
            initial = colors.plan(old, root / 'warm', jobs=1)
            output = colors.rewrite(old, initial)
            (root / 'ui9-eng.nsp').write_bytes(output)
            receipt = root / 'retained-colors.json'
            # Reproduce the first release's receipt: it had mode/saved_bytes,
            # but neither chosen, stream nor a slice fingerprint.
            records = [{key: value for key, value in choice.receipt.items()
                        if key not in ('stream', 'slice_sha256')} for choice in initial]
            receipt.write_text(json.dumps(dict(version=36,
                locales={'eng': {'output_sha256': tiles.sha256(output)}}, results=records)))
            previous = colors.previous_results(receipt)
            with mock.patch.object(colors.lzma, 'compress', side_effect=AssertionError('Unexpected recompression')):
                repeated = colors.plan(old, root / 'cold', jobs=1, previous=previous)
            self.assertEqual(colors.rewrite(old, repeated), output)
            self.assertEqual(sum(row.receipt['saved_bytes'] for row in initial),
                             sum(row.receipt['saved_bytes'] for row in repeated))
            self.assertEqual(len(list((root / 'cold').glob('*.lzma2'))),
                             sum(row.payload is not None for row in initial))
            # Rerunning already-filtered input produces entries without a raw
            # fingerprint. Those must not cause a KeyError when reused.
            already = colors.plan(colors.view(output), root / 'warm', jobs=1)
            receipt.write_text(json.dumps(dict(results=[row.receipt for row in already])))
            self.assertEqual(len(colors.previous_results(receipt)),
                             sum('raw_sha256' in row.receipt for row in already))
            self.assertEqual(colors.rewrite(colors.view(output), already), output)
            # A receipt cannot import an unrelated sibling output.
            receipt.write_text(json.dumps(dict(version=36,
                locales={'eng': {'output_sha256': '0' * 64}}, results=records)))
            with self.assertRaisesRegex(ValueError, 'does not match'):
                colors.previous_results(receipt)

    def test_legacy_winner_populates_cache_and_missing_stream_is_recomputed(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            old = colors.view(source(group_bytes=4096))
            initial = colors.plan(old, root / 'warm', jobs=1)
            group, choice = next((group, choice) for group, choice in zip(colors.groups(old), initial)
                                if choice.payload is not None and
                                all(shape >> 24 != 1 for _at, shape in group.slices))
            row = choice.receipt
            previous = {(group.first_image, row['raw_sha256']): dict(
                chosen={'mode': row['mode']}, stream=row['stream'],
                stored_bytes=row['original_bytes'] - 32, best_bytes=row['output_bytes'] - 32)}
            cached = root / 'imported'
            cached.mkdir()
            with mock.patch.object(colors.lzma, 'compress', side_effect=AssertionError('Unexpected recompression')):
                imported = colors.candidate(old, group, cached, previous)
            self.assertEqual(imported.payload, choice.payload)
            self.assertEqual(len(list(cached.glob('*.lzma2'))), 1)
            self.assertTrue(imported.receipt['reused_candidate'])
            previous[group.first_image, row['raw_sha256']]['stream'] = str(root / 'missing.lzma2')
            recomputed_cache = root / 'recomputed'
            recomputed_cache.mkdir()
            recomputed = colors.candidate(old, group, recomputed_cache, previous)
            self.assertEqual(recomputed.payload, choice.payload)

    def test_unknown_receipt_shape_does_not_mean_nonbeneficial(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            old = colors.view(source(group_bytes=4096))
            initial = colors.plan(old, root / 'warm', jobs=1)
            group, choice = next((group, choice) for group, choice in zip(colors.groups(old), initial)
                                if choice.payload is not None and
                                all(shape >> 24 != 1 for _at, shape in group.slices))
            cache = root / 'unknown'
            cache.mkdir()
            previous = {(group.first_image, choice.receipt['raw_sha256']): {'chosen': None, 'unknown': True}}
            actual = colors.candidate(old, group, cache, previous)
            self.assertEqual(actual.payload, choice.payload)

    def test_exact_conversion_relocation_and_shared_art(self):
        with tempfile.TemporaryDirectory() as directory:
            before = {language: colors.view(source(language, count))
                      for language, count in (('eng', 1), ('zhs', 3))}
            choices = colors.plan(before['eng'], Path(directory), jobs=1)
            self.assertTrue(any(choice.payload for choice in choices))
            outputs = {}
            for language, old in before.items():
                result = colors.rewrite(old, choices)
                outputs[language] = result
                proof = colors.verify(old, result)
                self.assertEqual(proof['decoded_images'], len(old.images))
                self.assertTrue(proof['all_images_decoded'])
                self.assertLess(len(result), len(old.data))
                target = colors.view(result)
                self.assertEqual(target.version, 36)
                self.assertEqual(target.images[0], target.images[1])
                self.assertNotEqual(target.glyphs[0][1], old.glyphs[0][1])
                self.assertNotEqual(target.strings[0][0], old.strings[0][0])
                for group in colors.groups(target):
                    payload = target.block(group.offset, group.size)
                    raw_size, start, mode, slices = group_layout(payload, 36)
                    if mode:
                        filtered = tiles.decode_lzma2(payload[start:], raw_size)
                        raw = transform_group(filtered, slices, mode, True)
                        self.assertEqual(transform_group(raw, slices, mode), filtered)
                # A second conversion is byte-identical and does not add a
                # second filter or a redundant table to a previously saved group.
                again = colors.plan(target, Path(directory), jobs=1)
                self.assertTrue(all(choice.payload is None for choice in again))
                self.assertEqual(colors.rewrite(target, again), result)
            common, locales, _receipt = split_content.split(outputs)
            for language in outputs:
                self.assertEqual(split_content.reconstruct(common, locales[language], language), outputs[language])
            # Cache reuse must prove decoded filtered bytes, not trust a name.
            cached = sorted(Path(directory).glob('*.lzma2'))[0]
            cached.write_bytes(b'\0')
            with self.assertRaises(ValueError):
                colors.plan(before['eng'], Path(directory), jobs=1)

    def test_requested_group_version_table_and_stream_bounds(self):
        with tempfile.TemporaryDirectory() as directory:
            old = colors.view(source())
            choices = colors.plan(old, Path(directory), jobs=1)
            output = colors.rewrite(old, choices)
            target = colors.view(output)
            group = next(group for group in colors.groups(target)
                         if target.data[group.offset:group.offset + 4] == b'RGR2')
            cases = ((4, 0), (4, tiles.GROUP_LIMIT + 1), (8, 0), (12, 13),
                     (16, 0), (16, 0x20000000), (20, 0), (20, 3),
                     (24, 0), (28, 1), (32, 1), (36, 0), (36, (5 << 24) | 8192))
            for field, value in cases:
                changed = bytearray(output)
                struct.pack_into('<I', changed, group.offset + field, value)
                with self.subTest(field=field, value=value):
                    candidate = colors.view(changed)
                    with self.assertRaises(ValueError):
                        candidate.image_raw(group.first_image)
                    with self.assertRaises(ValueError):
                        split_content.layout(changed, 'eng')
            changed = bytearray(output)
            struct.pack_into('<I', changed, 4, 35)
            with self.assertRaises(ValueError):
                colors.view(changed).image_raw(group.first_image)
            with self.assertRaises(ValueError):
                split_content.layout(changed, 'eng')

    def test_catalog_mismatch_and_relocation_into_replaced_bytes_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            old = colors.view(source())
            choices = colors.plan(old, Path(directory), jobs=1)
            changed = bytearray(old.data)
            group = colors.groups(old)[0]
            changed[group.offset + 20] ^= 1
            with self.assertRaisesRegex(ValueError, 'Bilingual retained group'):
                colors.rewrite(colors.view(changed), choices)
            relocate = colors.Relocation([(10, 30, bytes(4)), (50, 70, bytes(6))])
            self.assertEqual([relocate(value) for value in (0, 10, 30, 49, 50, 70)],
                             [0, 10, 14, 33, 34, 40])
            for value in (11, 29, 51, 69):
                with self.assertRaises(ValueError):
                    relocate(value)

    def test_grouped_glyph_and_text_relative_links_remain_exact(self):
        old = colors.view(source())
        data = bytearray(old.data)

        def append(payload):
            data.extend(bytes(len(data) & 1))
            offset = len(data)
            data.extend(payload)
            return offset

        def group(raw):
            stream = stored(raw)
            payload = tiles.GROUP.pack(b'RGR1', len(raw), len(stream), 14, 1, 0, 0, 0) + stream
            at = append(payload)
            descriptor = colors.aligned(len(data))
            return append(struct.pack('<iIII', at - descriptor, len(payload), 0, len(raw)))

        glyph = old.glyph_payload(0)
        descriptor = group(glyph)
        row = old.glyphs[0]
        tiles.GLYPH.pack_into(data, old.glyph_at, row[0], descriptor, *row[2:6], row[6] | tiles.GLYPH_GROUP_FLAG)
        text = b'Exact grouped string: RGB565 alpha and all links stay intact.\0'
        descriptor = group(text)
        struct.pack_into('<II', data, old.string_at, descriptor, (len(text) - 1) | tiles.STRING_GROUP_FLAG)
        struct.pack_into('<I', data, 12, len(data))
        source_view = colors.view(data)
        with tempfile.TemporaryDirectory() as directory:
            choices = colors.plan(source_view, Path(directory), jobs=1)
            output = colors.rewrite(source_view, choices)
            colors.verify(source_view, output)
            target = colors.view(output)
            self.assertEqual(target.glyph_payload(0), glyph)
            self.assertEqual(target.string_payload(0), text)
            self.assertTrue(target.glyphs[0][6] & tiles.GLYPH_GROUP_FLAG)
            self.assertTrue(target.strings[0][1] & tiles.STRING_GROUP_FLAG)
            split_content.layout(output, 'eng')


if __name__ == '__main__':
    unittest.main()
