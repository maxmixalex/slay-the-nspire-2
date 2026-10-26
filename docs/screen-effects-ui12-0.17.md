# UI12 screen motion, sleeping creatures and Architect actors

Reference: original v0.111.0 / `41cef1ea`. This report records the audited
camera hooks, sleeping-creature effects and the later Architect actor layer.
It does not promote all monster
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
replacement. The subsequent UI13 stage retains the independently decaying
trauma object across target changes, matching `NScreenShake.ClearTarget`; only
punch/rumble and target-owned future cues are cleared. See
`ui13-combat-feedback-0.17.md` for the clock rebase and executed-stage status.

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

The original direction is 180 degrees with small chaotic jitter. UI13 adds
the same ±10-degree range from an independent cosmetic hash, without consuming
gameplay RNG.
When the player dies, the combat scene remains the camera target through
the complete 300 ms pulse before the terminal room takes ownership. This
fixes the case where a one-tick combat linger discarded the effect before
the next rendered frame. The subsequent UI13 stage implements the frozen
hurt body, bounded sinking viewport, original front/back particle textures,
animated flame material and overlay from `DoomPower.StartDoomAnim` and
`NDoomVfx`. Its exact source cases, RAM bounds and validation status are in
`ui13-combat-feedback-0.17.md`.

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
| First dialogue gate, other characters | 2250 ms | 2250 ms |
| First dialogue gate, Silent | 3950 ms | 3750 ms |
| Architect FireBurst camera cue | 500 ms after the player group's 2 s tail | 250 ms after the same 2 s tail |
| After the Architect cue | 500 ms before the terminal intro | 500 ms before the terminal intro |

Ironclad, Defect and Regent have five player hits. Silent and Necrobinder
have four. Silent starts all her dialogues with the player group; her later
two dialogues end with the Architect alone. Nonfinal player hits are
Weak/Short, the final player hit is Strong/Normal, and the FireBurst camera
cue is Weak/Normal. Fixed `Cmd.Wait` intervals do not get halved in Fast
mode; only `CreatureCmd.TriggerAnim` changes its wait.

The first dialogue waits for the source reading sequence before exposing
text or Proceed. `PlayCurrentLine` has a fixed 750 ms lead, another 1000 ms
when there was no player attack, and a fixed 500 ms stop-reading interval.
The Silent instead finishes her attack group and its 2000 ms tail before
that same 500 ms interval. These waits share the existing `SequenceEnd`
gate; generic event text/button fades do not substitute for them. Both
ordinary keyboard confirmation and refreshed pointer hits respect the gate.
The final sequence retains the original event backdrop even
after the canonical event becomes inactive. Its terminal black fade starts
after the sequence; future timestamps clamp to age zero and tolerate
32-bit clock wrap. Camera Off changes displacement, not sequence completion.
Return, Continue and terminal reload clear transient cues.

### Original actor layer

The subsequent source stage `8aaeab5` / `70c488e` / `19fee6c` / `f3da494` closes a real
missing layer: the event background contains the environment and decorative
desk robot, not the player or Architect. The already installed original
Architect attack, hurt and idle clips are directory entries 49, 50 and 51.
Their original durations are 1500, 733 and 5333 ms. The five original player
idle/attack clips were already installed too. This stage adds no art,
resource directory entry, persistent cache, `App` field or canonical field.

`TheArchitectEventEncounter` has one enemy with no named slot and the
default camera. `NCombatRoom` therefore places the actors at source
(-480,200) and (480,200), relative to the 1680×1260 viewport centre. The
existing 4/21 projection and baked sprite registrations produce LCD bases
near (69,158) and (251,158); the normal combat hand-clearance lift does not
apply in this event. Necrobinder retains her source 150-unit left shift.
`CombatEventVisuals.Allies` contains player creatures only, so this room
does not synthesize Osty from the previous boss's saved companion state.

The real event begin/final-Proceed clocks now sample each player's attack
and the Architect's hurt/attack, followed by the original looping idle.
Fast changes the source command wind-up, not the clip's frame duration.
The original dialogue frame is reflowed below the actor silhouettes with
the full text, existing draggable scrollbar and original Proceed control.
`TalkCmd` creates `NSpeechBubbleVfx`, whose alpha and visible-character ratio
rise together over 400 ms immediately after that line's origin. Architect
uses that clock instead of the unrelated static-event title/body delay.
The option has a different source owner: `NEventOptionButton.AnimateIn`
waits 500 ms and animates 500 ms in Normal mode, or 250 ms plus 250 ms in
Fast mode. Its input gate therefore remains 1000/500 ms after the line;
the Proceed image starts entering with the line rather than starting a
second 500 ms entrance after the gate is already ready. These samplers all
reuse the existing event start timestamp.
The head/reading overlays are not included in these three body clips.

At the end, `NGameOverScreen.MoveCreaturesToDifferentLayerAndDisableUi`
retains the VisualOnly creatures above its backstops. The complete caller
chain matters: `RunManager.WinRun` calls `GuaranteeKillAllPlayers`, which
starts the player's death after the Architect attack sequence. The port
therefore hands that same left-side pose to its existing original death
clip at the terminal origin, with no second idle body or centred copy.
The Architect continues its attack tail and idle at its original position.
Restoring a completed run does not infer and replay the transient attack
from a newly opened summary timestamp.

### Exact scope of remaining effects

These are distinct from an unknown desktop pixel comparison:

| Source effect and actual owner | Current status |
| --- | --- |
| `DoomPower.DoomKill` at a lethal side-end or `EndOfDays`; `StartDoomAnim` / `NDoomVfx` freezes the hurt body and sinks its viewport after 750 ms, with front/back particles | Implemented by the subsequent UI13 stage; see `ui13-combat-feedback-0.17.md` for its separate source references and validation status. |
| `TheArchitect.OnRoomEnter` and the first `PlayCurrentLine`: `head_reading`, `head_stop_reading`, `head_normal` on a second Spine track | Initial source waits are implemented. These separate overlay clips are inventoried with `used:false` and are not packaged; the new actor layer does not claim them. |
| `NArchitectVfx` reacts to the attack's `trail_start` and idle's `trail_end` | The original body clips play. The independent inner/outer `NBasicTrail` is not reconstructed by the frame renderer. |
| `TheArchitect.AnimPlayerAttackIfNecessary` creates score-derived damage numbers, a hit spark and each character's shuffled attack VFX | Original player attack and Architect hurt are now sampled; the corresponding camera pulses were already verified. The per-hit numeric/particle set is not implemented by this actor stage. |
| `TheArchitect.AnimArchitectAttackIfNecessary` spawns attack lightning and `NFireBurstVfx` on the player | Architect attack and the real Weak/Normal camera cue are implemented. Its lightning/FireBurst particles remain missing. |

Integer noise, bounded LCD displacement, the readable subtitle reflow and
the sleeping-particle mapping are implemented platform adaptations. The
separate 65-family inventory remains an uncompleted differential review;
it is neither a list of 65 proved missing functions nor an all-family pass.

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
existing camera cue array in the original UI12 stage. UI13 additionally adds
a bounded Doom presentation object to `App`; `State` and the save codec remain
unchanged. Overall launcher allocation and device timing belong to the
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

The earlier camera/sleep extension gate passed **2,122,118 checks**, including real Doom commands, all five characters'
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

The initial Architect source stage ends at
`f3da49429efce3d7ee25da6a933471a2007202ac`, integrated and independently
read back at public checkpoint `defe52b321f60faccb13325f5122cd5e34ad72c2`.
Complete `app.cpp` / existing
screen-effects-test translation units and `original_events.cpp` pass the
Clang 18 C++17 syntax check; `git diff --check` passes. That is a buildable
source checkpoint, not acceptance of its new actor pixels or changed first
dialogue waits. Its independent real-resource actor gate, integrated native
execution and final ARM/Firebird receipts are recorded separately when they
finish. The 2,122,118-check result above remains the exact prior runtime169
camera/sleep/lifecycle evidence and is not silently relabelled for this patch.

### Actual-resource failures and narrow corrections

The first independent actor gate was run against real packs, through the
ordinary event entry and final Proceed. It found an actual visual failure:
the older-save compatibility message was opaque while the terminal origin
was still in the future, and during the first 200 ms of the black fade.
Journal `architect-visual-actual-a5b7a0fe7872` is retained as **failed**, exit
1, `source_changed=false`; its independent two-actor pixel band differs at
609 pixels for Ironclad at terminal age 200 ms. Commit `69672d3` retains the
message but gives it the same intro-delay plus 2000 ms opacity ramp as the
ordinary victory quote, so it does not hide the ending sequence.

Inspection also found a blank dialogue panel. The event's source
`LocTable` is `ancients`, but `App::original_event_text` had read its valid
`THE_ARCHITECT.talk.*` key from `events`. Commit `356ddab` reads this event's
body, authored per-line next label and final Proceed from `ancients`, keeping
the existing bound-value formatter. A line-specific next label now takes
precedence over the source Continue/Respond fallback. The required strings
were already installed; this fix adds no text or artwork to a resource pack.
All 22 English/Chinese next labels were inspected; the longest English
labels are eight letters and the Chinese labels are two characters.

The corrected production and independent bilingual dialogue/scroll target
are publicly checkpointed at
`9398633d8fd715c008560cc038f61d8f85c98466`, tree
`1b7a17a1e7734d5c9edddbcf5a839abca7236356`. At the preceding source-document checkpoint,
the corrected actual-resource run, integrated 135-target native execution
and final ARM/Firebird receipts were still pending. The failed actor-band
assertion was retained; no actor area or compatibility text was removed to
turn that failure into a pass. The generated entry fixture supplies earlier
room outcomes through the world controller and is not represented as three
ordinary played acts or a new performance benchmark.

### Closed bilingual actor acceptance

The corrected actual-resource actor gate now passes **7279 checks in English**
and **7279 in Simplified Chinese**: journals
`architect-visual-complete-eng-93498ff2c976` and
`architect-visual-complete-zhs-204837870e07`, both exit 0 and
`source_changed=false`. The verified test checkpoint is
`1848338c68d4f10e9fa46d414729087fbd0b1224`. The private
`source-match-939-receipt.json` compares all 504 production source/header
files to runtime checkpoint `9398633` with no differences; later commits
change tests only. Build journal `architect-visual-ui-clock-build-c2e8f30483f3`
passed. The matched archive replaces the changed `original_events.cpp`
object, while the test compiles the complete current `app.cpp` directly.

The two languages cover 20 combinations: all five characters in Normal
and Fast mode. Checks include the literal source anchors and original
sprite texels, all three Architect body clips, repeated player attacks and
Architect hurt, the initial source gates, nonempty localized dialogue and
authored next labels, mouse/keyboard text scrolling to the actual end,
unchanged canonical saves, no residual Osty, original-position player
death, and the complete two-actor pixel band during the terminal fade.
The 200 ms band that originally had 609 extra pixels now matches exactly.
Actual English/Chinese Necrobinder two-line captures also show readable
bottom glyphs and the full text after scrolling; no runtime geometry
change was needed to force the test through.

The subsequent fixture corrections preserve the ordinary rejection of
terminal Continue, use the production codec for legacy terminal reload,
keep the Silent's 1867 ms death clip separate from the 1950 ms terminal UI
gate, and compare scroll endpoints in consistent fixed-point units. None
changes runtime939. Check totals include fixture routing and capture row
writes; they are not a count of restored game functions. Complete-suite
and ARM/Firebird acceptance remain separate integrated release receipts.


### Closed integrated follow-up

The delivered runtime is `9398633d8fd715c008560cc038f61d8f85c98466`. The root
native build and current English/Japanese screen-effects gate pass with
**2,122,238 checks** (`ui12-architect-actual-screen-effects-cbb2ef00c2da`).
Test checkpoint `1848338` differs from that runtime only in the independent
Architect regression source. Complete registered acceptance is **135/135**, plus
**310 Python tests** and tool compilation. The delivered ARM is 1,600,738 bytes
and requests 20,363,472 bytes; its source remained unchanged during construction.

The ordinary Ndless/Firebird follow-up `55becdaab6a4` closes normally: saved-run
Continue/SaveQuit/OS, a separate native-generated original-entry Ironclad
Architect fixture through dialogue/actions/ending/summary/MainMenu/OS, and exact
USB restoration of the ordinary save/progress. It has zero resource failures.
The native source/pixel gates cover exact clocks; device screenshots show the
performed interaction and visible actor changes without pretending that a cold
frame's control-wait label is its exact animation age. This is not a new
performance benchmark or a natural three-act playthrough. Detailed identities
and the preserved earlier failures are in `validation-ui12-0.17.json`.
