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
        recipe.source_stages()
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
