# UI16 coalesced locale reads

## Scope and behavior

Suffix alignment and artwork locality removed the first storage regression's
long frame, but the same Firebird comparison still measured three extra
physical reads and about 120 ms of cumulative resource time in its six-second
action window. This stage changes only the NSL3 runtime reader. Resource files,
images, glyphs, text and game behavior remain unchanged.

`BlockedResourceSource::read` now groups adjacent compressed and literal pages
covered by the **current request** into at most 32 KiB of encoded input. Each
batch uses one provider read, then decodes/copies its constituent pages. A
compressed edge page needs its complete encoded stream; a literal edge retains
its exact requested byte range. No other page is prefetched. Literal-only large
ranges keep the original single direct read and are not split at 32 KiB.

The existing 4 KiB decoded cache is preserved. A cached first page is copied
before reading the remainder. Literal data preceding a cached compressed page
is read directly, avoiding a reread of that cached page. A failed batched read
or failed decode cannot leave a partially written page marked as cached.

## Memory and provider boundaries

The 32 KiB encoded buffer replaces the original 4 KiB member buffer. Each
adapter therefore adds 28,672 buffer bytes plus four bytes of aligned busy-state
metadata on ARM. The calculator owns one static `SplitResourceSource`, whose
size grows by **28,676 bytes**. There is no allocation, large stack array,
`ResourceTransferLease` use or image-pool change.

The buffer belongs to its adapter. A provider may synchronously read a different
adapter, which has independent encoded and decoded storage. While an operation
is active, recursive `read` or `open` on the same adapter returns false and
`close` leaves the active source untouched. The guard is released on every
success/failure return. Normal foreground providers only sample input and do
not recursively dispatch application work.

## Exact real-resource oracle

The unchanged 57cd512 production renderer and canonical Haunted Ship fixture
run ordinary Continue and End Turn inputs against the actual `ui16-final`
resource files. Baseline and candidate use independent executables and the
same frozen core archive. Neither run changes resource contents or warms any
extra data. Each captures every complete action frame and canonical state.

| Current final resources | Physical reads | Requested bytes | Seeks | Cold artwork opens |
| --- | ---: | ---: | ---: | ---: |
| Original per-page reader | 19 | 102,505 | 17 | 0 |
| Coalesced reader | **17** | **102,505** | **17** | **0** |

The remaining mixed glyph request now uses one physical read instead of three.
The native count reaches the old-container replay's 17 reads with fewer bytes
than that old container's 103,971 bytes. This comparison makes no device FPS
claim.

The complete oracle agrees byte for byte:

- **182 RGB565 frames / 27,955,200 bytes**.
- **182 canonical state records / 41,966,288 bytes**, including their lengths.
- All **117 App-issued logical NSP2 requests**, including startup and the action window,
  retain the same order, offset, length and frame.

The frame hash transcript also matches. Private evidence is in
`ui16-locale-coalesced/build/coalescing-oracle.json`; the full `.rgb565` and
`.states` pairs are beside it. The reproduction harness is
`tests/locale_io_coalescing_regression.cpp`. Its base is checkpoint
`79172fb5853baba73c4a90d46ffaa8faa38a2086`, with production batching at
`ba9a2329d96213eb1b8325acde83df22018eafef`.

## Boundary and failure validation

`storage_source_regression` adds exact-32-KiB and just-over-capacity batches,
partial literal edges, cached first/last compressed pages, an unsplit 96-KiB
literal read, the full 2,048-page bound, maximum-file byte identity, partial
provider failure and retry, later-stream decode failure, and provider reentry
**after** filling the outer destination. Reentry reads an entire different
mixed payload so shared-buffer corruption would be visible.

The synthetic gate passes **10,075 checks** in both native and
AddressSanitizer/UndefinedBehaviorSanitizer builds. Actual English and Simplified
Chinese pass **12,485 / 12,619 checks** in both builds, comparing respectively
**76,368,876 / 80,793,488 logical bytes** and retaining the active image transfer
workspace. The storage adapters and resource mapping translation units are
instrumented directly; unrelated core archive code is not sanitizer-instrumented.
LeakSanitizer alone is disabled because this execution environment uses ptrace.

The newly linked coalesced reader also passes the complete actual `ui16-final`
16-language/font/startup gate: **89,816 checks, 53,072 descriptions, zero
failures**, including the 90 selected-language failure/corruption recoveries
to English. Its journal is
`ui16-locale-coalesced/build/journal/locale-coalesce-real16-1bbd31b8eb2c.json`.
The native complete-resource journals are
`locale-coalesce-real-eng-67e19859696f.json` and
`locale-coalesce-real-zhs-fa0d2ea28010.json` in the same directory.

This branch adds no CTest registration: the new cases run inside the existing
`storage_source_regression` test. `locale_io_coalescing_regression.cpp` is an
explicit private-resource oracle, not a misleading no-asset CTest pass. The
pre-integration 147-test count is therefore unchanged by this branch.

## ARM build and final gates

The complete ordinary ARM build passes. `BlockedResourceSource::read` uses
**72 stack bytes**, and `open` uses **136**. Compared with the frozen 57cd512 ARM:

| Metric | Baseline | Coalesced |
| --- | ---: | ---: |
| Executable plus static memory | 22,335,988 B | 22,365,404 B |
| Headroom below 22,450,000-byte execution budget | 114,012 B | 84,596 B |

The executable/static increase is **29,416 bytes**, including code and the
28,676-byte adapter increase. The adapter lives in `.data` because its cache
marker has a nonzero initializer; BSS size is unchanged. The candidate's total
Zehn allocation is **22,522,480 bytes** and its launcher is **1,759,688 bytes**.
These measurements precede integration of the independent artwork startup
header reuse change.

After the final integration, the actual resource gates are:

```sh
cmake --build build/native --target locale_editions_regression storage_source_regression --parallel 2
build/native/locale_editions_regression ../private-resources/ui16-final
build/native/storage_source_regression ../private-resources/ui16-final ../private-resources/baseline/eng.nsp eng
build/native/storage_source_regression ../private-resources/ui16-final ../private-resources/baseline/zhs.nsp zhs
```

Run these through `tools/run_logged.py` with the final source checkpoint. The
first command with resources exercises all 16 editions, actual font rendering,
model descriptions and selected-language failure/corruption fallback to English.
Its no-argument registry-only mode is not a substitute for that real-file gate.
The other two compare complete logical resources through production C++ adapters.
The target must still repeat its same-scene control/action pair and ordinary
cold/repeated startup before claiming the remaining device delay is resolved.
