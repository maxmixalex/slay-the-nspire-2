# UI12 frontend feedback corrections

Reference: v0.111.0 / 41cef1ea. Continues UI11 `40def2b`.

## Source stages

- Shared source HSV control rendering now processes bounded 64×48 tiles. The
  previous helper silently skipped controls larger than 64×48 whenever their
  hover material was active; this hid Settings tabs, Credits/Reset buttons,
  card-library filter textures and the Proceed face. Actual source textures
  are retained. The maximum temporary surface is unchanged.
- Settings content is narrowed to x=8..194, with the original scrollbar at
  x=201 and a separate x=213..317 tooltip column. Long row labels wrap at the
  existing 10px font. The language arrow is vertically centered.
- Settings General and Input scrollbars use the retained source track, edge
  and grabber artwork, with click-to-position and continuous pointer dragging.
- The shared original Back component is reduced from 61×42 artwork / 52×30
  hit region to 47×32 artwork / 40×24 hit region at the bottom left.

Ordinary H/J/M Firebird sessions have exercised these controls; their scope
and the resumed final validation are recorded below. Native diagnostics are
correctness evidence and are not calculator performance measurements.

- Credits roles are right-aligned toward the central gutter, as in the source
  scene. Headers use the complete native 12px band to avoid sparse 14px glyph
  omissions. The port credit appears after Trailer and before the FMOD logo.
- Compendium artwork/text gain interior margins; descriptions use 10px and
  remain inside their panel. Focused long text scrolls inside the clipped
  label after a pause. The two lower buttons start beyond the Back artwork.
- Statistics and history content start at x=52 and retain the right scrollbar.
  Statistics no longer displays excluded achievements/Timeline or unknown
  unlocked/N/A entries. Potion discovery uses actual observed progress.
  History reflows its metadata and uses eleven icon columns / narrower cards.
- Bestiary restores the dim background and the original blue region dividers;
  boss red / elite purple / normal cream coloring remains implemented. Its
  mode button is 118×22 with readable 12px type.
- Title logo/flame/glow move up 12px, the menu moves up 24px, and the stacked
  fog opacity is bounded for the LCD crop. The private foreground was refreshed
  from current source atlas decoding: the previous foreground predated the
  rotation correction and inverted the spire. The final Firebird title shows
  the regenerated upright tower and the menu above the sea.

## Focused native boundary result

Actual logical English and Simplified Chinese UI11 packs pass the UI12
frontend interaction regression at `097d1ad9032bd9d175a5450d5ba52c92bb7c5da5`:
large material controls draw, both Settings tabs' scrollbars click/drag,
Credits opens/closes, Back hits do not overlap the changed content, and the
canonical run save stays byte-identical. The first Credits fixture incorrectly
used one 1000ms tick; the runtime deliberately caps each advance at250ms.
The corrected fixture uses four250ms advances. This is a fixture repair,
not evidence that the earlier failed run passed. Firebird review remains the
integrated acceptance gate.

The additional original developer console is documented separately in
`developer-console-ui12-0.17.md`; command probes alone are not its UI acceptance.

## Recovered progress and completed fractional paths

The previous `ui12-front-final` branch was clean but had four test-only commits
not yet reachable from the integration branch: `6154520`, `a215094`, `45c978a`
and `3c99819`. They retain the Settings focus/locale rules and the actual LCD
geometry in the frontend, profiles, statistics, history and input gates. Their
content is now integrated; the old pass logs were preserved separately.

The UI40 font audit found remaining specialized paths that rounded each
glyph despite the central renderer using fractional source metrics:

- Main-menu labels measured their centering and highlight width by summing
  integer advances. They now use the same source kerning and fractional total
  as the painted label.
- Relic/potion category headings now retain 1/64-pixel advances and kerning
  across glyphs using the same actual font family.
- Card-library filters, plain search text/caret and victory counters now use
  those same positions. Scaled filter glyphs also retain their negative left
  bearings. Typed brackets remain literal search text.

These are production changes `7e02d0d` and `8d130b4`, not test-only changes.
They add no font payload or persistent state. Unused width helpers and
measurements that cannot change the fixed 10px layout were left untouched.

## Matched native acceptance after recovery

The existing `library_ui7_regression` still described the old four-column,
46×64 cards and 49px sidebar. `d3f402e` updates its explicit adaptation
coordinates to the two-column, 69×96 LCD layout and 76px sidebar. It retains
its independent alpha compositing oracle, complete category/color comparisons,
filter/search/navigation behavior and canonical-save invariance. Its oracle
decodes glyphs independently and retains their source fractional metrics;
production label/position helpers are not used to paint the expected image.

The matched, closed `front-library-final-native-8a121e816ab5` journal passed
with `source_changed=false`:

| Complete executed gate | Checks |
| --- | ---: |
| Library/collections, synthetic | 3,106 |
| Library/collections, final H English resources | 810,999 |
| Library/collections, final H Simplified Chinese resources | 752,040 |
| Frontend boundaries, final H English resources | 114 |
| Frontend boundaries, final H Simplified Chinese resources | 114 |

Both real-asset tests compile the current `app.cpp` directly. The remaining
library objects come from the matched M core; all non-App production sources
and headers were compared before reuse. The actual App includes the merged
card-face corrections from `83c5c17`.

An earlier seven-suite run also passed statistics, profiles, history
(4,213,415 checks), run-history journal (8,442), input bindings (138), frontend
(36,996) and auxiliary/credits (14,429). That whole driver is explicitly
**failed**, because its eighth executable lacked its execute bit. Correcting
that locally generated binary's mode allowed its 95 console checks to pass
separately. These earlier executables bind to the earlier frontend stage;
they do not replace the complete final integration gates.

## Ordinary frontend evidence recovered from the prior session

H's closed `ui12-h-front-clean-firebird-0db302ff8510` session demonstrates
normal OS/Ndless startup and the actual Flag-key console: typing, completion,
history, underscore input, real additive energy/gold mutations and Potion
Belt/Fire Potion procurement. Console-created state is not an ordinary reward.

J's successful interactions show original tabs, both draggable Settings
scrollbars, the separate tooltip lane, Credits hover, correctly aligned roles,
complete Market Support/Advisors headings, the port credit between Trailer
and FMOD, library textures, history and colored Bestiary rows. Its journal
later failed on a 600-second simulated USB performance-log retrieval timeout
after normal game quit, so it is not called a fully passed session.

M's closed `ui12-m-front-recovered-firebird-91761b0f1770` journal passed,
flushed the independent NAND and quit normally. It verifies the corrected
statistics tooltip lane in both languages, current-language dropdown behavior,
compendium panels and normal quit. The earlier snapshot/USB failures remain
separate failed receipts and are not attributed to game execution.

## Final ARM frontend acceptance

`final-front-cold-firebird-74d73e8882a4` is closed **passed**, with
`source_changed=false`, return code 0, no controller error events and 29 LCD
captures. It binds to runtime `f4a7cd59e7c86477fd820549c394ec44f4d72139` and the
1,573,025-byte launcher whose SHA-256 is
`c8ff786ed76d511111c3917e87b7df2c83c8202a44a63d1c78458c336de5dd91`.

The final launcher was transferred by the emulator's ordinary USB path to an
independent NAND copy. Acceptance started with a cold TI OS boot, ran the
ordinary Ndless installer from Documents and opened the launcher from its
folder. The initial attempt to restore an older snapshot is separately
recorded as failed: that snapshot retained the former read-only NAND path.
No snapshot metadata, OS state or runtime was patched to continue; the cold
boot replaced that attempt.

The closed session exercises the upright English title, compendium panels,
two-column card library and actual keyboard search for `strike`, relic category
headings, both Settings tabs, dragging the original Input scrollbar from its
top to its bottom, the English tooltip in the separate right column, actual
Flag-key console opening and keyboard submission of `help`, language-dropdown
selection of Simplified Chinese, the Credits button while hovered, and opening
and scrolling the Chinese Credits. Its role rows visibly align 工程师 / 美术总监
at the central gutter. It then returns through Settings to the Chinese title,
accepts the ordinary quit dialog, returns to TI Documents, flushes NAND and
closes the controller normally.

Screenshots 19, 20, 24, 26 and 29 respectively retain the tooltip, typed
console, hovered Credits button, role alignment and normal OS return. The
intermediate capture 22 still shows the dropdown with Chinese highlighted;
capture 23 is the completed language change. The session did not retrieve a
performance log, and its correctness captures are not performance evidence.
