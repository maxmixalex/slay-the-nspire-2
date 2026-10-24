# UI8 observed per-floor history

## Pinned behavior

The complete repaired source is available at
`reference-selected/MegaCrit.Sts2.Core.Nodes.HoverTips.NMapPointHistoryHoverTip.decompiled.cs`.
Its `_Ready`, `PopulateActionStats`, and `PopulateRewardAndSkippedEntries` are the
basis of both current-map and compendium-history hover readers. The empty class
in `reference-code` is not a source-availability limitation.

The reader retains original order: player and first-room fields; ancient/event
and rest choices; Fur Coat; maximum-HP loss, damage, maximum-HP gain, healing and
first-combat turns; completed quests; potions used/discarded; gold spent/lost/stolen;
then Rewards and Skipped. Rewards contain gold, gained cards/relics/potions, removed
cards/relics, upgrades/downgrades, enchantments and transformations. A selected card
is displayed once through CardsGained; picked-card and purchase facts remain
recorded metadata rather than duplicate lines. Ancient skipped options belong to
the action section. The second reward/skipped column appears only above five
entries, and the first column receives `max(5, ceil(count / 2))` entries, matching
the original column-first population.

Original 22-pixel body text uses the real regular 5-pixel font; title and section
headers use real bold face 5. Four source 24×24 sprite-font icons project to 5×5:
`images/packed/sprite_fonts/{gold,card,chest,potion}_icon.png`. Their source-only
build descriptors live in `tools/history_journal_ui8_assets.py`; the v30 suffix is
auxiliary 2045–2048, with 2049 total images. Later resource suffixes retain these
indices. The original map quest marker also appears on historical points with
observed completed quests or Fur Coat.

A bounded device-height reader allows long histories to remain readable. Wheel
input over the tip belongs to its content and does not scroll the underlying map
or history page. In history keyboard navigation, Confirm/Inspect opens reading;
Up/Down scroll; Back/Confirm/Inspect returns to point navigation. This is UI-only
and cannot apply a historical run to the current simulation.

## Actual observations

`CoreObserver::on_history` receives committed model commands; it does not modify
State, canonical logs, random streams or save continuations. Permanent card
add/remove/upgrade/downgrade/enchant/transform, relic obtain/remove/replace,
potion obtain/use/discard and maximum-HP deltas report their actual results.
Combat-only card upgrades do not become permanent upgrades. Automatic Fairy
consumption records use, and real event PotionCmd.Discard paths also report it.

CardReward cancellation back to its parent list leaves its offers unclassified.
A successful choice reports selected and remaining candidates; successful reroll
reports the old candidates skipped. Leaving a nonterminal event/relic reward set
reports its remaining offers. Ordinary terminal rewards and merchants remain
interactive after Proceed opens the map; their unclaimed rewards/stock are
reported only on actual `world_choose_point` departure. Returning from an event
combat or changing acts performs the actual exit immediately. Repeated Proceed
and failed/full-belt purchases do not create duplicate outcomes.

Ordinary event choices capture the original title localization ID and its
parameter before page changes and item consumption. All 200 original event
option title keys have stable registered IDs. Ranwid's gold/potion/relic,
Forgotten Soul, potion rarity and the Doll Room overridden relic title retain
actual captured values. Architect and Round Tea Party's excluded continuation
follow the source's history suppression. FakeMerchant has no ordinary source
EventOptions, so its artificial internal purchase indices are not fabricated
EventChoices; actual purchases and remaining stock are recorded separately.

GoldLossType is preserved as three distinct observed categories: real
merchant/paid-event spending, other losses and Gremlin theft. The original paid
Zen Weaver, Crystal Sphere, Endless Conveyor, Tea Master, Luminous Choir, Ranwid,
Wongo, Whispering Hollow, Waterlogged Scriptorium and FakeMerchant commands also
contribute to the original cumulative gold-spent badge fact. Gameplay gold/log
values and RNG order remain unchanged.

A retained battle State is not evidence that a newly entered shop, rest site or
event contained combat. An observed combat-start entry identifies the point's
first actual battle. Its encounter/turn count is retained across a second boss
or later event battle, matching the source hover's first combat room. Missing
legacy or evicted first-battle metadata never invents a zero-damage outcome or
relabels a later combat as the first.

## Bounded storage and compatibility

The journal is one shared 4,096-entry ring, each entry 8 bytes, with 33 owner
records for the live run and 32 retained history runs. Total journal allocation
is exactly **33,696 bytes**. There are no additional State or UiFloorHistory
fields; the ephemeral hover reader is at most 28 bytes. Entries retain order and
multiplicity. Scalar values aggregate per owner/floor with saturation explicitly
marking partial data. When full, evicting an entry marks its original floor
partial. Pruning dead owners compacts the logical ring before reusing an owner
index, preventing stale entries from attaching to a later run.

NSH7 writes only live owners and used entries. NSH1–6 still load. A legacy floor
has no invented journal; later actual observations can extend it while its
unknown prefix remains marked. New floors observed from entry can be complete.
Overflow or unknown data uses the existing localized unavailable-history notice.

Batch transformations store a pending old full token and generated replacement
before the canonical queue removes the old card. The matching actual completion
updates that row in place, including after NSH7 persistence. The canonical queue
continues storing its existing base-kind format. Old saves without a pending
observation retain only the actually known base kind and mark the floor partial.
Pending rows are not shown as completed transformations.

Full profile decode retains the existing in-place private decode contract. A
late malformed tail can write into that private output, but production restore
publishes a completely valid backup or a cleared profile; it never treats a
failed decode as a usable history. Profile-card summary decoding intentionally
reads only the small statistics prefix and does not claim to validate a journal.
No second UiProgress-sized transactional staging allocation is introduced.

## Verification

The implementation and new/updated tests pass Clang 18 short syntax checks.
`run_history_acquisition_regression` tests actual accepted/rejected model commands
with and without observers and requires byte-identical State results.
`run_history_journal_ui8_regression` covers actual event variable capture,
potion discard, FakeMerchant purchase, CardReward cancel/reroll/claim/skip,
retained-room departure, combat-to-noncombat history, both hover input surfaces,
State-pure scrolling, NSH1–7 migration, late-tail restore fallback, full-capacity
encoding, transformation continuation and owner reuse.

The independent bounded owner audit completed 34,500 appends over 150 run serials
and 6,762,772 oracle comparisons, including wrap/eviction/prune/reuse. This proves
the container invariants only; it does not replace the frozen runtime regression
or real-resource visual checks. Full native/ARM/resource validation is pending
the root-owned immutable Stage8 checkpoint.

The retained entry pool is deliberately bounded. Earlier facts from old profiles
and entries lost to this bound remain unavailable. This work does not claim a
source-parity percentage from class counts or diagnostic coverage.
