# UI19 lossless tile pairing and bounded cache integration

Reference: StS II **v0.111.0 / 41cef1ea**. The application version is
`nSpire 0.1.0 (StS II v0.111.0)`. This report covers the additional tile
compression and cache work after the combat/UI feedback fixes. The RGR3
retained-image filter and the two retained-group merging passes are separate
changes; the final resources include all three stages.
The [retained-group report](ui19-retained-groups.md) records those stages and
the complete per-stage installation table.

The final tile converter checkpoint is
`83b08dd025c41fbdbf56821b8dca9fc6eaf6d660`. The integrated production runtime
is `090023d45ec6211da3b2e5a887cfcc2d1dd68004`. Later documentation/tooling
commits do not change that tested C++ runtime. Public source synchronization
was rejected by automatic approval review; these are local source checkpoints
and preserved source bundles, not a claim of a verified remote push. Original
resources, source assembly, private fixtures, and device images remain outside
the source repository.

## Selected complete result

The final output uses NSP2/44, one NSM1 index, eight required concern parts,
and sixteen NSL3 locale files. It is produced from the frozen second retained
merge, with the previously encoded tile streams reused by exact content keys.
This is a complete conversion, including all frame-index remapping, lookup
tables, split boundaries, locale recompression, and alignment costs.

| Data installed | Original UI18 NSP2/42 | Final combined NSP2/44 | Saved bytes |
| --- | ---: | ---: | ---: |
| Index and eight common parts | 72,727,684 | 66,893,270 | 5,834,414 |
| English | 778,844 | 775,441 | 3,403 |
| Simplified Chinese | 4,455,923 | 4,451,796 | 4,127 |
| Default bilingual data | 77,962,451 | **72,120,507** | **5,841,944** |
| All sixteen languages and common data | 96,506,908 | **90,617,943** | **5,888,965** |

These are installed resource bytes, excluding the executable, state, licenses,
and documentation. They are not ZIP sizes. Tile pairing alone reduces the
second-retained-pass all-language result from 94,474,866 to 90,617,943 bytes,
an additional **3,856,923 bytes**. The integrated ordinary launcher is
1,724,027 bytes; its size is accounted for separately by the delivery report.

The complete logical English pack is 69,231,408 bytes, SHA-256
`e9caae6a215b96d9cc5e6160de2f1fc0ec37e06e61987a633d6e4a29838d3355`.
The final converter receipt SHA-256 is
`082d00ec73f152f8bcf5dcdb838c92208537633e67d66dc0524ae629d8f4b531`;
the complete 25-file manifest SHA-256 is
`29764c3278ec36dbb5e23b93249027e0d2980f66497c161ed54ad06092525a53`.
The local private delivery directory is `../private-ui19-combat/paired/data`.

## Why the selected compression is useful

Adjacent direct tile groups now share a 1 MiB LZMA dictionary. The original
layout's common 680-record groups already approached the 512 KiB raw bound;
the initial idea that these were generally 128 KiB groups was incorrect.
The selected implementation explicitly raises only the direct tile limit,
then provides the matching bounded runtime decoder and cache allocation.

| Complete conversion measure | Before | After |
| --- | ---: | ---: |
| Direct tile groups | 1,155 | 578 |
| Slots per group | 680 | 1,360 |
| Complete frame-index streams | 500 | 500, all remapped |
| Active frame/group occurrences | 79,279 | 69,616 |
| Largest raw tile group | At most 512 KiB | 1,047,200 bytes |
| Largest encoded group including header | — | 190,147 bytes |
| Added resident picture-arena bytes | — | **0** |

The 3,829,760-byte logical-pack reduction includes 3,826,621 bytes saved in
pixel streams, 2,533 in index streams, and 4,616 in lookup tables. It pays
3,146 bytes of padding to retain the suffix's proven 4 KiB page phase.
NSL3 directory and locale effects are included in the larger installed-byte
figure above. A group count or work-unit reduction does not establish faster
calculator frames; ordinary device results are reported separately.

No frames, pixels, glyphs, strings, clips, or timing records are discarded.
The converter compares **516,960,290 bytes of original tile records** after
decoding and reconstruction, including hidden RGB in zero-alpha pixels,
dimensions, and padding. All 58,060 image descriptors and all original
indices, geometry, timing, retained blocks, and decoded preparation costs are
checked. Every locale must have the same shared encoded artwork before the
single validated tile plan can be applied to it; localized catalog and
preparation data are independently checked for each locale.

The other measured tile candidate, sorting records within existing groups,
saved 7,174 pixel-stream bytes but enlarged the complete affected index
streams by 6,730 bytes: only 444 net bytes in the sample. It was rejected.
It is not included in the selected resources and is not reported as a
delivered storage improvement.

## Runtime memory and ownership

The global picture arena stays **13,369,344 bytes**. Normal combat keeps its
existing 6.75 MiB decoded-image / 6 MiB tile-group split. For NSP2/44 rooms,
the same arena is divided into 11.25 MiB of images and 1.5 MiB of tile groups;
older resources retain the 11.75 MiB / 1 MiB division. This trades 512 KiB of
decoded room cache for larger direct groups within the existing allocation.

The new 12-page room layout accommodates a 1 MiB raw group, a 64 KiB decoder
workspace, and the small saved dimension/alpha planes without another full
raw buffer. The tile plan copies its geometry and at most 300 frame tile IDs
into the existing workspace before releasing the source index pin. Its
workspace footprint is 65,272 bytes, below 65,536. The in-place inverse
processes the high region backward and the low prefix forward; the boundary
includes the record header's starting byte. Tests cover all 1–1,360 record
counts, overlap boundaries, hidden color, both filters, and malformed input.

The 1 MiB dictionary/property is accepted only for NSP2/44 direct tiles.
Legacy tile layouts, retained groups, indices, glyphs and strings keep their
existing 512 KiB limits. The encoded-input transfer arena is unchanged; the
maximum permitted encoded block including its header remains 617,323 bytes.
Future version 45 is rejected. No resource-wide integrity scan is added to
ordinary startup; offline verification remains developer work.

The final ordinary ARM allocation gate reports **22,298,288 bytes
(21.2653 MiB)**, below both the 24 MiB ceiling and the existing approximately
22.45 MB guard. The ELF sections are text 2,520,080, rodata 665,492,
data 4,092,000, and bss 14,863,032 bytes; their sum is 22,140,604 bytes
(22,140,608 including alignment at the end). Allocation/section accounting
is not a measurement of total process peak memory or peak heap use.
The ordinary ARM journal is `ui19-final-release-arm-633f454a09d8`, passed
with unchanged source. The launcher SHA-256 is
`fd40e3badd58366a1b48f9525daa4ef92709510b0c7d89756b0b87349dfd1d52`.

### Foreground and background index handoff

Independent review identified a possible shared-pin defect. A private
reproduction confirmed the failure: a background tile preparation and a
foreground frame shared one index, the foreground destructor cleared their
single Boolean pin, and a later retained-image reservation evicted the index
while background preparation still used it. The next preparation step failed.

The selected fix cancels the old background preparation only when a
foreground frame actually acquires an already-pinned index, and then pins
that published index for the foreground owner. It does not cancel every
cold image request. The published data survive cancellation; workspace and
unpublished reservations do not. The positive regression combines the same
shared index, two distinct frame descriptors, a 512 KiB retained-image
eviction, early/late preparation, both tile filters, and exact recovery.

The separate suspicion that any full small-room cache would fail an unrelated
foreground retained image was disproved: the existing reservation fallback
already cancels background work when space is actually required. No blanket
cancellation change was made. The original failing reproduction and the
disproved probe remain in the private evidence, separately labeled.

## Executed verification

The six targeted native suites passed after the shared-index correction,
with unchanged source (`ui19-shared-index-fixed-native-af27612891b5`):

| Gate | Checks |
| --- | ---: |
| Tile filtering, complete large groups, late fault/cancel, shared pin | 2,334,558 |
| Incremental preparation | 1,580,840 |
| Cache age and bounded reuse | 89,472,698 |
| Content tile readers | 18,733,171 |
| Retained groups | 6,251,916 |
| LZMA2 | 7,859,958 |

The converter's three end-to-end Python regressions cover complete tile
mapping, version rejection, all locales, and split reconstruction. They
passed under `ui19-merge-tile-tools-373e31438ca4`. The final integrated native
suite passes 157/157, and the final Python suite passes 362/362; the integrated
delivery report identifies their distinct frozen checkpoints.

Actual-resource merchant preparation also passed for the original NSP2/42
and the first complete paired NSP2/44 candidate: 27,535 and 35,323 checks.
The test was corrected to reuse a caller-selected logical resource source
across every reopen, rather than assuming an old split-directory layout.
It checks 185 stock images, 76 font updates, seven visible stocks, and the
Continue/open/reopen behavior. Its 2,255 versus 2,904 cooperative work steps
are not elapsed frame time or FPS. The older combat preparation benchmark
also fails on the unchanged NSP2/42 control at its outdated Continue
assumption; its output is not used for performance acceptance.

The final all-sixteen-language native gate passes **89,786 checks and
53,072 descriptions with zero failures**:
`ui19-final44-all16-locales-4aa31d0fa0a8`, unchanged source, 5.417 seconds.

The final combined NSP2/44 resources passed the complete native comparison
against the immutable original NSP2/42 control: **58,060 images /
1,167,930,140 decoded bytes**, each through memory, ranged, and incremental
decoding. Twenty contiguous ranges cover every image exactly once; all
13,299 English glyph records, 8,704 strings, 1,783 clips, and 40 identical
memory/range UI frames also pass the native checks. The successful journal is
`ui19-final44-all-images-4c094c092250`, unchanged source, **594.889 seconds**.
The maximum reference-spool bound is **67,108,736 bytes**, below the
67,108,864-byte (64 MiB) limit. No 1.16 GB temporary spool was required.

The final native receipt is `paired/final-native-images.json`, SHA-256
`970cd94d78287a15a1395fbf57bf6033789c87e2bf7b65a009ece3072bec2685`.
It records the tested integrated runtime checkpoint and native binary
SHA-256 `2d79d6d6dd7e0dacd1566089481ecbc39284262be84e2ed70e961827ba52d227`.
Its complete log SHA-256 is
`ad9339e833913266c2aaf19a2d806fd5d13dea32cd56185ec0e28d006076a8ff`.
This single final comparison covers RGR3, both retained-group merging passes,
and tile pairing together. No selected-range or intermediate-candidate
success is being substituted for that final coverage.

Ordinary Firebird comparisons of this exact integrated runtime with original
NSP2/42 and final NSP2/44 resources, including startup, Continue, animated
combat, character selection, and room pressure, belong to the final device
record. Host correctness and ARM compilation alone do not establish those
performance results. No physical-calculator result is implied.

## Rebuilding and updating

The checked-in transformation is `tools/merge_tile_groups.py`. It accepts
an existing complete 512 KiB tile layout, writes a new independent output,
and refuses an already-paired input. For the final UI19 sequence it follows
RGR3 and the two retained-group passes:

```sh
python3 tools/merge_tile_groups.py \
  --input /private/frozen-second-retained/data \
  --output /private/final-paired/data \
  --work /private/final-paired/work \
  --workers 2
```

Omitting `--locales` requires and processes all sixteen locales. A selected
subset must be named explicitly and include English. Cached
compressed streams are keyed by the exact transformed bytes and encoder
parameters and are decoded and compared before reuse. They are not trusted
on filename alone. All following split headers, descriptors, and locale
directories are generated from the new relocation; suffix page alignment
is checked after the physical split/round trip.

`tools/verify_resource_ranges.py` then covers the complete catalog with
contiguous, non-overlapping native oracle ranges, capping each temporary
reference spool at 64 MiB. It requires the union of the ranges to cover the
entire catalog; a single selected-range native success cannot be substituted
for the final proof. New game updates can use the same transformation and
validation stages once their extraction/schema/content changes have been
implemented and reviewed. Regeneration is not automatic proof that new
cards, enemies, effects, or save semantics have matching runtime behavior.
