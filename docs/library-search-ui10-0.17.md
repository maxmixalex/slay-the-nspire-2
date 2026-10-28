# Card Library search follow-up — UI10

Source: pinned v0.111.0 `NCardLibrary.UpdateFilter`/`TextFilter` and
`NSearchBar.Normalize`, `RemoveHtmlTags`, `ClearText`.

The original joins the card title, one space and the stripped description,
then trims, collapses whitespace and lowercases that combined candidate.
The query itself is only lowercased. Therefore a query can span the boundary
between title and description, while extra spaces around a nonblank query
can deliberately prevent a match. A wholly whitespace query leaves the
unfiltered discovered/undiscovered grid intact. The source clear button also
leaves a wholly whitespace draft unchanged while restoring input focus.

The previous implementation searched two independently normalized fields
and trimmed the query. This follow-up implements the source semantics while
retaining the250ms debounce, immediate Enter commit, English rarity keyword
match, discovery gate and cancellation of a captured pointer index when
results change. Upgraded descriptions still use canonical preview values.

The matcher streams the joined normalized candidate. It prepares one bounded
128-byte prefix table per query change and reuses it across the catalogue;
there is no concatenated card-text buffer, allocation or new resident pool.
The old2048-byte normalization buffer is removed. A title-only success can
still skip building a description. Unicode whitespace follows the source
Trim/regex categories; case mapping covers the Latin cased letters present
in the pinned English content while Chinese is unchanged. This is not a
claim of general search localization beyond the supported two languages.

Regression additions distinguish doubled/leading/trailing query whitespace,
cross-field English and Chinese phrases, stripped rich/HTML markup,
overlapping substring prefixes, nonbreaking spaces, all-whitespace unknown
cards, clear-button focus and preserved canonical state. Full matching
runtime execution belongs to the next source checkpoint.
