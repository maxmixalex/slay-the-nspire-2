# UI16 storage adapter validation

These are offline native and sanitizer checks of the NSL3 language-page adapter
and the NSM1/NSS1 artwork mapping. They add no startup integrity scan. Resource
files remain private. Current runtime
`4f3c7abfb1c580d37f63a158385ff79a47e87874` combines the bounded mixed-page reader
with single-probe artwork headers. Results below retain the source checkpoint
of each actual run; historical counts are not relabelled as current acceptance.

## Independent no-asset cases

`storage_source_regression` builds its own headers and LZ4 token streams instead
of asking the production Python converter for matching fixtures. It checks:

- Literal and compressed4096-byte pages, partial final pages, cross-page reads,
  contiguous literal-read coalescing and unchanged guard bytes.
- Warm-cache reads, page eviction, reopening another source, failed reopen,
  missing or unreadable input, invalid addressing, invalid compressed data and
  successful retry after a provider read failure.
- Exact 2,048-page and 128-span capacity boundaries, wrong format/version/source
  reference, wrong physical/virtual lengths, gaps/overlaps, invalid part IDs,
  missing/misidentified concern files and out-of-range reads.
- NSM1 header/spans and reverse/random traversal, zero-byte requests, close and
  reopen behavior.
- Both adapters under a live `ResourceTransferLease`: every byte of the active
  foreground workspace is compared after cold reads, and the lease remains
  owned by its caller. The tests do not acquire an overlapping workspace.
- Mixed compressed/literal requests at and just above 32 KiB, exact partial
  literal edges, cached compressed endpoints, an unsplit 96 KiB literal-only
  range, failed batch read/decode and retry, and same/different-adapter reentry.

The real-resource mode presents the new NSM1 plus eight concern files and an
NSL3 locale through the production `SplitResourceSource`, then compares the
complete virtual NSP2 stream with the immutable prior logical pack. It also
reads the raw directory suffix, artwork and language tail while an image lease
is active. The already-cached inner NSL2 directory prefix is deliberately not
redecoded under that lease, matching `Content`'s normal rendering contract.

## Current focused gates — 2026-10-29

The following gates execute the mixed-page production change at
`ba9a2329d96213eb1b8325acde83df22018eafef`, now integrated unchanged into the
final runtime. Actual input is the complete 89-span, aligned-language
`private-resources/ui16-final` edition. The lower counts than the original
13,270-check suite reflect changed table capacities and the revised cases;
test counts are not restoration percentages.

| Gate | Executed result |
| --- | --- |
| Synthetic storage, native | **10,075 checks passed** |
| Synthetic storage, ASan + UBSan | **10,075 checks passed**, no address/undefined-behavior report |
| Actual English, native | **12,485 checks**, all **76,368,876** logical bytes identical |
| Actual Simplified Chinese, native | **12,619 checks**, all **80,793,488** logical bytes identical |
| Actual English, ASan + UBSan | Same **12,485 checks** and complete logical-byte identity |
| Actual Simplified Chinese, ASan + UBSan | Same **12,619 checks** and complete logical-byte identity |
| Actual 16-language/font/startup gate | **89,816 checks, 53,072 descriptions, zero failures**; includes 90 selected-language failure recoveries to English |

These native and sanitizer runs preserve an active image transfer lease and
compare its workspace after the relevant cold resource reads. The sanitizer
executable directly instruments the storage adapters, resource mapping source
and regression. Unrelated game objects remain in the native core archive;
this is not a whole-game sanitizer run. AddressSanitizer and UBSan stay
enabled; only LeakSanitizer is disabled for the executor's ptrace environment.

Completed, unchanged-source journals under
`ui16-locale-coalesced/build/journal` are
`locale-coalesce-synthetic-a2114f8f2635`,
`locale-coalesce-sanitize-390228aacbbe`,
`locale-coalesce-real-eng-67e19859696f`,
`locale-coalesce-real-zhs-fa0d2ea28010`,
`locale-coalesce-sanitize-eng-f12557ec09f2`,
`locale-coalesce-sanitize-zhs-363a78fa0757` and
`locale-coalesce-real16-1bbd31b8eb2c`.

The independent exact replay also compares 182 complete RGB565 frames
(27,955,200 bytes), 182 canonical state records (41,966,288 bytes including
lengths), and all 117 App-issued logical requests. Baseline and batched reads
agree byte for byte. The action's physical reads decrease from 19 to 17,
with the same 102,505 requested bytes and 17 seeks. Details and source
identities are in `ui16-locale-read-coalescing-0.17.md`. This native replay is
not a calculator FPS measurement.

### Single-probe artwork headers and integration

At `946ca639114f5fea39803f5fd569d72c193f74b4`, the independent header-reuse change
passes **2,645 file-cache checks** in native and ASan/UBSan builds, with the
changed header/storage code directly instrumented. These include all header
fields, unused 0/1/31-byte parts, missing providers, length and read failures,
reused FILE addresses, cursor reset, LRU eviction, seven-stream language/state
combinations, sampled input and cleanup. Probe counters are checked against
actual fake-host operations; saved headers create no fake physical reads.

With the actual final resources, startup artwork opens decrease from 16 to 8,
while the same eight 32-byte reads and sixteen actual seeks remain. After
warmup the same 182-frame action and I/O agree. See
`ui16-artwork-startup-0.17.md` for these separate baseline/candidate journals
and the explicit resource-total profiling coverage change.

The integrated source preserves the exact independently checked reader/guard
and header/cache code. Header verification finishes before the locale adapter
is opened; it retains no stack pointers. Language switching and cleanup run
after reads return and release their guards, while provider polling only
samples input. The final complete ARM build passes at
`4f3c7abfb1c580d37f63a158385ff79a47e87874`
(`ui16-combined-final-arm-59335313de91`). It allocates 22,523,484 bytes including
157,096 bytes of metadata, with 22,366,388 bytes of executable/static memory.
For the final full native sweep and ordinary-OS execution/timing results, use
the delivery ledger; the focused tests here do not substitute for those gates.

## Historical initial NSM1/NSL3 results — 2026-10-29

Execution checkpoint: `024720dad7ea286203772c23dc40fa7ed5fb6237` in the isolated
locale worktree, using the parent storage adapter implementation and its
1,024-span and 4,096-page limits. These results predate the smaller current
tables, artwork-island folding, aligned locale suffixes, mixed-page batching,
and single-probe headers. They are retained as historical evidence, not the
current final test counts. Actual data: the original complete
`private-resources/ui16` output.

| Gate | Result |
| --- | --- |
| No private assets | 13,270 checks passed; CTest registration passed |
| Real English, native | 15,680 checks; all76,368,876 logical bytes identical |
| Real Simplified Chinese, native | 15,814 checks; all80,793,488 logical bytes identical |
| Real English, ASan + UBSan | Same15,680 checks and byte-identical complete stream; no address/undefined-behavior report |
| Real Simplified Chinese, ASan + UBSan | Same15,814 checks and byte-identical complete stream; no address/undefined-behavior report |
| New NSL3 / NSM1 complete16-language UI gate | 89,802 checks,53,072 descriptions, zero failures/unexpanded templates |

That historical 16-language gate includes 90 failed-selection recoveries to the real English
title screen: absent, truncated, bad magic, provider read failure after open,
bad NSL3 table and structurally valid container with a broken LZ4 stream. The
same checks include the actual normal-label pixel oracle and energy icons.

The sanitizer executable directly instruments `storage_source.cpp`,
`resource_source.cpp` and the new regression. Unrelated game objects come from
the native core archive. AddressSanitizer and UndefinedBehaviorSanitizer remain
enabled; LeakSanitizer alone is disabled because this executor runs under
ptrace, which its leak detector does not support. This is not a full-game
sanitizer or calculator performance claim.

The first independent fixture used the old generic version1 field with the new
NSL3 magic. The runtime correctly rejected it. The fixture now explicitly uses
version3; the production decoder was not relaxed. All runs listed above use
that corrected fixture. Failed first-run journals remain available.

Private command journals are under `ui16-locale/build/journal`, including
`locale-nsl3-sixteen-ed94fe0024ff`, `storage-no-assets-88d450774a40`,
`storage-native-eng-v2-582082418ede`, `storage-native-zhs-ea3cecfae636`,
`storage-asan-eng-v2-aa374b6ea2ba` and `storage-asan-zhs-399adc8a454c`.
