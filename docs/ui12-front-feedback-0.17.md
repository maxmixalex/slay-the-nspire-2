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

This source stage has not yet had integrated Firebird acceptance. Source
changes and native diagnostics are not calculator performance measurements.

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
  fog opacity is bounded for the LCD crop. The original foreground must be
  refreshed from current source atlas decoding: the shipped old foreground
  predates the rotation correction and inverted the spire. Correct regenerated
  320×240 private images are supplied to the release owner for pack replacement.

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
