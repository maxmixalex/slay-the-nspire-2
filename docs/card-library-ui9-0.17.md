# Card Library native labels and controls — 0.17

The pinned reference is v0.111.0 / 41cef1ea. The ordinary browser exposed a
visible implementation defect: both bottom checkbox labels were displayed as
`View...`. The original scene supplies fixed22px KreonBold labels; the old
port sent them through an8px minimum instead of their native4px band.

## Source contract

The implementation uses `scenes/screens/card_library/card_library.tscn` and
its `library_sort_button`, `rarity_tickbox`, `card_library_tickbox`,
`card_cost_tickbox`, `library_pool_toggle` and `card_library_stats` scenes.
C# source counterparts are `NCardViewSortButton`, `NCardRarityTickbox`,
`NLibraryStatTickbox`, `NCardPoolFilter` and `NCardLibraryStats`. Their label
setters do not override the scene font sizes.

The FourByThree viewport is1680×1260, projected to320×240. The source VBox
spacing is applied before projection, preserving the narrow sidebar and
four-column card grid. Sorting controls start at source y216,338,558,666;
the two visible single-player bottom checkboxes start at1156 and1202.

| Label | Original face / size | Native face / size |
| --- | --- | --- |
| Sorting, rarity and bottom checkboxes | KreonBold22 | face5 /4px |
| Search field | Kreon Regular24 | regular /5px |
| Energy cost filters | KreonBold26 | face5 /5px |
| Card count | Kreon Regular22 | regular /4px |
| No results | Kreon Regular32 | regular /6px |
| Victory count on each card | Kreon Regular30, holder scale.8 or1.05 | regular /5 or6px |

Fixed labels use the complete source text, source cream/gold, black shadows
or outline and their authored boxes. They do not call the general card-text
fitter. A sorting arrow follows its label's natural width plus the12px source
HBox separation. Rarity hover preserves the scene's color multiplication;
disabling a rarity row multiplies by the source gray instead of fading its
alpha. The pool icons retain the source unselected saturation/value under
hover. Pressed labels use lightGray. Search uses plain UTF-8 LineEdit text,
including literal brackets, and scrolls to its existing end caret.

Victory counters no longer allocate an8px glyph bitmap and resize that
bitmap. The original `MegaRichTextLabel` enables vertical size checking but
leaves its horizontal bound disabled. A very large value therefore retains
the source band and nowrap behavior. RichTextLabel defaults to
`clip_contents=true` (Godot4.5 `scene/gui/rich_text_label.cpp`, constructor),
so output is clipped to the authored label box. No ellipsis, numeric cap or
modified win statistic is introduced.

## Resource scope

The separate font converter adds only missing exact scene glyphs. Its source
proof removes nine Chinese face6/4px keys accidentally collected from relic
flavor BBCode delimiters/attribute names; the actual C++ RichReader emits
none of them for all306 retained flavor strings. It preserves every visible
old glyph and every image/clip/string payload. Expected final counts are
English3292 and Chinese32764, below the unchanged32768 bound. The new labels
must be accepted against that complete font output; the previous21-kaiser
pack does not contain their entire4px alphabet.

## Verification

The source-stage short diagnostic checks actual control hit rectangles,
filter/sort/search/checkbox semantics, canonical-save purity and the existing
collection flows. `library_ui7_regression` also contains an independent
pixel oracle for the complete sidebar, all four rarity labels and five cost
values, both bottom checkbox labels, zero/nonzero card counts, no-results
text, and victory counters0/17/999/UINT32_MAX at both holder scales. It
reads exact glyph keys and alpha payloads rather than accepting a fallback
face, a nonempty output or a fitted-text plan as success.

The new source passes syntax checks and the short synthetic diagnostic.
Complete two-language font/pixel/capture acceptance requires the next remote
checkpoint and the new font packs. These pending resource gates are not
claimed by the earlier frozen8l front group, which separately passed13/13
actual-resource suites in each language.
