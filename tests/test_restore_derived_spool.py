import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from restore_derived_precision import Spool

class SpoolTests(unittest.TestCase):
    def test_verified_atomic_replacement(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'source.raw';path.write_bytes(b'prior complete artifact')
            spool=Spool(path);spool.add(12,(2,1,0,b'\x01\x00\x02\x00'))
            self.assertEqual(path.read_bytes(),b'prior complete artifact')
            receipt=spool.finish();self.assertEqual(path.read_bytes(),b'\x01\x00\x02\x00')
            self.assertEqual(receipt['bytes'],4);self.assertEqual(receipt['rows'][0]['image'],12)
            self.assertFalse(spool.temporary.exists())

    def test_truncated_spool_cannot_publish(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'source.raw';spool=Spool(path)
            spool.add(12,(2,1,0,b'\x01\x00\x02\x00'));spool.handle.flush()
            with spool.temporary.open('r+b') as output:output.truncate(2)
            with self.assertRaises(Exception):spool.finish()
            self.assertFalse(path.exists())

    def test_equal_size_corruption_cannot_publish(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/'source.raw';spool=Spool(path)
            spool.add(12,(2,1,0,b'\x01\x00\x02\x00'));spool.handle.flush()
            with spool.temporary.open('r+b') as output:output.write(b'\x03')
            with self.assertRaises(Exception):spool.finish()
            self.assertFalse(path.exists())

if __name__=='__main__':unittest.main()
