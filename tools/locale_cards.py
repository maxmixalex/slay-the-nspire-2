"""Bind private original card templates to public, audited numeric metadata.

No localized source text is stored here. Numeric defaults travel with each
template, so both languages keep original plural/conditional grammar and the
runtime can replace a default with the selected card instance's live value.
"""
from __future__ import annotations
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
KEYWORDS = ('EXHAUST', 'INNATE', 'RETAIN', 'ETHEREAL', 'SLY', 'UNPLAYABLE', 'ETERNAL')
RUNTIME = frozenset(('IfUpgraded', 'InCombat', 'IsTargeting', 'IsOstyAlive',
    'TargetType', 'singleStarIcon', 'energyPrefix', 'CardType', 'HasRider',
    'Sapping', 'Violence', 'Choking', 'Energized', 'Wisdom', 'Chaos', 'Expertise',
    'Curious', 'Improvement', 'None', 'SeekingEdge', 'Parry', 'SwordSage'))


def model_rows():
    from build_all_characters import models as later_models
    rows = {}
    for stem in ('ironclad', 'silent'):
        obj = json.loads((ROOT / f'config/{stem}-cards-0.15.json').read_text())
        for row in obj['cards'] + obj.get('tokens', []):
            rows[row['id']] = dict(row)
    for row in later_models():
        rows[row['id']] = dict(row)
    obj = json.loads((ROOT / 'config/world-cards-0.17.json').read_text())
    for row in obj['cards']:
        rows[row['id']] = dict(row)
    # Earlier colorless models predate structured metadata. Their public
    # Definition records contain the exact same audited values used in play.
    lines = []
    for name in ('card_definitions.inc', 'colorless_definitions.inc'):
        for line in (ROOT / 'src' / name).read_text().splitlines():
            if re.match(r'\s*\{"', line) and '"GIANT_ROCK"' not in line:
                fields = line.split('}', 1)[0].strip().lstrip('{').split(',')
                lines.append(fields)
    if len(lines) != 114:
        raise ValueError('Legacy card definition order changed')
    power_names = ('Power', 'Energy', 'StrengthLoss', 'PlatingPower', 'ExtraBlock',
        'Replay', 'PanacheDamage', 'PrepTimePower', 'StrengthPower',
        'DexterityPower', 'ExtraDamage', 'RollingBoulderPower', 'BombDamage',
        'Gold', 'NoBlockPower', 'VulnerablePower', 'WeakPower')
    for number, fields in enumerate(lines):
        if number in rows:
            continue
        f = fields + ['0'] * (21-len(fields))
        def pair(index, delta):
            return dict(base=int(f[index]), upgraded=int(f[index])+int(f[delta]))
        variables = dict(Damage=pair(2,6), CalculatedDamage=pair(2,6),
            Block=pair(3,7), Cards=pair(4,8), Turns=dict(base=3,upgraded=3),
            IncrementAmount=dict(base=5,upgraded=5))
        for name in power_names:
            variables[name] = pair(5,9)
        keywords = {}
        for variant, i in (('base',19),('upgraded',20)):
            bits = int(f[i]); values = [name.title() for bit,name in enumerate(KEYWORDS) if bits & (1<<bit)]
            if f[15] == 'true' and 'Exhaust' not in values:
                values.append('Exhaust')
            keywords[variant] = values
        rows[number] = dict(id=number,key=json.loads(f[0]),type=f[10].split('::')[-1],
            variables=variables,dynamic_variables={},keywords=keywords)
    if sorted(rows) != list(range(558)):
        raise ValueError('Missing card metadata')
    return [rows[i] for i in range(558)]


def defaults(row, upgraded):
    variant = 'upgraded' if upgraded else 'base'
    values = {}
    for name, values_by_variant in row.get('variables', {}).items():
        value = values_by_variant.get(variant, values_by_variant.get('base'))
        if isinstance(value, str) and re.fullmatch(r'-?[0-9]+', value):
            value = int(value)
        if type(value) is int:
            values[name] = value
    for name in row.get('dynamic_variables', {}):
        if name in ('CalculatedDamage','CalculatedBlock','CalculatedChannels',
                    'CalculatedCards','CalculatedFocus','CalculatedForge','CalculatedDoom'):
            if 'CalculationBase' in values:
                values[name] = values['CalculationBase']
            elif name == 'CalculatedBlock':
                values[name] = 0
        elif name == 'CalculatedHits':
            values[name] = values.get('Repeat', 0)
        elif name == 'CalculatedShivs':
            values[name] = 0
    if row['key'] == 'NORMALITY':
        values['CalculatedCards'] = values['CalculationBase']
    if row['key'] == 'STACK':
        values['CalculatedBlock'] = values['CalculationBase']
    if row['key'] == 'SOVEREIGN_BLADE':
        values.update(CalculatedBlock=0,GainsBlock=0,Repeat=1)
    return values


def bind_template(source, row, upgraded):
    values = defaults(row, upgraded)
    unknown = set()
    def bind(match):
        name = match[1]
        if not name or name in RUNTIME:
            return match[0]
        if name in values:
            return '{' + name + '=' + str(values[name]) + match[2]
        unknown.add(name)
        return match[0]
    result = re.sub(r'\{([A-Za-z][A-Za-z0-9_.]*|)(:|\})', bind, source)
    if unknown:
        raise ValueError(f"Missing card variable {row['key']}: {sorted(unknown)}")
    return result


def card_strings(tables, language):
    cards, keywords = tables['cards'], tables['card_keywords']
    output = []
    for row in model_rows():
        key = row['key']
        title = cards[key + '.title']
        raw = cards.get(key + '.description', '')
        variants = []
        for upgraded in (False, True):
            text = bind_template(raw, row, upgraded)
            for name in row['keywords']['upgraded' if upgraded else 'base']:
                kw = name.upper()
                if kw not in KEYWORDS:
                    raise ValueError('Unknown card keyword: ' + name)
                text += ('\n' if text else '') + '[gold]' + keywords[kw + '.title'] + ('。' if language == 'zhs' else '.') + '[/gold]'
            variants.append(text)
        output.append((title, *variants))
    return output
