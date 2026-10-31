# UI22 character-selection storage and rendering

This document records the implemented selection path and its controlled
measurements. It is not the final installer-size report or a physical-calculator
FPS result. The complete original-color resource control, its source provenance,
and the complete release acceptance are described separately in
`ui22-source-precision.md`, `ui22-storage-performance.md` and `WORK_STATE.md`.

## Exact representation and bounded replay

The five existing background clips retain all 540 source samples, their IDs,
their dimensions and their clocks. The offline conversion changes their index
geometry, while retaining the original RGB565 tile streams. Frame zero is the
complete opaque image. Every eighth frame is a checkpoint relative to frame
zero; the intervening records copy pixels that differ from their predecessor.
A sequential render applies one new patch. A random jump restores frame zero,
the relevant checkpoint and at most seven intervening patches. It cannot grow
into an unbounded replay of the full animation.

`tools/character_select_delta_ui22.py` reconstructs every output frame and
compares its complete 320×240 RGB565 hash with the source composition. It keeps
the static background planes, particles, animation samples and original
colors. It introduces no alternate pixel decoder and no color quantization.
The selection conversion itself emits NSP2/45 and adds five explicit auxiliary
registrations, taking the preparation catalog from 7,206 to 7,211 entries.
The final UI22 release uses NSP2/46 and 7,212 entries: its additional marker
binds the complete profile after proved unreachable legacy frames are retired.
Ordinary implicit geometry is an independent later storage step. The final
selection frames retain their explicit copy spans; the FFFC bitmap candidate
is not adopted.

The renderer stores the current opaque image in the existing scene-raster
union, which is also used by Random, the merchant and the selected card
description in their respective scenes. The union does not grow. Claiming a
different member invalidates its identity; the next build writes its live
contents before marking it valid. Both `App::init` entry points also reset the
owner metadata, including the case where a destroyed App is reconstructed at
the same address and its Content generation restarts at one. The regression
covers both in-memory and ranged-file initialization in that situation.

## Cold loading and cache budgets

The first key/patch candidate improved warm rendering but decoded too many
future frames' span records on first entry. Merely eliminating warm resource
reads did not resolve the reported selection lag. The accepted grouping stores
at most eight selection index records per independently decoded group, and
never joins the end of one character's clip to the start of another's clip.
This limits cold geometry work to a checkpoint-sized group.

The existing 12.75 MiB picture allocation is repartitioned, rather than enlarged.
Ordinary scenes use 6.75 MiB for decoded images and 6 MiB for tile pages. Silent
and Defect use 8.5 MiB and 4.25 MiB respectively. Their larger span sets fit the
decoded-image cache while retaining bounded tile space. The offline builder
checks loop budgets of 1,600 / 2,000 / 2,000 / 1,600 / 1,600 decoded 4 KiB pages
for Ironclad, Silent, Defect, Regent and Necrobinder.

Two exact span-merging steps reduce geometry overhead. A loop-wide gap limit
first trades copied unchanged RGB565 pixels against five-byte span headers
within its fixed page budget. A second pass uses each frame's unused final-page
capacity: adjacent spans can join only when their gap is inside the same
physical 16-pixel tile. It proves that every frame retains the same decoded
page count and active tile set. This second pass removes 104,606 span headers,
copies 670,802 additional unchanged pixels and changes no source image reads
through adding a tile. It saves 126,752 bytes of encoded geometry in the
measured full-precision selection pack; the later combined rewrite also removes
708 bytes of existing alignment padding.

## Controlled whole-frame results

The native harness renders the actual App, including the UI, background,
particles, source reads and decompression. It records process CPU around
`App::render`, counts actual file-reader requests, compares full-LCD
fingerprints, and verifies that the canonical gameplay save does not change.
The last same-core gate additionally measures selection `App::advance` and
its resource reads, and reports advance plus render separately. FNV, transcript
writes, fixture construction, initialization and save comparisons are outside
these frame timers. Selection cold means the first render after setup; combat
initialization/preparation has a separate timer. Host filesystem cache is not
forcibly cleared. These are native process CPU and requested reader work, not
wall-clock presentation time, calculator flash latency or physical-device FPS.

The first formal grouping comparison used the same d940 runtime on both sides,
with the restored full-precision resource control and the eight-record
key/patch layout. An ABBA order checked 2,255 corresponding rendered frames.
The arithmetic means of the two runs per layout were:

| Character | Control: first 450 renders, CPU ms | Key/patch: first 450 renders, CPU ms |
| --- | ---: | ---: |
| Ironclad | 594.676 | 341.310 |
| Silent | 2,075.878 | 1,516.702 |
| Defect | 2,461.642 | 1,350.976 |
| Regent | 1,662.148 | 943.079 |
| Necrobinder | 2,679.350 | 724.828 |

All five first-window p95 values improved in that comparison. Later complete
windows read zero source bytes for the five character backgrounds. Cold single
frames were not uniformly faster: a relative-to-first checkpoint can still
fetch a different set of source groups, and individual maxima include host
scheduling variation. For example, one Necrobinder checkpoint read 709,629
bytes, despite the full first window dropping from about 34.4 MB to 7.17 MB.
The release cannot infer a guaranteed device frame rate from these means.

The same-tile refinement was subsequently tested with the a37 runtime in both
AB and reverse BA order. Each comparison matched 9,006 complete frames,
including Random and rapid switching. The five first-window means decreased
from 372.7 / 1,599.8 / 1,353.5 / 973.5 / 689.3 ms to
351.3 / 1,561.0 / 1,201.5 / 926.9 / 687.2 ms. Warm source reads remained zero.
Rapid-switch means varied from 721.2 to 739.9 ms while their actual source bytes
fell from 4,718,935 to 4,699,010. This small host-time variation is reported;
the representation's per-frame page and tile bounds are exact assertions.

The full-precision combat control also covers seven complete App scenes:
ordinary idle and Strike, Necrobinder with Osty and an actual Unleash, and
Ceremonial Beast idle, attack and Stamp-to-Plow feedback. Across 6,300 frames,
the selection-data rewrite preserved every rendered hash. The final warm
300-frame window in each scene had zero source reads and retained-group
decodes. The final resource verification retains this complete-LCD comparison, while
the separate source and Canvas oracles compare exact original bytes and pixels.

## Final ordinary-geometry performance gate

The final ordinary FFFD encoding is measured with the same b942 production
core and identical harness binaries on both sides, fixed to one allowed host
CPU, in ABBA order. The data control is `b9bfbb44` (NSP2/45); the ordinary
implicit-geometry candidate is `a467fea1` (NSP2/46). Each pass covers 8,106
selection renders, 900 rapid-switch renders, and 6,300 combat window renders.
All corresponding LCD fingerprints match. The later final orphan-record pass
only removes records without any image descriptor and passes its own complete
linked-ARM gate; it does not replace this data comparison with a renamed receipt.

All four passes record **zero selection resource bytes in `App::advance`**.
The following values sum advance and render CPU for the first 450-frame window;
the ranges show the two repeats per side. Initialization and hashing are excluded.

| Character | Explicit geometry CPU ms, mean (range) | Ordinary FFFD CPU ms, mean (range) |
| --- | ---: | ---: |
| Ironclad | 369.251 (358.644–379.858) | 365.641 (358.664–372.619) |
| Silent | 1,533.416 (1,530.371–1,536.461) | 1,481.835 (1,447.554–1,516.116) |
| Defect | 1,234.019 (1,230.669–1,237.369) | 1,294.102 (1,257.547–1,330.656) |
| Regent | 834.954 (829.848–840.061) | 833.640 (818.972–848.309) |
| Necrobinder | 687.678 (667.397–707.958) | 692.269 (688.585–695.954) |
| Random | 600.258 (582.757–617.758) | 632.455 (619.637–645.273) |

Requested selection bytes are identical for each corresponding window, and
warm no-read windows still vary in both directions. These percentages do not
establish an FFFD-induced selection speedup or slowdown. Rapid switching
measures 745.656 → 696.496 ms for 900 advance-plus-render frames, with the same
4,629,565 source bytes. The substantive selection improvement remains the
bounded key/copy layout and cache reuse tested against the full-frame control.

Combat provides a separate useful acceptance signal. Across seven scenes,
initialization/preparation measures 554.627 → 488.729 ms and
20,162,400 → 19,528,978 reader bytes. The first render after preparation totals
15.786 → 14.695 ms. The 6,300 window frames total 3,705.156 → 3,527.798 ms for
advance plus render, with 1,422,556 → 1,364,734 requested bytes. Unleash and Plow
opening 300-frame render windows measure 206.247 → 187.021 and
223.278 → 200.312 ms, accompanied by lower reader work. Individual zero-read
warm windows retain host variation, so the release does not claim every frame
or every encounter is faster on a calculator.

The FFFC selection bitmap experiment was separately compared with identical
b942 cores. It saved 163,840 logical bytes, but the affected Defect and
Necrobinder opening windows measured +2.86% and +4.43% CPU and rapid switching
+3.86%. Unchanged controls also varied; the experiment did not establish an
acceptable performance benefit. **Final resources contain no FFFC records.**
Its offline converter requires explicit experimental opt-in and identifies
its output as not release-approved. The unused runtime decoder remains bounded
within the same scratch allocation.

## Final storage scope

Final logical English is **94,924,498 bytes**, SHA-256
`e2fc2beb4fe1e4d9ff5e54188f0c134bc9da2ee680ef2acc06ad5f7cd6784147`.
The original-color/native-density control is 103,133,990 bytes. The difference
is 8,209,492 bytes, but the catalogs are not literally identical: the final
pack adds selection/retirement metadata and removes proved unreachable
legacy payloads. Source precision, visible sampled frames, clocks and active
content are the controlled quantities. No lossy color reduction is a saving.

The final removal of 988 physical index records is based on every current
image descriptor, including aliases and rarely used routes, rather than
animation-name guesses. All 21 affected groups retain every live record byte,
update descriptor offsets, and keep group IDs and per-image cache costs. They
save **249,856 bytes** after suffix alignment. The actual final ARM ELF takes
96,820,329 → 11,989,047 instructions for their complete LZMA2-plus-index-inverse
work; every group improves, with the smallest reduction 17.53%. This is a
bounded decoder-work result, not a whole-game FPS claim.

| Installation comparison | Required launcher + bilingual `.tns` bytes |
| --- | ---: |
| UI22 full-precision/native-density control | 107,729,436 |
| Final UI22 | 99,526,983 |
| Historical UI21, with inherited lossy colors | 73,295,445 |

UI22 saves **8,202,453 bytes** against the same-quality active-content control,
while occupying **26,231,538 bytes more** than the lower-precision historical
UI21 release. The fourteen optional locales add 18,854,842 bytes if all are
installed. ZIP sizes, documentation/license bytes and mutable saves are
separate from these required-runtime counts. Exact archive identities and
complete extraction totals belong in the final delivery report.

Masking and re-dicing selection colors increased data by 3,561,218 bytes;
a separate XOR residual representation added 2,154,078 bytes. Both were
rejected. Random-particle speed candidates also failed whole-frame comparisons
and were reverted. No reduction in particles, source frame count, native density
or color precision is part of the accepted selection improvement.

The final b942 ARM launcher has a formal allocation of 22,535,324 bytes, leaving
2,630,500 bytes below 24 MiB. This includes the 230,400-byte ordinary-geometry
scratch; it is not a measured calculator peak-memory trace. Its 4,945 linked
ARM vectors cover dense/sparse inverses, LZMA2, memory bounds and AAPCS.
A recursive 33-symbol, 45,556-byte machine-code/constant/relocation comparison
binds the earlier complete-stream profile measurements to the final linked
helpers. Native full-resource and scene gates cover the additional NSP2/46
reader paths. The alpha-to-geometry reconstruction itself has not been
instruction-counted through a complete `Content::image` load on ARM. The helper
comparison alone is not claimed to validate that path or prove it faster on
a calculator.
