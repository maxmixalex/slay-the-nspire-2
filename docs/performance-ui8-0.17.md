# UI8 performance and storage — 2026-10-24

This work preserves original UI assets, decoded pixels, animation frames and
map generation order. It changes the map workspace and lookup costs, image
cache key lookup, scaled sampling, and the offline encoder. The measured
resource reduction is **1,316,666 bytes per complete language pack**. The
owner's 30–40 MB installation goal remains open.

Long commands ran from pushed, immutable source snapshots with
`tools/run_logged.py`. Private receipts retain the exact command, source hashes,
start/end times, return code and output hash. Names below identify those
receipts without publishing original-derived packs or captures.

## Map workspace and generation

The segment workspace stored up to 22 columns as separate bytes even though a
column needs only three bits. Segments now store the same ordered column path
in nine packed bytes, plus its first row, length and linked-list index. Stable
point IDs are captured before graph pruning so clearing a grid slot cannot
change the textual key of a previously recorded path. These are representation
changes; path visits, random draws, group order and pruning decisions are
unchanged.

The former group search repeatedly performed a binary search over the sorted
key vector. A 4,096-bucket index now finds existing groups by FNV-1a of that
same key, with full key comparison on collisions. Only the first occurrence
of a new group updates the sorted vector. The hash index never determines
iteration order and never consumes the RNG.

| Measurement | Baseline | Packed paths and key index |
| --- | ---: | ---: |
| `sizeof(MapWorkspace::Segment)` | 26 B | 14 B |
| `sizeof(MapWorkspace)` | 1,114,120 B | 794,788 B |
| Workspace reduction | — | **319,332 B** |
| 1,024-case native elapsed time | 57,657.904 ms | 19,180.886 ms |
| Successful cases | 1,024 | 1,024 |
| Explicit failures | 0 | 0 |

The timing is one sequential A/B pair using Clang 18 `-O2`, after the other
team builds, converters and emulator run had ended. It measures **3.006×**
throughput, or **66.73%** less elapsed time, on this shared host. It is not a
calculator or ARM frame-rate claim, nor a repeated-run statistical estimate.
An earlier packed-only candidate reduced RAM but ran slower under concurrent
load; the committed key index addresses that additional lookup work.

`tests/map_storage_benchmark.cpp` covers 768 act/region/variant configurations
and 256 row-count, pruning, second-boss and treasure configurations. Each
transcript records the complete graph with ordered parent/child slots, settings,
stage draw counts and the four-word final RNG state. Baseline and candidate
transcripts are byte-for-byte equal: **1,307,868 bytes**, SHA-256
`e42c0bb68358a654e59f58cc958cc3401b159448b37efa0ac2ea714efaf655dc`.

Evidence: baseline checkpoint
`a82eb84fe2928523abaf375f91a5357e9cf057ff`, candidate checkpoint
`d45fe06ea9457729ed7623fb1c7cdeabede9288c`;
`ui8-map-quiet-baseline-73d81ec75b99.json`,
`ui8-map-quiet-hash-a58c1cd579c8.json` and `map-quiet-receipt.json`.
Both recorded `source_changed: false` and passed.

## Image cache and scaled sampling

The decoded-picture pool keeps its existing **748 × 4 KiB** pages and 320
entries. A 128-bucket index adds only **256 bytes**. Entry size is unchanged:
the image key word carries its 16-bit image ID, one-based nine-bit chain link
and seven-bit bucket. An exact owner/descriptor comparison remains mandatory,
including descriptor aliases. LRU order, eviction selection, contiguous-page
allocation and publication only after successful decoding are preserved.

`content_cache_age_regression` exercises 320 colliding keys, distinct owners,
descriptor aliases, capacity eviction, resets and age wrap. Together with
`canvas_fast_path`, `content_retained`, `content_tiles`, `resource_source` and
`content`, the affected stage-one native suites passed in
`ui8-cache-canvas-84651b48b8d7.json`. This establishes behavior and memory
accounting; no isolated image-lookup speed percentage is claimed.

Nearest-neighbor scaling for dense, palette and alpha pictures now advances
source coordinates with an exact quotient/remainder recurrence instead of
performing integer division for each destination pixel. Independent frame and
alpha comparisons in `canvas_fast_path_regression` passed. This preserves
sampling and clipping, including the recovered scaled-clip work; it does not
lower image quality or discard animation frames.

## Actual lossless recompression

`tools/recompress_resources.py` rewrites existing TGR1/RGR1 groups without
changing the NSP layout or the runtime codec. Every decoded group keeps its
original bytes and its original maximum **512 KiB** random-access bound.
Directories, group-relative references, glyph/string references and image
descriptors are relocated. A candidate is used only when its aligned stored
size is smaller; groups can never grow merely because the encoder is rerun.

The first stage uses the existing literal contexts with preset 9, nice length
273 and search depth 512. The second stage also tries three bounded context
combinations for each data kind. All have `lc + lp <= 4`, compatible with the
existing **28,544-byte** probability arena. The encoder's private cache is
keyed by the encoder version, normalized header and decoded raw bytes. It
reuses exact results between languages and later asset/font appends; invalid
cache entries are rejected and recomputed.

The actual v27 inputs contain **1,643 groups**: 1,184 TGR1 and 459 RGR1,
representing **552,278,819 decoded bytes**. Every output group, item slice,
literal image, glyph, string, geometry and timing entry was verified against
the input. This is a produced and verified resource transformation, not a
compression estimate.

| Complete pack | Original v27 | First stage | Adaptive second stage |
| --- | ---: | ---: | ---: |
| English | 94,100,448 B | 93,441,618 B | **92,783,782 B** |
| Simplified Chinese | 96,893,097 B | 96,234,267 B | **95,576,431 B** |
| Reduction per pack | — | 658,830 B | **1,316,666 B cumulative** |

The first stage improved 1,204 groups and took 125.142 seconds. The adaptive
stage improved another 1,629 group encodings and took 325.966 seconds. Those
are offline host durations under concurrent work, not runtime costs.

Adaptive output SHA-256:

- English: `2fa716191876c7c51e015c87e24cbc8e0d1cb1415da5ca4444656b9676baebbc`.
- Chinese: `541ee5dc7915272a08042ce55564d466d4432a748da9d7e1dbaffa97cc35313b`.

Receipts: `ui8-lossless-compression-92dcd280fe44.json`,
`ui8-adaptive-compression-3f32f510c920.json`, and each output directory's
`recompressed-receipt.json`. Both production conversions passed with their
immutable source fingerprints unchanged. Four synthetic encoder tests also
cover relocation, idempotence and cache validation. Native LZMA2 fixtures
exercise the three added context combinations, output bounds, damaged streams
and arena reuse after failure.

The exact split boundaries in those adaptive v27 files imply a 90,949,884-byte
common file, 1,834,058-byte English sidecar and 4,626,707-byte Chinese sidecar:
**97,410,649 bytes** for both languages, before a launcher, new UI8 assets,
new glyphs and saves. These are arithmetic from actual pack layouts; the final
UI8 split still needs to be emitted and reconstructed. Common payload savings
must be counted once for an installed bilingual set, not once per locale.

## Portable validation gates and recorded failures

The actual stage-three ARM build at
`dc4c07c0da37e7ade17ee042868b4e6e42883303` failed at link time because new
signed 64-bit pose/raster expressions required `__aeabi_ldivmod`, while the
runtime previously supplied only the unsigned helper. The failed receipt
`ui8-arm-stage3-9c4648be6a97.json` is retained. The fix supplies a shared signed
wrapper without changing pose math, converts `INT64_MIN` to an unsigned
magnitude, restores quotient/remainder signs, preserves r4–r11 and keeps the
stack aligned. It needs the next frozen linked-ELF validation before a portable
pass can be claimed.

`tests/arm_division_regression.py` now checks both production linked helpers
against exact Python integer quotients/remainders, including signed extremes,
random vectors, zero-divisor fallback and `INT64_MIN / -1` local bit behavior.
`tests/arm_lzma2_regression.py` also covers the adaptive contexts in actual
ARM926 instructions. Instruction-level checks are separate from ordinary
calculator launch and do not measure device frame rate.

The initial GCC 13 map-driver build encountered an internal compiler error in
the unchanged baseline and the candidate at the same aggregate reset. The
actual comparison therefore uses Clang 18 for both sides. One stage-two
command used an unavailable executable name; that launch-error receipt was
retained, and the successful retry uses the absolute compiler path.

An ordinary Firebird preparation session booted the owner's original OS from
a disposable copy of their NAND, installed Ndless through Documents, enumerated
the USB filesystem, prepared the game folder, flushed and quit successfully.
`ui8-firebird-prepare-fba94a9d4a0c.json` passed. No owner NAND was modified and
no emulator memory was injected. This preparation does **not** establish that
the new UI8 launcher has run; that requires the final ARM binary and matching
private resource set.

The final linked ARM allocation must still be measured against the existing
16 MiB limit after all UI8 features are integrated. The map workspace saving
provides room for features such as bounded map drawing, but it is not a
substitute for the final linked budget or a whole-process memory measurement.
