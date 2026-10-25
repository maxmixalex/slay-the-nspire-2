# UI12 screen motion and sleeping-creature completion

Reference: original v0.111.0 / `41cef1ea`. This report records the audited
camera hooks and sleeping-creature effects. It does not promote all monster
VFX families, all command waits, or desktop GPU output to verified parity.

## Camera model

`ScreenShakePresentation` implements the source directional punch, rumble,
drunk rumble and additive trauma. Strengths retain source units
`2 / 5 / 20 / 40 / 80`, durations retain `300 / 800 / 1200 ms`, and the five
saved settings multiply them by `0 / 0.5 / 1 / 2 / 4`. The latest punch takes
precedence over rumble. Trauma adds, saturates, decays at two units per second,
and preserves the source 50-unit vector cap. The source punch uses its cubic
envelope and 60-radian-per-second cosine.

The calculator uses bounded integer gradient noise in place of Godot's
FastNoiseLite. It rounds only the final LCD displacement, caps displacement
at 64 horizontal and 48 vertical pixels, and translates the existing scene
buffer in place. Hits move with their scene. The persistent top bar remains
fixed. There is no second frame buffer, heap allocation, gameplay RNG use,
or save-format addition. The 48-cue presentation object is less than
768 bytes; future cues retain chronological order and same-command punch
replacement. Trauma is reset when a different scene target is bound; that
is an explicit bounded presentation adaptation.

The source audit covers `NScreenShake`, `ScreenPunchInstance`,
`NScreenshakePaginator`, and the concrete command/VFX owners below. It keeps
the already implemented damage-result pulse. New triggers include settings
preview, character selection, Power arrival, the explicitly mapped card VFX
families, the audited monster heavy/scream paths, ordinary and event Smith,
Girya/Lift and Clone, Dense Vegetation, Punch Off, Amalgamator, Foul Potion,
and observed run-history previews. The 38 card keys and their actual delayed
VFX timings are listed in `include/screen_shake_sources.hpp`; a blocked
Bludgeon still constructs its authored heavy effect.

### Doom

`NDoomVfx` begins with a Weak/Short camera punch. The port receives a
presentation notice at the actual Doom execution branches: player-side end,
enemy-side end, and the immediate End of Days check. Merely having a large
Doom counter, or observing a later HP decrease, is not a substitute trigger.
The notice does not enter run statistics or the canonical save.

The original direction is 180 degrees with small chaotic jitter. The LCD
uses its deterministic 180-degree centre without consuming gameplay RNG.
When the player dies, the combat scene remains the camera target through
the complete 300 ms pulse before the terminal room takes ownership. This
fixes the case where a one-tick combat linger discarded the effect before
the next rendered frame. The Doom shader/destruction particle family is
not newly claimed as desktop-identical by this camera work.

### Architect

These hooks follow actual final-room entry and the successful final
`original_event_leave`, after the world controller commits completion.
They never infer entry from rendering or replay during Continue. The source
references are `TheArchitect.AnimPlayerAttackIfNecessary`,
`AnimArchitectAttackIfNecessary`, `PlayCurrentLine`, `WinRun`, the five
character `GetArchitectAttackVfx` lists, `CreatureCmd.TriggerAnim`, and
`Cmd.Wait`.

| Sequence | Normal mode | Fast mode |
| --- | --- | --- |
| Five player hits | 100, 300, 500, 700, 900 ms | 50, 200, 350, 500, 650 ms |
| Four player hits | 100, 300, 500, 700 ms | 50, 200, 350, 500 ms |
| Silent opening four hits | 850, 1050, 1250, 1450 ms | 800, 950, 1100, 1250 ms |
| Architect FireBurst camera cue | 500 ms after the player group's 2 s tail | 250 ms after the same 2 s tail |
| After the Architect cue | 500 ms before the terminal intro | 500 ms before the terminal intro |

Ironclad, Defect and Regent have five player hits. Silent and Necrobinder
have four. Silent starts all her dialogues with the player group; her later
two dialogues end with the Architect alone. Nonfinal player hits are
Weak/Short, the final player hit is Strong/Normal, and the FireBurst camera
cue is Weak/Normal. Fixed `Cmd.Wait` intervals do not get halved in Fast
mode; only `CreatureCmd.TriggerAnim` changes its wait.

The Silent's opening dialogue waits for this source sequence before exposing
Proceed. Both ordinary keyboard confirmation and refreshed pointer hits
respect it. The final sequence retains the original event backdrop even
after the canonical event becomes inactive. Its terminal black fade starts
after the sequence; future timestamps clamp to age zero and tolerate
32-bit clock wrap. Camera Off changes displacement, not sequence completion.
Return, Continue and terminal reload clear transient cues.

This is the corresponding camera sequence on the retained event backdrop.
It is not a claim that every character's attack sprite, damage-number VFX,
Architect animation track or FireBurst GPU particles were reconstructed.

## Sleeping creatures and entry scope

Slumbering Beetle and Lagavulin Matriarch now use their actual presented
`IsAwake` facts to start and stop the source sleeping emitter. Four rising
Z particles emit once per second with the source 3.6–4.0 second visible
lifetimes, accompanied by bounded stars and the brief pink/purple Glow.
After waking, new emission stops while already emitted particles finish.
The source node's later 5-second cleanup is not misreported as a visible
5-second particle lifetime.

Source `SleepVfxPos` anchors account for the parent Visuals scale of 0.3;
editor-only preview translation is excluded. Both original 128×128 English
and Japanese Z canvases are retained as 32×32 masks without cropping the
pivot. Centre sampling keeps tiny LCD glyphs visible. A four-pixel minimum,
integer trajectories, bounded star sampling and reused Glow are documented
LCD approximations. No GPU simulation is implied.

The beetle's third hit schedules its Stunned move. It wakes when that move
is actually presented, after both preceding Bowlbugs' queued moves in its
authored encounter. Lagavulin wakes on actual unblocked damage. The gate
checks these different lifecycles rather than immediately showing future
canonical facts.

The original bot `FallControl` entrance was already retained by the enemy
asset converter and is not replaced by another generic fall. Existing
Spine entrance/formation/body timelines remain. Reviewing 61 monster
`AfterAddedToRoom` overrides showed that these are frequently power setup,
not 61 missing independent entrance animations. The separate inventory of
65 VFX scene families is not an all-families verification claim. Per-monster
GPU/material variants and a differential audit of every command delay
remain outside this completed, explicitly named scope.

## Resources and bounds

The optional final catalogue contains auxiliary entries 2198–2200:
Entomancer's phobia swarm texture, English sleeping Z and Japanese sleeping
Z, with endpoint 2201. The three original masks add 2,284 shared bytes over
the startup-logo resource stage. All raw original artwork and generated
packs remain private. Phobia swarm has its separate implementation and
acceptance report; this report does not duplicate that agent's claim.

The sleep timeline uses 12 bytes per enemy. Camera translation uses the
existing framebuffer. Doom/Architect append observer notices and reuse the
existing camera cue array; they add no fields to `App`, `State`, or the
save codec. Overall launcher allocation and device timing belong to the
parent's final integrated ARM/Firebird receipts.

## Verification

The pre-Doom/Architect complete camera/sleep gate passed **1,988,846 checks**
against the real final English and Japanese masks. Its journal is
`screen-effects-actual-wake-boundary-ffd0e577c6d0`, with
`source_changed=false`, verified checkpoint
`44ebf0ea6265df4f064d58bb999cb40d0f2ea059`. A separate receipt compared all
503 production `src`/`include` files to that root checkpoint and found no
differences. It covers independent source equations, noise continuity,
in-place pixels/hits, settings, ordinary Smith and Lift, blocked card/Power
hooks, actual sleep/wake and unchanged canonical saves.

The final extension gate passes **2,122,118 checks**, including real Doom commands, all five characters'
normal/Fast Architect dialogue and terminal lifecycle, Silent branches,
Camera Off, stale pointer/keyboard input, Continue/reload, clock wrap and
actual localized terminal-background pixels. The verified checkpoint is
`169cea998135a47e5f8580b15df798244e0e7e6a`; target build journal
`screen-effects-doom-retained-build-b09f0b9c1002` and complete actual journal
`screen-effects-complete-actual-75aed13c3f47` both passed with
`source_changed=false`. The test source is `14695dbe334702f01e349ac5f10e473d2dac58a7`.
All 503 production source/header files match the integrated checkpoint.
The frozen root44 archive supplies unchanged objects; the three changed
core/original-events/input objects were recompiled, and the test includes
the current complete `app.cpp`, including the final player-Doom lifetime
fix. The private `final-matched-core-receipt.json` records library/object,
executable and source identities.

The earlier failed journals are retained. In particular,
`screen-effects-doom-boundary-d91683f26c87` proves the real player-Doom bug:
the punch existed at tick 30 / 990 ms, but the old linger ended at tick 31
and rebound the camera before its next frame. The final runtime preserves
that pulse, and the gate requires at least nine visible 33 ms observations
in a real world encounter before the actual death screen takes over.

These are native correctness, source-timing and pixel checks. They do not
measure calculator performance or replace the final Firebird session.
