# UI8 input, run modes and history — v0.111.0 / 41cef1ea

This work uses the supplied original C# and scene sources. It does not measure
whole-game fidelity by counting implemented registry entries. Audio, multiplayer
and Timeline remain outside the agreed single-player scope.

## Original sources

- `Nodes.CommonUi/NInputManager.cs`: the two default key maps, remappable-command
  lists, conflicting-key swap, the mouse/keyboard Confirm–EndTurn link, and reset.
- `Nodes.Screens.Settings/NInputSettingsPanel.cs` and `NInputSettingsEntry.cs`:
  command order, current-input-bank editing, listening capture and localized
  titles. Keyboard-only has Select and Top Panel; mouse/keyboard has no editable
  keyboard binding for those two mouse/controller operations.
- `Nodes.Combat/NPlayerHand.cs`, `NMouseCardPlay.cs`, `NControllerCardPlay.cs` and
  `NPeekButton.cs`: numeric card selection, cancellation, keyboard-only Select,
  and Peek on release. The separate combat work implements Peek's real overlay.
- `Nodes.Screens.MainMenu/NSingleplayerSubmenu.cs`,
  `Nodes.Screens.CustomRun/NCustomRunScreen.cs` and
  `Nodes.Screens.DailyRun/NDailyRunScreen.cs`, plus their `.tscn` scenes: three single-player entries,
  custom modifiers and seed, and daily generation. The custom/daily scene uses
  original left/right columns; submenu cards inherit the 330 × 705 base button.
- `Models.Modifiers/{Draft,SealedDeck,Insanity,Specialized,AllStar}.cs`: original
  Neow setup sequence. `ModifierModel.Pick2Good1Bad` supplies daily modifier order.
- `Nodes.Screens.RunHistoryScreen` and `Saves.Managers/ProgressSaveManager.cs`: history order and
  exclusion of custom/daily runs from standard victories and ascension progress.
- `Models.Modifiers/CharacterCards.cs`, `Factories/CardFactory.cs`,
  `Runs/CardCreationOptions.cs`, `Entities.Merchant/MerchantCardEntry.cs` and
  `MerchantInventory.cs`: the distinct reward and merchant card-pool semantics.

Paths above use the reference namespace directories (`MegaCrit.Sts2.Core.*`).
Runtime source and converters contain no original PCK, fonts or private packs.

## Implemented behavior

The title's Start opens Standard / Custom / Daily. The removed expedition-settings
page is not reintroduced. Custom runs have seed, ascension, character and the
actual modifier set. Mutually exclusive Draft, Sealed Deck and Insanity replace
one another. Daily uses the source's date seed, character/ascension RNG streams,
modifier ordering and UTC day transition. This host has no original server: its
UI explicitly describes local generation and does not fabricate rankings.

History uses source ordering: run facts, map, relics and grouped tiny cards. Cards
open the actual inspector sequence; relic focus shows the source-style enlarged
hover. Full scroll and the existing enter/shift motion preserve selection, and
custom/daily metadata survives history persistence. Standard wins, streaks and
ascension stay separate from unranked custom/daily history. A complete per-floor
card-decision journal, death quote and all original statistics categories are
still incomplete; their absence is not counted as complete history parity.

All five setup modifiers now perform real, resumable choices/acquisition:

- Draft: ten compulsory three-card offers, one choice each.
- Sealed Deck: thirty distinct cards, exactly ten selected; source rarity/title
  ordering is presentation-only and does not reorder simulation RNG.
- Insanity: thirty generated cards, each created and acquired in sequence.
- Specialized: one selected random model copied five times.
- All Star: five sequential colorless acquisitions with its source flags.

The exclusive setup modifiers clear the starting deck before the first act and
remove Pandora from remaining relic bags after completion. Hoarder/Bing Bong
acquisition is routed through the common cloned-card hooks rather than bypassed.
Every pending selection uses persisted event continuation fields.

## Keyboard settings and hosts

Settings → Input now displays all 25 commands with Mouse + Keyboard and
Keyboard-Only columns. It edits the bank matching the current input mode. Clicking
a disabled mouse-only row cannot create an unreachable binding. A listening
modal captures the next physical key; Escape may itself be assigned. A pointer
Back cancels listening. Editing a used key swaps its previous command into the
old binding, retaining source Confirm–EndTurn linkage in the mouse/keyboard map.
Input Reset restores both maps immediately, matching the source input panel.

The General list includes the original Send Feedback and Credits rows with their
right-hand action buttons, after Text Effects. All eight rows are reachable by
scroll, pointer and keyboard; opening the auxiliary screen retains the settings
selection for return. The title's Patch Notes button uses Confirm on release
(`E` in mouse/keyboard, Enter in keyboard-only), while Select and the desktop
Enter fallback keep their menu-selection role. Holding a shortcut does not
repeat it or act on a replacement screen. Auxiliary page contents and local
feedback export belong to the separate frontend module; its complete runtime
verification remains a checkpoint gate.

Desktop/browser defaults are the original key maps: mouse/keyboard uses D for
deck, A/S/X for piles, M for map, E for confirm/end turn, Space for Peek, and 1–0
for cards. Keyboard-only uses Q/A/D/F for deck/piles, Tab for map, Space for
Select, Enter for Confirm, E for End Turn, S for Peek and W for Top Panel.
Calculator defaults adapt physical 4/6/8/2 to navigation, keep M/Q/A/D/F for
map/deck/piles, use 5 as Select, and expose its alternative ten-card shortcut
sequence in the same editable table. Its matrix adapter handles all supported
letter, number, punctuation, Enter/Escape/Tab/Shift edges; seed text entry also
uses the text adapter instead of gameplay shortcuts.

The raw input layer retains the **semantic action at key-down**. A later bank or
binding change cannot prevent key-up from releasing a map-drawing stroke or
held direction. Repeated key events repeat navigation only. Holding a card key
cannot play another card or accept a newly opened modal. Focus loss cancels
held actions without triggering Peek. Existing pointer and semantic on-screen
button APIs remain available.

- X11 normalizes printable/special keys, recognizes autorepeat release/press
  pairs and clears held keys on focus loss. Its existing atomic profile file
  writer stores the complete record.
- Web passes normalized physical keys, keeps key-down identity through key-up,
  and transfers the bounded preference record. Initial optional reads remain
  inside the unavailable-storage fallback; a profile's record is read before
  committing its selector. The later v17 power-order stage also passes the actual save-header version and the embedded codec capability to run storage; profile metadata transfers retain their existing interface.
- Ndless uses the same binding/dispatch layer with a bounded matrix-edge table.
  It retains launch/exit key-release gates and the existing foreground file
  rollback behavior.

`NSPF` version 3 is 108 bytes: the original 8-byte flags header followed by two
25-entry, little-endian 16-bit key maps. Versions 1 and 2 still decode their
original eight bytes and select host defaults until the first save. Validation
checks supported key codes, duplicates and the linked pair; malformed records
do not partially overwrite preferences. Key rebinding itself does not alter
run `State`; the separate [power-order stage](power-order-ui8-0.17.md) now writes
v17 with historical v3–v16 reading. Web profile deletion and rollback include the new v3 preference
record. Legacy v1/v2 compatibility flags and the v3 map update together through
the existing transaction journal; writes through the old flags API preserve an
existing key map.

## Merchant card-pool correction

Reward modifiers use `Union`, and `CardFactory.CreateForReward` uses `Except`;
reward candidates remain distinct. Merchant generation differs: `Populate`
first excludes existing stock from the **base** pool, then modifier hooks use
`Concat`, and the factory applies type/rarity filters without a later Distinct.
An own-character modifier therefore appends a second copy of its pool, and can
reintroduce a card already in stock. Own-character followed by a foreign modifier
produces Own × 2 + Foreign; subsequent foreign additions stop. Courier refill
uses that same sequence and retains the live Shops RNG stream.

`world_merchant_card_candidates` implements that separate ordered multiset.
`world_card_candidates` remains distinct for rewards/combat. The new regression
compares all seven actual shop cards and a real Courier purchase/refill with an
independent source-order oracle, and compares the resulting Shops RNG words and
call count. It also checks that the own-character reward modifier has no extra
weighting.

## Evidence and current verification boundary

Stage 2's frozen native suite passed the mode, history, old-frontend-flow,
Silent-UI and world-frontend tests. Stage 3's modifier hook test originally
asserted a constructor-only map flag after save/reload. Inspection showed the
correct 12 elites and quota after round-to-even of 5 × 2.5; its replacement
asserts the generated/restored rooms. A temporary diagnostic linked to the
frozen stage-3 core completed all 709 existing hook checks.

For the current input/merchant source stage, short native syntax checks passed
for changed app/input/pointer/X11, run/card-factory and test sources. ARM Ndless
and WASM host syntax checks passed with Clang 18. The standalone binding test,
expanded browser-profile storage test, and player JavaScript syntax check passed.
These are not a full runtime acceptance result.

The checkpoint's full native suite, the new `input_bindings_ui8_regression` and
expanded `run_modifier_hooks_ui8_regression`, full ARM/WASM builds, and actual-pack
input captures remain for the parent's frozen checkpoint verification. The
optional input test capture names are `input-mouse-keyboard`, `input-listening`
and `input-last-shortcuts`. Modifier geometry and font readability still need
actual-pack review. Report the eventual executed results separately.

Settings localization occupies newly appended v28 text indices 8477–8506 in
`run_modes_ui8_assets.py`; the private converter reads the original settings
JSON. Additional frontend modules may append after that range. Resource images
and the total v28 auxiliary count are coordinated by the parent build, not
hardcoded in this input layer.

The stage-5 source adds complete cross-family power order, independent power
icons and Test Subject run/profile progression. See
[power-order-ui8-0.17.md](power-order-ui8-0.17.md) for the source contracts,
compact migration format, measured memory cost and actual verification boundary.

## Stage 8a character information geometry

`character_select_screen.tscn` gives InfoPanel/VBox a fixed 575 × 380 source
rectangle at (163, 414), with a centered 140-pixel Name slot, 20-pixel stats
spacer and 100-pixel relic slot. Its NinePatch is independently fixed at
(89, 411), 700 × 402. The earlier port imposed an 8-pixel body floor and grew
the panel upward. The current implementation preserves the source rectangles,
centers the natural description height within the VBox and uses real 5-pixel
Kreon body / explicit Kreon Bold stats and relic titles. Descriptions keep every
line rather than applying the former five/four-line limits.

Names use the original Spectral family at the available 14/12/10-pixel bands,
source two-pixel character spacing and a bounded alpha dilation for the
projected five-pixel, quarter-opacity outline. The 57 missing Chinese title
glyphs are appended by the parent's font converter, without reassigning existing
glyph descriptors. Neither this change nor its temporary outline mask adds
anything to App, State or the serialized records.

`font_styles_ui7_regression` now checks the source coordinates and, for all five
characters in each actual locale pack, verifies the title fits, every body and
relic-description codepoint is emitted without an ellipsis, the real 5-pixel
bands are selected and the full text stays above Ascension. The existing
five-character UI regression now captures all five settled information panels,
checks each original portrait hit and the Start hit, and confirms pointer
selection leaves the run's serialized State unchanged. App and both test
translation units pass short Clang syntax; the parent's frozen Stage 8a build
and two-language screenshots remain the actual acceptance gate.
