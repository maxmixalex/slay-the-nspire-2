"""Modern exact stream retuning covers large groups, split files and bad caches."""
from pathlib import Path
import lzma
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import coalesce_tile_resources as coalesce
import content_tiles as t
import merge_tile_groups as merge
import retune_resource_streams as tune
import split_content as split
import storage_parts as storage
from tighten_ui14_resources import tighten
from test_coalesce_tile_resources import fixture
from test_repack_retained import stored


class RetuneResourceStreamsTest(unittest.TestCase):
    def modern_fixture(self, folder):
        before = fixture(strings=6111)
        plan, _, _ = merge.make_plan(coalesce.view(before), folder/'merge', workers=1)
        result, _, _ = merge.rewrite(before, plan)
        return result

    def test_font_cohort_retunes_without_changing_glyph_or_artwork_phase(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);before=bytearray(self.modern_fixture(root))
            source=coalesce.view(before)
            raw=bytes(range(256))*4;encoded=stored(raw)
            before.extend(bytes(len(before)&1));group_at=len(before)
            group=t.GROUP.pack(b'RGR1',len(raw),len(encoded),14,1,0,0,0)+encoded
            before.extend(group);before.extend(bytes(len(before)&1));descriptor=len(before)
            before.extend(struct.pack('<iIII',group_at-descriptor,len(group),0,len(raw)))
            t.GLYPH.pack_into(before,source.glyph_at,65,descriptor,8,32,32,1,t.GLYPH_GROUP_FLAG)
            struct.pack_into('<I',before,12,len(before));before=bytes(before)
            source=coalesce.view(before)
            after,proof=tighten(before,root/'cache',workers=1,profiles=tune.PROFILES,
                                preserve_suffix_phase=True)
            target=coalesce.view(after)
            self.assertEqual(proof['glyph_groups_considered'],1)
            self.assertGreater(proof['saved_bytes'],500)
            self.assertEqual(source.glyph_payload(0),target.glyph_payload(0))
            self.assertEqual(source.glyphs[0][:1]+source.glyphs[0][2:],
                             target.glyphs[0][:1]+target.glyphs[0][2:])
            new_descriptor=target.glyphs[0][1]
            new_group=new_descriptor+struct.unpack_from('<i',after,new_descriptor)[0]
            self.assertEqual((group_at-new_group)%4096,0)
            self.assertEqual(t.TileReader(source).image_raw(0),t.TileReader(target).image_raw(0))

    def test_complete_modern_split_reconstruction_and_exact_cached_reuse(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            before = self.modern_fixture(root)
            source = coalesce.view(before)
            after, proof = tighten(before, root/'cache', workers=1, artwork_only=True,
                                   profiles=tune.PROFILES, preserve_suffix_phase=True)
            target = coalesce.view(after)
            self.assertEqual(target.version, 44)
            self.assertEqual(len(before)-len(after), proof['saved_bytes'])
            self.assertEqual(proof['saved_bytes'] % 4096, 0)
            self.assertEqual(source.clips, target.clips)
            self.assertEqual(source.cards, target.cards)
            self.assertEqual(source.aux, target.aux)
            source_reader, target_reader = t.TileReader(source), t.TileReader(target)
            for image in range(len(source.images)):
                self.assertEqual(source_reader.image_raw(image), target_reader.image_raw(image))
            bounds = split.layout(after, 'eng')
            common = split.common_file(after[bounds['prefix']:bounds['prefix']+bounds['artwork']])
            index, parts, _ = storage.partition(common, after)
            self.assertEqual(storage.join_parts(index, parts), common)
            language = split.compress_language_directory(split.language_file(after, bounds, 'eng', bytes(32)))
            language, _ = storage.compress_blocks(language)
            self.assertEqual(split.reconstruct(common, language, 'eng'), after)
            cached, repeated = tighten(before, root/'cache', workers=1, artwork_only=True,
                                       profiles=tune.PROFILES, preserve_suffix_phase=True)
            self.assertEqual(cached, after)
            self.assertEqual(repeated, proof)

    def test_cached_bytes_must_decode_to_the_exact_original(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            before = self.modern_fixture(root)
            tighten(before, root/'cache', workers=1, profiles=tune.PROFILES)
            cached = next((root/'cache').glob('*.bin'))
            value = bytearray(cached.read_bytes())
            value[-1] ^= 0x40
            cached.write_bytes(value)
            with self.assertRaises((ValueError, lzma.LZMAError)):
                tighten(before, root/'cache', workers=1, profiles=tune.PROFILES)

    def test_literal_context_exceeding_existing_decoder_workspace_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            before = self.modern_fixture(root)
            with self.assertRaisesRegex(ValueError, 'bounded LZMA2 profile'):
                tighten(before, root/'cache', workers=1,
                        profiles={'tile':((4,1,0),), 'index':((4,1,0),), 'retained':((4,1,0),)})

    def test_bounded_retuning_keeps_reduced_profile_and_actual_pixels(self):
        from retune_color_contexts_ui23 import convert
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            before=bytearray(self.modern_fixture(root))
            struct.pack_into('<I',before,4,46)
            before[36:40]=b'NAF1'
            before=bytes(before)
            after,proof=convert(before,root/'color-cache',workers=1)
            self.assertEqual(after[36:40],b'NAF1')
            self.assertTrue(proof['literal_contexts_not_increased'])
            source=coalesce.view(before);target=coalesce.view(after)
            a,b=t.TileReader(source),t.TileReader(target)
            self.assertEqual(a.dictionary,b.dictionary)
            for left,right in zip(a.groups,b.groups):
                old=coalesce.source_lzma_profile(before[left[0]:left[0]+left[1]])
                new=coalesce.source_lzma_profile(after[right[0]:right[0]+right[1]])
                self.assertLessEqual(sum(new[:2]),sum(old[:2]))
            for image in range(len(source.images)):
                self.assertEqual(a.image_raw(image),b.image_raw(image))
            self.assertEqual(source.clips,target.clips)


if __name__ == '__main__':
    unittest.main()
