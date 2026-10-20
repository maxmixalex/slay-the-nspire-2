"""The parallel cache must be readable by the unchanged serial converter."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from build_world import ConversionCache
from prefetch_world_clips import PrefetchCache, unique_atomic_bytes


class PrefetchProtocol(unittest.TestCase):
    def test_serial_and_prefetch_protocols_match(self):
        with TemporaryDirectory() as root:
            cache = PrefetchCache(Path(root), {'schema': 2, 'input': 'fixed'})
            serial = ConversionCache(Path(root), {'schema': 2, 'input': 'fixed'})
            metadata, payload = {'frames': 3, 'size': [4, 5]}, b'original frame data'
            scope, settings = {'model': 'pinned', 'pipeline': 'enemy-rgba-1'}, {'clip': 7}
            with ThreadPoolExecutor(max_workers=4) as pool:
                list(pool.map(lambda _: cache.put('clip', settings, metadata, payload, scope=scope), range(20)))
            self.assertEqual(serial.get('clip', settings, scope=scope), (metadata, payload))
            self.assertFalse(list(Path(root).rglob('*.tmp')))
            (cache.directory / 'clip' / (cache.key('clip', settings, scope) + '.bin')).write_bytes(b'broken')
            with self.assertRaisesRegex(ValueError, 'incomplete or changed'):
                serial.get('clip', settings, scope=scope)

    def test_atomic_file_is_one_complete_producer(self):
        with TemporaryDirectory() as root:
            target = Path(root) / 'result.bin'
            values = [bytes([i]) * (10000 + i) for i in range(20)]
            with ThreadPoolExecutor(max_workers=4) as pool:
                list(pool.map(lambda value: unique_atomic_bytes(target, value), values))
            self.assertIn(target.read_bytes(), values)
            self.assertEqual(list(Path(root).iterdir()), [target])


if __name__ == '__main__':
    unittest.main()
