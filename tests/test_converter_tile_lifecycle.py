"""Owned immutable tile outputs survive verified rebuilds and interrupted rotation."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import converter


def fixture(root,status='complete'):
    source=root/'world.nsp';source.write_bytes(b'unchanged original-derived input')
    output=root/'tiles/tiles.nsp';output.parent.mkdir()
    data=struct.pack('<4s9I',b'NSP2',15,0,43,0,0,0,0,0,0)+b'old'
    output.write_bytes(data)
    receipt=output.with_suffix('.json')
    report=dict(schema=1,complete=True,format='NSP2/15 sparse tiles',input_version=14,
        output_version=15,input_sha256=converter.digest(source),output_sha256=converter.digest(output),
        output_bytes=len(data))
    receipt.write_text(json.dumps(report))
    runner=root/'receipts/tiles.json';runner.parent.mkdir()
    # This is the actual historical one-output completion contract.
    runner.write_text(json.dumps(dict(stage='tiles',key='old',status=status,
        outputs={str(output):report['output_sha256']})))
    log=root/'logs/tiles.log';log.parent.mkdir();log.write_text('prior exact tile conversion evidence\n')
    stage=converter.Stage('tiles','content_tiles',('--input',str(source),'--output',str(output),
        '--receipt',str(receipt)),(output,receipt))
    return stage,source,output,receipt,runner,log,data


class TileLifecycleTests(unittest.TestCase):
    def test_legacy_and_failed_runner_records_preserve_the_verified_pair(self):
        for status in ('complete','failed'):
            with self.subTest(status=status), tempfile.TemporaryDirectory() as folder, \
                 contextlib.redirect_stdout(io.StringIO()):
                root=Path(folder);stage,source,out,receipt,runner,log,data=fixture(root,status)
                old_receipt,old_runner,old_log=receipt.read_bytes(),runner.read_bytes(),log.read_bytes()
                sibling=out.parent/'unrelated.txt';sibling.write_text('keep')
                def convert(*args,**kwargs):
                    # The real tile writer retains both no-overwrite guards.
                    self.assertFalse(out.exists());self.assertFalse(receipt.exists())
                    out.write_bytes(data);receipt.write_bytes(old_receipt)
                with patch.object(converter.subprocess,'run',side_effect=convert):
                    self.assertTrue(converter.run_stages([stage],root,'new-source'))
                history=root/'receipts/superseded/tiles'/hashlib.sha256(old_receipt).hexdigest()
                self.assertEqual((history/out.name).read_bytes(),data)
                self.assertEqual((history/receipt.name).read_bytes(),old_receipt)
                self.assertEqual(next(history.glob('runner-*.json')).read_bytes(),old_runner)
                self.assertEqual(next(history.glob('log-*.log')).read_bytes(),old_log)
                self.assertEqual(source.read_bytes(),b'unchanged original-derived input')
                self.assertEqual(sibling.read_text(),'keep')
                self.assertEqual(set(json.loads(runner.read_text())['outputs']),{str(out),str(receipt)})

    def test_interrupted_rotation_requires_exact_preserved_ownership(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);stage,source,out,receipt,runner,log,data=fixture(root)
            old_receipt=receipt.read_bytes()
            converter.prepare_tile_generation(stage,root,runner,log)
            # Interruption after removing the old output but before its receipt.
            receipt.write_bytes(old_receipt)
            converter.prepare_tile_generation(stage,root,runner,log)
            self.assertFalse(out.exists());self.assertFalse(receipt.exists())
            receipt.write_bytes(old_receipt)
            history=root/'receipts/superseded/tiles'/hashlib.sha256(old_receipt).hexdigest()
            (history/'ownership.json').unlink()
            with self.assertRaisesRegex(ValueError,'archived ownership'):
                converter.prepare_tile_generation(stage,root,runner,log)
            self.assertEqual(receipt.read_bytes(),old_receipt)

    def test_corrupt_pair_and_missing_receipt_preserve_existing_files(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);stage,source,out,receipt,runner,log,data=fixture(root)
            out.write_bytes(data+b'corrupt')
            with self.assertRaisesRegex(ValueError,'ownership receipt'):
                converter.prepare_tile_generation(stage,root,runner,log)
            self.assertEqual(out.read_bytes(),data+b'corrupt')
            receipt.unlink()
            with self.assertRaisesRegex(ValueError,'no ownership receipt'):
                converter.prepare_tile_generation(stage,root,runner,log)
            self.assertEqual(out.read_bytes(),data+b'corrupt')

    def test_archive_corruption_and_input_alias_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);stage,source,out,receipt,runner,log,data=fixture(root)
            raw=receipt.read_bytes();history=root/'receipts/superseded/tiles'/hashlib.sha256(raw).hexdigest()
            history.mkdir(parents=True);(history/out.name).write_bytes(b'bad history')
            with self.assertRaisesRegex(ValueError,'archived tile generation'):
                converter.prepare_tile_generation(stage,root,runner,log)
            self.assertEqual(out.read_bytes(),data);self.assertEqual(receipt.read_bytes(),raw)
            aliased=converter.Stage(stage.name,stage.module,
                ('--input',str(out),'--output',str(out),'--receipt',str(receipt)),stage.outputs)
            with self.assertRaisesRegex(ValueError,'preserve its input'):
                converter.prepare_tile_generation(aliased,root,runner,log)
            self.assertEqual(out.read_bytes(),data)


if __name__=='__main__':unittest.main()
