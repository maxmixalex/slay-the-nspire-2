#!/usr/bin/env python3
"""Asset-free preservation and strict localized-variable tests for NSP2/13."""
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from all_character_ui_profile import (BASE_CARDS, BASE_STRINGS, BASE_AUXILIARY,
    BASE_CLIPS, CARDS, STRINGS, AUXILIARY, CLIPS, ALL_CHARACTER_UI, NEW_CLIPS)
from build_all_characters import (models, localized, retained_prefix,
    assert_retained, validate_headers)
from build_content import fnv
from content_pack14 import PackData, PROFILES, CLIP_COUNTS, LIMIT


def synthetic_v12():
    ni = ng = 1
    start = 40 + ni * 16 + ng * 16 + BASE_STRINGS * 8 + BASE_CLIPS * 16 + BASE_CARDS * 16 + BASE_AUXILIARY * 4
    image = bytes(82 * 60 * 2)
    images = struct.pack('<IHHII', start, 82, 60, 0, len(image))
    glyphs = struct.pack('<IIHBBHH', 65, start + len(image), 10, 1, 1, 1, 0)
    strings = struct.pack('<II', start + len(image) + 1, 1) * BASE_STRINGS
    clips = struct.pack('<IHHhhI', 0, 1, 33, 0, 0, 0) * BASE_CLIPS
    cards = struct.pack('<4I', 0, 0, 0, 0) * BASE_CARDS
    body = images + glyphs + strings + clips + cards + bytes(BASE_AUXILIARY * 4) + image + b'\xffA\0'
    return struct.pack('<4s9I', b'NSP2', 12, 0x41cef1ea, len(body) + 40, ni, ng, BASE_STRINGS, BASE_CLIPS, BASE_CARDS, fnv(body)) + body


def expanded():
    pack = PackData(synthetic_v12()); prefix = retained_prefix(pack)
    pack.images.append((82, 60, 0, bytes([7]) * 82 * 60 * 2))
    pack.strings += ['new title', 'base {OstyDamage=10}', 'upgrade {OstyDamage=15}'] * (CARDS - BASE_CARDS)
    pack.cards += [(1, BASE_STRINGS + i * 3, BASE_STRINGS + i * 3 + 1, BASE_STRINGS + i * 3 + 2) for i in range(CARDS - BASE_CARDS)]
    pack.aux += [1] * (AUXILIARY - BASE_AUXILIARY)
    pack.clips += struct.pack('<IHHhhI', 1, 1, 125, 0, 0, 0) * (CLIPS - BASE_CLIPS)
    pack.glyphs[66, 10] = (1, 1, 1, 0, b'\x80')
    return pack, prefix


class AllCharacterPackTests(unittest.TestCase):
    def test_frozen_profiles_and_new_roundtrip(self):
        self.assertEqual(PROFILES[11], (114, 407, 185))
        self.assertEqual(PROFILES[12], (228, 749, 233))
        self.assertEqual(PROFILES[13], (497, 1556, 339))
        self.assertEqual({version: CLIP_COUNTS[version] for version in range(10, 14)},
                         {10: 17, 11: 17, 12: 17, 13: 38})
        self.assertEqual(CLIP_COUNTS[14], 1695)
        self.assertEqual(LIMIT, 13 * 1024 * 1024)
        pack, prefix = expanded(); data = pack.encode(13); read = PackData(data)
        self.assertEqual(read.version, 13); self.assertEqual(read.encode(13), data)
        assert_retained(read, prefix)
        repeated = retained_prefix(read)
        self.assertEqual(repeated[:5], prefix[:5])
        for key, value in prefix[5].items(): self.assertEqual(repeated[5][key], value)

    def test_every_historical_directory_and_payload_is_protected(self):
        for name in ('images', 'cards', 'strings', 'aux', 'clips', 'glyphs'):
            pack, prefix = expanded()
            if name == 'images': pack.images[0] = (82, 60, 0, bytes([1]) * 82 * 60 * 2)
            elif name == 'cards': pack.cards[0] = (0, 1, 0, 0)
            elif name == 'strings': pack.strings[0] = 'changed'
            elif name == 'aux': pack.aux[0] = 1
            elif name == 'clips': pack.clips = b'\x01' + pack.clips[1:]
            else: pack.glyphs[65, 10] = (1, 1, 1, 0, b'\x00')
            with self.subTest(name=name), self.assertRaises(ValueError): assert_retained(pack, prefix)

    def test_versions_and_animation_counts_cannot_be_relabelled(self):
        old = PackData(synthetic_v12())
        with self.assertRaises(ValueError): old.encode(13)
        new, _ = expanded()
        with self.assertRaises(ValueError): new.encode(12)
        new.clips = new.clips[:BASE_CLIPS * 16]
        with self.assertRaises(ValueError): new.encode(13)
        for data, version in ((synthetic_v12(), 13), (expanded()[0].encode(13), 12)):
            wrong = bytearray(data); struct.pack_into('<I', wrong, 4, version)
            with self.subTest(version=version), self.assertRaises(ValueError): PackData(wrong)

    def test_all_registry_ids_keywords_and_asset_ranges_match(self):
        rows = models(); validate_headers()
        self.assertEqual([row['id'] for row in rows], list(range(228, 497)))
        self.assertEqual(len({row['key'] for row in rows}), 269)
        for character in ('defect', 'regent', 'necrobinder'):
            self.assertEqual(sum(row['character'] == character for row in rows), 86)
        self.assertEqual(len(ALL_CHARACTER_UI), 106); self.assertEqual(len(NEW_CLIPS), 21)
        tokens = {row['key']: row for row in rows if row['id'] >= 486}
        self.assertEqual(tokens['BURN']['variables']['Damage'], {'base': 2, 'upgraded': 2})
        self.assertEqual(tokens['FUEL']['variables']['Energy'], {'base': 1, 'upgraded': 2})
        self.assertEqual(tokens['SWEEPING_GAZE']['variables']['OstyDamage'], {'base': 10, 'upgraded': 15})

    def test_calculated_values_stars_and_energy_keep_exact_meaning(self):
        row = {'key': 'SYNTHETIC', 'type': 'Attack', 'variables': {
            'OstyDamage': {'base': 10, 'upgraded': 15}, 'CalculationBase': {'base': 5, 'upgraded': 7},
            'Stars': {'base': 2, 'upgraded': 3}, 'Energy': {'base': 1, 'upgraded': 2},
            'CardPlay': {'base': 5, 'upgraded': 5}}, 'dynamic_variables': {'CalculatedForge': 'runtime'},
            'keywords': {'base': [], 'upgraded': []}}
        text = '{OstyDamage:diff()} {CalculatedForge:diff()} {Stars:starIcons()} {Energy:energyIcons()} {CardPlay}'
        self.assertEqual(localized(text, row, True, {}), '{OstyDamage=15} {CalculatedForge=7} 3 点[gold]星星[/gold] 2 点能量 5')
        self.assertEqual(localized('0{energyPrefix:energyIcons(1)} / {energyPrefix:energyIcons(1)} / {energyPrefix:energyIcons(4)}', row, False, {}), '0 点能量 / 1 点能量 / 4 点能量')
        self.assertEqual(localized('{singleStarIcon}', row, False, {}), '[gold]星星[/gold]')

    def test_osty_combat_and_target_conditions_keep_distinct_markers(self):
        row = {'key': 'SACRIFICE', 'variables': {'CalculationBase': {'base': 0, 'upgraded': 0}},
               'dynamic_variables': {'CalculatedBlock': 'runtime'}, 'keywords': {'base': [], 'upgraded': []}}
        self.assertEqual(localized('{InCombat:{IsOstyAlive:{CalculatedBlock:diff()}|0}|}', row, False, {}), '\x1e{CalculatedBlock=0}\x1f')
        row['key'] = 'OTHER'
        with self.assertRaises(ValueError): localized('{IsOstyAlive:{CalculatedBlock:diff()}|0}', row, False, {})
        rows = {entry['key']: entry for entry in models()}
        for key, name, value in (('NO_ESCAPE', 'CalculatedDoom', 10), ('TIMES_UP', 'CalculatedDamage', 0)):
            target = dict(rows[key]); target['keywords'] = {'base': [], 'upgraded': []}
            self.assertEqual(localized('{IsTargeting:{' + name + ':diff()}|}', target, False, {}), '\x1c{' + name + '=' + str(value) + '}\x1d')
        with self.assertRaises(ValueError): localized('{IsTargeting:{CalculatedBlock:diff()}|}', row, False, {})

    def test_empty_status_description_and_unpowered_burn(self):
        rows = {row['key']: row for row in models()}
        keywords = {'UNPLAYABLE.title': 'Unplayable', 'ETHEREAL.title': 'Ethereal', 'EXHAUST.title': 'Exhaust'}
        self.assertEqual(localized('', rows['WOUND'], False, keywords), '[gold]Unplayable。[/gold]')
        self.assertEqual(localized('', rows['DEBRIS'], False, keywords), '[gold]Exhaust。[/gold]')
        value = localized('Burn {Damage:diff()}.', rows['BURN'], False, keywords)
        self.assertIn('Burn 2.', value); self.assertNotIn('{Damage=', value)

    def test_unknown_values_and_unhandled_live_branches_fail(self):
        row = {'key': 'SYNTHETIC', 'variables': {}, 'dynamic_variables': {}, 'keywords': {'base': [], 'upgraded': []}}
        for text in ('{Unknown:diff()}', '{IsOstyAlive:one|two}', '{energyPrefix:energyIcons(0)}',
                     '{Other}', '{InCombat:live|other}', '{Stars:choose(2):one|two}',
                     '{IfUpgraded:show:one|two|three}', '{Damage:diff()', 'stray }'):
            with self.subTest(text=text), self.assertRaises(ValueError): localized(text, row, False, {})


if __name__ == '__main__': unittest.main()
