# Permanent acquisition observations — UI9 / 0.17

Reference: pinned v0.111.0 / `41cef1ea`. This is a presentation journal; the
simulation `State`, command logs, random streams and suspended-command codec
keep their existing semantics.

## Source boundaries

`MegaCrit.Sts2.Core.Commands/CardCmd.cs` records transformations after the new
card enters the permanent pile, upgrades/downgrades only for permanent cards,
and enchantments with the affected card and enchantment model. Its internal
transform removal/addition is distinct from ordinary acquisition and removal.
`PotionCmd.cs` records successful procurement and discarded bottles, while the
use wrapper consumes a bottle before its use callbacks. `RelicCmd.cs` records
successful ownership and removal. The port observes those committed boundaries
through `CoreObserver::on_history`, never through a broad interpretation of
combat `LogKind::Upgrade` or presentation relic flashes.

## Runtime coverage

- `event_deck_acquired` reports the completed permanent card after modifiers.
  Hoarder and BingBong report each actual clone. Pael's Tooth reports a returned
  card when it really rejoins the deck. Generic internal snapshot take/restore
  does not automatically manufacture a permanent history command. Thieving
  Hopper's `SwipePower.Steal` explicitly uses source `RemoveFromDeck`, so its
  successful theft records removal; accepting its `SpecialCardReward` records
  the subsequent real acquisition.
- Permanent removal, upgrade, downgrade and enchantment paths cover event
  selection, event random changes, smithing, relic choices/random upgrades,
  Fishing Rod, Neow's Talisman and post-combat Improvement. Hand-only Forge and
  other combat upgrades do not become permanent upgrade entries.
- Normal transform emits one old/new card pair. Its original card does not
  emit a normal removal and the replacement does not emit a normal acquisition;
  any independently produced Hoarder/BingBong clones remain actual acquisitions.
- Relic ownership is observed inside the actual `Obtain` continuation. Failed
  capacity/eligibility checks do not announce a relic. Replace records the
  old ownership removal and the successful new obtain in their real order.
- Potion procurement, manual consumption, discard and automatic Fairy use
  report the bottle's model exactly once. Sozu or a full/occupied slot produces
  no procurement. A failed manual-use request produces no use entry.
- Maximum HP observations use the actual clamped difference. Deferred combat
  loss is recorded when `SetMaxHp` commits, after damage/death-prevention
  callbacks; it is not announced merely when that continuation is queued.
- Source `PlayerCmd.CompleteQuest` is observed at Spoils Map's actual gold
  reward/removal and War Historian Repy's Lantern Key completion/removal. The
  initial Lantern Key acquisition event does not complete its quest.
- Fur Coat emits its affected-room marker in `BeforeCombatStart`, only when
  the real map point is marked. Subsequent enemy spawns still receive the
  source HP effect but do not append another room marker.

## Observer payload

The `RunHistoryNotice` card model fields use `card_token`, including its upgrade
bit. Enchantments use `model=card_token`, `detail=actual amount`,
`flags=EventEnchantment`; potion models retain the existing 1-based potion IDs,
and relic models retain `RelicId`. HP amounts occupy the model/detail low/high
16-bit fields (all current maximum-HP changes fit the low field).

Batch transforms already persist only the original base card kind in their
canonical continuation. To preserve the original upgrade status without
changing that save format, the observer receives two phases:

| Flags | Model | Detail | Journal action |
| --- | --- | --- | --- |
| 0 | Original complete token | Final replacement complete token | Commit ordinary transform |
| 1 | Original complete token | Prepared replacement complete token | Store hidden pending pair before internal removal |
| 2 | Original base kind | Final replacement complete token | Finalize matching pending pair after deck modifiers |

The UI journal owns pending pairs and their persistence. It matches the prepared
pair first, then the original/replacement kinds if a deck modifier upgraded the
replacement. An old history without a pending pair remains explicitly partial
instead of inventing the original upgrade state. No pending observer payload is
stored in `State`.

## Verification

`run_history_acquisition_regression` executes actual card, relic, potion, Fairy,
maximum-HP, quest-completion and batch-transform commands twice from identical states, with and
without the history observer. It checks every resulting `State` byte, including
RNGs and command logs, and verifies successful/failed command notice counts,
complete card tokens, per-clone entries, deferred commit timing and absence of
false remove/gain or permanent-combat-upgrade entries.

The new test and affected core translation units pass Clang 18 syntax checking.
Execution belongs to the complete Stage8 checkpoint gate; syntax success is
not a claim that the runtime gate has passed.
