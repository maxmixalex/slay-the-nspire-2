# UI16 ordinary-OS Firebird acceptance (0.17)

These are actual LCD captures and production runtime counters from **Firebird emulating a TI-Nspire CX II CAS at 396 MHz**, OS 6.4.0.74, Ndless r2022. This is **not a physical-calculator test** and is not comparable with older 132 MHz CX FPS measurements. PNG conversion preserves every PPM pixel. Controlled room/encounter fixtures use the production console and save codecs; they are not a natural playthrough.

## Final measured revision and lifecycle

Runtime `4f3c7abfb1c580d37f63a158385ff79a47e87874`, launcher 1,760,617 B, SHA256 `cf83955453bc6046e57895a94efdf68ecf00a6c6e4ba2eec5e3a172c94f36158`. The data remains the identical 57cd512 installation: NSM1 index with 89 spans, eight NSS1 semantic parts and aligned NSL3 language containers. The default bilingual installation totals 81,022,714 B. The formal Zehn allocation is 22,523,484 B.

`combined/` records two separate, passing lifecycle checks with this exact launcher and complete final data:

1. Cold Chinese OS with no game preference → Chinese title → normal title Quit → OS → immediate reopen without USB → Chinese title → normal Quit again.
2. The exact original UI15 failure path: warm Haunted Ship combat → normal pause Save & Quit → title Quit → OS → immediate reopen without USB → English title → normal Quit again. This uses the same untouched warmed encounter as the performance control branch. It adds no End Turn and does not repeat or replace any performance measurement.

The actual input/event transcripts for both lifecycle paths are in `reproduction/`. The warm-combat transcript has no USB operation between exit and reopen. No allocation or resource error appeared in either path.

## Functional route and language selection

The eight `core/` captures and the original `locales/` captures use the earlier runtime `def1db3c68d3730edfddb4985feb403cc86b256b`, launcher SHA256 `6698dc8e4fb02589ce810a9b1c5bf565601bcdc88b71982457648a07be0f5a3a`, with NSM1, eight NSS1 parts and NSL3. The subsequent revision changes memory reuse and storage layout; the complete UI route was not redundantly repeated for those scoped changes. Do not relabel these earlier screenshots with the new launcher SHA.

- `core/`: deck sorting labels, map, shop, treasure ribbon, Smith pointer checkbox and upgrade animation, an actual touchpad eye click with tucked cards, and Tablet of Truth upgrade.
- `locales/chinese-default-title.png` and `chinese-reopen-no-usb.png`: Chinese OS with no initial game preference selects Chinese, and normal exit/immediate reopen with no USB succeeds. This lifecycle was also rechecked in the latest revision.
- French OS and French title/deck: cold OS startup with no game preference selects French. `auto-french.lang.tns` contains `NSL1fra` followed by LF.
- Dutch OS and English title: cold OS startup with no game preference falls back to English because the game has no Dutch locale. `auto-dutch.lang.tns` contains `NSL1eng` followed by LF.
- `missing-shared-english.png`: deleting only the required shared semantic part from the working copy produces the English `RESOURCES NOT FOUND` / `ESC BACK` screen under Chinese OS.

## Final UI15 versus combined comparison

All variants use the same Chinese OS, explicit English game preference, 396 MHz CPU setting, production Haunted Ship fixture, 20-second title warmup and 10-second combat warmup. For each version, A exits immediately from its warm snapshot; B holds Menu 600 ms and waits 5,400 ms before the identical normal exit sequence. The raw profiles are written on exit and downloaded afterward. Combat count/total counters are subtracted; maxima cannot be subtracted. A finite number of exit-boundary frames may differ. This is one paired action window per variant, not an all-scene statistical FPS estimate. No pair was rerun to choose better numbers.

| Measurement | UI15 baseline 49040d5 | Final combined 4f3c7ab |
| --- | ---: | ---: |
| Selected-resource validation + app-init total | 664.978 ms | 1328.674 ms |
| Combat frame-count difference | 166 | 164 |
| Interval total, 32768-Hz ticks | 197371 | 196282 |
| Mean render-stage elapsed time | 26.51095 ms | 25.93622 ms |
| Mean complete frame interval | 36.28485 ms | 36.52470 ms |
| Interval-derived FPS | 27.55971 | 27.37873 |
| Resource reads during action window | 19 | 19 |
| Requested resource bytes | 116259 | 113070 |
| Resource seeks | 19 | 19 |
| Resource time during action window | 332.214 ms | 452.789 ms |
| Intervals >50 ms | 5 | 6 |
| Intervals >100 ms | 4 | 4 |
| Intervals >250 ms | 0 | 0 |

The final package restores action-window read and seek counts to the UI15 counts and requests 3,189 fewer bytes. Mean render-stage elapsed time is 2.168% lower. Whole-window FPS is 0.657% lower in this single pair, so it does not demonstrate an overall FPS improvement. Resource time remains 120.575 ms above UI15, and the selected-resource/app-init scope remains slower. These residual differences have not been assigned a speculative cause or hidden by the smaller read count.

The render metric is elapsed SP804 timer time around `app.render()`, including any synchronous work inside that call; it is not an exclusive CPU-cycle measurement. The startup timer covers selected-resource validation and app initialization, not the complete Zehn document launch. Its start/end points remain unchanged across these versions.

The final probe observer now also accounts for length-probe open/close and seek/tell costs in the global `resource_total`; earlier versions omitted those costs from that global attribution. The old eight header reads had eight accounted read-seeks, while the final probe records the actual sixteen end/rewind seeks. Global resource totals therefore have more complete coverage. The combat hot-path counters and the total startup timer retain their old definitions and remain the comparison fields above.

`performance/` contains unchanged control/action profile JSON, exact counter differences, computed `comparison.json`, and action captures. The final 2.6/3.6-second images confirm Weak and five Dazed. Both final performance branches end in the ordinary OS browser. Their full action-branch maximum intervals are 181.976 ms for UI15 and 186.737 ms for combined; these include warmup and exit boundary and are not subtractable window maxima.

## Intermediate measurements retained

| Candidate | Action frames | Reads / bytes | Resource time | Mean render elapsed | Mean interval | >250 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Rendering/libc 18c3e3c, original NSC1/NSL2 data | 168 | 19 / 116259 | 332.275 ms | 25.39535 ms | 35.87632 ms | 0 |
| def1db3, first NSM1/NSL3 layout | 159 | 31 / 133382 | 638.397 ms | 26.92188 ms | 37.88306 ms | 1 |
| 57cd512, aligned locale + 89 spans | 164 | 22 / 113070 | 452.942 ms | 25.93752 ms | 36.52563 ms | 0 |

The first split layout's 288.42 ms full-branch maximum and additional reads led to the storage corrections. The final combined version removes twelve reads and 185.608 ms of resource time relative to that candidate, and no combat interval exceeds 250 ms. The later coalescing and single-probe changes restore read counts but barely change measured target latency relative to 57cd512; the report does not substitute native results for this device observation. The earlier 4.208% render improvement with original data is retained as its own experiment rather than substituted for the final package.

Intermediate startup totals are 598.358 ms (18c3e3c), 1156.677 ms (def1db3), and 1333.557 ms (57cd512). The `final` filename prefix denotes def1db3; `revised` denotes 57cd512; `r4f3c7ab` is the final combined candidate. All five raw result sets remain immutable.

## Warm-OS allocation boundary retained

Immediately after changing the OS language to French and uploading a launcher, Zehn can fail before the game starts. Exact RAM analysis of the earlier def1db3 prelaunch state found 28,797,068 B total free but only 22,145,408 B in its largest contiguous block; that candidate needs a 22,411,820 B execution block before metadata. The observed warm state remains unsupported without an OS restart; cold French and Dutch starts passed.

The subsequent 57cd512 launcher was then installed into the same original warm-French OS snapshot, retaining the old compatible resources to keep the OS/USB input sequence controlled. It still fails with the serial message `[Zehn] Memory allocation failed!`, despite its smaller 22,335,988 B execution block. This is a loader allocation boundary before resource parsing, not a failure of French fallback. Before and after snapshots each have matching NAND copies in the private validation workspace; neither ROM nor NAND is included here.


The final combined candidate was not redundantly tested in that already-failing warm-French state. Its cold/title lifecycle and the exact warm-combat exit/reopen path were tested separately and passed as recorded above. The retained warm-French failure is an OS fragmentation boundary, not a failed locale-default test.

No ROM, NAND, DLL, PCK, proprietary source or full save fixture is included in this evidence folder.

## Reproduction and artifacts

The control frontend is built by `tools/build_firebird_control.py` against the unmodified upstream Firebird core. It loads a user-owned BootROM and a working copy of the user-owned NAND. These private inputs are deliberately absent from git. Native host tests are separate from the emulated ARM execution described above.

The development workspace's `private-ui16-device/README.md` contains the complete restore/install sequence. The important invariant is that a restored snapshot overrides the configured flash path with the path embedded in its header: copy its exact matching NAND to that working path before every restore, and run only one emulator process against it. `flush` must precede capture of a matching NAND copy.

`run_pair.py VARIANT` restores a variant's warmed combat snapshot twice and records the following action branch before the same normal exit as its control branch:

```text
press menu 600 0
run 1000
screen 1600ms.ppm
run 1000
screen 2600ms.ppm
run 1000
screen 3600ms.ppm
run 1400
screen 5000ms.ppm
run 1000
screen 6000ms.ppm
```

The normal exit sequence is Esc 150/300 ms, Up 150/200, Enter 150/1000, Up 150/200, Enter 150/300, Right 150/200, Enter 150/1000. The first value is key hold duration and the second settling duration. Profiling is explicitly enabled; the production runtime writes its profile only on a normal exit. The emitted JSON is downloaded afterward using the ordinary USB link. Each profile declares the 32768 Hz foreground timer and its startup scope.

The controlled encounter was created through production save/progress codecs from `new IRONCLAD 1104089578` and `fight HAUNTED_SHIP_NORMAL`. No cards are played before the recorded End Turn. Screens at 2.6 and 3.6 seconds directly show Weak and five Dazed, and every branch's last screen shows the ordinary OS browser.

The final distributable evidence archive is `ui16-firebird-evidence-4f3c7ab.zip`, 1,501,699 B, SHA256 `d25a35ac8240d61b7c5336f53acb65446ce5f8bcc83ef5f65f2797b3a628d4dd`. It includes final and historical captures, raw counters, the warm-combat reopen event transcript, exact input scripts and the paired-window helper, without ROM/NAND/original assets. The earlier `ui16-firebird-evidence-def1db3.zip` and `ui16-firebird-evidence-57cd512.zip` remain immutable so the discovered regressions are not replaced by later measurements.
