#!/usr/bin/env python3
"""Generate resource-key-only card hover metadata from an owner's reference.

The output contains no game text, artwork, or decompiled implementation. The
parser deliberately rejects unfamiliar factory expressions instead of quietly
dropping associated explanations. Runtime-dependent cases stay explicit in the
independent calculator implementation.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
REFERENCE = 'v0.111.0 / 41cef1ea'
WORDS = {'Exhaust': 1, 'Innate': 2, 'Retain': 3, 'Ethereal': 4,
         'Sly': 5, 'Unplayable': 6, 'Eternal': 7}


def slug(name: str) -> str:
    name = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', name)
    return re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', name).upper()


def property_text(source: str, name: str) -> str:
    found = re.search(r'\b' + name + r'(?:\([^)]*\))?\s*(?==>|\{)', source)
    if not found:
        return ''
    tail = source[found.end():]
    end = re.search(r'\n\t(?:public|protected|private|internal) ', tail)
    return tail[:end.start()] if end else tail


def definition_keys(path: Path) -> list[str]:
    keys: list[str] = []
    for line in path.read_text().splitlines():
        include = re.match(r'#include "([^"]+)"', line)
        if include:
            keys.extend(definition_keys(path.parent / include[1]))
        else:
            key = re.match(r'\s*\{"([A-Z0-9_]+)"', line)
            if key:
                keys.append(key[1])
    return keys


def extras(name: str, source: str) -> list[tuple[str, str, int]]:
    expression = property_text(source, 'ExtraHoverTips')
    if not expression:
        return []
    if name == 'MadScience':
        return [('Special', 'TINKER_RIDER', 0)]
    if name == 'Whistle':
        return [('Static', 'STUN', 0)]
    if name == 'FranticEscape':
        return [('Special', 'SANDPIT_POWER', 0)]
    calls = list(re.finditer(r'HoverTipFactory\.([A-Za-z]+)(?:<([^>]+)>)?\(([^)]*)\)|\bEnergyHoverTip\b', expression))
    if not calls:
        raise ValueError(f'Unrecognized hover expression: {name}')
    result: list[tuple[str, str, int]] = []
    for call in calls:
        factory, model, args = call.groups()
        if factory is None or factory == 'ForEnergy':
            result.append(('Static', 'ENERGY', 0))
        elif factory == 'FromForge':
            result.extend([('Static', 'FORGE', 0), ('Card', 'SOVEREIGN_BLADE', 2)])
        elif factory == 'FromEnchantment' and model == 'Inky':
            result.extend([('Enchantment', 'INKY', 0), ('Power', 'WEAK_POWER', 0)])
        elif factory == 'FromCard' and name == 'Fasten' and args == 'cardModel':
            result.append(('Special', 'DEFEND_OWNER', 0))
        elif factory in ('FromPower', 'FromOrb', 'FromCard', 'FromCardWithCardHoverTips'):
            if not model or args not in ('', 'IsUpgraded'):
                raise ValueError(f'Unknown {factory} argument in {name}: {call[0]}')
            kind = {'FromPower': 'Power', 'FromOrb': 'Orb',
                    'FromCard': 'Card', 'FromCardWithCardHoverTips': 'Card'}[factory]
            flags = (args == 'IsUpgraded') | (2 if factory == 'FromCardWithCardHoverTips' else 0)
            result.append((kind, slug(model), flags))
        elif factory == 'FromKeyword':
            match = re.fullmatch(r'CardKeyword\.([A-Za-z]+)', args)
            if not match or match[1] not in WORDS:
                raise ValueError(f'Unknown keyword in {name}: {args}')
            result.append(('Keyword', slug(match[1]), 0))
        elif factory == 'Static':
            match = re.fullmatch(r'StaticHoverTip\.([A-Za-z]+)(?:, DynamicVars\.Summon)?', args)
            if not match:
                raise ValueError(f'Unknown static tip in {name}: {args}')
            result.append(('Static', slug(match[1]), 0))
        else:
            raise ValueError(f'Unhandled factory in {name}: {call[0]}')
    # Every factory token must have been parsed, including custom getter cases.
    if expression.count('HoverTipFactory.') != sum('HoverTipFactory.' in m[0] for m in calls):
        raise ValueError(f'Unparsed factory in {name}')
    return result


def generate(reference: Path) -> str:
    folder = reference / 'MegaCrit.Sts2.Core.Models.Cards'
    sources = {slug(p.stem): (p.stem, p.read_text()) for p in folder.glob('*.cs')}
    keys = definition_keys(ROOT / 'src/card_definitions.inc')
    if len(keys) != 558 or len(set(keys)) != 558:
        raise ValueError('Card catalog changed; audit its hover metadata before regenerating')
    absent = [key for key in keys if key not in sources]
    if absent:
        raise ValueError(f'Reference is missing card models: {absent}')
    rows, flattened = [], []
    for key in keys:
        name, source = sources[key]
        associated = extras(name, source)
        canonical = re.findall(r'CardKeyword\.([A-Za-z]+)', property_text(source, 'CanonicalKeywords'))
        upgraded = re.findall(r'AddKeyword\(CardKeyword\.([A-Za-z]+)\)', property_text(source, 'OnUpgrade'))
        order = list(dict.fromkeys(canonical + upgraded))
        if any(word not in WORDS for word in order):
            raise ValueError(f'Unknown keyword order: {name} {order}')
        packed = sum(WORDS[word] << (i * 4) for i, word in enumerate(order))
        evoke = property_text(source, 'OrbEvokeType')
        flags = 1 if evoke and 'OrbEvokeType.None' not in evoke else 0
        rows.append(f' {{{len(flattened)},{len(associated)},{flags},0x{packed:x}u}}, // {key}')
        flattened.extend(associated)
    powers = sorted({key for kind, key, _ in flattened if kind == 'Power'} | {'STRANGLE_POWER', 'TAINTED_POWER', 'SANDPIT_POWER'})
    debuffs = []
    for key in powers:
        paths = [p for p in (reference / 'MegaCrit.Sts2.Core.Models.Powers').glob('*.cs') if slug(p.stem) == key]
        if len(paths) != 1:
            raise ValueError(f'Power not found: {key}')
        if re.search(r'Type\s*=>\s*PowerType.Debuff', paths[0].read_text()):
            debuffs.append(key)
    out = [f'// {REFERENCE}; resource keys and association metadata only.',
           '// Generated by tools/generate_keyword_hovers.py; no reference text or images.',
           'static const HoverTipSpec card_hover_specs[]={']
    out += [f' {{HoverTipKind::{kind},"{key}",{flags}}},' for kind, key, flags in flattened]
    out += ['};', 'static const CardHoverCatalog card_hover_catalog[]={', *rows, '};',
            'static_assert(sizeof card_hover_catalog/sizeof *card_hover_catalog==CardKinds,"Hover catalog must cover every card");',
            'static const char* const hover_debuff_keys[]={']
    out += [f' "{key}",' for key in debuffs]
    out += ['};', '']
    return '\n'.join(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path, help='Owner-provided decompiled reference directory')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    output = generate(args.reference)
    path = ROOT / 'src/keyword_hover_catalog.inc'
    if args.check:
        if not path.exists() or path.read_text() != output:
            raise SystemExit('Keyword hover metadata is out of date')
        print('Keyword hover metadata: 558 models match the pinned reference')
    else:
        path.write_text(output)


if __name__ == '__main__':
    main()
