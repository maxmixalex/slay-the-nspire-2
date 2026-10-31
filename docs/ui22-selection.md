# UI22 character-selection storage and rendering

This document records the implemented selection path and its controlled
measurements. It is not the final installer-size report or a physical-calculator
FPS result. The complete original-color resource control, its source provenance,
and the remaining release gates are described separately in
`ui22-source-precision.md` and `WORK_STATE.md`.

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
The NSP2 version remains 45; the complete selection catalog adds five explicit
auxiliary registrations, taking the preparation catalog from 7,206 to 7,211
entries. Subsequent complete-profile registrations may append further entries.

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
rendering, counts actual file-reader requests, compares complete frame hashes,
and verifies that the canonical gameplay save does not change. Setup, first
render, first 450-frame window, later windows and rapid character switches are
reported separately. These timings do not include calculator flash latency,
display transfer or the real ARM processor's cycle costs.

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
decodes. These controls must be bound to the final accepted resource streams
after subsequent storage changes.

## Storage scope and rejected candidates

The restored original-color English logical control is 103,133,990 bytes.
That is a development control, not a calculator-ready bilingual installation.
The eight-record geometry layout is 104,817,618 bytes before the other accepted
index work. Exact TGI2 index selection and the same-tile refinement combine to
103,797,230 bytes at checkpoint 944a. Those numbers are logical English packs;
they exclude the launcher, Chinese locale payload, optional locales and ZIP
container overhead. The final packaging report must state all of those
separately and must assess actual installed capacity.

Masking and re-dicing unused tile colors increased this dataset by 3,561,218
bytes. A separate XOR residual representation added 2,154,078 bytes. Both were
rejected; the runtime retains ordinary exact copy patches. Attempts to optimize
the Random particles also failed whole-frame timing checks and were reverted.
No reduction in particles, source frame count, density or color precision is
part of the accepted selection improvement.

The independent linked-ARM gates use the actual production functions. The
in-place tile inverse checks both 0 and 2 mod 4 addresses through the complete
1 MiB group size, output and scratch guards, and AAPCS registers. The LZMA2 gate
accepts the supported dictionary properties 14 and 16 and rejects 15 and 17.
These checks corrected stale test assumptions about an unused out-of-place
symbol and the old 512 KiB-only limit; they did not change runtime behavior.

