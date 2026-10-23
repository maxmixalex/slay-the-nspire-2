# Card rendering and startup measurements — 2026-10-23

## Startup metadata reuse: implemented and measured at `06c7fd16`

The next startup optimization is implemented without disabling any validation
or enlarging resident allocations. It reuses spare directory memory for
descriptor rows and validated index-record summaries. A same-input v18 split
resource comparison observes fewer file requests in both languages. Warm-host
complete-open medians fell by **3.95% for Chinese and 3.40% for English**;
the timing ranges overlap substantially, so these modest host observations do
not establish a calculator cold-start improvement.

### Exact execution and measurement boundary

The baseline core is `61970600a8d3f7fb2833af13f82f09c86105ebec`; the saved,
executed candidate core is `06c7fd16e6d4d47dc25b628913ecdeef3a4fe76d`.
One identical `resource_open_benchmark` object was compiled with Clang 18.1.3,
C++17 and Release `-O3 -DNDEBUG`, then linked separately to the two frozen core
libraries. Both use the same receipt-bound v18 common and Chinese/English
locale files. The newer v19 merchant resources are not part of this A/B.

After one excluded warmup per binary and language, seven rounds alternate
binary and language order. All **28 measured opens and four warmups** run in
fresh processes, with other team builds, browser work and emulator work paused.
Hashing the input files before timing intentionally warms host filesystem
caches. The accepted report also verifies unchanged input and binary hashes
afterward, complete phase counters, zero read failures and identical FNV-phase
file requests between versions.

The interval covers `SplitResourceSource::open` and `Content::open`. It excludes
file opening/length discovery, allocation of the `Content` object, progress
drawing, `App::init`, language preference writes and first render. This is
steady-clock host wall time, not a CPU profile. File callbacks, requested bytes
and seeks describe the production ranged FILE adapter; they are not physical
disk transfers. These boundaries differ from the earlier Firebird sample's
selected-resource validation and App-initialization interval.

### Observed time and deterministic read reduction

Times are milliseconds. Ranges show the minimum and maximum of seven measured
opens, not confidence intervals.

| Language / scope | Baseline median | Candidate median | Median reduction | Baseline range | Candidate range |
| --- | ---: | ---: | ---: | ---: | ---: |
| Chinese complete open | 796.041 | 764.588 | 3.95% | 743.189–838.358 | 746.011–821.701 |
| English complete open | 769.710 | 743.533 | 3.40% | 716.220–830.187 | 727.590–827.891 |
| Chinese tile-index phase | 566.783 | 554.114 | 2.24% | 546.247–624.050 | 520.954–604.872 |
| English tile-index phase | 554.148 | 529.195 | 4.50% | 523.602–605.111 | 525.263–608.077 |

The deterministic request changes are a stronger result than attributing all
of the wall-time difference to the new code. Per-phase request counters are
identical across each version's seven runs:

| Scope | Baseline file callbacks | Candidate file callbacks | Baseline requested bytes | Candidate requested bytes | Baseline → candidate seeks |
| --- | ---: | ---: | ---: | ---: | ---: |
| Chinese tile-index phase | 1,382 | 1,155 | 10,476,206 | 10,008,974 | 939 → 937 |
| English tile-index phase | 1,382 | 1,155 | 10,476,206 | 10,008,974 | 942 → 937 |
| Chinese complete open | 10,091 | 9,864 | 142,988,701 | 142,521,469 | 6,248 → 6,246 |
| English complete open | 9,433 | 9,206 | 137,514,113 | 137,046,881 | 6,239 → 6,234 |

Each language eliminates **227 tile-index file requests (16.43%) and 467,232
requested bytes (4.46%)**. The complete-open totals include the split adapter's
five unphased requests, 212 bytes and four seeks. The FNV phase still completes
all 91,439,606 Chinese / 88,663,928 English body bytes. Its FILE counters and
the retained-image phase's FILE counters are unchanged. Unmodified phases also
show timing variation, including a 7.72% lower English FNV median, which must
not be presented as a hash optimization. No statistical confidence or target
speed ratio is claimed from these seven-round host observations.

### Reuse and validation coverage

The implementation borrows the unused tail of the existing 2 MiB directory
buffer during startup. It optionally copies the complete 16-byte descriptor
table once and derives up to 64 per-group summaries, each containing an exact
record offset, exact record length, minimum acceptable width and row count.
The summary occupies 768 bytes and is replaced for each index group.
Binary search replaces repeated scans from a group's first variable-length
record. The already required generic geometry traversal produces the summary;
it still checks every record, including records not referenced by an image.

After that traversal, each image must still satisfy its exact row count,
minimum width and exact tile-grid byte count. Every descriptor is checked for
the correct shared header, group, bounds and exact record boundary/length.
Aliases with conflicting image shapes retain the original independent-check
path. The first-use tile checks still validate actual tile dimensions, edge
padding, alpha requirements and reconstructed output size. Summary reuse does
not mark pixel payloads as already decoded or validated.

For the actual v18 Chinese directory, the available tail is 953,513 bytes.
The existing grouped lists use 233,088, the summary uses 768 and the descriptor
copy uses 462,560, leaving 257,097 bytes unused. Other legal directories can
omit the descriptor copy, then the summary, or fall back to the original
sequential path when space is insufficient. No persistent validation cache,
new global buffer, larger `Content` allocation or larger image/group pool is
introduced. Reopen and failure continue to clear owner state.

The integrated `06c7fd16` ARM link separately reports **16,513,264 bytes** of
Zehn allocation, leaving **263,952 bytes** below 16 MiB. This total includes
the concurrent UI/resource-version work; it is not a measurement of the
summary's temporary workspace. The ARM build is a passed compile/allocation
gate, not a target startup measurement.
The later intent-slot correction at `4f50193b` links at **16,513,432 bytes**,
leaving **263,784 bytes**. Its `content.cpp`, `content_tiles.inc` and
`resource_source.cpp` are byte-identical to `06c7fd16`; it does not replace
the A/B's actual execution identity.

### Separate same-v18 Firebird observation

The candidate `06c7fd16` also completed a Chinese Firebird invocation with the
same 91,439,646-byte v18 virtual resource, exported through ordinary emulated
USB. Its profile has one sample for startup and each phase, a 32,768 Hz SP804
clock, and zero resource-read failures:

| Observed scope | Timer ticks | Simulated seconds |
| --- | ---: | ---: |
| Selected-resource validation and App initialization | 1,618,575 | 49.394989 |
| Full-body integrity | 755,499 | 23.056000 |
| Retained image validation | 408,394 | 12.463196 |
| Tile-index validation | 398,390 | 12.157898 |

This invocation started from a fresh OS boot and Ndless installation, using a
copy of the earlier closed flash image, without an initial RAM snapshot. The
resource files already existed, and host caches were not flushed. It is a
**separate target observation, not a controlled cold-start A/B** against the
earlier `61970600` sample of 50.472626 seconds. The two invocations differ in
cache history and integrated runtime. Their numerical difference is not
reported as a causal or repeatable startup speedup.

As in the earlier target profile, the interval begins after language selection
and includes validation, progress drawing and App initialization. It excludes
language-selection waiting, Ndless/Zehn loading and first render. The latter
is separately 24,171 ticks / 0.737640 seconds here. The invocation-wide
resource totals include work after initialization; they do not supply a
target per-phase I/O/CPU breakdown. The same-input host request reductions
above must not be substituted for measurements of target storage traffic.

The exported `v18-control.perf.json` is 4,323 bytes, SHA-256
`b3a5277aaed46a4c498b71c723b70e61538953fa1511fba8c26978e9ce307cf3`.
Its execution journal, `merchant-firebird-v18-control-63403a59129e`, is closed
passed with unchanged frozen source and an independently matching log hash.
The final `4f50193b` runtime with v19 resources has a separate platform receipt;
it is not substituted into either this same-v18 observation or the host A/B.

### Verification and retained failures

At `06c7fd16`, the tiled-content suite passes **13,941,796 checks**. Its new
cases cover the descriptor table larger than the reader cache, reversed
record lookup order, unreferenced invalid descriptors, wrong record boundaries
and lengths, too-narrow shapes, wrong row counts, different tile grids,
conflicting-shape aliases, first-use invalid edge tiles, metadata read failure
after FNV, successful reopen and all workspace fallback levels. The large
fixture bounds descriptor-phase reads without imposing a fragile timing
threshold. Resource-source and split-source suites pass 8,606 and 1,014 checks.

Actual v18 Chinese/English memory-versus-file validation passes 352,068 and
306,748 checks. Each compares 128 selected images (8 retained and 120 tiled;
2,206,189 decoded bytes), the full glyph/string/clip catalogs and 40 UI frames.
These selected image comparisons are not an all-47,183-image pixel traversal.
Both opens still validate the entire startup directory and index catalog.

The first aggregate synthetic run is retained as failed: its cache-pressure
fixture still assumed 90 entries while the real cache holds 320. The unchanged
fixture fails at the same assertion when linked to the saved `61970600` core.
The correction uses `Content::image_capacity_entries() + 32` distinct images
and explicitly checks eviction before injecting the read failure; it preserves
the failure, other-owner-health and reopen assertions. That test-only change
was saved in `89dc0f80fc48a78bd8bae49041343442d2764aea`; its corrected execution
passes **1,554 checks** at the later frozen runtime
`4f50193b212bb5d27e4a7e82deaa7622a5d4757d`, with a closed journal, unchanged
source and matching log hash. That execution is not retroactively assigned
to `06c7fd16`.

The cache-age executable initially could not launch because its mode was
0644. A logged permission-only repair to 0755 preserved the executable SHA-256;
the subsequent independent run passes 1,953 checks. The original failed CTest
journal remains failed.

The first A/B command's outer journal says passed, but independent readback
found only 27 measured samples, a still-running inner report and a log hash
that did not match the outer journal. No surviving benchmark child or temporary
report was found. That attempt is **not accepted** and its files are preserved;
the cause is unproven. The accepted repeat uses a different output, checks
28 + 4 successful child results and final report status inside the command,
flushes the final report, and independently verifies its report and log hashes.
It does not combine samples from the incomplete attempt.

The metadata-only receipt [validation-startup-reuse-0.17.json](validation-startup-reuse-0.17.json)
records execution identities, all accepted timing samples and request counts,
resource/artifact hashes, closed journals and these rejected/failed attempts.
The earlier rendering and target observations below retain their original
identities and values. **Full-body FNV, startup structure/alias checks and
first-use payload checks remain enabled.** Combining the FNV stream with
later encoded-record validation remains a separate, unimplemented optimization;
this metadata slice does not remove their repeated payload reads.

## Earlier card-rendering slice at `069f6d51`

The rendering changes remove repeated small-card composition and glyph-pixel
reads during text measurement. The two static Chinese grid scenes made **zero
resource-source callbacks over their 180-frame warm windows**; both languages
also reduced measured host render time. All animation clocks continued to
advance. This report does not claim calculator FPS or a device cold-start gain.

## Source and method

The baseline runtime is `e8444322b494db67d905e4772ff95373d7bfd34a`; the remotely
preserved candidate is `069f6d51f37aef3f31d11a6282bd02a61d7afdef`.
Both static libraries were built with Clang 18.1.3, C++17, CMake Release
(`-O3 -DNDEBUG`). The unchanged `tests/card_grid_benchmark.cpp` was separately
linked against each library and its matching headers. Original NSP2/17 Chinese
and English inputs were held identical between baseline and candidate.

The driver opens the resource read-only through the production ranged-file
callback and cursor. Seed 781 practice fixtures provide eight-card library and
ten-card removal/selection pages. Removal uses `buy(ShopRemove)` after setting
up a shop fixture. Selection places an already owned Secret Technique instance
in the opening hand, preserves zones, then uses production `play()` to suspend
on its actual draw-pile choice. It does not fabricate a choice queue.

After preparation and the explicit settling interval, each scene receives 180
updates of `advance(33)` plus `render()`. Canonical save bytes remain identical
before and after every window, and no save callback is attempted. Observations
were sequential, with no display pacing and already warm OS file pages. Host
elapsed time is not an estimate of physical calculator performance.

## Measured warm windows

Time is mean host milliseconds per update plus render. Source bytes and calls
are totals across 180 frames, not per-frame counts. MB uses decimal units.

| Language | Scene | Baseline ms | Candidate ms | Time reduction | Source MB | Source calls |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Chinese | library_8 | 1.047 | 0.549 | 47.53% | 49.315 → 0.000 | 12,060 → 0 |
| Chinese | remove_10 | 1.275 | 0.639 | 49.88% | 57.508 → 0.000 | 14,040 → 0 |
| Chinese | draw_choice_10 | 2.056 | 1.585 | 22.90% | 93.429 → 65.437 | 22,830 → 15,996 |
| English | library_8 | 1.894 | 0.615 | 67.54% | 0.000 → 0.000 | 0 → 0 |
| English | remove_10 | 2.075 | 0.655 | 68.41% | 0.000 → 0.000 | 0 → 0 |
| English | draw_choice_10 | 3.045 | 1.599 | 47.50% | 0.000 → 0.000 | 0 → 0 |

English already fit its glyph/text working set in the reader's page cache;
its zero source calls do not imply that its previous repeated layout and
composition were inexpensive. The larger Chinese working set previously
refetched pages during every static-grid frame. The animated combat choice
retains nonzero reads and less acceleration; its changing backdrop and live
scene remain visible and are not frozen to obtain a faster result.

## What changed and what was checked

- `Content::glyph_metrics` reads only resident descriptors. Full-catalog
  measurement leaves all resource-read statistics unchanged and does not evict
  the 64 glyphs prepared immediately before it.
- Card-title fitting and centering run once for all eight outline passes and
  the foreground pass. A pixel comparison preserves their prior paint order,
  including antialiased overlap.
- Eight bounded cache slots retain exact before/after RGB565 card regions.
  Cache hits compare every destination background pixel; changed backgrounds
  rebuild through the original renderer. Slots used earlier in the same frame
  are protected, so a ten-card page cannot cycle through and evict all eight.
  Dynamic rich text, clipping, modulation, unusually wide costs and oversized
  keys fall back to normal drawing.
- The focused hand outline derives from the actual 69 × 97 card face and
  follows the same pose scale and rotation. Geometry is checked at 0.5×,
  0.65625× and 1×, plus a 90-degree rotation.

The synthetic card/HUD test passed 385,178 checks. The same test passed 329,691
checks on Chinese resources and 318,361 on English resources. Its full-surface
oracle compares cached and uncached production rendering on eight/ten-card
loads, varied backgrounds, costs, stars, animations, clipping and tint. The
existing Canvas regression also passed 159,808 cases and 319,758 checks.

Private side-by-side captures show zero changed pixels on the entire library
screen in both languages. Removal-screen differences include the intended
replacement of literal markup/template fields with the correctly formatted
localized heading. The choice captures also include the concurrent HUD change;
they are not claimed to be pixel-identical to the old release.

The host cache symbol is 130,944 bytes plus a four-byte frame counter, below its
130 KiB compile-time ceiling. This historical host measurement did not establish
integrated ARM allocation. The final `61970600` link separately verified
**16,507,640 bytes**, leaving **269,576 bytes** below 16 MiB.

## Full startup validation remains enabled

`resource_open_benchmark` opens each actual pack in a new process and measures
the existing progress phases. This is a host stage breakdown with warm OS
pages, not a flash-storage cold-start measurement. It does not bypass a check.

| Stage | Chinese ms | English ms |
| --- | ---: | ---: |
| Complete open | 766.178 | 722.788 |
| Header | 0.007 | 0.006 |
| Full-file integrity | 123.852 | 106.007 |
| Directory | 0.507 | 0.226 |
| Retained image validation | 62.986 | 59.566 |
| Tile-index validation | 577.504 | 556.592 |
| Glyph validation | 0.990 | 0.040 |
| String and final directory checks | 0.266 | 0.316 |

Full-file FNV accounts for **16.2%** of the observed Chinese open and **14.7%**
of English. Tile-index validation accounts for **75.4%** and **77.0%**. The complete
open reads 117,346,301 and 111,875,809 callback bytes respectively, including the
full 81,124,876 / 78,349,198-byte integrity ranges after their 40-byte headers.

These are the historical `069f6d51` host observations. On that host, removing
only the full-file hash could not account for the largest measured stage.
The later target observation below changes the priority: the hash is the
largest single target stage. The different resource versions, execution
environments and cache histories do not form a startup A/B comparison. No
validation policy was changed in either measurement.

## Earlier target startup observation and validation decision at `61970600`

**Decision for the current resource format, retained by `06c7fd16`: retain the full-body FNV check,
all startup structure checks, and the existing first-use decoder checks.
Prioritize removing repeated reads and repeated traversal while keeping their
coverage.** This is an evaluated decision, not an assumption that validation
is free or that it has already been disabled. The observed approximately
50-second selected-resource initialization remains a performance problem;
visible progress and this assessment do not constitute a startup speedup.
The bounded-metadata follow-up is now implemented and measured separately
above; the rest of this section preserves the earlier target assessment.

### Final target sample and its limits

The Chinese split-resource invocation used frozen checkpoint
`61970600a8d3f7fb2833af13f82f09c86105ebec`, with the same v18 virtual stream as
`feedback-zhs.nsp`: 91,439,646 bytes, SHA-256
`74279728406d3362993ef82e92c5ee9196bb3d4a84d3ede7bfc9bb081914997b`.
The foreground SP804 timer reports 32,768 ticks per simulated second. Each
startup phase has one sample:

| Measured target scope | Timer ticks | Simulated seconds | Share of startup |
| --- | ---: | ---: | ---: |
| Selected-resource validation and App initialization | 1,653,887 | 50.472626 | 100.00% |
| Header | 121 | 0.003693 | 0.01% |
| Full-body integrity | 755,492 | 23.055786 | 45.68% |
| Directory | 7,215 | 0.220184 | 0.44% |
| Retained image validation | 408,393 | 12.463165 | 24.69% |
| Tile-index validation | 438,325 | 13.376617 | 26.50% |
| Glyph validation | 28,527 | 0.870575 | 1.72% |
| Strings and final directory checks | 3,749 | 0.114410 | 0.23% |
| Ready / remaining App initialization | 6,080 | 0.185547 | 0.37% |
| Startup work outside the named phase intervals | 5,985 | 0.182648 | 0.36% |

The three large phases account for 96.88% of this sample. The first render is
separate: 13,494 ticks / 0.411804 seconds. The timing begins after the language
selection, immediately before `file_source` and `SplitResourceSource::open`,
and ends after `App::init` and any language-preference write. It excludes
Ndless/Zehn loading, the user's language-selection time and first render.
Phase intervals include the work and progress drawing performed between the
corresponding callbacks; they are not pure CPU or filesystem timings.

This is a Firebird invocation from the restored OS/installer snapshot, not a
physical-device cold boot. The resource files had previously been written by
USB and host caches were not flushed. Neither this one sample nor its similar
predecessor establishes a cold-start distribution or an A/B improvement.

The exported profile `first-zhs-usb.perf.json` is 4,326 bytes, SHA-256
`23a9d61091dc3ec4c3b28a815bbbd69480ec6a7d5b972e92cc9644612dd15ad1`.
Its parsed JSON equals the independently recovered flash record; the files
differ by a trailing newline. It reports zero resource-read failures. Its
invocation-wide counters are 10,133 callbacks, 143,205,501 requested bytes and
6,283 seeks, but include work after initialization. **There is no target
per-phase I/O-byte/call or CPU breakdown.** Those totals cannot be assigned to
the integrity, image or index phases, nor treated as startup-only counters.

The original `final-firebird-runtime-d1c7b32eed5d.json` remains a failed journal
(exit 4) because two stalled USB requests were cancelled. The profile was
subsequently exported after reconnecting and checking the directory. Its
verified timing sample is usable; it does not turn that complete command into
a passed runtime gate. This distinction is retained in the platform owner's
separate execution receipts.

### What the current checks actually cover

The following review refers to the frozen `61970600` source, rather than
assuming every image has a checksum:

| Layer and implementation | Actual coverage | What it does not replace |
| --- | --- | --- |
| Split adapter, `SplitResourceSource::open` in `src/resource_source.cpp` | Checks container magic, revision, sizes, split boundaries, header FNV values, equal common-identity fields and the actual language marker. | The 32-byte common SHA-256 identity is compared between headers; the runtime does **not** recompute the common payload's SHA-256 here. This small adapter check is not a payload scan or trusted persistent-validation cache. |
| Full-body FNV, `Content::open` and `ResourceReader::fnv1a_buffered` | Checks every virtual NSP2 byte in `[40, length)` against the stored 32-bit FNV-1a value, including directories, compressed art, glyphs and strings. The first 40 header bytes are checked separately by their fields. Uses the existing encoded scratch in 64 KiB chunks. | A consistent checksum does not prove valid dimensions, references, decoded geometry or supported codecs. FNV is an accidental-corruption/consistency check, not an authenticity signature. |
| Header and directory validation in `Content::open`, `tile_header_bounds`, `valid_tile_directory` | Bounds counts, lengths, offsets, alignment, formats, image dimensions, cache limits, clip ranges/timing, card/UI references, glyph ordering and string terminators. | Plausible dimensions and a terminating zero do not prove that a text character, glyph mask or color byte is the intended byte. |
| Retained image validation in `Content::image`, `decode_lz4`, `valid_image` | At startup, every distinct retained descriptor is decoded/read, then checked for exact raw length or bounded, ordered sparse spans. LZ4 checks input/output bounds, back-reference validity and exact output size. | **There is no per-retained-image CRC in this path.** A changed color/literal that still produces a structurally valid picture can pass the decoder and geometry checks. The actual v18 Chinese directory has 4,994 format-5 LZ4 entries, representing 4,925 distinct full descriptors. |
| Startup tile catalog, `validate_tile_catalog` | Checks every tile-group header; decodes every index group; verifies record boundaries, tile IDs, index/geometry sizes and every descriptor's actual image shape. LZMA2 index decoding also checks CRC32 of the decoded group. | Startup does not decompress every tile-pixel group. Their encoded bytes are covered by the whole-stream FNV, but their decoded CRC and pixel-record constraints are deliberately deferred. |
| First-use tile pixels, `load_tile_group`, `decode_lzma2_block`, `rebuild_tile_frame` | On a cache miss, checks LZMA2 completion/end marker, exact consumed/produced sizes and decoded-group CRC32; checks tile dimensions and zero padding. First reconstruction validates all tiles belonging to that frame, including edge dimensions; span assembly checks geometry, opaque-alpha requirements and exact copied size. | These are group/first-use checks, not a checksum on every image, string or glyph. An unread tile group is not fully decoded merely because startup succeeded. |

For glyphs, startup checks offsets/dimensions and, for encoded RLE masks,
nonzero run lengths and the exact total number of pixels. Raw mask values and
RLE coverage values have no separate payload CRC. Strings have bounds and a
terminal-zero check, not a checksum or complete UTF-8/prose validation. These
facts are why removing the complete FNV scan would reduce coverage even if all
decoder and structural checks remained enabled.

Existing validation reuse must also be preserved. Startup deduplicates
**identical full retained descriptors**, not just payload offsets; a different
shape must receive its own check. The tile catalog groups descriptors by index
group when workspace permits. Successful groups and images are cached within
one open operation, and the per-image validation bitmap avoids repeating
inactive-tile checks on subsequent reconstructions. All of these caches are
cleared when the owner is reopened or fails. Adding another indiscriminate
"already checked" flag would not be a new safe optimization.

### Repeated work and the implementation priority

There is definite repeated **I/O**, even though the checks answer different
questions. The initial 64 KiB FNV scan bypasses the 4 KiB reader-page cache and
does not populate decoded image/group caches. The image stage later rereads
retained encoded records, and the tile stage rereads index streams and group
headers. Their decompression/shape work is not performed by FNV. Conversely,
the payload integrity scan is not reproduced by checking their dimensions.

The fixed pack directory contains 732 tile-pixel groups and 452 index groups.
The index groups occupy 6,493,413 stored bytes; the 4,925 distinct retained
descriptors refer to 26,186,120 stored bytes. These are layout counts, **not
measured target callback totals**. They identify substantial records that the
current scan visits before the subsequent validators read them again. Paging,
aliases, short header reads and split boundaries affect the actual callbacks.

**The next implementation with the broadest direct opportunity is one ordered
validation read plan:** feed each stored byte to the existing FNV in its
original order while reusing those encoded bytes for the same retained-image,
index and header validators. Keep tile-pixel decompression deferred exactly as
today. Do not publish a usable `Content` until both the complete final FNV and
all the existing startup structural checks succeed. Repeated aliases still
need every distinct shape checked. The shared encoded scratch, current image/
tile pools and unused directory workspace bound the work; no complete-pack RAM
copy or larger permanent cache is justified by the 16 MiB target budget.

This requires an explicit ordered plan: simply updating the hash on every
`read()` would count repeated ranges, omit unread bytes and produce the wrong
FNV. Header/metadata bounds must be established before scheduling payload work,
large records must span chunks safely, and rejection must clear any temporary
cache state. Progress accounting must remain honest when work is combined.
The optimization is proposed, **not implemented or benchmarked in this review**.
It can remove duplicate storage reads while retaining coverage; it cannot
remove the cost of hashing every byte or of the required decompression. The
present telemetry cannot predict its elapsed-time benefit.

A smaller follow-up can retain descriptor rows and bounded record summaries
inside the existing directory workspace during catalog validation. The
current path reads descriptor rows more than once, `tile_record_at` scans from
the beginning of a variable-size index group for each lookup, and generic
geometry validation is followed by shape-specific geometry walks. A summary
created during the validated parse can retain exact record boundaries, row
counts and maximum span extent, allowing the subsequent width/height checks
without walking the same geometry again. It must still validate unreferenced
records, exact output sizes, all tile IDs, conflicting-shape aliases and the
capacity-exhaustion fallback.

For this particular directory, 462,560 descriptor bytes plus 233,088 bytes of
the existing grouped-validation workspace fit in its 953,513-byte available
tail after the current page/validation tables. That is a bounded source-layout
opportunity, not a measured speedup or permission to grow total RAM. Other
legal packs still require the existing bounded fallback. The already present
exact-descriptor deduplication, grouped index validation, 64 KiB scan and
table-driven CRC32 should not be presented as newly completed work.

### Alternatives considered

| Option | Effect on current coverage and cost | Decision |
| --- | --- | --- |
| Keep current checks unchanged | Preserves early whole-stream consistency, structural rejection and first-use decoded checks; retains the observed startup cost. | Current release policy. The cost is acknowledged, not dismissed. |
| Reuse one ordered read stream and bounded parsed metadata | Preserves every current check while eliminating duplicate storage reads and traversals. Does not avoid hashing/decompression themselves. | Highest-priority implementation direction; needs actual same-input target A/B before a speed claim. |
| Disable only the complete FNV | Removes the observed 23.056-second phase but loses coverage for otherwise valid damaged LZ4/raw content, glyph values and text. Subtracting it gives 27.417 seconds of other recorded work, not a measured optimized startup. | Do not make this the default for the current format; decoder CRCs are not equivalent coverage. No toggle was added. |
| Defer retained-image/index validation until use, or hash while gameplay is already active | Moves work and failures to later scenes and changes when invalid packs are rejected. Tile-pixel decompression is already deferred. A background pass still consumes the same target CPU/I/O budget. | Not an equivalent no-weakening change. It needs an explicit first-use/metadata contract and scene-latency assessment, rather than silently removing a startup gate. |
| Persist a validation result keyed only by pathname, size, timestamp or declared common identity | May skip work, but those fields do not establish that the current bytes equal the previously validated bytes. The existing common SHA field is only a stored label until the payload is verified. | Insufficient basis for a trusted skip. Not adopted. |
| A cache bound to independently trusted immutable content identity and validator/version identity | Could reuse structural results for proven identical bytes. A fresh strong digest computed at launch still requires a complete read; it may avoid repeated decode/geometry work, not the scan itself. Avoiding that read requires a reliable immutable/versioned file guarantee that the current `ResourceSource` interface does not provide. | Possible future design, with invalidation and rejection semantics specified first. No trusted persistent cache or device saving has been demonstrated. |

This decision is specific to today's format and available evidence. A future
resource format that covers every payload type with independently verified
chunks could support a different startup policy, but the current LZMA2 CRCs
cannot be assumed to provide that format-wide coverage. No new benchmark,
validation bypass, caching policy or source change was made for this assessment.

### Audit source identity

The target code review uses the saved `61970600` source inventory
`575955810f28f5c6112d497f8b377cd06507941bb0264b441ac0476c509a8529`.
The relevant file SHA-256 values are:

| Source file | SHA-256 |
| --- | --- |
| `src/content.cpp` | `f4e9d6161fb6064807c65b2b00f765d5e4f00f76802b6530cd4d2b086ac766b6` |
| `src/content_tiles.inc` | `c44e99330d4a300d8ea47ac09f183f6b1e7620f43729f4a06ae32bf8245c0a29` |
| `src/resource_source.cpp` | `aa2ed95060f76c6e41e68e9573dacf180638f838644a748f342afc4bc554f8b1` |
| `include/lz4_block.hpp` | `5485710a26f069cf6b9ed5da7416ca26874334da6dbe902b58a8817642228494` |
| `src/lzma2_block.cpp` | `f6fc844087686340880bd43fc1aacd03ca4bff763895739f6bbb05d765e0dca6` |
| `platform/ndless/main.cpp` | `dfdcac19ef9d8ff3684ea0016053426dc1344c4fc5e6218e07407d5aef586500` |

## Evidence and remaining integration

[Machine-readable results and closed journal receipts](validation-rendering-0.17.json)
record input identities, driver/library hashes, all warm-window observations,
startup phases and source-stability checks. The accepted synthetic receipt is
`perf-card-pixel-final-b44e32276ac9.json`: an earlier parallel invocation's
journal remained `running` on readback despite printing completion, so it was
not accepted and the identical command was rerun sequentially. All thirteen
accepted receipts are closed `passed`, exit 0, with unchanged frozen source.

The English event-reader scroll helper was separately diagnosed as counting
character wraps instead of complete-word wraps (Baths: 12 versus 13 lines).
Its follow-up keeps the original scroll assertions and is not relabeled as
part of this candidate's execution. Later feedback resources, split-container
hosts and the ARM allocation gate retain their own integration evidence. The
target sample above is a separate observation and does not change any of the
historical host measurements or establish physical-device performance.
