# Source27 UI and rendering evidence — 2026-10-21

The current built runtime is **source27**, `9d1e2a90ef5e942bd93fe304cd47eda0d88d3f69`.
Its native and WASM scene checks preserve the source25 output, and actual WASM
resource traces show substantially fewer repeated reads. Source25's complete
native test set has also finished through its recorded retry chain. The same-save
source27 Firebird comparison, fresh new-run route and their save/exit roundtrips
have completed. **The final v16 calculator pairing also completed its normal
new-run, actual UI, save and OS-exit path**, with an automation input incident
and the two profiler records' distinct provenance retained below.

The reference remains **v0.111.0 / 41cef1ea**. Exact report hashes, frozen source
inventories, journal statuses and completion fields are in
[validation-source27-ui-performance.json](validation-source27-ui-performance.json).
Evidence IDs below refer to that file's `evidence_catalog` and `journal_catalog`.
This is an evidence consolidation; preparing it ran no new builds or game tests.

## Checkpoint and artifact identities

| Checkpoint | Role in this receipt |
| --- | --- |
| Source25 `fc769a017d83051e34592e661d02c92aa0836b20` | Health/tooltip/button corrections; bounded State reset and codec workspace; native64 completion chain |
| Source26 `8536cadf8f8e3d88b6856c53e3970170cfd95d63` | Read-only startup-profiler marker; completed historical Firebird baseline with v15 |
| Source27 `9d1e2a90ef5e942bd93fe304cd47eda0d88d3f69` | Current ARM/WASM runtime with reader/image/tile cache and map-drawing changes |
| Source28 `c92b1733f1b54bff820583d8f6ab2d62fb56e824` | Corrected cache-pressure fixture and documentation; runtime inputs match source27 |

Source27's frozen validation-source fingerprint is
`95e3e6643724750436aac1be6a33bfbb5e27aa1da4cc984a223fddc879c7ad66`.
That logged fingerprint includes tests. The source28 equivalence receipt
separately identifies **274 identical runtime inputs**;
the changed inventory path is `tests/content_tiles_regression.cpp`.
No new source28 runtime binary is claimed. Evidence: `source27_runtime_io`,
`source28_corrected_fixture` and `source28-runtime-equivalence-and-io-9bebc0c3cbd2`.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Source27 ARM launcher | 911,249 | `79a095948a2297ea9a3ae4e2dba6c11a6c4743c63ec5a7993b792c17e0a66789` |
| Source27 WASM | 5,145,486 | `8688714c7d6c23dbec25e9729e14596242c0a84ff5a3dc869d5a660f270136bd` |
| Final v16 UI resource file | 80,879,519 | `89367cefcf3103d9bd834c47f1eb33e90119d3ab7cbb13b56e952501d2afc342` |
| Baseline v15 resource file | 80,846,058 | `407589a297a6e1bf935b6f2a5024ad532efd84423b0374db13c080e6671ba7dd` |

The successful ARM link reports **16,245,852 bytes of allocation** and **29,180
absolute relocations**. The WASM link retains its **16 MiB initial and maximum
memory** bounds. Linked allocation is not a measured peak working set. Evidence:
`source27-arm-build-e905c3005773`, `source27-wasm-build-af30568df3c9`,
`source27_wasm_link_source`, `source27_runtime_io` and `source25_ui_pack`.

## Source25 UI acceptance

The private v16 conversion appends **13 original UI images and 10 orb strings**,
adding **33,461 bytes** while retaining the existing catalog and compressed
payload. The native review uses the actual pack at **320 × 240** for keywords,
localized text, the selected-image oracle, world replay and the end-turn glow.
Its health regression deliberately uses synthetic image/text payloads.
Evidence: `source25_ui_pack` and `source25_ui_review`.

| Completed scope | Result | Interpretation |
| --- | ---: | --- |
| Actual keyword tooltips | 246,626 checks; 3 captures | Catalog title/body/layout plus pointer and keyboard paths; hand, detail and library surfaces |
| Actual localized world text | 1,841 checks | Includes Crystal remaining-count binding |
| Synthetic health UI | 467,441 checks; 5 captures | Production renderer and layout/state invariants; includes capture writes |
| Selected appended images | 59,062 checks; 13 images | 183,816 decoded bytes, 11,915 glyphs, 6,110 strings, 1,695 clips and 40 memory/range-equal UI frames |
| Actual world replay | 23 cases; 480 frames; 30 saves | Fixed scenarios, not exhaustive gameplay |
| Controlled actual end-turn glow | 226 completed checks; 7 pairs / 14 captures | Valid energy-zero fixture through production App/Canvas |

The former blank orb tooltip and Crystal count issue are closed within these
scopes. The glow captures cover its pulse and fade; after the 500 ms fade, the
entire frame equals the playable control. Repeated draws at the same clock
neither change pixels nor advance the cue. Save bytes stay unchanged by the
presentation operations. The label remained readable in the inspected samples;
the separate cream-color pixel mask was inconclusive because of antialiasing
and is **not** counted as a passing occlusion oracle. Evidence: `source25_ui_review`.

Against source24, all **23 fixture files and 30 saves** match exactly;
**474/480 frames** match exactly. The six changed Crystal frames differ by
**398 or 399 pixels** within `[222,41,295,51]`, accounting for the scoped text
repair. The selected image gate does not constitute a complete traversal of the
45,737-image v16 catalog. Actual normal-health captures were inspected, while
eight-enemy/block/infinite-health synthetic cases establish layout and state
invariants. Evidence: `source25_ui_review`, including its source24 comparison.

## P0 ARM stack repair and complete native test chain

The source25 reset helper value-constructs directly in existing storage, retaining
nonzero default members. The codec uses one shared BSS workspace and copies the
save input into it; load publishes a decoded state only after validation. This
removes full-State stack temporaries while preserving save bytes, immutable
inputs and failure atomicity. Its busy lease follows the single-thread event
loop contract. Reentry rejection was reviewed; dynamic nested-call coverage is
not claimed. Evidence: `source25_codec_source`, `source25_reset_source` and
`source25_arm_audit`.

| Linked ARM own frame, including saved registers | Source22 bytes | Source25 bytes |
| --- | ---: | ---: |
| `new_run` | 198,344 | 80 |
| `save` | 198,168 | 56 |
| `load` | 198,312 | 64 |
| `enter_combat` | 9,336 | 504 |
| `original_event_begin` | 39,792 | 104 |
| `original_event_new_run` | 39,752 | 16 |

These are individual frames, not total call-chain peaks. The State value
constructor adds a 160-byte frame where called, and the event constructor adds
56 bytes. The workspace itself is **198,112 bytes in BSS**, with a four-byte
pointer and one-byte busy flag. The linked fixed-entry inventory found no frame
above 64 KiB among 1,100 functions; its largest was `App::render`, 17,000 bytes.
That bounded entry scan does not prove whole-CFG, recursion or aggregate stack
usage. Evidence: `source25_arm_audit`.

Linked ARM instruction checks passed **5,249 division vectors**, **28 LZMA2
vectors**, and **20 codec cases** covering five characters in normal/practice
modes plus ten historical fixtures from versions 3–13. The new codec ran with a
64 KiB mapped stack and unmapped surrounding pages; bytes, state guards,
callee-saved registers, input immutability and failed-load atomicity held.
These instruction checks complement the later Firebird save roundtrip; they
are not OS execution or speed measurements. Evidence: `source25_arm_audit` and
its five linked-audit journals.

All **64 registered source25 native tests now have completed passing runs**
from the same GCC 13.3, `-O1 -g1`, ASan/UBSan build with X11 disabled. Leak
checking was disabled for the environment's `/proc` restriction. The evidence
is a completion chain, with the initial failed invocation retained:

| Invocation | Completed result |
| --- | --- |
| Initial full64 | 60 passed; core, relic-combo, events and character-UI timed out at the 300-second limit; exit 8 retained |
| Core-only retry | Passed in 394.29 seconds; 103,307 checks / 9,885 generated transitions |
| Remaining-three retry | Events 759.221 s; relic-combo 777.644 s; character-UI 535.321 s; all passed |

The remaining-three run used the existing binaries, two workers and a
900-second per-test limit. Events completed 244,163 checks; relic-combo
186,609 checks / 2,776 transitions; character-UI 38,001 checks / 860 acquired
variants with synthetic content. Their binaries were unchanged across the
retry. No ASan/UBSan error markers were emitted. Already-passing tests were
not replayed. This is not one all-green full CTest invocation or a fresh
source27/source28 full64 run. Evidence: `source25_native64` and its five journals,
including failed `source25-native-asan-ctest-ab7c419c31f7`.

## Completed source26 Firebird baseline

The baseline executed the ARM launcher in Firebird with the supplied CX II
ROM/OS, turbo mode, nominal 396 MHz CPU and a 32,768 Hz foreground SP804 timer.
The read-only sibling marker successfully enabled profiling before app startup.
Measurements below are emulated time; external watches count update starts,
not completed screen presentations. Evidence: `source26_firebird_baseline` and
`source26-v15-profile-4ec0a1945866`.

| Baseline measurement | Result |
| --- | ---: |
| File-open and app-init time | 44,083.34 ms |
| Separate first render | 535.28 ms |
| Stable title window | 150 updates in 5 simulated seconds; 30 updates/s |
| First six combat windows | 0.6, 0.7, 0.7, 0.7, 0.7, 0.9 updates/s; each window 10 simulated seconds |
| Post-authored-duration combat window | 12 updates in 10 simulated seconds; maximum observed gap 1,300 ms |
| Combat aggregate | 59 completed frames; 77,113.07 ms render time |
| Combat provider activity | 837 calls; 42,621,686 requested bytes; 770 seeks |
| Combat resource time | 43,183.47 ms; reported resource/render ratio approximately 56.0% |

Startup excludes the loader and marker probe. The combat aggregate includes
Continue/loading, idle animation and save input, so it is not a pure steady
frame distribution. Resource calls can occur during update as well as render.
The animation clock crossed **10,626 ms** against a **9,963 ms** longest enemy
idle duration. Clamped advances can skip authored frames, so this crossing does
not prove every source frame had entered the cache. Evidence:
`source26_firebird_baseline`.

The continued encounter was the actual three-enemy `SLIMES_WEAK` save: Ironclad
at 80 HP, three energy and five cards, with enemy HP 8/35/14. Continue, save key,
save-and-return, normal OS exit and USB retrieval completed. The retrieved
230,356-byte save exactly matches the source25 save, SHA-256
`c60baca9345faec5f106dad220c9249d5c414c427c697af789dc0d1af131c5ca`.
This completed baseline belongs to source26 with v15; current-runtime device
execution has its own separate receipts. Evidence: `source26_firebird_baseline`.

## Source27 cache/map changes and executed results

### Completed same-save ARM/Firebird comparison

Source27 continues the identical v15 battle from the same retained post-exit
baseline. The launched binary, discovered runtime layout and enabled profiler
were checked again. Save, return to title, normal exit and USB retrieval
completed; the save is still exactly the 230,356-byte source25/source26 file.
The journal passed with unchanged frozen source. Evidence:
`source27_firebird_v15` and `source27-v15-profile-77fc358822c2`.

| Observation | Source26 / v15 | Source27 / v15 |
| --- | ---: | ---: |
| App startup, simulated seconds | 44.083344 | 44.088898 |
| Animation elapsed at the comparison threshold, ms | 10,626 | 10,692 |
| Time to the sampled threshold from the first battle window, simulated seconds | 60 | 50 |
| Following ten-second window: update starts | 12 | 136 |
| Same window: maximum observed gap, ms | 1,300 | 1,350 |
| Additional current-only ten-second windows: starts/s | Not measured | 28.4; 30.0 |
| Additional current-only windows: maximum gaps, ms | Not measured | 320; 40 |

The primary matched window shows about **11.3 times** as many update starts,
but still includes cold decoding stalls. The later **30.0 starts/s** interval
is a separate, current-only warmed result; it has **zero provider calls,
requested bytes and seeks**. It is not a matched 25-fold frame-rate comparison
or evidence of physical-calculator FPS. Cold startup is effectively unchanged.

The primary window's global provider difference is **59 calls / 3,375,106
bytes / 57 seeks**. Its completed-frame aggregate instead attributes **57
calls / 3,170,027 bytes / 55 seeks** to 136 completed frames: a render can
cross the observation boundary. The receipt keeps both definitions. Likewise,
the full 815-frame current combat aggregate includes cold loading, warm idle
and save input; it must not be compared as an equal-duration mean against the
59-frame historical aggregate. The ten-millisecond watch observes update
starts, while the internal scene totals count completed frames. Evidence:
`source27_firebird_v15`.

### Completed fresh new-run observation

An independent fresh source27/v15 run also completed title → new Ironclad run
→ Neow → Fishing Rod → map → save → title → normal OS exit. Both the save and
profiler JSON were retrieved. Its startup was 44.03091 simulated seconds and
the run reported no resource failures. Evidence: `source27_firebird_newrun_v15`
and `source27-v15-newrun-f23c00cc0581`.

| Fresh-run window | Simulated seconds | Update starts | Starts/s | Maximum observed gap, ms |
| --- | ---: | ---: | ---: | ---: |
| Neow, initial/cold | 5 | 17 | 3.4 | 4,270 |
| Neow, following window | 5 | 141 | 28.2 | 40 |
| Map, entrance/display | 5 | 67 | 13.4 | 140 |

A read-only production-loader comparison proves the source25 and source27
runs use seed `0x20261003`, the same Fishing Rod choice and identical map-state
records. Only Act0 is generated at this point; its 143-slot drawing layout
also matches exactly. The two later-act records are still ungenerated, so this
is not an executed comparison of three generated acts. The newer saved route
is the expected prefix `[1]` of the old first-battle route `[1,17]`.
However, the map samples start approximately 4.20 versus 2.08 seconds after
the entry input, with different camera positions and profiler activation.
The old 33 starts and new 67 starts in five seconds therefore remain
**same-map entrance/display observations**, not a controlled stable-camera
twofold comparison. One stale-base observer read was identified, retained
and excluded; corrected discovery was read-only. Evidence:
`source27_newrun_seed_map` and `source27_firebird_newrun_v15`.

### Changes and native/WASM verification

The reader expands from **8 to 64 × 4 KiB pages**. The decoded picture pool
retains **748 × 4 KiB pages**, while its entry ceiling rises from **90 to 256**.
The selected tile pool becomes **40 × 128 KiB, or 5 MiB**, with 64 group entries.
The recorded ARM cache increment is **1,051,232 bytes**, plus **229,824 bytes**
for the reader change. Assets, source animation frames and pack bytes remain
unchanged by these runtime optimizations. The map path computes unrotated
inverse coordinates once per row/column, retaining pixel-center rounding and
painting rules. Evidence: `source27_native`, `source27_reader_source` and
`source27_map_source`.

The selection model replays **13,022 actual image accesses**, including startup,
title and Continue. Its old configuration matches the actual per-frame group
read sequence above 4 KiB. The chosen model predicts 665,963 encoded-group bytes
in the second 180-frame window; actual provider transport below is a separate
measurement. Small groups may use reader pages, so those quantities need not
match. Evidence: `source25_cache_model` and `source27_native`.

Native replay matches source25 in **all 480 frames, 30 saves and 23 fixtures**.
Fresh source27 WASM also matches native source27 across **23 scenarios, 480
RGB565 frames and 30 saves**, using final v16. This uses a storage adapter and
does not exercise browser DOM, compositor or persistent localStorage. The
selected clip-967 oracle matches **23 images / 1,557,577 decoded bytes per
backend**, with 253,981 checks and 40 UI frames. Its second cycle makes zero
provider calls or reader-page accesses. That is one uninterrupted clip,
not a complete catalog or battle-cycle claim. Evidence: `source27_native`,
`source27_world_golden` and `source27-world-wasm-replay-49d9ee428986`.

Actual Node/WASM resource measurements use the unchanged **v15** pack and the
same Continue fixtures, including the real Firebird save. Each window advances
180 frames at 33 ms without choosing Neow options or ending the battle turn:

| Scene / window | Source25 calls | Source27 calls | Source25 requested bytes | Source27 requested bytes |
| --- | ---: | ---: | ---: | ---: |
| Neow, first 180 | 11,340 | 3 | 46,448,640 | 12,288 |
| Neow, next 180 | 11,340 | 0 | 46,448,640 | 0 |
| Controlled Nibbits, first 180 | 49 | 45 | 273,445 | 257,061 |
| Controlled Nibbits, next 180 | 0 | 0 | 0 | 0 |
| Actual three-enemy save, first 180 | 790 | 399 | 36,747,150 | 22,296,766 |
| Actual three-enemy save, next 180 | 501 | 11 | 20,064,764 | 662,603 |

The actual battle's second-window requested bytes fall by **96.70%**. An
additional source27-only 360-frame window has zero reads in all three scenes.
Three declared frame hashes per scene match and saves remain unchanged;
the separate world gate provides the complete 480-frame comparison. These
callback measurements establish reduced repeated I/O, without establishing a
browser or calculator FPS improvement. Evidence: `source27_runtime_io` and
`source27-actual-resource-hotspots-a83f02d496dd`.

### Final v16 pack: separate current-runtime check

A completed additional source27 WASM run uses the **final v16 pack**, the same
three fixtures and **720 frames per scene at 33 ms**. This checks the cache with
the newly appended UI assets present. Its first-180, next-180 and final-360
window counts match the corresponding source27/v15 execution:

| Final v16 scene | First 180: calls / bytes | Next 180: calls / bytes | Final 360: calls / bytes |
| --- | ---: | ---: | ---: |
| Neow | 3 / 12,288 | 0 / 0 | 0 / 0 |
| Controlled Nibbits | 45 / 257,061 | 0 / 0 | 0 / 0 |
| Actual three-enemy save | 399 / 22,296,766 | 11 / 662,603 | 0 / 0 |

The final pack does not reintroduce repeated reads in these declared scenes
and windows. This is a current-runtime/final-pack check; **96.70% remains the
separate old/new comparison using v15 on both sides**. Actual ARM/Firebird
execution is established by the separate session below, not by this WASM run. Evidence:
`source27_v16_resource_trace` and
`source27-v16-resource-hotspots-0d3942538073`.

### Completed final v16 ARM UI and save path

The final 80,879,519-byte resource has now been installed in the CX II CAS
Firebird environment and opened by the exact source27 launcher. Normal UI
navigation reached a fresh Ironclad run, Neow, the first map node and the
three-enemy battle. Actual 320 × 240 captures show the native health bars,
end-turn control, pause plaques and red/gold confirmation controls. The initial
battle capture is retained separately from the later settled frame.

The selected card was verified to be **Defend**, with its actual five-Block
description. A real emulated touchpad pointer produces a nonempty **Block**
hover tooltip, and the **V** detail path shows that same keyword and its
description with a 1/1 page indicator. Early misselected Strike captures are
retained as failed navigation observations, not counted as keyword acceptance.
The parent independently inspected the final battle, confirmation, card-detail,
hover and keyword-page images. These are this port's actual device-environment
captures, without a paired original-engine visual comparison.

Startup on this complete-v16 installation took **59.006317 simulated seconds**;
the separately timed first render took **0.836823 seconds**. This session
deleted the old resource and transferred the complete replacement over USB.
The v15 comparison used its retained installation. Therefore these are distinct
installation observations, not a controlled attribution of the difference to
the UI additions or an individual validation stage. The matched v15 startup
comparison above remains 44.083344 versus 44.088898 seconds.

Normal save, return to title, OS exit and final USB save retrieval completed.
The retrieved **230,356-byte** save is byte-identical to the source25,
source26 and source27/v15 canonical battle save, SHA-256
`c60baca9345faec5f106dad220c9249d5c414c427c697af789dc0d1af131c5ca`.
The final journal is **passed / returncode 0 / source unchanged**, and the
backing flash file remains unchanged. Evidence: `source27_final_v16_firebird`
and `source27-v16-final-70720be96b12`.

The first normal exit was followed by an unintended launcher reopening after
the automation held its confirmation key for two simulated seconds. That key
dispatch explanation is inferred from the timing and reset startup counters.
The original captures and queue-cancellation records remain preserved. A short
confirmation then completed the second normal exit; the second invocation
contained only title and Quit-confirmation activity, without gameplay.

The first **3,714-byte** profiler record was recovered exactly from the preserved
snapshot and independently reconstructed from two modified-NAND data pages.
Its startup, first-render and first-present counters match the earlier read-only
runtime observation; the same page-extraction method reproduces the older v15
USB report. **This is recovered profile content, not a first-invocation USB
export or proof of the active filesystem path.** No complete filesystem/FTL
directory mapping was performed. The separate **3,442-byte** second-invocation
USB report contains 605 title frames and 38 Quit-overlay frames; it is not used
as the first UI run's timing or gameplay evidence. Evidence:
`source27_v16_first_profile`, `source27_v16_nand_crosscheck`,
`source27_v16_startup_observation` and `source27_v16_second_profile`.

The first UI sequence also records **236 overlay frames, 497 reads and
20,514,241 requested bytes**. Those combine card detail, hover, keyword and
confirmation operations in cold and warmer states. They identify another
bounded performance workload to investigate; they do not establish steady
overlay FPS or inherit the v15 battle's later 30.0 starts/s result.

## Preserved source27 failure and source28 fixture correction

`source27-resource-map-asan-18eef84a4b7c` remains **failed, exit 1**. The reader
passed 8,572 checks before the tiled-content injection assertion failed:
`!range->image(1).pixels&&fallback.failed>0`. Its 300 adjacent 806-byte test
blocks fit the expanded reader cache; valid resident bytes prevented the
injected provider failure from being reached. No sanitizer marker accompanied
that assertion. Evidence: `source27_native` and the retained failed journal.

Source28 places those 300 referenced fixture blocks on separate 4 KiB pages,
asserts cache pressure, and preserves the original failure assertion. Runtime
inputs remain identical to source27. Four affected ASan/UBSan tests, built from
independent new objects, then passed: **8,572 reader checks, 10,735,059 tiled
content checks, 1,563 cache-age checks, and 159,808 Canvas cases / 319,758
checks**. The corrected Release fixture also passed against the preserved
source27 library. These counts include fixture assertions and do not represent
independent game scenarios. Evidence: `source28_corrected_fixture`,
`source28-resource-map-asan-b1c4e7f38439` and
`source28-runtime-equivalence-and-io-9bebc0c3cbd2`.

## Completed gates and remaining acceptance boundaries

| Gate | Status in this receipt |
| --- | --- |
| Source27 same-save Firebird comparison against source26/v15 | Passed; matched save/normal exit, bounded cold and warm windows |
| Source27 fresh new run → Neow → map route | Passed; normal save/exit and retrieved profiler; map-window qualification retained |
| Source27 launcher paired with final v16 in the device environment | Passed; actual nonempty hover/V UI, identical retrieved save, normal exit; input incident and report-recovery limits retained |
| Physical handheld FPS and browser compositor FPS | Not measured |
| Original-engine UI similarity percentage | Not established; no matched original-engine capture set |
| Complete 30–40 MB calculator installation | Not achieved; final resource file alone is 80,879,519 bytes |

The remaining visual scope includes the original Doom animated shader,
block gain/break particles and the per-hit asynchronous visual timeline.
The completed source25 UI review, historical source26 profiler baseline,
actual WASM callback traces and current-runtime completed device gates retain their
separate scopes. Evidence: `source25_ui_review`, `source26_firebird_baseline`,
`source27_runtime_io`, `source27_final_v16_firebird` and the JSON receipt's
`completed_device_gates`. No declared execution gate remains pending in this
receipt; the acceptance boundaries above remain open.

## Next development priorities

1. **Original UI fidelity:** close the remaining Doom shader, block gain/break
   particles and per-hit visual timeline, then build a matched original-engine
   capture set at declared scenes and times. The present regression captures
   preserve this port's output; they cannot supply a 95% similarity score.
2. **Measured responsiveness:** address cold startup and first-use stalls before
   treating the warmed battle result as representative. `Content::open` currently
   scans the complete file for FNV integrity and then validates retained images,
   encoded glyphs and directory data; the runtime cache changes do not remove
   that startup work. Separate sequential transport, hash, validation/decode and
   first-scene costs before choosing a larger read batch or a different loading
   schedule, while retaining corrupt-input rejection and the memory bound.
   Card-detail, keyword and pause overlays need separate cold/warm measurements;
   the mixed final-v16 activity above must not be treated as a warm battle test.
   Map entry also needs a controlled camera/animation comparison before another
   speedup claim. Physical handheld and local-browser measurements remain separate
   targets from the completed Firebird and Node/WASM observations.
3. **Stored size:** implement and measure the bounded retained-image/glyph
   grouping experiment only with exact decode and cold/random/warm access checks.
   Its earlier component model still exceeds 40 MB; the current cache increase
   spends bounded RAM and provides no reduction in the delivered resource file.

Startup-path source: [Content::open](../src/content.cpp) and
[ResourceReader::fnv1a](../src/resource_source.cpp). The grouping measurements and
their unimplemented-format limits remain in [resource-budget-0.17.md](resource-budget-0.17.md).
