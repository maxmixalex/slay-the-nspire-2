# UI16 calculator delivery and acceptance — 2026-10-29

Reference: the owner's pinned **v0.111.0 / 41cef1ea**. Work resumes from the
recovered, selected UI15 source, resources and complete Git history, rather
than the older public checkout. Original PCK, assembly, fonts, NAND and derived
resource packs remain private. Implementation, native correctness and ordinary
OS execution have distinct evidence below; probes and asset counts are not
new gameplay features.

## Selected runtime and resources

Runtime **`4f3c7abfb1c580d37f63a158385ff79a47e87874`** produces a
**1,760,617-byte** launcher, SHA256
`cf83955453bc6046e57895a94efdf68ecf00a6c6e4ba2eec5e3a172c94f36158`.
Resources are the complete, regular files at converter checkpoint
**`57cd512e56d3a10f6cb8b424cbae969c8216b2d3`**, including the optional exact
Portuguese/Turkish glyph repairs. NSP2/40 logical resources remain unchanged;
NSM1/NSS1 and NSL3 are physical storage wrappers, separate from NSZ1 saves.
Documentation and the test-launch helper may have later source-only commits;
the delivered production files retain the runtime identity above.

The complete final native gate passes **147/147 CTest registrations** at
`99063e30349500b6e845f8a46a2e831fd5d86163`, whose production sources match the
ARM runtime above. The ARM, actual ENG/ZHS, actual sixteen-language and package
gates all pass with `source_changed:false`. Final source publication remains
blocked as explained below; playable archives and recoverable source are the
local deliverables.

## The nine feedback items

| Requested concern | Implemented behavior or verified reference decision |
| --- | --- |
| One oversized common file | A **1,520-byte index** and eight required files for interface, cards, characters, enemies, bosses, rooms, effects and shared groups; all original logical artwork bytes reconstruct exactly. |
| Default languages and failures | A healthy saved explicit preference wins; otherwise system language, English and remaining usable editions are tried. Unsupported calculator languages select English when installed. Failed candidates are removed for the current session. Built-in technical recovery text is English, independent of locale fonts. |
| Upgrade checkbox | The original pointer-only **View Upgrades checkbox** remains. It toggles the entire grid's preview; directional navigation hides it and resets preview. It is not an extra radio choice. |
| Tiny deck sort labels | Text grows from **6 px to 8 px**; the strip and pointer hit area grow while original ordering, arrows and long-label scrolling remain. |
| Tucked card and eye | Card center moves **(31,88) → (18,78)** and shares the eye's layout constants. The original 250 ms cubic transition and half scale remain. |
| Mixed font families | The original normal-label substitution and explicit rich bold/italic roles now apply consistently to the audited controls in all 16 editions. Chinese Sans ordinary labels and Serif rich emphasis are intentional original roles. |
| Tablet versus Smith | Original Tablet **EventLayout/MessyLayout** and the separate **Smith VFX** are intentionally different and retained. |
| Map icons looking smaller | Normal/hover controls remain **20/29 px**, matching the original projection. Full transparent atlas margins are preserved instead of stretching the cropped ink to the control bounds. |
| Treasure title below ribbon | A single 10 px line now uses the authored banner's projected inner label rectangle at **(117,62), width 87**, with horizontal scrolling for long titles. |

Original class/scene evidence, exact geometry and executed gates are in
[UI feedback](ui16-ui-feedback.md) and [language/font roles](ui16-language-fonts-0.17.md).
Settings language-switch failure restores the previous working language; it
is not a forced English change. A missing shared artwork part cannot be repaired
by switching languages and produces the independent English recovery screen.

## Installed storage, without saves

These are calculator file sizes, excluding notices and explanatory text, not
ZIP download sizes.

| Configuration | UI15 bytes | Final UI16 bytes | Net saving |
| --- | ---: | ---: | ---: |
| English/Chinese resources | 80,155,768 | **79,262,097** | **893,671** |
| English/Chinese plus launcher | 81,910,800 | **81,022,714** | **888,086 (1.0842%)** |
| All 16 language resources | 104,025,419 | **97,815,664** | **6,209,755** |
| All 16 languages plus launcher | 105,780,451 | **99,576,281** | **6,204,170 (5.8651%)** |

Default installation is **81.022714 MB / 77.269281 MiB**. Splitting artwork adds
1,712 bytes; compression savings come from independent 4 KiB language pages.
The old language files alone shrink by 6,211,467 bytes across all 16, including
the optional glyph repair's 468-byte cost. The launcher grows by 5,585 bytes
against UI15. No additional original texture pixel, animation frame, existing
string or valid glyph is removed.

The former **65–70 MB default target is not achieved**. Approximately 74 MB
of already-compressed artwork still dominates. Sampled predictors, channel
planes, palettes and further LZMA parameter profiles did not justify a smaller,
faster production replacement. Those investigations are not counted as
features or savings. Existing source-to-LCD temporal sampling is separate from
the byte-identical container repacking.

[Resource layout](ui16-storage-0.17.md) lists every concern file. All eight and
the index belong to one edition and are required; only additional languages
are optional. Short complete A/B/A islands fold into neighboring matching
concerns, reducing 95 spans to 89 without splitting or duplicating a decoder
group. NSL2 suffix padding aligns the physical and virtual 4 KiB page phase;
physical NSL2 offsets change, while each complete logical NSP2 remains exact.

## Runtime work and memory

The production changes include exact opaque RGB565 blending without three
variable divisions, a direct unshaded flying-card loop, and aligned ARM926
memory words with safe byte heads/tails. Short/differently aligned operations
retain their measured small dispatch overhead; unused memcmp is not counted
as a shipping speedup.

Saved preview and synchronous rollback share one owned State workspace.
Random's tables, the merchant rug and selected-hand raster share a mutually
exclusive scene workspace. The selected hand's small identity and scroll
clock remain independent. No large workspace clearing or additional picture
cache eviction was introduced. The **13,369,344-byte image pool stays intact**.

The final Zehn allocation is **22,523,484 bytes**, **356,412 bytes below UI15**.
Metadata is 157,096 bytes; the executable/BSS contiguous request is
**22,366,388 bytes**, leaving 83,612 bytes below the enforced 22,450,000-byte
execution budget. The 24 MiB total ceiling is also enforced. These are loader
requests, not a measured peak of all OS and stack memory or a guarantee against
arbitrary OS heap fragmentation.

The final mixed-page reader owns a 32 KiB encoded batch and 4 KiB decoded-page
cache, adding 28,676 bytes to its per-instance storage. It merges only pages
needed by the current request; literal edge bytes remain exact, large literal
reads stay direct, and image transfer workspace ownership is unchanged.
Same-instance reentry is rejected. In the actual fixed native replay, physical
reads decrease **19 → 17**, with identical **102,505 requested bytes, 17 seeks,
zero cold artwork opens**, all 182 complete frames, canonical states and 117
logical requests unchanged.

Eight artwork providers still use at most four live artwork streams. Startup
now reads each file length and its 32-byte header in the same open; actual
initialization artwork opens decrease **16 → 8**. The temporary headers are
on the startup stack and are not retained. Main stack grows 176 bytes to 1,400;
this path adds no persistent pool. Headers, bounds and reference checks remain,
without eager payload scans or runtime integrity hashes. Startup profiler
coverage now includes previously uncounted length probes/open/close costs;
global resource totals across that change are not like-for-like. Startup whole
scope and combat-window counter differences remain comparable.

See [locale batches](ui16-locale-read-coalescing-0.17.md),
[startup probes](ui16-artwork-startup-0.17.md),
[hand raster ownership](ui16-selected-hand-memory-0.17.md), and the earlier
[renderer measurements](performance-ui16-0.17.md).

## Executed gates

| Gate | Passed result and actual scope |
| --- | --- |
| Final complete native build/test | **147/147**, `ui16-final-full-native-9a366dc71d23`; all registrations remain, including two uses of the same event-test executable |
| Full final ARM build | `ui16-combined-final-arm-59335313de91`; total and execution memory limits pass |
| Final real sixteen-language editions | **89,816 checks / 53,072 descriptions / zero failures**, `ui16-final-all16-b6743ae37fd5`; 90 bad-selected-pack recoveries to an English title |
| Final complete English / Chinese byte reconstruction | **12,485 / 12,619 checks**, **76,368,876 / 80,793,488 logical bytes identical**; `ui16-final-actual-eng-cbb52fdc06c1`, `ui16-final-actual-zhs-f0c0169450ed` |
| Mixed-page storage boundary and sanitizers | **10,075** synthetic checks native and ASan/UBSan; the named full actual ENG/ZHS ASan/UBSan gates also pass at the isolated identical locale implementation |
| Startup/cache boundary and sanitizers | **2,645** native and ASan/UBSan checks, including length 0/1/31, missing provider, reused FILE address, retry, cleanup and seven-stream host ceiling |
| Default-language policy | **137,139 checks**, 43 system tags and all 65,536 installed-language masks |
| Exact font inventory | **3,166** required font/size/codepoint entries; **48/48** original-family sample masks and metrics |
| Locale physical-read oracle | **182 complete frames / 27,955,200 pixel bytes**, **41,966,288 canonical-state bytes**, and all 117 logical requests identical |
| State workspace | Actual ENG/ZHS 246 checks each; 738 matching complete frames, canonical states and resource counters |
| Random/merchant raster workspace | 18 complete byte-identical frames, 2,764,800 compared bytes; actual Random 4,899,904 checks per language |
| Selected-hand raster workspace | 174 complete matching frames / 26,726,400 pixel bytes, 696 matching state/I/O/scroll rows; targeted sanitizer checks pass |
| ARM memory paths | 4,799 vectors per linked launcher version; separate memory object 9,908 vectors, including memcmp that is not called by the launcher |
| UI feedback | Actual ENG/ZHS deck, map, card/HUD, room, eye and world-selection paths, plus ordinary-OS captures at their stated checkpoints |
| Final player package | `ui16-combined-final-package-f162c3321418`; every member read back against its input, both logical defaults reconstruct, and all 14 optional locales have matching format |

The final native gate invokes each unchanged generated ELF through its own
declared system interpreter because this executor can lose generated execute
bits. It preserves test names, arguments, properties and both registrations
of the shared event binary. The preceding combined build completed all 257
steps but the first launcher helper rejected duplicate executable registration
before CTest; that helper-only failure remains recorded. Its corrected full
run above passes all 147 without dropping a test.

The all-native suites use synthetic resources where appropriate. Actual-pack
checks are separately named; test counts do not imply proprietary inputs in
every suite. ASan/UBSan apply to the named instrumented targets, not an
unexecuted whole-game sanitizer sweep. LeakSanitizer alone is disabled because
its leak detector is incompatible with executor ptrace.

Earlier compile/fixture corrections, mode-loss `BAD_COMMAND` failures,
source-changed provisional journals and the rejected initial storage-performance
candidate remain in history. No failed checkpoint is rewritten as a pass.

## Ordinary-OS Firebird acceptance and performance

The device environment is **TI-Nspire CX II CAS, 396 MHz, OS 6.4.0.74,
Ndless r2022**, with the user's NAND and matching Boot1 in an unchanged upstream
Firebird core. These are ordinary OS/ARM executions, not physical-calculator
measurements or the older 132 MHz CX configuration.

The final runtime passes both lifecycle reproductions: (1) cold Chinese OS
without a saved language preference → Chinese title → normal Quit → immediate
reopen without USB → Chinese title → normal Quit; and (2) the original warm
Haunted Ship combat → normal Save & Quit → title Quit → OS → immediate reopen
without USB → English title → normal Quit. The warm-combat check uses the same
controlled saved fixture, only ordinary keys and screenshots, and does not
replace or rerun the performance pair.

The final six-second control/action pair uses the same OS/clock, explicit
English preference, title/combat warmups and Weak/five-Dazed End Turn fixture
as UI15. Count and total counters are subtracted; maxima are never subtracted.
There is one paired window per version, with finite exit-boundary frame
variation. The render stage is an elapsed timer around `app.render()` and may
include synchronous resource work; it is not a hardware CPU-cycle measurement.

| Metric | UI15 | Initial split def1db3 | Corrected split 57cd512 | Final combined 4f3c7ab |
| --- | ---: | ---: | ---: | ---: |
| Frames | 166 | 159 | 164 | **164** |
| Interval ticks, 32,768 Hz | 197,371 | 197,375 | 196,287 | **196,282** |
| Render ticks | 144,206 | 140,266 | 139,387 | **139,380** |
| Resource ticks | 10,886 | 20,919 | 14,842 | **14,837** |
| Resource reads / bytes / seeks | 19 / 116,259 / 19 | 31 / 133,382 / 21 | 22 / 113,070 / 19 | **19 / 113,070 / 19** |
| Intervals over 50 / 100 / 250 ms | 5 / 4 / 0 | 5 / 4 / 1 | 6 / 4 / 0 | **6 / 4 / 0** |
| Mean elapsed render stage | 26.51095 ms | 26.92188 ms | 25.93752 ms | **25.93622 ms** |
| Whole-window interval-derived FPS | 27.55971 | 26.39702 | 27.37803 | **27.37873** |
| Selected validation + App init | 21,790 ticks | 37,902 ticks | 43,698 ticks | **43,538 ticks** |

Final mean render time is about **2.17% lower** than UI15, while whole-window
FPS is about **0.66% lower**, not improved. Reads return to UI15's 19 and
requested bytes decrease 3,189, but resource elapsed time remains 452.789 ms
versus 332.214 ms: the remaining approximately 120.57 ms is not assigned a
speculative cause. The initial split candidate's roughly 288 ms action stall
is absent from the revised/final windows, each with zero intervals over 250 ms.

The selected startup scope is **1,328.674 ms**, versus **664.978 ms** for UI15.
The last two fixes save only 4.88 ms against 57cd512 in this run, despite fewer
reads and opens; this is not a significant startup recovery. That scope excludes
full Zehn document launch. The earlier old-container renderer result and native
CPU improvements are retained as separate experiments, not substituted for the
final multipart result. No broader FPS or cold-stall claim follows.

Raw paired counters, exact captures, scope details and prior failed candidates
are preserved in [Firebird acceptance](ui16-firebird-0.17.md) and its evidence
archive.

The earlier def1db3 route retains its own identity: 196 ordinary UI inputs after
controlled room setup cover deck, map, shop, treasure, Smith pointer/directional
control and effects, physical eye click, and Tablet upgrade. Cold French OS
selects French without a game preference; cold Dutch OS selects English and
continues the English combat fixture. Saved preference bytes are respectively
`NSL1fra\n` and `NSL1eng\n`. A missing required shared part produces English
`RESOURCES NOT FOUND`, and Esc returns to the OS. These earlier screenshots are
not relabeled as final-runtime captures.

A warm OS immediately after a language change plus USB transfer can still fail
Zehn allocation before main. The frozen warm-French heap had 28,797,068 bytes
total free but only **22,145,408 bytes in its largest block**, smaller than the
final execution request even before metadata. Cold French passes; Chinese
normal exit/reopen passes. This is an explicit fragmented-OS boundary, not a
French font/fallback failure. Restarting the OS and reinstalling Ndless
reestablishes the tested cold-start condition. No arbitrary-fragmentation
immunity or leak diagnosis is inferred from this case.

## Recovered differences and explicit boundaries

UI15 already implements surrounding monster command awaits, conditional
summons/dialogue/deaths, Vantom's independent 750 ms bone transition, compound
clips/masks and four source emitters. The recovered production paths remain
connected and their suites pass. Doom, Architect, Random, Fake Merchant,
independent rainbow/blur clocks, card trails, generated-card arrivals, orbs and
other creature effects also remain. This turn does not rebrand rereading or
retesting those paths as newly implemented features.

The following limits remain disclosed:

- Full Ancient actor/background animation is the previously accepted static
  storage exception; Ancient gameplay, dialogue, choices, healing and card
  materials are implemented.
- Seed editing remains a 1–64 ASCII subset. Full Unicode normalization and the
  obsolete lowercase `old` path remain unimplemented. This does not affect
  localized game text or the sixteen language editions.
- RGB565/LDR, integer LCD rasterization, bounded CPU particles, independent
  cosmetic randomness and sampled material phases are calculator mappings,
  not continuous Godot GPU/global cosmetic-history equivalence.
- UTC interpretation, 96-floor/32-run history and finite drawing capacities
  remain. Audio, multiplayer, Timeline and desktop/network services remain
  outside the agreed offline single-player scope.

No 95% restoration score, universal 30 FPS, zero cold stalls, natural three-act
victory or exhaustive original-desktop visual acceptance follows from these
results. No manual browser/WASM acceptance was performed this turn.

## Installation and persistence

Preserve the existing `state` folder. When updating the old monolithic version,
**replace `data/nSpire2.dat.tns` with the small new index first**, freeing the old
74 MB file, then copy all eight concern files, desired languages and launcher.
Launch only after the complete matching set is present. Keep English for the
normal fallback. Do not require all sixteen locales to fit simultaneously on
the validated calculator, which reports roughly 92.3 MB user storage.

The player package contains an empty state directory, no test save or profiling
marker, and no NAND/ROM/original DLL/PCK. The optional archive contains fourteen
additional locales. Complete source and Git history are saved separately.

GitHub publication remains blocked: automatic approval review rejected the
attempted source push because sufficient outbound authorization evidence for
that payload and destination was unavailable. Recovered project notes contain
only a transcribed permission claim and Personal Context is unavailable here.
No connector bypass, remote ref update, main synchronization or new CI result
is claimed. Local/source-archive delivery is completed independently of that
remaining publication step.
