# UI7 cards, collections and potion interaction

Reference: **v0.111.0 / 41cef1ea**. This is an implementation-stage inventory,
not a fidelity percentage or a completed acceptance report. Original assembly,
scene, shader and artwork inputs stay private. Public generators emit numeric
metadata and asset paths only.

## Surface and interaction inventory

| Surface | Pinned source contract | Implementation in this stage | Verification state |
| --- | --- | --- | --- |
| Deck, draw, discard and exhaust grids | `NCardPileScreen`, `NDeckViewScreen`, `NCardGrid`, `ScrollHelper`, `NScrollbar` | Four columns; continuous content offset; wheel, content drag and draggable thumb; row navigation; deck-only sorting and upgraded preview. Retired ten-card page actions do nothing. | Meaningful wheel, drag, focus and save-isolation fixtures migrated; current complete stage awaits a checkpoint build. |
| Mandatory deck/combat selections | `NCardGridSelectionScreen`, `NDeckCardSelectScreen`, `NCombatPileCardSelectScreen`, `NSimpleCardSelectScreen`, `NDeckUpgradeSelectScreen`, `NChooseACardSelectionScreen` | The same continuous grid owns pointer capture and scrolling; mandatory choices retain their original confirmation and cannot be consumed by unrelated pile input. Three-card offers remain separate. | Existing gameplay-selection suites retained and adapted; no claim that every special selection has matched PC pixel output. |
| Card compendium | `NCardLibrary`, `NCardLibraryGrid`, `NSearchBar`, `card_library.tscn`, pool/type/rarity/cost controls | Independent page, three columns, eight exclusive pool controls, multi-toggle type/rarity/cost predicates, priority sorting, localized card count, upgrade preview, search and original Back geometry. | New `library_ui7_regression` covers the complete 558-model union, source pool counts, predicates, ordering, discovery, scrolling and capture invalidation. Runtime execution is pending. |
| Card search | `NSearchBar`, `NCardLibrary` search predicates | 250 ms debounce, Enter commit, UTF-8, case/space normalization and title/body search for discovered cards; English rarity names retain the original additional match. Browser IME commits, X11 XIM commits and calculator physical letters/numbers reach the actual application field. | UTF-8 and calculator edge-adapter fixtures added. Native/browser/Firebird text interaction still requires actual execution. |
| Unseen cards | `NCard.UpdateVisuals`, `UpdateEnergyCostVisuals`, `blur.gdshader` | Unknown localized title/body, `?` energy, hidden star cost and blurred original portrait. Opening or carousel-navigation cannot reveal an unseen card. | Unknown/seen and carousel-boundary assertions added. The bounded CPU blur is an approximation of the original Gaussian shader, not shader equivalence. |
| Relic compendium | `NRelicCollection`, `NRelicCollectionCategory`, `NRelicCollectionEntry`, `NRelic` | Independent continuous category page, ten columns, original starter rows and eight Ancient subgroups, localized ordering, outlines, known-character colors, unknown tooltip and hover animation. Click focuses the tooltip rather than opening an invented detail dialog. | Numeric membership and locale metadata generated; fixture checks categories, uniqueness, keyboard rows and scrolling. Actual assets/captures pending. |
| Potion compendium | `NPotionLab`, `NPotionLabCategory`, `NLabPotionHolder`, `NPotion` | Independent continuous Common/Uncommon/Rare/Special page with 64 entries, pool grouping, original outlines, unknown state, character color and hover scale. | Same collection fixture preserves all 64 potion acquisition tests separately from the hover-only directory. Actual assets/captures pending. |
| Card inspection | `NInspectCardScreen`, `NInspectCardUpgradeToggle`, `NCard`, `inspect_card_screen.tscn` | Centered card and full backstop, delayed entrance, spring scale, neighboring arrows, no-wrap carousel, canonical upgrade toggle and automatic related-card/text tips. Root adds run-history carousel context. | Old card-model census still exercises 860 character variants and all 106 colorless variants through real card hits. Current stage must execute those migrated suites. |
| Held card and targets | `NMouseCardPlay`, `NControllerCardPlay`, `NCardPlay`, `NTargetManager`, `NTargetingArrow`, `NTargetReticle` | Original arrow assets retained; single-enemy, self, all-enemy and random-enemy presentations use the corresponding original target type. Scene punch offsets also move target endpoints. Spatial keyboard targets and select/aim/confirm remain functional. | Existing targeting fixtures plus new pulse/reticle tests; pending current-stage execution. |
| Top bar and potion navigation | `NTopBar`, `NTopBarDeckButton`, `NPotionHolder` | Deck-gain pulse; P enters holder focus, including empty holders; Confirm opens an occupied holder. Mouse clicks restore mouse mode even with Keyboard Mode enabled. | Lifecycle/overflow coverage from UI6 retained; UI7 P-focus regressions migrated. |
| Obtaining potions | `PotionCmd.TryToProcure`, `PotionModel`, `PotionReward`, `NPotionHolder`, `NPotionPopup` | First empty slot is filled. A full belt reports the source error without silently replacing a slot. The player can discard an existing potion and retry a pending reward. No legacy choose-slot page remains. | Full-belt State/RNG preservation, manual discard/retry and Sozu fixtures added or updated; merchant failure uses the same presentation entry point. |
| Potion visibility metadata | `PotionCmd`, `MerchantPotionEntry`, `PotionReward.MarkContentAsSeen`, `NHoverTipSet` | Root records successful possession, shop offers and visible reward offers in independent progress metadata. The source also marks an actual canonical tooltip as seen. | Call-site evidence supplied to root. Remaining special reward/tooltip discovery paths need audit and execution; merely listing the catalog must not discover it. |

## Metadata and storage decisions

`generate_library_metadata.py` produces 558 numeric pool/Star-X rows from the
pinned source. Original ordinary pool counts are 85 Ironclad, 86 Silent,
86 Regent, 86 Necrobinder, 86 Defect and 53 Colorless; the Ancient and Misc
controls contain 19 and 67 models and can overlap character pools. There is
no invented All filter.

`generate_collection_metadata.py` emits numeric localized title ranks and
Ancient/potion membership. Before generating collection ranks, its installed
ICU comparison is checked against all 2,232 existing .NET-generated English
and Chinese card ranks. This short generation check passed; it is not a
substitute for the frozen converter/regression gate.

The resource append contains 372 original image paths at auxiliary indices
1505–1876: 296 available relic outlines, 64 potion outlines, four additional
Yummy Cookie variants and eight Ancient icons. Forgotten Soul has no outline
at the pinned path and keeps `-1`; no outline is invented. The root pipeline
owns conversion, split-pack accounting and resource validation.

The card library caches only its filtered/sorted index list until the query,
filters, upgraded preview or content generation changes. UTF-8 descriptions
are not rebuilt on every hover. This is an implementation decision, **not a
measured performance improvement**. The new original 53×75 grid and 53×75
merchant hover exceed the old small-face cache height. Actual cold/warm
measurements after functional acceptance must guide any cache-budget change.

## Explicit remaining differences and evidence limits

- The scrollbar thumb currently follows the displayed scroll value directly;
  the original thumb has an additional 50 ms SmoothDamp. Content interpolation,
  wheel movement and overscroll recovery are implemented separately.
- Card inspection closes immediately; its original closing fade and some
  related-tip positioning/stacking still need completion. There is no claim
  that every upgrade/select screen has all original entrance/exit motion.
- Collection tooltip bodies are supported, but the complete original chain of
  related relic/potion tips is not yet reconstructed. The special randomized
  Yummy Cookie preview is not reproduced; the existing character variant is
  used consistently. Ancient headings currently become known from an observed
  relic, while the source also accepts prior Ancient encounter statistics.
- Card-library per-card statistics and its statistics checkbox are not yet
  implemented. Timeline lock visibility is outside the agreed Timeline scope;
  absent observation is treated as NotSeen, never misrepresented as Locked.
- Text input currently appends or deletes the final UTF-8 character and supports
  clear/commit/cancel. Original LineEdit selection, caret movement and native
  clipboard editing are not complete. The calculator has physical ASCII entry,
  not an invented Chinese IME. Browser paste and IME commits are supported.
- Top-bar focus does not yet reproduce every room/boss target or the original
  downward transition into the first relic. Full keyboard remapping belongs to
  the shared settings/input audit.
- No supported single-player card uses the original multiplayer ally-target
  path. It remains excluded with multiplayer; this does not prove all possible
  ally/Osty reticle geometry. Character/sprite scaling remains coordinated
  with the combat source audit.
- Short Clang syntax checks passed for the application, X11 host and affected
  UI fixtures. No dirty-tree native build, completed UI7 native/WASM equality,
  browser acceptance, Firebird run, or performance improvement is claimed.

The physical key adapter uses public Ndless `ndless-sdk/include/keys.h`
numeric matrix facts, reviewed at blob `71f141c361b8333056117f32ff87403aa7dc6895`.
The actual device target is the same touchpad/CX keyboard as the existing host.
