# UI17 exact constant-color rendering

Source stage: `082869933a7654b2d62c1d4ee3e02879098876dc`.
Baseline runtime: UI16 `4f3c7abfb1c580d37f63a158385ff79a47e87874`,
recovered with final documentation at `1526ede2a84e168cd46d430338e5d5a540ecaf18`.

## Production change

Large `Canvas::dim` rectangles resolve their constant foreground color and
layer opacity once. A 128-entry RGB565 channel map then applies exactly the
former independently rounded source-over operation. The table occupies 256
stack bytes, is rebuilt for the current color/alpha, and has no persistent
cache or image-pool cost. Opaque and transparent cases retain their simple
branches. The foreground sampling cadence remains every sixteen rows,
including transparent and empty-width rectangles.

Coverage-backed card tiles, occlusion masks and small rectangles retain the
ordinary `paint` path. The fast path applies color matrix, brightness and RGB
tints in the same order as `paint`; its effective alpha preserves the separate
layer rounding. No background pixels, animation frames, input transaction,
material approximation or resource request are removed.

## Executed checks

The native production-Canvas regression passes **330,237 cases / 2,951,459
assertions** at the source stage above. Its new exhaustive comparison covers
all **65,536 RGB565 destination colors × 256 call alpha values** using an
independent channel formula. Additional cases cover layer alpha, color matrix,
tint, brightness, coverage, mask, clipping, empty/offscreen rectangles, pitched
surfaces, guard bytes and foreground collection cadence. Existing image,
palette, sparse-row, glyph, atlas and unaligned-source cases also pass.

The focused journal is `ui17-dim-exact-native-d200f8f756af`; the Clang 18 build
is `ui17-dim-clang-build-eb98b265b4fe`. Both completed with unchanged source.
The first GCC invocation was interrupted after a mistyped checkpoint ID was
noticed; its journal is explicitly invalid for acceptance. Its compiler log
also exposed an existing aggregate-assignment portability error in hover
preparation. Neither failure is counted as a pass.

## Local CPU measurement

`canvas_dim_benchmark` compiles both the former per-pixel `paint` loop and the
production path in the same translation unit, consumes visible output after
every frame, alternates run order and compares hashes. Seven runs each render
240 fullscreen backstops with the same changing alpha values.

| Case | Former median | New median | Matching hash |
| --- | ---: | ---: | --- |
| Normal layer | 343.386921 ms | 42.579836 ms | `da9ac2b5` |
| Opacity/brightness/tint layer | 496.696158 ms | 47.264607 ms | `50ecc402` |

Journal: `ui17-dim-native-benchmark-fb74b19c107e`. These numbers measure only
this native CPU operation, not a complete frame, calculator FPS, or a device
speedup factor. Ordinary-OS Firebird acceptance belongs to the integrated
delivery ledger once executed.

## ARM size and stack

The increment copied the 29 hash-recorded immutable baseline ARM objects to
a separate output directory, recompiled only `src/app.cpp` with the same
Clang 18 ARM926 flags, and linked/packed the ordinary launcher. The original
baseline build reproduces the UI16 delivered launcher hash exactly.

| Metric | UI16 | Dim stage | Change |
| --- | ---: | ---: | ---: |
| Launcher | 1,760,617 B | 1,758,707 B | −1,910 B |
| Zehn total allocation | 22,523,484 B | 22,520,364 B | −3,120 B |

`Canvas::dim` has a 328-byte ARM stack frame. The complete image pool remains
13,369,344 bytes. Journal `ui17-dim-arm-e34a529625c8` passed with unchanged
source. Candidate SHA-256:
`f8e99d5f2e67aa7aefeb7875d24292ed408c0e878096a9968efc083786cccf20`.
The later hover/seed/storage/integration stages require their own final sizes;
do not relabel this intermediate launcher as final.
