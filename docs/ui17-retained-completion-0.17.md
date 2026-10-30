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

## Complete actual candidate — 2026-10-30

Converter checkpoint: `44b239e5f41208929661ca70c96545ff65903e08`.
The independent, complete candidate is
`../private-ui17-retained/data/retained-completion-receipt.json`; its parent
directory contains the NSM1 index, all eight artwork concerns and all sixteen
NSL3 locales. English and Chinese logical packs are
`../private-ui17-retained/work/eng-retained41.nsp` and
`../private-ui17-retained/work/zhs-retained41.nsp`.

The actual 956 eligible groups yield **608 strictly smaller replacements**.
They save **262,042 artwork bytes**, including group headers, slice tables and
alignment. All 1,167 pre-existing filtered groups and three incomplete slice
groups stay unchanged. English checks all **2,126 complete retained groups /
106,620,585 raw bytes** independently before and after conversion. Every locale
then checks its own descriptors/text/fonts and the same already-proved artwork.
All 1,655 tile/index compressed streams are unchanged from the TGI2 candidate.

| Actual installed component | Bytes |
| --- | ---: |
| Artwork index and eight concerns | 72,957,060 |
| English NSL3 | 779,148 |
| Simplified Chinese NSL3 | 4,454,226 |
| All sixteen NSL3 editions | 23,775,602 |
| Unchanged first-stage index41 launcher | 1,761,477 |
| Default English/Chinese, including launcher | **79,951,911** |
| All sixteen, including launcher | **98,494,139** |

This stage saves **264,684 default-install bytes** and **272,378 all-language
bytes** compared with the first TGI2 candidate. Against the original UI16
81,022,714 / 99,576,281-byte deliveries, both implemented stages together save
**1,070,803 / 1,082,142 bytes**. The 65–70 MB target remains unmet. These counts
use the measured index41 launcher; any independently integrated runtime memory
or I/O changes must be counted again in their final combined build.

| Completed gate | Exact evidence |
| --- | --- |
| All sixteen conversion and split roundtrips | `ui17-retained16-production-b2d38b253e09`: passed, unchanged source, 582.90 seconds |
| Retained converter/filter Python tests | `ui17-retained-python-c2f1d28ec394`: 25 tests passed |
| Actual sixteen-language native loading and descriptions | `ui17-retained-actual16-7885670e572e`: 89,816 checks / 53,072 descriptions / zero failures |
| All actual image bytes and UI against UI16 | `ui17-retained-sequential-eng-430edb62bdbf`: 169,595,016 checks, 58,060 images / 1,167,930,140 decoded bytes, 40 identical complete UI frames |

The full-image gate uses the independent sequential reference spool introduced
by test-only checkpoint `f5a31dd841ed528b22d4e65e97b34e8ff98e84fc`, linked with
the unchanged `c495273` runtime. Both new memory and ranged decoding compare
every decoded image byte with the original UI16 decoder. It also checks 13,299
glyph records, 7,124 kerning pairs, 56,019 exact-size glyph samples, 8,704 strings
and 1,783 clips. The other gate journals use the converter checkpoint above.
Every completed journal has `source_changed:false` and `status:passed`.

The previous first-stage ARM binary and allocation remain valid because this
stage changes only offline conversion. This is not an additional Firebird
performance result. The eventual integrated runtime/data pair still needs its
own device acceptance; no frame-rate claim follows from fewer stored bytes.
