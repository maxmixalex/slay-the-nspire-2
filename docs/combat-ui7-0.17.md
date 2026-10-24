# UI7 combat feedback and bounded playback

Reference: **v0.111.0 / 41cef1ea**. Checkpoint 6's native combat-damage suite
passed its canonical and presentation cases. Final composed original-asset
screen acceptance and complete original VFX parity are separate requirements.

## Damage and secondary feedback

`CreatureCmd.Damage` and `DamageResult` retain blocked damage, actual HP loss
and overkill separately. The port now observes those real command results
instead of deriving the displayed number from clamped health-bar movement.
Overkill numbers, fully blocked hits, explicit zero damage and prevented
nonzero hits follow separate paths. Osty and the player's shield/HP outcomes
remain separate events. Force-kill has no fabricated damage number.

The App queue releases hit numbers and health facts at the corresponding
command time. Actual power Flash hooks produce independent presentation
events. `NHealthBar` / `NBlockBrokenVfx` supply the first-shield entrance and
the authored 400 ms split / 600 ms fade. Original glow, ring, streak and speck
images drive bounded hit/block particles. Their 60 Hz display integration
does not read the gameplay RNG. `ScreenPunchInstance` supplies the latest-hit
300 ms scene offset and settings multipliers 0, 0.5, 1, 2 and 4; the HUD stays
outside that transform.

`combat_damage_ui7_regression` exercises real commands with and without the
observer and compares complete save bytes, then checks pet damage, Buffer,
Intangible flashes, particle cadence and screen-shake strengths. It passed
inside checkpoint 6's overall 75/87 native gate. The previous checkpoint's
particle fixture compared struct padding; the replacement compares every
live particle field and the presentation random state.

## Queue memory correction

The new exact-damage queue exposed an older 8 KiB budget assertion.
Checkpoint 6's `CombatFeedback` occupied **9,320 bytes**: its 128 health
samples occupied 40 bytes each and its separate 128 damage results occupied
16 bytes each. Raising the test's budget would not recover calculator RAM.

The checkpoint-8 layout instead stores canonical block, tracked block and
Doom facts at their existing signed 16-bit width. HP, maximum HP, derived
poison damage, identity and timestamp retain all 32 bits. Actor and infinite
flags occupy the remaining two aligned bytes. A health sample is now
**28 bytes**, and the complete feedback object is **7,784 bytes**, saving
**1,536 bytes**. No packed unaligned fields, smaller queue, save-format change
or discarded damage event is introduced. The static 8 KiB budget is restored.

A boundary regression fills both 128-entry queues with billion-HP values,
two-billion damage results and the full poison arithmetic series, verifies
the canonical-width extrema, then checks overflow and final observed state.
The short compiler/size inspection passed; its formal checkpoint-8 execution
remains pending in this document's current stage.

## UI8 original scene registration and composition

The scene renderer now composes the actual `NGame.FourByThree` viewport,
1680×1260, with the original `combat_room.tscn` parent anchors and camera
pivot. It applies `NCombatRoom.AdjustCreatureScaleForAspectRatio` once for a
combat binding and retains the resulting fit after deaths and summons. All
24 authored encounters and the unnamed, width-based formation path share
that transform. Creature Bounds, Center and IntentPos remain distinct
anchors. The previous fixed122px floor and Bounds-derived raster scale have
been removed from the original-world path.

The original animation bake's common extent is not the creature's Bounds.
`tools/measure_combat_registration.py` independently recovers that extent
with the pinned Spine runtime and verifies dimensions/pivots against every
existing clip before emitting numeric source-units-per-pixel metadata. The
stage1 measurement actually passed for109 models:107 independent models and
two Kaiser shared sprites, checking1,144 stored clips. It also remeasured
all five player bakes and Osty, including the original scene child's scale
and position. No new pixel bake, runtime scan or extra decoded image pool
is needed. Vantom's large transparent common bake legitimately uses a
larger scale; imposing an arbitrary scale cap would shrink its body.

Health bars use the authored Bounds plus the monster-only24-source-pixel
margin; Decimillipede segments apply the original35px subtraction first.
The7px HealthBar and18px StateDisplay offsets are source-space offsets,
not7/18 calculator pixels. The readable6px calculator bar retains a3px
separation before status icons. A multirow status group that cannot fit
below the bar moves as a whole above it, rather than drawing over the HP
number or the hand. This readable bar/icon sizing remains an explicit
small-screen adaptation.

## UI8 Osty and orb presentation

Osty now observes displayed, queued HP facts. Its original750ms growth
interpolates visual scale with Sine/InOut and root position linearly;
Bounds change only when that animation completes. Initial placement uses
`PositionLocalPlayerOsty`, while later HP changes use the player's actual
position and `GetOstyOffsetFromPlayer`. Death holds the terminal death frame,
waits for that clip plus500ms, then shrinks. A stage2 regression exposed an
ordering defect when queued health was observed before the death clip was
selected; the next source stage reserves that same delay in either order.

Defect orbs now occupy the source `NOrbManager.TweenLayout` arc: angles
−150° through−25°, radius225px through300px for3 through10 slots, and the
Orbs container's195px Y offset from the creature's OrbPosition fallback.
Slot changes interpolate over450ms. Occupied and empty-slot entrances retain
the original500/250ms timing. Dark's passive/evoke labels, Plasma's lack of
labels, Glass's zero tint and per-slot hover tips use the original model
rules and localized strings. Clicking an orb no longer opens the port's
legacy aggregate status panel. The existing keyboard status route remains
available. These slots currently use original raster icons; this is not a
claim of complete original orb skeleton, evoke or shader parity.

## Executed evidence and remaining work

The immutable UI8 stage2 checkpoint
`d45fe06ea9457729ed7623fb1c7cdeabede9288c` built successfully with Clang18 and
passed83/87 native suites. Combat layout, health-bar UI, card HUD, five
characters and input suites passed. The four recorded failures were two
outdated resource-version fixtures, the merchant fixture's missing original
700ms entrance wait, and the Osty death-delay ordering defect above. Their
corrections and later hand changes require a new immutable gate; the failed
stage2 journal is not relabeled as passed.

The actual v28 English five-character run passed8,460 checks. Actual
original-pack combat/character/large-enemy, orb-hover and Osty growth images
were generated and inspected during development. The actual world run
stopped at the same Osty assertion; final integrated bilingual review is
still required. Numerical scene coverage and a passing port regression do
not establish a whole-game fidelity percentage.

Remaining differences include per-model random scale/hue, complete
rumble/trauma triggers, GPU turbulence, original orb skeletons and evoke
materials, and secondary move VFX that are not yet independently accepted.
Generated/played card scales and arc projection have been compared and
corrected; the remaining material trails need their own source comparison. The prior fixed-ground/aspect-fit gaps are now
implemented; these separate gaps must not be hidden by that progress.
