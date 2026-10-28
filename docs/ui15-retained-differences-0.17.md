# UI15 reconciliation of retained differences

Reference: the owner's pinned v0.111.0 / `41cef1ea`. This document supersedes
only the open-implementation status in the historical UI14 ledger. Earlier
measurement receipts remain historical and are not rewritten as UI15 results.

## Previously missing implementations now restored

| UI14 retained item | UI15 implementation | Executed evidence |
| --- | --- | --- |
| Surrounding awaited monster commands beyond AttackCommand hit profiles | The pinned original method bodies are extracted into command programs and connected to the actual move/selection/death hooks. Conditional summon/dialogue paths, block/heal/power boundaries, generated-card batch waits, wake/stun and sequential death callbacks retain their own normal/fast timing. Animation duration is not substituted for the command's awaited return. | All108 actor kinds and1,312 legal move executions at ascension0/9, normal/fast; hand-authored boundary cases; observed/unobserved production-save equality. The integrated140-suite native run passes. See `combat-ui15-0.17.md`. |
| Vantom's750ms `root_scale_adjust` transition with independently controlled tail | Original compound clips retain source draw order and separate bone motion. Rigid parts use compact layers; nonrigid parts retain original triangle/UV topology and independently sampled scale intervals. The tail stays in its authored coordinate space. Replacement tween, death-time Shrink removal and same-run restore are implemented. | All116 compound clips,2,040 full-scale frames,3,232 nonrigid references and3,750,351 sampled source vertices. Maximum verified interpolation error0.19998293 LCD pixels at the larger4/21 projection. At integration9, complete-resource combat traversal77,480 checks and Vantom effects774,616 checks pass. Integration16 retains the same semantic effects cases with6,596 assertions after omitting PPM file-byte write checks. |
| Vantom source shader/particle callbacks needed by the completed layered actor | Separate tail/hook/megablade masks and four original emitters retain source event clocks, particle count, source textures, coordinate modes and the delayed death track. | Five original textures compare exactly through the particle painter; real Ink Blot and death callbacks execute with ordinary/delayed paint intervals and actual Beetle Juice. The final compressed pack is used in the integrated gate. |

UI15 also fixes the requested Ancient healing, Tablet permanent-upgrade
notification, physical-keyboard selection observer, full rest Smith impact
sequence and per-relic inventory flashes. These are actual commit/callback
paths, not registry declarations. The final ordinary-OS acceptance and
performance measurements remain separately identified in WORK_STATE and the
UI15 feedback ledger.

## Earlier implementations carried forward

Doom's sink/particles, Architect's reading/pen/attack families, complete Random
water/light/dot emitters, Fake Merchant's scene, blur/rainbow clocks, exhaust
and discard trails, generated-card arrival, orb activation, other creature
scale/hue/sleep effects, preview contexts, startup logos and natural font
metrics already had implementations before this turn. UI15 retains them and
fixes the reported defects; rereading their source does not make them newly
completed features. The historical UI14 reconciliation identifies their
original implementation ledgers.

## Retained scope and calculator representations

The accepted omission of full Ancient actor animation remains the agreed
storage exception. Ancient gameplay, dialogue, compact options, healing and
Ancient card materials are implemented. Audio, multiplayer and Timeline
remain outside the agreed offline single-player scope.

RGB565 output, integer LCD rasterization and bounded CPU particles remain
calculator representations. Vantom uses source-derived analytic particle
integration and an independent cosmetic seed, not the GPU's exact process
history. Affliction materials retain their disclosed8-phase CPU sampling;
Infection keeps30 source frames and Ancient card fire keeps10. No additional
animation frames or original texture pixels are discarded by UI15's storage
or performance changes. The resource converters separately verify lossless
paging and grouping after the defined source-to-LCD conversion.

Calendar values retain UTC interpretation without a desktop timezone
database. History keeps96 floors and32 runs, with explicit finite drawing
and transition capacities. Presentation-only randomness preserves canonical
run RNG but does not reconstruct an unrecorded global cosmetic RNG history.
These hardware/scope boundaries are distinct from missing gameplay or an
unimplemented animation callback.

The native suites and ordinary Firebird CX OS scenarios establish their named
flows. They do not establish a physical-calculator FPS measurement, every
possible Bestiary pose combination, arbitrary cross-track Spine composition,
an exhaustive desktop-Godot visual comparison or a percentage restoration
score. Natural first-combat/save/Continue acceptance is reported separately
from controlled boss/status-card fixtures and from a natural three-act victory.
