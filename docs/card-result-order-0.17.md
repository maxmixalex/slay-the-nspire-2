# Card result locations and listener order — 0.17

Reference: the owner's pinned **v0.111.0 / 41cef1ea** assembly. Original code remains private; the contracts below identify the types and methods used for the implementation.

## Original ordering

`CardModel.OnPlayWrapper` first moves the instance to Play, obtains `GetResultLocationForCardPlay`, applies `Hook.ModifyCardPlayResultLocation`, awaits the changed modifiers' callbacks, and then generates the physical play count. This location is used by every repeated `CardPlay` and by the final move. Neither nested cards nor powers acquired during the card body recalculate it.

`CombatState.IterateHookListeners` snapshots each creature's powers in their list order before its other listeners. `Creature.ApplyPowerInternal` appends a new power, while removing a power removes that list entry. Stacking an existing power does not move it. The Hook compares the complete location before and after each listener; only a real change registers its later callback. The returned location includes the pile, position and player. This single-player implementation has one owner, but preserves pile and Top/Bottom position.

The complete pinned model search finds four result-location power overrides:

| Modifier | Eligibility and change | Callback when changed |
| --- | --- | --- |
| Feral | Owned non-Dupe Attack, zero EnergyValue and remaining quota; changes any previous location to Hand Top | Increments its internal used counter once |
| Corruption | Owned Skill; changes the pile to Exhaust and preserves position | Flashes the power |
| Nostalgia | Owned Attack or Skill currently destined for Discard, with fewer qualifying plays started this turn than its amount; changes to Draw Top | Flashes the power |
| Rebound | Owned card currently destined for Discard; changes to Draw Top | Decrements and removes itself at zero |

Consequently, Rebound followed by Corruption consumes a charge before sending a Skill to Exhaust, while Corruption followed by Rebound preserves the charge. Rebound followed by Feral consumes both changed listeners for an eligible Attack; Feral followed by Rebound preserves Rebound. Nostalgia and Rebound can produce the same final Draw Top result but differ in whether Rebound was the listener that changed it. A fixed priority list cannot represent these cases.

## Base result and card overrides

`CardModel.GetResultLocationForCardPlay` returns None for a Dupe or Power card before checking exhaust flags. Otherwise it consumes `ExhaustOnNextPlay` and returns Exhaust when that flag or the Exhaust keyword applies; all other cards initially return Discard Bottom.

`ParticleWall.GetResultLocationForCardPlay` changes only a base Discard result to Hand Bottom. `ShiningStrike.GetResultLocationForCardPlay` changes only a base Discard result to Draw Top. Both overrides run before the four power modifiers. Thus forced Exhaust defeats their intrinsic return, but Feral can subsequently return a zero-cost Shining Strike to Hand. Corruption can turn Particle Wall's Hand result into Exhaust and can even change a Skill Dupe's base None to Exhaust. The other original card override, The Ball, is multiplayer-only and remains excluded.

Unplayable automatic cards use `MoveToResultPileWithoutPlaying`, which bypasses these modifiers and play histories. A Dupe is removed from combat; other instances exhaust when required or enter Discard. The read-only preview uses this separate rule for the same unplayable instances.

## Saved implementation

`src/card_result_core.inc` owns the pure base/modifier evaluator, the acquisition order, one-time callbacks, committed result snapshots and placement. The four relevant power acquisition/removal functions maintain the shared order. The snapshot is captured before play-count modifiers, then read by the completion path only while that instance still occupies Limbo. A card already moved by a nested effect is not moved again.

The existing 13-byte `feral_result` contract remains intact. The unpublished v15 suffix appends `result_order[4]` and `result_location[13]`, **17 additional encoded bytes**. A zero location identifies a historical pending play whose final location was not saved. The validator checks the active ordered prefix, rejects gaps/duplicates/inactive entries, constrains location codes and requires every captured location to belong to a real active physical frame. Combat cleanup clears captured results; player power cleanup clears the order.

A relic selection whose saved return phase is Combat or a combat choice retains those physical frames. Its owner, offers and continuation remain subject to the relic validator. A forged return phase outside combat is rejected; the execution meaning of `combat_phase(Phase)` is unchanged. The resulting conservative save-capacity bound is 255,992 bytes within the 262,144-byte buffer, including native-tail padding.

Historical v3–v14 field offsets remain unchanged. Those formats contain no Rebound and cannot recover cross-module acquisition order. Their active original powers receive an explicit stable migration order; their already-pending plays retain the stored Nostalgia decision and the prior pending-result policy once. New plays after migration use the ordered capture. This migration policy is separate from the original rule for newly started plays.

`card_play_result(const State&, int instance, Zone& pile, bool& top)` is read-only. During an active physical play it reports the committed snapshot, including outer frames during a nested choice. Otherwise it evaluates the current instance without consuming Feral, Rebound or ExhaustOnNextPlay and without changing RNG. `Zone::Power` represents the original None result in this bounded engine, where removed combat instances remain outside recyclable piles.

Nostalgia's history counter now counts Attack and Skill entries only. Feral's zero-energy Attack history advances at the shared play-start boundary after BeforeCardPlayed listeners, rather than during their preparation. Hand insertion distinguishes Particle Wall's Bottom from Feral's Top, including the original Top position when a full hand redirects Feral into Discard.

## Regression and verification status

`tests/card_result_regression.cpp` covers acquisition order in both directions, Rebound removal and reacquisition, intrinsic card locations, forced Exhaust, a Corruption acquired inside Havoc, playable Status history, repeated and nested physical plays, saved choices, pure previews, full-hand ordering, true Dupes and malformed captured state. Existing card and genuine historical-save suites remain necessary regression gates.

The source stage has short syntax validation only until it is pushed/read back and executed from a frozen checkpoint. These tests and the rule translation do not establish an unmodified-game differential result.
