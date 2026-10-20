"""Append-only public room recipes; the owner's art remains a private input."""
from __future__ import annotations
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def room_profile() -> dict:
    profile = json.loads((ROOT / 'config/world-room-ui-0.17.json').read_text())
    if profile.get('schema') != 1 or profile.get('reference') != 'v0.111.0 / 41cef1ea':
        raise ValueError('Original room resource reference/schema drift')
    if (profile['conversion_viewport'] != [1920, 1080] or
            profile['output']['size'] != [320, 218] or
            profile['output']['screen_origin'] != [0, 22] or
            profile['output']['fit'] != 'uniform_cover'):
        raise ValueError('Original room presentation geometry drift')
    return profile


def room_plan(profile=None) -> dict:
    profile = room_profile() if profile is None else profile
    size, origin = profile['output']['size'], profile['output']['screen_origin']
    viewport = profile['conversion_viewport']
    images, clips = [], []
    for scene in profile['rest']['scenes']:
        for dark in (False, True):
            images.append(dict(key=scene['key'] + ('_DARK' if dark else ''),
                kind='rest-scene', path=scene['path'], size=size, viewport=viewport,
                screen_origin=origin, dark=dark, fit='uniform_cover'))
    queue = profile['treasure']['animation_queue']
    if ([item['clip'] for item in queue] != ['animation', 'shine_fade'] or
            any(item['loop'] or item['track'] != 0 for item in queue) or
            queue[1]['add_delay'] != 0):
        raise ValueError('Original treasure queue drift')
    durations = [item['source_duration_seconds'] for item in queue]
    if any(not math.isfinite(value) or not 0 < value < 10 for value in durations):
        raise ValueError('Invalid original treasure clip duration')
    for family in profile['treasure']['families']:
        common = dict(resource=family['path'], size=size, viewport=viewport,
            screen_origin=origin, pivot=[0, 0], loop=False, family=family['key'])
        for suffix, skin, time in [('CLOSED', family['skin'], 0.),
                                  ('CLOSED_OUTLINE', family['outline_skin'], 0.),
                                  ('OPEN', family['skin'], sum(durations))]:
            images.append(dict(common, key=family['key'] + '.' + suffix,
                kind='treasure-still', skins=[skin], time=time))
        for index, suffix in enumerate(('OPENING', 'SHINE')):
            duration = durations[index]
            frames = math.ceil(duration * 20)
            clips.append(dict(common, key=family['key'] + '.' + suffix,
                kind='treasure-clip', skins=[family['skin']],
                clip=queue[index]['clip'], start=0. if index == 0 else durations[0],
                duration=duration, duration_ms=round(duration * 1000),
                frames=frames, frame_ms=max(1, round(duration * 1000 / frames))))
    return dict(images=images, clips=clips)


def generated_room_header() -> str:
    profile = room_profile()
    opening, shine = profile['treasure']['animation_queue']
    width, height = profile['output']['size']
    x, y = profile['output']['screen_origin']
    return ('#pragma once\n#include <cstdint>\n'
        '// Generated original room timings and canvas geometry; no art data.\n'
        'namespace spire {\n'
        f'constexpr uint32_t WorldTreasureOpeningDurationMs={round(opening["source_duration_seconds"] * 1000)};\n'
        f'constexpr uint32_t WorldTreasureShineDurationMs={round(shine["source_duration_seconds"] * 1000)};\n'
        f'constexpr int WorldRoomWidth={width},WorldRoomHeight={height};\n'
        f'constexpr int WorldRoomOriginX={x},WorldRoomOriginY={y};\n'
        '}\n')
