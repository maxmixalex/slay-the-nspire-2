# Combat UI6 — original presentation and bounded playback

Reference: **Slay the Spire 2 v0.111.0 / 41cef1ea**. The assembly used for
inspection has SHA-256
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.
This document records independently encoded behavior/numeric facts. Original
assembly, decompiled classes, scene text, images and derived resource packs
remain outside the public repository.

## Implemented behavior

### Power text and amount labels

`NPower.RefreshAmount` renders an amount only for `PowerStackType.Counter`.
The existing power-classification table now also records the inspected
`Single`/`None` classifications, including Minion, Illusion, Barricade,
Corruption, NoDraw, Hex and Surrounded. Strip, hover tip and expanded status
view share the same decision. The dynamic Shrink classification hides its
negative/infinite form's amount. Monologue displays its applied Strength
rather than the number of its instances. Galvanic was checked separately and
is a Counter.

`NCreature.OnPowerRemoved` creates `NPowerRemovedVfx`. A visible power's actual
transition to zero while its owner is alive now schedules the original icon,
power title and `vfx/POWER_WEARS_OFF` line. The renderer uses a 250 ms entrance,
500 ms hold and 1000 ms fade; upward movement follows the authored 2000 ms,
160 source-pixel motion. Ordinary duration decrements do not create a false
application popup. Death/combat cleanup does not create a wall of removals.

Application and removal records retain separate identities and times. Three
bounded rows per creature and a pending-row queue keep simultaneous changes
readable. Text rectangles are packed only when their motion would overlap;
this is an explicit adaptation to the calculator's 320×240 display. Core
settlement is never delayed by the text queue. Removed/expired effects are
compacted once per display tick, including after the command trace ends.

### Independent damage numbers and block entry

Each released HP-loss cue now owns its own damage-number instance, instead
of overwriting a single `last_hit` label. `NDamageNumVfx` and
`vfx_damage_num.tscn` supply the 2 s lifetime, initial 2.5× scale, 1.2 s Quad.Out
scale recovery, red-to-cream color transition, quadratic fade, gravity and
independent cosmetic velocity range. Numbers retain the same damage cue
clock as the displayed HP, survive subsequent hits, and use existing font
pixels. A small alpha scratch line is decoded once and shared by scaling and
outline passes. Saved RNG is not read or advanced by this motion.

`NHealthBar.AnimateInBlock` animates only the first gain from zero: 500 ms
Sine.Out opacity and Back.Out position, starting 20 source pixels above its
normal location. Later increases update the amount without restarting the
entry. Normal and Fast both retain this animation. Existing HP interpolation
and the exact decoded health textures are preserved.

### Enemy-generated cards

The actual `EnemyGenerateCard` core effect now produces a presentation cue
for new cards entering a non-hand combat pile. `CardPileCmd.AddToCombatAndPreview`
and `CardCmd.PreviewInternal` establish the sequence: horizontal preview for
up to five cards, 250 ms Cubic.Out scale-in, 1.2 s display, then the existing
`NCardFlyVfx`-derived pile flight. Larger batches use bounded, independently
sampled positions with the original 150 source-pixel minimum spacing adapted
to this viewport. No card is added twice and the canonical pile is unchanged.

The original command waits once per batch: 1 s for a non-hand pile or 100 ms
for a hand destination. The preview and subsequent flight have independent
lifetimes. Their bounded metadata reuses existing card-face raster storage.

### Reward timing and leaving the room

`NCombatUi.ShowRewards` does not wait for a corpse's complete dissolve. It
measures the longest currently remaining death track and selects:

| Room / mode | Reward wait after the completed command |
| --- | --- |
| Ordinary, Normal | Remaining death track + 1 s |
| Ordinary, Fast | 0.5 s |
| Boss, Normal | Remaining death track + 1 s |
| Boss, Fast | Half the remaining death track |

The delay is now scheduled once. The death track's own half-second hold and
2.5 s dissolve continue separately. Repeated renders no longer extend the
reward deadline to `hide_at`. Restored removed creatures do not replay a new
reward wait.

A completed encounter remains in canonical state for save/reward purposes.
The UI now refuses to reconstruct those models in a new non-combat room
without a current combat trace or its pending victory transition. This
addresses the old enemy-room flash after entering an event, without clearing
core encounter state or breaking event-origin combat.

### Named formations and surrounding encounters

`NCombatRoom.PositionCreaturesWithSlots` and `AddCreature` bind actors to the
encounter's named Marker2D; summons do not re-layout every other actor. The
new compact numeric table contains all **24** encounters with scene markers,
using their authored order, positions and camera getters. Actor drawing,
intent, health/status placement, target hit regions and pointer targeting
share `enemy_pose(State, slot)`.

Five of these scenes do not override `EncounterModel.Slots`: both Bowlbugs
encounters, Gremlin Merc, Knights and Slumbering Beetle. Auditing `HasScene`
and the literal names passed by `GenerateMonsters` caught this distinction.
Gremlin Merc's `merc`, `sneaky` and `fat` markers follow the actual actor kind;
their simulation slot indices intentionally retain the original spawn order.
Splitting Merc therefore keeps Sneaky at Merc's location and Fat at his own
authored marker.

The existing per-model normalized sprite raster is resampled using original
CreatureVisuals Bounds for relative size. This makes small Tough Eggs remain
small beside Ovicopter, and reserves their locations before they appear.
The ground rail, minimum readable health width and maximum body size remain
320×240 adaptations. This does not claim identical PC viewport geometry.

Encounters without a scene now use `NCombatRoom.PositionEnemies`: add their
original Bounds widths, start with 70 source-pixel gaps in a `960 / zoom`
half-screen, and reserve the initial 150 source-pixel inset. Crowded groups
recalculate their gap with the original 5 source-pixel minimum and alternate
vertical offset between 60 and 40 source pixels when the gap is below 30.
Q4 authored coordinates retain fractional centers without floating point.
The retained roster includes dead actors so survivors never slide inward
after a kill. Ordinary encounters no longer use the old uniform columns or
two-row count-based arrangement.

An adjacent discrepancy was also corrected: KaiserCrabBoss sets
`FullyCenterPlayers=true`. The player, Osty, health/status/orb positions and
floating text now move between the left Crusher and right Rocket. The normal
single player starts at the original left-half center (80 adapted pixels),
and the same encounter camera offset also applies to the player. The local
Necrobinder keeps the additional authored 150 source-pixel offset for Osty.

## Performance and storage decisions

- No changes to `State`, `SaveData`, gameplay RNG, or effect queue formats.
- Damage records and popup lanes add roughly 1.5 KiB of bounded metadata;
  no framebuffer, texture atlas or unbounded cache was added.
- Expired popup/damage records disappear from idle render loops, once per
  presentation tick. Re-rendering one frame does not advance or repack time.
- Power-title outlines fetch each glyph once, replacing nine repeated text
  traversals. Single/None powers also skip amount formatting and measurement.
- Scene formations use a sorted-table lookup. Unnamed formations use bounded
  width sums over at most eight actors; no string lookup, allocation or
  transform tree is involved. Neither path mutates saved state or RNG.
- Each displayed enemy now retains its 28-byte pose alongside its displayed
  identity. Sprite, intent hit, health bar and floating text consume that one
  pose until the identity cue releases its replacement. This fixes future
  Bounds leaking through a delayed replacement, prevents existing actors
  from moving when a summon arrives, and removes repeated per-frame formation
  sums from those consumers. A fresh/reset view still derives the current pose.
- Formal measurements belong to the later frozen checkpoint, not to this
  source-only implementation note. No physical calculator FPS claim is made.

## Verification status

Short Clang 18 syntax checks of the production `app.cpp`, card-motion,
health-motion, world-UI, debug and dedicated formation tests passed after
these changes. The first debug build on stage 2 used GCC and stopped at
an aggregate assignment rejected by that compiler; the live fix reconstructs
that bounded reward record in place. It is not recorded as a passing test.

Frozen stage 3 `20262f09eb1edab0035e632ca9ee4f065fb51fc4` completed the full
Clang 18 debug build. Its 79-test run passed 73 tests; two debug reward
assertions incorrectly expected the old `Phase::Reward` fields instead of
the current unified `OfferReward` group. They now check the real group and
its card/claimable records, without bypassing reward selection. The UI run
also exposed the delayed-replacement geometry leak described above, and a
Q4 truncation moved Aeonglass's centered position one display pixel left.
Both implementation fixes await the next frozen checkpoint. The remaining
potion/map failures belong to their parallel UI scopes. The layout test's
first debug invocation lacked executable permission; after repairing only
that built-file mode, a logged retry reproduced the same Aeonglass assertion
as the normal build. Actual English/Chinese world and debug UI attempts
stopped at those same failures; they are not counted as passing coverage.

Regressions now cover first block entry/re-gain/restoration, generated-card
preview and flight, state/RNG invariance, simultaneous status rows and expiry,
Single/Counter decisions, independent multi-hit numbers, reserved Ovicopter
slots, all 24 scene layouts, original width/spacing and alternating-height
cases, Gremlin Merc's real split, surrounding boss placement, original reward
delays, and stale room presentation rejection. Existing assertions that expected no removal VFX or
an immediately opaque new shield were updated to the original behavior.

## Remaining boundaries

Damage numbers currently use the released HP-loss projection, so original
OverkillDamage and separately blocked-hit VFX still need an explicit damage
result observer. Per-power GPU flash particles and the original block-break
particles are not implemented here. Text-row packing, the large-batch preview
sampler, death dissolve opacity and sprite Bounds scaling are bounded raster
adaptations, not shader/Spine identity claims. Original random visual scale/
hue variation and the unusual viewport-overflow container rescale are not
reproduced by this fixed 320×240 adapter. These are not counted as completed
fidelity work.
