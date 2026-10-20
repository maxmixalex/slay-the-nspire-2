"""Public original-enemy animation directory and bounded animator tables.

Only keys, numeric rules and independent conversion logic live here. Skin choices
retain their original multiplicities. Every used skeleton clip is reachable;
role aliases never replace the original states or move-specific track sequences.
"""
from __future__ import annotations
from collections import OrderedDict
from functools import lru_cache
from itertools import product
import argparse
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE_CLIPS = 38
FRAME_LIMIT = 256


def eq(fact, value):
    return dict(fact=fact, op='eq', value=int(value))


def conjunction(*conditions):
    rows = [row for row in conditions if row is not None]
    return None if not rows else rows[0] if len(rows) == 1 else {'all': rows}


def skins(model):
    """Distinct rendered composites, with the original RNG choice weight."""
    spec = model['skins']
    absent = set(spec.get('unavailable', []))
    choices = OrderedDict()
    if spec['mode'] == 'random_composite':
        for selected in product(*spec['groups']):
            rendered = tuple(s for s in selected if s not in absent)
            choices.setdefault(rendered, dict(weight=0, slot_mask=0))['weight'] += 1
    elif spec['mode'] == 'named_slot':
        for variant in spec['variants']:
            rendered = tuple(variant['skins'])
            item = choices.setdefault(rendered, dict(weight=1, slot_mask=0))
            for slot in variant['slots']:
                item['slot_mask'] |= 1 << slot
    else:
        choices[tuple(spec['default'])] = dict(weight=1, slot_mask=0)
    default = tuple(s for s in spec['default'] if s not in absent)
    if default not in choices:
        raise ValueError('Default skin is not an original choice: ' + model['model'])
    order = [default] + [key for key in choices if key != default]
    return [dict(key='+'.join(key) or 'empty', skins=list(key), **choices[key]) for key in order]


def states(model):
    if not model.get('shared_group'):
        return [dict(row) for row in model['states']]
    # The shared actor uses direct AnimationState tracks instead of
    # CreatureAnimator. Expose those same literal states to the UI.
    clips = [c['name'] for c in model['clips'] if c['used']]
    result = []
    for i, clip in enumerate(clips):
        loop = clip.endswith('_loop')
        following = next((move.get('next_clip') for move in model['moves']
                          if clip in move.get('direct_clips', [])), None)
        if '/hurt' in clip:
            following = ('left/idle_loop' if clip.startswith('left/') else
                         'right/charged_loop' if 'charged' in clip else
                         'right/rest_loop' if 'resting' in clip else 'right/idle_loop')
        result.append(dict(id=i, clip=clip, loop=loop,
                           next=clips.index(following) if following in clips else None))
    return result


def transitions(model, model_states):
    if not model.get('shared_group'):
        return [dict(row) for row in model['transitions']]
    result = []
    by_clip = {s['clip']: s['id'] for s in model_states}
    for role, trigger in [('idle', 'Idle'), ('attack', 'Attack'), ('cast', 'Cast'),
                          ('hurt', 'Hit'), ('die', 'Dead'), ('revive', 'Revive')]:
        entry = model['entries'][role]
        if entry['playback'] == 'hold_current':
            continue
        for variant in entry['variants'] or [dict(clip=entry['clip'], when=None)]:
            condition = variant.get('when')
            if condition and isinstance(condition.get('value'), str):
                condition = dict(condition, value=['default', 'charging', 'resting'].index(condition['value']))
            result.append(dict(trigger=trigger, state=by_clip[variant['clip']],
                               **{'from': 'any'}, when=condition))
    result.append(dict(trigger='CombatOver', state=by_clip['body/die'],
                       **{'from': 'any'}, when=None))
    return result


def is_loop(model, clip):
    # Prefix variants use the same looping flags as their original state.
    plain = clip.removeprefix('_no_bag/')
    return (any(s['clip'] == plain and s['loop'] for s in model['states']) or
            any(t['clip'] == clip and t['loop'] for t in model['tracks']) or
            (model.get('shared_group') and clip.endswith('_loop')))


def phases(model):
    axes = [(row['fact'], sorted(map(int, row['values']))) for row in model.get('phase_tracks', [])]
    if model['model'] == 'SewerClam':
        axes.append(('coral_growth_step', list(range(8))))
    if model['model'] == 'Vantom':
        axes.append(('Shrink', [0, 1]))
    if model.get('shared_group'):
        axes.extend([('kaiser_left_dead', [0, 1]), ('kaiser_right_dead', [0, 1]), ('right_arm_state', [0, 1, 2])])
    return [dict(zip([a[0] for a in axes], values)) for values in product(*(a[1] for a in axes))]


def shared_setup(model, clip, facts, move):
    catalog = {row['name']: row for row in model['clips']}
    right = ['right/idle_loop', 'right/charged_loop', 'right/rest_loop'][facts['right_arm_state']]
    left = 'left/die' if facts['kaiser_left_dead'] else 'left/idle_loop'
    if facts['kaiser_right_dead']:
        right = 'right/die_resting' if facts['right_arm_state'] == 2 else 'right/die'
    setup = [dict(track=0, clip='body/idle_loop', loop=True)]
    for track, name, dead in [(1, left, facts['kaiser_left_dead']), (2, right, facts['kaiser_right_dead'])]:
        row = dict(track=track, clip=name, loop=not dead)
        if dead:
            # Replaying the other arm's death from time zero would resurrect it
            # whenever this arm attacks. Preserve its terminal original pose.
            row['time'] = catalog[name]['duration']
        setup.append(row)
    track = 0 if clip.startswith('body/') else 1 if clip.startswith('left/') else 2 if clip.startswith('right/') else 3
    post = []
    if move and move.get('reaction'):
        post = [dict(track=3, clip=move['reaction'], loop=False)]
    elif '/hurt' in clip:
        side = 'left' if clip.startswith('left/') else 'right'
        post = [dict(track=3, clip='reactions/hurt_' + side, loop=False)]
    return dict(shared_background=True, setup_tracks=setup, tracks=post, track=track)


def contexts(model, clip, facts):
    """Ordinary context followed by the exact move-specific composites."""
    common = dict(facts=facts)
    if model['model'] == 'SewerClam':
        common['coral_scale'] = min(1.5, .1 + .2*facts['coral_growth_step'])
    if model['model'] == 'Vantom':
        common['shrink'] = bool(facts['Shrink'])
    if model.get('shared_group'):
        common.update(shared_setup(model, clip, facts, None))
    result = [(-1, common)]
    for move in model['moves']:
        additional = False
        if model.get('shared_group'):
            additional = clip in move.get('direct_clips', [])
        elif model['model'] == 'Vantom':
            additional = any(t['method'] == move['method'] and clip in t.get('applies_to_clips', []) for t in model['tracks'])
        elif model['model'].startswith('DecimillipedeSegment'):
            additional = move['node'] < 3 and clip in ('idle_loop', 'dead_loop')
        if not additional:
            continue
        context = dict(common, move=move['key'])
        if model.get('shared_group'):
            context.update(shared_setup(model, clip, facts, move))
        if model['model'] == 'Vantom' and clip == 'charge_up':
            catalog = {row['name']: row for row in model['clips']}
            tail = next(t for t in model['tracks'] if t['method'] == move['method']
                        and t['clip'].startswith('_tracks/charge_up_'))
            # The body returns to idle before the second/third tail extension
            # ends. Bake that real base-track transition, then retain the full
            # overlay period instead of cutting it at the body's 2/3 second.
            tracks = [dict(track=0, clip='idle_loop', loop=True, queued=True)]
            delay = .15 if move['method'] == 'InkBlotMove' else 0.
            if delay:
                # InkBlot awaits CHARGE_UP for .15s before SetAnimation on the
                # tail. Restore the prior phase before this delayed override.
                phase = next(p for p in model['phase_tracks'] if p['fact'] == 'tail_charge')
                tracks.extend([
                    dict(track=1, **phase['values'][str(facts['tail_charge'])]),
                    dict(track=1, clip=tail['clip'], loop=False, queued=True, delay=delay),
                    dict(track=1, clip='_tracks/charged_2', loop=True, queued=True),
                ])
            context['tracks'] = tracks
            context['duration_seconds'] = max(catalog[clip]['duration'], delay + catalog[tail['clip']]['duration'])
        if model['model'].startswith('DecimillipedeSegment'):
            context['decimillipede_strike'] = True
            context['duration_seconds'] = 1.25
        result.append((move['node'], context))
    return result


@lru_cache(maxsize=1)
def plan():
    source = json.loads((ROOT / 'config/enemy-visuals-0.17.json').read_text())
    models = source['models']
    if [r['id'] for r in models] != list(range(109)):
        raise ValueError('Original enemy id directory changed')
    jobs = []
    for model in models:
        variants = skins(model)
        for original in model['clips']:
            if not original['used']:
                continue
            clip = original['name']
            looping = bool(is_loop(model, clip))
            for skin, skin_row in enumerate(variants):
                for facts in phases(model):
                    for move, context in contexts(model, clip, facts):
                        duration = float(context.get('duration_seconds', original['duration']))
                        if model['model'] == 'SewerClam':
                            grows = [e['time'] for e in original.get('events', []) if e['name'] == 'grow']
                            if grows:
                                duration = max(duration, max(grows) + 1.)
                                context['duration_seconds'] = duration
                        if not 0 <= duration <= 120:
                            raise ValueError('Original clip outside bounded sampler duration')
                        frames = max(1, min(FRAME_LIMIT, math.ceil(duration * (8 if looping else 12))))
                        if duration*1000/frames > 1000:
                            raise ValueError('Original clip cannot fit timing directory')
                        suffix = []
                        if len(variants) > 1:
                            suffix.append('s' + str(skin))
                        suffix.extend(name + '=' + str(value) for name, value in facts.items())
                        if move >= 0:
                            suffix.append('m' + str(move))
                        key = model['model'] + '.' + clip + (('@' + ','.join(suffix)) if suffix else '')
                        jobs.append(dict(key=key, model=model['model'], model_id=model['id'],
                            clip=clip, skin=skin, skins=skin_row['skins'], move=move,
                            when=conjunction(*(eq(k, v) for k, v in facts.items())), loop=looping,
                            duration=duration, original_duration=original['duration'], frames=frames,
                            visual_state=context))
    if len({row['key'] for row in jobs}) != len(jobs) or len(jobs)+BASE_CLIPS > 65535:
        raise ValueError('Duplicate or overflowing original clip directory')
    for i, row in enumerate(jobs, BASE_CLIPS):
        row['id'] = i
    return dict(models=models, jobs=jobs, used_clips=sum(c['used'] for r in models for c in r['clips']))


def _q(value):
    return 'nullptr' if value is None else json.dumps(value, ensure_ascii=True)


def generated_cpp():
    """One translation-unit include. Struct declarations are a stable API."""
    data = plan()
    conditions = [(None, 0, 0, 0, 0)]
    condition_ids = {'null': 0}
    facts = set()
    opcodes = dict(eq=1, ne=2, lt=3, le=4, gt=5, ge=6)
    def cond(value):
        key = json.dumps(value, sort_keys=True)
        if key in condition_ids:
            return condition_ids[key]
        if 'fact' in value:
            facts.add(value['fact'])
            row = (value['fact'], int(value['value']), 0, 0, opcodes[value['op']])
        else:
            kind = 'all' if 'all' in value else 'any'
            children = [cond(c) for c in value[kind]]
            if not children:
                raise ValueError('Empty animator condition')
            current = children[0]
            for child in children[1:]:
                row = (None, 0, current, child, 7 if kind == 'all' else 8)
                conditions.append(row)
                current = len(conditions)-1
            condition_ids[key] = current
            return current
        conditions.append(row)
        condition_ids[key] = len(conditions)-1
        return len(conditions)-1
    models_out, states_out, transitions_out, moves_out, actions_out = [], [], [], [], []
    skins_out, clips_out, initial_out, resume_out, prefixes_out = [], [], [], [], []
    for model in data['models']:
        mid = model['id']
        ss = states(model)
        ts = transitions(model, ss)
        if [s['id'] for s in ss] != list(range(len(ss))):
            raise ValueError('Animator states are not dense')
        initial = next((s['id'] for s in ss if s['clip'] == model['initial_clip']), -1)
        if initial < 0:
            raise ValueError('Initial animator state missing: ' + model['model'])
        starts = [len(a) for a in (states_out, transitions_out, moves_out, skins_out, clips_out)]
        states_out.extend((s['clip'], s.get('bounds'), -1 if s['next'] is None else s['next'], int(s['loop'])) for s in ss)
        transitions_out.extend((t['trigger'], -1 if t['from'] == 'any' else t['from'], t['state'], cond(t['when'])) for t in ts)
        for move in model['moves']:
            actions = [(trigger, 0) for trigger in move['triggers']]
            if model.get('shared_group'):
                actions.extend((name, 1) for name in move['direct_clips'])
            if model['model'].startswith('DecimillipedeSegment') and move['node'] < 3:
                # SegmentAttack animates external Drive nodes. It has no
                # registered Attack state and must not invent one.
                actions = [('idle_loop', 1)]
            moves_out.append((move['key'], move.get('next_clip'), move['node'], len(actions_out), len(actions)))
            actions_out.extend(actions)
        for skin in skins(model):
            skins_out.append((skin['key'], skin['weight'], skin['slot_mask']))
        for job in data['jobs']:
            if job['model_id'] == mid:
                clips_out.append((job['clip'], job['key'], job['id'], cond(job['when']), job['move'],
                                  max(1,round(job['duration']*1000)),max(1,round(job['original_duration']*1000)),
                                  job['skin'], int(job['loop'] and not job['visual_state'].get('decimillipede_strike'))))
        ends = [len(a) for a in (states_out, transitions_out, moves_out, skins_out, clips_out)]
        models_out.append((model['model'], *[value for pair in zip(starts, [e-s for s,e in zip(starts,ends)]) for value in pair],
                           initial, int(bool(model.get('shared_group'))), {'left': 1, 'right': 2}.get(model.get('side'), 0)))
        for row in model.get('initial_variants', []):
            state = next(s['id'] for s in ss if s['clip'] == row['clip'])
            initial_out.append((mid, state, cond(row['when'])))
        # Load/resume chooses an existing persistent loop. Original conditional
        # Hit transitions usually identify that loop; never replay spawn/death.
        def stable(state):
            visited = set()
            while state >= 0 and state not in visited:
                visited.add(state)
                s = ss[state]
                if s['loop']:
                    return state
                state = -1 if s['next'] is None else s['next']
            return -1
        extra = []
        if model['model'] == 'LagavulinMatriarch':
            extra = [('sleep_loop', eq('IsAwake', 0)), ('idle_loop', eq('IsAwake', 1))]
        elif model['model'] == 'Tunneler':
            extra = [('hidden_loop', eq('Burrowed', 1))]
        elif model['model'] == 'CubexConstruct':
            extra = [('burrowed_loop', eq('IsBurrowed', 1))]
        elif model['model'].startswith('DecimillipedeSegment'):
            extra = [('dead_loop', eq('EnemyDown', 1))]
        elif model['model'] == 'Rocket':
            extra = [('right/charged_loop', eq('right_arm_state', 1)), ('right/rest_loop', eq('right_arm_state', 2))]
        for clip, condition in extra:
            resume_out.append((mid, next(s['id'] for s in ss if s['clip'] == clip), cond(condition)))
        seen = set()
        for t in ts:
            if t['trigger'] == 'Hit' and t['when']:
                target = stable(t['state'])
                condition = cond(t['when'])
                if target >= 0 and (target, condition) not in seen:
                    resume_out.append((mid, target, condition)); seen.add((target, condition))
        target = stable(initial)
        resume_out.append((mid, target if target >= 0 else initial, 0))
        for row in model.get('clip_prefix_variants', []):
            prefixes_out.append((mid, row['prefix'], cond(row['when'])))
    if max(map(len, (models_out, states_out, transitions_out, moves_out, actions_out, skins_out, clips_out, conditions))) > 65535:
        raise ValueError('Animator table index exceeds uint16')
    out = ['// Generated by world_ui_profile.py. Keys, numbers and independent lookup logic only.\n',
           'namespace {\nstruct WECondition {const char* fact;int value;uint16_t left,right;uint8_t op;};\n',
           'struct WESelect {uint16_t model;int16_t state;uint16_t condition;};\n',
           'struct WEPrefix {uint16_t model;const char* prefix;uint16_t condition;};\n']
    def table(kind, name, rows):
        out.append('static const ' + kind + ' ' + name + '[]={\n')
        for row in rows:
            out.append(' {' + ','.join(_q(v) if isinstance(v, str) or v is None else str(v) for v in row) + '},\n')
        out.append('};\n')
    table('WorldEnemyVisualModel', 'we_models', models_out)
    table('WorldEnemyVisualState', 'we_states', states_out)
    table('WorldEnemyVisualTransition', 'we_transitions', transitions_out)
    table('WorldEnemyVisualMove', 'we_moves', moves_out)
    table('WorldEnemyVisualAction', 'we_actions', actions_out)
    table('WorldEnemyVisualSkin', 'we_skins', skins_out)
    table('WorldEnemyVisualClip', 'we_clips', clips_out)
    table('WECondition', 'we_conditions', conditions)
    table('WESelect', 'we_initial', initial_out)
    table('WESelect', 'we_resume', resume_out)
    table('WEPrefix', 'we_prefixes', prefixes_out)
    out.append('static const char* const we_facts[]={' + ','.join(_q(f) for f in sorted(facts)) + '};\n')
    out.append(_LOOKUPS)
    return ''.join(out)


_LOOKUPS = r'''
static bool we_equal(const char*a,const char*b){if(!a||!b)return a==b;while(*a&&*a==*b){++a;++b;}return *a==*b;}
static bool we_starts(const char*value,const char*prefix){if(!value||!prefix||!*prefix)return false;while(*prefix)if(*value++!=*prefix++)return false;return true;}
template<class T,unsigned N> static unsigned we_size(const T(&)[N]){return N;}
}
const WorldEnemyVisualModel* world_enemy_visual(unsigned model){return model<we_size(we_models)?we_models+model:nullptr;}
const WorldEnemyVisualState* world_enemy_state(unsigned model,int state){auto*m=world_enemy_visual(model);return m&&state>=0&&unsigned(state)<m->state_count?we_states+m->state_first+state:nullptr;}
const WorldEnemyVisualMove* world_enemy_move(unsigned model,int node){auto*m=world_enemy_visual(model);if(m)for(unsigned i=0;i<m->move_count;++i){auto*r=we_moves+m->move_first+i;if(r->node==node)return r;}return nullptr;}
const WorldEnemyVisualAction* world_enemy_action(const WorldEnemyVisualMove&m,unsigned index){return index<m.action_count?we_actions+m.action_first+index:nullptr;}
const WorldEnemyVisualSkin* world_enemy_skin(unsigned model,unsigned skin){auto*m=world_enemy_visual(model);return m&&skin<m->skin_count?we_skins+m->skin_first+skin:nullptr;}
const WorldEnemyVisualClip* world_enemy_clip_row(unsigned model,unsigned index){auto*m=world_enemy_visual(model);return m&&index<m->clip_count?we_clips+m->clip_first+index:nullptr;}
const WorldEnemyVisualClip* world_enemy_clip_directory(unsigned directory){return directory>=38&&directory-38<we_size(we_clips)?we_clips+directory-38:nullptr;}
const WorldEnemyVisualTransition* world_enemy_transition_row(unsigned model,unsigned index){auto*m=world_enemy_visual(model);return m&&index<m->transition_count?we_transitions+m->transition_first+index:nullptr;}
bool world_enemy_condition(unsigned id,WorldEnemyFactReader read,void*ctx){
 if(id>=we_size(we_conditions))return false;const auto&r=we_conditions[id];int value=0;
 if(!r.op)return true;if(r.op==7)return world_enemy_condition(r.left,read,ctx)&&world_enemy_condition(r.right,read,ctx);
 if(r.op==8)return world_enemy_condition(r.left,read,ctx)||world_enemy_condition(r.right,read,ctx);
 if(!read||!read(ctx,r.fact,value))return false;
 switch(r.op){case 1:return value==r.value;case 2:return value!=r.value;case 3:return value<r.value;
 case 4:return value<=r.value;case 5:return value>r.value;case 6:return value>=r.value;default:return false;}
}
int world_enemy_initial(unsigned model,WorldEnemyFactReader read,void*ctx){
 auto*m=world_enemy_visual(model);if(!m)return -1;
 for(const auto&r:we_initial)if(r.model==model&&world_enemy_condition(r.condition,read,ctx))return r.state;
 return m->initial_state;
}
int world_enemy_resume(unsigned model,WorldEnemyFactReader read,void*ctx){
 if(!world_enemy_visual(model))return -1;
 for(const auto&r:we_resume)if(r.model==model&&world_enemy_condition(r.condition,read,ctx))return r.state;
 return world_enemy_initial(model,read,ctx);
}
int world_enemy_transition(unsigned model,int state,const char*trigger,WorldEnemyFactReader read,void*ctx){
 auto*m=world_enemy_visual(model);if(!m||!trigger)return -1;
 for(unsigned i=0;i<m->transition_count;++i){const auto&r=we_transitions[m->transition_first+i];
  if((r.from<0||r.from==state)&&we_equal(r.trigger,trigger)&&world_enemy_condition(r.condition,read,ctx))return r.to;}
 return -1;
}
int world_enemy_resolve_clip(unsigned model,const char*native,unsigned skin,int move,WorldEnemyFactReader read,void*ctx){
 auto*m=world_enemy_visual(model);if(!m||!native||skin>=m->skin_count)return -1;
 char prefixed[192];const char* selected=native;
 bool explicit_prefix=false;for(const auto&r:we_prefixes)if(r.model==model&&we_starts(native,r.prefix))explicit_prefix=true;
 for(const auto&r:we_prefixes)if(!explicit_prefix&&r.model==model&&world_enemy_condition(r.condition,read,ctx)){
  unsigned n=0;const char*p=r.prefix;while(*p&&n+1<sizeof prefixed)prefixed[n++]=*p++;
  p=native;while(*p&&n+1<sizeof prefixed)prefixed[n++]=*p++;if(*p)return -1;
  prefixed[n]=0;selected=prefixed;break;
 }
 for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<m->clip_count;++i){const auto&r=we_clips[m->clip_first+i];
  if(r.skin!=skin||!we_equal(r.native_clip,selected)||r.move!=(pass==0?move:-1))continue;
  if(world_enemy_condition(r.condition,read,ctx))return r.directory;
 }
 return -1;
}
unsigned world_enemy_fact_count(){return we_size(we_facts);}
const char* world_enemy_fact_name(unsigned index){return index<we_size(we_facts)?we_facts[index]:nullptr;}
'''


def summary():
    data = plan()
    return dict(models=len(data['models']), used_original_clips=data['used_clips'],
                rendered_clip_variants=len(data['jobs']), total_clips=BASE_CLIPS+len(data['jobs']),
                animation_images=sum(row['frames'] for row in data['jobs']),
                longest_original_seconds=max(row['original_duration'] for row in data['jobs']),
                max_frames=max(row['frames'] for row in data['jobs']))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-plan', type=Path)
    args = parser.parse_args()
    if args.write_plan:
        args.write_plan.write_text(json.dumps(plan()['jobs'], indent=2) + '\n')
    print(json.dumps(summary()))
