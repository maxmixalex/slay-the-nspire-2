# UI16: selection, deck sorters, treasure and reference checks

Reference: Slay the Spire 2 `v0.111.0 / 41cef1ea`. This pass starts from the recovered UI15 delivery, not the older published UI10 checkout. Original PCK/DLL assets and render captures remain private.

## Implemented

### The tucked played card follows the eye

`NCombatUi` moves the current play-pile card toward `NPeekButton.CurrentCardMarker` during the 250 ms cubic peek transition and halves its scale. The source `scenes/combat/peek_button.tscn` owns that marker under the eye, at `(64, -105)` in a 128 px button.

The LCD eye is at `(5, 108)`, 24 px square. Previously its button had moved while the card still targeted the independent hard-coded center `(31, 88)`. Both now share constants in `include/combat_layout.hpp`; the tucked card targets `(18, 78)`. The one-pixel horizontal adjustment and the vertical clearance are an explicit LCD adaptation for the larger readable card faces. They keep the left edge visible and the lower edge above the eye. Existing duration, easing, half-scale and pending-selection behavior remain intact.

`combat_layout_regression` checks endpoints, intermediate interpolation and bounds. `world_ui_regression` now plays an actual Prepared+ card, waits for its two real draw commands, peeks its pending discard choice, and verifies the play-pile sprite and unchanged canonical continuation. English and Simplified Chinese captures show the resulting layout.

### Deck sorters use 8 px text

The four original horizontal sort controls retain their order, label source, spacing and arrows. Their text increases from 6 px to 8 px; the strip grows from 12 px to 18 px, with 14/15 px button art and a 16 px pointer hit area. The first card row stays at its existing position and does not overlap the controls. Long labels retain horizontal overflow behavior. `pile_sort_label` now measures/prepares the same 8 px text used for painting.

This requires no additional image assets. Exact 8 px sort-label glyphs were verified in the shipped UI15 English and Simplified Chinese packs; the locale/font pass covers the other languages.

### Treasure title uses the authored banner label rectangle

`scenes/ui/common_banner.tscn` defines a 654 × 162 banner with the label inset 100 px on each side, 21 px above and 56 px below. The previous LCD text box `(108,61,104,20)` was too wide and low and permitted a second line below the ribbon.

The 125 × 31 LCD banner keeps its image position. Its title now uses a single 10 px line at `(117,62)` in an 87 px wide box, matching the projected internal label rectangle and its vertical center. Long labels scroll horizontally within that box. English “What's Inside?” and Chinese “里面有什么？” render on the ribbon. Missing banner text falls back to English “Treasure”.

## Verified original behavior retained

### “View Upgrades” is an original pointer control

`NDeckUpgradeSelectScreen._Ready` binds `%Upgrades` to `ToggleShowUpgrades`. Its controller update hides the control during directional navigation and resets the preview. The port already does the same: the upgrade checkbox appears for pointer input, toggles the entire selection grid, and disappears/reset when directional navigation takes over. It is not a newly introduced extra choice. `deck_selection_ui9_regression` covers pointer toggle, directional reset and the actual upgrade confirmation.

### Tablet of Truth and Smith intentionally use different effects

`TabletOfTruth` upgrades a random eligible card with `CardPreviewStyle.EventLayout`; its final whole-deck option upgrades eligible cards using `MessyLayout`, with 0.2 s normal / 0.1 s fast staggering. It does not use the Smith picker. `SmithRestSiteOption` calls the upgrade command with preview style `None` and separately creates `NCardSmithVfx`.

UI15 already implements these paths separately, including detached card snapshots, random eligible-card selection and whole-deck staggering. `combat_ui15_regression` exercises the original Tablet options; the room/deck regressions exercise Smith. Their visual difference should remain.

### Map point control size already matches the reference

`normal_map_point.tscn` gives the icon and outline a 92 × 92 control. At the map's existing 0.22 projection this is 20.24 px, quantized to 20 px. Source hover scale 1.45 corresponds to 29 px; the allowed-node pulse around 1.2 ± 0.25 corresponds to 24 ± 5 px. These sizes already match the runtime.

The apparent reduction between older deliveries and UI15 comes from restored atlas margins: the original map sprites have 128 × 128 logical canvases, including transparent margins. For example the monster region is 66 × 68 with margin `(30,29,62,60)`. Drawing that cropped region as if it filled the entire 92 px source control had enlarged the visible ink incorrectly. UI15 restores the complete canvas and keeps icon/outline aligned. This pass preserves the correct 20/29 px controls.

## Verification

The Clang 18 Release native build covers these eight relevant targets: `combat_layout_regression`, `card_hud_ui_regression`, `world_room_ui_regression`, `world_map_ui_regression`, `deck_selection_ui9_regression`, `combat_ui15_regression`, `world_ui_regression`, and `merchant_preparation_regression`.

Actual UI15 English / Simplified Chinese resources passed:

| Suite | English | Simplified Chinese |
|---|---:|---:|
| Card / HUD | 713,342 checks | 790,968 checks |
| World rooms | 41,676 | 41,720 |
| World UI and played-card peek | 33,637 | 33,645 |
| World map | 13,031 | 13,031 |
| Deck / Smith selection | 552,286 | 592,618 |

The combat UI15 suite passed 1,094,485 checks with the actual common and English language files. The merchant preparation suite passed 25,515 checks and completed its optional cold idle/font work.

Two old scene regressions assumed all merchant background preparation would finish within 128 native updates. That assumption was invalid for the fully retained UI15 animation resources: native replay deliberately yields after four decoder units per update, and the optional idle sequence continues after stock is already interactive. Their assertions now wait for the visible stock or the hover set actually used by that test, under the existing preparation suite's bounded 8,192-update offline limit. Empty synthetic fixtures remain valid without asynchronous preparation. No runtime queue, deadline, decoder allowance or game behavior was changed for these test adjustments.

Native execution checks behavior and render geometry. It does not establish calculator frame rate or storage performance; ordinary OS validation and final package measurements belong to the integrated UI16 delivery.

## Reproduce

Configure with Clang 18 and `SPIRE_X11=OFF`, then build the eight named targets. Run the same targets with `ctest --output-on-failure`.

For real-resource render checks, reconstruct one logical NSP2 profile from the matching shipped common and locale files using `tools/split_content.py`'s `reconstruct` helper. The card/HUD, room, map, deck-selection and world UI executables accept that logical profile as argument 1 and a precreated capture directory as argument 2. `combat_ui15_regression` and `merchant_preparation_regression` accept the installation root containing `data/nSpire2.dat.tns` and its language file.

All image payloads in this UI pass are unchanged. The implementation adds no runtime allocations or additional resource files; the larger sorter text reuses existing exact-size glyphs.
