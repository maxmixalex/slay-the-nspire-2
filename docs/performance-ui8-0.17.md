# UI8 performance and storage — 2026-10-24

This work preserves original UI assets, decoded pixels, animation frames and
map generation order. It changes the map workspace and lookup costs, image
and text caches, scaled sampling, serialization staging, and lossless resource
storage. The later NSP2/33 conversion saves **8,756,489 bytes in the English
full pack and 10,419,445 bytes in Chinese** against the complete NSP2/31 input,
despite adding the original card-resource cross and 24 missing Chinese title
glyphs. These per-language reductions must not be added to calculate a
bilingual installation: most art is shared. The owner's 30–40 MB installation
goal remains open.

Long commands ran from pushed, immutable source snapshots with
`tools/run_logged.py`. Private receipts retain the exact command, source hashes,
start/end times, return code and output hash. Names below identify those
receipts without publishing original-derived packs or captures.

## Stage 8e map sampling, final resources and ARM budget

The rotated map-icon inverse is affine along each scanline. It now prepares
the exact integer quotient and a nonnegative remainder, then advances both
with addition and a carry. Off-image negative coordinates use floor correction;
the original bounds check, pixel-center convention, palette, alpha, shading
and viewport clipping remain unchanged. This removes the two software divisions
from each covered rotated pixel without adding a framebuffer or resident pool.
The unrotated paper already had a separate axis table and is unchanged.

The old direct inverse remains an independent pixel oracle. The expanded
renderer regression covers negative coordinates, positive/negative rotations,
odd dimensions, opacity, palettes, coverage and clipping. Formal stage-8e
native rendering also compares the old stage-8c2 binary with the new binary on
the **same** NSP2/33 resources: **3,246 frame hashes and 24 complete PPMs per
language match exactly** across all six scenes, and canonical State bytes do
not change. All third 180-frame windows have zero reads and zero requested
bytes. The quiet native map pair is 1.783 to 1.832 ms in English and 1.781 to
1.735 ms in Chinese; that small mixed variation is not claimed as a host CPU
speedup. Receipt: `map-rotation-whole-frame-comparison.json`.

Actual ARM `-O2` disassembly confirms two step divisions per picture and two
coordinate divisions per row, with none in the rotated pixel loop. The function
grows from 2,044 to 2,400 bytes; stack use increases only 768 to 784 bytes.
An additional short linked check verifies 9,110 signed 32-bit quotient/remainder
and AAPCS vectors, including negative map starts. All existing 12,880 64-bit
division, 42 LZMA2 and 14 tile-filter linked vectors also pass.

The ordinary Firebird comparison uses the same actual English save, OS boot,
Ndless installer, Continue, map-open delay and 20-second idle window:

| SP804 map measurement | Stage 8d | Stage 8e |
| --- | ---: | ---: |
| Rendered map frames | 250 | 343 |
| Mean render time | 89.892 ms | 65.256 ms |
| Minimum render time | 78.979 ms | 54.382 ms |
| Resource reads / requested bytes | 24 / 137,854 | 25 / 141,950 |
| Resource failures | 0 | 0 |

Mean rendering falls **27.41%**, and the minimum falls **31.14%**. The actual
7,456-byte NSZ1 save is byte-identical after both runs. These are Firebird ARM
SP804 measurements, not physical-calculator FPS. The scene aggregates include
cold opening and fades; the final resource set appends tutorial assets, and
the extra read is not attributed to an unrecorded offset. Every measured map
frame still exceeds 50 ms, so further map optimization remains warranted.
Receipt: `map-comparison-receipt.json`, ordinary gate
`ui9-firebird-stage8e-map-94534a265b38.json`.

The final tutorial append retains all prior payloads and produces 2,058
auxiliaries, 3,189 English glyphs and 31,689 Chinese glyphs, below the 32,768
directory limit. Both reconstructed full packs match the split files exactly:

| Final resource part | Bytes |
| --- | ---: |
| Shared data | 84,772,852 |
| English sidecar | 2,160,488 |
| Chinese sidecar | 5,184,978 |
| All three resource files | **92,118,318** |
| Stage-8e launcher | 1,370,912 |
| Installed bilingual program and resources | **93,489,230** |

Stage 8e (`492915341106b129f730e46ed84d152637a24b7d`) allocates
**16,672,244 bytes**, leaving **104,972 bytes** below the unchanged 16 MiB
limit. Ordinary USB transfer installed these exact file lengths and downloaded
the launcher back with the build SHA-256. Replacing the old Chinese sidecar
in place first failed with USB status -255; deleting only that obsolete resource
and transferring again succeeded. The failed journal is preserved. Upgrades
should remove obsolete resource files before copying the new sidecars, while
preserving profile/save files. Fresh ordinary startup and the map gate then
passed independently.

The full NSH8 history fixture is 295,349 bytes. With the valid dense NSZ1 run,
the three-profile history-update peak is 1,910,754 bytes, or 2,305,029 bytes
when reserving the conservative raw-backup migration bound. Those are file-byte
budgets; ordinary filesystem peak installation and restoration remain to be
checked with the final launcher, and the dense compressed fixture is not a universal upper bound
on every possible valid compressed run.

## Stage 8c/8d actual resources and installation

The completed pre-FTUE NSP2/33 set reconstructs byte-for-byte from its split
files. These are actual emitted files, before the later FTUE resource append:

| Part | Bytes |
| --- | ---: |
| Shared data | 84,719,566 |
| English sidecar | 2,153,050 |
| Chinese sidecar | 5,149,970 |
| Both languages and shared data | **92,022,586** |
| Stage-8c2 ordinary launcher | 1,350,761 |
| Installed pre-FTUE bilingual program and resources | **93,373,347** |

The original art, frame indices, frame counts, timing, alpha and hidden RGB565
values are retained. The production changes are purpose-grouped retained art,
32 KiB grouped glyphs, 32 KiB grouped strings, and reversible TGR2 color/layout
filtering before the existing LZMA2 decoder. None adds a resident pixel pool
or increases the image/group cache capacity. See `grouped-glyphs-0.17.md`,
`grouped-strings-0.17.md` and `tile-filter-0.17.md` for format bounds.

The TGR conversion keeps 728 smaller replacements and leaves four non-saving
groups in their original TGR1 form. It saves **3,733,886 common bytes**. Each
language's receipt proves **443,044,043 decoded tile/index bytes exact**;
all 452 index streams and retained-art streams remain byte-exact. All old
glyph payloads and all 8,704 strings are also compared. The later 24 Chinese
title glyphs add 1,608 bytes without changing an existing key or payload.
Production receipt: `ui9-tile-filter-production-fba3cf355336.json`.

Ordinary Firebird USB transfer installed all three sidecars and the production
launcher into a disposable copy of the owner's original OS. The original NAND
was not changed. The OS showed 92.3 MB total and 2.4 MB remaining after the
stage-8c2 program; the one-decimal display is not an exact free-byte count.
The launcher was downloaded back through ordinary USB and matched its build
SHA-256. This establishes the actual bilingual installation, but its remaining
space alone did not establish worst-case three-profile save/update capacity.

## Actual cache-key working set and budget

Traced history windows needed 1,249 glyph keys for only about 62 KiB of glyph
payload, and 932 string keys for about 10 KiB of text. The earlier 512/256 key
limits evicted useful entries while payload space remained unused. The cache
now provides 1,536 glyph keys with 163 KiB payload, and 1,024 string keys with
102 KiB payload. String pages change from 128 to 64 and hash buckets from 64
to 256. Total ARM allocation for these caches falls **331,332 to 324,132
bytes**, a **7,200-byte reduction**. The image and retained-group pools are
unchanged.

The actual English/Chinese history traces each compare 2,160 complete rendered
frames, with canonical State bytes unchanged. The dense Chinese scroll window
falls from **6,958 reads / 28,677,506 requested bytes to zero**, and the
interleaved window from **3,470 / 14,275,970 to zero**. All three warm windows
in both locales have zero resource reads. The receipt is
`history-cache-exact-comparison.json`; the recorded candidate logs are
`ui9-history-cache-candidate-eng-e0161baa8d04.json` and
`ui9-history-cache-candidate-zhs-758c03f97d27.json`. These concurrent trace
runs establish read/pixel behavior, not a trustworthy CPU speed percentage.

## NSP2/32 to NSP2/33 cold and warm rendering

The final stage-8c2 native binary at
`a0df79d8347b76dc86804b0ab8d17f948c600204` rendered both the preserved NSP2/32
packs and final NSP2/33 packs sequentially while other builds, conversions and
emulation were paused. Each locale records **3,246 frame hashes and 24 PPM
captures**, all byte-identical between packs. The map, merchant, card reward,
covered Crystal Sphere, revealed Crystal Sphere and Ancient fixtures preserve
canonical State bytes. Every third 180-frame window has **zero reads and zero
requested bytes**, before and after conversion.

This comparison covers the complete NSP2/33 change: grouped strings, tile
filtering and the small original-art/glyph additions. It is not an isolated
TGR2 timing test. Three further alternating-order runs measured cold
first-render/preparation medians with the resource cache reset by each scene;
the operating system page cache was not flushed:

| Scene | English NSP32 → NSP33, ms | Chinese NSP32 → NSP33, ms |
| --- | ---: | ---: |
| Map | 13.798 → 16.312 | 35.402 → 39.368 |
| Merchant | 29.496 → 31.962 | 29.678 → 30.358 |
| Card reward | 16.634 → 18.889 | 23.456 → 26.627 |
| Crystal, covered | 21.465 → 22.387 | 26.994 → 29.447 |
| Crystal, revealed | 22.282 → 24.602 | 27.468 → 32.987 |
| Ancient | 12.092 → 13.669 | 17.542 → 19.370 |

The cold CPU cost increases by about **0.68–5.52 ms** in these cases. Cold
requests increase by 3–11 reads and 19,768–70,470 bytes because the comparison also
introduces grouped string payloads; those totals cannot identify the cost of
tile restoration alone. Warm read behavior and pixels remain unchanged.
No warm CPU percentage or physical-device FPS is inferred from these samples.
Receipts: `ui9-room-v32-eng-1c0450af1bb6.json`,
`ui9-room-v33-eng-6601e15c9212.json`,
`ui9-room-v32-zhs-ac830cab8af6.json`,
`ui9-room-v33-zhs-5df20e77ceae.json` and
`ui9-room-cold-repeat-b51029e826ab.json`.

The production ARM `-O2` disassembly then exposed an invariant mode comparison
inside every filtered tile pixel. Stage 8d dispatches once per group into two
compile-time modes. The linked inverse retains exact output across every
RGB565 value, alpha, dimensions, maximum 680-tile groups and malformed bounds.
Its text grows 404 to 676 bytes, while stack use falls 52 to 48 bytes; the
linear and zigzag loops contain 23 and 27 instructions instead of 26 and 34.
These are instruction counts, not measured device-cycle savings.

## Ordinary ARM flow and lossless run-file storage

The stage-8c2 ordinary Firebird flow reached the Chinese title without a
prelaunch language chooser, changed to English in Settings, selected Standard
Ironclad, took Golden Pearl from Neow, traversed the map, manually won the
first fight in five turns, collected 11 gold and Twin Slam, then used Save and
Quit and Continue. HP 69/80, gold 260 and deck size 11 survived. The optional
ordinary performance marker was used only in the disposable test clone and
is excluded from delivery. All 402 resource requests succeeded.

The emulated ARM SP804 report includes cold loads and interaction: 1,407 combat
frames average 16.107 ms of rendering; 729 room frames average 24.710 ms;
79 map frames average 79.467 ms; and 238 title frames average 36.836 ms.
These are emulator measurements, not physical-calculator FPS. The map's
minimum recorded render is about 56.85 ms, so its cost cannot be explained
solely by one cold frame. A separate map sampling optimization is recorded
in the later stage-8e section above, with its direct inverse-transform
oracle and ordinary ARM comparison preserved.

The canonical run remains NSV1 save version 17. Stage 8d adds a lossless NSZ1
file envelope only in calculator file I/O, retaining raw input support and
the existing atomic primary/backup rotation. It uses a 512-byte stack block;
it does not allocate another State or static pool. A raw fallback is used
when compression would not reduce size.

The real ordinary run's **230,540-byte** raw save continued successfully in
stage 8d, then saved as **7,456 bytes**. Its first backup remained the exact
old raw file. Decoding both downloaded files reproduces every original byte.
The program was launched again and actually continued from NSZ1; after another
normal save, both primary and backup were 7,456-byte, byte-identical NSZ1 files.
One long-lived USB session timed out on its later download. That failed log
is retained; after flushing and rebooting the ordinary OS, a new USB session
downloaded the files and launcher successfully. No save contents were repaired
or substituted to recover that transfer.

The more demanding valid dense-state fixture compresses **250,964 to 121,503
bytes**, rather than relying on the mostly-zero ordinary example. Full history
is **295,347 bytes**. The tested three-profile peak with dense compressed
primary/backup runs and a full atomic history update is **1,910,746 bytes**;
three compressed primaries plus three bounded old raw backups during migration
are bounded by **2,305,021 bytes**. Final resource additions and ordinary OS
space must still be checked against those peaks.

Stage 8d (`d938a661c7a69e46b829a4a879ce6cd411be0f2b`) produces a
**1,352,081-byte launcher**, allocation **16,639,308 bytes**, and **137,908
bytes headroom** below the unchanged 16 MiB gate. Its 12,880 division, 42
LZMA2 and 14 filtered-tile linked ARM vectors pass. Successful receipts are
`ui9-stage8d-arm-7508b890e38a.json`,
`ui9-stage8d-arm-division-d6d6e5fdd401.json`,
`ui9-stage8d-arm-lzma-6ea2d61000c2.json` and
`ui9-stage8d-arm-tiles-6ff18d2026b0.json`.

The remaining sections record the earlier implemented steps and their
historical checkpoint measurements.

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

## Shared serialization and resource-transfer storage

Stage six removes the separate **200,368-byte State staging
array** used by the state codec. The existing 617,323-byte encoded-resource
capacity can also hold a `SaveData` prefix followed by aligned State storage;
the union adds at most alignment padding, not a second large allocation.
Image, retained-group and tile-group cache capacities remain unchanged.
The actual ARM symbol comparison removes 200,368 bytes after accounting for
the old codec pointer/busy flag, new lease flag and four added alignment bytes.
The shared union is 617,328 bytes; its payload capacity stays 617,323 bytes.

Every user of this storage now holds a bounded foreground lease:

| Caller | Lifetime and overlap |
| --- | --- |
| Ndless save/load | A Save lease covers reading/writing and atomic rename; one codec can use its disjoint State suffix. |
| Ndless history and profile summaries | An exclusive Bytes lease covers the entire read/decode or encode/write operation, including backup fallback. |
| Cold pictures and coalesced image preparation | An exclusive Bytes lease covers all range callbacks, decoding and publication; tiled images retain it across their index and tile groups. |
| X11 and browser save/load | Their existing host buffers remain independent; a codec borrows the shared State suffix only for its synchronous call. |
| Browser history/preferences/feedback handoff | Existing staging remains separate because JavaScript may consume it after the WebAssembly call returns. |

A nested codec or cold image request cannot overwrite active encoded or save
bytes. A busy image request returns an empty, retryable view without declaring
the resource corrupt; already published decoded pictures remain readable.
Every return path releases its lease. The codec constructs a fresh State
lifetime after an image/history overwrite, preserving nonzero defaults in
historical saves. It still replaces the caller's State only after full decode
and validation. The save format, file atomicity and legacy acceptance are
unchanged; this is not a thread-safe API or an asynchronous lifetime extension.

The new regression cases invoke nested saves, loads and cold-image requests
from a real range-reader callback after encoded data was copied; check retained
pixels, untouched output buffers and live state; exercise failed I/O recovery;
verify prefix/suffix separation during Ndless-style save/load; and overwrite
the entire transfer union between historical codec operations. Stage-six
`state_codec_regression` passed. The retained-reader test initially failed
because its tiny tile payload shared an already warm descriptor page, so the
required reentrant callback was never invoked. Moving that synthetic payload
to a separate cold page makes the intended nested operation happen. The
corrected test, compiled with the frozen stage-six headers and archive, passed
473,283 checks. Its complete checkpoint gate remains part of stage seven.

## Repeated room rendering and conservative merchant occlusion

Frozen stage six (`f937cf13374ec5ba2e010f8c8899b691600565f0`) and its actual
English/Chinese packs passed six room fixtures, each with three consecutive
180-frame windows at 33 ms animation steps. All canonical saves were unchanged
and no save callback was attempted. Every room except the merchant performed
zero source reads in windows two and three. The measured host means were:

| Scene | English steady windows | Chinese steady windows |
| --- | ---: | ---: |
| Map | 1.848–1.853 ms | 1.824–1.831 ms |
| Card reward | 0.681–0.736 ms | 0.627–0.658 ms |
| Crystal, full/revealed | 1.271–1.437 ms | 1.174–1.299 ms |
| Ancient | 0.668–0.683 ms | 0.630–0.649 ms |
| Merchant | 1.595–1.640 ms | 1.513–1.515 ms |

These are unpaced host intervals from one quiet sequential pass, not device
FPS or an A/B speedup. Captures are emitted outside the timed intervals.
Receipts: `ui8-room-render-real-eng-f2c268a08437.json` and
`ui8-room-render-real-zhs-3e3d2fb452fc.json`.

Merchant windows two and three still read 862,809 and 744,197 bytes, in 55 and
40 calls respectively, identically in both languages. A separate longer trace
used three 720-frame windows. Every one of the final two windows' 166/169
reads revisited an already seen retained group; those diagnostic timings are
not used as performance measurements. The five room clips span 6,481,363
decoded bytes in 55 retained pages, exceeding the unchanged 40-page group
pool. The source is animation working-set churn, not Chinese glyph traffic.
Trace receipt: `ui8-room-render-trace-eng-2c43af282c0f.json`.

The stage-seven candidate addresses the hidden work directly. Once the
inventory opening tweens finish, it reads each lower clip's dimensions from
resident metadata and checks every visible destination alpha sample of the
original rug. Only a whole rectangle covered by alpha 255 can skip its lower
frame. Any translucent edge, hole, unsupported sparse cover, opening, closing
or interrupted tween uses the full original rendering path. No frame is
removed from the pack and no cache budget changes. A new independent alpha
sampling test and complete merchant covered/unculled frame comparison cover
five character cycles and opening/closing transitions.

Frozen stage seven (`e9aaac1c252d38d9b4b773d9b9e549ab468c72c4`) passed
**41,589 checks and 2,376 exact full frames per language**, using the same
original v28 rug/clip resources accepted by the v29-compatible loader. The
reference branch requests every original lower layer. All five character
cycles, opening/closing/interruption, 32-bit clock boundaries, hit regions and
canonical saves matched. A separate cold-cache assertion proves all five
fully hidden character frames avoid decoding. Receipts:
`ui8-merchant-occlusion-eng-v28-1b0a16c7b7b4.json` and
`ui8-merchant-occlusion-zhs-v28-b740f67dcc5e.json`.

A production range-reader run with three 720-frame windows then reduced the
merchant's final two windows to **zero source calls, bytes and seeks**. The
same v28 baseline had 166/169 calls and 3,090,210/3,172,373 bytes. All three
merchant window-end frame hashes also match the baseline, and saves remain
unchanged. That run was not isolated from other work, so its timings are not
used for a CPU speedup claim. Receipt:
`ui8-room-occlusion-io-eng-v28-947c82cb84d4.json`. A quiet CPU comparison and
the final matching v29 resource set were then tested separately.

After the other builds and converters ended, one sequential baseline/candidate
pair per language used that same v28 pack and two steady 180-frame windows:

| Merchant | Stage six | Stage seven | Mean time reduction |
| --- | ---: | ---: | ---: |
| English | 1.704048 ms | 1.214242 ms | 28.74% |
| Chinese | 1.537949 ms | 1.231255 ms | 19.94% |

These are quiet shared-host measurements, not ARM frame rates or repeated-run
statistical estimates. All four window-end frame hashes match their baseline.
The English candidate makes no source reads in either measured window; Chinese
reads one 4,096-byte page in window two and none in window three. The baseline
performs 55 and 40 reads in each language. Exact outputs are retained in
`merchant-quiet-comparison.json` and receipts
`ui8-rooms-quiet-baseline-eng-v28-72c8607c54e1.json`,
`ui8-rooms-quiet-occlusion-eng-v28-f646260edf70.json`,
`ui8-rooms-quiet-baseline-zhs-v28-e600d1d8775e.json` and
`ui8-rooms-quiet-occlusion-zhs-v28-169a2448678b.json`.

The matching final v29 packs, including all 1,005 newly baked character death
frames, also passed all six room fixtures in both languages. Every scene makes
**zero source reads, bytes and seeks in its third 180-frame window**, with
unchanged canonical saves and no save callbacks. The measured third-window
means are:

| Scene | English | Chinese |
| --- | ---: | ---: |
| Map | 1.796836 ms | 1.716024 ms |
| Merchant | 1.256786 ms | 1.338539 ms |
| Card reward | 0.655312 ms | 0.640821 ms |
| Crystal, full | 1.533636 ms | 1.207597 ms |
| Crystal, revealed | 1.342181 ms | 1.221099 ms |
| Ancient | 0.727189 ms | 0.746982 ms |

Receipts: `ui9-room-render-real-eng-71587f70d1e1.json` and
`ui9-room-render-real-zhs-10a287743292.json`. Each run also emitted 24 PPM
captures outside the timed regions. The private `final-v29-contact.png` was
visually inspected for the map, merchant, card reward, both Crystal states and
Ancient in both languages. Later changes still require their matching final
gate; these results describe frozen stage seven.

## Crystal coordinate work and actual resource measurements

The stage-four ARM disassembly shows software integer division inside the
Crystal Sphere's invariant axis preparation: its four projected axis lengths
are 158, 157, 159 and 160 pixels, with three dynamic divisions per coordinate.
The stage-five candidate evaluates that exact integer mapping at compile time.
Four immutable tables contain **634 four-byte records, 2,536 bytes total**.
This removes **1,902 coordinate divisions per two-layer frame** and the former
1,280-byte local axis arrays. The two unsigned 64-bit clock phase calculations
are also shared by the layers. Pixel colors, polar/noise lookup, coverage,
border blending, cell timing and the number of rendered frames are unchanged.
The actual stage-five ARM shader body is **2,080 bytes**, down from 2,864;
its total local stack use is **400 bytes**, down from 1,648. No software
division helper call remains in that shader body. The four linked tables
occupy exactly 2,536 bytes. These changes preserve the formula; they do not
by themselves establish a frame-rate improvement.

`crystal_shader_regression` compares the axes against independent floor-based
arithmetic and the clock phases against 128-bit arithmetic. With an actual
resource pack, it compares the complete 320×240 buffers after each of both
layers over 192 clock/coverage/opacity cases, including 32-bit clock wrap, then
times the preserved stage-four software shader against the candidate. Frozen
stage-five binaries with actual stage-five-c English and Chinese font/resource
packs passed **7,439 checks per locale**, including all **384 whole-frame
layer comparisons per locale**. Their reference is the preceding port renderer,
not Godot. The two successful receipts are
`ui8-crystal-shader-real-eng-c69aa843cf6b.json` and
`ui8-crystal-shader-real-zhs-e6e9e9bd42d6.json`.

The single, unpaced shared-host timing pair for 360 two-layer renders was
133.023 → 139.595 ms in English and 141.428 → 144.103 ms in Chinese. The
candidate was slightly slower in these host observations; no native speedup
is claimed. The measured ARM stack/text reductions and removal of software
division are separate evidence from a future actual calculator profile.

An executed stage-four-b actual-English-pack `card_grid_benchmark` run found
zero resource reads during 180 warm frames for the 16-card Ironclad library,
its detail view and the 12-card removal grid. Their measured host means were
1.078, 1.711 and 0.756 ms respectively. The seven-card merchant's first measured
180-frame animation window averaged 1.478 ms with 73 reads / 909,485 bytes;
its hover case averaged 1.558 ms with 71 reads / 886,929 bytes. This first-cycle
I/O does not by itself establish cache thrashing, because the sparse setup
advances have not visited all sprite frames. The retained successful receipt
is `ui8-hotframes-stage4b-eng-ab1a0f23c2e0.json`, using source
`2f3f18f11b16263456cf3dd0cc8a7525291ddf71` and the 93,345,135-byte English
resource pack. These are shared-host observations, not quiet A/B or device FPS.

The new `room_render_benchmark` measures three consecutive animation windows
for map, merchant, card reward, covered/revealed Crystal Sphere and Ancient
rooms. It records real range reads, seeks, decoded-cache occupancy and CPU
intervals, checks the canonical state is unchanged, and optionally writes
PPM captures outside the measured intervals. Scene setup uses production
controllers with explicit fixture outcomes for unrelated routing rooms; it
does not certify a whole run or count a diagnostic as completed gameplay.
The first baseline execution exposed a fixture that forced `Phase::Map` while
an original event room was still open. The candidate now keeps that actual
room beneath the map, using the normal overlay contract. Successful execution
and inspection of the corrected driver remain the stage-six gate.

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

The later, actually emitted v29 split is larger because it includes the new
UI assets, complete small font faces, and all character death frames. Its
successful conversion/split receipts report:

| Installed part | Actual bytes |
| --- | ---: |
| Shared artwork and other common data | 92,635,042 |
| English locale | 2,937,411 |
| Simplified Chinese locale | 7,522,168 |
| Both locales plus common data | **103,094,621** |

The reconstructed full packs are 95,572,293 bytes for English and 100,157,050
bytes for Chinese. The split is a tested delivery artifact, not the earlier
v27 projection. The final launcher and saved data are additional. The requested
30–40 MB installed size is still unmet; these results do not conceal that gap
by dropping original frames or lowering pixel quality.

## Portable validation gates and recorded failures

The actual stage-three ARM build at
`dc4c07c0da37e7ade17ee042868b4e6e42883303` failed at link time because new
signed 64-bit pose/raster expressions required `__aeabi_ldivmod`, while the
runtime previously supplied only the unsigned helper. The failed receipt
`ui8-arm-stage3-9c4648be6a97.json` is retained. The fix supplies a shared signed
wrapper without changing pose math, converts `INT64_MIN` to an unsigned
magnitude, restores quotient/remainder signs, preserves r4–r11 and keeps the
stack aligned. Frozen stage four
`a5ad4a955af293d66001cf2aa1e046ff85a196b0` then successfully built the
1,256,810-byte launcher. Its linked allocation is **16,610,388 bytes**, leaving
**166,828 bytes** below the unchanged 16 MiB packaging limit. The actual ELF
SHA-256 is `7144e5b8533158e122b95c881d7187dddb7251c94657de09c024aa9758a11a9b`.
The unsigned and signed helpers occupy 140 and 84 bytes respectively.

`tests/arm_division_regression.py` now checks both production linked helpers
against exact Python integer quotients/remainders, including signed extremes,
random vectors, zero-divisor fallback and `INT64_MIN / -1` local bit behavior.
`tests/arm_lzma2_regression.py` also covers the adaptive contexts in actual
ARM926 instructions. Instruction-level checks are separate from ordinary
calculator launch and do not measure device frame rate.

Against that actual linked ELF, **12,880 division vectors** passed (5,249
unsigned and 7,631 signed), including exact quotient/remainder, return PC,
stack and r4–r11 preservation. **42 LZMA2 vectors in seven fixture families**
passed, including contexts 3/0/0, 4/0/1 and 3/1/1, raw/compressed 512 KiB
chunks, bounds, guards and arena reuse. The three retained successful receipts
are `ui8-arm-stage4-ed45b31b1f29.json`,
`ui8-arm-stage4-division-f40b85211cd0.json` and
`ui8-arm-stage4-codec-075d0d9c3b10.json`; each reports unchanged frozen source.

Stage five, `41c8875812712a469e19577c7fcd3426722839c9`, also linked and
packaged successfully: **1,300,556-byte launcher**, **16,724,676-byte
allocation**, **52,540 bytes** below the unchanged 16 MiB limit. The actual
ELF SHA-256 is
`d759a5fc4b69c20d0753e99d160ee0c3760b3cd994ba1a0ddb3680b587018175`.
All 12,880 linked division vectors and 42 LZMA2 vectors passed again.
Receipts: `ui8-arm-stage5-space-retry-1b7b51ef9a05.json`,
`ui8-arm-stage5-division-ab1e65f5ee47.json` and
`ui8-arm-stage5-codec-b5ca1a9e1a22.json`. The tight remaining budget motivates
the actual shared-staging change above rather than shrinking image caches.

The initial stage-five ARM/native builds and font writes were interrupted by
the workspace becoming full. Their failed/incomplete journals and logs remain
intact, with an additional interruption-classification receipt. Only explicitly
approved, reproducible older build directories were removed, after preserving
their CMake/Ninja/test records; the latest stage-four-b and stage-five builds,
all source snapshots and all original inputs were retained. The ARM retry used
the same immutable source. A failed first run is not counted as a pass.

Stage six also linked successfully: **1,316,074-byte launcher**,
**16,552,328-byte allocation**, **224,888 bytes** below the unchanged 16 MiB
limit. Its net allocation is 172,348 bytes smaller than stage five despite
the intervening UI additions. ELF SHA-256:
`1d69c4466120cdab66de3fac1dfb7b6a89c7f75aad2389c0d3050ea782fbcbe2`.
All 12,880 linked division vectors and 42 linked LZMA2 vectors passed again.
Receipts: `ui8-arm-stage6-cf38b1840abd.json`,
`ui8-arm-stage6-division-af4d4debaaff.json` and
`ui8-arm-stage6-codec-7b615539abac.json`.

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

These allocations are actual checkpoint results. The final linked
ARM allocation must still be measured after later UI8 features are integrated.
The map workspace saving
provides room for features such as bounded map drawing, but it is not a
substitute for the final linked budget or a whole-process memory measurement.
