# Crystal Sphere and scene fonts — UI8

Reference: original v0.111.0 / `41cef1ea`, the owner's pinned PCK and DLL.
No original artwork, font bytes, dialogue or decompiled source is stored here.

## Implemented source contracts

The crystal overlay now uses the source's uniformly projected 1680×1260
viewport, centered 1.04-scale background, sphere and inset 11×11 board.
Twenty-four removed corner cells have no interaction targets. Big Divination
precedes Small Divination on the right, the explanation follows them, and the
remaining-use label is at the lower left. The original draw/discard hotkeys
select the two tools. Revealing a cell transfers keyboard focus to the nearest
remaining cell with the source's stable x-major tie order.

The presentation keeps the 500 ms opening fade, 150 ms cell highlights,
500 ms revealed-cell fade, item brightness/scale sequence and independent
cosmetic speech stream. Finishing rewards returns to the crystal terminal
screen with Proceed. Reward overlays retain the crystal behind their panel.
The core's original board generation, rewards, subscriptions and RNG remain
unchanged. App animation records never enter gameplay save data.

The original two procedural noise samples are compiled into polar, noise and
radial color lookup images. Runtime angle interpolation preserves their
independent 0.0065 and 0.0011 cycles/second rates, with no sine/power computation
or dynamic allocation in the pixel loop. These images use the existing bounded
image cache. There is no additional persistent full-screen bitmap. The radial
coordinate/noise quantization, RGB565 color and software antialias approximation
are explicit differences from the original GPU derivatives; this is not a
pixel-equivalence claim.

## Font data and measurement

UI8 keeps the 16-byte glyph descriptor and assigns its previously unused size
bit 5 to a second family. Existing UI7 regular/bold/italic keys stay identical.
Only selected screens request Spectral Bold, explicit Kreon Bold or Bitter
Medium Italic. Chinese selects its original locale font overrides. Missing
scene faces fall back to the corresponding ordinary style. Layout and painting
use the same face key, including rich-text runs and card-grid measurements.

The converter adds real 6 px ordinary/styled glyphs for dense source controls;
the original five larger sizes remain available. The 14 px Spectral title is
packed only in the scene family. The complete appended localization alphabet
is included, addressing missing glyphs in new modes, controls and tooltips.
Glyph pixels are rendered from the owner's original fonts with FreeType; font
kerning and rasterization do not claim identity with Godot.

The Ancient banner has independently keyed room timing, source glyph spacing,
Godot 4.5.1 Spring X-scale samples, Circ motion, epithet delay and red fade.
Its final integration is coupled to the separately implemented Ancient dialogue
layout; static top headings are not a replacement for this sequence.

## Acceptance boundary

This checkpoint adds actual pointer-driven crystal acquisition/finish tests,
save-byte purity checks and resource-capture support. Long validation must run
only after the complete source checkpoint is stored and read back remotely.
The final actual ENG/ZHS captures, portable allocation and emulator results are
recorded in `WORK_STATE.md` after their runs; source syntax alone is not acceptance.
