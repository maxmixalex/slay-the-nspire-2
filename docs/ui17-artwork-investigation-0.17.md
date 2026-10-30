# UI17 bounded artwork investigations

These measurements are separate from the implemented TGI2 index compression
and completion of existing RGR2 coverage. They are not new runtime features,
installed-size savings or calculator performance results. The complete UI16,
first TGI2 and second retained-color candidates remain immutable.

## Exact retained slice duplication

The original complete-group audit found no repeated encoded retained group.
That does not imply that all of its individual image slices are unique, so a
separate complete slice pass checks the actual second-stage English pack.

`tools/audit_retained_duplicates_ui17.py`, checkpoint
`f47b3affb3c1bddcf7ba27c74e8e88fbde8eacd9`, excludes **1,123 existing descriptor
aliases** before counting **14,343 distinct retained slices**. Their raw bytes
sum to 106,023,757. Only **one** different slice has identical format, dimensions
and byte content: **9,240 raw bytes**, located in another group. Hashes locate
candidates; the actual byte strings are then compared. There are no additional
same-group duplicates. No compressed saving is inferred from the raw count,
and no cross-group runtime reference is introduced for this small opportunity.

Evidence: `ui17-retained-slice-audit-2a20c9f4a9f9` passed with unchanged source
in 70.90 seconds. Its private result is
`../private-ui17-storage/retained-slice-duplicates.json`.

## Exact 8×8 dictionaries within the existing groups

A bounded variation splits every original 16×16 pixel record into four exact
8×8 blocks, removes duplicate blocks **only inside the same existing group**,
and stores four uint16 local references per original tile. The measured stream
includes all four references, original dimensions, dictionary bytes and group
header/alignment. It restores the complete original 770-byte records, including
hidden RGB, alpha and padded edge pixels. Frame indices, independent group
membership and the current decoded group size remain unchanged.

This avoids assuming that four times as many global frame indices are free,
or that global re-cutting leaves disk requests/cache behavior unchanged. It
also avoids retesting the previously rejected color-plane/predictor variants:
each sample keeps its current color mode and uses one fixed LZMA2 profile.
A same-profile 16×16 control separates tile-shape effects from recompression.

Checkpoint `d6fbbb4bebc5615d6137d02e4d188000f887fbb5` samples **24 groups** at
systematic quantiles of the actual compressed-byte distribution. It compares
**12,566,400 original tile bytes** exactly and covers **1,371,438 stored bytes**.
None of the 24 candidates beats either the actual stored payload or the fixed-
profile 16×16 control. Replacing all 24 would add **99,132 bytes**. Selecting
only smaller candidates therefore saves **zero bytes**, and no new runtime
codec or disk format was adopted.

Evidence: `ui17-subtile-sample-a34fd01dc378` passed with unchanged source;
the private result is `../private-ui17-storage/subtile-dictionary-sample.json`.
The tools are `subtile_dictionary.py` and
`measure_subtile_dictionary_ui17.py`. A full global 8×8 regrouping was not
executed; the local result does not prove that every possible global grouping
would fail. Such a rewrite would need independent frame-index, memory and
actual-device I/O evidence before it could replace the completed candidate.
