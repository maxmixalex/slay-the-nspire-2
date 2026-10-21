"""Public rest-character recipes and append-only clip jobs; no private assets.

Keep registry generation independent of the PCK, Pillow, NumPy and Spine.
Actual scene sampling lives in world_rest_actor_assets and is imported only by
the converter. Source facts are from the pinned v0.111.0 / 41cef1ea scenes and
NRestSiteRoom._Ready / NRestSiteCharacter._Ready / HideFlameGlow.
"""
from __future__ import annotations

import math
from world_room_profile import room_profile


# Character ordinals match the existing five-character runtime directory. Osty
# is part of the Necrobinder rest scene regardless of his combat health. The
# single-player scene keeps Necro before Osty; its right anchor is multiplayer.
REST_ACTORS = (
    dict(key='IRONCLAD', character=0, scene_character='IRONCLAD', node='SpineSprite',
         resource='animations/rest_site/ironclad/rest_site_ironclad_skel_data.tres',
         root_position=[-2., 42.], position=[0., 0.], scale=[1., 1.],
         source_version='4.2.37', duration=6.666667, blends=[0]),
    dict(key='SILENT', character=1, scene_character='SILENT', node='SpineSprite',
         resource='animations/rest_site/silent/rest_site_silent_skel_data.tres',
         root_position=[-2., 42.], position=[67.1047, -9.21046], scale=[1., 1.],
         source_version='4.2.37', duration=8., blends=[0]),
    dict(key='DEFECT', character=2, scene_character='DEFECT', node='SpineSprite',
         resource='animations/rest_site/defect/rest_site_defect_skel_data.tres',
         root_position=[-2., 42.], position=[-22.4452, -38.9917], scale=[.95, .95],
         source_version='4.2.43', duration=8., blends=[0, 1, 0],
         plane_boundaries=['glow4', 'r arm 1'],
         draw_order=['BG', 'shadow', 'cape back', 'bottom foot', 'bottom leg',
                     'hips', 'top leg', 'top foot', 'orb', 'glow4', 'glow3',
                     'glow2', 'glow1', 'r arm 1', 'r arm 2', 'r hand', 'l arm 1',
                     'l arm 2', 'l hand', 'neck', 'cape', 'head', 'eye shine',
                     'cast shadow', 'highlight']),
    dict(key='REGENT', character=3, scene_character='REGENT', node='SpineSprite2',
         resource='animations/rest_site/regent/rest_site_regent_skel_data.tres',
         root_position=[0., 0.], position=[7.894674, 11.842011], scale=[1., 1.],
         # The authored empty CanvasItemMaterial replaces the skeleton's
         # multiply slot with Godot's default MIX. The slot enum alone would
         # incorrectly darken this shadow a second time.
         source_version='4.2.43', duration=8., blends=[0], material_overrides={2: 0},
         draw_order=['Background ref', 'ground shadow', 'throne', 'throne shadow',
                     'guys', 'bottom leg', 'top leg', 'lower arm', 'body', 'head',
                     'neck', 'top arm', 'chain', 'shoulders', 'hands', 'eye',
                     'shadow', 'front_minion_shadow', 'back_minion_shadow', 'highlight']),
    dict(key='NECROBINDER', character=4, scene_character='NECROBINDER', node='Necro',
         resource='animations/rest_site/necrobinder/rest_site_necrobinder_skel_data.tres',
         root_position=[-2., 42.], position=[6.5789, -10.5262], scale=[1.12518, 1.12518],
         source_version='4.2.43', duration=21.333334, blends=[0], material_overrides={2: 0}),
    dict(key='OSTY', character=4, scene_character='NECROBINDER', node='Osty',
         resource='animations/rest_site/necrobinder/rest_site_osty_skel_data.tres',
         root_position=[-2., 42.], position=[-227.63, 352.629], scale=[.812, .812],
         source_version='4.2.43', duration=10.000001, blends=[0]),
)

ROOT_SCALE = [.760006, .760006]
ACT_CLIPS = ('overgrowth_loop', 'hive_loop', 'glory_loop')
LIGHT_OFF_TRACK = dict(track=1, clip='_tracks/light_off', loop=True)


def rest_actor_plan(profile=None) -> dict:
    """Return 48 ordered plane clips, all registered to the full room camera.

    Six actors have independent source periods. Defect needs three consecutive
    blend planes rather than a flattened alpha-over image. Regent's original
    scene material overrides its skeleton's multiply slot with normal MIX.
    A 256-frame cap lowers only sample rate, never the complete loop duration.
    """
    profile = room_profile() if profile is None else profile
    actor = profile['rest']['singleplayer_actor']
    if (tuple(actor['clips_by_act_index']) != ACT_CLIPS or actor['flip_x'] or
            actor['child_position'] != [0., 0.]):
        raise ValueError('Original single-player rest animation dispatch drift')
    clips = []
    for recipe in REST_ACTORS:
        duration_ms = round(recipe['duration'] * 1000)
        # Normalize the float32 source's 10.000001 seconds to its reviewed
        # millisecond period before choosing the nominal 20-Hz frame budget.
        frames = min(256, math.ceil(duration_ms * 20 / 1000))
        for act, clip in enumerate(ACT_CLIPS):
            for dark in (False, True):
                model = f'rest_actor.{recipe["key"]}.ACT{act + 1}.{"DARK" if dark else "LIT"}'
                for plane, blend in enumerate(recipe['blends']):
                    clips.append(dict(key=f'{model}.P{plane}', kind='rest-actor-clip',
                        actor=recipe['key'], character=recipe['character'],
                        path=actor['scenes'][recipe['scene_character']],
                        resource=recipe['resource'], node=recipe['node'],
                        act_index=act, clip=clip, dark=dark, plane=plane, blend=blend,
                        size=list(profile['output']['size']),
                        viewport=list(profile['conversion_viewport']),
                        screen_origin=list(profile['output']['screen_origin']),
                        pivot=[0, 0], fit='uniform_cover', background='transparent', loop=True,
                        duration=recipe['duration'], duration_ms=duration_ms,
                        frames=frames, frame_ms=max(1, round(duration_ms / frames)),
                        shader_time=0., phase=0.,
                        plane_encoding='additive_rgb_alpha255' if blend == 1 else 'straight_rgba'))
    return dict(clips=clips)


def generated_rest_actor_header() -> str:
    rows = ''.join(' {"' + row['key'] + '",' + str(round(row['duration'] * 1000)) + ',' +
                   str(row['character']) + ',' + str(len(row['blends'])) + ',{' +
                   ','.join(str(value) for value in row['blends'] + [0] * (3 - len(row['blends']))) + '}},\n'
                   for row in REST_ACTORS)
    return ('#pragma once\n#include <cstdint>\n'
            '// Generated original rest actors; periods and ordered blend planes only.\n'
            'namespace spire {\n'
            'struct WorldRestActor { const char* key; uint32_t duration_ms; uint8_t character,planes,blend[3]; };\n'
            'constexpr WorldRestActor WorldRestActors[]={\n' + rows + '};\n'
            'constexpr unsigned WorldRestActorCount=sizeof(WorldRestActors)/sizeof(WorldRestActors[0]);\n'
            'constexpr unsigned world_rest_actor_frame(uint64_t elapsed,unsigned count,uint32_t duration){\n'
            ' return count&&duration?unsigned((elapsed%duration)*count/duration):0;\n'
            '}\n}\n')


def sample_rest_actor(pack, row, sampler, *, texture_loader=None):
    """Lazy converter entry point; importing the plan never loads art tooling."""
    from world_rest_actor_assets import sample_rest_actor as sample
    return sample(pack, row, sampler, texture_loader=texture_loader)
