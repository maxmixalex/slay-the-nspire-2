"""Character progression resources replace only the reviewed locale/art tails."""
import contextlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import character_unlock_assets as art
import character_unlock_text as labels
import group_strings
import split_content as split
import timeline_text_ui24 as timeline
from test_ftue_assets import fixture


class CharacterUnlockResources(unittest.TestCase):
    def test_replaces_legacy_tail_and_copies_pixels_glyphs_and_prefix_for_both_profiles(self):
        for code, reduced in (('eng', False), ('zhs', True)):
            data = bytearray(group_strings.repack(fixture(code))[0]); struct.pack_into('<I', data, 4, 46)
            data[36:40] = b'NAF1' if reduced else bytes(4); data = bytes(data)
            original = labels.view(data)
            for legacy in (False, True):
                before = data
                if legacy:
                    with patch.object(timeline, 'BASE_STRINGS', 8192):
                        before, _ = timeline.append_records(data, ['RETIRED TIMELINE '+str(i) for i in range(257)], [])
                additions = [(0x10d000, 10, 2, 2, 0x8000 | 128, 0, b'\0\x40\x80\xff')]
                texts = ['Locked', 'Unknown Relic', 'Undiscovered \U0010d000']
                with patch.object(labels, 'BASE_STRINGS', 8192), patch.object(labels, 'LEGACY_STRINGS', 8449):
                    output, _ = labels.replace_records(before, texts, additions)
                    proof = labels.verify(before, output, texts, {(0x10d000, 10, 0)})
                target = labels.view(output)
                self.assertEqual(len(target.strings), 8195)
                self.assertEqual(proof['removed_timeline_strings'], 257 if legacy else 0)
                self.assertNotIn(b'RETIRED TIMELINE', output)
                self.assertEqual(output[36:40], data[36:40])
                old_tiles, new_tiles = labels.t.TileReader(original), labels.t.TileReader(target)
                for i, row in enumerate(original.images):
                    self.assertEqual(old_tiles.image_raw(i) if row[3] == 6 else original.image_raw(i),
                                     new_tiles.image_raw(i) if row[3] == 6 else target.image_raw(i))
                bounds = split.layout(output, code)
                common = split.common_file(output[bounds['prefix']:bounds['prefix']+bounds['artwork']])
                leaf = split.compress_language_directory(split.language_file(output,bounds,code,common[32:64]))
                self.assertEqual(split.reconstruct(common,leaf,code),output)

    def test_original_locale_is_required_and_combining_glyph_keys_do_not_collide(self):
        values = ['ล็อก', 'เครื่องรางปริศนา', 'ยังไม่เคยค้นพบเครื่องรางของตัวละครนี้']
        class Pack:
            def read(self, name):
                self.name = name
                return json.dumps(dict(zip(labels.KEYS, values)),ensure_ascii=False).encode()
        pack = Pack(); original, proof = labels.original_texts(pack,'tha')
        self.assertEqual(pack.name,'localization/tha/main_menu_ui.json')
        self.assertEqual(original,values); self.assertEqual(proof['english_fallback_slots'],[])
        shaped,inverse = labels.shape_texts(original,{0x10d000,0x10e000})
        self.assertTrue(inverse); self.assertNotIn(0x10d000,inverse)
        self.assertEqual([''.join(inverse.get(ord(c),c) for c in text) for text in shaped],original)
        wanted = labels.glyph_requirements(shaped,'tha')
        self.assertTrue(all((ord(c),18,0) in wanted for c in shaped[0]))
        with patch.object(pack,'read',return_value=b'{}'),self.assertRaisesRegex(ValueError,'missing'):
            labels.original_texts(pack,'tha')

    def test_original_eight_textures_preserve_aspect_and_unmodulated_alpha(self):
        scene = '\n'.join(('custom_minimum_size = Vector2(100, 148)',
            'modulate = Color(1, 1, 1, 0.4)','scale = Vector2(0.6, 0.6)',
            'offset_left = -20.0','offset_top = -30.0','offset_right = 50.0','offset_bottom = 74.0'))
        class Pack:
            def read(self,name):
                self.name=name; return scene.encode()
        source_sizes = {path:source_size for _,path,_,source_size in art.SPECS}
        def texture(_pack,path):return Image.new('RGBA',source_sizes[path],(64,128,240,255))
        with tempfile.TemporaryDirectory() as directory,patch.object(art,'load_scene_texture',side_effect=texture):
            pack=Pack();rows=art.source_assets(pack,Path(directory))
            self.assertEqual(pack.name,art.SCENE)
            self.assertEqual([row['auxiliary'] for row in rows],list(range(7229,7237)))
            for i,row in enumerate(rows):
                picture=Image.open(Path(directory)/(row['key']+'.png'))
                self.assertEqual(picture.size,(36,40) if i<6 else (30,40))
                self.assertEqual(picture.getbbox(),(4,0,31,40) if i<6 else (0,5,30,35))
                self.assertEqual(picture.getpixel((picture.width//2,20))[3],255)

    def test_new_stage_follows_all_quantization_and_works_in_all_converter_modes(self):
        from converter import LANGUAGES, Recipe
        from converter_finalize import add_final_stages
        for languages in (('eng','zhs'),LANGUAGES):
            for reduced in (False,True):
                recipe=Recipe('/game','/work','/runtime','dotnet','/noise','-',languages=languages)
                add_final_stages(recipe,recipe.source_stages(),Path('/output'),languages,reduced)
                stages={stage.name:stage for stage in recipe.stages};names=list(stages)
                self.assertEqual(names[-3:],['character-locks46','publish-locales','validate-install'])
                self.assertEqual(stages['character-locks46'].arguments[1],str(stages['final-index46'].outputs[0]))
                self.assertIn('--character-unlock-pck',stages['publish-locales'].arguments)
                self.assertNotIn('--timeline-pck',stages['publish-locales'].arguments)


if __name__=='__main__':unittest.main()
