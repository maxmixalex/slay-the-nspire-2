# UI18 cold preparation and ordinary-OS performance

## Result and remaining limit

The accepted implementation reduces the measured longest cold combat frame
interval from **1,129.30 ms to 192.54 ms** on the fixed CX II CAS Firebird
configuration. The first full-color HP/End Turn controls appear at **5,480 ms**
after Continue, compared with **5,530 ms** for the UI17 control. This includes
the real preparation and transition time; no clock alignment is used in those
device measurements.

A final, specifically measured correction prepares the common card-particle
texture and the status-card frame promised by an already displayed enemy
intent. Compared with the otherwise complete `a7e3e94` UI18 candidate, it adds
**20 ms** to cold entry and removes **64.97 ms** of warm-window resource time.
The same action window increases from 169 to 170 combat frames and from
28.02455 to **28.29246 FPS**. These are paired ordinary-launcher results, not
developer-trace or native CPU timings.

The original UI17 control remains **28.45571 FPS** in this finite window.
The accepted result is **0.5737% lower**, with **23.47 ms more resource time**.
The cold stall improvement and the final warm correction are real; they do
not establish an across-the-board FPS improvement. All intermediate results,
including the worse integrated candidate and repeated measurements, remain
in the evidence package.

## Tested identities

| Item | Identity |
|---|---|
| Reference game | v0.111.0 / `41cef1ea` |
| UI17 ordinary runtime | `d4303baf9a720bbeed58c1bb1859ff63343afcd6`; documentation tip `433445c` |
| Incremental preparation component | `5cc6018ec066b3139a14245ee7ba40d9d4b408a5` |
| First integrated UI18 runtime | `a7e3e9441c0dbd1c0372475769f0ea2a92c0cb81` |
| Accepted two-image correction | `feb344bac503d47484ccf9d88751d35cb38aed2d` |
| Integration of that correction | `44d7fb79f7d87d522038a074fa80d25e2bdeff31` |
| Resource converter | `3acb5b310d2d2cb42dbf98f21a79f0b45d8b42ed`; NSP2/42, NSM1, NSL3 |
| Accepted device launcher | 1,719,223 bytes; SHA-256 `7d34c4163a61c5f4984915d08272c42a3b4b69e003ddb1c388826d21d9098be7` |
| Firebird core | Unchanged upstream `b10f3b51a9ab8e27d2703444b5e6bd278d4c5e6e` |
| Device configuration | CX II CAS, 396 MHz, OS 6.4.0.74, Ndless r2022 |

The final component and integration have identical `include/`, `src/`,
`platform/`, and `third_party/` contents, recorded in
`device/final-runtime-source-equivalence.json`. The independent complete
`44d7fb7` ARM rebuild is byte-identical to the device-tested launcher;
`build-identities/root-full-arm.json` records that verification. The scoped
candidate build copied the 29 ordinary `a7` ARM objects, recorded every input
hash, and replaced only `11.o` (`app.cpp`). It retained the ordinary `24.o`
host, ARM926 flags, safe ICF and Zopfli. No trace object was linked into it.

Zehn allocation is **22,289,380 bytes**: 157,612 bytes of metadata and
22,131,768 bytes of contiguous execution/static storage. The correction adds
136 bytes relative to `a7`, changes no BSS pool, and retains all
**13,369,344 picture-pool bytes** and **four artwork file slots**. Its two
additional pictures use 29,952 decoded bytes, rounded to eight existing
4 KiB picture pages. This is ordinary cache use, not another allocation.

## Implemented preparation

The original cold path could synchronously decode a tiled actor or draw the
first HUD/card glyphs inside a single update or render. The implementation
now prepares these resources as explicit, resumable operations:

- Format-6 tile preparation uses the existing decoder, with at most 32 KiB
  encoded input and 4 KiB decoded output per decoding step. Tile filtering,
  frame-index prediction, geometry and tile copies advance in bounded units.
- The decoder borrows a pinned 64 KiB workspace from the existing tile arena.
  No partly decoded image becomes a cache hit. Completion, failure,
  cancellation, presentation reset and resource reopen release unpublished
  reservations and pins.
- The validator preserves the strict old NSP2/41 item count. NSP2/42 compact
  groups reserve their actual `items * 770` raw length and reject local tile
  references outside that actual item count. Group IDs and frame/tile access
  semantics remain unchanged.
- The current battle's background, actor poses and already announced action
  tracks retain their existing count/page budget. Current controls, hand
  faces, fitted text and glyphs join the same preparation interval. Text is
  visited with a zero-sized drawing surface; no hidden combat render runs
  hitboxes, discoveries, card-holder motion or rich-label clocks.
- Each update has a **soft 819-tick budget** (about 25 ms), at most 1,024
  microsteps, and at most four completed cold pictures. The deterministic
  offline path uses 256 steps. Foreground input is polled between operations.
- The host's black preparation loop can advance at a 262-tick minimum
  interval (about 8 ms). Visible gameplay retains the original 1,092-tick
  pacing. Preparation does not advance canonical combat or actor animation
  time; the visible transition starts after the resources are ready.

The final correction is deliberately small. `I7_GlowSpeck` is the existing
texture used by card draw/discard/exhaust/generated trails and is prepared
when the battle has cards. `U_ColorlessSkill` is the existing status-card
frame and is added only when an observed enemy intent contains
`EnemyIntentStatus`. It uses no encounter ID, Dazed special case, file offset,
future card choice, simulated move, or RNG call. The existing 256-entry UI
queue, sorting, preparation budget and fault paths are unchanged. Missing
assets in older supported packs still return a negative image ID and are
ignored by the existing helper.

The soft budget cannot preempt an OS read. The measured 192.54 ms maximum
is therefore still a real limitation. Frame/update maxima are reported as
such; this report does not claim a measured 25 ms maximum input-poll gap.

## Correctness and bounded-work gates

The original component includes actual tiled preparation, App scheduling,
cache cancellation and resource-failure tests. The compact-group additions
passed **1,580,418 Content checks** and **796,473 App checks** under scoped
ASan/UBSan. The final two-image correction passed **824,868 checks** in both
ordinary native and scoped ASan/UBSan builds. LeakSanitizer was disabled
because its process-task inspection was unavailable in this environment;
these are not whole-program or leak-sanitizer claims.

The final targeted test uses independent, non-aliasing textures. It checks
their residency and zero further reads at first use, two different announced
status-card moves, ordinary attack and stunned paths that do not prepare the
extra frame, unchanged canonical State after each preparation step, bounded
work, no save callback, cancellation and resource failure.

Actual English and Simplified Chinese replay compares the first complete
RGB565 frame, 182 subsequent complete frames and serialized canonical States
byte for byte against `a7`. All match. The presentation clock is explicitly
aligned to 200,000 only for this correctness oracle; its native ready time is
not used as device latency evidence. The English native action changes from
20 requests / 112,253 bytes / 18 seeks to 16 / 94,889 / 16, with zero action
opens. The Chinese candidate has 29 / 172,656 / 28, also with zero opens.
These native requests are not substituted for the different device window.

The integrated resource gate separately compares all 58,060 images and
1,167,930,140 decoded bytes through memory, ranged and incremental backends
against UI17. It executes 18,622,022 incremental steps, with a maximum of
11,093 per image, and verifies that Ready images are cached and require no
additional read when retrieved. See the resource report for the converter
and full-directory proof; this report does not count unused removed tile
records as per-battle CPU savings.

Important scoped journals in the evidence archive are:

| Gate | Journal stem | Result |
|---|---|---|
| Compact incremental ASan/UBSan | `ui18-preparation-compact-asan-fixed-035ada2bb272` | Passed |
| Final targeted native | `ui18-transition-art-native-c201ef6addbc` | Passed |
| Final targeted ASan/UBSan | `ui18-transition-art-asan-1b6829337b62` | Passed |
| Scoped ordinary ARM | `ui18-transition-art-arm-7a9c2a6328bd` | Passed |
| Actual ENG/ZHS complete-frame/State replay | `ui18-transition-art-real-eng-zhs-f6078a036715` | Passed |
| Matched ordinary device comparison | `ui18-transition-art-matched-device-2c1f5678a054` | Passed |
| Accepted program Save/Quit/reopen | `ui18-transition-art-save-reopen-499ff66ba590` | Passed |

These completed journals all retain their source identity and unchanged
source receipt. The earlier synthetic compact-test failure is retained: its
corrupt TGI2 index mutation used a two-byte multiplier instead of the actual
four-byte entry. Correcting the test did not change production decoding.
An earlier App test also held its rich-label snapshot across real black
renders, which legitimately increment the render-frame bookkeeping. Moving
that assertion to immediately surround each preparation update tests the
intended contract; the production render-clock lifecycle was preserved.

## Device measurement method

All timing uses the guest SP804 clock at **32,768 Hz** with the unchanged
Firebird core. The performance scenario is the real Haunted Ship encounter
and its displayed five-status-card move. Menu is held for 600 ms and followed
by 5,400 ms of running time. Ordinary control and action branches restore
identical per-version warm RAM and NAND, then use the same normal quit path.

Counts and totals are subtracted from the paired profiles. Maxima cannot be
subtracted and are never presented as action-window maxima. FPS is derived
from the difference in accumulated combat frame intervals; finite start/end
boundaries can change the frame count. No best-of selection is performed.

The cold witness samples the LCD every 10 ms and waits for fixed full-color
HP/End Turn pixels. The resulting wait includes the real loading and fade.
It is not an instrumented timestamp of the first enabled input instruction.
Accepted screenshots show the complete first hand and controls, the Weak
effect, five visible Dazed cards, and normal OS exit.

For the final causal comparison, both `matched-a7` and `matched-warmup` start
from the exact same `bilingual-installed.snap` and NAND. Both transfer their
ordinary launcher, wait the same time, open the title, Continue and run the
same paired action. This removes the earlier difference between opening an
already installed launcher and uploading a candidate immediately before it.

### Cold results

All times below are milliseconds except the explicitly labelled tick column.

| Program / data | Visible controls | Preparation updates | Maximum interval, ticks | Preparation read requests / bytes / seeks |
|---|---:|---:|---:|---|
| UI17 / original default11 | 5,530 | 33 | 37,005 | 71 / 877,484 / 69 |
| `5cc` / original default11 | 5,660 | 90 | 8,024 | 122 / 947,479 / 86 |
| `a7` / original default11 | 5,680 | 90 | 8,024 | 122 / 947,479 / 86 |
| `a7` / new data plus French12 | 5,470 | 90 | 6,309 | 122 / 946,945 / 86 |
| `a7` / new default11, initial control | 5,430 | 90 | 6,309 | 122 / 946,945 / 86 |
| `a7` / new default11, matched transfer | 5,460 | 90 | 6,308 | 122 / 946,945 / 86 |
| **Accepted `feb` / same default11** | **5,480** | **91** | **6,309** | **125 / 959,233 / 89** |

The accepted maximum interval is **192.5354 ms**, down **82.9510%** from
UI17. Maximum update time is 6,279 ticks (**191.6199 ms**), and maximum render
time is 1,115 ticks (**34.0271 ms**). UI17's corresponding maxima were 24,474
and 35,982 ticks. The longest operations are no longer a full synchronous
actor load or a full first HUD/card render.

The two-image correction changes total preparation intervals from 118,334
to 118,807 ticks and resource time from 73,159 to 73,292 ticks: only
**4.06 ms** more measured resource time in cold preparation. Thus the
64.97 ms warm saving is not simply hidden as an equal unreported entry wait.
Both matched programs have the same 36,203-tick startup measurement. That
counter retains its original selected-resource-validation/App-init scope;
it is not full launcher-entry-to-menu wall time.

### Warm results, including rejected and repeated controls

| Program / data | Frames | Interval ticks | Render ticks | Resource ticks | Requests / bytes / seeks | FPS |
|---|---:|---:|---:|---:|---|---:|
| UI17 / original default11 | 171 | 196,914 | 138,015 | 6,985 | 19 / 113,079 / 19 | 28.45571 |
| UI17 original retained-snapshot repeat | 171 | 196,905 | 138,015 | 6,985 | 19 / 113,079 / 19 | 28.45701 |
| `5cc` / original default11 | 172 | 197,666 | 138,803 | 6,910 | 18 / 105,199 / 18 | 28.51323 |
| `a7` / original default11 | 171 | 197,257 | 138,282 | 6,908 | 18 / 105,199 / 18 | 28.40623 |
| `a7` / new data plus French12 | 168 | 196,741 | 138,521 | 9,887 | 18 / 105,199 / 18 | 27.98107 |
| Exact repeat of that warm snapshot | 168 | 196,741 | 138,521 | 9,887 | 18 / 105,199 / 18 | 27.98107 |
| `a7` / new default11 | 169 | 197,460 | 139,253 | 9,881 | 18 / 105,199 / 18 | 28.04513 |
| `a7` / new default11, matched transfer | 169 | 197,605 | 139,306 | 9,883 | 18 / 105,199 / 18 | 28.02455 |
| **Accepted `feb` / same default11** | **170** | **196,892** | **138,600** | **7,754** | **15 / 91,338 / 15** | **28.29246** |

Removing the additional French file did not explain the regression. The
matched correction improves FPS by **0.9560%** relative to `a7`; the remaining
UI17 comparison is disclosed above. The two final matched action differences
both have zero frames above 250 ms. Mean render time changes from 25.15551
to 24.88080 ms; this is a mixed rendering/resource measure, not a pure CPU
rasterization benchmark.

## Why the warm correction was necessary

The ordinary profiler's 18 requests did not mean 18 OS calls: the cursor
splits larger transfers into 8 KiB reads. The existing developer recorder was
built separately and used only for attribution. Its same-program old/new
default11 control recorded **22 OS reads and 18 seeks**, with **no action
open/close** and no dropped records on either side.

| Identical operation or corresponding locale range | Old ticks | New ticks | Difference |
|---|---:|---:|---:|
| `interface`, offset 3,459,206, 4,096 bytes | 48 | 2,015 | +60.03 ms |
| `interface`, offset 43,866, 4,096 bytes | 47 | 2,009 | +59.88 ms |
| English, offset 352,627 → 352,323, 3,391 bytes | 1,997 | 1,021 | −29.79 ms |

The complete syscall difference is 2,970 ticks (**90.64 ms**), explaining
the ordinary result's 2,973-tick difference. The interface file is entirely
byte-identical between both resource versions: 10,786,410 bytes, SHA-256
`61e4972efd5b2d35f91afa5fdd82ba32e5e6f58b3fc4d2dc1569ac97d5d0c199`.
Its prior cold-operation shapes also match; neither target range had been
read earlier. The evidence therefore identifies variable first-use OS I/O
cost, not an added open, decoder payload change or extra split request.
It does not prove a particular internal OS cache or physical flash policy.

A native `Content::image` wrapper independently identifies image 47,801
(`I7_GlowSpeck`) at action frame zero and image 1,038 (`U_ColorlessSkill`)
at frame 18. That mapping led to the two actual preparation changes, instead
of hard-coded read offsets, a fifth stream, added padding or a fixture-only
prefetch. No developer trace program is included in the calculator package.

## Lifecycle, language and failure scope

The accepted `feb` program passes actual End Turn → Save & Quit → ordinary
OS → immediate **no-USB reopen** → Continue → normal quit. The restored hand,
HP, enemy intent and piles match. The device-written run is **7,320 bytes**
and is byte-identical to the `a7` run produced by the same played turn,
SHA-256 `3c60a3aa29eb334c3e257b7f72fcfb80011132dbcc35a2ad8003fd2857b30d61`.

The actual default-language matrix uses `a7` and the same final resource
bytes. The final two-image correction changes no language, loader or write
callback. These are the executed scopes, not a claim that all OS languages
were rerun on every later launcher:

| OS/default test | Actual result |
|---|---|
| Simplified Chinese, no saved preference | Chinese title; persisted `zhs`; immediate no-USB title reopen |
| French after an actual warm OS language change | French title; persisted `fra`; immediate no-USB reopen |
| Dutch after cold OS boot and ordinary Ndless installation | English title; persisted `eng` fallback |

The Dutch result is explicitly a cold default-language test. The earlier
warm NavNet upload/navigation automation problem is retained and is not
counted as an unexecuted game test.

Failure tests also use ordinary `a7` with the final resources:

- **Early missing shared part, French requested:** two launches show the
  built-in English `RESOURCES NOT FOUND` screen, separated by a normal ESC
  return to the OS and no USB. The original 230,352-byte run and 17,591-byte
  progress file remain byte-identical. Restoring the correct part and
  default eleven-file setup returns to a normal English title and OS exit.
- **Late enemy-part header mismatch, French requested:** the French title
  appears, Continue reaches the first invalid enemy header, and both attempts
  show built-in English `RESOURCE LOADING FAILED`. The run remains exactly
  unchanged. Progress is not byte-identical across the complete attempted
  launches: Continue legitimately persists before the header is first read.
  Its 300-byte NSZ envelope decodes to the original 17,591-byte NSH7 data with
  only the four-byte `last_saved_at` field changed; the other 17,587 bytes are
  equal. It is incorrect to call this entire launch a zero-write operation.

The permanent failure latch, per-input/advance/render checks and actual
mutating host callbacks are unchanged and covered by
`resource_failure_regression`, including failed advance with pending input,
same-input callback failure and language rollback. The order of the legal
Continue timestamp write is a source/control-flow conclusion, not a new
CPU-level write trace. The device proof and this limitation are kept separate.

The first bad-header attempt failed to transfer the replacement enemy file
over USB and is not accepted as a game-failure test. The retry's original
whole-progress equality assertion also failed for the legitimate timestamp
reason above. Both failed journals and the canonical comparison are retained.
The first `final` and `default` cold-setup attempts merely acknowledged an OS
sent/deleted-document dialog and had not entered the intended combat. Their
invalid timing attempts are retained, not relabelled as passing runs.

An additional ESC-during-entry probe preserves the original character
departure input gate: it does not actually open pause and is not offered as
device pause acceptance. Native preparation cancellation/pause tests cover
the non-departure path.

## Evidence and reproduction

The separate device evidence ZIP contains an explicit allowlist and SHA-256
manifest. `device/ui18-measurements-summary.json` reproduces all timing
arithmetic from the original profiles and receipts. Key folders are:

- `device/matched-a7-*` and `device/matched-warmup-*`: the final causal pair,
  ordinary startup/cold profiles, complete action screenshots and commands.
- `device/matched-warmup-warm-lifecycle/`: accepted program save, reopen and
  screen evidence, with hashes rather than the save payload.
- `device/release-default-locales/`: actual OS choices, titles and persisted
  default-language receipts.
- `device/release-fault-v2-preparation-header/` and
  `device/release-fault-v3-missing-part/`: late/early failure boundaries and
  normal resource restoration.
- `device/traced-*/` and `device/traced-data-comparison.json`: decoded syscall
  attribution; `layout-audit/` records the independent image mapping.
- `journals/` and `native-oracles/`: passed and failed scoped gates, full-frame
  comparison hashes, request/step transcripts and reproduction scripts.

Private NAND/RAM snapshots, Boot ROMs, game packs/fonts, raw saves/preferences,
launchers/ELFs and complete RGB565/State oracle payloads are excluded. Their
identities are retained where needed to identify a run. Reproduction requires
the user's original calculator inputs and private resource build; the ZIP
does not redistribute them. Only one process ever writes the working NAND.

Production is frozen after the two-image correction. Queue-full fallbacks,
later unannounced state changes and OS read latency can still require work
during play. No further optimization experiment or unverified performance
claim is hidden behind this acceptance record.
