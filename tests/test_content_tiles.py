"""Synthetic byte/format/bounds tests for the private-pack lossless repacker."""
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
from lz4_block import encode as lz4_encode


def sparse(width, height, salt=0, empty=False):
    """Independent authored fmt4 bytes, including colored alpha-zero pixels."""
    output = bytearray()
    for y in range(height):
        spans = []
        if not empty and y % 3:
            spans.append((0, min(2, width), 0))
            if width > 3:
                spans.append((3, width - 3, 1))
        elif not empty and y % 2:
            spans.append((1, width - 1, 1))
        output.extend(struct.pack('<H', len(spans)))
        for x, count, mode in spans:
            output.extend(struct.pack('<HHB', x, count, mode))
            for at in range(x, x + count):
                color = (0x1234 + salt * 23 + at * 317 + y * 191) & 65535
                output.extend(struct.pack('<H', color))
                if mode:
                    output.append((at + y + salt) * 73 % 256 if (at + y) % 7 else 0)
    return bytes(output)


def fixture(extra=0, shared_retained=False, new_frames=None, string_values=None):
    # Image 0, the only old clip/card/auxiliary image, remains direct RGB565.
    frames = [(4, 2, 0, bytes(range(16)))]
    if new_frames is None:
        raw = sparse(33, 17, 3)
        frames.extend([(33, 17, 4, raw), (33, 17, 5, struct.pack('<BI', 4, len(raw)) + lz4_encode(raw))])
        frames.append((17, 3, 4, sparse(17, 3, 9)))
        frames.append((48, 2, 4, sparse(48, 2, empty=True)))
    else:
        frames.extend(new_frames)
    for i in range(extra):
        frames.append((17, 3, 4, sparse(17, 3, 20 + i)))
    if shared_retained:
        frames.append(frames[0])
    ni, ng, ns, nc, nk, nu = len(frames), 2, 2, 2, 1, 1
    base = 40 + ni * 16 + ng * 16 + ns * 8 + nc * 16 + nk * 16 + nu * 4
    data = bytearray(base)
    seen = {}

    def payload(raw, align=1):
        key = (align, raw)
        if key not in seen:
            data.extend(bytes((-len(data)) % align))
            seen[key] = len(data)
            data.extend(raw)
        return seen[key]

    for i, (w, h, fmt, raw) in enumerate(frames):
        tiles.IMAGE.pack_into(data, 40 + i * 16, payload(raw, 2), w, h, fmt, len(raw))
    at = 40 + ni * 16
    # One plain glyph and one RLE glyph; neither is an original font bitmap.
    tiles.GLYPH.pack_into(data, at, 63, payload(b'\x11\x22'), 8, 2, 1, 2, 0)
    tiles.GLYPH.pack_into(data, at + 16, 200, payload(b'\x02\x33'), 10, 2, 1, 2, 2)
    at += ng * 16
    for i, raw in enumerate(string_values or (b'\0', 'synthetic 文本'.encode() + b'\0')):
        struct.pack_into('<II', data, at + i * 8, payload(raw), len(raw) - 1)
    at += ns * 8
    tiles.CLIP.pack_into(data, at, 0, 1, 33, -1, 1, 0)
    tiles.CLIP.pack_into(data, at + 16, 1, ni - 1, 57, -12, 19, 0)
    at += nc * 16
    struct.pack_into('<4I', data, at, 0, 0, 1, 0)
    tiles.HEADER.pack_into(data, 0, b'NSP2', 14, tiles.REFERENCE, len(data), ni, ng, ns, nc, nk,
                           tiles.fnv(memoryview(data)[40:]))
    return bytes(data)


def seal(data):
    struct.pack_into('<I', data, 12, len(data))
    struct.pack_into('<I', data, 36, tiles.fnv(memoryview(data)[40:]))
    return bytes(data)


class ContentTiles(unittest.TestCase):
    def setUp(self):
        self.temporary = TemporaryDirectory(prefix='nspire-tiles-test-')
        self.root = Path(self.temporary.name)
        self.source = self.root / 'input.nsp'
        self.source.write_bytes(fixture())

    def tearDown(self):
        self.temporary.cleanup()

    def convert(self, per_group=85, name='output.nsp', **kwargs):
        output = self.root / name
        receipt = tiles.repack(self.source, output, self.root / 'work', auxiliary_count=1,
                               first_clip=1, tiles_per_group=per_group, workers=1, **kwargs)
        return output, receipt

    def test_all_group_sizes_roundtrip_exact_original_bytes_and_aliases(self):
        original = tiles.PackView(self.source.read_bytes(), 1)
        for per in (1, 85, 170, 680):
            with self.subTest(per_group=per):
                output, receipt = self.convert(per, f'output-{per}.nsp')
                packed = tiles.PackView(output.read_bytes(), 1, versions=(15,))
                reader = tiles.TileReader(packed)
                self.assertEqual(receipt['statistics']['canonical_frames'], 3)
                self.assertEqual(receipt['statistics']['selected_images'], 4)
                self.assertEqual(packed.images[1], packed.images[2])
                self.assertEqual(reader.image_raw(1), original.image_raw(1))
                self.assertEqual(reader.image_raw(2), original.image_raw(2))
                self.assertEqual(reader.image_raw(3), original.image_raw(3))
                self.assertEqual(reader.image_raw(4), original.image_raw(4))
                self.assertTrue(receipt['verification']['all_original_fmt4_bytes_equal'])
                self.assertEqual(sum(receipt['byte_accounting'].values()), output.stat().st_size)
                self.assertEqual(self.source.read_bytes(), fixture())
                self.assertLessEqual(receipt['max_group_encoded_bytes'], tiles.ENCODED_LIMIT)
                self.assertLessEqual(receipt['max_group_raw_bytes'], tiles.GROUP_LIMIT)
                self.assertEqual(reader.descriptors, reader.prefix_end)
                if per == 1:
                    self.assertEqual(reader.tile_groups, reader.tile_count)

    def test_old_payload_shared_by_new_timing_entry_stays_retained(self):
        self.source.write_bytes(fixture(shared_retained=True))
        output, receipt = self.convert()
        original = tiles.PackView(self.source.read_bytes(), 1)
        result = tiles.PackView(output.read_bytes(), 1, versions=(15,))
        self.assertEqual(receipt['statistics']['retained_images'], 2)
        self.assertEqual(result.image_raw(len(result.images) - 1), original.image_raw(len(original.images) - 1))
        self.assertEqual(result.images[0], result.images[-1])

    def test_repeat_build_reuses_strictly_verified_cache_and_is_deterministic(self):
        first, a = self.convert(name='first.nsp')
        second, b = self.convert(name='second.nsp')
        self.assertEqual(first.read_bytes(), second.read_bytes())
        self.assertEqual(b['verified_cache_hits'], b['tile_groups'] + b['index_groups'])
        self.assertEqual(a['output_sha256'], b['output_sha256'])

    def test_separate_cache_import_checks_metadata_payload_and_raw_bytes(self):
        first, _ = self.convert(name='first.nsp')
        output = self.root / 'imported.nsp'
        receipt = tiles.repack(self.source, output, self.root / 'different-work', auxiliary_count=1,
            first_clip=1, tiles_per_group=85, workers=1, reuse=[self.root / 'work' / 'encoded'])
        self.assertEqual(first.read_bytes(), output.read_bytes())
        self.assertEqual(receipt['verified_cache_hits'], receipt['tile_groups'] + receipt['index_groups'])
        payload = next((self.root / 'work' / 'encoded').glob('*.lzma2'))
        payload.write_bytes(payload.read_bytes() + b'bad')
        with self.assertRaisesRegex(ValueError, 'size outside bound'):
            tiles.EncodedCache(self.root / 'reject-import', [self.root / 'work' / 'encoded'])

    def test_hash_collision_never_substitutes_different_tile_bytes(self):
        path = self.root / 'collision.raw'
        spool = tiles.TileSpool(path)
        a = bytes((16, 16)) + bytes(768)
        b = bytes((16, 16)) + b'\x01' + bytes(767)
        try:
            with patch.object(tiles.TileSpool, 'key', return_value=b'forced-same-key'):
                self.assertEqual(spool.add(a), 0)
                self.assertEqual(spool.add(b), 1)
                self.assertEqual(spool.add(a), 0)
                self.assertEqual(spool.add(b), 1)
            self.assertEqual(spool.count, 2)
            self.assertEqual(spool.duplicates, 2)
            self.assertGreaterEqual(spool.collisions, 2)
        finally:
            spool.close()
        self.assertEqual(path.read_bytes(), a + b)

    def test_canonical_frame_hash_collision_compares_original_bytes(self):
        source = tiles.PackView(fixture(extra=2), 1)
        with patch('content_tiles.sha256', return_value='forced-frame-collision'):
            plan = tiles.build_plan(source, self.root, 1, 85)
        self.assertEqual(len(plan.frames), 5)
        self.assertEqual(plan.mapping[1], plan.mapping[2])
        self.assertNotEqual(plan.mapping[3], plan.mapping[5])
        self.assertGreater(plan.stats['canonical_hash_collisions_compared'], 0)

    def test_empty_canvas_edges_and_zero_alpha_colors_are_preserved(self):
        raw = sparse(17, 3, 9)
        geometry, colors, alpha = tiles.validate_sparse(raw, 17, 3, pixels=True)
        self.assertTrue(np.any((colors != 0) & (alpha == 0)))
        self.assertLess(len(geometry), len(raw))
        output, _ = self.convert()
        reader = tiles.TileReader(tiles.PackView(output.read_bytes(), 1, versions=(15,)))
        self.assertEqual(reader.image_raw(3)[3], raw)
        self.assertEqual(reader.image_raw(4)[3], b'\0\0\0\0')

    def test_index_group_starts_at_new_record_after_64_frames(self):
        self.source.write_bytes(fixture(extra=64))
        output, receipt = self.convert()
        self.assertEqual(receipt['statistics']['canonical_frames'], 67)
        self.assertEqual(receipt['index_groups'], 2)
        reader = tiles.TileReader(tiles.PackView(output.read_bytes(), 1, versions=(15,)))
        self.assertEqual(reader.groups[reader.tile_groups][-1], 64)
        self.assertEqual(reader.groups[reader.tile_groups + 1][-1], 3)
        self.assertEqual(reader.image_raw(68)[3], sparse(17, 3, 83))
        final_tile_group = reader.groups[reader.tile_groups - 1]
        self.assertEqual(final_tile_group[-1], reader.tile_count - (reader.tile_groups - 1) * reader.per_group)

    def test_maximum_original_sparse_size_retains_exact_cache_class(self):
        raw = bytearray()
        for y in range(240):
            raw.extend(struct.pack('<H', 320))
            for x in range(320):
                raw.extend(struct.pack('<HHBHB', x, 1, 1, (x * 317 + y * 193) & 65535, (x + y) % 256))
        self.assertEqual(len(raw), tiles.IMAGE_LIMIT)
        self.source.write_bytes(fixture(new_frames=[(320, 240, 4, bytes(raw))]))
        output, receipt = self.convert(680)
        reader = tiles.TileReader(tiles.PackView(output.read_bytes(), 1, versions=(15,)))
        self.assertEqual(reader.image_raw(1), (320, 240, 4, bytes(raw)))
        self.assertEqual(receipt['statistics']['original_canonical_fmt4_bytes'], tiles.IMAGE_LIMIT)
        self.assertEqual(reader.groups[reader.tile_groups][2], 12 + 1200 + 480 + 5 * 320 * 240)

    def test_sparse_validation_rejects_overlap_mode_and_trailing_bytes(self):
        invalid = [b'\x01\0', struct.pack('<HHHB', 1, 0, 0, 0),
                   struct.pack('<HHHB', 1, 0, 1, 2) + b'\0\0\0\0',
                   struct.pack('<HHHB', 1, 2, 2, 0) + b'\0' * 4,
                   b'\0\0extra',
                   struct.pack('<HHHB', 2, 1, 1, 0) + b'\0\0' + struct.pack('<HHB', 1, 1, 0) + b'\0\0']
        for raw in invalid:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                tiles.validate_sparse(raw, 3, 1)

    def test_lzma_end_marker_truncation_declared_length_and_trailing_bytes(self):
        cache = tiles.EncodedCache(self.root / 'cache')
        row = cache.encode(b'abcd' * 200)
        encoded = Path(row['path']).read_bytes()
        self.assertEqual(tiles.decode_lzma2(encoded, 800), b'abcd' * 200)
        for block, size in ((encoded[:-1], 800), (encoded + b'\0', 800), (encoded, 799), (encoded, 801)):
            with self.subTest(size=size, length=len(block)), self.assertRaises(ValueError):
                tiles.decode_lzma2(block, size)

    def test_corrupt_cached_payload_stops_before_publishing(self):
        output, _ = self.convert()
        before = output.read_bytes()
        payload = next((self.root / 'work' / 'encoded').glob('*.lzma2'))
        payload.write_bytes(b'broken')
        with self.assertRaises(ValueError):
            self.convert(name='failed.nsp')
        self.assertFalse((self.root / 'failed.nsp').exists())
        self.assertEqual(output.read_bytes(), before)
        self.assertEqual(self.source.read_bytes(), fixture())

    def test_output_capacity_and_existing_files_are_protected(self):
        with self.assertRaisesRegex(ValueError, 'size limit'):
            self.convert(maximum=100)
        self.assertFalse((self.root / 'output.nsp').exists())
        output, _ = self.convert()
        with self.assertRaisesRegex(ValueError, 'overwrite'):
            self.convert()
        with self.assertRaisesRegex(ValueError, 'must differ'):
            tiles.repack(self.source, self.source, self.root / 'work', auxiliary_count=1,
                         first_clip=1, tiles_per_group=85)
        self.assertTrue(output.exists())

    def test_input_output_and_receipt_cannot_overlap_writable_cache_namespace(self):
        work = self.root / 'work'
        encoded = work / 'encoded'
        encoded.mkdir(parents=True)
        unsafe_input = encoded / 'valid-pack-with-cache-metadata-name.json'
        original = fixture()
        unsafe_input.write_bytes(original)
        with self.assertRaisesRegex(ValueError, 'cache namespace'):
            tiles.repack(unsafe_input, self.root / 'failed.nsp', work, auxiliary_count=1,
                         first_clip=1, tiles_per_group=85, maximum=100)
        self.assertEqual(unsafe_input.read_bytes(), original)
        self.assertFalse((self.root / 'failed.nsp').exists())
        input_link = encoded / 'input-symlink.json'
        input_link.symlink_to(self.source)
        with self.assertRaisesRegex(ValueError, 'cache namespace'):
            tiles.repack(input_link, self.root / 'link-failed.nsp', work, auxiliary_count=1,
                         first_clip=1, tiles_per_group=85, maximum=100)
        self.assertTrue(input_link.is_symlink())
        self.assertEqual(self.source.read_bytes(), original)
        alias = self.root / 'cache-alias'
        alias.symlink_to(encoded, target_is_directory=True)
        for bad_output, bad_receipt in ((alias / 'pack.nsp', None),
                                        (self.root / 'pack.nsp', alias / 'receipt.json')):
            with self.assertRaisesRegex(ValueError, 'cache namespace'):
                tiles.repack(self.source, bad_output, work, auxiliary_count=1, first_clip=1,
                             tiles_per_group=85, receipt_path=bad_receipt)

    def test_input_pathname_replacement_cannot_hide_behind_unchanged_mmap(self):
        original_verify = tiles.verify_pack
        def replace_after_verification(*args, **kwargs):
            result = original_verify(*args, **kwargs)
            replacement = self.root / 'replacement'
            replacement.write_bytes(self.source.read_bytes())
            replacement.replace(self.source)
            return result
        with patch('content_tiles.verify_pack', side_effect=replace_after_verification):
            with self.assertRaisesRegex(ValueError, 'pathname changed'):
                self.convert()
        self.assertFalse((self.root / 'output.nsp').exists())

    def test_cli_guards_original_symlink_entry_before_normalizing_paths(self):
        work = self.root / 'work'
        encoded = work / 'encoded'
        encoded.mkdir(parents=True)
        link = encoded / 'reserved-name.json'
        target = self.root / 'outside-cache-output'
        link.symlink_to(target)
        for output, receipt in ((link, self.root / 'receipt.json'),
                                (self.root / 'output.nsp', link)):
            with self.assertRaisesRegex(ValueError, 'cache namespace'):
                tiles.main(['--input', str(self.source), '--output', str(output),
                            '--work', str(work), '--receipt', str(receipt)])
        self.assertTrue(link.is_symlink())
        self.assertFalse(target.exists())

    def test_unused_checksum_ignored_and_catalog_clip_bounds_rejected(self):
        data = bytearray(self.source.read_bytes())
        data[36] ^= 1
        tiles.PackView(data, 1)
        with self.assertRaisesRegex(ValueError, 'catalog'):
            tiles.PackView(self.source.read_bytes(), 1, profile=(1, 2, 3, 1))
        source = tiles.PackView(self.source.read_bytes(), 1)
        data = bytearray(self.source.read_bytes())
        struct.pack_into('<I', data, source.clip_at, 999)
        with self.assertRaisesRegex(ValueError, 'clip'):
            tiles.PackView(seal(data), 1)

    def test_glyph_order_and_runtime_string_limit_match_native_contract(self):
        original = self.source.read_bytes()
        source = tiles.PackView(original, 1)
        changed = bytearray(original)
        at = source.glyph_at
        changed[at:at + 32] = original[at + 16:at + 32] + original[at:at + 16]
        with self.assertRaisesRegex(ValueError, 'strictly increasing'):
            tiles.PackView(seal(changed), 1)
        valid = fixture(string_values=(b'\0', b'a' * (tiles.STRING_LIMIT - 1) + b'\0'))
        tiles.PackView(valid, 1)
        invalid = fixture(string_values=(b'\0', b'a' * tiles.STRING_LIMIT + b'\0'))
        with self.assertRaisesRegex(ValueError, 'String exceeds'):
            tiles.PackView(invalid, 1)

    def test_plain_payloads_cannot_alias_descriptor_bytes(self):
        output, _ = self.convert()
        original = output.read_bytes()
        pack = tiles.PackView(original, 1, versions=(15,))
        reader = tiles.TileReader(pack)
        # Empty string zero can otherwise point into zero-valued descriptor
        # fields and pass semantic byte equality, yet violate native bounds.
        changed = bytearray(original)
        struct.pack_into('<I', changed, pack.string_at, reader.descriptors + 4)
        with self.assertRaisesRegex(ValueError, 'String invades descriptor'):
            tiles.TileReader(tiles.PackView(seal(changed), 1, versions=(15,)))
        changed = bytearray(original)
        struct.pack_into('<I', changed, 40, reader.descriptors)
        with self.assertRaisesRegex(ValueError, 'Image invades descriptor'):
            tiles.TileReader(tiles.PackView(seal(changed), 1, versions=(15,)))
        changed = bytearray(original)
        struct.pack_into('<I', changed, pack.glyph_at + 4, reader.descriptors)
        with self.assertRaisesRegex(ValueError, 'Glyph invades descriptor'):
            tiles.TileReader(tiles.PackView(seal(changed), 1, versions=(15,)))

    def test_legal_even_descriptor_gap_is_readable(self):
        output, _ = self.convert()
        original = output.read_bytes()
        pack = tiles.PackView(original, 1, versions=(15,))
        reader = tiles.TileReader(pack)
        # Insert an even but non-word-aligned gap. Descriptor-relative, not
        # absolute, sixteen-byte membership is the native format contract.
        gap, split = 2, reader.descriptors
        changed = bytearray(original[:split] + bytes(gap) + original[split:])
        struct.pack_into('<I', changed, pack.base_end + 32, reader.descriptors + gap)
        for i, entry in enumerate(pack.images):
            struct.pack_into('<I', changed, 40 + i * 16, entry[0] + gap)
        for i, entry in enumerate(pack.glyphs):
            struct.pack_into('<I', changed, pack.glyph_at + i * 16 + 4, entry[1] + gap)
        for i, entry in enumerate(pack.strings):
            struct.pack_into('<I', changed, pack.string_at + i * 8, entry[0] + gap)
        table = pack.base_end + tiles.GLOBAL.size
        for i in range(len(reader.groups)):
            offset, = struct.unpack_from('<I', changed, table + i * 8)
            struct.pack_into('<I', changed, table + i * 8, offset + gap)
        shifted = tiles.TileReader(tiles.PackView(seal(changed), 1, versions=(15,)))
        self.assertEqual(shifted.image_raw(1), reader.image_raw(1))

    def test_reader_ignores_legacy_reserved_and_rejects_ranges_and_descriptor_interior(self):
        output, _ = self.convert()
        data = output.read_bytes()
        pack = tiles.PackView(data, 1, versions=(15,))
        reader = tiles.TileReader(pack)
        # The v15 trailing header word is unused, so it must not block the
        # selected frame. Actual group and index slice bounds still apply.
        changed = bytearray(data)
        struct.pack_into('<I', changed, pack.base_end + 60, 1)
        accepted = tiles.TileReader(tiles.PackView(seal(changed), 1, versions=(15,)))
        self.assertEqual(accepted.image_raw(1), reader.image_raw(1))
        mutations = [
            (pack.base_end + 44, 681, 'tile counts'),
            (pack.base_end + 64, reader.prefix_end - 2, 'group range'),
        ]
        for offset, value, text in mutations:
            changed = bytearray(data)
            struct.pack_into('<I', changed, offset, value)
            with self.subTest(text=text), self.assertRaisesRegex(ValueError, text):
                tiles.TileReader(tiles.PackView(seal(changed), 1, versions=(15,)))
        changed = bytearray(data)
        struct.pack_into('<I', changed, reader.descriptors + 8, 1)
        bad = tiles.TileReader(tiles.PackView(seal(changed), 1, versions=(15,)))
        with self.assertRaisesRegex(ValueError, 'complete index record'):
            bad.image_raw(1)

    def test_truncation_rejected_and_unused_group_crc_ignored(self):
        output, _ = self.convert()
        data = output.read_bytes()
        with self.assertRaisesRegex(ValueError, 'size mismatch'):
            tiles.PackView(data[:-1], 1, versions=(15,))
        pack = tiles.PackView(data, 1, versions=(15,))
        reader = tiles.TileReader(pack)
        changed = bytearray(data)
        first_header = reader.groups[0][0] - tiles.GROUP.size
        changed[first_header + 20] ^= 1
        bad = tiles.TileReader(tiles.PackView(seal(changed), 1, versions=(15,)))
        self.assertEqual(bad.group(0), reader.group(0))

    def test_tile_padding_dimensions_and_index_bounds(self):
        valid = bytes((1, 1)) + b'\x34\x12' + bytes(510) + b'\xff' + bytes(255)
        tiles.validate_tiles(valid)
        for offset in (2 + 2, 514 + 1, 2 + 16 * 2, 514 + 16):
            corrupt = bytearray(valid)
            corrupt[offset] = 1
            with self.subTest(offset=offset), self.assertRaisesRegex(ValueError, 'padding'):
                tiles.validate_tiles(corrupt)
        with self.assertRaisesRegex(ValueError, 'dimensions'):
            tiles.validate_tiles(b'\0\x10' + bytes(768))
        record = tiles.FRAME.pack(2, 4, 2) + struct.pack('<I', 0) + b'\0\0'
        self.assertEqual(tiles.index_records(record, 1, 1), {0: len(record)})
        for corrupted, count in ((record + b'\0', 1), (record, 2), (record[:12] + b'\x01\0\0\0' + record[16:], 1)):
            with self.assertRaises(ValueError):
                tiles.index_records(corrupted, count, 1)

    def test_group_and_spool_resource_limits_fail_explicitly(self):
        source = tiles.PackView(self.source.read_bytes(), 1)
        for per in (0, 681):
            with self.assertRaisesRegex(ValueError, 'tiles per group'):
                tiles.build_plan(source, self.root, 1, per)
        with self.assertRaisesRegex(ValueError, 'spool exceeds'):
            tiles.build_plan(source, self.root, 1, 85, spool_limit=770)
        with self.assertRaisesRegex(ValueError, 'outside the public'):
            tiles.private_path(tiles.ROOT / 'private-output.nsp', 'test')


if __name__ == '__main__':
    unittest.main()
