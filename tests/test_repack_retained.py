"""Public synthetic coverage for lossless NSP2/20 groups and the v21 title extension."""
from pathlib import Path
import json
import random
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import repack_retained as retained
import patch_frontend_assets as frontend
import split_content as split
from lz4_block import encode as lz4_encode


def stored(raw):
    result = bytearray()
    for at in range(0, len(raw), 65536):
        chunk = raw[at:at + 65536]
        result.extend(bytes((1 if at == 0 else 2, (len(chunk) - 1) >> 8, (len(chunk) - 1) & 255)))
        result.extend(chunk)
    return bytes(result) + b'\0'


def fixture(language='eng', glyphs=1, incompressible=False, tail_image=False, world_profile=False):
    ni, ng, ns, nc, nk, nu = 67 if tail_image else 6, glyphs, 6111, 1, 1, 1
    if world_profile:
        ns, nc, nk, nu = 8192, 1705, 558, 1262
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
    if tail_image:
        for image_id in range(6, 66):
            out[40 + image_id * 16:56 + image_id * 16] = out[40:56]
        raw = b'\x67\x89' * 1024
        offset = append(raw)
        tiles.IMAGE.pack_into(out, 40 + 66 * 16, offset, 32, 32, 0, len(raw))
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
    if world_profile:
        for slot, _key in frontend.FRONTEND_TEXTS:
            struct.pack_into('<II', out, string_at + slot * 8, offset + 3, 0)
    for i in range(nc):
        tiles.CLIP.pack_into(out, string_at + ns * 8 + i * 16, 5, 1, 100, 0, 0, 0)
    tiles.GLOBAL.pack_into(out, base, b'TIL1', 1, 16, 14, 1, 1, 1, 1,
        descriptors, table, 770, 1, tiles.GROUP_LIMIT, tiles.ENCODED_LIMIT, 0, 0)
    tiles.DESCRIPTOR.pack_into(out, descriptors, base, 0, 0, len(index))
    tiles.HEADER.pack_into(out, 0, b'NSP2', 19, tiles.REFERENCE, len(out), ni, ng, ns, nc, nk, 0xdeadbeef)
    return bytes(out)


class RetainedPackTest(unittest.TestCase):
    def test_exact_payloads_and_page_costs(self):
        data = fixture()
        output, receipt, _ = retained.repack(data, fast_images='none')
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
        eng, _, batches = retained.repack(fixture('eng', glyphs=1), fast_images='none')
        zhs, _, _ = retained.repack(fixture('zhs', glyphs=3), batches=batches, fast_images='none')
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
        output, _, _ = retained.repack(fixture(incompressible=True), group_bytes=8192, fast_images='none')
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
        output, _, _ = retained.repack(fixture(), fast_images='none')
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

    def test_fast_ui_policy_retains_original_encoding_and_groups_other_images(self):
        source = fixture(tail_image=True)
        output, receipt, _ = retained.repack(source)
        old, new = retained.view(source), tiles.PackView(output, 1, versions=(20,))
        for image_id in (0, 1, 2, 3, 4, 65):
            self.assertEqual(new.images[image_id][3], old.images[image_id][3])
            self.assertEqual(new.image_raw(image_id), old.image_raw(image_id))
        self.assertEqual(new.images[66][3], 7)
        self.assertEqual(new.image_raw(66), old.image_raw(66))
        self.assertEqual(receipt['fast_image_policy'], 'ui')
        self.assertGreater(receipt['fast_stored_bytes'], 0)
        self.assertEqual(receipt['retained_groups'], 1)

    def test_title_extension_preserves_compendium_logo_clips_and_shared_art(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            rows, expected = {}, {}
            for key, width, height in (('backdrop', 320, 240), ('foreground', 320, 240), ('particles', 270, 216)):
                # Empty scanlines exercise format 5 while remaining generated
                # test data, independent of every private source texture.
                raw = bytes(height * 2)
                encoded = struct.pack('<BI', 4, len(raw)) + lz4_encode(raw)
                (path / (key + '.bin')).write_bytes(encoded)
                rows[key] = dict(width=width, height=height, format=4, raw_file=key + '.raw', bytes=len(raw),
                    stored_file=key + '.bin', stored_format=5, stored_bytes=len(encoded))
                expected[key] = (width, height, 4, raw)
            manifest = dict(images=rows, output_version=21, added_images=3, added_clips=0,
                            added_auxiliary=['TitleBackdrop', 'TitleForeground', 'TitleParticles'])
            manifest_path = path / 'title-background.json'
            manifest_path.write_text(json.dumps(manifest))
            frontend_rows = []
            for i in range(frontend.AUXILIARY - frontend.BASE_AUX):
                raw = bytes((17 + i, 32 + i)) * 4
                name = f'frontend-{i}.bin'
                (path / name).write_bytes(raw)
                frontend_rows.append(dict(file=name, width=2, height=2, format=0, bytes=len(raw),
                    sha256=tiles.sha256(raw), raw_sha256=tiles.sha256(raw)))
            outputs, frontend_outputs, batches = {}, {}, None
            localized_text = {language:{key:language + ' synthetic ' + str(slot)
                                      for slot,key in frontend.FRONTEND_TEXTS} for language in ('eng','zhs')}
            frontend_manifest = dict(images=frontend_rows,localized_text=localized_text)
            for language, glyphs in (('eng', 1), ('zhs', 3)):
                old = fixture(language, glyphs=glyphs, tail_image=True, world_profile=True)
                extended, receipt = retained.append_title_art(old, manifest_path)
                self.assertTrue(receipt['unchanged_old_payload'])
                self.assertEqual(receipt['appended_images'], 3)
                self.assertEqual(receipt['appended_clips'], 0)
                output, report, batches = retained.repack(extended, batches=batches, output_version=21)
                source = retained.view(old)
                target = tiles.PackView(output, 1265, versions=(21,), profile=(558, 8192, 1705, 1265))
                self.assertEqual(target.aux[:1262], source.aux)
                self.assertEqual(target.aux[1262:], [67, 68, 69])
                self.assertEqual(target.images[66][3], 7)
                self.assertEqual(target.clips, source.clips)
                self.assertEqual(target.cards, source.cards)
                for i in range(5):
                    self.assertEqual(target.image_raw(i), source.image_raw(i))
                for i, key in enumerate(retained.TITLE_IMAGES):
                    self.assertEqual(target.image_raw(67 + i), expected[key])
                    self.assertEqual(target.images[67 + i][3], 5)
                self.assertEqual(tiles.TileReader(target).image_raw(5), tiles.TileReader(source).image_raw(5))
                self.assertEqual(report['output_version'], 21)
                outputs[language] = output
                # v22 inserts a larger image/auxiliary directory and decoded
                # cost table without disturbing old format-7 relative links,
                # tile descriptors, animation blocks, localized text or art.
                extended, report = frontend.patch_pack(output, frontend_manifest, path)
                newer = tiles.PackView(extended, frontend.AUXILIARY, versions=(22,))
                self.assertEqual(newer.aux[:1265], target.aux)
                self.assertTrue(report['unchanged_old_payload'])
                self.assertTrue(report['unchanged_old_decoded_costs'])
                self.assertEqual(report['appended_images'], frontend.AUXILIARY - frontend.BASE_AUX)
                self.assertEqual(report['appended_clips'], 0)
                self.assertEqual(report['localized_slots'], [slot for slot, _key in frontend.FRONTEND_TEXTS])
                self.assertTrue(report['unchanged_preexisting_strings'])
                new_labels = dict(frontend.FRONTEND_TEXTS)
                for slot, ((old_at, old_length), (new_at, new_length)) in enumerate(zip(target.strings, newer.strings)):
                    expected_label = localized_text[language][new_labels[slot]].encode() + b'\0' if slot in new_labels else output[old_at:old_at+old_length+1]
                    self.assertEqual(extended[new_at:new_at+new_length+1], expected_label)
                # A later catalog that occupies a reserved slot is rejected;
                # historical text can never be silently overwritten.
                occupied = bytearray(output)
                occupied_at = target.string_at + frontend.FRONTEND_TEXTS[0][0] * 8
                struct.pack_into('<II', occupied, occupied_at, *target.strings[6110])
                with self.assertRaisesRegex(ValueError, 'already occupied'):
                    frontend.patch_pack(occupied, frontend_manifest, path)
                for i, record in enumerate(target.images):
                    if record[3] == 6:
                        self.assertEqual(tiles.TileReader(newer).image_raw(i), tiles.TileReader(target).image_raw(i))
                    else:
                        self.assertEqual(newer.image_raw(i), target.image_raw(i))
                for i, row in enumerate(frontend_rows):
                    self.assertEqual(newer.image_raw(len(target.images) + i), (2, 2, 0, (path / row['file']).read_bytes()))
                frontend_outputs[language] = extended
            common, locales, _ = split.split(outputs)
            for language in outputs:
                self.assertEqual(split.reconstruct(common, locales[language], language), outputs[language])
            common, locales, _ = split.split(frontend_outputs)
            for language in frontend_outputs:
                self.assertEqual(split.reconstruct(common, locales[language], language), frontend_outputs[language])
            manifest['added_auxiliary'] = ['TitleForeground', 'TitleParticles']
            manifest_path.write_text(json.dumps(manifest))
            with self.assertRaises(ValueError):
                retained.append_title_art(fixture(world_profile=True), manifest_path)


if __name__ == '__main__':
    unittest.main()
