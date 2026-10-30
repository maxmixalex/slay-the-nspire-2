# UI17 resident-directory review and address-table fix

The independent review covered resident image-directory compaction introduced
by `3bfc6c8771cb9f08283caeafb52b68f29974bf4a`, its restored maximum-image fixture
capacity, and the combined checkpoint
`e1deee4601ca98be037b7869949b98c38002c0fe`. The subsequent address-table production
fix is `14b3185c8ced7d0e06c4c5a10f7dcc04e908b330`.

## Addressing and lifetime review

The streamed image records retain offset and shape words and combine the
three-bit format with the bounded length in a 12-byte resident record. Direct
memory sources keep their original 16-byte records. Valid descriptors are
reversible; malformed format/length values become a sentinel which cannot pass
the existing image-use bounds. Cache aliases compare the entire applicable
record, and cache publication uses the matching row stride.

Glyph, string, clip, card and UI catalogs all start after the resident image
directory. The tile header's two resident-table coordinates are reduced by the
same amount; descriptor and image payload coordinates remain original virtual
file offsets. A selected frame passes the original tile-header offset to its
identity check by adding the reduction back. Existing header bounds establish
that both resident-coordinate subtractions are nonnegative.

Prepared image costs reuse only the owned mutable directory copy. Direct
memory sources still use a separate owned cost tail, preserving immutable
input. The preload task stores both the record pointer and its stride. Opening
again first cancels the owner's preload and clears its image, tile, glyph and
string cache entries. A later failed chunk publishes no partial directory.
The 64 KiB temporary is a lexical `ForegroundScratch` allocation; saved image
views point to the normal image cache, not that temporary. No additional
addressing, object-lifetime, preload or cache-alias defect was found by this
read-only review.

The input directory bound remains 2 MiB. The compact resident budget is
**1,835,012 bytes**, preserving the existing 65,535-image full-directory
fixture and fitting the actual 16 editions. This does not preserve every
synthetic combination formerly able to fill 2 MiB: a directory with fewer
images and many unused group rows can save less than the required amount and
be rejected. No image, glyph or animation frame is removed from the actual
editions to obtain the memory saving.

## Measured startup issue and implemented fix

The new streamed compactor reads each logical 64 KiB directory block
separately. The prior `SplitResourceSource::read_directory` fetched the
corresponding NSL2 address rows on every request. Under the NSL3 wrapper, a
payload read could replace the cached page containing those rows. Re-reading
them then introduced extra physical reads and seeks. The isolated real English
package reproduced this issue; Chinese did not show the same extra reads.

The fix retains the at-most **32 × 8-byte address table** during split-source
open, after its header establishes the table bounds. Directory requests then
select their rows from that owned table. This is functional addressing
metadata, not a payload scan or integrity check. Per-row bounds and exact
literal/LZMA2 decoding checks still occur when a block is requested. A bad
unused row is not eagerly decoded or inspected as an asset.

The source remains invalid while the table is loaded. If a provider fails
after partially overwriting the table, no previous or partial mapping becomes
available. Close, reopen, switching between NSL1 and NSL2, language changes,
and switching between one and 32 rows are covered. Providers are immutable
for the lifetime of an open adapter; modified test resources are reopened
before their changed addressing is exercised.

### Actual file I/O

These are physical read counters from `resource_open_benchmark` using the same
reconstructed NSC1 and unchanged UI16 NSL3 English/Chinese files. The scope is
`split_open_and_content_open`; it excludes the Ndless launcher, OS startup,
NSM file-handle cache and subsequent `App::init`. Host timings are retained in
the private raw JSONL, but this one comparison ran alongside compilation and
does not establish a calculator-time or CPU-speed improvement.

| Locale / implementation | Physical reads | Requested bytes | Seeks |
|---|---:|---:|---:|
| English, UI16 full resident records | 42 | 246,312 | 17 |
| English, compact records before table retention | 44 | 253,295 | 19 |
| English, compact records with retained table | **36** | **218,380** | **13** |
| Chinese, UI16 full resident records | 46 | 298,847 | 12 |
| Chinese, compact records before table retention | 46 | 298,847 | 12 |
| Chinese, compact records with retained table | **45** | **298,847** | **12** |

The directory phase specifically changes from 25 reads / 136,729 bytes /
4 seeks to **22 reads / 129,746 bytes / 2 seeks** for English. Chinese stays at
32 reads / 218,101 bytes / 2 seeks in that phase. The virtual bytes requested
by `Content` are unchanged in both languages: 1,370,544 for English and
2,011,760 for Chinese over the full measured open.

## Executed gates and memory cost

All listed formal journals ran at production checkpoint `14b3185` with
`source_changed:false`:

| Gate | Journal | Result |
|---|---|---|
| Standard native build of affected targets | `ui17-directory-table-native-build-65ea87a05ae1` | Passed |
| Split source / resource reader / storage source CTest | `ui17-directory-table-three-native-8584dd224380` | 3/3 passed |
| Split-source ASan/UBSan | `ui17-directory-table-asan-5bd5717a454b` | 43,793 checks passed |
| ARM resource adapter and Ndless main compilation | `ui17-directory-table-arm-tus-d50d2da90991` | Both affected translation units passed |
| Actual English/Chinese file open | `ui17-directory-table-actual-8338e8ae57d2` | Both passed; counters above |

The three native suites executed 43,793, 24,871 and 10,075 checks respectively.
The new checks require separate 64 KiB requests to have the same physical I/O
as a whole-directory request; they also exercise table-read failure with a
scribbled destination, deferred malformed rows, payload failures, maximum/
minimum table lengths, raw-source transitions, language changes and successful
retry. ASan/UBSan uses the actual adapter, storage decoder and production
foreground lease types; only the otherwise unrelated game-core translation
unit's three workspace variable definitions are supplied by the test harness.
LeakSanitizer is disabled because of this container's ptrace limitation.

Actual ARM object comparison against the completed `e1deee4` build shows the
Ndless split-source instance increasing from **45,184 to 45,440 bytes**,
exactly **256 bytes of initialized storage**. Ndless main's code and BSS sizes
are unchanged; the resource adapter's ARM object code decreases from 7,492 to
7,204 bytes. These are object-level measurements, not a substituted final
launcher allocation. Static stack usage for `read_directory` falls from 344 to
72 bytes; `open` changes from 352 to 360 bytes. The change uses no heap
allocation and reduces no pixel cache. Complete combined-launcher and
Firebird results are separate integration gates; this component's two-TU
compilation is not presented as those results.
