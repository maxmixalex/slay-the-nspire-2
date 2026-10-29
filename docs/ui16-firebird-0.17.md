# UI16 ordinary-OS Firebird acceptance (0.17)

These are actual LCD captures and production runtime counters from **Firebird emulating a TI-Nspire CX II CAS at 396 MHz**, OS 6.4.0.74, Ndless r2022. This is **not a physical-calculator test** and is not comparable with older 132 MHz CX FPS measurements. PNG conversion preserves every PPM pixel. Controlled room/encounter fixtures use the production console and save codecs; they are not a natural playthrough.

## Latest measured revision

Runtime `57cd512e56d3a10f6cb8b424cbae969c8216b2d3`, launcher 1,759,282 B, SHA256 `a52f71c94095a771f0af98d761c0949ba8ea7449ec6167a9fa073014fe0fa012`. It uses an NSM1 index with 89 spans, eight NSS1 semantic parts and aligned NSL3 language containers. Default bilingual installation is 81,021,379 B. The loader allocation is 22,493,064 B, with a 22,335,988 B execution block and 157,076 B metadata.

`revised/` shows a cold Chinese OS launch without a game language preference, normal Quit, immediate reopen without USB to the Chinese title, and a second normal Quit. These passed with the complete latest split data. A separate clean branch explicitly selects English for the identical performance encounter. `revised/warm-french-allocation-boundary.png` preserves the still-failing heavily fragmented OS state described below.

## Functional route and language selection

The eight `core/` captures and the original `locales/` captures use the preceding runtime `def1db3c68d3730edfddb4985feb403cc86b256b`, launcher SHA256 `6698dc8e4fb02589ce810a9b1c5bf565601bcdc88b71982457648a07be0f5a3a`, with NSM1, eight NSS1 parts and NSL3. The subsequent revision changes memory reuse and storage layout; the complete UI route was not redundantly repeated for those scoped changes. Do not relabel these earlier screenshots with the new launcher SHA.

- `core/`: deck sorting labels, map, shop, treasure ribbon, Smith pointer checkbox and upgrade animation, an actual touchpad eye click with tucked cards, and Tablet of Truth upgrade.
- `locales/chinese-default-title.png` and `chinese-reopen-no-usb.png`: Chinese OS with no initial game preference selects Chinese, and normal exit/immediate reopen with no USB succeeds. This lifecycle was also rechecked in the latest revision.
- French OS and French title/deck: cold OS startup with no game preference selects French. `auto-french.lang.tns` contains `NSL1fra` followed by LF.
- Dutch OS and English title: cold OS startup with no game preference falls back to English because the game has no Dutch locale. `auto-dutch.lang.tns` contains `NSL1eng` followed by LF.
- `missing-shared-english.png`: deleting only the required shared semantic part from the working copy produces the English `RESOURCES NOT FOUND` / `ESC BACK` screen under Chinese OS.

## Six-second action-window comparison

`performance/` contains untouched profile JSONs for control and action branches, their counter differences, and 3.6-second screenshots showing the real five Dazed reveal. All variants use the same Chinese OS, explicit English game preference, CPU frequency, production Haunted Ship fixture, and 10-second combat warmup. For each variant, A exits immediately from its warm snapshot; B holds Menu 600 ms and waits 5,400 ms before the identical exit sequence. Combat count/total counters are subtracted. Maxima cannot be subtracted. A finite number of exit-boundary frames may differ; this is one paired window per variant, not an all-scene statistical FPS claim. `comparison.json` retains the exact computed metrics and the raw inputs.

| Measurement | UI15 49040d5 | Rendering/libc 18c3e3c, old data | def1db3, first split layout | 57cd512, corrected split layout |
| --- | ---: | ---: | ---: | ---: |
| Frame-count difference | 166 | 168 | 159 | 164 |
| Interval ticks (32768 Hz) | 197371 | 197500 | 197375 | 196287 |
| Mean interval | 36.28485 ms | 35.87632 ms | 37.88306 ms | 36.52563 ms |
| Mean render CPU | 26.51095 ms | 25.39535 ms | 26.92188 ms | 25.93752 ms |
| Total render ticks | 144206 | 139802 | 140266 | 139387 |
| Total resource ticks | 10886 | 10888 | 20919 | 14842 |
| Resource reads | 19 | 19 | 31 | 22 |
| Resource bytes requested | 116259 | 116259 | 133382 | 113070 |
| Resource seeks | 19 | 19 | 21 | 19 |
| Intervals >50 ms | 5 | 4 | 5 | 6 |
| Intervals >100 ms | 4 | 3 | 4 | 4 |
| Intervals >250 ms | 0 | 0 | 1 | 0 |

The initial split-storage regression was used to guide the correction. Relative to def1db3, 57cd512 removes nine reads and 185.46 ms of resource time from this window, and no interval exceeds 250 ms. Relative to UI15, it reduces mean render CPU by about 2.16%, while mean frame interval is about 0.66% longer (27.378 versus 27.560 interval-derived FPS). Whole-window FPS therefore does not demonstrate an improvement in this single pair. The earlier 4.208% render improvement with old resources is retained as its own experiment, not substituted for the final result.

Selected-resource validation and app initialization counters are also retained: UI15 21,790 ticks (665 ms), rendering/libc 19,607 (598 ms), def1db3 37,902 (1,157 ms), and 57cd512 43,698 (1,334 ms). These are not complete Zehn document-launch times. Fewer reads and requested bytes did not make this measured startup scope faster; the residual device I/O difference is not assigned a speculative cause.

## Warm-OS allocation boundary retained

Immediately after changing the OS language to French and uploading a launcher, Zehn can fail before the game starts. Exact RAM analysis of the earlier def1db3 prelaunch state found 28,797,068 B total free but only 22,145,408 B in its largest contiguous block; that candidate needs a 22,411,820 B execution block before metadata. The observed warm state remains unsupported without an OS restart; cold French and Dutch starts passed.

The latest 57cd512 launcher was then installed into the same original warm-French OS snapshot, retaining the old compatible resources to keep the OS/USB input sequence controlled. It still fails with the serial message `[Zehn] Memory allocation failed!`, despite its smaller 22,335,988 B execution block. This is a loader allocation boundary before resource parsing, not a failure of French fallback. Before and after snapshots each have matching NAND copies in the private validation workspace; neither ROM nor NAND is included here.

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

The distributable evidence archive is `ui16-firebird-evidence-57cd512.zip`, 1,081,715 B, SHA256 `fabd6c3ec7d66a6f287b1ed2fac7d86637dd5bea8fe157f9639d7709e3918315`. It includes screenshots, raw counter JSON, computed differences and a file manifest, without ROM/NAND/original assets. The older `ui16-firebird-evidence-def1db3.zip` remains immutable so the discovered regression is not replaced by later measurements.
