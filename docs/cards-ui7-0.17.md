# UI7 cards, collections and potion interaction

Reference: **v0.111.0 / 41cef1ea**. This is an implementation-stage inventory,
not a fidelity percentage or a completed acceptance report. Original assembly,
scene, shader and artwork inputs stay private. Public generators emit numeric
metadata and asset paths only.

## UI8 hand geometry, selection and inspector transitions

`HandPosHelper`, `NPlayerHand.RefreshLayout`, `NHandCardHolder` and the
original holder scenes now drive the combat hand. All55 authored positions
and angles for1–10 cards have been recovered; the angle arrays were checked
against the pinned assembly's initialized numeric data. Only numeric
metadata is committed. The fan uses the1680×1260 projection and the
original0.8 base scale, reducing it to0.76/0.72/0.68 for8/9/10 cards.

A hand cache is80×112 pixels, but its actual card frame is69×97 with
transparent padding for energy and star artwork. The frame corresponds to
the original300×422 card, so the holder's rotation pivot now follows that
frame's center. The cache's outer bounds no longer determine scale or the
mouse hitbox. The focused card projects to Y200 and Q10 scale848, while
`NCardPlay.CenterCard` targets Y210 and scale636. Immediate hover resets Y,
scale and angle; X keeps the original interpolation. The nearest three
neighbours move outward by75/50/25 source pixels. Position/scale/angle
responses retain the original7/8/10-per-second rates.

Every hand hit uses the current visible pose and an inverse rotation test.
The old fixed bottom rail and the focused card's hardcoded80px hit column
have been removed. Tooltip ownership uses that same card frame. Selection
reparents the holder in the original: the port therefore contracts the
remaining displayed hand immediately without changing canonical piles.
During a real drag, other hand holders ignore pointer events. Self/all-target
cards follow the mouse; a single-target card centers after entering the
source play zone. Existing instance capture, cancel-on-modal, queued card
transfers and save-state comparisons remain in place.

Card transfers now use the same registered frame scale. `PileType.Play`
projects the full viewport center minus100 source pixels to Y101 and the
source0.8 scale to678. Generated previews reach source scale1, use325px
source spacing and the original50px downshift. Their250ms entrance and
1,200ms hold are unchanged. Flight arcs use the1680×1260 projection; the
existing draw/discard/exhaust destination centers remain attached to the
port's visible pile controls.

The original CombatUi node order places energy/stars, end turn and piles
before preview/hand/played cards. Stage 4 implements this order, so an outer
focused card covers the controls beneath its visible face. The card's
visible face owns overlapping pointer input. Hovering away restores access
to controls beneath it, as in the original scene.

The inspector now releases its modal input immediately on close, retaining
a frozen presentation for the original100ms card/arrow and250ms backstop
fade. Continuous collection grids retain a separate50ms smoothed scrollbar
thumb; dragging and wheel movement still select the current visible models.
Stage2's card-HUD suite passed, including full save invariants; later hand
geometry and layer changes require the next immutable gate and actual
bilingual screenshots. This is implementation evidence, not95% acceptance.

## Checkpoint 9 implementation and evidence

Checkpoint 8 (`e378d4a20b5a2a43a419efad513ba6aed135840a`) built successfully
and passed **83 of 87** native suites. The actual four-column library/model
capture fix, inspector geometry, new grid-cache pixel cases, library metadata,
and progress fixtures are included in that checkpoint. This is evidence for
those tests, not a whole-game visual-parity result. The Web profile-storage
test ran from checkpoint 7 and passed with receipt
`ui7-web-profile-storage-82c03e149d3b`; browser profile interaction is a separate
acceptance gate.

The next snapshot adds original room/floor/boss HUD nodes, exact dynamic boss
names and double-boss ordering, and the first-relic keyboard transition.
`NTopBar.UpdateNavigation`, `NPotionContainer.UpdateNavigation`,
`NRelicInventory.UpdateNavigation`, `NRelicInventoryHolder`, `top_bar.tscn`
and `run.tscn` are the private reference sources. The relic inventory no
longer truncates after 15 entries: its 68-pixel source cells form 23 columns
at the verified 1680-pixel viewport. Left/right wrap through all owned relics,
Up returns to the first potion, and Down returns to the active screen. R is
still a port shortcut; it now focuses the original-style inventory rather
than opening the former in-run paginated panel.

The v27 resource append contains **56 original image paths** at auxiliary
indices **1889–1944**, plus eight reused Ancient icons. Room and boss symbols
use the source run-history icons and outlines, not animated map sprites.
The count/visibility/tooltip logic distinguishes the current room, event
parent rooms, first/second boss, and the final victory room. No strings, game
rules, gameplay-save fields or RNG draws are added. Relic hover state uses a
bounded 320-holder array of approximately 3.8 KiB; no pixel cache is enlarged.

Three checkpoint-8 failures have concrete follow-up fixes. The deck sorting
control's source-height hit area no longer overlaps the first card row. The
debug full-belt fixture explicitly opens its debug detail before asserting
that acquisition is disabled. The combat-selection fixture obtains its fresh
card click from the real hit rectangle while retaining the stale-press and
exact selected-model/save assertions. These fixes and the new HUD paths have
passed short Clang syntax checks only; their frozen native and actual bilingual
resource execution remains pending at this snapshot.

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
| Top bar and potion navigation | `NTopBar`, `NTopBarDeckButton`, `NPotionHolder`, `NRelicInventory` | Deck-gain pulse; P enters holder focus, including empty holders; Confirm opens an occupied holder. Room, Floor and Boss complete the rightward chain; Down enters the first relic. Mouse clicks restore mouse mode even with Keyboard Mode enabled. | Checkpoint 8 card-HUD suite passed. New node, dynamic-name, double-boss, per-holder hover and relic-navigation assertions are implemented for checkpoint 9 and await frozen execution. |
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
versus uncached full-surface pixel checks now include the two new sizes and
passed in checkpoint 8's synthetic suite.
Actual cold/warm measurements after functional acceptance must guide any
further cache-budget change; final actual-resource measurements remain pending.

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
The gameplay save format and RNG are unchanged. Progress and library regression
suites passed at checkpoint 8; that does not establish browser or calculator
profile acceptance.

A further source audit corrected a previous layout assumption:
`NGame.ApplyDisplaySettings(FourByThree)` sets **1680×1260**, not 1440×1080.
`NCardGrid.Columns` derives its column count from the scroll-container width:
ordinary grids remain four columns, while the card library must also have four
at this viewport. The 53×75 cards and 2/9 projection in checkpoint 4 are a port
adaptation, not evidence of the original viewport. The follow-up uses the
shared `reference_viewport` projection for ordinary/library grids and relic/
potion collection geometry. It preserves authored fractional row spacing
instead of accumulating a rounded row height through large decks. Card
library filter text retains an eight-pixel readability floor. Checkpoint 8 also
projects the inspector card to 114×161, its owner bounds to (93,40,134,159),
and its arrows to the actual 1680-pixel scene. Full descriptions and related
tooltips remain available. Some selection-page header/footer spacing and the
22-pixel top-bar height retain explicit readability adaptations. Formal
captures of the final revised resources are pending.

The Web host now queues three-profile operations through optional exports and
retains the established three imports. Slot one keeps all historical storage
keys; slots two and three scope run versions 3–15 and progress separately.
The checkpoint-9 host follow-up makes gameplay preferences profile-specific;
language/display remain global. Switching requires the current metadata
to be stored first; selector-write failure leaves the active slot unchanged.
Deletion records a bounded recovery journal before removing any original
key, and failed deletion rolls back or keeps that journal for recovery on the
next launch. The independent JavaScript regression covers these failure paths
and isolation. The frozen checkpoint-7 JavaScript suite passed; actual
browser/profile execution is still required. A nonempty legacy run with no readable metadata is shown
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
- The new top-bar room/boss/relic chain awaits frozen execution and actual art
  review. Full keyboard remapping belongs to the shared settings/input audit.
  The original run timer is not yet rendered. Three-slot potion paging, the
  retained 22-pixel HUD height and minimum text size are port adaptations.
- Owned-relic tooltips now wrap the full main body to their available screen
  height, but complete related-tip chains and the legacy relic detail window
  remain open fidelity work. A hover animation or source sprite alone does not
  establish complete relic UI parity.
- No supported single-player card uses the original multiplayer ally-target
  path. It remains excluded with multiplayer; this does not prove all possible
  ally/Osty reticle geometry. Character/sprite scaling remains coordinated
  with the combat source audit.
- Checkpoint 8 built and passed 83 of 87 suites. The new checkpoint-9 changes
  are still unverified beyond short syntax checks. No dirty-tree native build,
  complete UI7 native/WASM equality, browser acceptance, Firebird run, or
  performance gain is claimed here. Earlier table entries describe the gates
  available at their implementation checkpoint; this section records the
  latest complete native result.

The physical key adapter uses public Ndless `ndless-sdk/include/keys.h`
numeric matrix facts, reviewed at blob `71f141c361b8333056117f32ff87403aa7dc6895`.
The actual device target is the same touchpad/CX keyboard as the existing host.
