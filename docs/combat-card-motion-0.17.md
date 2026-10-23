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

`CombatCardMotion` is capped at 6 KiB of App-only metadata (4,312 bytes in the
verified native build). It does not add
another card-pixel allocation. The existing ten hand tiles now match visual
keys across holder positions and flights, so removing the first card does not
invalidate all the shifted tiles. Canonical State, save format and RNG code
are unchanged. Queue overflow omits intermediate cosmetic entries and converges
to the recorded hand/count result; it cannot create a playable extra card.

## Executed verification

The final targeted native checkpoint is
`9a554fe04e63b35546be708a81c85f41864a307f`, built with Clang 18 in Release mode.
All three targeted suites passed: card motion (1,079 checks), existing combat
input/focus (1,228), and synthetic world UI (6,658). The final v21 English and
Chinese resource packs each passed 19,016 world-UI checks with private captures.
See [the execution receipt](validation-ui4-combat-motion-0.17.json).

The motion tests compare fixed-point results against independent floating-point
response/curve calculations, exercise skipped frames and ten outgoing plus ten
incoming cards, and check every brightness level for RGB565 channel leakage.
Real core actions cover Normal/Fast manual play, independent discard flight and
arrival counts, three sequential draws, a full ten-card hand, the power-card
play tween, and Prepared's suspended discard choice. Each settled manual outcome
is compared to a directly executed, serialized core result. Existing Retain,
future-hand hit exclusion, choice suspension/continuation, overflow and load
tests also pass. The integrated Leaf Slime S/M and Damp Cultist tests check
actual impact/command-end times; generated Slimed cards change the discard
counter by one/two only at the audited impact.

The first runs exposed a real duplicate focused-card hit region, an old Toasty
fixture that assumed five draws were instantaneous, and a new manual-play fixture
without initial energy. The focused face now owns one actual moving hit region,
including its lower portion until a stable selection rail exists. Existing
combat focus assertions were retained. Choice expectations now follow the
original awaited draw/hook sequence, and the manual fixture supplies ordinary
turn energy. Earlier failed journals remain recorded; their passing replacements
do not erase those results. GCC 13 also hit an internal compiler error in the
unchanged relic translation unit, so the native execution used Clang 18.

The six movement captures in each language show play, discard flight and the
second arriving card in Normal/Fast modes. Native numerical and image checks
establish these exercised port behaviors; they are not physical-calculator
performance or complete original-game visual equivalence. Final ARM execution
is a separate acceptance step.

## Remaining fidelity work

Character trails, exhaust erosion/sparks, power removal, shuffle card backs,
generated-card entrance effects, queue/autoplay choreography and all individual
card-command waits are not implemented by this stage. They retain their prior
logical transitions; they are not replaced with an invented discard animation.
The first combat deal still uses the existing preparation boundary. Cost/text
history for every transforming or temporarily modified card is also outside
the compact identity/position view; manual resource labels are retained across
payment, including X-cost cards.
