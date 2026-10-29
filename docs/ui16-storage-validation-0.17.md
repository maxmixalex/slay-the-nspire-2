# UI16 storage adapter validation

These are offline native and sanitizer checks of the NSL3 language-page adapter
and the NSM1/NSS1 artwork mapping. They add no startup integrity scan. Resource
files remain private. The production implementation belongs to the storage
integration; this change adds tests and their CMake registration only.

## Independent no-asset cases

`storage_source_regression` builds its own headers and LZ4 token streams instead
of asking the production Python converter for matching fixtures. It checks:

- Literal and compressed4096-byte pages, partial final pages, cross-page reads,
  contiguous literal-read coalescing and unchanged guard bytes.
- Warm-cache reads, page eviction, reopening another source, failed reopen,
  missing or unreadable input, invalid addressing, invalid compressed data and
  successful retry after a provider read failure.
- Exact4096-page and1024-span capacity boundaries, wrong format/version/source
  reference, wrong physical/virtual lengths, gaps/overlaps, invalid part IDs,
  missing/misidentified concern files and out-of-range reads.
- NSM1 header/spans and reverse/random traversal, zero-byte requests, close and
  reopen behavior.
- Both adapters under a live `ResourceTransferLease`: every byte of the active
  foreground workspace is compared after cold reads, and the lease remains
  owned by its caller. The tests do not acquire an overlapping workspace.

The real-resource mode presents the new NSM1 plus eight concern files and an
NSL3 locale through the production `SplitResourceSource`, then compares the
complete virtual NSP2 stream with the immutable prior logical pack. It also
reads the raw directory suffix, artwork and language tail while an image lease
is active. The already-cached inner NSL2 directory prefix is deliberately not
redecoded under that lease, matching `Content`'s normal rendering contract.

## Executed results — 2026-10-29

Execution checkpoint: `024720dad7ea286203772c23dc40fa7ed5fb6237` in the isolated
locale worktree, using the parent storage adapter implementation and its
1024-span limit. The final tests are independently cherry-pickable onto the
parent integration. Actual data: the complete `private-resources/ui16` output.

| Gate | Result |
| --- | --- |
| No private assets | 13,270 checks passed; CTest registration passed |
| Real English, native | 15,680 checks; all76,368,876 logical bytes identical |
| Real Simplified Chinese, native | 15,814 checks; all80,793,488 logical bytes identical |
| Real English, ASan + UBSan | Same15,680 checks and byte-identical complete stream; no address/undefined-behavior report |
| Real Simplified Chinese, ASan + UBSan | Same15,814 checks and byte-identical complete stream; no address/undefined-behavior report |
| New NSL3 / NSM1 complete16-language UI gate | 89,802 checks,53,072 descriptions, zero failures/unexpanded templates |

The16-language gate includes90 failed-selection recoveries to the real English
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
