# Original conditional text and Pael markup — 0.17

Reference remains the owner's pinned v0.111.0 / `41cef1ea` installation.
This stage follows the retained room/map/enemy checkpoint. Original locale
files and decompiled sources remain private; test fixtures distribute syntax
with neutral labels, not the original prose.

## Cause and implementation

The actual-pack ancient UI exposed unexpanded `AncientCard.StringValue` and
`StarterCard.StringValue` expressions. Both card bindings already existed.
The missing behavior was the named `cond` formatter for empty/nonempty strings
and booleans. The old interpreter only understood a single numeric comparison
with zero. It also misread Shrink's successive `==1` and `>1` branches.

`LocalizedTemplate` now handles the forms actually present in the pinned
Chinese locale: string/boolean alternatives and ordered integer comparison
branches with a final fallback. Nested placeholders retain the selected value
as their context, including the empty selector `{}`. Comparison uses the stored
rational value without multiplying signed operands, and accepts the signed
64-bit endpoints without overflow. Unsupported or missing bindings remain
visible; this is a bounded locale interpreter, not a general SmartFormat port.

The original `LocManager.LoadLocFormatters` registers SmartFormat's
conditional formatter. Its string branch tests empty/nonempty, whereas the
game's `StringVar` constructor defaults to an empty string. The existing
presentation resolver now honors that explicit default for Dusty Tome,
Archaic Tooth, Touch of Orobas and Sea Glass when no prepared or owned data
exists. This lets the original generic description appear in the library.
Other unknown variables are not silently converted to empty text.

The original Archaic Tooth `UpdateHoverTips` binds `CardModel.Title`, which
includes `+` for upgraded cards. The presentation reference previously dropped
that saved upgrade bit. It now appends the suffix for upgraded card references;
Dusty Tome continues binding its base card because its original template adds
the suffix itself. No offer is regenerated while displaying a description.

## Pael's text effect

`RichTextThinkyDots` is a paired BBCode effect, not a variable. It wraps Pael's
dialogue and can contain nested color tags. The reader now consumes the paired
markers, preserves their UTF-8 content, tracks the effect's relative character
index and restores the previous style at the closing marker.

The original motion uses a 400 ms upward half-sine, a 4,000 ms rest and a
100 ms delay per relative character. The calculator presentation quantizes
that movement to at most one screen pixel and uses its existing presentation
clock. This amplitude/clock choice is an explicit small-screen adaptation,
not an assertion of original GPU/subpixel rendering equivalence. It does not
affect wrapping, gameplay RNG or serialized state.

## Regression coverage

All **19** entries containing `cond` in the private original Chinese locale
were inventoried:

| Table | Entries | Relevant forms |
| --- | ---: | --- |
| Cards | 1 | Sovereign Blade's optional Block line |
| Gameplay UI | 1 | Optional modifier count |
| Powers | 6 | Source-name strings; negative values; Shrink's ordered branches |
| Relics | 9 | Ancient offers, character titles and Pael's swallowed-card list |
| Run history | 2 | Positive-count category fragments |

The permanent header regression retains those keys and conditional structures
with neutral labels. It also checks negative/zero/one/multiple amounts, nested
contexts, rational comparisons, numeric bounds, unknown selectors and Pael's
tag nesting, Unicode content and pulse timing.

The separate `world_text_regression` exercises the production App resolver with
a synthetic content profile, and accepts the owner's private full pack as an
optional argument. It covers all five characters, prepared and owned relics,
upgraded/unupgraded Archaic Tooth cards, exact title references, generic empty
defaults, save/load continuation, Pael's original first-visit markup and Shrink.
It checks complete saved-state equality around read-only presentation.

## Status before the next checkpoint

The standalone header diagnostics passed **124 assertions** with ASan/UBSan.
A separate private probe read all 19 original strings directly from the pinned
PCK and exercised four values each: **76 cases**. The unchanged old interpreter
failed **52** cases with visible syntax or the wrong Shrink branch; the repaired
interpreter passed all 76. The private probe uses controlled bindings, so that
result does not by itself verify every production variable binding.

The new integration translation unit passed syntax checking. Its linked
execution, actual-pack rendering, and cross-target builds still require the
next preserved source checkpoint. LeakSanitizer is disabled in this sandbox
because its process/thread enumeration is unsupported; AddressSanitizer and
UndefinedBehaviorSanitizer remain enabled. No full-suite or UI-fidelity
percentage claim is made here.
