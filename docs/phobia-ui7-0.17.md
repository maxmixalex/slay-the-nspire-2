# UI7 original phobia visuals

Reference: **v0.111.0 / 41cef1ea**. Checkpoint 6's original-art conversion,
five Python suites, native synthetic fixture and both actual-pack App flows
have passed. Final integrated UI7 resources and platform acceptance remain
pending; this is a scoped phobia result, not whole-game UI acceptance.

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
The executed source is **`871b2b363ab1145e488c1f226831388f5ed21aaf`**.
The five Python suites passed in `ui7-phobia-public-geometry-600b8bac4e80`.
The native fixture passed in the checkpoint-6 CTest gate, whose overall
result was 75/87. Both actual-pack runs passed separately:
`ui7-phobia-actual-eng-5b374d823046` and
`ui7-phobia-actual-zhs-dbd03ce8d51e`. Each produced 29 private normal,
enabled and dead-body captures. The 13 English pairs and three dead-body captures
were visually reviewed for registration, clipping and unchanged actor
placement. These isolated encounter fixtures do not cover whole-run routing.

Actual conversion journal `ui7-phobia-original-assets-noise-c2cd54f1d72e`
passed with unchanged source. It measured the ordinary registrations before
sampling 811 images, 43 clips and four backgrounds. All 811 new images decode
exactly after retained grouping; previous payload, indices and locale strings
are unchanged. The groups store **3,080,418 bytes** instead of **6,734,218**,
saving **3,653,800 bytes**, with 73 groups and 21 exact frame aliases. The
largest group's decoded payload is 209,280 bytes; no runtime buffer grows.

The intermediate English v25 file is 94,054,914 bytes and Chinese is
96,847,546 bytes. Profiles, additional HUD resources and final original-font
replacement are later appends; these are not final delivery sizes or hashes.
The first conversion attempt, `ui7-phobia-original-assets-74b8b8e4deb4`,
failed for a missing reviewed FastNoiseLite dependency. Its failed receipt
is retained. The successful retry used the exact Godot 4.5.1 header already
identified in [enemy-assets-0.17.md](enemy-assets-0.17.md).

The Entomancer's alternate body has a separate 100-particle, two-second swarm
system. That independent particle effect is not yet converted. Original
background dynamic particles, shaders and alternative layer choices remain
subject to the existing explicit scene-renderer limits. A working phobia
toggle does not establish complete original GPU-material equivalence.
