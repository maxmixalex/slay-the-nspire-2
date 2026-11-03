# Porting notes

The reference game is **v0.111.0 / 41cef1ea**, recorded in
[`config/reference-release.json`](../config/reference-release.json). Updating to
another original release requires reviewing its rules, catalogs and conversion
inputs together. Original resources remain private.

## Gameplay and presentation

Keep card effects, pools, encounter rules, enemy behavior and random draws
separate from screen adaptation. Readable wrapping, scrolling, larger hit
regions and detail views are appropriate for the 320×240 display. Hover,
paging, inspection and visual clocks must not consume gameplay RNG or mutate
canonical save state. Choices retain actual instance IDs; modal changes and
scrolling cancel captured pointer actions.

The native world uses named RNG streams and saved generated maps. Matching a
seed alone does not specify an entire run: character, act, difficulty,
modifiers and the player's choices also matter. Existing source regressions
and fixtures preserve save compatibility and deterministic transitions.

Audio, multiplayer, hosted leaderboards, cloud saves, a desktop timezone/DST
database and calculator IME are not implemented. These are remaining platform
or feature gaps, not replacements for their original behavior.

## Calculator input

The Ndless [touchpad reader](https://github.com/ndless-nspire/Ndless/blob/e52415fb568e92cd36ed049067b3e9c2727b84af/ndless-sdk/libndls/touchpad.c)
reads page 4 registers 0x00–0x0A inclusive: byte 0 is contact, bytes 2–5 are
big-endian coordinates and byte 10 is the mechanical switch. CX center/arrow
keys use touchpad classification in [keys.h](https://github.com/ndless-nspire/Ndless/blob/e52415fb568e92cd36ed049067b3e9c2727b84af/ndless-sdk/include/keys.h).
They are not an independent second matrix input.

A side press keeps its direction until release. A center press takes its owner
when queued input is replayed: a visible pointer means click, otherwise it
confirms keyboard focus. Mechanical down/up reanchors the finger without moving
the cursor. Keyboard input owns existing contact until lift; a fresh contact
needs six normal-speed pixels of net movement to restore the pointer. That
threshold is independent of pointer speed. Held movement still supports dragging.
The foreground collector queues reports and never dispatches App callbacks.

Tap to Click defaults off. Mouse Speed offers 50/75/100/125/150%, with 100%
retaining the original gain. Both live in the existing 108-byte NSPF v7 profile
record; v1–6 migrate to off/100%. These are game preferences: the inspected
Ndless [syscall declarations](https://github.com/ndless-nspire/Ndless/blob/e52415fb568e92cd36ed049067b3e9c2727b84af/ndless-sdk/include/syscall-decls.h)
and [table](https://github.com/ndless-nspire/Ndless/blob/e52415fb568e92cd36ed049067b3e9c2727b84af/ndless-sdk/include/syscall-list.h)
provide no supported OS tap-preference getter. The referenced
[nZATO backend](https://github.com/zyf722/nZATO/blob/86a4785560e18d8230e4e7f9f9e18d9be7adc96e/cpp/platform/ndless/main.cpp)
also uses physical pressed/arrow state. The brief click outline uses only the
presentation clock, with no new image pool or gameplay RNG.

## Rendering

The calculator uses RGB565 software drawing and offline sampled animation.
The full profile retains the established actor samples, dimensions and action
clocks. Actor color reduction is bounded in RGB565 with at most four alpha
units of error. Character selection has a separate dark-color policy;
Ironclad, Defect and Regent selection keeps exact alpha and at most one green
RGB565 code of change. Static interface/card art and typed parameter tables
remain exact.

Neow/Tezcatara scene loops retain their samples at the original 33 ms cadence.
Their temporal color policy allows one red/blue and two green RGB565 codes
while holding a color. Full checkpoints occur every 32 retained samples.
Reduced animation keeps exact representative actor/body poses, original clip
clocks and final poses; scene shader clocks are held at zero.

The shared reduced-resolution Tezcatara noise and calculator material
interpolation are approximations. Neow's held light field is baked as NWF3,
exact to the port's held-clock field. Integer rasterization, bounded particles,
local cosmetic noise and sampled materials do not reproduce the continuous
Godot renderer. Review effects across their lifecycle, not just one still.

For decoder and cache ownership, see [resource formats](resource-format.md).
For current target budgets and validation commands, see
[DEVELOPMENT.md](../DEVELOPMENT.md).

## Seeds and map arithmetic

The ordinary seed field holds 64 UTF-8 bytes. Seed canonicalization follows the
original invariant uppercase, O/I replacement and whitespace rules; it does
not apply NFC/NFKC normalization. The legacy raw/debug path stays separate.
The converter's ASCII encoder streams longer text and returns a 64-bit root
identity for the explicit import mode. Ordinary text never auto-detects a code.

Unicode tables come from Unicode 15.1.0 and the recorded .NET invariant behavior.
Their generators are `tools/generate_seed_case_tables.py` and
`tools/generate_seed_legacy_prefix.py`; the license is preserved in
`third_party/unicode/LICENSE`.

The map quota sampler uses two fresh random draws per Gaussian attempt,
nearest-even rounding and rejection outside the inclusive bounds. It uses the
sine branch, without caching a spare normal sample. `src/reference_math.cpp`
implements binary64 arithmetic with fixed-width integers for ARM without an
FPU. Its logarithm and sine are approximations; near-half-integer cases return
`AmbiguousRounding` rather than choosing a side or consuming replacement draws.
The Go, Sun and Cephes notices remain in [NOTICE.md](../NOTICE.md).

## Calendar localization

Run history applies the original localized date/time patterns with the
selected language's culture. Daily-run dates use their localized pattern with
an invariant provider, following the source behavior. For example, Thai
history uses the Buddhist year while a daily date keeps a Gregorian year and
invariant month name. This formatting is separate from RTC/timezone behavior.

`include/ui_calendar_locale_data.inc` is generated by
`tools/generate_calendar_locale_data.py` from `calendar_reference_oracle`
culture metadata. The source provider is .NET 9.0.22 / ICU 74.2. The bounded
Unicode-derived facts include month/genitive forms, day periods, separators
and Thai's +543 year offset; no ICU library is linked into the runtime.
The Unicode License V3 in `third_party/unicode/LICENSE` applies.

The custom formatter covers the patterns used by the game plus quoted/escaped
literals. An undersized destination is empty instead of partial UTF-8. Dynamic
calendar glyphs are generated at their actual font roles. Eleven fixed Thai
month graphemes use display-only IDs U+10F000–U+10F00A, separate from the
pack-local shaping and kerning ranges. Logical text stays Unicode. Older packs
missing those glyphs use readable English fallback dates.

## Rainbow text provenance

The pinned engine implementation is the authority for the numerical behavior:

- [Godot 4.5.1 `rich_text_label.cpp`](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/gui/rich_text_label.cpp),
  blob `41d1cded4dcd685a94fea6afab94c978138509a7`:
  `ITEM_RAINBOW` in `_draw_line`, `push_rainbow`, the BBCode option parser, and
  `_update_fx` / `NOTIFICATION_INTERNAL_PROCESS`.
- [Godot 4.5.1 `color.cpp`](https://github.com/godotengine/godot/blob/4.5.1-stable/core/math/color.cpp),
  blob `e61c26de687eb19f13bd07e0ff4fc8f0d526c057`:
  `set_ok_hsv` calls the OkHSV conversion and clamps the resulting RGBA.
- [Godot 4.5.1 `ok_color.h`](https://github.com/godotengine/godot/blob/4.5.1-stable/thirdparty/misc/ok_color.h),
  blob `4d0f0049bdf9cd81630175e751c3c8e8275d3b5a`:
  Björn Ottosson's `okhsv_to_srgb`, `toe_inv`, and sRGB transfer functions.
- [Godot 4.5 BBCode reference](https://docs.godotengine.org/en/4.5/tutorials/ui/bbcode_in_richtextlabel.html#rainbow)
  documents the four named options. The exact pinned source formula below
  determines how frequency and speed combine.

The source computes the hue before OkHSV conversion as:

```text
hue = max(freq, 0) * abs(elapsed_seconds * speed + local_glyph_x / 50)
```

Defaults are `freq=1`, `speed=1`, `sat=0.8`, and `val=0.8`. Frequency multiplies
both the position term and the time term in this source. A negative frequency
therefore selects hue zero; it does not wrap or reverse the palette. Negative
speed is handled inside the absolute value, before multiplying by frequency.
The result replaces the text's RGB while preserving its alpha; later canvas
modulation still applies. Godot uses **OkHSV**, not ordinary HSV or a fixed
seven-color sequence.

The generated numerical table and integer adaptation in
`include/rich_rainbow.hpp` retain the applicable MIT copyright and permission
notice. The data comes from public color mathematics, not the game's images,
fonts or localization payload. No original prose is included in these tests.

## Bounded parser

`RichReader` accepts optional `freq`, `sat`, `val`, and `speed` in any order.
Their decimal representation is exact in thousandths, with at most three
fractional digits. Frequency and speed cover `-10` through `10`; saturation and
value cover `0` through `1`. The pinned parameterized form and the bare tag's
defaults are represented exactly. Leading signs, omitted integer zero and
spaces/tabs between properties are supported.

Unknown or duplicate attributes, malformed numbers, exponent/quoted syntax,
excess precision and out-of-range values preserve the complete opener as
visible text. Its closer remains visible too, and does not accidentally close
an enclosing supported rainbow. An unmatched rainbow closer also stays
visible. Parsing has a 128-byte tag scan and a 16-entry style stack; additional
nested rainbow scopes remain literal. Nested colors, sine effects and supported
rainbows restore their previous styles and parameters on close.

## Integer color calculation

The runtime has no floating-point or libm dependency and allocates no heap
storage. Its 136 hue nodes and 255 gamma thresholds occupy **7,582 bytes**.
Each hue node contains linear RGB at value 1 plus the original `L_v`, at
saturation `0, .2, .4, .6, .8, 1`. Stored values use unsigned 16-bit precision.
Hue and saturation interpolate the neighboring nodes. Value uses the original
toe-inverse relationship and homogeneous cubic scaling of linear RGB, rather
than a coarse set of brightness palettes. Gamma thresholds then select the
nearest RGB888 value before the existing RGB565 conversion.

To reproduce the table from the linked public header:

1. Sample `okhsv_to_srgb({hue / 65536, saturation / 5, 1})` for saturation
   indexes 0 through 5. Clamp RGB and convert it back to linear sRGB with the
   header's inverse transfer function; round each component times 65535.
   Compute `L_v` from the same `find_cusp` / `to_ST` equations and round it
   times 65535.
2. Start hue nodes at every 512 units, including the wrap endpoint 65536.
   Refine any interval whose integer interpolation differs by more than one
   RGB888 unit from the original function at saturation .8 and value .8 or 1.
   Check every integer hue inside the interval and insert the first location
   of its largest component error. Repeat until no interval fails. The seven
   added nodes are 5343, 25928, 48013, 48068, 48069, 48070 and 59647.
3. Generate gamma thresholds by rounding upwards the inverse sRGB transfer
   of `(i + .5) / 255`, times 65535, for indexes 0 through 254. This permits
   bounded integer binary search without evaluating a power function.
4. Use the explicit black limit for value zero. The upstream intermediate
   expression otherwise has a zero divided by zero at that endpoint.

An independent diagnostic includes the unmodified public Godot header and the
production native helper. It compares all **65,536 hue samples**, at saturation
.8 and both original values .8 / 1: **393,216 channel comparisons passed** with
maximum RGB888 error **1 / 1 / 1**. This bound applies at the same discrete hue
samples; the hue itself is quantized down by less than 1/65536 of a turn. The
original OkHSV approximation has a sharp blue cusp, so this is not a claim of a
uniform continuous-color error bound before hue quantization. The refined
nodes retain that cusp, including the original jump between samples 48069 and
48070. Native RGB565 quantization follows after this comparison.

The observed parameter forms have exact saturation nodes. Other supported
saturations interpolate .2-wide intervals. A separate deterministic 200,000
sample diagnostic, including the full observed hue curves, found maximum
RGB888 errors of 17 / 3 / 5 and mean absolute channel error 0.14520 over that
mixed corpus. These are sampled diagnostics for the wider accepted range,
not proven bounds or a claim that every possible Godot parameter is ported.

## Calculator layout and clock adaptations

Godot's horizontal coordinate belongs to its RichTextLabel, including content
margin, indentation, alignment and shaped glyph advances. The port uses
`at - x` within the current native paragraph, resetting it on wrapped lines.
The original event body's font size is 26 in
`scenes/events/default_event_layout.tscn`; native glyph advances are normalized
by `26 / native_font_size`. Screen position and UTF-8 character index are not
used as the color phase. Native font metrics, readable wrapping and missing
Godot shaping remain calculator layout adaptations.

Godot maintains elapsed time separately for each `ItemFX`, updating a visible
label through process notifications. The renderer gives rendered labels independent
origins through `RichLabelClocks`, a 32-entry table bounded below 768 bytes. An
outer render-frame guard observes which labels remain present; a label omitted
from the rendered tree loses its origin, so a newly created or reopened label
starts at zero. Repeated draws, shadow passes and nested renderer calls do not
restart it. Different owners of identical text remain independent, and a new
text or content generation creates a new effect instance.

The generic text, paragraph, fitted, source-hover, compact-room, scrolling-room
and Ancient-dialogue painters all use that label time. The actual Colossal
Flower body keeps a stable owner while its paragraph scrolls; Crystal Sphere
banter keeps a stable owner through its bubble's entrance position/scale and
uses a new identity for each dialogue instance. Presentation reset clears the
table. No new bitmap, heap storage, canonical State field, save version or
gameplay random draw is added.

The elapsed delta still comes from `App::map_clock_ms`, which is not multiplied
by Fast mode and retains the host's existing per-advance cap. The native
renderer has frame-sampled creation/visibility and readable LCD glyph layout;
this implementation does not claim identical desktop GPU frames or shaping.
