# UI11 calculator LCD adaptation

This is the platform adaptation of the UI10 source layouts for the 320 × 240
LCD. The source art, cards, typography families and interaction semantics stay
in use. Larger controls are positioned and reflowed individually; there is no
global bitmap scaling pass. This document records implementation and the
limits of the current verification, rather than a numerical fidelity score.

## Reading sizes and layout

- Focused combat cards are rendered at a native 86 × 121 pixels, with 10-pixel
  body text and a matching larger hand-tile cache. This avoids stretching a
  small text bitmap. The fan is raised to leave the card fronts on screen.
  Energy uses a 34-pixel orb and 12-pixel numbers; Regent stars retain their
  separate position and use 10-pixel labels.
- Card inspection uses a 144 × 203 card, 10-pixel minimum body text and a
  104-pixel keyword column. Long descriptions and keyword lists have real
  pages. Related cards occupy the lower part of the keyword column instead
  of overlapping the inspected card. Full keyword reading can use two text
  columns when there is no related card.
- Piles and upgrade/removal selection use three columns of 69 × 96 cards,
  with matching scrolling and pointer hit regions. The upgrade confirmation
  displays both 96 × 135 cards and their before/after values. The card library
  uses an 82-pixel filter panel and two card columns; bottom statistics and
  upgrades controls share a keyboard navigation row.
- Relic and potion collections use seven columns, larger icons, larger group
  headings and larger tooltips. Scrolling is based on the new row dimensions.
- Relic inspection keeps its paper, rarity frame, separate icon modulation
  and original opening/closing motion. The paper now occupies a 194 × 224
  left pane, with a two-line 12-pixel title and 10-pixel description and
  italic flavor text. The complete description and flavor share a clipped
  scrolling viewport, with a scrollbar and pointer page controls. Up/Down
  scroll the body, Left/Right retain inventory navigation, and Peek/V switches
  Up/Down to the separate 104-pixel keyword column. Pointer scrolling uses
  the pane under the pointer. Changing relics resets the reading position.
- The merchant keeps five upper cards and two lower cards. Upper cards are
  65 × 91; the focused card is 80 × 113, draws last and has at least 10-pixel
  body text. Lower focused cards move up 12 pixels so their price stays above
  the screen edge. Prices use at least 10 pixels, and the merchant speech
  bubble wraps inside a larger box. The small Back control leaves the lower
  cards unobstructed.
- Normal events use a 10-pixel body and 10-pixel option text. Three 40-pixel
  options fit per page; long event bodies have the existing full text reader
  and truncated option descriptions are available in the paginated tooltip.
  Ancient dialogue uses 10-pixel text with 11-pixel line spacing, a 232-pixel
  speech bubble and three 35-pixel options. The persistent Ancient banner
  moves to the upper-left after its entrance animation.
- Reward rows are 24 pixels high with 16-pixel item icons. The banner uses
  12-pixel text, card offers are larger, and the reward list scroll region and
  pointer drag geometry agree. Potion actions use 68 × 24 controls with
  10-pixel labels.
- Settings use 12-pixel tabs and 10-pixel rows. General settings show seven
  rows and scroll to reveal the selected row; input settings show four
  binding rows. Binding-column headers wrap into two lines. The Reset label
  uses the existing short localized text to fit its control. Language choices
  are owned by the locale implementation and use the same enlarged layout.
- The Bestiary uses 10-pixel text and two-line names in the right list. Its
  left action list displays five 26-pixel rows per page; keyboard selection
  reveals the current page and pointer page controls keep every action
  accessible. The 12-pixel title is confined to the left 196-pixel pane.
- Tutorial popups use 12-pixel titles and 10-pixel bodies and controls. Most
  use a 260-pixel-wide panel; merchant and rest panels retain the required
  live room targets. The treasure relic lesson uses a 136 × 224 left pane,
  leaving the actual center relic and its original flipped arrow visible.
  The combat tutorial preserves its original three
  illustrated pages. Longer prose has a real scrolling viewport rather than
  shrinking or omitting words. Reading controls obey the same entry delay as
  the original popup; the one-second Ascension lock applies to pointer and
  keyboard scrolling as well as confirmation.
- Crystal dialogue, tool labels, remaining charges and instructions use
  10-pixel text. Tools occupy 110 × 30 controls and the instruction panel is
  110 × 95; the sphere and 11 × 11 grid retain their source positions. Clicking
  instructions or using Detail opens the existing complete 12-pixel text
  reader; returning preserves the cursor and does not spend a tool charge.
- Game Over facts use 10-pixel full text in a scrolling body, 22-pixel badge
  rows and 28-pixel score rows, with a fixed 152 × 26 footer. Up/Down scroll
  the body, Left/Right reveal the selected item, and clipped items do not
  retain active off-screen pointer regions. Pause menu items wrap into two
  10-pixel lines; treasure headings use 10-pixel text.
- Common Proceed, Back, Confirm and End Turn controls use larger hit regions
  and text. Most compact secondary labels now have an 8-pixel floor. Original
  title/bold/italic faces are selected through locale-aware content metadata.
  Font rasterization, Latin spacing and outline weight are documented in the
  separate localization report.

## Verification boundaries

Native rendering was used only as a layout aid. The frozen B, D and E cores
built with Clang 18. The GCC 13 build hit the pre-existing compiler ICE at
`world_relics.cpp:323`; it is not recorded as a game failure. No WASM or browser
test is part of this work.

Private canonical NSV1 fixtures were constructed against E
`106453bce5921764ee4ef8cb7c2d40a5d764d65e` for merchant, camp, upgrade,
Neow, Abyssal Baths, reward, five characters and a multi-enemy combat. Every
fixture passes State validation and exact save/load/save byte equality. These
fixtures are standard save files loaded by the production ARM launcher; they
are not evidence of reaching those rooms through an ordinary playthrough.
The private manifest contains the exact file sizes and SHA-256 hashes.

### Actual ordinary-OS Firebird execution

D failed before game startup: after ordinary Ndless installation, the loader
reported `[Zehn] Memory allocation failed!`. The installed launcher was read
back and matched the build hash. This is an observed OS allocation failure,
not a rendering or save failure. Increasing the hard ceiling alone did not
make that allocation available under the tested OS.

F `cbdd5d412e9ca19b33407b8614d9c8659190f73d` starts normally after the
runtime owner reduced the caches. The independent LCD validation NAND has a
snapshot after ordinary Ndless installation, source-guarded session journal,
serial log, LCD captures and saved-state readbacks. It uses the new English and
Simplified Chinese font resources with unchanged shared art. It does not
modify the user's original NAND/profile.

Executed F flows so far:

- Standard OS Open → title → Continue → merchant entrance → inventory → full
  Pillage inspection → return → purchase. The UI shows gold 1299 → 1226 and
  deck 10 → 11. Save & Quit → normal application Quit → OS → USB readback
  yields a valid canonical state with those values and revision 5 → 6.
  Upper/lower card focus and relic tooltips were inspected on the LCD.
- Continue from the upgrade fixture → keyboard move to the second row with
  scrolling → side-by-side Strike/Strike+ confirmation → confirm → return
  to the used camp with Proceed. A saved-state readback is retained.

The upgrade readback is a valid canonical state with one upgraded card,
revision 6 → 7, unchanged deck size 10 and unchanged gold 99. The upgrade
selection phase returns to the used camp phase.

The later LCD H attempt selected the OS `ti_hub` library by mistake. Its
`fatal library error, lookup self` happened before the game launcher executed
and is not an H game failure. A second attempt stopped in ordinary OS
navigation before game execution. The resource installation itself completed
and flushed normally, leaving an independent NAND containing H and the final
shared/English/Chinese packs. Chinese Ancient/event/library execution remains
pending final K testing; those probes do not count as completed game flows.

The I native tutorial check reached the Ascension entry-delay assertion and
found that the newly added reader exposed pointer hits too early. K moves
reader input and hit registration behind the same one-second lock. Both final
English and Simplified Chinese pack checks pass that lock and all earlier
tutorial scenarios. They then detect an actual treasure lesson layout defect:
the enlarged centered popup obscured the highlighted relic and the relocated
arrow pointed away from it. The follow-up fix moves only this lesson to the
left reading pane and restores the authored arrow at the treasure holder.
The original arrow pixel check is retained and passes in the follow-up run.

K `90d981c4e42e5f67f7a72b52f37fa1ac45410a71` narrow actual-pack results:

| Gate | Result | Journal token |
| --- | --- | --- |
| Relic inspector, final English | Passed, 1,127,280 checks | `ui11k-relic-actual-eng-9047b8f58739` |
| Relic inspector, final Simplified Chinese | Passed, 1,076,919 checks | `ui11k-relic-actual-zhs-5b85f2ded7d2` |
| Crystal reader, final English | Passed, 310 checks | `ui11-crystal-reader-final-eng-k-ffa135e8eaa8` |
| Tutorial, final English | Failed at treasure arrow pixels after entry-lock scenarios passed | `ui11k-ftue-actual-eng-dd6d79c117e3` |
| Tutorial, final Simplified Chinese | Same treasure arrow failure | `ui11k-ftue-actual-zhs-66c1413ddf67` |

L `7722cad8adb6626081b0100e5017d3de095904c5` closes that tutorial defect:

| Gate | Result | Journal token |
| --- | --- | --- |
| Tutorial, final English | Passed, 7,162 checks | `ui11l-ftue-actual-eng-578ad573b943` |
| Tutorial, final Simplified Chinese | Passed, 5,862 checks | `ui11l-ftue-actual-zhs-01170b873dcf` |

Both L journals record unchanged frozen source. The checks include the exact
Ascension entry delay, held-key/release behavior, pre-shuffle presentation,
treasure pickup blocking, authored flipped-arrow pixels and canonical-state
invariance. Auxiliary native images confirm the treasure lesson's 10-pixel
body and visible center relic; the screenshot is not a runtime measurement.

Relic checks cover full-size glyphs in every relic description/flavor, material
composition, source navigation and ownership, scrolling to the end and back,
keyword reading without changing the body position, and canonical-state
invariance. Crystal checks cover both keyboard and pointer reader routing,
cursor/charge preservation, and existing board/tool/acquisition behavior.
These are narrow native logic/layout checks; they do not establish calculator
runtime performance or replace the ordinary launcher emulator flows.

Emulator execution establishes behavior on the tested Firebird/OS
configuration; this document makes no physical-device performance or
universal memory-allocation claim. Root integration separately records the
ordinary-save H battle and final K runtime measurements.
