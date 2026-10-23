"""Asset-free rest-only raster binding and exact reference-cache migration."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import animation
import build_world as world
import prefetch_world_rest as prefetch
import world_enemy_assets
import world_raster


class RestBatchBinding(unittest.TestCase):
    def test_only_rest_call_uses_batch_and_restores_reference(self):
        old = animation.raster
        enemy = world_enemy_assets.raster
        args = object(), object(), object()
        result = object()

        def reference(*received):
            self.assertEqual(received, args)
            self.assertIs(animation.raster, world_raster.raster)
            self.assertIs(world_enemy_assets.raster, enemy)
            return result

        with patch.object(world, 'sample_rest_actor', side_effect=reference):
            self.assertIs(world.sample_rest_actor_batched(*args), result)
        self.assertIs(animation.raster, old)
        self.assertIs(world_enemy_assets.raster, enemy)
        self.assertIs(prefetch.sample_rest_actor, world.sample_rest_actor_batched)

    def test_exception_restores_actual_previous_binding(self):
        sentinel = object()

        def fail(*_args):
            self.assertIs(animation.raster, world_raster.raster)
            raise RuntimeError('failed private rest sample')

        with patch.object(animation, 'raster', sentinel):
            with patch.object(world, 'sample_rest_actor', side_effect=fail):
                with self.assertRaisesRegex(RuntimeError, 'failed private rest sample'):
                    world.sample_rest_actor_batched(None, None, None)
            self.assertIs(animation.raster, sentinel)


class RestBatchCache(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.frozen_sources = world.source_fingerprint()

    def setUp(self):
        directory = TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.sources = dict(self.frozen_sources)
        self.cache = world.ConversionCache(Path(directory.name),
            {'schema': 2, 'input': 'rest-batch-synthetic'}, source_files=self.sources)
        self.scope = world.render_scope('rest-actor-clip', self.sources)
        self.reference = world.render_scope('rest-actor-clip-reference', self.sources)
        self.job = deepcopy(world.REST_ACTOR_CLIP_ROWS[0])
        self.job.update(frames=2, size=[2, 2], frame_ms=3334)
        self.metadata = {key:deepcopy(self.job[key]) for key in world.REST_ACTOR_PROFILE_FIELDS}
        self.metadata.update(source_duration_seconds=self.job['duration'],
            actor_scene=self.job['path'], runtime_sha256=world.RUNTIME_SHA256,
            diagnostics={'synthetic': True})
        self.images = [Image.new('RGBA', (2, 2), color)
            for color in ((31, 87, 193, 255), (201, 73, 17, 128))]
        self.payload = world.encoded_frames([
            world.encode_image(image, animation=True) for image in self.images])

    def read(self, scope=None, reference=None):
        return world.cached_rest_actor_clip(self.cache, self.job,
            self.scope if scope is None else scope,
            reference_scope=self.reference if reference is None else reference)

    def seed_reference(self, metadata=None, payload=None):
        return self.cache.put('clip', self.job,
            self.metadata if metadata is None else metadata,
            self.payload if payload is None else payload, scope=self.reference)

    def receipt(self, scope):
        key = self.cache.key('clip', self.job, scope)
        return self.cache.directory / 'clip' / (key + '.json')

    def test_reference_keeps_exact_prebatch_scope_contract(self):
        paths = world.GEOMETRY_SOURCES + world.ENCODER_SOURCES + (
            'tools/world_scene_assets.py', 'tools/world_room_assets.py',
            'tools/world_room_profile.py', 'config/world-room-ui-0.17.json',
            'tools/world_rest_actors.py', 'tools/world_rest_actor_assets.py',
            'tools/world_room_shaders.py')
        expected = {'pipeline': 'rest-actor-plane-rgba-1',
            'files': {name:self.sources[name] for name in paths},
            'functions': {function.__name__:world.function_hash(function)
                          for function in (world.sample_rest_actor, world.encode_image)}}
        self.assertEqual(self.reference, expected)
        self.assertEqual(world.sample_rest_actor.__name__, 'sample_rest_actor')
        self.assertEqual(self.scope['pipeline'], 'rest-actor-plane-batched-rgba-1')
        self.assertEqual(set(self.scope['files']) - set(self.reference['files']),
                         {'tools/world_raster.py'})
        self.assertEqual(set(self.scope['functions']) - set(self.reference['functions']),
                         {'sample_rest_actor_batched'})
        for category in ('files', 'functions'):
            for name, value in self.reference[category].items():
                self.assertEqual(self.scope[category][name], value)

    def test_exact_reference_migrates_without_rewriting_old_record(self):
        self.seed_reference()
        before = self.receipt(self.reference).read_bytes()
        self.assertEqual(self.read(), (self.metadata, self.payload))
        self.assertEqual(self.receipt(self.reference).read_bytes(), before)
        receipt = json.loads(self.receipt(self.scope).read_bytes())
        self.assertEqual(receipt['reused_from'], {
            'pipeline': self.reference['pipeline'],
            'key': self.cache.key('clip', self.job, self.reference)})
        self.assertEqual(self.read(), (self.metadata, self.payload))

    def test_prefetch_uses_same_reference_migration(self):
        self.seed_reference()
        record = prefetch.cached_rest_clip(self.cache, self.job, self.scope)
        self.assertEqual(record, self.read())

    def test_reference_can_arrive_from_running_old_producer_later(self):
        self.assertIsNone(self.read())
        self.seed_reference()
        self.assertEqual(self.read(), (self.metadata, self.payload))

    def test_changed_batch_file_cannot_reuse_another_batch_version(self):
        self.cache.put('clip', self.job, self.metadata, self.payload, scope=self.scope)
        self.sources['tools/world_raster.py'] = 'different-batch-code'
        changed = world.render_scope('rest-actor-clip', self.sources)
        reference = world.render_scope('rest-actor-clip-reference', self.sources)
        self.assertEqual(reference, self.reference)
        self.assertNotEqual(changed, self.scope)
        self.assertIsNone(self.read(changed, reference))
        self.assertFalse(self.receipt(changed).exists())

    def test_changed_batch_wrapper_cannot_reuse_another_batch_version(self):
        self.cache.put('clip', self.job, self.metadata, self.payload, scope=self.scope)
        original = world.function_hash
        with patch.object(world, 'function_hash', side_effect=lambda fn:
                'changed-wrapper' if fn is world.sample_rest_actor_batched else original(fn)):
            changed = world.render_scope('rest-actor-clip', self.sources)
            self.assertIsNone(self.read(changed))

    def test_changed_reference_renderer_cannot_import_old_reference(self):
        self.seed_reference()
        self.sources['tools/world_room_shaders.py'] = 'different-authored-shader-sampling'
        changed = world.render_scope('rest-actor-clip', self.sources)
        reference = world.render_scope('rest-actor-clip-reference', self.sources)
        self.assertNotEqual(reference, self.reference)
        self.assertIsNone(self.read(changed, reference))

    def test_migration_does_not_accept_batched_scope_as_reference(self):
        self.seed_reference()
        with self.assertRaisesRegex(ValueError, 'exact renderer dependency scopes'):
            self.read(reference=self.scope)
        self.assertFalse(self.receipt(self.scope).exists())

    def test_complete_job_is_required_even_when_pixels_look_compatible(self):
        self.seed_reference()
        for field, value in (('dark', not self.job['dark']), ('phase', .125),
                             ('duration', 9.), ('plane', 1), ('key', 'other')):
            with self.subTest(field=field):
                changed = dict(self.job, **{field:value})
                self.assertIsNone(world.cached_rest_actor_clip(self.cache, changed,
                    self.scope, reference_scope=self.reference))

    def test_all_profile_and_original_identity_fields_are_validated(self):
        fields = (*world.REST_ACTOR_PROFILE_FIELDS, 'source_duration_seconds',
                  'actor_scene')
        for field in fields:
            with self.subTest(field=field):
                self.seed_reference(metadata=dict(self.metadata, **{field:'invalid'}))
                with self.assertRaisesRegex(ValueError, 'original profile'):
                    self.read()
                self.assertFalse(self.receipt(self.scope).exists())

    def test_complete_payload_count_dimensions_and_suffix_are_validated(self):
        alternatives = [
            (world.encoded_frames([world.encode_image(self.images[0], animation=True)]), 'count mismatch'),
            (world.encoded_frames([world.encode_image(Image.new('RGBA', (1, 2)), animation=True)]*2), 'dimensions'),
            (self.payload+b'x', 'Unexpected frame cache suffix')]
        for payload, message in alternatives:
            with self.subTest(message=message):
                self.seed_reference(payload=payload)
                with self.assertRaisesRegex(ValueError, message):
                    self.read()
                self.assertFalse(self.receipt(self.scope).exists())

    def test_truncated_reference_payload_fails_and_unused_digest_is_ignored(self):
        self.seed_reference()
        self.receipt(self.reference).with_suffix('.bin').write_bytes(b'broken')
        with self.assertRaisesRegex(ValueError, 'incomplete or changed'):
            self.read()
        self.seed_reference()
        path = self.receipt(self.reference)
        row = json.loads(path.read_bytes())
        row['sha256'] = 'unused historical integrity field'
        path.write_bytes(world.canonical(row))
        self.assertEqual(self.read(), (self.metadata, self.payload))

    def test_bad_current_record_cannot_fall_back_over_its_failure(self):
        self.seed_reference()
        self.cache.put('clip', self.job, dict(self.metadata, frames=3),
                       self.payload, scope=self.scope)
        before = self.receipt(self.scope).read_bytes()
        with self.assertRaisesRegex(ValueError, 'original profile'):
            self.read()
        self.assertEqual(self.receipt(self.scope).read_bytes(), before)



if __name__ == '__main__':
    unittest.main()
