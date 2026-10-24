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

## Explicit remaining composition differences

Named slots and unnamed width-based formations retain the accepted UI6
contracts. Original `NCombatRoom.AdjustCreatureScaleForAspectRatio` additionally
fits ally/enemy containers to the viewport's rightmost creature extent and
offsets the enemy container. The port's fixed 122-pixel ground rail, omitted
aspect-fit step, random scale/hue variations, remaining rumble/trauma triggers,
GPU turbulence and all secondary move VFX are still adaptations or gaps.
They are not hidden by the native/WASM determinism checks or this queue test.
