# UI6 cards, top bar and targeting

Source reference: **v0.111.0 / 41cef1ea**. The implementation was compared with
the owner's pinned assembly and PCK. Original images and decompiled source stay
outside the public repository. This document records the card/HUD agent's scope;
the integrated checkpoint and final validation receipt belong in `WORK_STATE.md`.

## Implemented behavior

| Area | Implementation | Original reference |
| --- | --- | --- |
| Top bar | Map/deck/settings icons enlarge on hover and use their distinct rotation responses; deck count appears at the deck icon's lower right. HP, gold, floor, map, deck, settings and pile icons expose original localized hover text. Settings opens the pause screen over an existing deck/map screen. | `NTopBarButton`, `NTopBarDeckButton`, `NTopBarPauseButton`, `top_bar.tscn`, `static_hover_tips` localization |
| Deck | Original character-colored sort strip, four independent ascending/descending sorters with a priority stack, upgrade-preview checkbox, original scrollbar images, dark backstop, footer and side back tab. Sorting does not mutate deck order or card facts. | `NDeckViewScreen`, `NCardGrid`, deck screen scene |
| Combat piles | Draw, discard and exhaust are separate screens. The former invented tab strip is gone. Draw is displayed by rarity then model ID, hiding its canonical shuffled order; discard/exhaust retain their pile order. | `NCardPileScreen`, `NDrawPileScreen`, `NDiscardPileScreen`, `NExhaustPileScreen` |
| Locale sorting | Alphabetical deck sorting uses numeric ranks generated with .NET `en-US` and `zh-CN` collation. Base and upgraded titles share each locale's rank space, including equal titles. Chinese ordering therefore follows the reference locale rather than UTF-8 bytes. | `NCardGrid` title comparison and `CardModel.Title` |
| Card targeting | First directional Confirm selects the playable card; Left/Right select a live enemy in screen-X order with wrapping; Up/Down retain that target; second Confirm commits the play; Back returns to the hand. Selection/cancel preserve canonical state. The held card and arrow follow the existing interpolated holder pose, and a play begins from that actual pose. Mouse drag/release remains separate. Targeted potions reuse the same spatial focus graph and keep Up/Down on the current target. | `NControllerCardPlay`, `NMouseCardPlay`, `NPotionHolder`, `NCardPlay.CenterCard`, `NHandCardHolder`, `NCombatRoom.UpdateCreatureNavigation`/`RestrictControllerNavigation` |
| Target arrow | Nineteen original segment images, the original head image, quadratic curvature, original target offsets, enemy tint and the valid-target head-size change replace the drawn dotted line and target rectangle. Controller movement uses the reference 14-per-second approach factor. | `NTargetingArrow`, `targeting_arrow.tscn` |
| Cursor | Original default/pressed/inspect cursor images and scaled hotspots replace the drawn cursor. Targeting hides the normal cursor. The inspect cursor is restricted to library cards. | `NCursorManager`, original cursor `.image` resources |
| Hand tooltip | When the actual fitted hand-card description overflows, its complete resolved description is included automatically before keyword tips. It retains the live instance, target and permanent facts. The result uses the same wrapping rules and cached overflow decision as the visible card. | Original card-description/hover responsibilities, adapted for the calculator's smaller card face |
| Potion tooltip | Both real bottles and empty slots expose the appropriate original description. Hover works from deck/map overlays as well as combat. | Potion holder and `POTION_SLOT` static hover text |
| Covered overlays | An open settings/confirmation overlay suppresses hover and scroll interaction with the pause/title or other controls beneath it, including input arriving between rendered frames. | Original overlay mouse ownership |

The visible “V keywords” and “I details” labels were removed from these screens.
The keyboard inspection commands remain available. Original tooltip titles can
show the actual control binding through their existing optional Hotkey template.

## Keyboard adapter

The native keyboard-only mapping supplies **A draw**, **D discard**, **F exhaust**,
**Q deck** and **E end turn**. These work in browser, X11 and the calculator host.
Existing calculator/navigation aliases remain usable: Tab for the deck, M for
map, Space for end turn, Enter/5 for Confirm, and Escape for Back.

The added calculator matrix positions were checked against the official Ndless
[`keys.h`](https://github.com/ndless-nspire/Ndless/blob/master/ndless-sdk/include/keys.h)
touchpad entries. The shared key enum appends the three pile commands at 19–21;
the debug command is a separate flag-controlled entry at 22.

Production no longer depends on the old practice/library injection UI. The
character, five-character and expansion UI regressions create necessary input
cards through explicitly named fixture helpers. Their production tests require
missing Lab hit targets and no-op Lab actions; the retained Lab UI assertions
are compiled only with `SPIRE_DEBUG_COMMANDS`.

## Performance work after implementation

* Deck/draw sorting retains one bounded 512-entry `uint16_t` index array. The
  cache invalidates on canonical revision, presentation reset, content reload,
  seed, pile or sorter changes. Type/cost sort keys are resolved once per card
  on a miss, rather than in every comparator and on every rendered frame.
* Visible pile portraits and shared face layers are prepared in one bounded,
  deduplicated batch before composing the grid. No whole-pack preflight was
  added.
* Keyboard inspection opens the actual sorted instance/deck identity directly.
  It no longer forces a full render merely to find a hit region.
* The existing eight-entry 56×70 small-card cache now includes dimensions in
  its key and accepts the restored merchant sizes, 36×50 and 44×62, alongside
  47×66. Its exact backdrop comparison and fixed raster-pool size are retained.
* A hand face sampled again by the automatic-tooltip path in the same rendered
  frame reuses its resolved dynamic description and overflow result. The key
  includes instance/deck identity, target, state revision and visual identity.
* Small rotated UI images use bounded inverse affine sampling with integer
  arithmetic; no transformed image buffer or per-frame heap allocation is
  created. Original arrow/cursor resources are ordinary addressable images.

The locale rank data adds 4,464 bytes of read-only numeric metadata. The pile
index cache is approximately 1 KiB; the existing hand and small-card raster
pools were not enlarged. These are implementation bounds, **not measured device
speedups**. Integrated ARM allocation and runtime measurements still need their
own build/validation receipt.

## Verification status at the second source checkpoint

`git diff --check` passed. Regression source covers separate pile screens,
two-stage keyboard selection/target/cancel, covered-settings hover, cached versus
uncached merchant-size composition, full automatic hand descriptions, and
immediate versus interpolated card-play origins. The correction after stage 2
also covers card and potion targets around Kaiser on both sides of the player and Living Fog summons
whose storage order differs from their screen positions. Existing active-play fixtures
were migrated specifically to two Confirms; pending-choice and blocked-input
Confirms were not globally replaced.

The rank generator was first run under the verified source receipt
`d982baf323695bb35b76af99e650e988f6bbb24d`. The restricted .NET CLI could not read
its own process timing. An approved execution outside that restriction produced
both locale tables successfully. The generator journal reports `source_changed`
because it deliberately created the public numeric include while parallel source
work continued. It is a generation receipt, **not an immutable test pass**. The
generated table is part of the following source checkpoint before compilation.

At stage 2, no native suite, final private-pack visual acceptance, WASM comparison,
ARM build or physical-device performance result had been claimed. The later
sections below record the subsequent executed gates with their own checkpoints.

## Executed stage-3 validation and repairs

The unchanged source receipt `20262f09eb1edab0035e632ca9ee4f065fb51fc4`
was tested from its immutable archive with the real corrected v23 English and
Chinese packs. These are host regression results; repeated pixel assertions
are not separate gameplay scenarios.

| Regression | English | Chinese |
| --- | ---: | ---: |
| Card/HUD, automatic descriptions and UI6 captures | 325,331 passed | 336,661 passed |
| Five-character UI | 3,138 passed | 3,138 passed |
| Colorless/potion acquisition UI | 5,689 passed | 5,689 passed |
| Keyword tooltip UI | 402,636 passed | 402,528 passed |
| UI flow | 3,861 passed | 3,861 passed |

The common input regression passed 170 checks. All these journals ended as
`passed` with `source_changed: false`. Their stems begin
`ui6-stage3-card-hud`, `five-character`, `expansion`, `keyword`, `ui-flow`
and `input`, under the immutable archive's `build/journal` directory.
The expansion driver's historical success text still says NSP2/11; both
supplied inputs and the current-profile check identify the actual v23 pack.

The character acquisition driver reached all 860 model/upgrade variants in
each language, then failed at Inky checks. English exposed a historical test
that hard-coded Chinese words. Chinese exposed a real localized-description
bug: modifier suffixes could borrow an inspected combat instance while the
compendium was open. The repair makes `card_modifier_text` respect library
model isolation, retaining the existing assertion; the English assertion
checks its own locale. These failures remain failures until the repaired
checkpoint runs the complete driver.

The integrated potion suite separately caught empty holders marked as active
click targets. `NPotionHolder.OnFocus` explicitly allows their tooltip, while
`OpenPotionPopup` requires `HasPotion`. The repair restores a disabled action
hit without removing hover geometry or the original empty-slot description.
The added actual-pack regression requires visible tooltip pixels and no popup
or canonical-state change when clicked.

Arithmetic review also found a signed-overflow path when the rotating settings
icon approached +360 degrees before focusing toward -180. The repaired Q12
multiply uses a wide product, preserves truncation and uses a shift instead of
a 64-bit division. A focused test covers the real +355-degree start, BackOut
overshoot and final normal pose. These repairs are source-complete and await
the next frozen test receipt.

Eight actual English/Chinese frames were reviewed for deck sorting, separate
pile presentation, deck-count/setting tips and potion descriptions. The first
arrow capture sampled only the start of controller interpolation; the revised
capture records both its start and twenty subsequent 33-ms frames. A static
initial frame is not treated as acceptance of the settled targeting arrow.

## Executed stage-4 verification

Receipt `8c9270300aae0f479dadb216cfb37744e9c2b220` was then tested with
the final, losslessly compacted v23 packs. Actual card/HUD passed **325,828
English / 337,158 Chinese** checks; actual potion UI passed **1,048 per
language**. These runs cover the large top-bar rotation repair, disabled empty
holder hover, and the settled original targeting-arrow textures. Both initial
and settled arrows and the empty-slot tips were inspected in both languages.
The files remain private captures; no original art is added to this repository.

The Chinese character UI now passed the complete **38,846 checks / 860
acquired variants**, including compendium isolation, all-enemy Inky behavior,
mouse second-click and drag play. English reached the same final behavior but
stopped on the fixture's lowercase `all enemies`: the pinned SHIV template
uses uppercase `ALL enemies`. The expected text has been corrected to that
actual template, with the combined Inky/all-target assertion retained. This
last adjustment changes only the test and still requires its own complete
passing receipt.

All five passing stage-4 journals record unchanged frozen source. Their names
are `ui6-stage4-card-hud-eng-7b7f174ff9fa`,
`ui6-stage4-card-hud-zhs-6e02dff6c4ba`,
`ui6-stage4-potion-eng-b12f1eec6f2e`,
`ui6-stage4-potion-zhs-b8de1789e235`, and
`ui6-stage4-character-zhs-7fa042e1ddf2`.
The failed English journal `ui6-stage4-character-eng-bac16083b382` remains a
failure and is not counted as a complete pass.

## Final targeted stage-5 result

At frozen receipt `c4431cc647b5b1710d23c877759f4f25cc6ca278`, the English
character driver passed its complete **38,846 checks / 860 acquired variants**,
including the corrected uppercase original text and the final mouse-play paths.
The Chinese complete pass remains explicitly tied to stage 4; the stage-5
change to that driver was the English expected string only.

The final native build also repeated actual English/Chinese card/HUD
(**325,828 / 337,158**) and potion UI (**1,048 each**) against the same final
packs. All five stage-5 journals ended `passed`, `source_changed: false`:

* `ui6-stage5-character-eng-93a2a7cf4431`
* `ui6-stage5-card-hud-eng-e9f59e018f94`
* `ui6-stage5-card-hud-zhs-37b6e6a9df78`
* `ui6-stage5-potion-eng-ebe482907915`
* `ui6-stage5-potion-zhs-a05026ec4c75`

The final **34 captures per language** are byte-identical to the corresponding
stage-4 frames, including the reviewed settled arrow and disabled empty-slot
tooltips. The positional arithmetic review also confirmed that the final
camera-scaled formation uses the same displayed pose for actors, hit regions,
health bars, card/potion arrows and directional targeting. None of these host
results establishes physical-calculator performance or exact original-PC
pixel equivalence. Integrated platform and timing receipts belong to the root
validation report.

## Backend lifecycle correction after stage 5

The complete native/WASM matrix found ten differing frames, all confined to
the top-bar deck icon. The remaining pixels and saves still matched. This was
a real session-lifecycle bug: the native fixture reconstructed `App` at the
same address, so `Content` again opened at generation 1, while the three static
HUD motion records retained an earlier `from_angle` and `start`. When the new
clock reached that old start, a prior unfocus animation resumed. Each WASM
scenario had fresh static storage and therefore did not reproduce the residue.

Both production `App::init` overloads now explicitly invalidate those motion
records, as they already invalidate hand-card caches. The driver was left
unchanged. The new card/HUD regression reconstructs the real App at the same
address, checks its repeated generation-1 open, induces a complete hover and
unfocus, then compares a fresh session at the previous start time with clean
HUD pixels. It covers memory and `ResourceSource` initialization.

Frozen repair receipt `a59cdc2bc139fcf6be7a99f0655fa8ee4bd1e8ea` passed
actual card/HUD **325,846 English / 337,176 Chinese** checks under journals
`ui6-stage6-card-hud-eng-10ee36ca62de` and
`ui6-stage6-card-hud-zhs-09ebc20a1e41`, both unchanged-source passes. Their
30 captured card/HUD frames per language remain byte-identical to the already
reviewed stage-5 counterparts. The rooms agent separately completed the full
world/rooms native/WASM comparison at this same repair: **2,376 exact frames
and 106 exact saves** across both languages. The ten previous deck-icon
mismatches disappeared without weakening the checker or resetting motion in
the driver. Earlier failed matrix runs remain recorded as failures.

## Sanitizer arithmetic follow-up

The full stage-6 ASan/UBSan run found one further overflow in the settings
icon's open rotation: `6200 * 360000` was evaluated as signed 32-bit before
division. The earlier wide-interpolation repair did not cover this separate
clock-to-angle expression. The normal host passes above therefore did not
establish sanitizer acceptance; the failed sanitizer journal is retained.

The clock product now stays explicitly unsigned: its maximum
`6282 * 360000 = 2261520000` fits uint32, and division returns an angle below
360000. The pressed branch also checks its 250-ms endpoint before multiplying
elapsed time, preventing an extended hold from overflowing. Phase conversion
reduces whole turns before its Q12 multiply, preserving periodic output while
bounding that product below signed 32-bit range. No 64-bit division was added.
The existing 6200-ms expectation is retained and strengthened to its exact
angle; new cases cover 6282/6283-ms wrap and a 600000-ms hold. The repaired
checkpoint received a new targeted sanitizer run rather than inheriting the
earlier ordinary-build acceptance.

Frozen stage 8 `96f7b5c6914762fad998f03ab5da1b4c283ecaa0` passed the
Clang 18 AddressSanitizer/UndefinedBehaviorSanitizer card/HUD target with
`-O1`, a 32-MiB stack, leak detection disabled and sanitizer errors configured
to stop execution. The synthetic profile completed **385,274 checks**;
the final private English and Chinese resources completed **318,593** and
**329,923 checks**, respectively. The retained 6200-ms angle expectation,
6282/6283-ms period endpoints and extended hold all execute in each run.
There were no sanitizer reports, and all three journals recorded unchanged
source. Receipts are `ui6-asan8-cards-build-33d169515129`,
`ui6-asan8-cards-synthetic-f926492e7833`,
`ui6-asan8-cards-eng-74b85f054443` and
`ui6-asan8-cards-zhs-c082ff03921d` under the frozen stage-8 journal directory.
These are targeted sanitizer results; the parent report records final full
native, portable and backend acceptance separately.

## Remaining fidelity boundaries to keep explicit

* The 320×240 grid uses readable 47×66 cards in discrete ten-card pages. The
  reference game's continuous grid scrolling and draggable scrollbar thumb are
  not reproduced by the current page adapter.
* Single-target arrows are restored. Original all-target/ally reticle animations
  and the deck-count increase pulse remain separate fidelity work.
* Private v23 assets are needed for the new cursor, arrow and sort/scroll images.
  Synthetic and historical packs remain logic fixtures and cannot establish
  visual parity for those assets.

Do not count diagnostic probes, source-only tests or agreeing backends as original
game parity. Append actual integrated validation and reviewed screenshots only
after the corresponding frozen source has passed.
