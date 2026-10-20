"""Asset-free bounds, resumable-cache and original clip-coverage regressions."""
import copy
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from build_world import (ConversionCache, card_description, choose_layout,
    decoded_frames, encoded_frames, plan_summary, private_path, ROOT, FONT_SIZES)
from world_enemy_assets import manifest
from world_enemy_profile import plan, skins
from world_ui_profile import BASE_CLIPS, CLIP_ROWS, AUXILIARY_ROWS, validate_headers


class WorldPack(unittest.TestCase):
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
        jobs = [j for j in CLIP_ROWS if j['model'] == 'Crusher' and
                j['visual_state']['facts']['kaiser_right_dead'] and j['clip'] == 'left/idle_loop']
        self.assertTrue(jobs)
        for job in jobs:
            other = next(t for t in job['visual_state']['setup_tracks'] if t['track'] == 2)
            self.assertGreater(other['time'], 0)
            self.assertFalse(other['loop'])
            self.assertTrue(other['clip'].startswith('right/die'))

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

    def test_frame_cache_rejects_truncation_and_extra_bytes(self):
        frame = (2, 1, 0, b'\x01\x02\x03\x04')
        data = encoded_frames([frame])
        self.assertEqual(decoded_frames(data, 1), [frame])
        for bad in (data[:-1], data + b'\x00', b'\x00\x00'):
            with self.assertRaises(ValueError):
                decoded_frames(bad, 1)
        with self.assertRaises(ValueError):
            decoded_frames(data, 2)

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
        validate_headers()


if __name__ == '__main__':
    unittest.main()
