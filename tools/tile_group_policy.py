"""Resolve measured combat cache protection from semantic animation profiles.

Only an observed hot encounter family is selected. Group numbers and physical
offsets are always derived from the actual pack's active image indices. Keeping
each selected source group separate prevents paired-group overfetch for any
combination or phase of the selected tracks; it does not promise that the
original working set fits every cache.
"""
from __future__ import annotations

import ast
import json
from pathlib import Path
import re

import content_tiles as t

ROOT = Path(__file__).resolve().parents[1]
POLICY = 'bowlbug-family-all-combat-actors-v1'


def literal_profile(path, names):
    """Read public declarative values without loading the private raster stack."""
    result = {}
    for node in ast.parse(Path(path).read_text()).body:
        if not isinstance(node, ast.Assign) or len(node.targets) != 1:
            continue
        target = node.targets[0]
        targets = target.elts if isinstance(target, (ast.Tuple, ast.List)) else [target]
        if not any(isinstance(item, ast.Name) and item.id in names for item in targets):
            continue
        value = ast.literal_eval(node.value)
        values = value if isinstance(target, (ast.Tuple, ast.List)) else [value]
        for item, value in zip(targets, values):
            if isinstance(item, ast.Name) and item.id in names:
                result[item.id] = value
    t.require(set(result) == set(names), 'Combat animation profile is incomplete')
    return result


def combat_clips(source):
    """All tracks in the measured encounter family and its playable actors.

    BowlbugsNormal chooses two of Egg/Silk/Nectar alongside Rock. Selecting the
    entire semantic model family also covers the alternative encounter seeds,
    rather than tuning only the Ironclad/Egg/Nectar/Rock acceptance fixture.
    """
    from asset_profile import CLIPS as original
    from all_character_ui_profile import BASE_CLIPS, NEW_CLIPS
    from world_ui_profile import CLIP_ROWS, validate_headers

    validate_headers()
    enum = dict((key, int(value)) for key, value in re.findall(
        r'\b(Anim\w+)\s*=\s*(\d+)', (ROOT / 'include/assets.hpp').read_text()))
    selection = {}

    def add(key, clip, enum_key=None):
        t.require(key not in selection and 0 <= clip < len(source.clips),
                  'Protected animation is missing: ' + key)
        if enum_key:
            t.require(enum.get(enum_key) == clip, 'Character clip directory drift: ' + enum_key)
        selection[key] = clip

    for clip, (name, model, track, *_rest) in enumerate(original):
        if model == 'ironclad':
            add('character.ironclad.' + track, clip, 'Anim' + name)
    silent = literal_profile(ROOT / 'tools/build_silent.py', {'NEW_CLIPS'})['NEW_CLIPS']
    t.require(len(original) + len(silent) == BASE_CLIPS, 'Base character directory drift')
    for clip, (model, track, *_rest) in enumerate(silent, len(original)):
        if model == 'silent':
            role = 'Idle' if track == 'idle_loop' else track.title()
            add('character.silent.' + track, clip, 'AnimSilent' + role)
    for clip, (name, model, track, *_rest) in enumerate(NEW_CLIPS, BASE_CLIPS):
        if not name.endswith('Camp'):
            add('character.' + model + '.' + track, clip, 'Anim' + name)

    death = literal_profile(ROOT / 'tools/character_death_assets.py', {'CHARACTERS', 'BASE_CLIPS'})
    for offset, model in enumerate(death['CHARACTERS']):
        add('character.' + model + '.die', death['BASE_CLIPS'] + offset)
    # These currently use retained images, so they select no tile groups. Keep
    # their semantic coverage if a later regeneration moves them into tiles.
    for offset, model in enumerate(('defect', 'regent'), 2 * len(death['CHARACTERS'])):
        add('character.' + model + '.die_particles', death['BASE_CLIPS'] + offset)

    models = set()
    for row in CLIP_ROWS:
        if row.get('model', '').startswith('Bowlbug'):
            models.add(row['model'])
            add('enemy.' + row['key'], row['id'])
    t.require({'BowlbugEgg', 'BowlbugNectar', 'BowlbugRock', 'BowlbugSilk'} <= models,
              'Bowlbug encounter family is incomplete')
    actors = {key.split('.')[1] for key in selection if key.startswith('character.')}
    t.require(actors == {'ironclad', 'silent', 'defect', 'regent', 'necrobinder', 'osty'},
              'Playable combat actor coverage changed')
    return selection


def resolve(source, reader, frames, clips):
    """Return original adjacent pairs that must stay separate, with provenance."""
    by_frame = {(local, at): active for local, at, active in frames}
    groups, selected_frames, rows, owners = set(), set(), [], {}
    for key, clip in sorted(clips.items()):
        t.require(isinstance(key, str) and isinstance(clip, int) and
                  0 <= clip < len(source.clips), 'Invalid protected animation selection')
        first, count, *_rest = source.clips[clip]
        t.require(count > 0 and first + count <= len(source.images), 'Invalid protected clip range')
        active_groups, tiled_frames = set(), 0
        for image in range(first, first + count):
            off, _w, _h, fmt, _length = source.images[image]
            if fmt != 6:
                continue
            _global_at, local, at, _size = t.DESCRIPTOR.unpack_from(source.data, off)
            t.require((local, at) in by_frame, 'Protected image has no decoded active index')
            current = set(map(int, by_frame[local, at] // reader.per_group))
            active_groups.update(current)
            selected_frames.add((local, at)); tiled_frames += 1
        groups.update(active_groups)
        for group in active_groups:
            owners.setdefault(group, []).append(key)
        rows.append(dict(key=key, clip=clip, clock=list(source.clips[clip]),
                         tiled_frames=tiled_frames, source_groups=sorted(active_groups)))
    pairs = {group // 2 for group in groups if (group // 2) * 2 + 1 < reader.tile_groups}
    provenance = []
    for pair in sorted(pairs):
        source_groups = []
        for group in (pair * 2, pair * 2 + 1):
            at, size, raw, _crc, _kind, count = reader.groups[group]
            source_groups.append(dict(group=group, records=count, raw_bytes=raw,
                stream_sha256=t.sha256(source.data[at-32:at+size]),
                selected_by=owners.get(group, [])))
        provenance.append(dict(source_pair=pair, groups=source_groups))
    report = dict(selected_clips=rows, selected_active_groups=sorted(groups),
                  protected_source_pairs=provenance, protected_pairs=len(pairs),
                  selected_tiled_frames=len(selected_frames),
                  resolved_selection_sha256=t.sha256(json.dumps(rows, sort_keys=True,
                      separators=(',', ':')).encode()),
                  group_page_bytes=131072, ordinary_group_pages=48,
                  selected_track_group_pages_do_not_increase=True,
                  guarantee='Every selected active source group remains a separate exact stream; '
                            'any union of selected tracks/phases has its original group residency. '
                            'This does not assert the original working set always fits 48 pages.')
    return pairs, report
