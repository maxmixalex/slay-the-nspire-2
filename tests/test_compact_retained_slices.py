"""Real converter/reader round-trips on synthetic retained gaps and aliases."""
from contextlib import redirect_stdout
from dataclasses import replace
import io
import lzma
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import compact_retained_colors as colors
import compact_retained_slices as compact
from complete_retained_resources_ui17 import decoded_group
import content_tiles as t
from retained_filter import transform_group, group_layout
import repack_retained as retained
import split_content as split
import storage_parts as storage
from test_repack_retained import fixture, stored


def payload(language='eng', *, mode=0, orphan=True, duplicate=False, compressed=False, separate=False):
    source = retained.view(fixture(language, glyphs=3 if language == 'zhs' else 1, incompressible=True))
    batches = retained.plan_images(source, fast_images='none')
    if separate:
        first, second, third, fourth = batches[0].records
        a = [replace(first, ids=[0]), second]
        b = [replace(first, ids=[1]), third, fourth]
        batches = [retained.Batch(rows, None, sum(len(r.raw) for r in rows), 0) for rows in (a, b)]
    if duplicate:
        # Two disjoint records with exactly equal pixels, not two descriptor
        # aliases. The production compactor may deduplicate only inside this group.
        first = batches[0].records[0]
        first.ids = [0]
        batches[0].records.insert(1, replace(first, ids=[1]))
        batches[0].raw_size += len(first.raw)
    for batch in batches:
        batch.encoded = stored(b''.join(record.raw for record in batch.records))
    result = retained.write_pack(source, batches, output_version=45)[0]
    if mode or compressed:
        source = colors.view(result)
        choices = []
        for group in colors.groups(source):
            old = source.block(group.offset, group.size)
            raw = decoded_group(old, source.version)
            encoded = lzma.compress(transform_group(raw, group.slices, mode) if mode else raw,
                format=lzma.FORMAT_RAW, filters=[dict(id=lzma.FILTER_LZMA2, dict_size=t.GROUP_LIMIT)])
            table = b''.join(struct.pack('<II', *row) for row in group.slices) if mode else b''
            tag = b'RGR3' if mode >= 8 else b'RGR2' if mode else b'RGR1'
            proposed = t.GROUP.pack(tag, len(raw), len(encoded), 14, len(group.slices),
                mode-8 if mode >= 8 else mode, len(table), 0) + table + encoded
            choices.append(colors.Choice(t.sha256(old), group.slices, proposed, {}))
        result = colors.rewrite(source, choices)
    if orphan:
        source = colors.view(result)
        result = bytearray(result)
        source_at = source.images[2][0]
        relative, size, raw_at, shape = retained.DESCRIPTOR.unpack_from(result, source_at)
        target_at = source.images[0][0]
        retained.DESCRIPTOR.pack_into(result, target_at, source_at+relative-target_at, size, raw_at, shape)
        for image in (0, 1):
            t.IMAGE.pack_into(result, 40+image*16, target_at, *source.images[2][1:])
        result = bytes(result)
    return result


class RetainedSliceTest(unittest.TestCase):
    def check_roundtrip(self, before, directory):
        old = colors.view(before)
        plan = compact.make_plan(old, directory, workers=2)
        after = compact.rewrite(old, plan)
        proof = compact.verify(old, after, plan)
        new = colors.view(after)
        self.assertEqual(old.version, new.version)
        self.assertEqual((len(before)-len(after)) % 4096, 0)
        for image in range(len(old.images)):
            a = t.TileReader(old) if old.images[image][3] == 6 else old
            b = t.TileReader(new) if new.images[image][3] == 6 else new
            self.assertEqual(a.image_raw(image), b.image_raw(image))
        self.assertTrue(proof['group_membership_unchanged'])
        return plan, after

    def test_all_existing_codecs_keep_pixels_hidden_rgb_alpha_and_page_phase(self):
        for mode in (0, 1, 2, 8, 9, 10):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as folder:
                before = payload(mode=mode)
                plan, after = self.check_roundtrip(before, Path(folder))
                self.assertLess(len(after), len(before))
                self.assertEqual(sum(row.receipt['removed_raw_bytes'] for row in plan), 8192)
                source, target = colors.view(before), colors.view(after)
                a, b = colors.groups(source)[0], colors.groups(target)[0]
                self.assertEqual(group_layout(source.block(a.offset, a.size), 45)[2], mode)
                self.assertEqual(group_layout(target.block(b.offset, b.size), 45)[2], mode)
                self.assertEqual(plan, compact.make_plan(source, Path(folder), workers=1))

    def test_disjoint_identical_slices_are_deduplicated_within_the_same_group(self):
        with tempfile.TemporaryDirectory() as folder:
            before = payload(orphan=False, duplicate=True)
            plan, after = self.check_roundtrip(before, Path(folder))
            self.assertLess(len(after), len(before))
            new = colors.view(after)
            self.assertEqual(new.image_raw(0), new.image_raw(1))
            offsets = [retained.DESCRIPTOR.unpack_from(after, new.images[i][0])[2] for i in (0, 1)]
            self.assertEqual(offsets[0], offsets[1])
            self.assertEqual(sum(row.receipt['removed_raw_bytes'] for row in plan), 8192)

    def test_complete_group_stays_byte_identical_and_small_gain_is_declined(self):
        with tempfile.TemporaryDirectory() as folder:
            original = payload(orphan=False)
            plan, after = self.check_roundtrip(original, Path(folder))
            self.assertTrue(all(row.payload is None for row in plan))
            self.assertEqual(after, original)
            # Keep the random main image, orphan only the small last slice.
            before = bytearray(payload(orphan=False, compressed=True))
            source = colors.view(before)
            t.IMAGE.pack_into(before, 40+4*16, *source.images[2])
            plan, after = self.check_roundtrip(bytes(before), Path(folder))
            self.assertEqual(after, before)
            self.assertEqual(plan[0].receipt['preserved_reason'], 'cannot_preserve_page_phase_profitably')

    def test_overlapping_live_slices_are_preserved(self):
        before = bytearray(payload(orphan=False))
        source = colors.view(before)
        off = source.images[2][0]
        relative, size, at, shape = retained.DESCRIPTOR.unpack_from(before, off)
        # This six-byte view now overlaps the large source image; both remain valid.
        retained.DESCRIPTOR.pack_into(before, off, relative, size, 0, shape)
        with tempfile.TemporaryDirectory() as folder:
            plan, after = self.check_roundtrip(bytes(before), Path(folder))
            self.assertEqual(after, before)
            self.assertEqual(plan[0].receipt['preserved_reason'], 'overlapping_live_slices')

    def test_nonimage_references_prevent_compaction_and_bad_cache_is_rejected(self):
        before = payload()
        source = colors.view(before)
        group = colors.groups(source)[0]
        with tempfile.TemporaryDirectory() as folder:
            cache = Path(folder)
            choice = compact.candidate(source, group, cache, {group.offset})
            self.assertIsNone(choice.payload)
            self.assertEqual(choice.receipt['preserved_reason'], 'nonimage_reference')
            compact.make_plan(source, cache)
            next((cache/'retained-slices').glob('*.bin')).write_bytes(b'bad')
            with self.assertRaises(ValueError):
                compact.make_plan(source, cache)

    def test_equal_pixels_in_separate_groups_are_not_regrouped(self):
        with tempfile.TemporaryDirectory() as folder:
            before = payload(orphan=False, separate=True)
            plan, after = self.check_roundtrip(before, Path(folder))
            self.assertEqual(len(plan), 2)
            self.assertEqual(after, before)
            # Reclaim only the first group's dead main slice; keep the same
            # bytes in the second group and every following disk page phase.
            source = colors.view(before)
            edited = bytearray(before)
            old_at, source_at = source.images[0][0], source.images[2][0]
            relative, size, raw_at, shape = retained.DESCRIPTOR.unpack_from(edited, source_at)
            retained.DESCRIPTOR.pack_into(edited, old_at, source_at+relative-old_at, size, raw_at, shape)
            t.IMAGE.pack_into(edited, 40, old_at, *source.images[2][1:])
            plan, after = self.check_roundtrip(bytes(edited), Path(folder))
            self.assertIsNotNone(plan[0].payload)
            self.assertIsNone(plan[1].payload)
            self.assertEqual(colors.view(after).image_raw(1), source.image_raw(1))
            for old, new in zip(colors.groups(colors.view(edited)), colors.groups(colors.view(after))):
                self.assertEqual((old.offset-new.offset) % 4096, 0)

    def test_real_grouped_glyph_reference_is_protected(self):
        before = bytearray(payload())
        source = colors.view(before)
        group = colors.groups(source)[0]
        before.extend(bytes(len(before) & 1))
        at = len(before)
        before.extend(retained.DESCRIPTOR.pack(group.offset-at, group.size, 0, 1))
        cp, _old, px, w, h, advance, _encoded = source.glyphs[0]
        t.GLYPH.pack_into(before, source.glyph_at, cp, at, px, w, h, advance, t.GLYPH_GROUP_FLAG)
        struct.pack_into('<I', before, 12, len(before))
        with tempfile.TemporaryDirectory() as folder:
            source = colors.view(before)
            self.assertIn(group.offset, compact.nonimage_groups(source))
            plan, after = self.check_roundtrip(bytes(before), Path(folder))
            self.assertEqual(after, before)
            self.assertEqual(plan[0].receipt['preserved_reason'], 'nonimage_reference')

    def test_incomplete_offset_map_and_invalid_padding_are_rejected(self):
        source = colors.view(payload())
        with tempfile.TemporaryDirectory() as folder:
            plan = compact.make_plan(source, Path(folder))
            original = plan[0].padding
            for value in (-2, 1, 4096):
                plan[0].padding = value
                with self.assertRaisesRegex(ValueError, 'padding'):
                    compact.rewrite(source, plan)
            plan[0].padding = original
            plan[0].raw_offsets = {}
            with self.assertRaisesRegex(ValueError, 'offset map'):
                compact.rewrite(source, plan)

    def test_bilingual_physical_installation_and_idempotent_second_pass(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = {code:payload(code, mode=10) for code in ('eng', 'zhs')}
            common, languages, _ = split.split(original, compress_directory=True)
            index, parts, _ = storage.partition(common, original['eng'])
            source = root/'input'
            source.mkdir()
            (source/'nSpire2.dat.tns').write_bytes(index)
            for name, data in parts.items():
                (source/name).write_bytes(data)
            for code, data in languages.items():
                (source/f'nSpire2.{code}.dat.tns').write_bytes(storage.compress_blocks(data)[0])
            with redirect_stdout(io.StringIO()):
                receipt = compact.convert(source, root/'output', root/'cache', locales=['eng', 'zhs'])
            self.assertTrue(receipt['complete'] and receipt['beneficial'])
            self.assertGreater(receipt['installed_bytes_saved'], 0)
            for name, row in receipt['files'].items():
                data = (root/'output'/name).read_bytes()
                self.assertEqual((len(data), t.sha256(data)), (row['bytes'], row['sha256']))
            with self.assertRaisesRegex(ValueError, 'fresh directory'):
                compact.convert(source, root/'output', root/'cache', locales=['eng', 'zhs'])
            shared = storage.join_parts((root/'output/nSpire2.dat.tns').read_bytes(),
                                        {n:(root/'output'/n).read_bytes() for n in storage.PART_FILES})
            for code in ('eng', 'zhs'):
                after = split.reconstruct(shared, (root/f'output/nSpire2.{code}.dat.tns').read_bytes(), code)
                compact.verify(colors.view(original[code]), after,
                               compact.make_plan(colors.view(original[code]), root/'cache'))
            with redirect_stdout(io.StringIO()):
                again = compact.convert(root/'output', root/'again', root/'cache', locales=['eng', 'zhs'])
            self.assertTrue(again['complete'])
            self.assertFalse(again['beneficial'])
            self.assertEqual(again['installed_bytes_saved'], 0)
            for path in (root/'output').glob('*.tns'):
                self.assertEqual(path.read_bytes(), (root/'again'/path.name).read_bytes())
            # Invalid or missing locales cannot leave a partial new installation.
            with self.assertRaises(ValueError):
                compact.convert(source, root/'failed', root/'cache', locales=['eng', 'fra'])
            self.assertFalse((root/'failed').exists())
            original_verify = compact.verify
            def fail_second(old, new, choices):
                if old.string_payload(split.MARKER) == b'zhs\0':
                    raise ValueError('controlled locale verification failure')
                return original_verify(old, new, choices)
            with mock.patch.object(compact, 'verify', fail_second), self.assertRaisesRegex(ValueError, 'controlled locale'):
                compact.convert(source, root/'failed-locale', root/'cache', locales=['eng', 'zhs'])
            self.assertEqual(list((root/'failed-locale').iterdir()), [])


if __name__ == '__main__':
    unittest.main()
