#!/usr/bin/env python3
"""Generate numeric UI targeting metadata from a pinned private reference.

Only model target enum values enter the output. Source C#, localized strings,
art and executable bytes stay in the owner's private input directory. The four
runtime overrides are implemented separately against the existing simulation.
"""
from __future__ import annotations
import argparse
from collections import Counter
from pathlib import Path
import re
from locale_cards import model_rows

TARGETS = ('None', 'Self', 'AnyEnemy', 'AllEnemies', 'RandomEnemy',
           'AnyPlayer', 'AnyAlly', 'AllAllies', 'TargetedNoCreature', 'Osty')


def extract(reference: Path):
    paths = {p.stem.casefold(): p for p in reference.glob('*.cs')}
    values = []
    for row in model_rows():
        stem = row.get('model') or row['key'].replace('_', '')
        path = paths.get(stem.casefold())
        if path is None:
            raise ValueError(f"Missing reference model: {row['key']}")
        source = path.read_text()
        found = re.findall(r':\s*base\([^;{}]*?TargetType\.([A-Za-z]+)', source)
        if len(found) != 1 or found[0] not in TARGETS:
            raise ValueError(f"Expected one constant constructor target: {row['key']}")
        if row.get('target') and row['target'] != found[0]:
            raise ValueError(f"Audited catalog disagrees with constructor: {row['key']}")
        values.append(TARGETS.index(found[0]))
    return values


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference', required=True, type=Path)
    p.add_argument('--output', type=Path, default=Path('src/card_target_types.inc'))
    args = p.parse_args()
    values = extract(args.reference)
    lines = ['// Numeric TargetType constructor values for all 558 port models.',
             '// Pinned v0.111.0 / 41cef1ea; generate_card_target_types.py.',
             '// None, Self, AnyEnemy, AllEnemies, RandomEnemy, AnyPlayer,',
             '// AnyAlly, AllAllies, TargetedNoCreature, Osty = 0..9.']
    for i in range(0, len(values), 24):
        lines.append(','.join(map(str, values[i:i+24])) + ',')
    args.output.write_text('\n'.join(lines) + '\n')
    print({'models': len(values), 'targets': {TARGETS[k]: v for k, v in sorted(Counter(values).items())}})


if __name__ == '__main__':
    main()
