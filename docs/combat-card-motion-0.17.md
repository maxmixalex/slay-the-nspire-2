# Combat card movement — UI4 implementation scope

This stage follows the privately supplied v0.111.0 / `41cef1ea` assembly.
Only the port's implementation and a summary of the inspected behavior are
committed; the original assembly, decompilation and scenes remain private.

## Inspected original behavior

| Original method | Behavior used by this stage |
| --- | --- |
| `CardPileCmd.DrawInternal` | Each actual draw awaits `Add` before drawing the next card; a full hand produces no transfer. |
| `CombatManager.SetupPlayerTurn` and `ToastyMittens.AfterPlayerTurnStart` | The turn setup awaits the complete hand draw before invoking turn-start hooks. Toasty's selection is therefore after the actual Add waits, while the player banner starts at the turn boundary. |
| `CardPileCmd.AppendPileLerpTween` | Pile movement lasts 250 ms normally and 100 ms in Fast. The play-pile scale tween always lasts 250 ms and targets 0.8 scale. |
| `CardPileCmd.AddDuringManualCardPlay` | Ordinary manual movement runs alongside effects; powers await the play-pile tween. |
| `CardModel.OnPlayWrapper` | The effect section has a minimum elapsed presentation of 300 ms normally or 150 ms in Fast before its result-pile transition. This is a minimum, not a new delay after every effect. |
| `NPlayerHand.Add` and `RefreshLayout` | Preserve the card's previous position when creating its holder. A layout change repositions existing holders. Focus snaps Y/angle/scale while X continues moving. |
| `NHandCardHolder` | Position, angle and scale approach their targets with response factors 7, 10 and 8 per second. |
| `NCardFlyVfx` | Draw/discard flights are independent quadratic curves with cosmetic speed, acceleration and duration variation. The body shrinks and darkens in the first third of its progress, then holds briefly at the destination. Pile-add completion occurs on arrival. |

The calculator keeps its readable 320 × 240 hand fan, card dimensions and HUD
anchors. Original response functions are sampled on the established 33 ms
presentation clock. Consecutive Add waits accumulate in milliseconds before
rounding to that clock, avoiding a new rounding delay for every drawn card.
Flight variation uses a separate cosmetic generator and never advances a saved
or gameplay RNG stream.

## Implemented boundary

Settled core effects supply real instance/zone transitions. The existing hand
ID view and the new movement queue run on the same clock as health/status
feedback. Actual draw/add commands reveal one arriving holder at their own
boundary; retained cards reflow without disappearing. Manual cards move into
the play area and, when their actual result is draw/discard, begin a separate
flight. Hand hit regions, focus outlines and keyword anchors use the displayed
pose. Discard/draw counters account for cards still flying toward their pile.

`CombatCardMotion` is capped at 6 KiB of App-only metadata. It does not add
another card-pixel allocation. The existing ten hand tiles now match visual
keys across holder positions and flights, so removing the first card does not
invalidate all the shifted tiles. Canonical State, save format and RNG code
are unchanged. Queue overflow omits intermediate cosmetic entries and converges
to the recorded hand/count result; it cannot create a playable extra card.

## Remaining fidelity work and verification status

This is the source checkpoint before targeted execution. It is not an emulator
acceptance claim. Tests and actual-pack captures are recorded separately after
the checkpoint is persisted.

Character trails, exhaust erosion/sparks, power removal, shuffle card backs,
generated-card entrance effects, queue/autoplay choreography and all individual
card-command waits are not implemented by this stage. They retain their prior
logical transitions; they are not replaced with an invented discard animation.
The first combat deal still uses the existing preparation boundary. Cost/text
history for every transforming or temporarily modified card is also outside
the compact identity/position view; manual resource labels are retained across
payment, including X-cost cards.
