# UI7 original font faces and bounded glyph storage

Reference: **v0.111.0 / 41cef1ea**. This records an implemented font path and
executed private conversion, not final bilingual screen acceptance or Godot
pixel equivalence. Original fonts and generated resources remain private.

## Source contracts

`MegaCrit.Sts2.Core.Localization.Fonts.FontPathSets/ZhsFontPathSet.cs`
assigns **Noto Sans Mono CJK SC Regular** to ordinary Chinese text,
**Source Han Serif SC Bold** to bold text and **Source Han Serif SC Medium**
to the locale's italic override. The Medium face is not the default Chinese
face. `MegaRichTextLabel.RefreshFont` applies these three explicit overrides.

The ordinary English card/rest scenes use **Kreon Regular**. Hover-tip scenes
explicitly override bold with **Kreon Bold**; Bestiary dialogue uses
**Bitter Medium Italic** and some headings use **Spectral Bold**. These are
scene-specific families, not a global English regular/bold/italic triplet.
Godot 4.5's default theme supplies its own Open Sans SemiBold variation when
a rich-text control does not override a face. It would be incorrect to apply
the Bestiary dialogue family to every English italic tag.

The current complete suffix conversion therefore supplies Chinese's three
actual locale faces and English Kreon Regular. English codepoints not present
in Kreon use the original Noto Mono face. Unprovided English styles fall back
to the same Regular codepoint. Exact per-scene English family inheritance,
kerning, glyph bearings and Godot rasterization are remaining differences.

The rest option's highlighted “升级” and “牌组” use the original gold tag.
The room renderer separately corrects its shadow tint; those words must not
be relabeled as a bold-font bug to explain that shadow repair.

## Extraction and runtime changes

`tools/extract_original_fonts.py` reads the original Godot `FontFile.data`
property through the documented RSRC/RSCC resource structure. It supports
the pinned Zstandard block container and follows the actual property table;
it does not search compressed or cached data for a plausible font signature.
Only a four-block decompression cache is retained. Five original faces were
extracted, and the two scene-specific English display faces were extracted
separately for later family-aware use.

UI7 reuses the 16-byte glyph descriptor. Its existing size field now records
the pixel size in the low six bits and a face selector in the next two bits.
Old resource versions always select Regular. A missing styled codepoint
falls back to the same Regular codepoint, then to the ordinary question mark.
Both Canvas measurement and painting use the selected descriptor; the former
one-pixel displaced duplicate and extra bold advance have been removed.
Real outlines and shadows are still separate rendering operations.

`tools/replace_original_glyphs.py` replaces the complete locale glyph suffix,
retains every preexisting Regular codepoint and original string index,
and adds actual styled glyphs only for characters referenced by styled spans
plus the variable-value ASCII alphabet. It relocates directory offsets while
preserving the common artwork byte for byte. Exact duplicate glyph payloads
share storage. Frequently used characters are stored with their faces/sizes
near each other, improving page locality without a new runtime cache.

The runtime keeps its existing bounded glyph cache and resident directory.
The converter does not retain obsolete font suffixes or introduce an eager
runtime font scan. The actual rasterizer is Pillow/FreeType at the port's
8/10/12/16/20-pixel sizes; this is original-font selection, not a claim that
FreeType's output equals Godot's final transformed text pixels.

## Executed conversion evidence

Tool checkpoint **`a1f2c9b127fd3bb5e3fce36c7b9c86560176b79f`** passed all five
public synthetic extraction/replacement tests. Journal:
`ui7-font-tools-d1982106bd9c`.

The same immutable source converted both actual UI7 v24 resource files:

| Property | English | Chinese |
| --- | ---: | ---: |
| Original Regular codepoints retained | 116 | 2,389 |
| Bold codepoints generated | 0 | 387 |
| Italic codepoints generated | 0 | 221 |
| Total glyph records | 580 | 14,985 |
| Resident directory bytes | 961,510 | 1,191,990 |
| Runtime directory limit | 2,097,152 | 2,097,152 |
| Directory growth from input | 0 | 48,640 |
| Complete pack byte change | +107 | +630,300 |
| Old Regular glyphs kept as unsupported fallback | 0 | 0 |

The shared artwork is **91,176,719 bytes** and remains byte-identical.
Every preexisting locale string and model index is unchanged. English has
ten codepoints absent from Kreon, resolved by the original Noto Mono face;
Chinese requires no fallback font. Conversion journals are
`ui7-original-glyphs-eng-e28b1261eda8` and
`ui7-original-glyphs-zhs-3f01db1594e6`, both passed with unchanged source.

These are intermediate resource files. The character-screen viewport is
being resampled from the corrected original 1680×1260 display contract, and
the later phobia/profile append will be followed by the same final font pass.
No intermediate resource hash is labeled as the final delivery hash.

## Regression boundary

The native `font_styles_ui7_regression` verifies distinct real face advances,
single-pass glyph painting, centered text, paragraph wrapping, fitted layout,
old-resource compatibility and provider-cache reuse. Its checkpoint-4 run
reached the final I/O assertion but failed because the tiny synthetic glyph
payload shared the locale-marker page already read during open. The updated
fixture separates those pages and retains the cold-read/warm-reuse check;
formal execution awaits the next checkpoint. The failed journal is retained.

An optional actual-pack mode draws a mixed Regular/gold/bold/italic sample,
captures the result and requires 64 repeated frames to have identical pixels
and add zero provider reads. It is a cache/geometry regression, not a physical
calculator performance result or a substitute for actual rest/card/tooltip
screen acceptance.
