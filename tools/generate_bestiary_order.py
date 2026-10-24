#!/usr/bin/env python3
"""Emit numeric Bestiary order for the pinned invariant-culture contract.

NBestiary uses CurrentCulture, while NGame sets the default current culture
to InvariantCulture. Locale-specific card/relic collation is a different
contract. Only entry indices are published; original titles stay private.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import re

from generate_collection_metadata import collate
from pck import Pack

ROOT = Path(__file__).resolve().parents[1]


def titles(pack, language, entries, test_subject_count=8):
    monsters = re.findall(r'^\s*\{"([A-Z_0-9]+)"',
                          (ROOT/'src/enemy_definitions.inc').read_text(), re.M)
    source = (ROOT/'src/enemy_encounter_definitions.inc').read_text()
    source = source.split('encounter_definitions[EncounterKinds]=', 1)[1].split('};', 1)[0]
    encounters = re.findall(r'^\s*\{"([A-Z_0-9]+)"', source, re.M)
    monster_text = json.loads(pack.read(f'localization/{language}/monsters.json'))
    encounter_text = json.loads(pack.read(f'localization/{language}/encounters.json'))

    def title(table, key):
        value = table.get(key+'.title', table.get(key+'.name', ''))
        # TestSubject.Title supplies saved TestSubjectKills +8. Its numeric
        # suffix cannot be left as an unformatted localization placeholder.
        # The complete order is checked below at the digit boundaries too.
        if key == 'TEST_SUBJECT':
            value = value.replace('{Count}', str(test_subject_count))
        if not value or '{' in value or '\0' in value:
            raise ValueError('Expected a complete plain original Bestiary title')
        return value

    bosses = [title(encounter_text, encounters[row['encounter']]) for row in entries]
    names = [boss if row['model'] == 255 else title(monster_text, monsters[row['model']])
             for row, boss in zip(entries, bosses)]
    return names, bosses


def ordered(entries, names, encounters):
    ranks = collate(names+encounters, 'root')
    count = len(entries)
    return sorted(range(count), key=lambda i: (entries[i]['region'], entries[i]['room'],
        ranks[count+i] if entries[i]['room'] == 3 else -1, ranks[i]))


def generate(pck):
    entries = json.loads((ROOT/'config/bestiary-ui7-0.17.json').read_text())['entries']
    if len(entries) != 101:
        raise ValueError('Pinned Bestiary catalogue changed')
    pack = Pack(pck)
    try:
        result = {}
        for language in ('eng', 'zhs'):
            orders = [ordered(entries, *titles(pack, language, entries, count))
                      for count in (8, 9, 10, 99, 100, 2147483647)]
            if any(order != orders[0] for order in orders[1:]):
                raise ValueError('Dynamic TestSubject title requires a runtime ordering')
            result[language] = orders[0]
        return result
    finally:
        pack.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pck', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT/'src/bestiary_order_ui7.inc')
    parser.add_argument('--oracle', type=Path, required=True,
                        help='Independent .NET InvariantCulture numeric reference order')
    args = parser.parse_args()
    result = generate(args.pck)
    expected = json.loads(args.oracle.read_text())
    if expected.get('culture') != 'InvariantCulture' or expected.get('order') != result:
        raise ValueError('ICU Bestiary order differs from independent .NET reference')
    lines = ['// Numeric entry indices only. v0.111.0 / 41cef1ea.',
        '// NGame invariant default + NBestiary room/boss/title comparer.',
        '// Generated with ICU root and checked against independent .NET metadata.',
        'static constexpr uint8_t bestiary_order[2][BestiaryEntries]={']
    for language in ('eng', 'zhs'):
        lines.append(' {')
        row = result[language]
        for i in range(0, len(row), 24):
            lines.append('  '+','.join(map(str, row[i:i+24]))+',')
        lines.append(' },')
    lines.append('};')
    args.output.write_text('\n'.join(lines)+'\n')
    print('Wrote two original Bestiary index orders; no private strings.')


if __name__ == '__main__':
    main()
