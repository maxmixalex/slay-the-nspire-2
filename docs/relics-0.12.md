# Relic combos and treasure interface - 0.12

Reference: the owner's hash-verified v0.111.0 / 41cef1ea installation. The five models occur in the inspected SharedRelicPool registry. Relevant method identifiers are recorded in `config/relic-audit-0.12.json`. The original executable was not run.

## Added models

| Relic | Supported effect | Counter lifetime |
|---|---|---|
| Kunai / 苦无 | Every third Attack grants 1 Dexterity. | Current player turn. |
| Shuriken / 手里剑 | Every third Attack grants 1 Strength. | Current player turn. |
| Ornamental Fan / 精致折扇 | Every third Attack grants 4 unpowered Block. | Current player turn. |
| Letter Opener / 开信刀 | Every third Skill deals 5 unpowered damage to each living enemy. | Current player turn. |
| Nunchaku / 双截棍 | Every tenth Attack grants 1 Energy. | Persists across turns and combats. |

The current pools contain 68 cards, 30 relics and 16 potions; this release does not add a new character or card model. The added relics can be selected by the implemented merchant/treasure pools. Uniform prototype sampling and existing merchant prices are not original acquisition parity.

## Card-play ordering

The inspected original card wrapper completes OnPlay, calls AfterCardPlayed, then moves the source card to its result pile. The native core invokes these five hooks once in Finish before the result-pile move, in relic acquisition order. A suspended Headbutt/Survivor/etc. is not counted until its captured choice completes. Random/multiple hits count as one card; a zero-energy X-cost Attack still counts. Powers and potions do not count as Attack/Skill plays. Direct fatal self-HP loss terminates queued effects before these hooks.

Shuriken's new Strength does not increase the just-completed third Attack. Ornamental Fan's fixed block does not use Dexterity or card-source No Block. Letter Opener does not use Strength, Vulnerable or attack multipliers; enemy Block can absorb its damage. Finish performs victory only after moving the source card, so a lethal Letter Opener trigger cannot leave a card orphaned in Limbo. The supported engine still lacks original death-prevention/minion mechanics and a complete original scheduler oracle.

The first four counters reset at combat entry, the next player-turn boundary and victory. Nunchaku persists. Counters and trigger revisions are serialized. Capacity guards retain the existing bounded stat model; complete original unbounded-stat behavior is not claimed. Relic masks currently use 30 bits, with a compile-time guard against silently exceeding the format.

## Relic inspection

Owned HUD icons address inventory positions, resolving the correct model after acquisition-order lookup. Clicking the sixth owned relic cannot accidentally open the first relic or interpret its bitmask as a model index. The R inventory uses eighteen-icon pages, six-column keyboard navigation and full detail overlays. Trigger highlights are timed presentation state; the HUD may briefly show the threshold in green, while the persisted counter has already reset. Detail text reports the actual stored counter.

Hovering, paging, inspecting and animation ticks cannot alter gameplay RNG or save bytes. Modal input prevents ending the turn or opening an incompatible status overlay through relic details. Save notices are hidden behind detail/inspection/treasure interfaces rather than covering their controls.

## Treasure stages

The original treasure room and relic-holder scenes informed the centered relic, banner and separate proceed control. A closed chest now reveals an item without granting it. The user can inspect it, claim it, or skip it. A claimed item remains visible but cannot be claimed twice; proceeding is explicit.

The chosen relic and stage are persisted. Reloading an opened chest cannot consume RNG to reroll the item. Reloading a claimed chest cannot duplicate it. Leaving clears this room-specific metadata. When all available prototype relics are owned, the historical 50-gold fallback remains; this is explicitly a prototype behavior, not an assertion about original exhausted pools.

An existing bounded-gold merchant bug was also fixed: buying Old Coin now deducts its price before granting gold. At the 9,999 cap, the old grant-then-deduct order lost the purchase price incorrectly. The deduction is rolled back if acquisition fails.

## Formats and migration

NSP2/9 keeps 68 card records and extends the auxiliary directory from 90 to 95 images. All 1,319 old image payloads, 3,812 glyph records/payloads, 17 clip records, strings and card records remain unchanged. Five original icons and required extra glyphs are appended. The new App checks both card count and auxiliary profile, rejecting a valid older pack that lacks the new art.

Save v9 appends five relic order/counter slots and the treasure stage/reward. Historical v7/v8 widths remain fixed at 25 relics; v9 is fixed at 30. The old v8 fixture was written by the original 0.11 runtime before use in migration tests. Version 3-8 project saves can load, but new saves cannot be opened by old launchers.

Audio, multiplayer and Timeline remain excluded. No original game code/data, fonts, calculator images or private disassembly are added to the public source.
