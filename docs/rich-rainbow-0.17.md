# Original parameterized rainbow text

Reference: **v0.111.0 / 41cef1ea**, whose PCK header identifies **Godot 4.5.1**.
This stage fixes the reachable `events.COLOSSAL_FLOWER.pages.INITIAL.description`
opener `[rainbow freq=0.3 sat=0.8 val=1]`. The previous reader accepted only the
exact name `rainbow` and limited the complete tag to 16 bytes, so its opener
appeared in the event description. The matching closer disappeared.

The actual-pack audit examined all 6,100 strings and 10,732 bracket tokens.
The same parameterized form occurs in one Crystal Sphere banter entry. Other
unrecognized tags remain outside this stage: 21 strings containing `font_size`,
one containing `pink`, one multiplayer `center`, and two containing `shake`.
Those strings are not used by the currently integrated UI paths. No blanket
tag removal is used to hide unsupported syntax.

## Original implementation and provenance

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
label through process notifications. This implementation uses the existing
`App::map_clock_ms` presentation clock. It is not accelerated by Fast mode, but
it does not recreate a label-local time origin when a label is created, hidden
or reopened. Its phase can consequently differ from the original scene, and
it inherits the host's existing per-advance time cap. This stage preserves the
parameterized color function and movement, **not original-frame phase
equality**. No canonical State field, save version or gameplay RNG changes.

## Regression and validation boundaries

`localized_template_regression` covers the actual nested sine/rainbow syntax,
all four defaults, reordered and nested parameters, signed phase calculation,
32-bit clock/position endpoints, literal unknown attributes, unmatched
closers, scan/depth limits and source-derived OkHSV color oracles including the
blue cusp. Black, white and neutral saturation endpoints are included.

`world_text_regression` starts the real Colossal Flower model and obtains its
initial description through `App::original_event_text`. The synthetic fixture
retains all original tags and nesting with neutral replacement prose. With
the optional actual pack, the test additionally renders the real word at two
elapsed times, verifies that only its description region changes, and checks
an exact full-frame return after 10 seconds / three rainbow cycles while
holding the independent sine/jitter frame fixed. Reading, rendering and
save/load continuation must leave canonical saved bytes unchanged.

Pre-checkpoint validation consists of short syntax checks, the standalone
ASan/UBSan parser/color regression, and the independent official-function
diagnostic described above. The new complete App-linked synthetic and
actual-pack tests require source18 preservation, a frozen build and their own
journal receipts. No original-PC screenshot comparison or 95% visual
acceptance is claimed here.
