# UI7 original phobia visuals

Reference: **v0.111.0 / 41cef1ea**. Source implementation and public fixtures
are ready for the next checkpoint. Private conversion, bilingual visual
acceptance and final resource-size measurement remain pending.

## Actual original switch

`MegaCrit.Sts2.Core.Models/MonsterModel.cs` defines alternate skeletal skins.
`MegaCrit.Sts2.Core.Nodes.Combat/NCreatureVisuals.cs` controls mutually
exclusive normal and `PhobiaModeVisuals` bodies. The original
`DecimillipedeSegment.AfterDeath` also selects a distinct shriveled texture,
and its revive path restores the living texture. Background replacement
uses `NCombatBackgroundLayer`'s `Visual` / `PhobiaModeVisual` branches.

The source-backed append contains:

| Replacement | Models / resources | Clip count |
| --- | --- | ---: |
| Animated `phobia` skin | Fossil Stalker, Haunted Ship, Skulking Colony, The Obscura, Twig Slime M | 32 |
| Static alternate body | Three Decimillipede segments, Entomancer, Phrog Parasite, Terror Eel, The Insatiable, Wriggler | 8 |
| Shriveled segment body | Decimillipede back, front and middle | 3 |
| Complete background branch | Factory, Hive, Knowledge Demon Boss, The Insatiable Boss | 0 |

The 18 authored background replacement nodes span variants and depth layers.
They are not 18 independent full-screen scenes. Factory has an authored
resource but no normal-run Act/Encounter route in the pinned model catalogue;
converting it is not counted as completing an additional gameplay route.

## Geometry and resource contract

Version 25 appends **43 clips at indices 1710–1752** and four auxiliary
background entries at **1877–1880**. Its complete profile is **1753 clips,
1881 auxiliary entries and 8704 strings**. Original directory indices and
the historical common-art payload remain unchanged.

`tools/patch_phobia_assets.py` first measures all ordinary clips of the 13
affected models through the reviewed official Spine runtime. It compares
the derived frame dimensions and pivot against every existing ordinary
frame's directory entry. An unexplained mismatch fails conversion.

Alternate skins and centered Sprite2D bodies then use that verified ordinary
scale and the original child transforms. Their common pivot/canvas may grow
to include the alternate geometry, but they are never independently resized
to fit a convenient enemy box. Dead segment textures share the living
registration. Out-of-range geometry fails instead of silently shrinking.
Background conversion explicitly activates the alternate branch; ordinary
scene conversion never changes authored visibility as a side effect.

Adjacent new frames use the existing bounded retained-group codec. Exact
duplicate frame payloads share a descriptor. Grouping validates every decoded
image and reports size changes; it adds no runtime decode buffer. This is
retained-image sharing and compression, not a new global tile codec.

The actual combat renderer and detached Bestiary previews select the new
indices when the saved preference is enabled. Switching back uses the same
original clip clock, position and presentation state. Packs before version 25
continue to use the ordinary model. The setting does not alter the canonical
State, save bytes or gameplay RNG.

## Tests and remaining limits

Public Python fixtures check explicit background branch isolation, original
Sprite2D anchoring/dead texture centers, fixed-scale overflow rejection,
ordinary-layout matching, lossless grouped output and the complete 43-entry
runtime lookup. Native fixtures exercise the actual App renderer across all
13 affected models, all 32 animated skin mappings, three actual segment
deaths, unaffected models, old-resource fallback and unchanged save bytes.
Optional private-pack captures cover the normal/enabled/dead presentations.
Only short syntax checks have run so far; no full pass is claimed here.

The Entomancer's alternate body has a separate 100-particle, two-second swarm
system. That independent particle effect is not yet converted. Original
background dynamic particles, shaders and alternative layer choices remain
subject to the existing explicit scene-renderer limits. A working phobia
toggle does not establish complete original GPU-material equivalence.
