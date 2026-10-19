"""Append-only original-asset profile for the complete five-character NSP2/13.

The NSP2/12 image, auxiliary and animation prefixes are immutable. These source
paths and sampling parameters contain no redistributed reference-game assets.
"""
BASE_CARDS = 228
BASE_STRINGS = 749
BASE_AUXILIARY = 233
BASE_CLIPS = 17
CARDS = 497
STRINGS = BASE_STRINGS + (CARDS - BASE_CARDS) * 3
CLIPS = 38

DEFECT_POWERS = [
    ('BiasedCognition', 'biased_cognition_power'), ('Coolant', 'coolant_power'),
    ('CreativeAi', 'creative_ai_power'), ('EchoForm', 'echo_form_power'),
    ('Feral', 'feral_power'), ('Hailstorm', 'hailstorm_power'),
    ('Iteration', 'iteration_power'), ('LightningRod', 'lightning_rod_power'),
    ('Loop', 'loop_power'), ('MachineLearning', 'machine_learning_power'),
    ('SignalBoost', 'signal_boost_power'), ('Smokestack', 'smokestack_power'),
    ('Spinner', 'spinner_power'), ('Storm', 'storm_power'),
    ('Subroutine', 'subroutine_power'), ('Thunder', 'thunder_power'),
    ('TrashToTreasure', 'trash_to_treasure_power'),
    ('ConsumingShadow', 'consuming_shadow_power'), ('FreePower', 'free_power_power'),
    ('EnergyNextTurn', 'energy_next_turn_power'),
    ('FocusedStrikeFocus', 'focused_strike_power'), ('HotfixFocus', 'hotfix_power'),
    # The pinned PCK has no HyperbeamFocusDown texture. The common original
    # Focus icon accompanies its explicit negative/temporary status label.
    ('HyperbeamFocus', 'focus_power'), ('SynchronizeFocus', 'synchronize_power'),
]
REGENT_POWERS = [
    ('Arsenal', 'arsenal_power'), ('BlackHole', 'black_hole_power'),
    ('ChildOfTheStars', 'child_of_the_stars_power'), ('Furnace', 'furnace_power'),
    ('Genesis', 'genesis_power'), ('MonarchsGaze', 'monarchs_gaze_power'),
    ('PaleBlueDot', 'pale_blue_dot_power'), ('Parry', 'parry_power'),
    ('PillarOfCreation', 'pillar_of_creation_power'), ('Reflect', 'reflect_power'),
    ('Royalties', 'royalties_power'), ('SeekingEdge', 'seeking_edge_power'),
    ('SpectrumShift', 'spectrum_shift_power'), ('SwordSage', 'sword_sage_power'),
    ('TheSealedThrone', 'the_sealed_throne_power'), ('Tyranny', 'tyranny_power'),
    ('VoidForm', 'void_form_power'), ('ForegoneConclusion', 'foregone_conclusion_power'),
    ('EnergyNextTurn', 'energy_next_turn_power'), ('StarsNextTurn', 'star_next_turn_power'),
    ('Monologue', 'monologue_power'), ('Orbit', 'orbit_power'),
]
NECRO_POWERS = [
    ('Calcify', 'calcify_power'), ('CallOfTheVoid', 'call_of_the_void_power'),
    ('Countdown', 'countdown_power'), ('DanseMacabre', 'danse_macabre_power'),
    ('Demesne', 'demesne_power'), ('DevourLife', 'devour_life_power'),
    ('Friendship', 'friendship_power'), ('Haunt', 'haunt_power'),
    ('Lethality', 'lethality_power'), ('NecroMastery', 'necro_mastery_power'),
    ('Neurosurge', 'neurosurge_power'), ('Pagestorm', 'pagestorm_power'),
    ('ReaperForm', 'reaper_form_power'), ('SentryMode', 'sentry_mode_power'),
    ('Shroud', 'shroud_power'), ('SleightOfFlesh', 'sleight_of_flesh_power'),
    ('SpiritOfAsh', 'spirit_of_ash_power'), ('Veilpiercer', 'veilpiercer_power'),
    ('BorrowedTime', 'borrowed_time_power'), ('EnergyNextTurn', 'energy_next_turn_power'),
    ('SummonNextTurn', 'summon_next_turn_power'), ('ForbiddenGrimoire', 'forbidden_grimoire_power'),
]

ALL_CHARACTER_UI = [
    *[(name + kind, source, size, fit)
      for name, slug in [('Defect', 'defect'), ('Regent', 'regent'), ('Necrobinder', 'necrobinder')]
      for kind, source, size, fit in [
          ('Background', '#select:' + slug, (320, 240), 'cover'),
          ('Portrait', f'images/packed/character_select/char_select_{slug}.png', (36, 40), 'contain'),
          ('Energy', '@card/energy_' + slug, (26, 26), 'contain'),
          ('Orb', '#energy:' + slug, (42, 42), 'contain'),
          ('MapMarker', f'images/packed/map/icons/map_marker_{slug}.png', (23, 23), 'contain'),
      ]],
    *[(color.title() + kind.title(), f'#frame:{color}:{kind}', (80, 112), 'stretch')
      for color in ('blue', 'orange', 'pink') for kind in ('attack', 'skill', 'power')],
    ('CrackedCore', 'images/relics/cracked_core.png', (25, 25), 'contain'),
    ('DivineRight', 'images/relics/divine_right.png', (25, 25), 'contain'),
    ('BoundPhylactery', 'images/relics/bound_phylactery.png', (25, 25), 'contain'),
    *[(name.title() + 'Orb', f'images/orbs/{name}_orb.png', (24, 24), 'contain')
      for name in ('lightning', 'frost', 'plasma', 'glass')],
    ('OstyPortrait', '#osty', (36, 40), 'contain'),
    ('StarCounter', '#stars', (42, 42), 'contain'),
    *[(character + 'Power' + name, 'images/powers/' + icon + '.png', (18, 18), 'contain')
      for character, powers in [('Defect', DEFECT_POWERS), ('Regent', REGENT_POWERS), ('Necro', NECRO_POWERS)]
      for name, icon in powers],
    *[(name, 'images/powers/' + icon + '_power.png', (18, 18), 'contain')
      for name, icon in [('Conqueror', 'conqueror'), ('Debilitate', 'debilitate'),
                         ('Hang', 'hang'), ('Oblivion', 'oblivion'), ('SicEm', 'sic_em')]],
]
AUXILIARY = BASE_AUXILIARY + len(ALL_CHARACTER_UI)
POWER_STARTS = {'Defect': 266, 'Regent': 290, 'Necro': 312}

# Entire original cycles are sampled. New clips use fewer samples per second
# than the historical Ironclad/Silent clips to fit the 13 MiB resident budget;
# neither their duration nor any historical frame payload is shortened.
NEW_CLIPS = [
    *[(name + kind, slug if kind != 'Camp' else 'restsite_' + slug,
       clip, 86 if kind != 'Camp' else 80, 160 if kind != 'Camp' else 108, fps)
      for name, slug in [('Defect', 'defect'), ('Regent', 'regent'), ('Necrobinder', 'necrobinder')]
      for kind, clip, fps in [('Idle', 'idle_loop', 8), ('Attack', 'attack', 12),
                              ('Hurt', 'hurt', 12), ('Cast', 'cast', 12),
                              ('Camp', 'overgrowth_loop', 4)]],
    *[('Osty' + kind, 'osty' if kind != 'Camp' else 'restsite_osty', clip,
       55 if kind != 'Camp' else 62, 100 if kind != 'Camp' else 108, fps)
      for kind, clip, fps in [('Idle', 'idle_loop', 8), ('Attack', 'attack', 12),
                              ('Hurt', 'hurt', 12), ('Die', 'die', 12),
                              ('Revive', 'revive', 12), ('Camp', 'overgrowth_loop', 4)]],
]
