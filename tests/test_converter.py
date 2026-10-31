import argparse
import contextlib
import importlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import converter


class ConverterTests(unittest.TestCase):
    def test_final_profiles_select_approved_color_policy_and_keep_reduced_exact(self):
        from converter_finalize import add_final_stages
        for reduced in (False, True):
            recipe=converter.Recipe('/game','/tmp/converter-profile-test','/runtime','dotnet','/noise','-')
            add_final_stages(recipe,recipe.source_stages(),Path('/tmp/converter-profile-output'),
                             ['eng','zhs'],reduced)
            stages={stage.name:stage for stage in recipe.stages}
            quality=stages['quality46'].arguments
            if reduced:
                self.assertEqual(quality[4:7],('exact','true',''))
                self.assertEqual(json.loads(quality[7]),{})
                self.assertNotIn('sparse46',stages)
                self.assertEqual(stages['reduced46'].arguments[1],str(stages['quality46'].outputs[0]))
            else:
                self.assertEqual(quality[4:7],('bounded-a5','false','dark-safe-a6'))
                self.assertEqual(json.loads(quality[7]),{'0':'555a8','2':'555a8','3':'555a8'})
                self.assertEqual(stages['sparse46'].arguments[0],str(stages['quality46'].outputs[0]))
                self.assertEqual(stages['retuned46'].arguments[0],str(stages['sparse46'].outputs[0]))
                self.assertEqual(stages['publish-locales'].arguments[1],str(stages['retuned46'].outputs[0]))
            names=list(stages)
            self.assertLess(names.index('ancient-scenes45'),names.index('implicit46'))
            self.assertLess(names.index('implicit46'),names.index('quality46'))

    def test_exact_tail_preserves_pixels_metadata_and_version46(self):
        import coalesce_tile_resources as coalesce
        import content_tiles as tiles
        import converter_finalize as finalize
        from test_sparse_tile_filter import sparse_fixture
        import struct
        with tempfile.TemporaryDirectory() as folder, contextlib.redirect_stdout(io.StringIO()):
            root=Path(folder)
            data=bytearray(sparse_fixture());struct.pack_into('<I',data,4,46)
            source=root/'source.nsp';source.write_bytes(data)
            sparse=root/'sparse.nsp';retuned=root/'retuned.nsp'
            finalize.sparse_exact(source,sparse,root/'sparse-work',1)
            finalize.retune_exact(sparse,retuned,root/'retune-work',1)
            before,after=coalesce.view(bytes(data)),coalesce.view(retuned.read_bytes())
            self.assertEqual(after.version,46)
            self.assertEqual(before.clips,after.clips)
            self.assertEqual(before.aux,after.aux)
            self.assertLessEqual(retuned.stat().st_size,len(data))
            old,new=tiles.TileReader(before),tiles.TileReader(after)
            for image in range(len(before.images)):
                self.assertEqual(old.image_raw(image),new.image_raw(image))
            self.assertTrue(json.loads(retuned.with_suffix('.json').read_text())['literal_contexts_never_larger'])

    def test_stale_optional_locale_is_rejected_before_mutation(self):
        with tempfile.TemporaryDirectory() as folder:
            output=Path(folder);(output/'data').mkdir()
            old=output/'data/nSpire2.jpn.dat.tns';old.write_bytes(b'previous locale')
            with self.assertRaisesRegex(ValueError,'unrequested'):
                converter.preflight_output(output,['eng','zhs'])
            self.assertEqual(old.read_bytes(),b'previous locale')
            converter.preflight_output(output,['eng','zhs','jpn'])

    def test_find_game_directory_and_pck(self):
        with tempfile.TemporaryDirectory() as folder:
            game=Path(folder)
            for name in ('SlayTheSpire2.pck','release_info.json'):
                (game/name).write_text('test')
            self.assertEqual(converter.locate_game(game),game)
            self.assertEqual(converter.locate_game(game/'SlayTheSpire2.pck'),game)
            with self.assertRaises(ValueError):
                converter.locate_game(game/'missing')

    def test_resume_checks_outputs_and_failed_stage_never_completes(self):
        with tempfile.TemporaryDirectory() as folder:
            work=Path(folder);output=work/'result'
            stage=converter.Stage('test','fake',(),(output,))
            calls=[]
            def run(*args,**kwargs):
                calls.append(args);output.write_text('verified')
            with patch.object(converter.subprocess,'run',side_effect=run):
                converter.run_stages([stage],work,'identity')
                converter.run_stages([stage],work,'identity')
                self.assertEqual(len(calls),1)
                output.write_text('modified')
                converter.run_stages([stage],work,'identity')
                self.assertEqual(len(calls),2)
            with patch.object(converter.subprocess,'run',side_effect=subprocess.CalledProcessError(1,['fake'])):
                with self.assertRaises(subprocess.CalledProcessError):
                    converter.run_stages([stage],work,'new-recipe')
            self.assertEqual(json.loads((work/'receipts/test.json').read_text())['status'],'failed')

    def test_original_stage_arguments_match_real_parsers(self):
        """Exercise the actual legacy parsers before long source conversion."""
        recipe=converter.Recipe('/game','/tmp/converter-parser-test','/runtime','dotnet','/noise','-')
        from converter_finalize import add_final_stages
        add_final_stages(recipe,recipe.source_stages(),Path('/tmp/converter-parser-output'),['eng','zhs'],True)
        parse=argparse.ArgumentParser.parse_args
        class Parsed(Exception):
            pass
        for stage in recipe.stages:
            if stage.module.startswith('@'):
                continue
            def capture(parser,*args,**kwargs):
                parse(parser,*args,**kwargs)
                raise Parsed()
            with self.subTest(stage=stage.name):
                with patch.object(argparse.ArgumentParser,'parse_args',capture):
                    with self.assertRaises(Parsed):
                        converter.internal(stage.module,stage.arguments)


if __name__=='__main__':
    unittest.main()
