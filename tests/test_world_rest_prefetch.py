"""Asset-free rest prefetch protocol, validation and publication checks."""
from concurrent.futures import ThreadPoolExecutor
from copy import deepcopy
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
from threading import Barrier
import unittest
from unittest.mock import patch

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from build_world import ConversionCache, encode_image, encoded_frames
from prefetch_world_clips import PrefetchCache
import prefetch_world_rest as rest


class RestPrefetchProtocol(unittest.TestCase):
    def setUp(self):
        directory = TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        dependencies = {'schema': 2, 'input': 'synthetic'}
        self.sources = {'tools/synthetic.py': 'frozen'}
        self.cache = PrefetchCache(Path(directory.name), dependencies, source_files=self.sources)
        self.serial = ConversionCache(Path(directory.name), dependencies, source_files=self.sources)
        self.scope = {'pipeline': 'rest-actor-plane-rgba-1', 'files': dict(self.sources)}
        self.job = deepcopy(rest.REST_ACTOR_CLIP_ROWS[0])
        self.job.update(frames=2, size=[2, 2], frame_ms=3334)
        self.metadata = {key:deepcopy(self.job[key]) for key in rest.PROFILE_FIELDS}
        self.metadata.update(source_duration_seconds=self.job['duration'],
            actor_scene=self.job['path'], runtime_sha256=rest.RUNTIME_SHA256,
            diagnostics={'synthetic': True})
        self.images = [Image.new('RGBA', (2, 2), color)
                       for color in ((31, 87, 193, 255), (201, 73, 17, 128))]
        self.payload = encoded_frames([encode_image(image, animation=True) for image in self.images])
        old_worker = rest._worker
        self.addCleanup(setattr, rest, '_worker', old_worker)
        rest._worker = self.cache, object(), object(), self.sources, self.scope
        sampler = patch.object(rest, 'sample_rest_actor', return_value=(self.images, self.metadata))
        self.sample = sampler.start()
        self.addCleanup(sampler.stop)

    def run_job(self, job=None):
        return rest.prefetch_job((0, self.job if job is None else job))

    def record(self, job=None):
        return self.serial.get('clip', self.job if job is None else job, scope=self.scope)

    def test_serial_protocol_and_strict_reuse(self):
        self.assertEqual(self.run_job()[2:4], ('baked', len(self.payload)))
        self.assertEqual(self.record(), (self.metadata, self.payload))
        self.assertEqual(self.run_job()[2:4], ('reused', len(self.payload)))
        self.sample.assert_called_once()
        self.assertFalse(list(self.cache.directory.rglob('*.tmp')))

    def test_complete_job_and_scope_are_cache_identity(self):
        self.run_job()
        for key, value in (('dark', True), ('plane', 1), ('phase', .125),
                           ('resource', 'different.tres'), ('duration', 9.)):
            with self.subTest(field=key):
                changed = dict(self.job, **{key:value})
                self.assertIsNone(rest.cached_rest_clip(self.serial, changed, self.scope))
        changed_scope = deepcopy(self.scope)
        changed_scope['files']['tools/synthetic.py'] = 'changed'
        self.assertIsNone(rest.cached_rest_clip(self.serial, self.job, changed_scope))

    def test_corrupt_payload_cannot_be_reused(self):
        self.run_job()
        key = self.cache.key('clip', self.job, self.scope)
        (self.cache.directory/'clip'/(key+'.bin')).write_bytes(b'incomplete')
        with self.assertRaisesRegex(ValueError, 'incomplete or changed'):
            self.run_job()
        self.sample.assert_called_once()

    def test_profile_and_encoded_frame_validation(self):
        changes = dict(frames=3, duration_ms=1, frame_ms=1, size=[3, 2],
                       pivot=[1, 0], blend=1, plane_encoding='wrong', actor='OTHER')
        for field, value in changes.items():
            with self.subTest(field=field):
                metadata = dict(self.metadata, **{field:value})
                with self.assertRaisesRegex(ValueError, 'original profile'):
                    rest.validate_record(self.job, (metadata, self.payload))
        wrong_size = encoded_frames([encode_image(Image.new('RGBA', (1, 2)), animation=True)]*2)
        wrong_count = encoded_frames([encode_image(self.images[0], animation=True)])
        for payload, error in ((wrong_size, 'dimensions'), (wrong_count, 'count mismatch'),
                               (self.payload+b'x', 'Unexpected frame cache suffix')):
            with self.subTest(error=error), self.assertRaisesRegex(ValueError, error):
                rest.validate_record(self.job, (self.metadata, payload))

    def test_invalid_render_is_not_published(self):
        self.sample.return_value = self.images, dict(self.metadata, frames=3)
        with self.assertRaisesRegex(ValueError, 'original profile'):
            self.run_job()
        self.assertIsNone(self.record())

    def test_prefetch_workers_share_one_completed_record(self):
        barrier = Barrier(2)
        def run(_index):
            barrier.wait(timeout=5)
            return self.run_job()[2]
        with ThreadPoolExecutor(max_workers=2) as pool:
            self.assertCountEqual(pool.map(run, range(2)), ['baked', 'reused'])
        self.sample.assert_called_once()
        self.assertEqual(self.record(), (self.metadata, self.payload))
        self.assertFalse(list(self.cache.directory.rglob('*.tmp')))

    def test_unused_payload_signature_does_not_block_cache_reuse(self):
        self.run_job()
        key = self.cache.key('clip', self.job, self.scope)
        path = self.cache.directory/'clip'/(key+'.json')
        row = json.loads(path.read_bytes())
        row['sha256'] = 'unused historical integrity field'
        path.write_bytes(rest.canonical(row))
        self.assertEqual(self.run_job()[2], 'reused')
        self.sample.assert_called_once()

    def test_runtime_dependency_selects_distinct_pixel_cache(self):
        self.run_job()
        different = dict(self.cache.dependencies, spine_runtime='another sampler implementation')
        other = ConversionCache(self.cache.directory.parent, different, source_files=self.sources)
        self.assertIsNone(other.get('clip', self.job, scope=self.scope))



if __name__ == '__main__':
    unittest.main()
