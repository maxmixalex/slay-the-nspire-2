# UI10 performance, storage and acceptance — 0.17

## Evidence boundary

The accepted F runtime is `361c36c27f06c4905e1899b8bdc2ab281ba9b9cc` (tree `a031c1bc29513b80fbf3fd629bb07ef20fd53453`). The matched native core archive SHA-256 is `dfc981f168c37f3e795002de554ba875277c175384b4f98ece9db2f2b7942333`. The memo, retained-color and initial NSL1/NSL2 sections measure that frozen implementation. The I comparison runtime is `76c022a1e2db17c49bf3ab8edb335dbb98618582` (tree `546128015f0087d2327b28f3514019ccdc53d44d`), with native core SHA-256 `8bbc99f44ab08dcde0fd2bd820300786f9d95f473f423c5a2762b954aa8ef433`. The later sections separately record F/I same-input reader/combat measurements, bounded H/I attribution, the adopted guard's exact final K binary association, and actual H/I/K calculator acceptance. No numerical original-game fidelity percentage is inferred from these performance or regression results.

The F quiet host measurements ran serially with CPU affinity set to one allowed CPU (CPU 0), after the other known build, conversion, browser and emulator tasks stopped. Process CPU time and elapsed time were both retained. A fresh application or fixture has cold application caches; the host OS file page cache was **not** flushed. All samples, including outliers and regressions, remain in the private receipts. This is native host evidence, not ARM FPS, physical calculator timing, or a guarantee for every scene.

Private execution records:

- Memo and retained-color four-repeat AB/BA comparison: `/tmp/slay-ui10-focused-benchmark/final-f/receipt.json`; formal journal `ui10f-quiet-glyph-and-retained-112e720c9d67.json` (passed).
- NSL1/NSL2 startup twenty-repeat AB/BA comparison: `/tmp/slay-ui10-focused-benchmark/startup-f/receipt.json`; formal journal `ui10f-quiet-nsl2-startup-0784b6493dcc.json` (passed).
- Each directory contains the raw JSONL samples. The private drivers are `run.py`, `content_open.cpp`, and `startup.py` in `/tmp/slay-ui10-focused-benchmark/`; driver/source/archive hashes are recorded in each receipt. Original content, generated packs, screenshots and executables are not stored in this source repository.

## Glyph metadata memo: same rendered UI

The baseline disables only the three descriptor lookup/remember calls in the F content reader. The memo retains 256 complete 32-bit keys and indices: 2,052 bytes on ARM, 2,056 bytes on the measured 64-bit host. It does not reduce image/font caches or alter animation clocks. The comparison uses the final E NSP2/38 logical content for both variants. All reported scene/hit/canonical-state observations and source I/O agree. The earlier A comparison independently matched 1,734 paired complete frames; the F timing driver checks scene observations, not an independent full-frame hash for every intermediate timing sample.

Each warm window contains 180 frames per repetition; there are four alternating AB/BA repetitions. Entries below are **mean; median; min–max**, in process CPU milliseconds per frame. The ranges are the four window-average measurements, not individual-frame latency ranges. All ten warm scene/locale windows issue zero source reads in both variants.

### English

| Warm scene | Without memo: mean; median; range | With memo: mean; median; range | Mean change |
|---|---:|---:|---:|
| Ironclad card grid | 1.7938; 1.7537; 1.6930–1.9750 | 1.5689; 1.5581; 1.5344–1.6249 | -12.54% |
| Ironclad inspector | 2.8316; 2.5919; 2.5074–3.6351 | 2.5593; 2.4220; 2.3236–3.0697 | -9.62% |
| Rainbow card grid | 2.7490; 2.5044; 2.4011–3.5860 | 2.1210; 2.0978; 2.0049–2.2837 | -22.84% |
| Rainbow inspector | 3.7042; 3.5569; 3.4796–4.2234 | 3.0894; 3.0877; 2.8732–3.3090 | -16.60% |
| Keyword hover | 4.0500; 3.5607; 3.4321–5.6465 | 3.1495; 2.9427; 2.8174–3.8952 | -22.24% |
| All five windows, 900 frames/repeat | 3.0257; 3.0838; 2.7227–3.2127 | 2.4976; 2.4365; 2.3368–2.7807 | -17.45% |

The all-window row first sums the five windows within each repeat and then computes its median/range. A different statistic, the equally weighted mean of the five **per-window medians**, is 2.793532 → 2.421658 ms/frame (-13.31%). Neither statistic removes any sample.

### Chinese

| Warm scene | Without memo: mean; median; range | With memo: mean; median; range | Mean change |
|---|---:|---:|---:|
| Ironclad card grid | 1.5475; 1.5557; 1.4968–1.5818 | 1.5343; 1.4645; 1.4005–1.8077 | -0.85% |
| Ironclad inspector | 2.3783; 2.3708; 2.3023–2.4694 | 2.2788; 2.2634; 2.2195–2.3689 | -4.19% |
| Rainbow card grid | 2.1321; 2.1262; 2.0802–2.1959 | 1.9407; 1.9214; 1.8557–2.0642 | -8.98% |
| Rainbow inspector | 3.3259; 2.9553; 2.8866–4.5066 | 2.8068; 2.7893; 2.7408–2.9077 | -15.61% |
| Keyword hover | 2.9907; 2.9425; 2.8746–3.2031 | 2.7647; 2.7524; 2.7060–2.8482 | -7.56% |
| All five windows, 900 frames/repeat | 2.4749; 2.4085; 2.3828–2.6998 | 2.2651; 2.2597; 2.2257–2.3152 | -8.48% |

The all-window row first sums the five windows within each repeat and then computes its median/range. A different statistic, the equally weighted mean of the five **per-window medians**, is 2.390067 → 2.238192 ms/frame (-6.35%). Neither statistic removes any sample.

### Cold/input costs and remaining variation

Warm improvements do not establish a universal input-latency improvement. The following actual F results are retained as counterexamples. Times are mean process CPU milliseconds for the stated phase; settle phases explicitly include eleven frames.

| Locale / phase | Without memo | With memo | Mean change |
|---|---:|---:|---:|
| English Ironclad first inspector response | 2.19300 | 2.31225 | +5.44% |
| Chinese Ironclad first inspector response | 7.05550 | 7.61125 | +7.88% |
| Chinese Rainbow first inspector response | 8.17575 | 11.67450 | +42.79% |
| Chinese Rainbow eleven-frame entry | 48.65825 | 51.87850 | +6.62% |
| Chinese first keyword-pointer frame | 2.71550 | 3.31975 | +22.25% |

For the Chinese Rainbow first inspector response, the four baseline samples are 8.270, 6.919, 8.913, 8.601 ms; memo samples are 8.097, 23.556, 7.496, 7.549 ms. Its median improves (8.4355 → 7.8230 ms) while its mean worsens. Warm outliers are also retained, including the English baseline keyword window at 5.6465 ms/frame and Chinese baseline Rainbow inspector at 4.5066 ms/frame. Four repetitions do not justify a tail-latency or device-performance claim.

## Retained-color compression: exact pixels, mixed timing/I/O

The RGR2 format preserves RGB565 and alpha exactly and uses the existing decoder/foreground workspace. Its measured conversion selected 689 of 840 retained groups and saved 949,484 bytes net in the shared bilingual artwork. Complete verification decoded all 51,123 images per language from the matching initial 35/36 packs, with glyph/text/clip clocks and tile/index streams preserved. Those matching 35/36 packs, not the subsequently enlarged 38 catalog, are used for the timing comparison below. Both variants run the same F executable.

| Locale / phase | Original35 CPU mean | RGR2 CPU mean | Original calls / bytes | RGR2 calls / bytes |
|---|---:|---:|---:|---:|
| ENG room open | 45.6183 ms | 41.6605 ms | 56 / 1,625,488 | 55 / 1,613,189 |
| ENG first inventory response | 10.4535 ms | 11.3667 ms | 4 / 95,335 | 4 / 91,280 |
| ENG 31-frame entry | 130.8947 ms | 106.0855 ms | 46 / 658,590 | 46 / 650,938 |
| ENG 180-frame animation | 378.3378 ms | 353.3117 ms | 6 / 174,986 | 6 / 174,521 |
| ZHS room open | 43.9570 ms | 43.1810 ms | 56 / 2,107,514 | 56 / 2,099,311 |
| ZHS first inventory response | 9.3415 ms | 9.7030 ms | 4 / 95,335 | 4 / 91,280 |
| ZHS 31-frame entry | 114.7198 ms | 113.7675 ms | 68 / 844,438 | 72 / 853,170 |
| ZHS 180-frame animation | 352.0825 ms | 339.6700 ms | 7 / 179,082 | 6 / 174,521 |

All compared observations match. The first response mean worsens by 8.74% in English and 3.87% in Chinese despite smaller bytes. Chinese inventory entry reads 8,732 more bytes and four more callbacks; layout/page boundaries can change the cost even when the stored format is smaller. The 180-frame window averages improve from 2.101876 → 1.962843 ms/frame (English) and 1.956014 → 1.887056 (Chinese), but the table and raw samples are the evidence boundary. The earlier C range-provider test independently matched 2,832 paired complete frames and also found a Chinese card-scene I/O increase. No universal decoder or cold-frame improvement is claimed.

## NSL2 directory compression: storage and startup tradeoff

F compresses only the metadata prefix already retained in Content’s existing 2 MiB directory cache. The end is aligned down to a 4 KiB page; frame descriptors and localized suffix remain raw. Each independently decodable block is at most 64 KiB, using the existing foreground Bytes lease. Common artwork and logical NSP2/38 bytes are identical to E. The actual bilingual saving is 2,394,329 bytes: English 979,157 and Chinese 1,415,172. The resulting split resources occupy 90,014,771 bytes; with the F launcher the installation occupies 91,480,069 bytes.

The startup comparison uses one F executable, E NSL1 files and F NSL2 files with identical common artwork. Twenty alternating repetitions per language start fresh processes. Adapter-open and Content.open are separately timed; metadata inspection occurs afterward and causes no extra I/O. All metadata fingerprints and virtual Content read counts/bytes match. The physical counts below are actual FILE-provider callbacks and cursor seeks, not NAND-controller measurements.

| Locale / phase | NSL1 CPU mean; median; range (ms) | NSL2 CPU mean; median; range (ms) |
|---|---:|---:|
| ENG adapter open | 0.0095; 0.0080; 0.0070–0.0410 | 1.8960; 1.8195; 1.3910–3.0160 |
| ENG Content.open | 0.7052; 0.6610; 0.5570–1.1470 | 9.3924; 9.2630; 7.4520–12.8470 |
| ENG total | 0.7147; 0.6685; 0.5660–1.1880 | 11.2883; 11.2865; 8.9010–15.8630 |
| ZHS adapter open | 0.0155; 0.0080; 0.0070–0.0950 | 1.6246; 1.5445; 1.3850–2.4120 |
| ZHS Content.open | 1.0692; 0.9310; 0.7670–2.0060 | 16.4682; 16.1675; 14.7780–18.4360 |
| ZHS total | 1.0847; 0.9690; 0.7750–2.1010 | 18.0928; 18.1055; 16.1630–20.0150 |

| Locale / format | Total physical callbacks | Total requested bytes | Total cursor seeks | Content.open callbacks / bytes / seeks |
|---|---:|---:|---:|---:|
| ENG NSL1 | 9 | 1,063,222 | 8 | 4 / 1,063,010 / 4 |
| ENG NSL2 | 46 | 106,899 | 43 | 38 / 90,032 / 37 |
| ZHS NSL1 | 9 | 1,584,806 | 8 | 4 / 1,584,594 / 4 |
| ZHS NSL2 | 62 | 191,590 | 59 | 54 / 175,108 / 53 |

NSL2 reduces requested startup bytes by 89.95% in English and 87.91% in Chinese, while adding about 10.57 ms and 17.01 ms of mean host process CPU time respectively. F also performs more small table seeks. These are measured startup costs, not free performance gains. Warm UI metadata accesses stay in the existing Content directory cache; compression does not add decode work to that warm path. The ordinary calculator acceptance below establishes successful real boot/language-switch/Continue behavior but does not supply a physical-device timing benchmark.

## Requested-row table batch: final I acceptance

The reader introduced in G and retained in I batches only the table rows needed for a single directory request into a bounded 256-byte stack array. It retains the same exact decoder, per-block bounds and foreground Bytes lease; it adds no resident pool and does not scan unrelated rows. A full four-block test uses five physical callbacks and one initial seek: one 32-byte table range followed by contiguous block data. Partial-block reads copy only the requested literal portion; compressed blocks still decode exactly. Tests cover a three-block subrange, directory/raw-tail/common boundaries, table read failure before output, malformed later rows, retry, guards and released leases. The short linked split test passes 42,948 checks, with the adapter still 128 bytes on the host.

Final comparison `ui10i-quiet-f-i-nsl2-reader-98ceb99af101.json` passes. F and I use their matching headers/archives and identical Release Clang 18 flags. Both read the **same physical F NSL2 files**. Each language has twenty alternating AB/BA repetitions, pinned to CPU 0 after known heavy jobs and the ordinary emulator stopped. Metadata fingerprints, virtual Content reads and physical requested bytes agree in every sample. The final I font/artwork additions are deliberately absent from this reader comparison. Raw samples and source/archive/input hashes are in `/tmp/slay-ui10-focused-benchmark/startup-final-fi/receipt.json`.

| Locale / runtime | Total physical callbacks | Total requested bytes | Total cursor seeks | Content.open callbacks / bytes / seeks |
|---|---:|---:|---:|---:|
| ENG F | 46 | 106,899 | 43 | 38 / 90,032 / 37 |
| ENG I | 31 | 106,899 | 12 | 23 / 90,032 / 6 |
| ZHS F | 62 | 191,590 | 59 | 54 / 175,108 / 53 |
| ZHS I | 39 | 191,590 | 12 | 31 / 175,108 / 6 |

These callback/seek reductions are deterministic in all repetitions. They are FILE-provider results, not a measured NAND latency saving. Startup process CPU remains mixed:

| Locale / phase | F CPU mean; median; range (ms) | I CPU mean; median; range (ms) |
|---|---:|---:|
| ENG adapter open | 1.8511; 1.8040; 1.4160–2.4940 | 1.7494; 1.7265; 1.3910–2.1320 |
| ENG Content.open | 9.4164; 9.0615; 8.1550–11.4990 | 9.5797; 9.2955; 7.6690–13.0270 |
| ENG total | 11.2674; 10.9265; 9.8340–13.9930 | 11.3291; 10.9745; 9.0600–14.5820 |
| ZHS adapter open | 1.8363; 1.8200; 1.3900–2.4550 | 2.0314; 1.7640; 1.3990–6.7750 |
| ZHS Content.open | 18.0972; 17.7195; 16.1130–22.8890 | 17.6091; 17.5270; 16.2820–19.4030 |
| ZHS total | 19.9334; 19.8770; 17.6980–24.6800 | 19.6404; 19.3595; 17.7150–26.1780 |

Mean total CPU changes by +0.55% in English and -1.47% in Chinese. The Chinese I adapter sample of 6.775 ms and its 26.178 ms total remain in the result; adapter I/O is unchanged by the batch. This does not establish a general CPU startup improvement or remove NSL2's previously measured decompression cost. The delivered improvement is fewer requested-row callbacks/seeks with identical decoded metadata and bytes, within the existing memory budget.

## Final default-off combat check and bounded attribution

The existing `combat_preparation_benchmark.cpp` was reused for one English combat. Its private common driver adds process CPU timing and computes every rendered-frame fingerprint outside the timed intervals. Both runtimes use the identical 86,926,376-byte logical F NSP2/38 pack (SHA-256 `3b451fa9fe08bc57710524f4c2508e0be3243ca2999a7adfdd0e8f4c23fded18`) and the same actual 230,556-byte combat save obtained by ordinary I play (SHA-256 `bdfd3d7188452e17697083f686560480735239ae74a394e336ebf6b174ca50c4`). It is the real floor-3, turn-1 battle reached from the preserved run, not a changed workload created for one runtime. Preferences are default, including Long Press off.

Four alternating AB/BA repetitions each measure 180 `advance(33)` plus render frames. All 720 paired RGB565 frame fingerprints match, every runtime loads/resaves the original canonical bytes exactly, each before/after canonical state is unchanged, and both warm windows issue zero source reads. Formal `ui10i-quiet-f-i-default-off-combat-a85783fe672a.json` passes those correctness conditions. It also records a performance counterexample:

| Runtime | Mean CPU ms/frame | Median CPU ms/frame | Range of four window averages |
|---|---:|---:|---:|
| F | 0.415188 | 0.415200 | 0.404933–0.425417 |
| I | 0.473847 | 0.460075 | 0.446372–0.528867 |

The measured I mean is **14.13% higher**, its median **10.81% higher**. Complete 180-frame CPU samples are F `[73.311, 76.575, 72.888, 76.161]` ms and I `[83.336, 82.291, 80.347, 95.196]` ms. This initial result is retained in `/tmp/slay-ui10-focused-benchmark/combat-final-fi/receipt.json`; the correctness pass is not relabeled as a performance pass.

One bounded H/I comparison then separated the new hold feature from the intervening G/H changes. It uses the original preserved H archive, SHA-256 `c53b519c5b21d82cfc4080e74cc1498b94824ce80e252a0313b532e8853d7e85`, and the same driver/input/frame checks. The deleted H build directory was not reconstructed: only the configuration command recorded in its formal build log was replayed to check Release/compiler flags; no H object was rebuilt. Formal `ui10i-quiet-h-i-archived-core-0e1aa112ba51.json` passes, again with all 720 paired frame fingerprints, canonical bytes and zero warm I/O matching.

The H 180-frame CPU samples are `[77.061, 77.221, 86.194, 70.986]` ms (mean 77.8655, median 77.1410); I samples are `[79.108, 80.815, 73.495, 72.229]` ms (mean 76.4118, median 76.3015). I is 1.87% lower by mean and 1.09% lower by median in that follow-up. The two I batches differ substantially. This short follow-up does **not** prove that the initial F/I regression vanished or identify its cause. All original samples and this bounded follow-up remain in `/tmp/slay-ui10-focused-benchmark/combat-hi-archived/receipt.json`. The longer comparison below addresses that unresolved difference without adding another scene.

### Longer balanced F/H/I comparison

Formal `ui10i-quiet-balanced-f-h-i-long-9c949d12fd59.json` reuses the already compiled F/H/I binaries and identical inputs. Four rounds use the order F/H/I, I/H/F, H/F/I, I/F/H. Every pair appears in each order twice; all versions have the same average position within a round. Each sample contains 3,600 frames, so there are 14,400 frames per version and 43,200 rendered frames in total. All per-frame fingerprints match across versions/repetitions, original and before/after canonical bytes remain exact, and every measured 180-frame subwindow has zero source I/O.

| Runtime | Four complete 3,600-frame process CPU samples (ms) | Mean ms/frame | Median ms/frame | Range of sample averages |
|---|---|---:|---:|---:|
| F | 1416.630, 1419.070, 1524.215, 1421.814 | 0.401509 | 0.394567 | 0.393508–0.423393 |
| H | 1517.970, 1418.274, 1455.084, 1479.796 | 0.407717 | 0.407622 | 0.393965–0.421658 |
| I | 1750.346, 1668.836, 1498.215, 1547.447 | 0.448948 | 0.446706 | 0.416171–0.486207 |

In this common longer window, I/F is +11.82% by mean and +13.21% by median; I/H is +10.11% and +9.59%. I is slower than H in each corresponding round. F/H differs by +1.55% by mean. Four samples still do not identify every host or compiler-layout influence, but this result retains a concrete I increment that the short H/I batch did not resolve. The complete data are in `/tmp/slay-ui10-focused-benchmark/combat-balanced-long/receipt.json`.

### Measured idle-off guard adopted for K

The resulting candidate changes only two guards. `App::advance` skips the hold helper when Long Press is off **and** no owner, remaining timer or unfinished fired fade requires work. Owner cancellation, released-timer decay and the 250 ms fade therefore retain their existing progression. `native_end_turn` evaluates and paints the hold bar inside the Long Press condition. This does not reduce animation fidelity, disable the feature, shrink a cache, or add a pool.

The private archive keeps I's exact member order and replaces only `app.cpp.o`; every other object member has the same SHA-256. Headers and App layout are unchanged. The native benchmark's global App has the same address `0x38d0d0` and size `0x3104a0`, and `App::render` retains its address and size. Thus this comparison does not silently move the framebuffer or change its alignment. The compiler retains the guard changes: `App::advance` grows from `0x603` to `0x653` bytes and `native_end_turn` from `0xcb7` to `0xcd8`, so smaller machine code is not the explanation being asserted. The existing hold regression passes all 1,696 threshold, ownership, cancel, fade and source-bar checks against the private implementation. Its build record is `ui10i-private-off-ordered-build-04a5c28e0a3a.json`.

Formal `ui10i-quiet-private-off-guard-fbb70cb2a89b.json` compares I and that candidate in four alternating AB/BA pairs, again with 3,600 frames per sample. All 14,400 paired frame fingerprints match, original/before/after canonical bytes remain exact, and all 28,800 rendered frames have zero measured warm source I/O.

| Pair | I CPU ms per 3,600 frames | Guarded candidate CPU ms | Candidate change |
|---|---:|---:|---:|
| 1 | 1435.104 | 1390.109 | -3.14% |
| 2 | 1443.469 | 1437.222 | -0.43% |
| 3 | 1788.794 | 1379.947 | -22.86% |
| 4 | 1448.716 | 1326.001 | -8.47% |
| Mean | 1529.02075 | 1383.31975 | -9.53% |
| Median | 1446.09250 | 1385.02800 | -4.22% |

The candidate is faster in all four paired samples. The unusually high third I sample remains in the mean and ranges; it makes the 9.53% mean improvement more optimistic than the **approximately 4.2% median improvement for this one scene**. Mean per-frame CPU is 0.424728 → 0.384255 ms; median is 0.401692 → 0.384730 ms. The result supports adopting these bounded guards, while preserving the initial regressions and all subsequent samples. It does not establish a general frame-rate gain or ARM/device FPS. Raw samples, input/binary/source hashes and both successful and failed private build attempts remain under `/tmp/slay-ui10-focused-benchmark/combat-off-long/` and `/tmp/slay-ui10-off-branch/`.

K adopts the two measured source changes exactly and adds App-level checks for an off setting with a retained owner, an ownerless remaining timer, and an unfinished fired fade; the short source regression passes 1,731 checks. The final remotely verified runtime checkpoint is `61c5a93acbd9730ab345a4add8293bda744820f0`, tree `3e155a32748e3917ba8f369de4da7cd8678389fe`.

Formal K association `9a986cce7aaf` verifies all 25 official core object members byte-for-byte against the measured private ordered archive, including identical member order. The official 5,781,550-byte core has SHA-256 `e6ca0482ad1bc97867f2c89f68120557d5529da3fc6f0fdea216193112f53eaa`. Linking the same driver and Release flags against it produces the exact previously measured 7,756,600-byte ELF, SHA-256 `431e336cd0fc591029d75ac8bb60700549771557ac6f19641675becfd8b2b571`. This directly associates the approximately 4.2% scene-specific median improvement with the final native production code; no additional timing matrix is substituted for the original samples. The association receipt is `/tmp/slay-ui10-ordinary/associate-k/receipt.json`. Final K ordinary acceptance is recorded separately below; the completed I 9.8-second physical hold and maximum-capacity executions remain labeled I.

## Memory and ordinary capacity acceptance

The foreground map/transfer union saves 617,324 static bytes without reducing map, transfer, image or glyph capacities. The verified F ARM allocation, including Zehn prefix and loaded image/BSS, is 16,213,224 bytes, leaving 563,992 below the unchanged 16 MiB limit. These linked allocation bytes are distinct from disk pack size and transient stack use. Appended-font compaction saved 21,498 bytes in D (English 1,046; Chinese 20,452), reusing the existing glyph-group format.

The independent ordinary calculator clone initially exposed genuine E capacity failures. Packed and raw-backup cases passed, but raw-history and all-raw writes failed; both failures remain recorded. Original run/backup/history/language files were restored exactly before F. F reused the already fully read-back identical common artwork, uploaded/read back the new launcher and both NSL2 locales, and then passed all four actual write/readback cases:

| Capacity case | Fixture and explicit padding bytes | Ordinary F journal | Result |
|---|---:|---|---|
| Packed profiles | 1,025,362 | `fed17db23002` | Passed all writes and exact readbacks |
| Raw backup migration | 1,419,637 | `03e41fc64562` | Passed all writes and exact readbacks |
| Raw history migration | 2,083,681 | `54992f521f6a` | Passed all writes and exact readbacks |
| All raw, specified history-write peak | 2,699,304 | `9d365e39de21` | Passed all writes and exact readbacks |

The upper-bound padding is a separately identified reserve file, not a purported valid save. The original independent NAND is never written. F restore `64635656ba78`, bilingual ordinary Continue/Save & Quit `ffafca969ff2`, and subsequent cold restart/Continue/Save & Quit `b2afd3a2c7f4` all pass. English → Chinese → English uses the ordinary settings UI. The canonical NSV17 run remains byte-identical to the preinstall real run: 73/80 HP, 109 gold, floor 2, eleven-card deck. History playtime changes normally. Independent NSZ1 unpack plus F-core run/history decoding and exact canonical round trip are recorded in `/tmp/slay-ui10-ordinary/reopen-f-validation/canonical-validation.json`. The emulator was stopped normally afterward. The following H/I results independently cover the subsequent NSH9 trailer and final installation; they are not inferred from F.

### NSH9 and H/I installation acceptance

H (`0eb82c19b8794f5235e6c5283d339f0c93f3a21f`) exported the actual maximum NSH9 profile: 295,885 raw bytes, exactly 536 more than NSH8. The actual Ndless storage writer produces a 74,535-byte NSZ1 container for that fixture; independent unpack and the matched core reproduce its bytes exactly. The valid C++ exported maximum NSH8 **and** NSH9 records also pass the actual web storage recovery/write-denial checks. H's full common, both locales and launcher were uploaded and completely read back through ordinary USB.

| H concrete capacity case | Fixture and explicit padding bytes | Ordinary journal | Result |
|---|---:|---|---|
| Packed profiles, history update | 1,027,498 | `cab403f00948` | All writes/readbacks exact |
| Raw backup migration, history update | 1,421,773 | `79ac82bc4c82` | All writes/readbacks exact |
| Raw history migration | 2,085,823 | `edb6a2da92aa` | All writes/readbacks exact |
| All raw, specified history-write peak | 2,701,448 | `75b5095ace0b` | All writes/readbacks exact |

The last case is `4 × 295885 + 6 × 252928 + 340 = 2701448`. The actual run fixture is 250,964 bytes, so 11,784 bytes of separate reserve padding represents the six run records' maximum-size difference. Padding is not represented as valid save data. It models the clean serial history-write peak; the corresponding clean run-write phase is 2,658,491 bytes. The packed/migration cases are their stated history-update scenarios, not individual claims of every save phase's maximum. None of these totals claims to include arbitrary stale backup files left by unrelated cleanup failures.

H ordinary English/Chinese Save and Quit migrates the real 17,731-byte NSH8 metadata to 18,267-byte NSH9 while preserving both copies of the actual 230,540-byte canonical run. The observed saved revision matches canonical revision 24; old unknown build IDs remain unknown. Separate Chinese and English cold reopens both pass. These are H executions, preserved in `/tmp/slay-ui10-ordinary/h-ordinary-summary.json` and its exact copied private proof.

Final I resources are fully byte-identical to H: common 84,834,792 bytes, English 1,204,642, Chinese 4,074,793; total **90,114,227 bytes**. I ordinary upgrade `ae9985261afc` replaces and fully reads back the 1,490,840-byte launcher, explicitly reusing the complete verified H resource readbacks after checking each new input's equality. The installed four files total **91,605,067 bytes**. I's linked allocation is **16,256,312 bytes**, leaving **520,904 bytes** under the unchanged 16 MiB limit. This is the loaded prefix/image/BSS budget; it does not turn disk pack bytes into runtime resident memory.

I repeats the maximum clean storage case with its actual 108-byte NSPF5 preference file: ordinary `7fc8225026e4` writes and reads all sixteen files exactly at 2,701,448 bytes, then restores all real profile files. The OS status screen reports approximately 1.5 MB remaining. The earlier H four-case evidence is kept under H, rather than relabeled as four new I executions.

The new Long Press preference is exercised through the real Settings UI, from default off to on. Its persisted record is exactly 108 bytes, version 5, flags 644 (bit 7 set), with the other defaults and both calculator key banks unchanged. Actual play then proceeds from the original reward room to the next enemy. A 250 ms physical E release shows the source playable-cards tutorial; another held press is canceled by opening Pause. After both cancellations, the saved 230,556-byte combat state is identical to the before state. A later E press held continuously for 9,800 simulated milliseconds crosses the enemy action and next player turn but fires only once. The saved result, turn 1 → 2 and HP 73 → 69, is byte-identical to the matching I core applying exactly one `end_turn` to the before state. This is a functional emulator test; the separate native oracle establishes the strict 500/501 ms boundary.

The original floor-2 run, history and English marker are restored afterward while independently retaining the real NSPF5 preference. Final I cold execution `07737e016f0c` shows the persisted checked control, continues that original run, saves and exits normally. Matched-core validation `fe762b8c00b2` confirms that primary and backup canonical bytes still have the original SHA-256 `a2af0f8668388320906812bf1e476c06fd96665c38519cc3cf5a5fc392e56bc6`; NSH9 records the actual successful save time/revision. The original NAND hash remains unchanged and the emulator is stopped. The complete I record is `/tmp/slay-ui10-ordinary/i-ordinary-summary.json`, including normal command transcripts, exact readbacks, the retained preparation failure and the excluded title/patch-notes detour before the actual combat measurements.

### Final K ordinary installation and cold reopen

K's formally regenerated resources are byte-identical to H and I, including both logical NSP2/39 files and all three installed resource files. Ordinary installation `1f6bbbeca0eb` checks the new inputs against H's complete USB readbacks, replaces the final 1,490,738-byte launcher and reads every launcher byte back exactly. The installed four files total **91,604,965 bytes**, 102 fewer than I. Formal linked ARM acceptance `4413aa9add78` passes with **16,256,400 bytes** allocated for prefix/image/BSS, leaving **520,816 bytes** under 16 MiB.

K cold execution `9808e0b2a6b0` starts normally from the independent calculator clone, displays the persisted Long Press setting, switches it off and back on through Settings, continues the original floor-2 reward room, saves, and exits normally to the OS. All seven LCD captures are reviewed. All five run/backup/history/language/preference files are fully read back. Matching K-core validation `a2b145331ee7` independently unpacks NSZ1 and round-trips both canonical copies exactly: the original 230,540-byte run SHA-256 above is unchanged, NSH9 records the actual successful save time bound to revision 24, and the 108-byte NSPF5 record retains flags 644 and both original key banks. The original NAND is unchanged and the emulator is stopped. The closed summary is `/tmp/slay-ui10-ordinary/k-ordinary-summary.json`.

This final execution covers the two adopted idle guards and normal persistence with the matching K launcher. H's four capacity scenarios and bilingual cold checks, and I's real long hold and maximum-capacity execution, retain their actual checkpoint labels. Their resource/record sizes remain unchanged in K; they are not reported as additional K executions.

The requested 30–40 MB resource target remains unmet. The delivered storage work preserves exact art/alpha/animation data and closes the measured migration-capacity failure; it is not presented as reaching that separate target.
