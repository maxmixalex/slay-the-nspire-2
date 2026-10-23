"""Public synthetic coverage for lossless NSP2/20 retained groups."""
from pathlib import Path
import random
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import repack_retained as retained
import split_content as split


def stored(raw):
    result = bytearray()
    for at in range(0, len(raw), 65536):
        chunk = raw[at:at + 65536]
        result.extend(bytes((1 if at == 0 else 2, (len(chunk) - 1) >> 8, (len(chunk) - 1) & 255)))
        result.extend(chunk)
    return bytes(result) + b'\0'


def fixture(language='eng', glyphs=1, incompressible=False):
    ni, ng, ns, nc, nk, nu = 6, glyphs, 6111, 1, 1, 1
    base = 40 + ni * 16 + ng * 16 + ns * 8 + nc * 16 + nk * 16 + nu * 4
    table, descriptors = base + 64, base + 80
    out = bytearray(descriptors + 16)

    def append(data):
        out.extend(bytes(len(out) & 1))
        offset = len(out)
        out.extend(data)
        return offset

    raw0 = random.Random(918).randbytes(8192) if incompressible else b'\x23\xa5' * 4096
    payloads = [(64, 64, 0, raw0), (2, 1, 1, bytes.fromhex('1122334400ff')),
        (2, 1, 3, bytes(i * 37 & 255 for i in range(512)) + bytes.fromhex('010200ad')),
        (2, 1, 4, struct.pack('<HHHB', 1, 0, 2, 1) + bytes.fromhex('1122003344ff'))]
    for (image_id, item) in zip((0, 2, 3, 4), payloads):
        width, height, fmt, raw = item
        offset = append(raw)
        tiles.IMAGE.pack_into(out, 40 + image_id * 16, offset, width, height, fmt, len(raw))
    out[56:72] = out[40:56]
    tiles.IMAGE.pack_into(out, 40 + 5 * 16, descriptors, 1, 1, 6, 16)
    tile = b'\x01\x01' + bytes(768)
    index = tiles.FRAME.pack(2, 4, 2) + bytes(6)
    for group_id, (kind, raw) in enumerate(((1, tile), (2, index))):
        encoded = stored(raw)
        group = tiles.GROUP.pack(b'TGR1', kind, group_id, len(raw), len(encoded), 0x12345678, 1, 0) + encoded
        offset = append(group)
        struct.pack_into('<II', out, table + group_id * 8, offset, len(group))
    glyph_at = 40 + ni * 16
    for i in range(ng):
        offset = append(bytes((55 + i,)))
        tiles.GLYPH.pack_into(out, glyph_at + i * 16, 65 + i, offset, 8, 1, 1, 1, 0)
    string_at = glyph_at + ng * 16
    offset = append(language.encode() + b'\0')
    for i in range(ns):
        struct.pack_into('<II', out, string_at + i * 8, offset, 3)
    tiles.CLIP.pack_into(out, string_at + ns * 8, 5, 1, 100, 0, 0, 0)
    tiles.GLOBAL.pack_into(out, base, b'TIL1', 1, 16, 14, 1, 1, 1, 1,
        descriptors, table, 770, 1, tiles.GROUP_LIMIT, tiles.ENCODED_LIMIT, 0, 0)
    tiles.DESCRIPTOR.pack_into(out, descriptors, base, 0, 0, len(index))
    tiles.HEADER.pack_into(out, 0, b'NSP2', 19, tiles.REFERENCE, len(out), ni, ng, ns, nc, nk, 0xdeadbeef)
    return bytes(out)


class RetainedPackTest(unittest.TestCase):
    def test_exact_payloads_and_page_costs(self):
        data = fixture()
        output, receipt, _ = retained.repack(data)
        source, target = retained.view(data), tiles.PackView(output, 1, versions=(20,))
        for image_id in range(5):
            self.assertEqual(source.image_raw(image_id), target.image_raw(image_id))
        self.assertEqual(target.images[0], target.images[1])
        self.assertEqual(tiles.TileReader(source).image_raw(5), tiles.TileReader(target).image_raw(5))
        costs = struct.unpack_from('<I', output, target.base_end + 56)[0]
        self.assertEqual(output[costs:costs + 6], bytes((2, 2, 1, 1, 1, 1)))
        self.assertEqual(struct.unpack_from('<I', output, 36)[0], 0)
        self.assertGreater(receipt['saved_bytes'], 0)
        self.assertEqual(receipt['new_resident_pixel_bytes'], 0)

    def test_bilingual_shared_artwork_with_different_directories(self):
        eng, _, batches = retained.repack(fixture('eng', glyphs=1))
        zhs, _, _ = retained.repack(fixture('zhs', glyphs=3), batches=batches)
        erow, zrow = split.layout(eng, 'eng'), split.layout(zhs, 'zhs')
        self.assertNotEqual(erow['prefix'], zrow['prefix'])
        self.assertEqual(eng[erow['prefix']:erow['prefix'] + erow['artwork']],
                         zhs[zrow['prefix']:zrow['prefix'] + zrow['artwork']])
        common, locales, receipt = split.split(dict(eng=eng, zhs=zhs))
        self.assertEqual(common[32:64], bytes(32))
        self.assertFalse(receipt['runtime_integrity_checks'])
        self.assertEqual(split.reconstruct(common, locales['eng'], 'eng'), eng)
        self.assertEqual(split.reconstruct(common, locales['zhs'], 'zhs'), zhs)

    def test_nonbeneficial_group_keeps_original_encoding(self):
        output, _, _ = retained.repack(fixture(incompressible=True), group_bytes=8192)
        target = tiles.PackView(output, 1, versions=(20,))
        self.assertEqual(target.images[0][3], 0)
        self.assertEqual(target.image_raw(0), retained.view(fixture(incompressible=True)).image_raw(0))

    def test_explicit_logo_replacement_preserves_catalog(self):
        replacement = (320, 240, 4, bytes(480))
        output, receipt, _ = retained.repack(fixture(), replacements={4: replacement})
        target = tiles.PackView(output, 1, versions=(20,))
        self.assertEqual(target.image_raw(4), replacement)
        self.assertEqual(target.counts, retained.view(fixture()).counts)
        self.assertEqual(receipt['verification']['replaced_image_ids'], [4])

    def test_requested_slice_bounds_and_group_target(self):
        output, _, _ = retained.repack(fixture())
        target = tiles.PackView(output, 1, versions=(20,))
        descriptor = target.images[0][0]
        for field, value in ((0, 0x80000000), (8, 0xffffffff), (12, (5 << 24) | 8192)):
            changed = bytearray(output)
            struct.pack_into('<I', changed, descriptor + field, value)
            bad = tiles.PackView(changed, 1, versions=(20,))
            with self.assertRaises(ValueError):
                bad.image_raw(0)
        with self.assertRaises(ValueError):
            retained.repack(fixture(), group_bytes=1024)
        with self.assertRaises(ValueError):
            retained.repack(fixture(), replacements={4: (321, 1, 0, bytes(642))})


if __name__ == '__main__':
    unittest.main()
