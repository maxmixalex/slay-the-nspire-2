# UI17 completion of existing retained-image color coding

This independent conversion stage follows the frozen TGI2 index candidate.
It adds no runtime code, changes no disk reader, and leaves the complete fixed
picture/group pools untouched. The original 81,022,714-byte UI16 delivery and
the first UI17 index candidate remain separate immutable inputs and evidence.

## Exact opportunity and scope

The UI16 artwork has 2,126 retained groups. Of these, 1,167 already use RGR2
and occupy 6,808,504 bytes; they are preserved. Another 959 use RGR1 and occupy
12,681,573 bytes. Of those, 956 groups covering 12,667,731 bytes have complete,
nonoverlapping image-slice descriptions. These later-appended unfiltered groups
can use the existing reversible RGB565 color coding already supported since
NSP2/36. The remaining three groups are preserved. There are no exact duplicate
encoded retained groups in the input, so none are claimed as deduplication
savings.

`tools/complete_retained_resources_ui17.py` uses the established RGR2 encoder
and complete-group smaller-result selection. Selection includes the slice
table, header and alignment bytes. Every image's dimensions, original format,
raw slice, palette/index data, alpha and hidden RGB remain exact. No quantization,
frame removal, resize, animation regrouping or cross-group reference is added.
Existing TGR1/TGR2 pixels and the new TGI2 index streams are copied byte for
byte. The common 512 KiB decoded-group and transfer bounds remain unchanged.

The shared historical converter now accepts later resource versions and emits
`max(input_version, 36)`. It therefore preserves NSP2/41 and does not downgrade
the TGI2 candidate when applying an older retained codec. Legacy conversion
tests still obtain NSP2/36 from their earlier input fixtures.

## Proof and output contract

The English conversion decodes and compares every complete original and new
retained group. All image descriptors must continue to point at the same raw
slice, with exactly the same dimensions and formats; literal pictures are
compared directly. All compressed tile/index groups, clocks, cache-cost bytes,
glyph pixels/metrics and strings are compared independently.

Every additional locale must have the original shared artwork and the exact
chosen group/slice mapping. Its complete new artwork must equal the already
proved English artwork. Its own glyphs, metrics, strings, metadata and split
roundtrip are checked. This avoids repeating the same 74 MB artwork proof
sixteen times while still checking each different locale payload.

The output contains the same eight required concern files, the small NSM1
index and all input locale editions. A separate receipt identifies preserved
filtered groups, incomplete slice groups, chosen modes, actual physical bytes
and per-locale proofs. First-stage and second-stage byte savings are reported
separately. Full native pixel/state and ordinary Firebird performance gates
follow the frozen output; fewer stored bytes alone are not a speed result.
