# Source22 shared decoded-image cache — 2026-10-21

Reference remains **v0.111.0 / 41cef1ea**, on `work/0.17-world-parity`.
This stage keeps the actual NSP2/15 pack and all gameplay, map and save formats.
It addresses measured animation cache churn and adds reproducible size probes
for the owner's new **30–40 MB** storage objective.

## Executed source21 evidence available before this checkpoint

Source21 `d5738010f2d7ebc6139a2fa181ff87820d344bbb` was pushed and read back:
all **505 paths/modes/blob IDs and 13 changed contents** matched. Its source-only
archive SHA-256 is
`e1d4a718aab154927955ec3fd674215ad63d7c97cf5c8050e688ea51d05a21f5`.
The frozen runtime fingerprint is
`10b99c0cd8f61dd46cbf1f0a9b4adff9d0cdf604e291e17bc630d38bb92d6c8c`.

| Gate | Source21 result |
| --- | --- |
| Native sanitizer build | All 147 steps passed |
| ARM launcher | 885,145 bytes; 14,713,908-byte allocation; 28,595 relocations |
| WASM | 4,879,462 bytes; fixed 16 MiB memory bound retained |
| Linked ARM vectors | 5,249 division/ABI and 28 LZMA2/ABI vectors passed |
| Affected registered suites | Ten ASan/UBSan suites passed in 50.61 s |
| Final synthetic tile suite | 5,455,291 checks, including fixture assertions |
| Actual native/WASM comparison | 23 scenarios / 480 exact frames / 30 exact saves |
| Actual act-entry UI | 43,364 checks and four captured scenes passed |
| Full new-memory versus original images | 45,724 images / 994,105,847 decoded bytes matched |
| Second complete range-backed traversal | Still running at source22 preparation; not yet a complete oracle pass |

The ten suites are content, tiled content, resource source, LZ4 compression,
LZMA2 blocks, world text, map UI, map travel, map window and map act introduction.
They are ten affected suites out of 56 registered, not a full 56-suite run.
The last complete baseline remains source17's 52 suites.

The source21 all-resource oracle continues on its unchanged frozen source.
It must finish the range-backed traversal, metadata, 40 three-owner UI frames
and failure/reopen checks before being recorded as passed. The actual pack is
80,846,058 bytes, SHA-256
`407589a297a6e1bf935b6f2a5024ad532efd84423b0374db13c080e6671ba7dd`.

Two failed optimized-host attempts remain in the evidence. GCC 13 emitted an
internal compiler error in `gimple_add_tmp_var` while compiling unchanged
`world_relics.cpp`; no source workaround was introduced. The first Clang
configuration referenced a missing C-compiler alias. Correcting that path and
using Clang 18.1.3 `-O2 -DNDEBUG` completed all 33 requested build steps.
The large actual-file oracle and host benchmarks use this non-sanitized build;
the ten registered suites use the separate sanitizer build.

## Why a shared image pool

The previous cache reserved 64 tiny, 16 small, 8 medium and 2 large buffers.
Their total pixel budget was 3,064,768 bytes, but a 23-frame clip with a combined
decoded size of 1,557,577 bytes could only use eight medium slots. Actual
two-cycle measurements confirmed repeated reads and decompression on every
frame, including the second cycle. See [resource-budget-0.17.md](resource-budget-0.17.md).

Source22 shares **748 × 4,096-byte pages** among the existing 90 entry capacity.
The pixel array is 3,063,808 bytes. A compile-time assertion includes entries,
page bookkeeping and the LRU counter in the previous total cache budget.
There is no heap allocation and no compaction. Surviving entries keep stable
addresses; a Picture view expires when its entry is evicted or its owner is
reset/failed. The rendering call sites consume views synchronously.

Allocation finds a contiguous free extent, evicting the least recently used
published entries only when needed. Hits refresh recency, including exact-key
timing aliases. The existing complete descriptor key remains unchanged.
Unpublished reservations are released on failure; a failed access clears the
failed owner's caches. Other owners remain usable. The age counter preserves
relative LRU order on wrap.

Regression pressure must now exceed the actual common capacity. Old fixtures
that depended on tiny/small tier round-robin positions are updated to request
enough unique pictures explicitly. Assertions for first-use corruption, alias
identity, un-evicted view stability, owner/reopen isolation and validation-bit
fallbacks remain required. New cases cover a 23-medium-frame loop and mixed-size
fragmentation/eviction through the public Content API.

Two selected pre-checkpoint Content-only ASan/UBSan diagnostics passed with
unchanged source: **361 alias checks + 7,411,030 tile/cache checks** in 22.075 s
including compilation, and **1,779,606 workspace/bitmap checks** in 13.151 s
including compilation. These are separate selected probes, not their sum as a
complete registered-suite result. No old compiled App methods were invoked.

The actual-resource oracle now accepts an explicit optional
`--images FIRST COUNT` after the reference path. It still compares both new
backends against independent original bytes and checks all metadata and 40
three-owner UI frames, while every report labels the selected image range.
Without that option the complete-catalog proof remains the default. This lets
the unchanged codec's source21 full proof be retained while source22 exercises
its concrete changed-cache risks on all retained pictures and the real 23-frame
animation, without calling a selected range a complete new-source proof.

## Storage research scope

`probe_tile_transforms.py` and `probe_retained_compression.py` are source tools,
not production format changes. They compare reversible representations using
the existing LZMA2 parameters and verify inverse bytes. Private output records
identify sample scope, exact input hashes, group bounds and unimplemented
metadata/runtime costs. No original-derived payload is committed.

The inspected Sprite Dicing upstream uses overlapping deduplication ideas, so
its example compression rate cannot be applied again to this pack. The first
exact-bounding-box sample grew by 1.986%. Further tile predictors, exact palettes,
retained-image and glyph grouping require actual measurements. A 30–40 MB
complete calculator installation has not yet been produced.

## Required checks after source preservation

1. Push and read back this coherent source, retain the source-only archive and
   freeze the exact stage before long build/experiment execution.
2. Run the final allocation, failure, alias and structural sanitizer regressions.
   Build ARM/WASM and check the unchanged memory limits on their linked results.
3. Repeat the actual clip-967 two-cycle benchmark with the same private pack;
   verify callback counts and compare every requested image against original
   bytes. A model predicting cache hits is insufficient.
4. Run fresh native/WASM scenario captures to detect renderer view-lifetime or
   shared-owner regressions. Keep source21's complete independent byte oracle.
5. Run the bounded size diagnostics from the frozen source and record actual
   sample/full scope. Do not present a grouped-payload model as a ready file.

No calculator or Firebird speed measurement, original-PC pixel acceptance or
95% original-UI result is claimed. Existing animation sampling, label clocks
and remaining shader/particle layers stay explicit fidelity gaps.
