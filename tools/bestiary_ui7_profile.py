#!/usr/bin/env python3
"""Extract original bestiary identifiers, gates and move selectors only.

The caller supplies private decompiled reference sources and a PCK. Generated
files contain no original prose, scene text, image payloads or game methods.
"""
from pathlib import Path
import argparse
import json
import re

ROOT = Path(__file__).resolve().parents[1]


def block(text, marker):
    at = text.index(marker) + len(marker)
    brace, arrow = text.find('{', at), text.find('=>', at)
    if arrow >= 0 and (brace < 0 or arrow < brace):
        at = arrow + 2
    else:
        at = brace
    depth = 0
    for end in range(at, len(text)):
        char = text[end]
        if char == '{': depth += 1
        elif char == '}':
            depth -= 1
            if depth == 0: return text[at:end + 1]
        elif char == ';' and not depth: return text[at:end + 1]
    raise ValueError('Unterminated reference member: ' + marker)


def unique(values):
    return list(dict.fromkeys(values))


def build(reference, pck):
    from pck import Pack
    models = json.loads((ROOT/'config/enemy-visuals-0.17.json').read_text())['models']
    by_name = {m['model']: m for m in models}
    encounters = re.search(r'enum class EncounterId[^}]+', (ROOT/'include/enemy_state.inc').read_text()).group()
    encounter_names = re.findall(r'\b([A-Z][A-Za-z0-9]+)\s*[,\n]', encounters.split('{', 1)[1])
    encounter_ids = {name: i for i, name in enumerate(encounter_names)}
    pack = Pack(pck)
    try:
        locale = json.loads(pack.read('localization/eng/monsters.json'))
    finally: pack.close()
    monster_dir = reference/'MegaCrit.Sts2.Core.Models.Monsters'
    encounter_dir = reference/'MegaCrit.Sts2.Core.Models.Encounters'
    acts = ('Overgrowth', 'Underdocks', 'Hive', 'Glory')
    entries, all_moves, model_rows = [], [], []

    def add_encounter(name, region, seen):
        source = (encounter_dir/(name+'.cs')).read_text()
        member = block(source, 'AllPossibleMonsters')
        names = re.findall(r'ModelDb\.Monster<([A-Za-z0-9]+)>', member)
        # Getter arrays live above the property; only explicitly referenced
        # MonsterModel array declarations are included, in declaration order.
        for array in unique(re.findall(r'\b_[a-zA-Z0-9]+\b', member)):
            match = re.search(r'MonsterModel\[\]\s+'+re.escape(array)+r'\s*=', source)
            if match: names += re.findall(r'ModelDb\.Monster<([A-Za-z0-9]+)>', block(source, match.group()))
            elif name == 'RubyRaidersNormal' and array == '_raiderValidCounts':
                names += re.findall(r'ModelDb\.Monster<([A-Za-z0-9]+)>', block(source, 'static RubyRaidersNormal()'))
        names = unique(names)
        if not names: raise ValueError('No audited monster identifiers: '+name)
        room = re.search(r'RoomType\s*=>\s*RoomType\.(\w+)', source)
        kind = {'Monster': 1, 'Elite': 2, 'Boss': 3}.get(room.group(1) if room else '', 1)
        for model in names:
            if model in seen or model.startswith('DecimillipedeSegment'): continue
            seen.add(model)
            if model not in by_name: raise ValueError('Missing executable enemy: '+model)
            entries.append(dict(model=by_name[model]['id'], encounter=encounter_ids[name], region=region, room=kind))

    for region, act in enumerate(acts):
        source = (reference/'MegaCrit.Sts2.Core.Models.Acts'/(act+'.cs')).read_text()
        names = re.findall(r'ModelDb\.Encounter<([A-Za-z0-9]+)>', block(source, 'GenerateAllEncounters()'))
        seen = set()
        for name in names: add_encounter(name, region, seen)
        if act == 'Hive': entries.append(dict(model=255, encounter=encounter_ids['DecimillipedeElite'], region=region, room=2))
    source = (reference/'MegaCrit.Sts2.Core.Models'/'ModelDb.cs').read_text()
    event_member = source.split(' EventEncounters => ', 1)[1].split('public static', 1)[0]
    seen = set()
    for name in re.findall(r'Encounter<([A-Za-z0-9]+)>', event_member): add_encounter(name, 4, seen)

    inserted = {
        'BowlbugRock': [(1, 'monsters.GENERIC.moves.STUNNED.title', 'stun')],
        'CeremonialBeast': [(2, 'monsters.GENERIC.moves.STUNNED.title', 'stun')],
        'Tunneler': [(3, 'monsters.GENERIC.moves.STUNNED.title', 'stun')],
        'LagavulinMatriarch': [(0, 'monsters.LAGAVULIN_MATRIARCH.moves.SLEEP.title', 'sleep_loop'), (1, 'monsters.LAGAVULIN_MATRIARCH.moves.WAKE_UP.title', 'wake_up')],
        'TheInsatiable': [(4, 'monsters.THE_INSATIABLE.moves.DEVOUR.title', 'eat_player')],
    }
    for model in models:
        first = len(all_moves)
        if model['id'] == 0:
            model_rows.append(dict(first=first, count=0));continue
        source = (monster_dir/(model['model']+'.cs')).read_text()
        if not source.strip():
            selected = reference.parent/'reference-selected'/('MegaCrit.Sts2.Core.Models.Monsters.'+model['model']+'.decompiled.cs')
            source = selected.read_text()
        if not source.strip(): raise ValueError('Empty original model: '+model['model'])
        omitted = set()
        if 'ShouldShowMoveInBestiary(string moveStateId)' in source:
            omitted.update(re.findall(r'moveStateId != "([A-Z0-9_]+)"', block(source, 'ShouldShowMoveInBestiary(string moveStateId)')))
        if 'GenerateBestiaryMoveList(NCreatureVisuals?' in source:
            member = block(source, 'GenerateBestiaryMoveList(NCreatureVisuals? creatureVisuals)')
            omitted.update(re.findall(r'(?:m\.)?stateId == "([A-Z0-9_]+)"', member))
        moves = []
        for move in model['moves']:
            state = move['key']
            if state in omitted: continue
            name = state.removesuffix('_MOVE')
            key = model['key']+'.moves.'+name+'.title'
            if key not in locale and name.endswith(('2', '3', '4')): continue
            moves.append(dict(label='monsters.'+key if key in locale else state, node=move['node'], clip=None))
        names = {clip['name'] for clip in model['clips']}
        for clip in ('revive', 'hurt', 'die'):
            if clip in names: moves.append(dict(label='bestiary.ACTION_NAME.'+clip, node=-1, clip=clip))
        for index, label, clip in inserted.get(model['model'], []):
            moves.insert(index, dict(label=label, node=-1, clip=clip))
        if model.get('shared_group'):
            side = 'left' if model['model'] == 'Crusher' else 'right'
            for action in ('hurt', 'die'): moves.append(dict(label='bestiary.ACTION_NAME.'+action, node=-1, clip=side+'/'+action))
        all_moves += moves
        model_rows.append(dict(first=first, count=len(moves)))
    first = len(all_moves)
    for key, clip, node in [('WRITHE', 'idle_loop', 0), ('REATTACH', 'regenerate', -1), ('DEAD', 'wither', -1)]:
        all_moves.append(dict(label='monsters.DECIMILLIPEDE_SEGMENT.moves.'+key+'.title', node=node, clip=clip))
    return dict(schema=1, reference='v0.111.0 / 41cef1ea',
        sources=['NBestiary.CreateEntries/AddAct/AddEvents', 'MonsterModel.GenerateBestiaryMoveList', 'NBestiaryEntry._Ready', 'NBestiaryLayoutKaiserCrab.Setup', 'NBestiaryLayoutDecimillipede.Setup'],
        notes=['Model ids are EnemyId values, never battle slots.', 'Runtime sorts names using the selected private locale.', 'Disabled unknown entries and hidden Epithet remain hidden/disabled.', 'Moves expose real animator clips; command-specific VFX are separately scoped.'],
        entries=entries, models=model_rows, decimillipede=dict(first=first, count=3), moves=all_moves)


def generated(data):
    quote = lambda value: 'nullptr' if value is None else json.dumps(value)
    text = '// Generated by bestiary_ui7_profile.py; identifiers and numeric selectors only.\n'
    text += 'static constexpr BestiaryEntrySpec bestiary_entries[]={\n'
    text += ''.join(' {'+','.join(str(row[key]) for key in ('model','encounter','region','room'))+'},\n' for row in data['entries'])+'};\n'
    text += 'static constexpr BestiaryModelSpec bestiary_models[]={\n'
    text += ''.join(' {'+str(row['first'])+','+str(row['count'])+'},\n' for row in data['models'])+'};\n'
    text += 'static constexpr BestiaryModelSpec bestiary_decimillipede={' +str(data['decimillipede']['first'])+','+str(data['decimillipede']['count'])+'};\n'
    text += 'static constexpr BestiaryMoveSpec bestiary_moves[]={\n'
    text += ''.join(' {'+quote(row['label'])+','+quote(row['clip'])+','+str(row['node'])+'},\n' for row in data['moves'])+'};\n'
    return text


if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--reference-root',type=Path,required=True);parser.add_argument('--pck',type=Path,required=True);args=parser.parse_args()
    result=build(args.reference_root,args.pck)
    (ROOT/'config/bestiary-ui7-0.17.json').write_text(json.dumps(result,indent=2)+'\n')
    (ROOT/'src/bestiary_catalog_ui7.inc').write_text(generated(result))
    print(json.dumps(dict(entries=len(result['entries']),models=len(result['models']),moves=len(result['moves']))))
