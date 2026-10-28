# Actual world resources and ancient option presentation — 2026-10-21

## Recovered complete resource pack

The source14 builder's old journal remains `running`, with no recorded exit.
It is not retroactively marked successful. The complete output and its manifest
were nevertheless present after continuation. A new independent verification
against frozen source14 `a205a6bd566dd2f5d7f60a1d3c7a06f2e1da9430` passed:

| Measured property | Actual value |
| --- | ---: |
| Complete NSP2/14 bytes | 231,314,177 |
| Directory bytes | 1,011,812 |
| Images | 45,724 |
| Animation clips | 1,695 |
| Glyphs | 11,915 |
| Strings | 6,100 |
| Cards | 558 |
| Auxiliary image records | 1,175 |

The actual file SHA-256 is
`342983cbb824f1c858375c7cb72193f7ea68778321cd430d1815f060625b7f42`.
The independent check verified the complete checksum, exact canonical
decode/re-encode bytes, conversion-source dependency hashes, all animation
directory records and dimensions, and five-size coverage of the original and
runtime text corpus. The 0.16 prefix remained byte-identical: 2,638 image
payloads, 497 cards, 1,556 strings, 339 auxiliary entries, 38 clips and 7,215
glyph payloads. This is a complete measured pack, rather than a cache count.

Frozen source15 `7225c880decfb7ff635f960d03ebecd4b0cdd1e2` then passed the
actual-pack memory/range regression: **317,981 checks**, covering every image,
glyph, string and clip plus **40 identical UI frames**. All 320 relevant
source14/source15 production, platform, conversion and actual-test inputs were
independently checked identical. The actual event/ancient suite also passed
**3,655 checks** and captured all eight ancient layouts and merchant flows.
These journals report unchanged source.

The 18-scenario native capture driver stopped at `world-crystal` frame 2: its
hard-coded cell zero was already cleared by the sphere's original round
boundary. The failed journal is retained. The fixture must choose visible
enabled cells through the actual hit list; no complete native/WASM result is
claimed for this run. Original-PC visual parity and physical-calculator
execution are separate, unverified claims.

## Real-asset finding and correction

The actual captures exposed opaque white ancient buttons behind light text.
Passing synthetic interaction tests did not detect this. The pinned
`NEventOptionButton` applies each ancient model's `ButtonColor` to a white
alpha mask; the previous port drew that mask unmodulated. The two 100/120-pixel
source textures had also been stretched as whole images rather than using the
authored `NinePatchRect` margins.

The correction preserves the existing image IDs and directory counts:

- Apply all eight original model RGBA colors, quantized to the canvas's 8-bit
  multipliers. Alpha remains separate from RGB and from parent modulation.
- Preserve neutral canvas tint exactly, including normal/additive blending and
  cached card aggregate initialization.
- Render the focused enabled option's original `78FFFEC0` blue additive
  outline. Its independent position, pivot, scale and size normalize to a
  301 × 52 outline around the adapted 292 × 40 body.
- Preserve the original 34-pixel body and 44-pixel outline source margins.
  Independent horizontal/vertical scale follows the calculator's readable row
  dimensions. Shared textures retain their original white mask; model colors
  remain runtime state.
- Match the locked option's red title and 0.7 label alpha, without adding a
  focus glow or dimming the entire original background mask.

An App-level synthetic white-mask test independently composes a clear
third-row pixel for all eight ancient colors. Asset tests cover unequal-axis
corner preservation and invalid scales. A short UBSan diagnostic passed
2,921,053 RGB565/coverage checks; seven short asset tests and generated-table
validation passed. These are pre-checkpoint diagnostics, not a replacement for
the forthcoming frozen App suites and updated actual-resource screenshots.

The corrected resource converter must run from the next remotely preserved
checkpoint. Enemy and rest cache scopes are unchanged; static UI components
must be baked with the corrected scope. Keep the recovered pack above as the
verified historical artifact, and report the new pack's own hash and results.
