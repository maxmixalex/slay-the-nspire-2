"""Asset-free bounds, resumable-cache and original clip-coverage regressions."""
import copy
import hashlib
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from build_world import (ConversionCache, card_description, choose_layout,
    decoded_frames, encoded_frames, plan_summary, private_path, ROOT, FONT_SIZES,
    canonical, LEGACY_BUILDER_SHA256, LEGACY_FUNCTIONS, sample_map_clip,
    cached_enemy_clip, render_scope, source_fingerprint, sample_enemy_batched)
from world_enemy_assets import manifest
from world_enemy_profile import plan, skins
from world_ui_profile import (BASE_AUXILIARY, BASE_CLIPS, ENEMY_CLIP_ROWS, MAP_CLIP_ROWS,
    ROOM_CLIP_ROWS, REST_ACTOR_CLIP_ROWS, CLIP_ROWS, AUXILIARY_ROWS, TEXT_KEYS, TEXT_BASE, validate_headers)


class WorldPack(unittest.TestCase):
    def test_reticle_appends_after_all_existing_map_common_and_room_images(self):
        # Pinned public key/index pairs from verified source12, before this
        # addition. An insertion into the earlier map catalog must fail even
        # when all regenerated tables happen to agree with that insertion.
        prior = [(row['key'], BASE_AUXILIARY + i)
                 for i, row in enumerate(AUXILIARY_ROWS[:831])]
        digest = hashlib.sha256(json.dumps(prior, separators=(',', ':')).encode()).hexdigest()
        self.assertEqual(digest, '7a2c7313947e81306646a10d4584faf3a3ca3a57f90de6e7a04731d107f9c546')
        added = AUXILIARY_ROWS[831]
        self.assertEqual(added['key'], 'map.SELECTION_RETICLE')
        self.assertEqual(BASE_AUXILIARY + 831, 1170)
        self.assertEqual(added['path'], 'images/ui/combat/combat_reticle.png')
        self.assertEqual(added['source_size'], [48, 48])
        self.assertEqual(added['size'], [48, 48])
        self.assertNotIn('modulate', added)
        self.assertNotIn('nine_patch', added)
        self.assertEqual(added['fit'], 'stretch')
        self.assertEqual(plan_summary()['expected_images'], 45724)
        self.assertEqual(plan_summary()['clips'], 1695)

    def test_map_circle_entry_appends_without_moving_terminal_frame_or_reticle(self):
        rows = {row['key']:(BASE_AUXILIARY + i, row) for i, row in enumerate(AUXILIARY_ROWS)}
        self.assertEqual(rows['map.CIRCLE'][0], 1134)
        self.assertEqual(rows['map.SELECTION_RETICLE'][0], 1170)
        self.assertEqual(rows['map.CIRCLE'][1]['path'],
                         'images/atlases/compressed.sprites/map/map_circle_4.tres')
        for frame in range(4):
            index, row = rows[f'map.CIRCLE_{frame}']
            self.assertEqual(index, 1171 + frame)
            self.assertEqual(row, dict(key=f'map.CIRCLE_{frame}', kind='atlas',
                path=f'images/atlases/compressed.sprites/map/map_circle_{frame}.tres',
                size=[48,48], fit='contain'))
        self.assertEqual(BASE_AUXILIARY + len(AUXILIARY_ROWS), 1175)

    def test_room_shader_and_actor_cache_closures_do_not_discard_enemy_frames(self):
        source = source_fingerprint()
        model = manifest()['models'][0]
        for path in ('tools/world_room_shaders.py', 'tools/world_rest_actor_assets.py', 'tools/world_rest_actors.py'):
            changed = dict(source, **{path: 'a changed room renderer'})
            for kind in ('bounds', 'clip-reference', 'clip', 'map-clip', 'texture', 'room-clip'):
                self.assertEqual(render_scope(kind, source, model=model),
                                 render_scope(kind, changed, model=model))
            self.assertNotEqual(render_scope('rest-actor-clip', source),
                                render_scope('rest-actor-clip', changed))
            if path.endswith('world_room_shaders.py'):
                self.assertNotEqual(render_scope('room-image', source),
                                    render_scope('room-image', changed))
            else:
                self.assertEqual(render_scope('room-image', source),
                                 render_scope('room-image', changed))

    def test_rest_actor_planes_append_after_every_prior_animation(self):
        before = ENEMY_CLIP_ROWS + MAP_CLIP_ROWS + ROOM_CLIP_ROWS
        self.assertEqual(CLIP_ROWS[:len(before)], before)
        start = BASE_CLIPS + len(before)
        self.assertEqual(start, 1647)
        self.assertEqual([row['id'] for row in REST_ACTOR_CLIP_ROWS],
                         list(range(start, start + len(REST_ACTOR_CLIP_ROWS))))
        summary = plan_summary()
        self.assertEqual(summary['rest_actor_frames'], sum(row['frames'] for row in REST_ACTOR_CLIP_ROWS))
        self.assertLessEqual(summary['expected_images'], 65535)
        self.assertLessEqual(summary['maximum_directory_bytes'], 2 * 1024 * 1024)

    def test_batched_sampling_restores_reference_binding_after_failure(self):
        import world_enemy_assets
        import world_raster
        previous = world_enemy_assets.raster
        def stopped(*args, **kwargs):
            self.assertIs(world_enemy_assets.raster, world_raster.raster)
            raise ValueError('sampling failure')
        with patch('build_world.sample_enemy', side_effect=stopped):
            with self.assertRaisesRegex(ValueError, 'sampling failure'):
                sample_enemy_batched(None, {}, {}, None)
        self.assertIs(world_enemy_assets.raster, previous)

    def test_batched_scope_does_not_remeasure_geometry_or_reuse_changed_batch_code(self):
        source = source_fingerprint()
        model = manifest()['models'][0]
        changed = dict(source, **{'tools/world_raster.py': 'a new raster implementation'})
        for kind in ('bounds', 'clip-reference', 'scene'):
            self.assertEqual(render_scope(kind, source, model=model),
                             render_scope(kind, changed, model=model))
        for kind in ('clip', 'map-clip'):
            self.assertNotEqual(render_scope(kind, source, model=model),
                                render_scope(kind, changed, model=model))

    def test_reference_pixel_reuse_checks_all_inputs_and_payload(self):
        with TemporaryDirectory(prefix='nspire-cache-test-') as directory:
            source = source_fingerprint()
            model = manifest()['models'][0]
            settings, metadata, payload = {'job': 'fixed'}, {'frames': 1}, b'reference pixels'
            cache = ConversionCache(Path(directory), {'schema': 2, 'pck': 'fixed'})
            original = render_scope('clip-reference', source, model=model)
            cache.put('clip', settings, metadata, payload, scope=original)
            self.assertIsNone(cached_enemy_clip(cache, {'job': 'different'}, source, model))
            changed = dict(source, **{'tools/animation.py': 'different original geometry'})
            self.assertIsNone(cached_enemy_clip(cache, settings, changed, model))
            self.assertEqual(cached_enemy_clip(cache, settings, source, model), (metadata, payload))
            scope = render_scope('clip', source, model=model)
            receipt = cache.directory / 'clip' / (cache.key('clip', settings, scope) + '.json')
            self.assertEqual(json.loads(receipt.read_bytes())['reused_from']['key'],
                             cache.key('clip', settings, original))
            receipt.with_suffix('.bin').write_bytes(b'corrupted batched record')
            with self.assertRaisesRegex(ValueError, 'incomplete or changed'):
                cached_enemy_clip(cache, settings, source, model)

    def test_common_text_append_preserves_all_original_world_ids(self):
        base = json.loads((ROOT / 'config/world-ui-0.17.json').read_text())['text_keys']
        self.assertEqual(TEXT_KEYS[:len(base)], base)
        self.assertEqual(TEXT_BASE + TEXT_KEYS.index('main_menu_ui.COMPENDIUM'), 6080)
        self.assertGreater(TEXT_KEYS.index('settings_ui.FASTMODE'), len(base)-1)
        self.assertEqual(TEXT_BASE + TEXT_KEYS.index('main_menu_ui.ABANDON_RUN'), 6095)
        self.assertEqual(len(TEXT_KEYS), len(set(TEXT_KEYS)))

    def test_every_used_clip_has_full_period_and_own_native_name(self):
        jobs = plan()['jobs']
        by_clip = {}
        for job in jobs:
            by_clip.setdefault((job['model'], job['clip']), []).append(job)
        used = 0
        for model in manifest()['models']:
            for clip in model['clips']:
                if not clip['used']:
                    continue
                used += 1
                variants = by_clip[(model['model'], clip['name'])]
                normal = [row for row in variants if row['move'] == -1]
                self.assertTrue(normal, (model['model'], clip['name']))
                self.assertTrue(all(row['duration'] + 1e-6 >= clip['duration'] for row in normal))
                self.assertTrue(all(1 <= row['frames'] <= 256 for row in variants))
        self.assertEqual(used, 732)
        self.assertEqual(used, plan()['used_clips'])
        self.assertEqual([row['id'] for row in jobs], list(range(BASE_CLIPS, BASE_CLIPS + len(jobs))))
        self.assertEqual(len({row['key'] for row in jobs}), len(jobs))

    def test_original_skin_multiplicity_and_named_slot_choices_survive(self):
        models = {row['model']: row for row in plan()['models']}
        self.assertEqual(len(skins(models['CubexConstruct'])), 9)
        rat = skins(models['TwoTailedRat'])
        self.assertEqual(len(rat), 3)
        self.assertEqual([row['weight'] for row in rat], [3, 3, 3])
        gardener = skins(models['PhantasmalGardener'])
        self.assertEqual(sorted(row['slot_mask'] for row in gardener), [2, 5])

    def test_shared_other_arm_keeps_terminal_death_track(self):
        jobs = [j for j in ENEMY_CLIP_ROWS if j['model'] == 'Crusher' and
                j['visual_state']['facts']['kaiser_right_dead'] and j['clip'] == 'left/idle_loop']
        self.assertTrue(jobs)
        for job in jobs:
            other = next(t for t in job['visual_state']['setup_tracks'] if t['track'] == 2)
            self.assertGreater(other['time'], 0)
            self.assertFalse(other['loop'])
            self.assertTrue(other['clip'].startswith('right/die'))

    def test_vantom_body_followup_preserves_complete_tail_extension(self):
        jobs = [j for j in ENEMY_CLIP_ROWS if j['model'] == 'Vantom' and
                j['clip'] == 'charge_up' and j['move'] >= 0]
        self.assertEqual(len(jobs), 24)
        for job in jobs:
            tracks = job['visual_state']['tracks']
            self.assertEqual(tracks[0], dict(track=0, clip='idle_loop', loop=True, queued=True))
            if job['move'] == 0:
                self.assertAlmostEqual(job['duration'], .15 + 1.3333334)
                phase = job['visual_state']['facts']['tail_charge']
                self.assertEqual(tracks[1]['clip'], '_tracks/charged_' + str(phase))
                self.assertEqual(tracks[2], dict(track=1, clip='_tracks/charge_up_2', loop=False, queued=True, delay=.15))
                self.assertEqual(tracks[3]['clip'], '_tracks/charged_2')
            elif job['move'] == 1:
                self.assertAlmostEqual(job['duration'], 1.3333334)
            else:
                self.assertEqual(job['move'], 3)
                self.assertAlmostEqual(job['duration'], .6666667)

    def test_common_layout_contains_all_clips_without_pivot_or_scale_change(self):
        rows = [dict(bounds=[[-20, -50], [20, 30]], reference_bounds=[[-20, -50], [20, 30]]),
                dict(bounds=[[-300, -40], [80, 60]], reference_bounds=[[-10, -25], [10, 15]])]
        layout = choose_layout(rows)
        self.assertLessEqual(layout['size'][0], 320)
        self.assertLessEqual(layout['size'][1], 218)
        for row in rows:
            low, high = np.asarray(row['bounds'])
            xy = np.asarray([low, high]) * [layout['scale'], -layout['scale']] + layout['origin']
            self.assertTrue(np.all(xy >= 1.99))
            self.assertTrue(np.all(xy <= np.asarray(layout['size']) - 1.99))
        # Reordering clips cannot move the actor or change its size.
        self.assertEqual(layout, choose_layout(list(reversed(rows))))

    def test_nonfinite_bounds_fail_instead_of_producing_placeholder_art(self):
        with self.assertRaises(ValueError):
            choose_layout([dict(bounds=[[float('nan'), 0], [1, 1]], reference_bounds=[[0, 0], [1, 1]])])

    def test_cache_resumes_only_complete_matching_hashed_jobs(self):
        with TemporaryDirectory(prefix='nspire-cache-test-') as directory:
            cache = ConversionCache(Path(directory), dict(pck='a', source={'converter.py': 'b'}))
            settings = dict(model='m', clip='c', skins=['first'], layout={'scale': .5})
            self.assertIsNone(cache.get('clip', settings))
            payload = encoded_frames([(2, 1, 0, b'\x00\x00\xff\xff')])
            cache.put('clip', settings, {'frames': 1}, payload)
            again = ConversionCache(Path(directory), copy.deepcopy(cache.dependencies))
            self.assertEqual(again.get('clip', settings), ({'frames': 1}, payload))
            changed = dict(settings, skins=['second'])
            self.assertIsNone(again.get('clip', changed))
            changed_dependency = ConversionCache(Path(directory), dict(pck='changed', source={'converter.py': 'b'}))
            self.assertIsNone(changed_dependency.get('clip', settings))
            binary = again.directory / 'clip' / (again.key('clip', settings) + '.bin')
            binary.write_bytes(payload + b'corrupt')
            with self.assertRaises(ValueError):
                again.get('clip', settings)

    def test_partial_cache_payload_is_not_a_completed_receipt(self):
        with TemporaryDirectory(prefix='nspire-cache-test-') as directory:
            cache = ConversionCache(Path(directory), {'pck': 'a'})
            key = cache.key('clip', {})
            (cache.directory / 'clip').mkdir()
            (cache.directory / 'clip' / (key + '.bin')).write_bytes(b'partial')
            self.assertIsNone(cache.get('clip', {}))

    def test_map_catalog_changes_do_not_invalidate_identical_enemy_jobs(self):
        with TemporaryDirectory(prefix='nspire-cache-test-') as directory:
            common = {'schema': 2, 'pck': 'a'}
            source = {'tools/animation.py': 'same', 'config/world-ui-0.17.json': 'before'}
            scope = {'pipeline': 'enemy-rgba-1', 'files': {'tools/animation.py': 'same'}, 'model': 'one-model'}
            first = ConversionCache(Path(directory), common, source_files=source)
            first.put('clip', {'job': 1}, {'frames': 1}, b'unchanged', scope=scope)
            source['config/world-ui-0.17.json'] = 'appended-map-catalog'
            source['src/core.cpp'] = 'different-combat-code'
            second = ConversionCache(Path(directory), common, source_files=source)
            self.assertEqual(second.get('clip', {'job': 1}, scope=scope), ({'frames': 1}, b'unchanged'))
            changed = dict(scope, model='different-model')
            self.assertIsNone(second.get('clip', {'job': 1}, scope=changed))
            changed = dict(scope, files={'tools/animation.py': 'different-rasterizer'})
            self.assertIsNone(second.get('clip', {'job': 1}, scope=changed))

    def test_v1_migration_pins_producer_code_settings_and_full_payload_hash(self):
        with TemporaryDirectory(prefix='nspire-cache-test-') as directory:
            root = Path(directory)
            source = {'tools/build_world.py': LEGACY_BUILDER_SHA256,
                      'tools/animation.py': 'geometry', 'config/enemy-visuals-0.17.json': 'all-models'}
            old = {'schema': 1, 'pck': 'pck', 'source': source}
            identity = hashlib.sha256(canonical(old)).hexdigest()
            folder = root / identity
            (folder / 'clip').mkdir(parents=True)
            (folder / 'dependencies.json').write_bytes(canonical(old))
            settings = {'job': {'clip': 'native-clip', 'frames': 1}, 'layout': [1, 2]}
            old_key = hashlib.sha256(canonical(dict(schema=1, kind='clip', dependency=identity, settings=settings))).hexdigest()
            payload = b'verified-original-frame'
            receipt = folder / 'clip' / (old_key + '.json')
            receipt.with_suffix('.bin').write_bytes(payload)
            receipt.write_bytes(canonical(dict(schema=1, key=old_key, bytes=len(payload),
                sha256=hashlib.sha256(payload).hexdigest(), metadata={'frames': 1})))
            scope = dict(pipeline='enemy-rgba-1', files={'tools/animation.py': 'geometry'},
                         functions={'encode_image': LEGACY_FUNCTIONS['encode_image']}, model='this-model')
            cache = ConversionCache(root, {'schema': 2, 'pck': 'pck'}, source_files=source)
            self.assertIsNone(cache.get('clip', dict(settings, layout=[2, 2]), scope=scope))
            self.assertIsNone(cache.get('clip', settings, scope=dict(scope, files={'tools/animation.py': 'changed'})))
            self.assertEqual(cache.get('clip', settings, scope=scope), ({'frames': 1}, payload))
            self.assertEqual(cache.legacy_hits, 1)
            imported = cache.directory / 'clip' / (cache.key('clip', settings, scope) + '.json')
            self.assertEqual(json.loads(imported.read_bytes())['reused_from']['key'], old_key)
            self.assertEqual(receipt.with_suffix('.bin').read_bytes(), payload)

    def test_frame_cache_rejects_truncation_and_extra_bytes(self):
        frame = (2, 1, 0, b'\x01\x02\x03\x04')
        data = encoded_frames([frame])
        self.assertEqual(decoded_frames(data, 1), [frame])
        for bad in (data[:-1], data + b'\x00', b'\x00\x00'):
            with self.assertRaises(ValueError):
                decoded_frames(bad, 1)
        with self.assertRaises(ValueError):
            decoded_frames(data, 2)

    def test_map_clips_append_without_reordering_enemy_directory(self):
        self.assertEqual(CLIP_ROWS[:len(ENEMY_CLIP_ROWS)], plan()['jobs'])
        self.assertEqual([row['id'] for row in MAP_CLIP_ROWS], [1638, 1639, 1640])
        self.assertEqual(sum(row['frames'] for row in MAP_CLIP_ROWS), 540)
        for row in MAP_CLIP_ROWS:
            self.assertEqual(row['shader'], 'boss_map_channels')
            self.assertTrue(row['loop'])
            self.assertLess(abs(row['frames'] * row['frame_ms'] - row['duration_ms']), 50)

    def test_map_clip_preserves_mask_channels_anchor_and_full_period(self):
        class Sampler:
            def metadata(self, resource):
                return {'clips': [{'name': 'original', 'duration': 1.}], 'source_version': '4.2'}
            def sample(self, resource, clip, times, **settings):
                self.times, self.settings = times, settings
                return [{'t': t} for t in times]
            def draw(self, resource, frame):
                pixels = np.zeros((2, 2, 4), dtype=float)
                pixels[:, :, 0] = pixels[:, :, 3] = 1
                return [(np.array([[0., 0.], [2., 0.], [0., 2.]]),
                         np.array([[0., 0.], [1., 0.], [0., 1.]]),
                         np.array([[0, 1, 2]]), pixels, np.ones(4), 0)]
        job = dict(key='map.original', resource='private.tres', clip='original',
                   duration=1., duration_ms=1000, frames=2, frame_ms=500, skins=['default'],
                   size=[4, 4], pivot=[2, 2], transform=[1, 0, 1, 0, -1, 3],
                   loop=True, shader='boss_map_channels')
        sampler = Sampler()
        frames, metadata = sample_map_clip(None, job, sampler)
        self.assertEqual(sampler.times, [0., .5])
        self.assertEqual(sampler.settings, {'skins': ['default'], 'loop': True})
        self.assertEqual(metadata['pivot'], [2, 2])
        self.assertEqual(metadata['bounds'], [[1., 1.], [3., 3.]])
        self.assertEqual(len(frames), 2)
        pixels = np.asarray(frames[0])
        visible = pixels[:, :, 3] > 0
        self.assertTrue(visible.any())
        self.assertTrue(np.all(pixels[visible, :3] == [255, 0, 0]))
        with self.assertRaisesRegex(ValueError, 'exceeds its explicit canvas'):
            sample_map_clip(None, dict(job, transform=[1, 0, 10, 0, -1, 3]), sampler)
        with self.assertRaisesRegex(ValueError, 'duration/name drift'):
            sample_map_clip(None, dict(job, duration=2), sampler)

    def test_localized_templates_keep_nested_branches_and_real_newlines(self):
        text = '{IfUpgraded:show:{Damage:diff()}|{Block:diff()}}\\n{InCombat:{Damage:diff()}|}'
        row = {'keywords': {'base': ['Exhaust'], 'upgraded': ['Exhaust', 'Innate']}}
        titles = {'EXHAUST.title': 'exhaust', 'INNATE.title': 'innate'}
        base = card_description(text, row, False, titles)
        upgraded = card_description(text, row, True, titles)
        self.assertTrue(base.startswith(text.replace('\\n', '\n')))
        self.assertNotIn('\\n', base)
        self.assertNotIn('innate', base)
        self.assertTrue(upgraded.endswith('[gold]innate。[/gold]'))

    def test_private_outputs_and_directory_budgets(self):
        with self.assertRaises(ValueError):
            private_path(ROOT / 'generated-original.nsp.tns', 'Output')
        numbers = plan_summary()
        self.assertLess(numbers['maximum_directory_bytes'], numbers['directory_limit'])
        self.assertLess(numbers['expected_images'], 65535)
        self.assertEqual(FONT_SIZES, (8, 10, 12, 16, 20))
        keys = {row['key'] for row in AUXILIARY_ROWS}
        self.assertIn('background_front.KAISER_CRAB_BOSS', keys)
        self.assertIn('rewards.CARD_REMOVAL', keys)
        self.assertTrue({f'intents.ATTACK_{i}' for i in range(2, 6)} <= keys)
        self.assertTrue({'events.ANCIENT_OPTION', 'events.ANCIENT_OPTION_OUTLINE',
                         'map.BG_OVERGROWTH_TOP', 'map.LEGEND', 'map.ANCIENT_VAKUU'} <= keys)
        validate_headers()


if __name__ == '__main__':
    unittest.main()
