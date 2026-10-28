# Original deck and world card selectors — UI9

Pinned reference: 0.111.0 / `41cef1ea`. This change replaces reachable legacy
card-selection pages; it does not infer an overall restoration percentage.

## Source contracts

The source `CardSelectCmd` chooses distinct screen implementations. These
contracts are kept separate:

| Caller | Source screen and behavior | Port route |
| --- | --- | --- |
| Rest-site Smith | `NDeckUpgradeSelectScreen`, cancelable, manual confirmation | Eligible deck grid → before/after upgrade preview → Confirm |
| Ordinary merchant removal | `NDeckCardSelectScreen`, curse-first eligible order, cancelable | Eligible deck grid → selected-card preview → Confirm; core checks the current removal price |
| Meat Cleaver Cook | `NDeckCardSelectScreen`, exactly two cards, cancelable | Two selected cards → preview; preview Back clears selection, grid Back returns to the unused rest option |
| Dolly's Mirror and fixed removal relics | `NDeckCardSelectScreen`, noncancelable | Original prompt, highlighted selections, separate preview/confirmation |
| Pomander/Yummy Cookie and event upgrades | `NDeckUpgradeSelectScreen` | Single before/after preview or upgraded multi-card preview |
| Astrolabe/New Leaf, Claws, Morphic Grove and other actual transformation callers | `NDeckTransformSelectScreen` | Original cards on the left; fixed replacements or independent cycling candidates on the right |
| Wood Carvings | `FromDeckGeneric`, despite the subsequent transform operation | Selected original card preview; no invented transformation preview |
| Enchantment relics and events | `NDeckEnchantSelectScreen` | Single before/after enchanted clone; the source multi preview keeps original models; original enchantment description panel |
| Sea Glass / Sealed Deck | `NSimpleCardSelectScreen`, manual | Grid and native Confirm; no added deck preview; Sealed Deck still requires Confirm after the tenth selection |
| Brain Leech / Room Full of Cheese | `NSimpleCardSelectScreen`, nonmanual | Completing the exact source selection count resolves the real reward immediately |
| Scroll Boxes | `NChooseABundleSelectionScreen` | Two overlapping three-card bundles → three-card preview → Back or Confirm |
| Hefty Tablet / Lead Paperweight | `NChooseACardSelectionScreen`, optional | Shared combat-agent source-offer renderer; direct choice after its original input gate, optional Skip |

Primary original sources, under `reference-code`:

- `MegaCrit.Sts2.Core.Commands/CardSelectCmd.cs` (`FromDeckForUpgrade`,
  `FromDeckGeneric`, `FromDeckForRemoval`, `FromDeckForEnchantment`,
  `FromDeckForTransformation`, `FromSimpleGridForRewards`).
- `MegaCrit.Sts2.Core.CardSelection/CardSelectorPrefs.cs`.
- `MegaCrit.Sts2.Core.Nodes.Screens.CardSelection/NDeckCardSelectScreen.cs`,
  `NDeckUpgradeSelectScreen.cs`, `NDeckTransformSelectScreen.cs`,
  `NDeckEnchantSelectScreen.cs`, `NSimpleCardSelectScreen.cs`,
  `NChooseABundleSelectionScreen.cs`.
- `MegaCrit.Sts2.Core.Nodes.Cards/NUpgradePreview.cs`, `NEnchantPreview.cs`,
  `NTransformPreview.cs`, `NCardBundle.cs`.
- `MegaCrit.Sts2.Core.Models.RestSiteOptions/SmithRestSiteOption.cs`,
  `CookRestSiteOption.cs`; merchant removal in `OneOffSynchronizer.cs`.
- The matching model sources for Dolly's Mirror, Claws, Gnarled Hammer,
  Kifuda, Sea Glass, Scroll Boxes, Sealed Deck, Brain Leech, Room Full of
  Cheese and Wood Carvings.

## Layout and interaction

The deck and simple grids use the original four columns, 46 × 64 projected
cards, 15px top and inset, and continuous source scrolling. Eligible display
positions map back to the original deck/offer IDs; no reordered deck is
stored. Removal puts curses first while preserving order within both groups.

The old top heading, selection-count line and Clear/Detail/footer buttons are
removed from these routes. The original 24px bottom prompt projects to native
5px text with its authored black backstop. The two source navigation buttons
use their actual partly offscreen parents and moving hit rectangles. Confirm
is unavailable until its screen-specific condition is met.

Mouse View Upgrades uses the original setting checkbox and 4px bold label.
Directional navigation hides it and resets the preview flag, as the source
controller-state callback does. Only the Upgrade and Transform screen types
show this checkbox. It never upgrades a canonical card.

Single upgrade/enchantment previews use the source 300 × 422 card size,
projected to 57 × 80. Three square 65px source arrows project to 12 × 12;
their actual HBox child positions preserve the unused trailing space rather
than stretching the children to the whole 203px container. The common helper
also accepts the combat hand screen's separate scale and vertical position.

World selectors retain the original difference between an empty selection
and cancellation. Claws, Gnarled Hammer and Kifuda are not cancelable merely
because their minimum is zero. The pinned enchant screen opens an empty
preview and visibly enables Confirm, but its confirmation handler ignores
zero cards; Back still returns to the grid. That behavior is preserved.
These three relics also keep their authored maximum when the core clamps the
number of available choices, so one eligible card does not invent a different
screen or an automatic completion.

Scroll Boxes uses the original two horizontally centered bundles, their
three increasingly bright overlapping card faces, 0.8/0.85 focus scales,
0.5-second return from focus, source CommonBanner, separate three-card preview,
0.5-second position transition, 150ms whitening, and 0.4-second return. Bundle
preview clicks open the real card inspector. This screen has no artificial
350ms click gate; that gate belongs to the different choose-one-card screen.

## State, memory, and safety of inspection

No State field, canonical save version, NSH field or resource cache is added.
The world preview flag occupies the existing 16-byte
`DeckSelectionPresentation` padding. Bundle motion reuses otherwise unused
world-grid face-motion slots. Preview cards and enchantment facts are local
values; `CardHoverTarget` owns a small copied fact record when necessary, so
copying a hover target cannot leave a pointer into another temporary.

Canonical acquisition, price checks, eligibility, inventory limits and
continuations remain in the existing core commands. Preview Confirm is gated
against the current phase/revision and live UI context. Raw Confirm/Cancel
fire on release, and a key pressed before a modal/context change cannot
accept a different screen.

Unknown transformations display actual permitted candidates every 200ms.
Their presentation ordering is deterministic and independent of run RNG;
it is not a prediction of the eventual transformation result. Known Claws
replacements retain eligible upgrade/enchantment facts. Unknown replacement
holders do not advertise hover/inspection, matching their source mouse filter.

## Verification and receipts

New suites:

- `deck_selection_ui9_regression`: actual Smith and merchant entry, eligible
  filtering, curse ordering, mouse/directional checkbox behavior, both Back
  levels, unchanged preview/save bytes, stale/paused/insufficient-gold
  rejection, release confirmation, real costs, long grids and restore reset.
- `world_selection_ui9_regression`: real acquisition and event commands for
  Dolly's Mirror, Empty Cage, Electric Shrymp, Gnarled Hammer (including one
  eligible card), Claws, Astrolabe, Sea Glass, Brain Leech, Room Full of Cheese,
  Scroll Boxes, Sealed Deck and Cook; copied hover facts, unchanged preview
  RNG/save bytes, native hits, modal cancellation and exact locale font keys.

Both support an actual resource file and a capture directory. The final
acceptance must use a matching frozen runtime and the final English/Chinese
resource files. Development syntax and short diagnostic runs are useful
evidence but do not substitute for that acceptance.

At the source-ready point, current UI translation units linked with the
frozen `1606be` core plus the source-ready fixed-deck relic shortcut object
passed 437 standard-deck checks and 377 world-selector checks. The latter
includes the one-eligible optional-enchantment case, source bundle motion,
and four potion-reward routes that preserve hover/focus instead of opening
a non-original potion inspector. Three use actual Cauldron, Drowning Beacon
and Potion Courier acquisition; the fourth covers a valid legacy pending
relic-potion save. Both direct detail and keyboard detail preserve the
canonical save bytes in every route.
The runs are short development diagnostics, pending the next frozen gate.
Earlier diagnostic failures are
retained in the task record: a fixture used unremovable Bad Luck, a fixture
advanced clocks without rendering the button's re-enable transition, and a
fixture assumed a transformed card stayed at its old index although the
existing batch command removes then appends. Assertions were corrected to
test actual eligibility, real frame progression and card-count/model results.

The ordinary merchant cancel diagnostic also exposed a real presentation
bug: entering Remove had closed the retained shop rug. Returning now keeps
the existing inventory open and immediately interactive instead of replaying
its entry animation. This change does not mutate shop stock or gold.
