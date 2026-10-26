# Combat and card feedback — UI14

Reference: the owner's pinned v0.111.0 / 41cef1ea assembly and original scenes.
Private decompilation, textures, generated packs, captures and saves remain
outside this public source repository.

## Implemented behavior

| Area | Runtime change and reference |
| --- | --- |
| Top bar | The clock occupies one icon slot and is enabled by default in combat; elapsed time appears only in its own tooltip. An explicitly disabled timer preference still hides the combat icon. Room/floor tooltips contain no timer. Normal two-slot, double-boss geometry has 15px gaps; five bottles have 6px gaps, and paged six-to-sixteen inventories retain 4px gaps. Character hover remains enabled at Ascension0; the flame and numeral share a centered anchor. |
| Pile controls | All three original pile icons use the source50ms focus-to1.25,500ms QuintOut return, and250ms pressed response. Counts remain sibling labels rather than scaling with the icon. |
| Card art | The nine Ancient cards use full-height original portraits, Ancient border/text background/banner and all ten flame frames. Infection retains all30 source frames. Bound, Entangled, Galvanized, Hexed, Tainted, Ringing and Smog each use their own original scene/material; afflictions take precedence over the model's built-in overlay. |
| Long descriptions | Selected and hovered hand cards scroll inside a clipped description rectangle in LCD-pixel increments. Only that small rectangle is restored/repainted. Small card caches retain the portrait/frame/banner while the foreground text, plaque and resource labels render in the authored order, so cache hits preserve overflow clocks without covering a cost or type plaque. |
| Generated cards | Unchanged combat instances and generated preview sprites reuse measured descriptions and the face raster across frames. Live deck flights continue reading permanent card facts; explicitly detached transformation/enchantment snapshots retain their own facts. |
| Shrink | ShrinkPower and Beetle Juice animate visual scale to0.5 over750ms with SineInOut. Formation, target bounds, health and intent anchors remain authored. Decimillipede segments explicitly reject scaling. Vantom uses its existing separately sampled multipart Shrink states for both sources. |
| Other source scaling | BigMushroom immediately calls1.5 on pickup/room entry. PhantasmalGardener retains the completed Enlarge count and applies1+0.1 ln(count+1). TestSubject applies1+0.1 Respawns with its100ms growth after the800ms respawn wait. Defect's orb anchor and manager scale follow NCreature.DoScaleTween. GigantificationPower does not call ScaleTo in this reference, so it receives no invented body growth. |
| Forge | Actual blade damage/creation/exhaustion and attacks feed a bounded presentation queue. The original hilt variants, orange forge glow, three smith hits and blade orbit are connected to the real Forge command. Forge strengthens all existing blades, including exhausted blades, while creating a usable blade when none exists outside Exhaust. |
| Enchant | The permanent enchantment commit emits one scoped observer callback. The effect retains the exact committed card, uses the original tab/icon,1s QuadInOut shader reveal,200ms sparkle start and1s hold, then flies to the deck. Failed or canceled choices emit no effect. Multiple committed cards use bounded four-card waves. |

## Bounded adaptations

The appended combat range is **5156..5264**, endpoint5265:109 images. Ancient
portraits replace nine existing image records and do not duplicate auxiliary
records. The standalone compressed image payloads total1,614,959 bytes before
the integrated common-pack grouping.

Procedural affliction shaders are sampled into eight phases per material, in
original time units. This is a bounded CPU/LDR representation, not full Godot
GPU simulation. Infection's thirty frames and Ancient fire's ten frames are
retained in full. Sovereign Blade retains its source orbit geometry and
command timing using bounded integer motion and source textures; individual
GPU particle streams are represented by bounded sprite emissions.

CombatCardMotion is7,888 bytes after adding the blade snapshots/cues; its
explicit allocation ceiling is8KiB. The hand reader retains one bounded
description surface, and the enchantment effect retains four small viewports.
These capacities do not by themselves establish calculator frame rate.

## Verification and device fixtures

`combat_ui14_regression` checks real potion, enemy turn, Forge and enchantment
commands against an independently executed canonical State, verifies save
round trips, and checks scale/overflow behavior. With an installation root and
output directory it also decodes the original appended images, captures the
effects and exports three controlled NSR saves: special card borders, Shrinker
Beetle plus Beetle Juice, and a pending Regent enchantment followed by a
playable Forge hand. Earlier routing outcomes, resources and cards in those
fixtures are supplied; they are not natural runs and are not release contents.

The stage4 asset job is closed PASS at97a63d78979daba7d12fa8475b305e836fc9503c:
109 images, nine portrait replacements, endpoint5265, unchanged frozen source.
Stage4's new native fixture initially failed validation because it used the
wrong no-origin sentinel; the fixture was corrected to0 and clears last_drawn.
The following source stage also fixes the strictly detected small-card layer
order and live deck-flight cache regressions. Final integrated native results,
ordinary-OS Firebird timing and acceptance are recorded in WORK_STATE rather
than inferred from this implementation ledger.

The stage6 combat regression is closed PASS at
`67e2822ef369dc62428dcc0698c0bda62586756f`: **11,149 synthetic checks** and
**11,395 checks with the actual integrated original resources** (endpoint5275).
The corresponding unchanged-source journals are
`combat-ui14-stage6-synthetic-7f00c7ec698a` and
`combat-ui14-stage6-original-c248f9bdbe38`. All three controlled saves pass the
production validator and identical save/load/save round trips. Actual captures
cover both shrinking actors, the three other source scaling cases, Forge,
enchantment reveal/settling and special card borders. These are native
correctness and resource-rendering results, not calculator performance results.
Stage5's intermediate compile failed because the orb helper preceded the
shared min/max helpers; stage6 replaces those calls with local expressions.

The following final source stage enables the clock icon in fresh preferences
and binds body-scale motion to the run number, so a repeated seed cannot
inherit the previous run's shrink tween. Its added checks cover default
combat-clock hits, elapsed-time pixel isolation, preservation of an explicitly
disabled preference, and an identical-seed combat restart. The older card/HUD
test now follows the pixel reader's forward/hold/return clocks and includes
keyboard navigation to the clock.
The stage7 source `0418cf3e63dabb456e4a9aaa49062d32f7ac747c` subsequently
passes **11,181 synthetic checks**, **11,430 actual-resource checks**, and the
existing combat UI12 actual-resource gate's **58,364 checks**. The latter
checks every scroll pixel against the unchanged background outside the text
rectangle and verifies that neighboring card caches remain stable. Journals:
`combat-ui14-stage7-synthetic-a1fd01535e59`,
`combat-ui14-stage7-original-9094773a2daf`, and
`combat-ui12-stage7-original-05342323d5ac`. The final three controlled saves
are byte-identical to the stage6 exports. Calculator gameplay/timing acceptance
remains a separate integrated gate.
