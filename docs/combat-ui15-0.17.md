# Combat UI15 integration — implementation and acceptance

Reference: v0.111.0 / 41cef1ea. The complete private C# and PCK inputs were
restored before this work; neither original input nor decoded art belongs in
the public repository. Historical failures and intermediate pending notes
below retain their original stage identity. The completed gates in this
section supersede those intermediate pending states.

## Completed integrated acceptance

The integration9 baseline (`a04451c07e1d63fa50cf95466381fce2a0d96ff8`)
passes all140 native suites, including the repaired Decimillipede/world-UI
fixtures, the complete command directory and actual-resource combat gates.
Its original ASan/UBSan sweep retains four real failures in one intermediate
enemy-clip pointer expression. The production expression and generator were
fixed together; the v14 delta passes18/18 native and18/18 sanitizer suites,
including all four original triggers. This does not relabel136/140 as a clean
full sanitizer sweep.

At integration16 (`a94050d53ccd8c437be69dd42ebeb10e0f507b8f`), all20 selected
native and20 sanitizer suites pass, with nine actual-resource paths in each.
They include real ENG/ZHS deck selection/upgrade paths, Spark, screen effects,
Vantom attack/death/shrink/restore and all16 languages. Vantom's smaller6,596
count removes screenshot-file byte-write assertions; its actual effect and
state cases are retained. Later merchant-only changes do not establish a new
full combat sweep. The public [validation manifest](validation-ui15-0.17.json)
records exact source, resource and executable identities.

Ordinary Firebird CX OS acceptance includes a legal new Ironclad run through
Neow, the first three-Slime victory, rewards, Proceed, SaveQuit and Continue.
Its7590-byte save is unchanged after action-free Continue and a second save.
At `bd158d205d877f767f59fe2a1eb33874cb925899`, a50ms card selection,50ms
target change and one50ms Enter hit the intended middle Slime26→20, consume
one energy and preserve selection through release. Two normal OS exits and a
fresh Continue preserve the same7430-byte save/backup. These are actual
ordinary OS operations, not direct state writes during the action.

The controlled Haunted Ship case at integration16 executes the real enemy
turn with full strings15 resources. Screens show five Dazed growing from
small faces at2875ms to full size at3000ms, holding and flying into Discard at
4375–4500ms. Production decoding after normal SaveQuit/OS exit finds exactly
five Dazed,15combat cards,10deck cards,playerHP80 and enemyHP63. The7320-byte
save is identical to v14's same action. This is a controlled fixture rather
than a natural-run encounter.

The measured0–6s action is13.4592FPS at CX132MHz. Its first2s include hand
discard, the enemy-turn banner and three frames over250ms (maximum293.9453ms).
The later growth/hold and flight windows have no frame over250ms. Input
capture and visible growth are verified; universal smooth30FPS, physical
calculator timings and a natural three-act completion are not claimed.

## Real command sequencing

`extract_enemy_command_program.py` rejects an unknown awaited expression and
extracts the surrounding commands for every configured original enemy move.
The resulting directory contains **328 rows: 324 named source move rows and
four Task.CompletedTask placeholders**. The earlier 327 named-move count
included three duplicate rows for the unpublished LegacyNibbit adapter. Those
rows are now excluded from extraction and runtime timing. The retained program
uses 546 deduplicated steps and 113 trigger names.

Commands are consumed by real core callbacks, including nested power changes,
removal, block, healing, spawning, stealing and direct HP changes. Attack
commands retain per-hit timing and one millisecond accumulator; skeleton clip
lengths do not manufacture waits. Source conditionals retain their actual
facts, including curled louses, off-balance bowlbugs, both Test Subject revives
and Vantom's live-creature branch. Generated cards retain each actual source
batch's hand/non-hand wait and mixed-pile previews.

The independent follow-up audit identified and repaired these concrete paths:

* Two-Tailed Rat waits 500ms only when a real summon slot exists. A full board
  retains the 300ms normal/150ms fast Summon trigger, with no invented tail.
* Gremlin Merc's first line follows actual Gimme execution. Whistle can cancel
  the selected intent; selecting that intent again does not count as having
  spoken. An existing model data cell stores the actual boolean, captured
  before the move body for this presentation.
* Block and healing await their visible result before AfterBlockGained and
  AfterCurrentHpChanged hooks. Ordinary event/shop healing observes the
  requested amount; history still records actual HP gain. Ancient entry is
  a separate source path, detailed below.
* Victory/death can clear the core queue before EnemyMoveFinish. That terminal
  path cancels the unexecuted presentation suffix and card-batch marker.
  Commands skip waits while the real combat-ending flag is set.

## Enemy-created cards

Enemy generation passes the original null-creator semantics to the shared
notification path. It no longer activates the player's Regalite, Arsenal,
Pillar of Creation, Smokestack or Trash to Treasure, discounts Rocket Punch,
or increments Supermassive's player-created-card history. Aeonglass's Wither
level hook and card-entered-combat hooks still execute. Ordinary player cards,
potions and relic creation paths retain their own notifications. Regalite's
own block command retains its explicit fast-block wait (30ms normal/0ms fast).

The existing preview already grew from zero for 250ms. A cold face decode on
its first visible frame can consume that entire interval. The new combat
render preparation resolves at most one already-committed upcoming generated
face per frame into the existing HandTile cache. It does not simulate future
RNG, allocate another face buffer, advance animation or paint a hidden card.
The completed ordinary-OS cold-action measurement above separates visible
growth from the preceding turn transition and reports its remaining cold frames.

## Other integrated presentation

The HUD uses the corrected zero-ascension behavior, independently spaced room,
floor, boss and time icons, source-based tooltip sizing, and the revised intent
sprite/number anchors. Card type labels use the smaller source projection.

Permanent deck upgrades notify their actual commit path, including random and
whole-deck Tablet of Truth upgrades. Upgrade views retain each card's own
facts. Event sparkles, the rest site's Smith renderer, and Sovereign Blade
forging are connected to real operations. Every relic acquisition/activation
keeps its own inventory flash. Large Capsule therefore flashes itself and the
two relics it actually grants; unrelated inventory is not made to flash.

Fullscreen healing uses four original private textures at auxiliary indices
5284–5287. Rooms owns Smith's 5275–5283 additions. Vantom's layered resource and
bone renderer belongs to the frontend UI15 stage and is integrated separately.

### Ancient entry healing

`AncientEventModel.BeforeEventStarted` calls `CreatureCmd.Heal` with animation
disabled and retains the actual HP gain. `NAncientEventLayout` schedules the
fullscreen effect only when that gain is positive, after a fixed200ms wait.
The earlier implementation note and test expectation that a full-health
Ancient should still show the effect were incorrect. The core now suppresses
the general heal notification for Ancient entry and notifies its real positive
gain with that200ms delay. Neow retains the delay when its room is revealed
after character departure. A full-health Ancient shows no invented healing;
an ordinary positive heal request at full health still retains its source
effect without adding healed HP to history.

### Explicit wake waits

The map-entry regression exposed a source await outside a monster move body:
`AsleepPower.AfterDamageReceived` and the natural wake path both await
`TriggerAnim("Wake", .6f)`. The core now supplies600ms normal/250ms fast at
those actual hooks. Room-entry relics can invoke them before a player action
binds the combat feedback trace, so the explicit request also advances the
entry enemy clock. The skeleton's duration remains independent. The route
regression now checks the exact requested duration rather than merely a
positive clock delta.

The following screen-effects acceptance also exposed an independent lifecycle
error. `LagavulinMatriarch.AfterDamageReceived` stops its sleeping effect even
when all damage was blocked and `IsAwake` stays false. The effect now consumes
that explicit stop callback, the natural wake callback, and the four attacking
move callbacks. A stopped emitter cannot restart when an older wake-animation
snapshot becomes current. Its existing four-second particles finish normally.
The additional latch fits the existing12-byte sleep presentation structure.
Slumbering Beetle's real stunned/natural WakeUp paths retain their own600ms
normal/250ms fast await and stop emission at their original callback boundary.

The complete world-UI sweep also checks damage hooks after their real power
command prefixes. Asleep's Wake follows removal of Plating; Flutter's final
decrement first removes the exhausted power. Flutter then retains its600ms
normal/250ms fast StunTrigger await and the separate, unscaled250ms Cmd.Wait.
Burrowed's GetStunned retains250ms normal/125ms fast; Plow's SetStunned retains
600ms normal/250ms fast. The corresponding suspended StillDizzyMove and
StunnedMove callbacks retain their own waits on the next enemy turn. These
awaits are requested by the real hooks, not inferred from animation duration.
Boundary tests retain damage at the earlier impact, delay the appropriate
trigger until its power prefix completes, and prevent the next TwinStrike
hit from overtaking a suspended damage callback.
The remaining callback audit restores Ravenous's500ms normal/250ms fast waits
at both DevourStart and DevourEnd. A real Corpse Slug death verifies each
survivor's sequential devour, its subsequent Strength application and its
later suspended-move continuation.
An explicit hook pause also captures its preceding HP/block mutation before
advancing the presentation clock. The real Tunneler block-break test caught
HP being held until the later Burrowed removal; damage now appears at the
impact while its stun animation and subsequent command remain awaited.
Death's bulk power removal also retains its distinct source operation:
`RemoveAllPowersAfterDeath` removes powers internally and awaits their real
`AfterRemoved` callbacks, without manufacturing PowerCmd.Remove's400/200ms
wait for every removed power. The Ravenous fixture checks the complete
sequential death callback interval, including this absence of an extra wait.
Ravenous and Crab Rage also emit their explicit source `Flash()` before
applying the subsequent effects. The two surviving Corpse Slugs each retain
their own flash at their separate devour start; Crab Rage flashes before its
Strength/block/removal sequence.

### Vantom scene effects

The independent source event clock includes attack/debuff spray, the die
event's1s QuadIn dissolution, both death sprays, and the30-particle explosion.
The odd `death_spray_off` event in the ordinary attack clip is retained as
authored; it is not silently changed into `spray_off`. Complete composite die
sampling includes the delayed track1 charge-up callback without interrupting
the body death clip.

Four original emitters retain80/50/50/30 fixed slots, source lifetime and
speed, separate X/Y scale curves, source world/local coordinate modes, and
the explosion's ring, color gradient and random2×2 source texture frame.
The world emitters capture each birth transform once; their particles do not
move with a later skeleton pose. The local explosion follows its slot.
World gravity and texture scale are not multiplied again by the parent
Visuals scale or Shrink. The CPU integration is analytic at the LCD clock,
with a separate cosmetic seed; it does not claim identical GPU random output
or frame-by-frame Euler integration. Four subpixel samples retain fine spray
coverage without enlarging the source9×20 texture. These facts follow
[Godot4.5.1's particle material](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/resources/particle_process_material.cpp).

The dissolve noise is the original512×512 ValueCubic field: seed0,
frequency.063, five-octaveFBm, normalized to8-bit grayscale. The converter
uses the pinned Godot4.5.1 FastNoiseLite header and the original material's
equation. The skeleton conversion retains separate UV masks for `tail`,
`tail_hook` and `megablade`; it does not fade an entire flattened creature.
Particle rendering, those masks and continuous skeleton scale are covered by
the complete-pack stage7 acceptance below; final compressed-pack and device
measurements remain separate gates.

Actual animator transitions also advance the effect clock. A short idle
between two painted frames therefore retains its original `spray_off` event.
Tail-only composite requests retain the idle body's events, and extended
charge-up composites switch their event clock at the body's original667ms
completion while the longer tail animation continues. Presentation reset
clears both Vantom scale motion and all particle state for the same run and
combat identity.

The generic creature-scale cache is explicitly cleared by reset_presentation.
Continuing the same run with the same combat identity and a monotonic clock no
longer inherits the previous Shrink/Beetle Juice/default-scale tween. This
reuses the existing static cache and adds no App or State allocation.

## Regression design

`combat_ui15_regression` creates all 108 original actor kinds through actual
encounters, summons or death replacements. It forces each legal directory
move to isolate its command body at ascension 0 and 9, normal and fast, for
**1,312 executions**. Knowledge choices continue through the real selection
API; downed segments, Test Subject's third form and Waterfall Giant's final
phase first execute their real prerequisites. Tests compare production save
bytes between observed and unobserved cores, validate completed state, require
queue closure/no overflow, and check that presentation advancement leaves
canonical state unchanged. Ordinary move-selection/full-turn coverage remains
in the separate enemy regression; forced move coverage does not replace it.

Additional hand-authored source-millisecond cases cover ordinary command
boundaries, conditional Rat spawning, Gimme after stun, and respawn growth.
The paired generation fixture compares a real slime status move against a
real player Turbo play with all seven affected listeners active. Other cases
cover generated-card growth/preparation, full-health Ancient healing, Tablet
upgrades and independent relic flashes. The earlier UI14 scale test now uses a
same-run restore without artificially incrementing run_number.

`vantom_vfx_ui15_regression` requires a complete private pack and is never a
synthetic-art pass. It compares each of the five decoded particle textures
through the affine renderer against the ordinary picture renderer, then runs
real Ink Blot and death callbacks at normal and delayed render intervals. It
checks both death sprays, the30-particle burst, dissolve/restart timing,
actual visible particle pixels, Shrink removal during death and same-run restore.
The original death command calls every removed power's `AfterRemoved` after
starting death animation. Shrink consequently restores Vantom to full scale
with its750ms tween; the local burst at2133ms is already at full size. The
gate follows that actual transition instead of freezing an artificial small
corpse through the burst.

## Historical implementation-stage acceptance

The following receipts describe their original incremental stages. Their
then-pending repairs are closed by the integrated gates above; the failures
are retained as evidence rather than rewritten as passes.

The available remote connector could not complete checkpoint writes, and the
CLI has no credentials. These acceptance receipts therefore identify real
**local-only commits** and immutable snapshots; they are not remote/CI proof.

* Stage2 `0b00d2a45932b3b17fdd20317df13553b4a1a526`: independent await
  re-extraction passed, byte-identical to the retained header
  (`be9d97b097b1805f971a5c0071964fe8b147d8594781adc1041d176e2979bb44`).
  Release compilation and all1,312 actual move executions completed. The
  subsequent hand-written boundary check exposed an incorrect test fixture:
  an unrevived Axebot applies0 Strength and therefore has no power wait.
* Stage4 `fc7b99fc221d06f3cf29690d32a545a84a81a8f3`: Release compilation
  passed. All1,312 move executions, source boundary cases, real revived
  Axebot, Rat slot conditions, Gremlin stun, Test Subject respawn, generated
  card growth, and null-creator listeners passed. The later Ancient fixture
  incorrectly attempted to begin Darv inside an already active Neow event;
  it now isolates the event like the existing event-UI regression. The suite
  has **not yet passed as a whole**.
* Stage4 original Vantom noise generation passed:512×512, full0–255 range,
  no seamless/panning substitution. Its private journal is
  `ui15-vantom-noise-8edd56b0e29a.json`.
* Stage5 `95c231260f83ae244268cefc1435fc3098732674`: the corrected complete
  combat suite passed19,006 synthetic checks and **1,094,275 actual-resource
  checks**, including all1,312 move executions, requested/actual healing
  semantics, the Neow departure reset, all five Tablet upgrade choices,
  null-creator cards and independent relic flashes. Journals are
  `ui15-combat-stage5-synthetic-beaae1712323.json` and
  `ui15-combat-stage5-actual-335437d2d718.json`. Five captured screens were
  visually inspected. This pack has the13 Smith/heal additions; it does not
  establish Vantom resource acceptance.
* Stage5 map-travel regression passed154,901 checks, including the exact
 600ms entry wake and canonical save/RNG oracle
  (`ui15-world-map-travel-stage5-14c46e6b7433.json`). The world-UI test passed
  the revised intent anchors, then exposed an obsolete Test Subject
  expectation: regeneration starts while displayed HP is still0. Its test
  then checked the real1950ms heal boundary and awaited re-run. The later
  `2131803` world-UI and integration9 gates below close this fixture.
* Stage5 extraction of Vantom's five source textures passed with the exact
 9×20 spray image and four48×48 explosion cells, unchanged RGBA payload
  semantics and contiguous auxiliary5404–5408
  (`ui15-vantom-five-source-textures-e9f4cded0aad.json`). This is resource
  extraction proof; complete rendering acceptance remains separate.

* Stage7 `02b8c1cca41c64800de3a75fb08ddd86636f84a2`: the explicit sleeping
 lifecycle passed **1,993,508 checks**, including fully blocked Lagavulin
 damage, its source Stop callback and old asleep snapshots after four seconds
 (`ui15-sleep-stage7-4921de16b71b.json`).
* Stage7 complete-pack Vantom effects passed **620,275 checks**
 (`ui15-vantom-stage7-actual-6ad18e865ac8.json`). The gate executes original
 Ink Blot with33ms and250ms paint intervals, normal death and death after
 real Beetle Juice use, all four emitters, exact source event boundaries and a
 same-run restore. Eight real one-damage hits consume Slippery before the
 killing damage. No debug-dead snapshot substitutes for those callbacks.
 The skeleton gate independently passed71,016 checks across all2,040 source
 frames with bounded, losslessly paged meshes. The final package compression
 must preserve those decoded inputs and receives its own gate.
* Stage7 world-UI reached and passed both Test Subject1950ms respawns. The
 following old single-move Decimillipede fixture omitted the normal input's
 `begin_combat_feedback` transaction; it has been corrected and strengthened
 to require all three segment drivers at the same cue, their original
 alive/dead clips, and direct-track restoration. At stage7 that revision
 still required a frozen re-run; the later `2131803` world-UI and integration9
 gates below close it. The failed stage7 receipt remains
 `ui15-world-ui-stage7-1593a70e65d3.json`.
* Stage7 controlled device fixtures passed136 checks using the production
 save/progress codecs (`ui15-controlled-fixtures-stage7-export-27b3cbc4de65.json`).
 The Haunt scene executes a real End Turn to generate five Dazed cards. The
 Vantom scene can be continued and killed with an ordinary Strike, or first
 shrunk by an ordinary Beetle Juice use. Both instructed death paths were
 independently executed before exporting the live pre-action state. These
 are explicitly controlled fixtures, not evidence of a natural run.

All listed stage7 receipts retain `source_changed=false`. The repaired
Decimillipede fixture and compressed-resource paths subsequently pass the
integrated gates above. Calculator frame times remain separate from these
native receipts.

### Calculator stack and physical confirmation

The calculator stack audit found aggregate temporaries even in ordinary
single-card confirmation. Reward reset now reconstructs records directly at
their existing addresses. The picker handoff retains the active upgrade,
transformation, removal, healing and relic motion records in that storage,
instead of copying whole MaxDeck arrays and the full relic inventory to the
stack. Full reset still restores every nonzero default sentinel. No new
large pool, heap allocation or reduced card/effect capacity is involved.

ARM926/O2 stack-usage output at local checkpoint
`a6a89bd98f961a3957f6286144a706ec24df3965` measures:

| Function | Before | After |
| --- | ---: | ---: |
| `App::confirm_deck_choice` | 27,360 B | 1,264 B |
| `RewardPresentation::reset` | 24,936 B | 56 B |
| `App::reset_presentation` | 9,712 B | 2,216 B |
| `App::init` memory/source variants | 9,504 / 9,520 B | 2,040 / 2,040 B |

The new production-keyboard regression exposed another real omission:
confirm-on-release called the deck, world and combat choice handlers directly,
without the observer used by ordinary semantic input and pointer actions.
That changed cards successfully but dropped upgrade/enchantment/removal and
history presentation notifications. The release dispatcher now owns the same
nested observer scope. A real Smith key-down/key-up commit preserves all
MaxDeck upgrade snapshots, the healing effect and relic flashes, and then a
normal reset clears them and restores their sentinels. The English/Chinese
actual-resource deck gates pass14,672/55,004 checks; the six focused native
keyboard/selection/world/combat suites pass at local checkpoint
`005ab52deb35b10914d55b7dfec3b1ced38cfc77`.

The final death-hook gate at local checkpoint
`2131803fc93a5c674a8aa51990158f48a0d1c5f9` passes19,216 synthetic checks and
96,046 checks with the final compressed common/English resources, including
1,312 actual move executions at ascension0/9 and normal/fast speed. The full
world UI gate passes7,863 synthetic checks and8,025 per actual English and
Chinese edition. Its independent intent-label oracle includes the source icon
underlay and checks every glyph pixel; the old icon-free background no longer
matched the corrected inside-icon label placement. Actual-resource fixtures
also finish combat preparation after resetting the scene before sampling its
visible controls. The separate complete-resource Vantom death/shrink/restore
gate passes774,616 checks with the new internal death-removal behavior.

These are exact local native/resource and ARM frame-size gates. They do not
replace the final integrated ordinary-OS gameplay or performance acceptance.
