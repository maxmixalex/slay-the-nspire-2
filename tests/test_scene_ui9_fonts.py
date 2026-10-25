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


class Labels:
    def __init__(self):
        index = scene.text_index()
        self.values = {}
        for item in json.loads((scene.ROOT/'config/profile-ui-fonts.json').read_text())['labels']:
            self.values[index[item['key']]] = '[blue]字[/blue]'+''.join(
                '{'+name+'}' for name in item.get('substitutions', {}))
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
            self.assertFalse(any(cp==ord(c) for cp,px,face in required))
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


if __name__=='__main__':
    unittest.main()
