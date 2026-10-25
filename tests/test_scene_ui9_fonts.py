"""Exact appended scene bands and versioned count/byte bounds."""
from pathlib import Path
import json
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
import card_library_fonts as library
import content_tiles as tiles
import scene_ui9_fonts as scene
from test_card_library_fonts import original_with_flavor_keys
from test_card_library_fonts import Labels as LibraryLabels


class Labels:
    def __init__(self):
        index = scene.text_index()
        self.values = {slot:'字' for slot in index.values()}
        self.values.update(LibraryLabels().values)
        self.values[6110] = 'eng'
        self.cards=[]
        self.strings=[None]*8704
        for item in json.loads((scene.ROOT/'config/profile-ui-fonts.json').read_text())['labels']:
            self.values[index[item['key']]] = '[blue]字[/blue]'+''.join(
                '{'+name+'}' for name in item.get('substitutions', {}))
        # The overlapping ordinary Card Library labels contain no BBCode in
        # the pinned source. Keep that premise independent of rich profiles.
        for key in library.BOLD_LABEL_KEYS:
            self.values[index[key]] = '字'
        for key, slot in index.items():
            if key.startswith('intents.') and key.endswith(('.title', '.description')):
                self.values[slot] = '攻势' if key.endswith('.title') else (
                    '攻击[blue]{Damage}[/blue]{Repeat:choose(1):|{}次}{IsMultiplayer:所有人|}')

    def string_payload(self, slot):
        return self.values[slot].encode()+b'\0'


class SceneFontsTest(unittest.TestCase):
    def test_source_label_faces_and_template_alphabets(self):
        required = scene.requirements(Labels())
        for px, face in ((7,0),(6,5),(5,0),(5,5),(4,0)):
            self.assertIn((ord('字'),px,face),required)
        for c in '0123456789x×':
            self.assertIn((ord(c),4,5),required)
        for c in '攻击次所有人':
            self.assertIn((ord(c),5,0),required)
        for c in '{}[]|':
            # These are legal literal seed characters at5px Regular. They
            # must still never leak out of formatter/BBCode parsing elsewhere.
            self.assertIn((ord(c),5,0),required)
            self.assertFalse(any(cp==ord(c) and (px,face)!=(5,0) for cp,px,face in required))
        # Ordinary normal fonts are locale-substituted even when the scene
        # authored Spectral or Bitter; rich bold is collected separately.
        english_common=scene.room_requirements(Labels(),'common-ui-fonts.json')
        self.assertIn((ord('字'),18,4),english_common)
        self.assertIn((ord('字'),4,6),english_common)
        chinese=Labels();chinese.values[6110]='zhs'
        chinese_common=scene.room_requirements(chinese,'common-ui-fonts.json')
        self.assertIn((ord('字'),18,0),chinese_common)
        self.assertIn((ord('字'),4,0),chinese_common)
        self.assertFalse(any(face in (4,6) for cp,px,face in chinese_common))
        self.assertIn((ord('字'),5,5),chinese_common)  # Collection headings retain rich bold.
        english_profile=scene.profile_requirements(Labels())
        chinese_profile=scene.profile_requirements(chinese)
        for c in '0123456789/:—':
            self.assertIn((ord(c),6,5),english_profile)
            self.assertIn((ord(c),6,0),chinese_profile)
        for c in '[]{}|@':
            self.assertIn((ord(c),5,0),english_profile)
            self.assertIn((ord(c),5,0),chinese_profile)
        labels=Labels()
        labels.values[scene.text_index()['main_menu_ui.PROFILE_SCREEN.BUTTON.title']]='{Unknown}'
        with self.assertRaisesRegex(ValueError,'Unknown profile substitution'):
            scene.profile_requirements(labels)

    def test_extended_directory_preserves_every_old_glyph_and_payload(self):
        before, auxiliary=original_with_flavor_keys()
        source=tiles.PackView(before,auxiliary,versions=(34,))
        additions=[(0x10000+i,7,1,1,1,0,bytes((i&255,)))
                   for i in range(32769-len(source.glyphs))]
        with self.assertRaisesRegex(ValueError,'exceeds32768'):
            library.rewrite_directory(before,auxiliary,set(),additions)
        after=library.rewrite_directory(before,auxiliary,set(),additions,output_version=35)
        target=tiles.PackView(after,auxiliary,versions=(35,))
        self.assertEqual(len(target.glyphs),32769)
        self.assertEqual(target.version,35)
        proof=library.verify_preserved(before,after,auxiliary,set())
        self.assertTrue(proof['exact_complete_old_payload'])
        self.assertTrue(proof['exact_other_glyph_keys_metrics_alpha'])
        self.assertEqual(target.glyph_alpha(len(target.glyphs)-1),source.glyph_alpha(len(source.glyphs)-1))
        old_version=bytearray(after);struct.pack_into('<I',old_version,4,34)
        with self.assertRaisesRegex(ValueError,'directory counts'):
            tiles.PackView(old_version,auxiliary,versions=(34,))
        too_many=bytearray(after);struct.pack_into('<I',too_many,20,65536)
        with self.assertRaisesRegex(ValueError,'directory counts'):
            tiles.PackView(too_many,auxiliary,versions=(35,))
        too_large=bytearray(after)
        struct.pack_into('<II',too_large,16,65535,65535)
        with self.assertRaisesRegex(ValueError,'Directory outside limit'):
            tiles.PackView(too_large,auxiliary,versions=(35,))
        with self.assertRaisesRegex(ValueError,'transition'):
            library.rewrite_directory(after,auxiliary,set(),[],output_version=34)
        with self.assertRaisesRegex(ValueError,'transition'):
            library.rewrite_directory(after,auxiliary,set(),[],output_version=36)
        # A font-only edit of a later resource keeps its actual encoding
        # version and all old payloads. It cannot silently downgrade RGR2.
        for version in (36,37,38,39):
            later=bytearray(after);struct.pack_into('<I',later,4,version)
            extra=[(0x20000,7,1,1,1,0,b'\x7f')]
            appended=library.rewrite_directory(bytes(later),auxiliary,set(),extra,output_version=version)
            current=tiles.PackView(appended,auxiliary,versions=(version,))
            self.assertEqual(current.version,version)
            self.assertTrue(library.verify_preserved(bytes(later),appended,auxiliary,set())['exact_complete_old_payload'])
            with self.assertRaisesRegex(ValueError,'transition'):
                library.rewrite_directory(bytes(later),auxiliary,set(),[],output_version=35)


if __name__=='__main__':
    unittest.main()
