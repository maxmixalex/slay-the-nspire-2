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
| Shrink | ShrinkPower and Beetle Juice animate ordinary creature visual scale to0.5 over750ms with SineInOut. Formation, target bounds, health and intent anchors remain authored. Decimillipede segments explicitly reject scaling. Both sources select Vantom's correct separately sampled multipart endpoints, but its special750ms skeletal transition is still missing; see the precise remaining difference below. |
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

## Follow-up from actual Firebird execution

The stage10 source `6355aef2fa174aa6c6e1134c4a088b4ba7cb6948` passes
**11,435 actual-resource checks**, including ordinary resume of a pending
combat enchantment choice. Preparation now finishes before the choice is
shown: confirming it cannot hide the first second of its reveal under a new
black preparation interval. Firebird captures at150/400/800/1300ms verify
this correction. Earlier stage7 captures also verify both real shrink
commands, the Smith effect and a resulting40-damage Sovereign Blade. These
are explicitly controlled saves, not natural runs. Journals include
`combat-ui14-stage10-original-12a335edaa9e` and
`combat-firebird-regent10-enchant-4ac5f25a8036`.

The source scaling review covers ShrinkPower, Beetle Juice, Big Mushroom,
Phantasmal Gardener and Test Subject, plus the special Decimillipede/Vantom
rules above. Existing Osty maximum-HP growth/death shrink is separately
implemented in `combat_osty_presentation.hpp`; its bounds update after its
750ms growth completes. Source instance-size/hue variation and aspect-fit
registration were completed in earlier stages. This inventory is not a
fresh exhaustive assembly-call-graph claim after the private input cleanup.

Vantom remains an exact visual difference. Its source scene scales the
`root_scale_adjust` bone over750ms with SineInOut while retaining an
independently controlled `tail_target_bone`. The converter implements this
driver and accepts `shrink_from`/`shrink_started`, but the integrated profile
supplies only the settled `shrink` boolean. The runtime therefore selects the
correct normal or half-body clip without the intermediate skeletal tween.
The actual retained idle endpoints at tail charge1 have the same sprite
density5376 but different registrations: normal99x113 at origin42,109 versus
shrunk76x96 at22,95. Their tail blade stays near its authored height while the
body shrinks. A whole-sprite scale cannot reproduce that change. The original
skeleton is needed to bake or otherwise verify the missing bone transition;
no guessed whole-sprite tween has been substituted. Sewer Clam's separate
coral growth is already sampled with its source500ms delay and500ms ElasticOut.

`ui14_device_fixtures.cpp` now produces a canonical Haunted Ship save and a
matching partial progress record with no introductory prompts. Its real first
End Turn generates five Dazed into Discard; the exporter independently
executes that command and checks all five instances. The console accepts the
canonical `HAUNTED_SHIP_NORMAL` key. An unknown encounter now fails before
state or RNG mutation, rather than entering a random encounter through the
`None` sentinel.

The first controlled stage10 Haunted Ship measurements exposed then-open
work: the first4s action window averages18.04FPS, while the narrower
approximately2–4s generated-card window averages21.98FPS. A12s window that
also contains settled idle averages25.80FPS and is not substituted for the
animation result. The matched prefixes are identical snapshot/flash copies;
all counters are exported by normal game exit. These historical measurements
did not establish a completed performance improvement.

That experiment found a concrete shared-raster cache defect: five identical
previews replaced the sole instance sample on their shared face and therefore
reformatted all five descriptions on every frame. The follow-up retains the
same ten rasters and adds forty small checked instance references. Revision,
resource generation, presentation reset, lock state, target, snapshot resource
values and raster-reuse serials invalidate the appropriate samples. Its new
regression covers the repeated batch, differing affliction facts and recycled
rasters. Its matching native and device results are recorded in stage12 below;
the subsequent stage13 correction addresses the remaining resource bottleneck.

Stage12 `d8510f202454270cb49b79fc4094429885d6ddea` passes11,552 synthetic
checks and11,803 actual-resource checks. Its ordinary-OS launch and exact USB
readback pass, but matched Firebird measurements show that the cache defect
was not the main bottleneck:0–4s averages18.146FPS and the narrower2–4s
generated-card window22.038FPS. Mean render improves by about0.8–1.1ms, while
the same cold resource reads remain. The normal game exits produce zero
resource failures; the stage12 result alone did not close this performance gap.

Stage13 `f1ad95b33bd46f3b7d14f8fff31f06e1109e52b7` prepares only the
already selected, displayed enemy
intent's registered action clips within the existing black transition. Actor
starts remain ahead of these optional tracks; the same28-track,288-image and
640-page limits apply. Unknown intents, active command presentations and
future random branches are excluded. The actual Haunted Ship fixture needs
439 pages including the player/enemy idle clips, background and its known
Haunt clip, with no new auxiliary images or resident allocation. The new
actual-pack regression checks that all23 Haunt frames are decoded and served
without further reads, verifies preserved actor starts, excludes the later
Swipe/Stomp clips, and compares the complete State before/after preparation.

### Stage13 final correctness and action-window measurements

The stage13 source passes **11,565 synthetic checks** and **11,902 checks
with the actual integrated original resources**. The new preparation checks
serve every selected Haunt image through the real decoder/cache interface
without additional source callbacks, preserve both actor starts, exclude the
later Swipe/Stomp clips and compare the complete State, including RNG.
Journals are `combat-ui14-stage13-synthetic-23610e3c5e82` and
`combat-ui14-stage13-original-948ab12f9176`, both with unchanged frozen source.

The ARM launcher is1,685,343 bytes, allocates22,655,568 bytes and has SHA256
`dc1f5a0b50f54ed872a397e527c8b6faf3b1827b0bc0c780ff2a916d0aed69db`.
Ordinary OS launch, the Chinese title, Continue and exact USB readback of the
launcher, controlled save and matching progress all pass. No new auxiliary
images, extra loading screen or future-state simulation were introduced.

Six independent snapshot/flash copies start from the same ordinary Continue
plus16s baseline. They execute the real opening `HAUNT_MOVE`, then use the
same normal Save and Return, Quit and OS-exit path to export performance
counters. Prefix subtraction isolates the requested action interval; minima
and maxima are not subtracted. The range names below are nominal script
times, while FPS uses the actual measured32,768Hz interval ticks. These are
Firebird virtual-hardware measurements, not host wall-clock or physical-device
FPS, and this deliberately controlled encounter is not a natural run.

| Stage13 combat window | Frames | FPS | Mean render | Mean resource time | Source reads / bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| First0–4s, including initial cold work | 95 | 23.6215 | 31.233ms | 6.729ms | 41 /355,727 |
| Generated-card preview2–4s | 59 | 29.5419 | 23.498ms | 0.284ms | 6 /62,496 |
| Flight to discard and next-hand draw4–6s | 58 | 28.9576 | 29.947ms | 1.081ms | 2 /8,192 |
| Complete first action0–6s | 153 | 25.3955 | 30.745ms | 4.588ms | 43 /363,919 |
|0–12s, explicitly including settled idle | 333 | 27.6963 | 26.996ms | 2.108ms | 43 /363,919 |

All six profiles report **zero resource failures** and complete normal OS
exit with no serial errors. The generated-card window has no intervals above
100ms or250ms; the flight/draw window has one above100ms and none above250ms.
**One initial interval above250ms remains in the first0–2s end-turn path.**
The first action therefore does not run at30FPS throughout. Its cold result
is reported separately from the near30FPS generated-card and flight windows;
the12s idle-inclusive average is not substituted for either animation result.

The3s capture shows all five generated Dazed. At5s, Discard reads10 (the five
original discards plus the five new cards), the player's second-turn banner
is visible and its new hand has arrived. The6s and12s captures show the
completed hand and next Swipe intent. Haunt is an opening-only action in the
original move graph; no second natural Haunt or altered enemy node is claimed.

For this same fixture, stage12 and stage13 produce byte-identical saved games
after each corresponding0s,2s and4s prefix:7,144 bytes before the action and
7,322 bytes after it. This is device-side state/RNG-preservation evidence for
the optimization, in addition to the native State comparison. The private
aggregate `combat-firebird-stage13/measurement-validation.json` retains all
profile identities, exact clone baselines, readback receipts and save hashes.
Actual captures also have unchanged-pixel PNG copies and a capture manifest.

The normal-exit measurement journals are
`combat-firebird-haunt13-measure-a-877bffc37f59`,
`combat-firebird-haunt13-measure-prefix-0423b1b6a5be`,
`combat-firebird-haunt13-measure-b-d7f44764220f`,
`combat-firebird-haunt13-measure-prefix3-df303a8a3ef0`,
`combat-firebird-haunt13-measure-prefix6-5c754f6e1ed7`, and
`combat-firebird-haunt13-measure-long-98a977e77aae`.
Every journal closes PASS with unchanged source. These results close the
named generated-card and flight/draw performance verification; the initial
cold stall, Vantom's missing bone tween and the broader original command-wait
evidence boundary remain explicit rather than being counted as completed.
