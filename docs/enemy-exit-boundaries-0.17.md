# Enemy escape and suspended death boundaries — 0.17

This stage follows source18 `5147ca97b377f662f363c61124e5022fd0277143`.
The original reference remains the owner's pinned v0.111.0 / `41cef1ea`
installation. Original source and assets remain private.

## Escape must not become an AfterDeath event

The original executable paths are:

| Reachable actor | Original method leading to `CreatureCmd.Escape` |
| --- | --- |
| Thieving Hopper | `ThievingHopper.EscapeMove`, after its five-move sequence |
| Fat Gremlin | `FatGremlin.FleeMove`, after Gremlin Merc's Surprise replacement and its wake-up move |
| Battleworn Dummy | `BattlewornDummyTimeLimitPower.AfterSideTurnEnd`, when the three-turn counter expires |

The pinned `CreatureCmd.Escape` first returns for a dead creature. For a live
combat creature it removes powers and presentation/state-machine participation,
then records the escape through `CombatState.CreatureEscaped`. It does not
invoke `BeforeDeath`, `AfterDeath` or the kill command. `Melancholy.AfterDeath`
therefore cannot reduce its cost because a creature escaped. `GremlinHorn`
likewise has no escape-triggered death reward.

The bounded engine already distinguished removed and escaped actors, but also
used HP zero as the removed-slot sentinel. After each saved command, the shared
Necrobinder death observer recognized that new zero HP as a death. A short
ASan/UBSan probe against the unchanged frozen source17 library reproduced the
same error on all three paths:

| Exit | Extra creature deaths | Incorrect Melancholy cost change |
| --- | ---: | ---: |
| Thieving Hopper escape | 1 | -1 |
| Fat Gremlin flee, with Sneaky Gremlin still alive | 1 | -1 |
| Battleworn Dummy timeout | 1 | -1 |

`enemy_escape` now marks that sentinel transition as already observed, without
invoking a death listener. It uses the existing saved observation mask, so there
is no new field or codec version. The normal slot-creation reset clears this
bit before a new combat identity occupies the slot. Genuine deaths still use
the existing queued death commands and separate relic dispatch.

The same function now requires a living actor, matching the original early
return. A saved Escape command cannot remove a zero-HP Test Subject waiting for
revival or cancel a pending Fat Gremlin death before its Heist reward and
Gremlin Horn listener execute.

The new regression covers the following boundaries:

- Ordinary and deadly ascension: Hopper's complete five-move sequence, stolen
  card loss and escape gold proportion; the Dummy's complete three-turn timeout;
  and the actual Fat Gremlin Flee command after its wake-up move.
- Byte-identical saves and continuation both before the terminal turn and with
  the actual Flee command already queued. Repeating an Escape for the removed
  actor changes nothing.
- A controlled production spawn into the escaped physical slot receives a new
  combat identity. Its subsequent real death still decrements Melancholy once
  and grants Gremlin Horn energy once. This extra spawn exercises the generic
  slot contract; it is not claimed to be Gremlin Merc's original AI.
- A pending real Fat Gremlin death remains a death if an Escape command is
  prepended. It returns the exact 20 stolen gold and executes its real death
  listeners once. A downed Test Subject remains available for its normal revive.

No encounter probabilities, HP ranges, move values, card costs or gameplay RNG
algorithms change.

## A real choice inside the minion cascade remains resumable

The earlier source16 last-primary repair already used saved death commands.
This audit adds a more demanding boundary to the previous pending-damage save:
Ovicopter and its three original Tough Eggs, Gremlin Horn and Stratagem.

The leader's death draws the final draw-pile card. The first minion's death then
requires a shuffle and suspends in Stratagem's selection. At that point one
zero-HP minion and two living minions still have pending death commands. The
regression verifies that they keep combat open, that an invalid selection is
atomic, and that saving/loading then selecting produces identical completed
state. All four real deaths trigger Melancholy and Gremlin Horn exactly once
before rewards. This new 72-check boundary passes against both the old frozen
library and the edited core; no additional cascade repair was necessary.

## Short validation and remaining gate

The changed core and enemy regression translation units pass C++17 syntax
checks. A bounded single-translation-unit ASan/UBSan diagnostic built the edited
core and linked it with the remaining objects from frozen source17. The two
new focused functions passed **683 checks**. The same test object linked with
the unchanged frozen library passes the 72-check suspended cascade and then
fails at Hopper's first incorrect death-count/cost assertion. Raw diagnostics
and source hashes remain private.

These are focused diagnostics, not a complete rebuilt checkpoint or an
original-game replay. After source19 is pushed, read back and frozen, run its
complete `enemies_regression` under `tools/run_logged.py`, including the existing
174 encounter/ascension executions. The broader enemy registry still does not
prove every randomized move branch or card/relic interaction. No new UI-fidelity
percentage or calculator performance result follows from this repair.
