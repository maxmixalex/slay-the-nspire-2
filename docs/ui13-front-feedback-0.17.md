# UI13 frontend feedback implementation

Reference remains the owner's v0.111.0 / 41cef1ea. These are the readable
320 × 240 adaptations over UI12; original textures, font families and flow
remain in use.

## Layout and input

- The complete title logo, its flame and glow move up another 12 pixels.
  The menu pitch decreases from 20 to 17 pixels; the five-row saved-run menu
  starts at y121 while the logo ends at y108, leaving a visible gap.
- Confirmation/cancellation hit regions decrease from 98 × 30 to 80 × 24.
  Their original faces/outlines decrease from 108 × 40 to 88 × 32, retaining
  the existing hover/press material and readable 10-pixel labels.
- The history Previous/Next arrows occupy the top corners above the scrollbar.
  Previously the right arrow at y111 overlapped the scrollbar, whose later hit
  registration intercepted its click. Other history content keeps its width.
- Expanded map legend paper grows to 129 × 170. Each icon and label is centered
  as one group, and the six rows retain 12-pixel side margins and at least
  16 pixels below the last row. Labels retain the current language and font.

## Original language dropdown

`settings_dropdown.tscn` and `language_dropdown_item.tscn` explicitly use
ColorRect backgrounds and highlights with RGB values (18,33,41) and
(44,88,112). They do not reference a background texture. The port retains
those source colors and restores centered language names, opens the list below
the current option, and uses the three actual `small_scrollbar_*` textures
from that scene instead of synthetic arrow buttons. The track uses the source
half-transparent black modulation; the nine-patch train uses quarter-white
normally and gold on hover/press, as in `NDropdownScrollbar`.

The dropdown has its own nonpersistent scroll offset. Pointer dragging and
wheel scrolling change that offset without changing the language selection or
the Settings page behind it. Its modal layer also suppresses the covered
Settings controls' hover animation while retaining the list's own hover.
Direction-key navigation keeps the selected
language visible, and selecting the active language retains the original
open-list behavior. Older UI40 resource packs retain a functioning scrollbar
using their existing large source assets until the matched new pack is loaded.

## Localization

The original history format `{ModelTitle:: {}|}` uses SmartFormat's empty
formatter name. The shared bounded formatter now applies the same conditional
semantics as explicit `cond`, preserving the nested current-value selector and
evaluating only the selected branch. Unknown variables remain visible so real
missing bindings are not hidden.

The port-credit title and author line are translated for all sixteen supported
languages in `config/port-credits.json`. The private conversion writes reserved
string slot 8703 and supplements only missing glyphs in the actual source font
chain. New Thai graphemes receive unoccupied private glyph IDs, preserving all
existing composed text. Old packs retain the previous English/Chinese fallback.
The block stays between Trailer and FMOD.

`tools/frontend_ui13_assets.py` exposes `source_assets(pck, cache)` for the
three fixed image IDs 2201..2203 and `localize_credits(source, language,
font_dir)` for the text/glyph rewrite. Root integration owns the resource
directory, publication and complete validation.

## Source checks and completed actual-resource validation

- Clang 18 syntax checking of App, pointer and UI input passes. The existing
  partial-switch warning in UI input remains.
- The complete standalone localized-template regression passes 1,688 checks,
  including populated/empty string branches, nested current values, numerical
  conditions and unknown-value preservation for the empty formatter.
- The locale binding inventory passes: 935 UI bindings and 142 additional
  original keys, without introducing unreviewed literal translations.
- Every port-credit grapheme in all sixteen languages successfully rasterizes
  at 8, 10 and 12 pixels through the restored original font chain, including
  Thai combining marks and the Cyrillic, Korean, Japanese and Chinese text.
- The frontend integration regression now exercises the installed sixteen-
  language dropdown's pointer drag, wheel, keyboard focus, modal ownership and
  unchanged canonical save. It also clicks the history Next arrow with an
  actual scrollable history. Matched `d8a1025` core and the complete UI13 packs
  passed all 138 checks in each of English and Simplified Chinese.

The profile/main-menu/Continue/compendium regression also passed against the
matched UI13 English and Chinese packs. Its independent scalar font oracle
reads the actual fractional advances, kerning and negative bearings, and still
compares complete rendered frames. The compact popup geometry additionally
passes 100,713 independent button-presentation checks. The saved-run main menu
and compact profile-delete popup were reviewed in actual 320×240 native frames.

The final resource conversion at `6e40566` produced all sixteen matched editions,
preserving previous glyphs and text except the reserved credit slot. The final
`b96bd0d` native locale gate initialized every language, checked 53,072 model
descriptions and their visible 8/10/12px glyphs, and verified all three energy
sprite sizes: 38,686 checks, no failures and no unexpanded model descriptions.
This is an actual application/resource check in addition to converter readback.

The final ARM runtime is `d5f9a3c`; these native checks do not claim Firebird
timing or physical-calculator frame rates. The aggregate status and actual
76.30 MB default installation size are recorded in
[`ui13-lcd-followup-0.17.md`](ui13-lcd-followup-0.17.md).
