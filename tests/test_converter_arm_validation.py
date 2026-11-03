from pathlib import Path
import json
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import converter_arm_validation as arm
from converter import Recipe
from converter_finalize import add_final_stages


class MandatoryArmContract(unittest.TestCase):
    def test_missing_helper_fails_closed(self):
        with tempfile.TemporaryDirectory() as root:
            with self.assertRaisesRegex(ValueError,'mandatory linked ARM'):
                arm.verify(Path(root))

    def test_source_and_binary_identity_are_both_required(self):
        with tempfile.TemporaryDirectory() as root:
            root=Path(root);raw=b'test-only identity fixture, not an executable ARM proof'
            (root/'nSpire2.elf').write_bytes(raw)
            value=dict(schema=1,unicorn_version='2.1.4',source_files={'src/content.cpp':'a'},elf_sha256=arm.sha256(raw))
            (root/'arm-validation.json').write_text(json.dumps(value))
            with patch.object(arm,'source_inventory',return_value={'src/content.cpp':'b'}):
                with self.assertRaisesRegex(ValueError,'frozen converter runtime source'):arm.verify(root)
            with patch.object(arm,'source_inventory',return_value=value['source_files']):
                (root/'nSpire2.elf').write_bytes(raw+b'bad')
                with self.assertRaisesRegex(ValueError,'ELF hash'):arm.verify(root)

    def test_both_profiles_validate_packed_art_before_appending_raw_lock_sprites(self):
        for reduced in (False,True):
            recipe=Recipe('/game','/work','/runtime','dotnet','/noise','-')
            recipe.reference={'version':'test-only-reference','pck_sha256':'pinned'}
            add_final_stages(recipe,recipe.source_stages(),Path('/output'),['eng','zhs'],reduced)
            rows={row.name:row for row in recipe.stages};names=[row.name for row in recipe.stages]
            tail='reduced46' if reduced else 'retuned46'
            self.assertGreater(names.index('final-index46'),names.index(tail))
            self.assertEqual(rows['final-index46'].module,'@mandatory-orphan-prune')
            self.assertEqual(rows['final-index46'].arguments[0],str(rows[tail].outputs[0]))
            locks=rows['character-locks46']
            self.assertEqual(locks.module,'character_unlock_assets')
            self.assertLess(names.index('final-index46'),names.index(locks.name))
            self.assertEqual(locks.arguments[locks.arguments.index('--input')+1],str(rows['final-index46'].outputs[0]))
            publish=rows['publish-locales']
            self.assertLess(names.index(locks.name),names.index(publish.name))
            self.assertEqual(publish.arguments[publish.arguments.index('--artwork')+1],str(locks.outputs[0]))
            self.assertIn('--exact-glyph-storage',publish.arguments)
            self.assertEqual(json.loads(rows['validate-install'].arguments[-1]),recipe.reference)

if __name__=='__main__':unittest.main()
