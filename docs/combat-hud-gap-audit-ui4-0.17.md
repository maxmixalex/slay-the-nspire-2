# Combat HUD: remaining original presentation differences after UI4

## Scope and evidence

This is a read-only review of runtime
`9a554fe04e63b35546be708a81c85f41864a307f`, using the pinned original
**v0.111.0 / 41cef1ea** assembly and scene resources. It is a next-work list,
not an implementation or a new acceptance result. The audited runtime remains
unchanged. No original assembly, scene, font, screenshot or pack is included in
this report's commit.

Original evidence below names methods in `sts2.dll` and paths inside the PCK.
The current source locations refer to the frozen runtime above. Existing actual
v21 native captures were inspected, including `energy-ironclad`,
`energy-zero-ironclad`, `combat-real-clock` and `hand-player-draw`, plus the UI4
card-motion captures reviewed separately. These use real bilingual resources
with regression-controlled game states; they are not original-PC screenshots
or additional ordinary emulator runs. Static frames cannot establish a timing
curve. Animation differences below are supported by the original methods and
the current production code.

No new operation-blocking defect was established. There is one confirmed
information omission in compound enemy intents, followed by four presentation
or discoverability gaps. None requires changing canonical combat state, saved
data, gameplay RNG, or startup resource reading.

## 1. Compound intents need an independent value label per part

**Confirmed; highest priority.** The original
`NCreature.UpdateIntent` creates/updates one `NIntent` per member of
`Monster.NextMove.Intents`. Each node has its own `IntentHolder/Value` in
`scenes/combat/intent.tscn`. `NIntent.UpdateVisuals` supplies the label for either
an `AttackIntent` or a `StatusIntent`; `StatusIntent.GetIntentLabel` binds
`CardCount` to `intents.FORMAT_STATUS_CARD_COUNT`.

A concrete original case is `MechaKnight.GenerateMoveStateMachine`:
`FLAMETHROWER_MOVE` has both `SingleAttackIntent(FlamethrowerDamage)` and
`StatusIntent(4)`. The port's `src/enemy_move_definitions.inc` correctly retains
flags 17 and status count 4 for this move. The omission is in rendering:
`src/world_intent_ui.inc`, `native_enemy_intent`, draws every icon, then draws
the attack number at the **whole group's** center. The status count is in an
`else if`, so it is not drawn when an attack flag is also present. The same
current data shape occurs in Soul Fysh's Gaze and Vantom's Dismember.

This causes two visible differences. The damage label does not belong to the
attack icon's own layout area when another icon is present; the existing
attack-plus-block capture demonstrates that grouping. Attack-plus-status also
loses the number of added status cards. Tooltip formatting still has the
retained numeric data; this is not a wrong combat calculation.

**Smallest next change:** lay out icon and value together for each part in
`native_enemy_intent`. Render attack and status values independently, preserving
the existing smaller repeat suffix and the same retained intent/owner used by
hover text. Do not describe the original as a group-centered label: the scene's
value belongs to its individual holder and has its own offsets. Keep the
calculator's minimum readable text size while preventing neighboring values
from colliding in crowded encounters.

**Required verification:** actual bilingual attack-plus-block, attack-plus-
status and repeated-hit captures; specifically verify that Flamethrower shows
both its damage and 4. Cover several neighboring enemies, hover/target regions,
retained intent during queued effects, death and reused enemy slots. Preserve
canonical save bytes. The new compound-status capture has not been executed in
this review.

## 2. Intent motion and visibility still use a static icon and an abrupt cut

**Confirmed.** `NIntent._Process` plays the authored animation at 15 frames per
second and moves its holder by a sine with speed PI, distance 10 and offset 8
in original scene units. `PlayPerform` emits the original one-shot particles.
`NCreature.PerformIntent` freezes the current value and starts its hide tween.
The exact original timing matters:

| Original operation | Time relative to PerformIntent |
| --- | --- |
| Emit the intent particles and freeze its value | Immediately |
| `AnimHideIntent(0.4)` starts fading | After a **0.4-second delay** |
| Alpha tween to zero | **0.5-second duration**, after that delay |
| Await before the enemy command | 0.4 seconds Normal / 0.25 seconds Fast |
| A later `RevealIntents` | Kills the previous fade, begins a 1-second reveal with a cosmetic 0–0.3-second delay |

`0.4` is the hide method's delay argument, not its fade duration. The original
Instant mode has an immediate-hide branch; the port's Normal/Fast distinction
must not treat Fast as Instant.

Current `src/world_intent_ui.inc` draws a retained static picture at fixed Y.
`src/combat_feedback_ui.inc`, `combat_feedback_move` and `combat_intent`, mark
that intent hidden at `hide_at = start` and return an empty intent at that
time. No separate fade, bob, frame or perform-particle view is retained.

**Smallest next change:** add bounded per-actor intent presentation state on
the existing display clock. First preserve the frozen label/icon through the
authored delay and fade, including interruption by reveal, death or slot
replacement. Add the small bob using the existing integer trigonometry. Import
the authored frame sequence and perform sprites as a separate coherent asset
stage if needed. Any cosmetic reveal offset must use an independent display
seed, never the saved gameplay RNG.

The display fade must not extend the enemy command wait or serialize the
enemies behind the banner. It also must not resurrect stale intents after a
slot replacement or prematurely display the next move.

**Required verification:** Normal/Fast boundaries immediately before and after
400 and 900 ms, interrupted reveals, multiple enemies, death, reused slots and
Fast commands that finish before an old fade would complete. Check visual/hit
agreement and exact state/save equivalence. Existing tests which deliberately
expect an immediate empty intent will need evidence-based replacement when
this behavior is implemented, not just relaxed assertions.

## 3. The energy counter has no original hover explanation

**Confirmed discoverability gap.** `NEnergyCounter._Ready` constructs a
localized hover tip from `static_hover_tips.ENERGY_COUNT.title` and
`.description`, with the character's `energyPrefix`. `OnHovered` displays that
tip above the counter, and `OnUnhovered` removes it.

Current `src/combat_ui.inc`, `combat_energy`, draws the 42-pixel counter and
current/max label but creates no energy hit region or hover action. These
original localized strings already have entries in `src/world_text_index.inc`;
the missing part is the control/tooltip path, not a need for another full text
pack. An unfamiliar player cannot inspect this HUD element as they can inspect
nearby powers or card keywords.

**Smallest next change:** register the visible counter's region and display
the original localized tip through the existing tooltip renderer. Preserve
class energy-icon token formatting. Make it reachable through the calculator's
existing inspection/navigation convention as well as a pointer; do not add a
new unrelated navigation system.

**Required verification:** both languages, all five character prefixes,
zero/positive energy, long wrapped text, and keyboard/pointer dismissal. Check
that this region does not steal the focused card, draw-pile or battlefield
actions. Confirm text contains no raw template fields.

## 4. Energy gains and counter entry/exit lack their original visual feedback

**Confirmed presentation gap.** `NEnergyCounter.OnEnergyChanged` restarts both
the back and front VFX when energy increases. The character counter scenes,
for example `scenes/combat/energy_counters/ironclad_energy_counter.tscn`, contain
the two corresponding particle containers. `NEnergyCounter.AnimIn` and
`AnimOut`, invoked by `NCombatUi`, move between the show position and
`(-480, 128)` with independent 0.6-second tweens: EXPO_OUT on entry and BACK_IN
on exit, in the original coordinate system.

Current `src/combat_ui.inc`, `combat_energy_layer` and `combat_energy`, always
draw at `(6,174)` with a `(27,195)` pivot. They implement the base layers and
rotation, but not energy-gain emitters or these entry/exit transforms. Gaining
energy therefore changes the correct number without the original burst, and
the counter does not follow the original combat-HUD lifecycle motion.

**Smallest next change:** separate the shared counter transform from layer
drawing and drive the original 600 ms lifecycle on App presentation state.
Then observe displayed energy-increase cues to restart the authored bounded
VFX, using their real sprites/scene parameters rather than an unrelated flash.
Do not trigger a future gain merely because eager canonical settlement has
already reached the next turn. Entry/exit coordinates need a deliberate
320-by-240 mapping, not direct use of original pixel offsets.

**Required verification:** ordinary combat entry and victory exit, zero-to-
positive and positive-to-higher energy, no burst on decrease, multiple queued
gains, a modal choice during the queue, and all five classes. Check that neither
the counter movement nor the VFX blocks subsequent gameplay commands. Actual
ARM cost and allocation need measurement after implementation; this review
does not assert the cost of unimplemented particles.

## 5. Turn-banner typography and outlines still differ from the scenes

**Confirmed representation difference; layout tuning must retain small-screen
readability.** `scenes/combat/player_turn_banner.tscn` uses Spectral Bold at 96
for the main label and Kreon Bold at 36 for the turn number. The title and
counter have semi-transparent black outlines of 24 and 16 original pixels.
The enemy scene uses the same main-label family, size and outline treatment.

Current `src/turn_banner_ui.inc`, `turn_banner_frame`, uses the generic bitmap
glyph path at sizes 16 and 10 and a one-pixel down-right black shadow. There is
no equivalent surrounding outline or separate font-family selection. The
resource builder's `tools/build_world.py:glyphs` has one font path and keys
glyphs by codepoint/size. The resulting main/counter size ratio is 1.6 rather
than the scene's 96/36. This does not mean that a six-pixel counter would be a
good calculator adaptation; it identifies a remaining visual hierarchy and
style difference, not permission to sacrifice legibility.

**Smallest next change:** reproduce the intended font family/style for the
small fixed set of banner labels and the turn-number glyph subset, with a
scaled surrounding outline. A small localized label atlas or styled glyph
subset can avoid a second complete font pool. Tune main/counter spacing and
size together at 320 by 240, preserving a readable counter and the current
center pivots. Keep the already implemented timing and color paths.

**Required verification:** Chinese and English Player/Enemy/Extra Turn labels,
one- and multi-digit turn counts, bright/dark rooms, partial alpha and enemy
2-to-1 zoom. Review overlap with actors, hand, tooltips and each other; a larger
title is not accepted just because it has a more similar numeric font ratio.

## Already implemented and not to be reopened as missing

- Attack art levels select by total damage at the original 5/10/20/40
  thresholds. Damage and repeat count are separate; the multiplier is already
  smaller, and the damage uses cream with a black outline. Compound anchoring
  and the omitted status count are the specific remaining problems above.
- The energy label already shows **current/max**, including a changed maximum
  rather than a fixed three. The five class counters, Necrobinder's three-layer
  variant, red zero-energy label, darkened zero-energy layers and rotation
  rates of 30/60 degrees per second (5/10 at zero) have explicit production
  paths. Existing actual v21 images show readable 2/4 and 0/4 labels. This is
  not a claim of identical original font/shader pixels.
- `include/turn_banner_presentation.hpp` already has the original independent
  banner durations, enemy 2-to-1 zoom, player opposing label motion, enemy
  gold-to-red color and separate fades. Normal/Fast command waits are separate
  from these clocks. Enemy actions begin while the banner is still present;
  the next hand no longer appears as one future batch. These are completed
  behavior improvements, not remaining gaps to be masked by longer delays.

## Review limits and recommended order

Implement item 1 first because it restores missing decision information. Item
3 is a small usability completion. Item 2 is the largest clearly sourced
intent-motion difference; keep it independent of command timing. Items 4 and
5 can then restore energy feedback and banner style without enlarging the
startup path or copying another full resource/font pack.

The screenshots reviewed here do not establish that all 5–8-enemy layouts,
all actor-specific intent anchors or every bright background are already
readable. Possible crowding in those unreviewed combinations is a follow-up
question, not a confirmed defect from this audit. Likewise no universal
original-UI percentage, original-PC pixel equivalence or physical-calculator
frame rate is claimed. The five entries above remain **unimplemented** at the
audited runtime and must be described that way in the current delivery.
