"""A named UI release must contain its actual content, not just a newer codec."""
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import release_content_guard as guard


def fixture(auxiliary=5422):
    # Public synthetic artwork. Preserve production directory dimensions and
    # clocks so a shape-only/format-only gate cannot pass these negative cases.
    ni, ng, ns, nc, nk = 58099, 1, 1, 1783, 1
    glyph_at = 40 + ni * 16
    string_at = glyph_at + 16
    clip_at = string_at + 8
    aux_at = clip_at + nc * 16 + 16
    base = aux_at + auxiliary * 4
    out = bytearray(base + 64)
    out[base:base + 4] = b'TIL1'

    def append(raw):
        if len(out) & 1:
            out.append(0)
        offset = len(out)
        out.extend(raw)
        return offset

    pixel = append(b'\x11\x22')
    image = tiles.IMAGE.pack(pixel, 1, 1, 0, 2)
    out[40:glyph_at] = image * ni
    glyph = append(b'\xff')
    tiles.GLYPH.pack_into(out, glyph_at, 65, glyph, 8, 1, 1, 1, 0)
    string = append(b'eng\0')
    struct.pack_into('<II', out, string_at, string, 3)
    clip = tiles.CLIP.pack(0, 1, 100, 0, 0, 5376)
    out[clip_at:clip_at + nc * 16] = clip * nc
    for clip_id, (frames, ms, reference) in zip(guard.CLIPS, guard.CLOCKS):
        tiles.CLIP.pack_into(out, clip_at + clip_id * 16, 0, frames, ms, 0, 0, reference)
    table_offsets = []
    table_ids = list(range(ni - 13, ni - 3))
    for row, image_id in enumerate(table_ids):
        if row < 9:
            frames, ms, reference = guard.CLOCKS[row]
            units = 5376
        else:
            frames, ms, units, reference = 3, 56, 4254, 4254
        words = [guard.MAGIC, 1, frames, ms, units, 0, reference, 0, 0, 3]
        words.extend([0] * (2 * frames))
        for frame in range(frames):
            words[10 + 2 * frame:12 + 2 * frame] = [len(words), 1]
            words.extend([frame if row < 9 else 0, 0, 0])
        words[8] = len(words)
        width = min(320, len(words))
        height = (len(words) + width - 1) // width
        words.extend([0] * (width * height - len(words)))
        raw = struct.pack('<' + 'H' * len(words), *words)
        offset = append(raw)
        table_offsets.append(offset)
        tiles.IMAGE.pack_into(out, 40 + image_id * 16, offset, width, height, 0, len(raw))
        if 5409 + row < auxiliary:
            struct.pack_into('<I', out, aux_at + (5409 + row) * 4, image_id)
    for i, image_id in enumerate(range(ni - 3, ni)):
        raw = struct.pack('<H', 0x1100 + i) * (23 * 23)
        offset = append(raw)
        tiles.IMAGE.pack_into(out, 40 + image_id * 16, offset, 23, 23, 0, len(raw))
        if 5419 + i < auxiliary:
            struct.pack_into('<I', out, aux_at + (5419 + i) * 4, image_id)
    tiles.HEADER.pack_into(out, 0, b'NSP2', 45, tiles.REFERENCE, len(out), ni, ng, ns, nc, nk, 0)
    return out, dict(aux=aux_at, tables=table_offsets, table_ids=table_ids)


class ReleaseContentGuardTest(unittest.TestCase):
    def test_complete_extension_and_older_stage_policy(self):
        data, _ = fixture()
        report = guard.validate_release_content(data, 'ui20')
        self.assertTrue(report['complete'])
        self.assertEqual(report['auxiliary_count'], 5422)
        self.assertEqual(sum(row['frames'] for row in report['framed_tables']), 328)
        self.assertEqual(report['framed_tables'][-1]['frame_ms'], 56)
        self.assertEqual(len(report['reward_icons']), 3)
        self.assertIsNone(guard.validate_release_content(b'not an NSP file', 'ui19'))
        self.assertIsNone(guard.validate_release_content(b'public fixtures', 'public-web'))

    def test_legacy_catalog_is_rejected_even_after_format_upgrade(self):
        data, _ = fixture(auxiliary=5409)
        for stage in ('ui20', 'UI20', 'ui20-preview', 'ui21', 'ui021_test'):
            with self.subTest(stage=stage), self.assertRaisesRegex(ValueError, 'UI20 content is missing'):
                guard.validate_release_content(data, stage)

    def test_header_layer_clock_and_reward_corruption_is_rejected(self):
        original, at = fixture()
        cases = (
            ('magic', at['tables'][0], 0, 'registry header'),
            ('frame clock', at['tables'][0] + 6, 1, 'frame clock'),
            ('empty layers', at['tables'][0] + 22, 0, 'layer range'),
            ('layer offset', at['tables'][0] + 20, 65535, 'layer range'),
            ('image identity', at['tables'][0] + (10 + 2 * 17) * 2, 58086, 'image identity'),
            ('Osty clock', at['tables'][9] + 6, 55, 'Osty dead_loop'),
        )
        for label, offset, value, error in cases:
            data = bytearray(original)
            struct.pack_into('<H', data, offset, value)
            with self.subTest(label=label), self.assertRaisesRegex(ValueError, error):
                guard.validate_release_content(data, 'ui20')
        data = bytearray(original)
        # A directory-sized copied icon must not masquerade as the gold set.
        data[40 + 58097 * 16:40 + 58098 * 16] = data[40 + 58096 * 16:40 + 58097 * 16]
        with self.assertRaisesRegex(ValueError, 'distinct artwork'):
            guard.validate_release_content(data, 'ui20')
        data = bytearray(original)
        struct.pack_into('<I', data, at['aux'] + 5418 * 4, at['table_ids'][0])
        with self.assertRaisesRegex(ValueError, 'distinct'):
            guard.validate_release_content(data, 'ui20')


if __name__ == '__main__':
    unittest.main()
