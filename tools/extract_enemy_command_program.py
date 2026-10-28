#!/usr/bin/env python3
"""Extract numeric await/trigger facts from the owner's pinned ILSpy input.

No assembly, original method bodies or assets are copied into the result.
The full public move directory is required; an unknown await fails extraction.
"""
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
KINDS = ['Wait', 'Trigger', 'Direct', 'Attack', 'Power', 'Remove', 'Block',
         'Heal', 'Cards', 'Spawn', 'SetHp', 'Steal', 'Kill', 'Escape',
         'Choice', 'Segments', 'Scale']
ALIVE, FIRST_SPEECH, UNCURLED, RESPAWN_ONE, RESPAWN_TWO, OFF_BALANCE, HAS_SLOT = 1, 2, 4, 8, 16, 32, 64


def balanced(source, begin, opening, closing):
    depth, quote, i = 1, None, begin + 1
    while i < len(source):
        c = source[i]
        if quote:
            if c == '\\':
                i += 2
                continue
            if c == quote:
                quote = None
        elif c in '\"\'':
            quote = c
        elif c == opening:
            depth += 1
        elif c == closing:
            depth -= 1
            if not depth:
                return i
        i += 1
    raise ValueError('Unbalanced source expression')


def method(source, name):
    match = re.search(r'\bTask(?:<[^>\n]+>)?\s+' + re.escape(name) + r'\s*\(', source)
    if not match:
        return None
    at = balanced(source, match.end() - 1, '(', ')') + 1
    while source[at].isspace():
        at += 1
    if source[at:at + 2] == '=>':
        return source[at + 2:source.index(';', at)]
    if source[at] != '{':
        raise ValueError(name)
    return source[at + 1:balanced(source, at, '{', '}')]


def awaits(body):
    result, at = [], 0
    while match := re.search(r'\bawait\s+', body[at:]):
        begin = at + match.end()
        i, depth, quote = begin, 0, None
        while i < len(body):
            c = body[i]
            if quote:
                if c == '\\':
                    i += 2
                    continue
                if c == quote:
                    quote = None
            elif c in '\"\'':
                quote = c
            elif c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            elif c == ';' and depth <= 0:
                break
            i += 1
        result.append((begin, body[begin:i].strip()))
        at = i + 1
    return result


def milliseconds(number):
    return round(float(number.rstrip('fm')) * 1000)


def extract(source_root):
    directory = json.loads((ROOT / 'config/enemy-visuals-0.17.json').read_text())
    models = {m['model']: m['id'] for m in directory['models']}
    enum = (ROOT / 'include/enemy_state.inc').read_text().split('enum class EnemyPower:uint8_t {', 1)[1].split('Count', 1)[0]
    powers = {name.strip(): i for i, name in enumerate(enum.split(',')) if name.strip()}
    triggers, rows, source_cache = [], [], {}

    def source(name):
        if name not in source_cache:
            source_cache[name] = (source_root / 'MegaCrit.Sts2.Core.Models.Monsters' / (name + '.cs')).read_text()
        return source_cache[name]

    def step(kind, arg=0, normal=0, fast=None, flags=0):
        return [KINDS.index(kind), flags, arg, normal, normal if fast is None else fast]

    def trigger(name, normal=0, fast=None, flags=0, direct=False):
        if name not in triggers:
            triggers.append(name)
        if fast is None:
            fast = min(normal // 2, 250)
        return step('Direct' if direct else 'Trigger', triggers.index(name), normal, fast, flags)

    def power(name, flags=0):
        return step('Power', powers[name], flags=flags)

    def remove(name, flags=0):
        return step('Remove', powers[name] if name else 65535, flags=flags)

    for model in directory['models']:
        name = model['model']
        # The unpublished old Nibbit adapter is not an original creature and
        # cannot be created by the original encounter runtime. Never count its
        # duplicate rows as covered source moves or ship a timing fallback.
        if name == 'LegacyNibbit':
            continue
        owner = 'FlailKnight' if name == 'MysteriousKnight' else 'DecimillipedeSegment' if name.startswith('DecimillipedeSegment') else name
        for move in model['moves']:
            body = method(source(owner), move['method'])
            if body is None:
                if name in ('Architect', 'BattleFriendV1', 'BattleFriendV2', 'BattleFriendV3'):
                    body = ''
                else:
                    raise ValueError((name, move['method'], 'missing method'))
            items = awaits(body)
            if name == 'Vantom':
                items += [(m.start(), 'Direct:' + m.group(1)) for m in re.finditer(r'\.SetAnimation\("(_tracks/[^\"]+)", loop: false, 1\)', body)]
            items.sort()
            program = []
            for offset, expression in items:
                text = expression.replace('\n', ' ')
                alive = ALIVE if name == 'Vantom' and move['node'] != 2 and 'if (TestMode.IsOff && base.Creature.IsAlive)' in body[:offset] else 0
                if text.startswith('Direct:'):
                    program.append(trigger(text[7:], flags=ALIVE if name == 'Vantom' else alive, direct=True))
                elif text.startswith('DamageCmd.Attack'):
                    anim = re.search(r'\.WithAttackerAnim\("([^\"]+)"\s*,\s*([\d.]+f?)', text)
                    if '.WithNoAttackerAnim()' not in text:
                        program.append(trigger(anim[1] if anim else 'Attack'))
                    program.append(step('Attack'))
                elif text.startswith('CreatureCmd.TriggerAnim'):
                    match = re.search(r'TriggerAnim\(.*?,\s*"([^\"]+)"\s*,\s*([\d.]+f?)', text)
                    if not match:
                        raise ValueError((name, text))
                    flags = UNCURLED if name == 'LouseProgenitor' and match[1] == 'Uncurl' else alive
                    program.append(trigger(match[1], milliseconds(match[2]), flags=flags))
                elif text.startswith('Cmd.CustomScaledWait'):
                    numbers = re.search(r'\(([\d.]+f?)\s*,\s*([\d.]+f?)\)', text)
                    flags = FIRST_SPEECH if name == 'GremlinMerc' and move['node'] == 0 else alive
                    program.append(step('Wait', normal=milliseconds(numbers[2]), fast=milliseconds(numbers[1]), flags=flags))
                elif text.startswith('Cmd.Wait'):
                    match = re.search(r'\(([\d.]+f?)\)', text)
                    if not match:
                        raise ValueError((name, text))
                    flags = HAS_SLOT if name == 'TwoTailedRat' and move['method'] == 'CallForBackup' else alive
                    program.append(step('Wait', normal=milliseconds(match[1]), flags=flags))
                elif text.startswith('PowerCmd.Apply<'):
                    pname = re.search(r'Apply<(\w+)Power>', text)[1]
                    program.append(power(pname))
                elif text.startswith('PowerCmd.Apply('):
                    program.append(power({'MagiKnight': 'Dampen', 'TheInsatiable': 'Sandpit', 'ThievingHopper': 'Swipe'}[name]))
                elif text.startswith('PowerCmd.Remove'):
                    match = re.search(r'Remove<(\w+)Power>', text)
                    program.append(remove(match[1] if match else None))
                elif text.startswith('CreatureCmd.GainBlock'):
                    program.append(step('Block'))
                elif text.startswith(('CreatureCmd.Heal', 'Revive(')):
                    program.append(step('Heal'))
                elif text.startswith(('CardPileCmd.AddToCombatAndPreview<', 'CardPileCmd.AddGeneratedCardToCombat')):
                    match = re.search(r'PileType\.(\w+)', text)
                    pile = {'Draw': 0, 'Hand': 1, 'Discard': 2, 'Exhaust': 3}.get(match[1], 65535) if match else 65535
                    wait = (100 if pile == 1 else 1000) if text.startswith('CardPileCmd.AddToCombatAndPreview<') else 0
                    program.append(step('Cards', pile, wait))
                elif text.startswith('CreatureCmd.Add<'):
                    program.append(step('Spawn', models[re.search(r'Add<(\w+)>', text)[1]]))
                elif text.startswith(('SpawnDefensiveBot', 'SpawnAggroBot')):
                    program += [step('Spawn', 65535), power('Minion')]
                elif text.startswith('ShowDialogueForMove'):
                    program.append(step('Wait', normal=500))
                elif text.startswith('(Background?.'):
                    duration = re.search(r',?\s*([\d.]+f?)\)\s*\?\?', text)
                    if not duration or len(move['direct_clips']) != 1:
                        raise ValueError((name, text, move['direct_clips']))
                    wait = milliseconds(duration[1])
                    program.append(trigger(move['direct_clips'][0], wait, wait, direct=True))
                elif text.startswith('AnimSegmentsAttack'):
                    program.append(step('Segments', normal=500))
                elif 'GetPower<ReattachPower>().DoReattach()' in text:
                    program.append(step('Heal'))
                elif text.startswith('Hatch('):
                    program += [trigger('Hatch', 500), step('SetHp')]
                elif text.startswith('Stun('):
                    program.append(trigger('Stun', 600, flags=OFF_BALANCE))
                elif text.startswith('CreatureCmd.Escape'):
                    program.append(step('Escape'))
                elif text.startswith('CreatureCmd.Kill'):
                    program.append(step('Kill'))
                elif text.startswith('Task.WhenAll') and name == 'KnowledgeDemon':
                    program.append(step('Choice'))
                elif text.startswith('CardPileCmd.RemoveFromCombat'):
                    program.append(step('Steal'))
                elif text.startswith(('swipe.Steal(', 'powerInstance.Steal(')):
                    pass  # RemoveFromDeck(showPreview:false)/LoseGold have no wait.
                else:
                    raise ValueError((name, move['node'], 'unknown await', text))
            # Branches and repeated mixed-pile creation are resolved by the
            # actual command, never by a guessed total animation duration.
            if name == 'TestSubject' and move['node'] == 0:
                program = [trigger('RespawnTrigger'), step('Wait', normal=800), step('Scale'),
                           step('Wait', normal=1150), step('Heal'), power('PainfulStabs', RESPAWN_ONE),
                           power('Nemesis', RESPAWN_TWO), remove('Adaptable', RESPAWN_TWO), remove('PainfulStabs', RESPAWN_TWO)]
            if name == 'TheInsatiable' and move['node'] == 0:
                program = [trigger('Liquify', 0), step('Wait', normal=500), step('Wait', normal=750),
                           power('Sandpit'), step('Cards', 0), step('Cards', 2), step('Wait', normal=1000)]
                # Use the directory's exact authored trigger, whose casing
                # is deliberately not inferred from the method's name.
                program[0] = trigger(move['triggers'][0])
            rows.append({'model': name, 'node': move['node'], 'method': move['method'],
                         'key': model['id'] * 32 + move['node'], 'steps': program})
    return directory['reference'], triggers, rows


def write_header(path, reference, triggers, rows):
    pool, spans, dedup = [], [], {}
    for row in rows:
        signature = tuple(tuple(s) for s in row['steps'])
        if signature not in dedup:
            dedup[signature] = len(pool)
            pool.extend(row['steps'])
        spans.append((row['key'], dedup[signature], len(signature), row))
    lines = ['#pragma once', '#include <stdint.h>', 'namespace spire { namespace enemy_command_program {',
             '// Numeric command facts from v0.111.0 /41cef1ea; generated by the pinned extractor.',
             'enum Kind:uint8_t {' + ','.join(KINDS) + '};',
             'enum Condition:uint8_t {Alive=1,FirstSpeech=2,Uncurl=4,RespawnOne=8,RespawnTwo=16,OffBalance=32,HasSlot=64};',
             'struct Step {uint8_t kind,flags;uint16_t arg,normal_ms,fast_ms;};',
             'struct Row {uint16_t key,first;uint8_t count;};',
             'static constexpr const char* triggers[]={' + ','.join(json.dumps(t) for t in triggers) + '};',
             'static constexpr Step steps[]={']
    lines += [' {' + ','.join(map(str, s)) + '},' for s in pool]
    lines += ['};', 'static constexpr Row rows[]={']
    lines += [' {%d,%d,%d}, // %s.%s' % (key, first, count, row['model'], row['method']) for key, first, count, row in sorted(spans)]
    lines += ['};', 'inline const Row* find(unsigned model,int move){',
              ' if(move<0||move>=32)return nullptr;const unsigned key=model*32u+unsigned(move);',
              ' unsigned lo=0,hi=sizeof(rows)/sizeof(rows[0]);while(lo<hi){const unsigned mid=(lo+hi)/2;if(rows[mid].key<key)lo=mid+1;else hi=mid;}',
              ' return lo<sizeof(rows)/sizeof(rows[0])&&rows[lo].key==key?rows+lo:nullptr;', '}', '}}', '']
    path.write_text('\n'.join(lines))
    return {'moves': len(rows), 'nonempty': sum(bool(r['steps']) for r in rows), 'steps': len(pool), 'triggers': len(triggers), 'reference': reference}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'include/enemy_command_program.hpp')
    parser.add_argument('--audit', type=Path)
    args = parser.parse_args()
    reference, triggers, rows = extract(args.source_root)
    result = write_header(args.output, reference, triggers, rows)
    if args.audit:
        args.audit.write_text(json.dumps({'summary': result, 'triggers': triggers, 'moves': rows}, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
