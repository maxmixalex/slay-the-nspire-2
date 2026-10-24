# UI7 cards, collections and potion interaction

Reference: **v0.111.0 / 41cef1ea**. This is an implementation-stage inventory,
not a fidelity percentage or a completed acceptance report. Original assembly,
scene, shader and artwork inputs stay private. Public generators emit numeric
metadata and asset paths only.

## Surface and interaction inventory

| Surface | Pinned source contract | Implementation in this stage | Verification state |
| --- | --- | --- | --- |
| Deck, draw, discard and exhaust grids | `NCardPileScreen`, `NDeckViewScreen`, `NCardGrid`, `ScrollHelper`, `NScrollbar` | Four columns; continuous content offset; wheel, content drag and draggable thumb; row navigation; deck-only sorting and upgraded preview. Retired ten-card page actions do nothing. The follow-up projects the verified 1680×1260 layout to 46×64 card faces with fractional row spacing. | Checkpoint 4 input, grid and selection fixtures passed where recorded; the complete run still has unrelated and migrated-UI failures. The new viewport projection has passed short syntax checks and awaits checkpoint-6 execution. |
| Mandatory deck/combat selections | `NCardGridSelectionScreen`, `NDeckCardSelectScreen`, `NCombatPileCardSelectScreen`, `NSimpleCardSelectScreen`, `NDeckUpgradeSelectScreen`, `NChooseACardSelectionScreen` | The same continuous grid owns pointer capture and scrolling; mandatory choices retain their original confirmation and cannot be consumed by unrelated pile input. Three-card offers remain separate. | Existing gameplay-selection suites retained and adapted; no claim that every special selection has matched PC pixel output. |
| Card compendium | `NCardLibrary`, `NCardLibraryGrid`, `NSearchBar`, `card_library.tscn`, pool/type/rarity/cost controls | Independent page, eight exclusive pool controls, multi-toggle type/rarity/cost predicates, priority sorting, localized card count, upgrade preview, search and shared source-based Back geometry. Follow-up corrects checkpoint 4's three-column assumption to the actual four-column viewport and adds the original Victories checkbox. | Checkpoint 4 `library_ui7_regression` passed the 558-model union, pool counts, predicates, ordering, discovery, scrolling and capture invalidation. Post-checkpoint layout, statistics, observation and hover fixes have only passed short syntax checks. |
| Card search | `NSearchBar`, `NCardLibrary` search predicates | 250 ms debounce, Enter commit, UTF-8, case/space normalization and title/body search for discovered cards; English rarity names retain the original additional match. Browser IME commits, X11 XIM commits and calculator physical letters/numbers reach the actual application field. | UTF-8 and calculator edge-adapter fixtures added. The native adapter fixture passed at checkpoint 4. Browser/XIM/Firebird text interaction still requires actual execution. |
| Unseen cards | `NCard.UpdateVisuals`, `UpdateEnergyCostVisuals`, `blur.gdshader` | Unknown localized title/body, `?` energy, hidden star cost and blurred original portrait. Opening or carousel-navigation cannot reveal an unseen card. | Unknown/seen and carousel-boundary assertions added. The bounded CPU blur is an approximation of the original Gaussian shader, not shader equivalence. |
| Relic compendium | `NRelicCollection`, `NRelicCollectionCategory`, `NRelicCollectionEntry`, `NRelic` | Independent continuous category page, ten columns, original starter rows and eight Ancient subgroups, localized ordering, outlines, known-character colors, unknown tooltip and hover animation. Click focuses the tooltip rather than opening an invented detail dialog. | Numeric membership and locale metadata generated; fixture checks categories, uniqueness, keyboard rows and scrolling. Actual assets/captures pending. |
| Potion compendium | `NPotionLab`, `NPotionLabCategory`, `NLabPotionHolder`, `NPotion` | Independent continuous Common/Uncommon/Rare/Special page with 64 entries, pool grouping, original outlines, unknown state, character color and hover scale. | Same collection fixture preserves all 64 potion acquisition tests separately from the hover-only directory. Actual assets/captures pending. |
| Card inspection | `NInspectCardScreen`, `NInspectCardUpgradeToggle`, `NCard`, `inspect_card_screen.tscn` | Centered card and full backstop, delayed entrance, spring scale, neighboring arrows, no-wrap carousel, canonical upgrade toggle and automatic related-card/text tips. Root adds run-history carousel context. Confirm toggles the upgrade checkbox and Back closes the inspector. | Old card-model census still exercises 860 character variants and all 106 colorless variants through real card hits. Checkpoint 4 character suite passed all 860 variants. Expansion reached its acquisition section but failed a later modal fixture; the complete replacement expansion run remains pending. |
| Held card and targets | `NMouseCardPlay`, `NControllerCardPlay`, `NCardPlay`, `NTargetManager`, `NTargetingArrow`, `NTargetReticle` | Original arrow assets retained; single-enemy, self, all-enemy and random-enemy presentations use the corresponding original target type. Scene punch offsets also move target endpoints. Spatial keyboard targets and select/aim/confirm remain functional. | Existing targeting fixtures plus new pulse/reticle tests; pending current-stage execution. |
| Top bar and potion navigation | `NTopBar`, `NTopBarDeckButton`, `NPotionHolder` | Deck-gain pulse; P enters holder focus, including empty holders; Confirm opens an occupied holder. Mouse clicks restore mouse mode even with Keyboard Mode enabled. | Lifecycle/overflow coverage from UI6 retained; UI7 P-focus regressions migrated. |
| Obtaining potions | `PotionCmd.TryToProcure`, `PotionModel`, `PotionReward`, `NPotionHolder`, `NPotionPopup` | First empty slot is filled. A full belt reports the source error without silently replacing a slot. The player can discard an existing potion and retry a pending reward. No legacy choose-slot page remains. | Full-belt State/RNG preservation, manual discard/retry and Sozu fixtures added or updated; merchant failure uses the same presentation entry point. |
| Potion visibility metadata | `PotionCmd`, `MerchantPotionEntry`, `PotionReward.MarkContentAsSeen`, `NHoverTipSet` | Successful possession across all belt slots, shop entries, ordinary/event/relic reward offers and actual canonical tooltips update independent progress metadata. An unopened CardReward does not discover its hidden cards. | The new actual Colorful Philosophers reward fixture checks hidden versus opened cards, read-only rendering, full-belt discovery and no catalog self-discovery. It awaits the next checkpoint run. |

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
measured performance improvement**. After the viewport correction, 46×64 grid
faces and 43×61 merchant faces fit the existing eight-slot 56×70 pool; those
two sizes are admitted without increasing its memory budget. The merchant's
53×75 focused face still exceeds its height and remains uncached. Exact cached
versus uncached full-surface pixel checks now include the two new sizes.
Actual cold/warm measurements after functional acceptance must guide any
further cache-budget change; the new cases have not yet executed formally.

## Post-checkpoint metadata and runtime findings

The original `NCard.Model` setter marks a locally owned run/combat model when
bound; `CardSelectCmd` and `RelicSelectCmd` mark all models in their actual
selection screens. `CardReward.MarkContentAsSeen` and the special-card variant
are empty. The follow-up now preserves this distinction through explicit
choice creation and first observed card/tooltip binding, without scanning the
complete catalog on each frame. Shop/reward observation is revision-gated.
The collection hover ring also no longer lets an unrelated, unfocused item
reset another item's scale merely because their indices share a cache slot.

Optional progress now writes `NSH2`, adding per-model card wins and the last
successful metadata-save timestamp. It still reads `NSH1`, retaining recorded
history and discoveries while setting new fields to zero. No wins or missing
timestamps are inferred from old histories. A failed save retains dirty state
and the prior successful timestamp. `decode_ui_progress_summary` reads only
the bounded fixed fields and five character statistics, with no history
allocation or history traversal, for the parent's profile-selection page.
The gameplay save format and RNG are unchanged. These post-checkpoint changes
have passed short syntax checks; their formal tests are still pending.

A further source audit corrected a previous layout assumption:
`NGame.ApplyDisplaySettings(FourByThree)` sets **1680×1260**, not 1440×1080.
`NCardGrid.Columns` derives its column count from the scroll-container width:
ordinary grids remain four columns, while the card library must also have four
at this viewport. The 53×75 cards and 2/9 projection in checkpoint 4 are a port
adaptation, not evidence of the original viewport. The follow-up uses the
shared `reference_viewport` projection for ordinary/library grids and relic/
potion collection geometry. It preserves authored fractional row spacing
instead of accumulating a rounded row height through large decks. Card
library filter text retains an eight-pixel readability floor. The inspector's
larger card and some selection-page header/footer spacing remain adaptations,
not source-exact viewport projections. Formal captures of the revised layout
are pending.

The Web host now queues three-profile operations through optional exports and
retains the established three imports. Slot one keeps all historical storage
keys; slots two and three scope run versions 3–15 and progress separately.
Settings and language remain global. Switching requires the current metadata
to be stored first; selector-write failure leaves the active slot unchanged.
Deletion records a bounded recovery journal before removing any original
key, and failed deletion rolls back or keeps that journal for recovery on the
next launch. The independent JavaScript regression covers these failure paths
and isolation. Only syntax checks have run; actual browser/profile execution
is still required. A nonempty legacy run with no readable metadata is shown
as an existing slot with unknown time, while Continue retains its full save
decoder. Profile summaries temporarily reuse the existing transfer buffer;
they do not allocate another 512-KiB buffer or load all histories at startup.

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
  used consistently. Ancient headings now accept observed Ancient events as
  well as observed relics; that follow-up awaits execution.
- Card-library Victories and its checkbox are implemented after checkpoint 4,
  with one win per distinct final-deck model and saturating counters. Their
  formal execution is pending. Timeline lock visibility is outside the agreed
  Timeline scope; absent observation is NotSeen, never misrepresented as Locked.
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
- Checkpoint 4 built and its new library/progress suites passed; the complete
  gate failed 16 of 84 suites. The follow-up fixes remain unverified beyond
  short syntax checks. No dirty-tree native build, complete UI7 native/WASM
  equality, browser acceptance, Firebird run, or performance gain is claimed.

The physical key adapter uses public Ndless `ndless-sdk/include/keys.h`
numeric matrix facts, reviewed at blob `71f141c361b8333056117f32ff87403aa7dc6895`.
The actual device target is the same touchpad/CX keyboard as the existing host.
