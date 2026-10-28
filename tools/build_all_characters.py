#!/usr/bin/env python3
"""Append complete Defect, Regent and Necrobinder art to private NSP2/12.

Reads the owner's v0.111.0 installation. All earlier image
payloads, card/text/auxiliary records, animation clips and glyphs survive byte
for byte. Public source includes paths, audited numbers and converter logic;
the generated pack, localized strings and rendered original art stay private.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageOps
from animation import Skeleton, load_textures, geometry, pose, raster, bake
from all_character_ui_profile import (BASE_CARDS, BASE_STRINGS, BASE_AUXILIARY,
    BASE_CLIPS, CARDS, STRINGS, AUXILIARY, CLIPS, ALL_CHARACTER_UI, NEW_CLIPS,
    DEFECT_POWERS, REGENT_POWERS, NECRO_POWERS, POWER_STARTS)
from build_characters import KEYWORD_KEYS, _branches
from build_content import ROOT, atlas, choose_font, image_data, prepared, texture
from build_expansion import rle
from build_tactics import compact_image
from card_banner import material_tint
from content_pack14 import PackData, LIMIT
from pck import Pack
from reference_inventory import verify_release
from sparse_image import encode as sparse_encode

LIVE_VARIABLES = frozenset(('Damage', 'Block', 'OstyDamage', 'BlockNextTurn',
    'CalculatedDamage', 'CalculatedBlock', 'CalculatedHits', 'CalculatedCards',
    'CalculatedChannels', 'CalculatedFocus', 'CalculatedForge', 'CalculatedDoom'))
GROUPS = [('defect', 'defect', 228, DEFECT_POWERS),
          ('regent', 'regent', 314, REGENT_POWERS),
          ('necro', 'necrobinder', 400, NECRO_POWERS)]


def shared_tokens(root: Path = ROOT) -> list[dict]:
    """The eleven generated/status models appended after the three pools."""
    def row(number, model, key, kind, cost, keywords, variables, maximum=0):
        return {'id': number, 'model': model, 'key': key, 'type': kind,
            'character': 'status' if kind == 'Status' else 'token',
            'portrait': f"images/packed/card_portraits/{'status' if kind == 'Status' else 'token'}/{key.lower()}.png",
            'cost': {'base': cost, 'upgraded': cost}, 'max_upgrade_level': maximum,
            'keywords': {'base': keywords[:], 'upgraded': keywords[:]},
            'variables': {name: {'base': values[0], 'upgraded': values[1]}
                          for name, values in variables.items()},
            'dynamic_variables': {}, 'generation_allowed': False}
    # Static values were read from the pinned model constructors, canonical
    # variables, keyword methods and maximum-upgrade getters. -2 is this
    # runtime's unplayable sentinel, distinct from playable X (-1).
    rows = [
        row(486, 'Dazed', 'DAZED', 'Status', -2, ['Ethereal', 'Unplayable'], {}),
        row(487, 'Wound', 'WOUND', 'Status', -2, ['Unplayable'], {}),
        row(488, 'Slimed', 'SLIMED', 'Status', 1, ['Exhaust'], {'Cards': (1, 1)}),
        row(489, 'Burn', 'BURN', 'Status', -2, ['Unplayable'], {'Damage': (2, 2)}),
        row(490, 'Void', 'VOID', 'Status', -2, ['Unplayable', 'Ethereal'], {'Energy': (1, 1)}),
        row(491, 'Fuel', 'FUEL', 'Skill', 0, ['Exhaust'], {'Energy': (1, 2)}, 1),
    ]
    metadata = json.loads((root / 'config/regent-cards-0.16.json').read_text())['generated_metadata']
    by_name = {entry['model']: entry for entry in metadata}
    for number, model in enumerate(('MinionStrike', 'MinionDiveBomb', 'MinionSacrifice', 'Debris'), 492):
        item = by_name[model]
        variables = {}
        for key in ('damage', 'block'):
            if key in item: variables[key.title()] = tuple(item[key])
        if 'draw' in item: variables['Cards'] = (item['draw'], item['draw'])
        rows.append(row(number, model, item['key'], item['type'], item['cost'],
                        item['keywords'], variables, item['max_upgrade_level']))
        if rows[-1]['portrait'] != item['portrait']: raise ValueError('Generated portrait path drift')
    rows.append(row(496, 'SweepingGaze', 'SWEEPING_GAZE', 'Attack', 0,
                    ['Ethereal', 'Exhaust'], {'OstyDamage': (10, 15)}, 1))
    return rows


def models(root: Path = ROOT) -> list[dict]:
    rows = []
    for stem, character, start, powers in GROUPS:
        path = root / 'config' / f'{stem}-cards-0.16.json'
        source = json.loads(path.read_text())
        reference = source.get('reference', {})
        if (source.get('reference_version', reference.get('tag')) != 'v0.111.0' or
            source.get('reference_commit', reference.get('commit')) != '41cef1ea'):
            raise ValueError(f'Unpinned character metadata: {path.name}')
        scope = source.get('scope', {})
        if len(source['cards']) != 86 or source.get('single_player_count', scope.get('single_player_pool')) != 86:
            raise ValueError(f'Incomplete single-player pool: {stem}')
        excluded = set(source.get('excluded_multiplayer', scope.get('multiplayer_excluded', [])))
        if [entry['enum'] for entry in source['state_powers']] != [name for name, _ in powers]:
            raise ValueError(f'Power-state/visual order drift: {stem}')
        for number, original in enumerate(source['cards'], start):
            row = dict(original)
            if row['id'] != number or row['model'] in excluded:
                raise ValueError(f'Card ID or single-player scope drift: {stem}')
            row['character'] = character
            row.setdefault('portrait', f"images/packed/card_portraits/{character}/{row['key'].lower()}.png")
            row.setdefault('dynamic_variables', {})
            rows.append(row)
    rows.extend(shared_tokens(root))
    if [row['id'] for row in rows] != list(range(BASE_CARDS, CARDS)):
        raise ValueError('Missing, duplicate, or reordered new model')
    return rows


def _canonical(row: dict, name: str, upgraded: bool) -> int:
    variant = 'upgraded' if upgraded else 'base'
    variables = row['variables']
    if name in variables:
        value = variables[name][variant]
    elif name in row.get('dynamic_variables', {}) and name in LIVE_VARIABLES:
        if 'CalculationBase' not in variables:
            raise ValueError(f"Missing calculation base: {row['key']}.{name}")
        value = variables['CalculationBase'][variant]
    else:
        raise ValueError(f"Unknown canonical variable: {row['key']}.{name}")
    if type(value) is not int or not -999999 <= value <= 999999:
        raise ValueError(f"Unsupported canonical value: {row['key']}.{name}")
    return value


def _live(row: dict, name: str) -> bool:
    # Burn damage is an unpowered end-turn effect, never an attack preview.
    return name in LIVE_VARIABLES and not (row.get('type') == 'Status' and name == 'Damage')


def localized(text: str, row: dict, upgraded: bool, keyword_titles: dict) -> str:
    """Resolve the pinned grammar, retaining explicit named live values.

    A missing grammar form or numeric variable is an error. Catalog previews
    use the literal fallback inside {CalculatedDamage=8}; live combat uses the
    matching C++ character hook. No unknown variable silently becomes zero.
    """
    def render(source: str, depth: int = 0, context: str | None = None) -> str:
        if depth > 8: raise ValueError('Localization nesting exceeds bound')
        result = []
        at = 0
        while at < len(source):
            if source[at] == '}': raise ValueError(f"Unbalanced description: {row['key']}")
            if source[at] != '{':
                result.append(source[at]); at += 1; continue
            end, nested = at + 1, 1
            while end < len(source) and nested:
                if source[end] == '{': nested += 1
                elif source[end] == '}': nested -= 1
                end += 1
            if nested: raise ValueError(f"Unclosed description: {row['key']}")
            expression = source[at + 1:end - 1]
            if expression.startswith('IfUpgraded:show:'):
                options = _branches(expression[len('IfUpgraded:show:'):])
                selected = options[0] if upgraded else options[1] if len(options) > 1 else ''
                result.append(render(selected, depth + 1, context))
            elif expression.startswith('InCombat:'):
                options = _branches(expression[len('InCombat:'):])
                if len(options) > 1 and options[1]: raise ValueError('Unsupported noncombat alternative')
                result.append('\x1e' + render(options[0], depth + 1, context) + '\x1f')
            elif expression.startswith('IsTargeting:'):
                options = _branches(expression[len('IsTargeting:'):])
                if row['key'] not in ('NO_ESCAPE', 'TIMES_UP') or len(options) != 2 or options[1]:
                    raise ValueError('Unsupported target conditional')
                result.append('\x1c' + render(options[0], depth + 1, context) + '\x1d')
            elif expression.startswith('IsOstyAlive:'):
                # Sacrifice's block hook explicitly returns zero while Osty
                # is dead. Keep that live value instead of a catalog boolean.
                options = _branches(expression[len('IsOstyAlive:'):])
                if row['key'] != 'SACRIFICE' or options != ['{CalculatedBlock:diff()}', '0']:
                    raise ValueError('Unsupported live Osty conditional')
                result.append(render(options[0], depth + 1, context))
            elif expression == 'singleStarIcon':
                result.append('[gold]星星[/gold]')
            elif expression.startswith('energyPrefix:'):
                match = re.fullmatch(r'energyPrefix:energyIcons\(([1-9][0-9]*)\)', expression)
                if not match: raise ValueError('Unsupported energy icon count')
                # A prefixed numeric cost (0 + one icon) must stay zero. A
                # standalone icon grant (Subroutine) must display its one.
                preceding = ''.join(result)
                result.append(('' if re.search(r'[0-9X]$', preceding) else match[1]) + ' 点能量')
            else:
                conditional = re.fullmatch(r'([A-Za-z][A-Za-z0-9]*):(plural|choose\(1\)):(.*)', expression, re.S)
                if conditional:
                    name, _, choices = conditional.groups()
                    options = _branches(choices)
                    if len(options) != 2 or _live(row, name) or name in row.get('dynamic_variables', {}):
                        raise ValueError(f'Unsupported count conditional: {expression}')
                    value = _canonical(row, name, upgraded)
                    result.append(render(options[0 if value == 1 else 1], depth + 1, name))
                    at = end; continue
                match = re.fullmatch(r'([A-Za-z][A-Za-z0-9]*)?:(diff\(\)|inverseDiff\(\)|energyIcons\(\)|starIcons\(\))', expression)
                if not match:
                    if expression != 'CardPlay': raise ValueError(f"Unsupported expression: {row['key']}: {expression}")
                    name, style = expression, 'diff()'
                else:
                    name, style = match.groups(); name = name or context
                if not name: raise ValueError('Value format lacks selected variable')
                value = _canonical(row, name, upgraded)
                if name in row.get('dynamic_variables', {}) and not _live(row, name):
                    raise ValueError(f"Unsupported live variable: {row['key']}.{name}")
                result.append('{' + name + '=' + str(value) + '}' if _live(row, name) else str(value))
                if style == 'energyIcons()': result.append(' 点能量')
                elif style == 'starIcons()': result.append(' 点[gold]星星[/gold]')
            at = end
        return ''.join(result)
    output = render(text.replace('\\n', '\n'))
    for name in row['keywords']['upgraded' if upgraded else 'base']:
        if name not in KEYWORD_KEYS: raise ValueError(f'Unsupported keyword: {name}')
        key = KEYWORD_KEYS[name] + '.title'
        if key not in keyword_titles: raise ValueError(f'Missing keyword title: {key}')
        output += ('\n' if output else '') + '[gold]' + keyword_titles[key] + '。[/gold]'
    markers = re.findall(r'\{([A-Za-z][A-Za-z0-9]*)=-?[0-9]+\}', output)
    if any(name not in LIVE_VARIABLES for name in markers) or output.count('{') != len(markers) or output.count('}') != len(markers):
        raise ValueError(f"Unresolved description: {row['key']}")
    return output


def retained_prefix(source: PackData) -> tuple:
    if source.version not in (12, 13): raise ValueError('Five-character conversion requires NSP2/12 or NSP2/13')
    count = len(source.images) if source.version == 12 else min(r[0] for r in source.cards[BASE_CARDS:])
    clips = source.clips[:BASE_CLIPS * 16]
    if (count < 1 or any(r[0] >= count for r in source.cards[:BASE_CARDS]) or
        any(i >= count for i in source.aux[:BASE_AUXILIARY]) or
        any(first + frames > count for first, frames, *_ in struct.iter_unpack('<IHHhhI', clips))):
        raise ValueError('Incomplete historical image prefix')
    return (source.images[:count], source.cards[:BASE_CARDS], source.strings[:BASE_STRINGS],
            source.aux[:BASE_AUXILIARY], clips, source.glyphs.copy())


def assert_retained(pack: PackData, prefix: tuple) -> None:
    images, cards, strings, aux, clips, glyphs = prefix
    if (pack.images[:len(images)] != images or pack.cards[:BASE_CARDS] != cards or
        pack.strings[:BASE_STRINGS] != strings or pack.aux[:BASE_AUXILIARY] != aux or
        pack.clips[:BASE_CLIPS * 16] != clips):
        raise ValueError('Historical image/card/text/auxiliary/animation bytes changed')
    if any(pack.glyphs.get(key) != value for key, value in glyphs.items()):
        raise ValueError('Historical glyph payload changed')


def validate_headers(root: Path = ROOT) -> None:
    def capacity(header: str, name: str) -> int:
        # The five-character prefix is frozen; later world profiles append to
        # it and resolve their total through world_assets.hpp.
        match = re.search(r'\b' + name + r'\s*=\s*(\w+)', header)
        if not match:
            raise ValueError('Missing header capacity: ' + name)
        value = match.group(1)
        if not value.isdecimal():
            world = (root / 'include/world_assets.hpp').read_text()
            match = re.search(r'\b' + value + r'\s*=\s*(\d+)\b', world)
            if not match:
                raise ValueError('Unknown header capacity: ' + value)
            value = match.group(1)
        return int(value)

    header = (root / 'include/ui_assets.hpp').read_text()
    for index, (name, *_rest) in enumerate(ALL_CHARACTER_UI, BASE_AUXILIARY):
        if not re.search(r'\bU_' + name + r'\s*=\s*' + str(index) + r'\b', header):
            raise ValueError('Auxiliary header/profile drift: ' + name)
    for name, index in POWER_STARTS.items():
        if not re.search(r'\bU_' + name + r'PowerStart\s*=\s*' + str(index) + r'\b', header):
            raise ValueError('Power range drift: ' + name)
    if capacity(header, 'U_Count') < AUXILIARY: raise ValueError('Truncated auxiliary prefix')
    header = (root / 'include/assets.hpp').read_text()
    for index, (name, *_rest) in enumerate(NEW_CLIPS, BASE_CLIPS):
        if not re.search(r'\bAnim' + name + r'\s*=\s*' + str(index) + r'\b', header):
            raise ValueError('Animation header/profile drift: ' + name)
    if capacity(header, 'AnimationKinds') < CLIPS: raise ValueError('Truncated animation prefix')


def _layers(pck: Pack, names: list[str], material: str | None = None) -> Image.Image:
    image = None
    for name in names:
        layer = texture(pck, name)
        if material: layer = material_tint(pck, layer, material)
        if image is None: image = layer.copy()
        else:
            if layer.size != image.size: layer = layer.resize(image.size, Image.Resampling.LANCZOS)
            image.alpha_composite(layer)
    if image is None: raise ValueError('Empty original layer composite')
    return image


def build(game: Path, source: Path, output: Path, font_path: Path) -> dict:
    verify_release(game)
    rows = models(); validate_headers()
    pack = PackData(source.read_bytes()); prefix = retained_prefix(pack)
    pack.images, pack.cards, pack.strings, pack.aux = ([*prefix[0]], [*prefix[1]], [*prefix[2]], [*prefix[3]])
    pack.clips = prefix[4]
    paths, animation_meta = [], []
    def add_image(image: Image.Image) -> int:
        image = image.convert('RGBA')
        alpha = image.getchannel('A').getextrema()[0] < 255
        fmt, raw = compact_image(int(alpha), image_data(image, alpha))
        index = len(pack.images); pack.images.append((*image.size, fmt, raw)); return index
    pck = Pack(game / 'SlayTheSpire2.pck')
    try:
        card_text = json.loads(pck.read('localization/zhs/cards.json'))
        keywords = json.loads(pck.read('localization/zhs/card_keywords.json'))
        for row in rows:
            name, key = row['portrait'], row['key']
            if name + '.import' not in pck.entries: raise ValueError('Missing original portrait: ' + name)
            if key + '.title' not in card_text: raise ValueError('Missing original card title: ' + key)
            text = card_text.get(key + '.description')
            if text is None and key not in ('DAZED', 'WOUND', 'DEBRIS'): raise ValueError('Missing original description: ' + key)
            first = len(pack.strings)
            pack.strings.extend([card_text[key + '.title'], localized(text or '', row, False, keywords),
                                 localized(text or '', row, True, keywords)])
            if row['id'] != len(pack.cards): raise ValueError('Appended card directory order drift')
            pack.cards.append((add_image(prepared(texture(pck, name), (82, 60), 'cover')), first, first + 1, first + 2))
            paths.append(name)
        cache, textures = {}, {}
        for name, source_path, size, fit in ALL_CHARACTER_UI:
            if source_path.startswith('#select:'):
                character = source_path.split(':')[1]; skeleton = 'characterselect_' + character
                sk = Skeleton(pck.read(pck.imported(skeleton + '.skel', '.spskel')))
                tex = load_textures(pck, skeleton, texture)
                layers = geometry(sk, *pose(sk, 'animation', 0), tex)
                image = raster(layers, 640 / 5120, np.array([0., 0.]), (640, 300))
                image = ImageOps.fit(image, (320, 240), Image.Resampling.LANCZOS, centering=(0.72, 0.5))
                paths.append('animations/character_select/' + character + '/' + skeleton + '.skel')
            elif source_path.startswith('#energy:'):
                character = source_path.split(':')[1]
                # Regent's original scene repeats layer 5 in Layer4/Layer5.
                layers = {'defect': (1, 2, 3, 4, 5), 'regent': (1, 2, 3, 5, 5), 'necrobinder': (1, 2, 3)}[character]
                names = [f'images/ui/combat/energy_counters/{character}/{character}_orb_layer_{i}.png' for i in layers]
                material = 'materials/ui/energy_orb_dark.tres' if character == 'regent' else None
                image = _layers(pck, names, material); paths.extend(names)
                if material: paths.append(material)
            elif source_path.startswith('#frame:'):
                _, color, kind = source_path.split(':')
                material = f'materials/cards/frames/card_frame_{color}_mat.tres'
                image = material_tint(pck, atlas(pck, f'@card/card_frame_{kind}_s', textures), material)
                paths.extend([f'@card/card_frame_{kind}_s', material])
            elif source_path == '#osty':
                frames, _ = bake(pck, 'osty', 'idle_loop', texture, frames=1, height=55, max_width=100, cache=cache)
                image = frames[0]; paths.append('animations/monsters/osty/osty.skel')
            elif source_path == '#stars':
                names = ['images/ui/combat/energy_star.png', 'images/ui/combat/energy_star_layer_2.png', 'images/ui/combat/energy_star_layer_3.png']
                image = _layers(pck, names); paths.extend(names)
            else:
                image = atlas(pck, source_path, textures) if source_path.startswith('@') else texture(pck, source_path)
                paths.append(source_path)
            pack.aux.append(add_image(prepared(image, size, fit)))
        for name, skeleton, clip, height, width, fps in NEW_CLIPS:
            print(f'Sampling original {skeleton}/{clip}', flush=True)
            if skeleton not in cache:
                sk = Skeleton(pck.read(pck.imported(skeleton + '.skel', '.spskel')))
                cache[skeleton] = sk, load_textures(pck, skeleton, texture)
            duration = cache[skeleton][0].animations[clip]['duration']
            samples = max(1, math.ceil(duration * fps))
            if samples > 256: raise ValueError('Animation frame directory bound exceeded')
            frames, meta = bake(pck, skeleton, clip, texture, frames=samples, height=height, max_width=width, cache=cache)
            first = len(pack.images)
            for image in frames:
                fmt, raw = compact_image(4, sparse_encode(image)); pack.images.append((*image.size, fmt, raw))
            pack.clips += struct.pack('<IHHhhI', first, len(frames), meta['frame_ms'], *meta['pivot'], 0)
            animation_meta.append(dict(id=BASE_CLIPS + len(animation_meta), enum='Anim' + name, first_image=first, **meta))
    finally:
        pck.close()
    alphabet = set(''.join(pack.strings))
    for folder in ('src', 'include'):
        for path in (ROOT / folder).rglob('*'):
            if path.suffix in ('.cpp', '.hpp', '.inc'): alphabet.update(path.read_text())
    for px in (8, 10, 12, 16, 20):
        font = ImageFont.truetype(str(font_path), px)
        for char in sorted(alphabet, key=ord):
            if ord(char) < 32 or (ord(char), px) in pack.glyphs: continue
            advance = max(1, math.ceil(font.getlength(char))); width = max(advance, font.getbbox(char)[2]); height = px + 5
            if width > 40 or advance > 40: raise ValueError('Glyph exceeds runtime bounds')
            bitmap = Image.new('L', (width, height)); ImageDraw.Draw(bitmap).text((0, px + 1), char, font=font, fill=255, anchor='ls')
            raw = bitmap.tobytes(); runs = rle(raw)
            pack.glyphs[ord(char), px] = (width, height, advance, len(runs), runs or raw)
    if (len(pack.cards), len(pack.strings), len(pack.aux), len(pack.clips) // 16) != (CARDS, STRINGS, AUXILIARY, CLIPS):
        raise ValueError('Incomplete five-character output profile')
    assert_retained(pack, prefix)
    data = pack.encode(13); decoded = PackData(data); assert_retained(decoded, prefix)
    if decoded.encode(13) != data: raise ValueError('Noncanonical five-character pack')
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + '.tmp'); temporary.write_bytes(data); temporary.replace(output)
    manifest = {'format': 'NSP2/13', 'reference': 'v0.111.0 / 41cef1ea', 'bytes': len(data),
        'sha256': hashlib.sha256(data).hexdigest(), 'cards': CARDS, 'single_player_pools': {
            'ironclad': 85, 'silent': 86, 'defect': 86, 'regent': 86, 'necrobinder': 86, 'colorless': 53},
        'generated_and_status_models': 15, 'potions': 64, 'relics': 33,
        'strings': len(pack.strings), 'auxiliary_images': len(pack.aux), 'images': len(pack.images),
        'glyphs': len(pack.glyphs), 'clips': CLIPS, 'resident_limit_bytes': LIMIT,
        'remaining_resident_bytes': LIMIT - len(data), 'retained_image_payloads': len(prefix[0]),
        'retained_card_records': BASE_CARDS, 'retained_strings': BASE_STRINGS,
        'retained_auxiliary_records': BASE_AUXILIARY, 'retained_animation_clips': BASE_CLIPS,
        'retained_glyph_payloads': len(prefix[5]), 'new_card_keys': [row['key'] for row in rows],
        'source_paths': paths, 'animations': animation_meta,
        'portrait_fallbacks': [], 'icon_adaptations': [{'status': 'HyperbeamFocus',
            'used': 'images/powers/focus_power.png', 'reason': 'Pinned PCK has no separate HyperbeamFocusDown power icon'}],
        'animation_sampling': 'Full original cycles; new idle 8 fps, actions 12 fps, camp 4 fps; historical clips unchanged',
        'excluded': ['audio', 'multiplayer', 'timeline'],
        'limitations': 'Offline Spine rasterization and material adaptation are not pixel parity with Godot. Content counts do not establish executable effect parity.'}
    output.with_name('content-manifest.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    print(json.dumps({key: value for key, value in manifest.items() if key not in ('source_paths', 'new_card_keys', 'animations')}, ensure_ascii=False), flush=True)
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path); parser.add_argument('source', type=Path)
    parser.add_argument('--output', type=Path, required=True); parser.add_argument('--font')
    args = parser.parse_args()
    build(args.game, args.source, args.output, choose_font(args.font))
