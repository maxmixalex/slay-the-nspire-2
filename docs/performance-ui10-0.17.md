# UI10 performance, storage and acceptance — 0.17

## Evidence boundary

The accepted F runtime is `361c36c27f06c4905e1899b8bdc2ab281ba9b9cc` (tree `a031c1bc29513b80fbf3fd629bb07ef20fd53453`). The matched native core archive SHA-256 is `dfc981f168c37f3e795002de554ba875277c175384b4f98ece9db2f2b7942333`. These results measure that frozen implementation; later G UI/font/history changes require their own acceptance. No numerical original-game fidelity percentage is inferred from these performance or regression results.

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

## G requested-row table batch — implementation ready, formal acceptance pending

The next reader batches only the table rows needed for a single directory request into a bounded 256-byte stack array. It retains the same exact decoder, per-block bounds and foreground Bytes lease; it adds no resident pool and does not scan unrelated rows. A full four-block test now uses five physical callbacks and one initial seek: one 32-byte table range followed by contiguous block data. Partial-block reads copy only the requested literal portion; compressed blocks still decode exactly. New tests cover a three-block subrange, directory/raw-tail/common boundaries, table read failure before output, malformed later rows, retry, guards and released leases. The short linked split test passes 42,948 checks, with the adapter still 128 bytes on the host.

This G optimization is not yet a formal performance result. After the coherent G checkpoint/build, compare F and G readers with the **same physical F NSL2 files**, retaining serial AB/BA samples, CPU, elapsed, callbacks and seeks. Updated G fonts/content must be measured separately to avoid attributing input changes to the reader. The added NSH9 metadata requires a new maximum-profile migration-capacity check and real old-save conversion/Continue/cold reopen.

## Memory and ordinary capacity acceptance

The foreground map/transfer union saves 617,324 static bytes without reducing map, transfer, image or glyph capacities. The verified F ARM allocation, including Zehn prefix and loaded image/BSS, is 16,213,224 bytes, leaving 563,992 below the unchanged 16 MiB limit. These linked allocation bytes are distinct from disk pack size and transient stack use. Appended-font compaction saved 21,498 bytes in D (English 1,046; Chinese 20,452), reusing the existing glyph-group format.

The independent ordinary calculator clone initially exposed genuine E capacity failures. Packed and raw-backup cases passed, but raw-history and all-raw writes failed; both failures remain recorded. Original run/backup/history/language files were restored exactly before F. F reused the already fully read-back identical common artwork, uploaded/read back the new launcher and both NSL2 locales, and then passed all four actual write/readback cases:

| Capacity case | Fixture and explicit padding bytes | Ordinary F journal | Result |
|---|---:|---|---|
| Packed profiles | 1,025,362 | `fed17db23002` | Passed all writes and exact readbacks |
| Raw backup migration | 1,419,637 | `03e41fc64562` | Passed all writes and exact readbacks |
| Raw history migration | 2,083,681 | `54992f521f6a` | Passed all writes and exact readbacks |
| All raw upper bound | 2,699,304 | `9d365e39de21` | Passed all writes and exact readbacks |

The upper-bound padding is a separately identified reserve file, not a purported valid save. The original independent NAND is never written. F restore `64635656ba78`, bilingual ordinary Continue/Save & Quit `ffafca969ff2`, and subsequent cold restart/Continue/Save & Quit `b2afd3a2c7f4` all pass. English → Chinese → English uses the ordinary settings UI. The canonical NSV17 run remains byte-identical to the preinstall real run: 73/80 HP, 109 gold, floor 2, eleven-card deck. History playtime changes normally. Independent NSZ1 unpack plus F-core run/history decoding and exact canonical round trip are recorded in `/tmp/slay-ui10-ordinary/reopen-f-validation/canonical-validation.json`. The emulator was stopped normally afterward. This F evidence does not pre-accept the subsequent G NSH9 trailer or G installation.

The requested 30–40 MB resource target remains unmet. The delivered storage work preserves exact art/alpha/animation data and closes the measured migration-capacity failure; it is not presented as reaching that separate target.
