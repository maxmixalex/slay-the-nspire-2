# UI6: measured host card and merchant scene cost

The clean merchant inventory uses less time in the measured warm windows: **18.8–19.6% for idle inventory and 23.8–27.8% while a card is hovered**. Opening the inventory itself is roughly unchanged in English and modestly lower in Chinese. Room initialization remains a separate cost, and merchant warm resource reads increase. These are whole-version host measurements; they are not physical-calculator FPS or an isolated cache gain.

The final comparison uses runtime `03a3ad36a3607899fabc1d92fa986bdbb16c51c9` with its original UI5 packs and runtime `96f7b5c6914762fad998f03ab5da1b4c283ecaa0` with the compacted UI6 packs. Both link the same driver from checkpoint `e9d4017629219a28f072fb54bcb0327fb538f121`, SHA-256 `9e35386789b8f1beb1530a6b04d33ea9c95139f74971b12d4242486824797eed`, built with Clang 18, `-O3 -DNDEBUG`, and debug commands disabled. Production runtime and resource files were unchanged by the driver correction.

## Method and acceptance

Each language has three A/B pairs. English uses AB / BA / AB and Chinese BA / AB / BA, interleaved by round. Every warm window advances the actual animations by 33 ms for 180 frames. Ten 250 ms settling updates precede the warm window and are excluded from its cost. Tracing is off. Builds, tests, browser checks and other agents were idle; Firebird time was paused. The timer is a host steady clock around input/update/render work without frame pacing. Operating-system storage cache history is not controlled.

The table reports the median of the three per-run warm means, or the median of the three first-frame intervals. The hover cold sample includes the opening frame and the first complete frame at the card’s actual hit target. Resource open, seeded room setup and first room render are measured separately. All three values, maxima, source calls/bytes/seeks, binary and pack hashes and journal receipts are in [the complete measurement record](validation-ui6-card-performance-0.17.json).

All 132 observed windows preserve byte-identical canonical state and make no save callback. The shop has seven actual visible card hit regions; its first, settled and last frame hashes differ, so the measured window keeps animating. The 132 saved captures form 44 groups with three byte-identical repetitions in each. Actual idle/hover captures were visually checked.

## Merchant timing

| Window | Language | UI5 ms | UI6 ms | Change |
| --- | --- | ---: | ---: | ---: |
| Warm idle frame | ENG | 1.847 | 1.485 | -19.6% |
| Warm idle frame | ZHS | 1.741 | 1.413 | -18.8% |
| Warm hovered frame | ENG | 1.871 | 1.425 | -23.8% |
| Warm hovered frame | ZHS | 1.993 | 1.439 | -27.8% |
| First inventory frame | ENG | 4.507 | 4.570 | +1.4% |
| First inventory frame | ZHS | 4.279 | 4.110 | -3.9% |
| First hovered frame, including opening | ENG | 5.894 | 5.423 | -8.0% |
| First hovered frame, including opening | ZHS | 6.133 | 5.375 | -12.4% |
| Room open/setup/first render, idle fixture | ENG | 5.176 | 5.813 | +12.3% |
| Room open/setup/first render, idle fixture | ZHS | 5.098 | 5.706 | +11.9% |

The room-opening row is not folded into the inventory first-frame row. In this set it rises by about 12% for the idle fixture, while the first inventory frame changes by +1.4% in English and approximately −4% in Chinese. Small changes from three samples should not be described as a broad first-open speedup. Layout, original artwork, text, cursor and caching differ together; the larger warm reductions describe their combined result.

## Ranged resource I/O

| Merchant window | Language | UI5 calls | UI6 calls | UI5 bytes | UI6 bytes |
| --- | --- | ---: | ---: | ---: | ---: |
| First inventory frame | ENG | 43 | 38 | 265193 | 249603 |
| First inventory frame | ZHS | 67 | 51 | 363552 | 306947 |
| Mean per warm frame | ENG | 0.3389 | 0.4222 | 4018.6 | 5102.8 |
| Mean per warm frame | ZHS | 0.3278 | 0.4278 | 3973.1 | 5125.5 |

Warm source reads rise from about 4.0 KB to 5.1 KB per frame even as host render time falls. The corrected artwork and other UI changes are present together, so these data do not isolate the cause of the extra reads. The measured increase is a concrete follow-up for a layer-by-layer resource profile and grouping/retention work. It must not be reported as an I/O reduction or as a proven physical-calculator speedup.

## Detail modal boundary

All four warmed detail windows and the hovered rainbow-keyword window make zero ranged resource reads in both versions. Their host timing moves modestly in either direction. The earlier detail improvement remains intact; this iteration does not demonstrate another large detail-modal speedup.

| Warm detail window | Language | UI5 ms | UI6 ms | Change |
| --- | --- | ---: | ---: | ---: |
| Library detail | ENG | 1.238 | 1.299 | +4.9% |
| Library detail | ZHS | 1.262 | 1.219 | -3.4% |
| Rainbow detail | ENG | 1.320 | 1.294 | -2.0% |
| Rainbow detail | ZHS | 1.199 | 1.241 | +3.5% |
| Removal selection detail | ENG | 1.345 | 1.392 | +3.5% |
| Removal selection detail | ZHS | 1.337 | 1.314 | -1.7% |
| Draw-pile choice detail | ENG | 1.959 | 1.979 | +1.0% |
| Draw-pile choice detail | ZHS | 1.969 | 1.843 | -6.4% |
| Rainbow keyword hover | ENG | 1.779 | 1.652 | -7.1% |
| Rainbow keyword hover | ZHS | 1.592 | 1.662 | +4.4% |

## Retained excluded run

The initial 12-run matrix completed, but its idle pointer `(10,35)` hovered the starter Burning Blood relic. Its tooltip covered part of the inventory, and the cold hover sample included that intermediate frame. Visual review found this measurement flaw. Those captures, data and receipt `acfa5b72dcbd` remain preserved and are excluded from the clean inventory summary.

The accepted driver moves the idle pointer to `(310,35)` and verifies after both the closed-room frame and the opening inventory frame that no enabled target exists there. Both binaries use this same driver, then repeat the complete serial 12-run matrix. The accepted aggregate receipt is `2419e164180a`; the baseline and candidate link receipts are `9fc2db4709b6` and `1cf28f27d552`. This correction changes the measurement fixture only.

## Resource size improvement already completed

The merchant art repair changed 1,517 existing images while retaining directory identities. Its separate lossless compaction recovered 2,555,380 bytes and verified every one of the 47,686 current images in both packs, plus locale/glyph/tile data. The corrected merchant artwork adds 201,858 bytes net. The complete UI6 common-plus-two-locale resource set is 81,543,574 bytes, 294,271 bytes larger than the accepted UI5 set. [The room implementation report](rooms-ui6-0.17.md) records those exact-byte proofs and their acceptance limits.
