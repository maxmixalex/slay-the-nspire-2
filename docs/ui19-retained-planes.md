# UI19 exact retained-plane storage

## Implemented result

The RGR3 candidate is fully converted for all sixteen locales and passes the
complete native image oracle. It preserves all **58,060 images**, **1,783 clips**,
frame clocks, original RGB565 words, alpha bytes, image descriptors, glyphs and
strings. Its default English/Chinese data occupy **77,227,911 bytes**, down
**734,540 bytes** from the complete accepted UI18 NSP2/42 installation. No image,
animation frame, font role or language was removed. This is an incremental
storage improvement; it does not meet the historical 65–70 MB complete-install
goal by itself.

| Physical installation component | Complete UI18 | RGR3 candidate | Change |
| --- | ---: | ---: | ---: |
| Shared index and eight concern files | 72,727,684 | 71,991,296 | −736,388 |
| English locale | 778,844 | 779,011 | +167 |
| Simplified Chinese locale | 4,455,923 | 4,457,604 | +1,681 |
| Default bilingual data | 77,962,451 | 77,227,911 | **−734,540** |
| All sixteen locale files | 23,779,224 | 23,789,407 | +10,183 |
| All sixteen languages plus shared data | 96,506,908 | 95,780,703 | **−726,205** |

These are actual installed data bytes, not ZIP sizes. The locale directories
contain relocated pointers, so recompressing them offsets part of the artwork
gain. Add the final integrated launcher's measured size to obtain the complete
program/data installation; this report does not reuse the old launcher size as
if it were the new build.

The converter considers every one of **2,126** retained groups and replaces
**710** only when the complete aligned encoded group is smaller. Mode counts
are 68 without color decorrelation, 77 with the existing linear green predictor,
and 565 with its existing zigzag predictor; 1,416 groups keep their prior bytes.
All 1,655 tile and index groups remain byte-identical. Full decoded retained
group comparison covers **106,620,585 bytes** before publication of the common
files. Each later locale then proves that it shares those exact verified bytes.

## Format and runtime cost

RGR3 is introduced in NSP2/43. Sparse retained images normally interleave row
geometry, spans, RGB565 words and mixed-span alpha. RGR3 stores each image's
geometry first, then its color words, then its alpha bytes. The image length,
slice position and group boundary do not change. Optional RGR2 color transforms
remain reversible, and even hidden RGB underneath zero alpha is preserved.

The existing LZMA2 decoder still uses a maximum 512 KiB decoded retained group.
There is no new resident image or group pool. Synchronous restoration reuses
the already consumed encoded transfer region after the slice table; the
converter requires the complete table plus largest scratch slice to fit that
existing region. Incremental restoration releases the LZMA workspace, borrows
space from the existing group pool, and performs copy/restore work in bounded
steps. The source and destination reservations remain pinned until Ready or
cancel. The small preparation cursor grows, but the pool capacities do not.

More inverse work can still affect a cold scene. Native byte equality and ARM
instruction emulation do not establish ordinary calculator timing. The final
paired ordinary-OS Firebird test uses the same integrated ARM build and fixtures
with the old 42 data and the final 44 installation, which includes the later
retained-group and paired-tile stages. Its scene and first-visible timing results
belong in the final UI19 integration record; this codec report alone does not
claim a calculator FPS improvement.

## Executed verification

Runtime implementation checkpoint: `cb3b6e2b1ac01eb187cb822fc877590b54eae508`.
Late cancellation, pool-boundary and linked-ARM test checkpoint:
`249b485cebb52be03ee46b7df6bfde44df4dec0c`. Final converter/reader checkpoint:
`da2ba50a0d4096f2a58196006a41951cdb7795f4`.

| Gate | Recorded result |
| --- | --- |
| Complete private conversion | `ui19-retained-planes-all16-final-77f9f06737f9`; all sixteen locales completed in 380.07 s; source unchanged |
| Every original image, three runtime read paths | `ui19-rgr3-all-images-exact-e71637b5cf42`; 58,060 / 58,060 images, 1,167,930,140 decoded bytes, 247,894,180 checks; source unchanged |
| All original locale descriptions and visible glyphs | `ui19-rgr3-all16-locales-b90f2a3baf75`; 89,818 checks, 53,072 descriptions, zero failures; source unchanged |
| Synthetic retained runtime boundaries | `ui19-retained-native-all-phases-02afe52eb4db`; 6,552,231 checks, including every pending cancellation step and 512 KiB raw + scratch on a 1 MiB group tier |
| Linked ordinary ARM inverse | `ui19-retained-arm-inverse-63f5950e05d3`; 119 vectors, 68 safe rejections, output/scratch guards preserved, r4–r11 and stack preserved; source unchanged |
| Public converter/package integration | `test_retained_planes_pipeline.py`; an actual synthetic RGR3 conversion passes NSM1/NSS1 partition/join, NSL3, ZIP creation and member readback, plus an identical cache rerun |

The complete native image comparison uses the actual old runtime decoder's raw
bytes as its oracle. Disk space was limited, so the existing `--images FIRST
COUNT --preload` interface covered contiguous nonoverlapping ranges whose union
is all 58,060 images. Every range checks memory, ranged reads and incremental
reads against the direct byte spool. No pixel hash substitutes for comparison.
The largest temporary spool is **131,090,340 bytes**, and it is closed between
ranges. Total duration was **222.55 s**. Metadata and UI checks repeat per range,
so its higher aggregate check count is not a throughput comparison with an old
single-range run.

That bounded run records 19,157,358 preparation microsteps, with a maximum of
11,746 for one image. These are calls within the frontend's existing time/step
budget, not one display frame per step. Cold-cache boundaries occur again at
each range, so these totals are not ordinary-OS scene latency measurements.

The exact executed orchestration script is committed as
`tools/verify_content_images.py` (identical bytes to the private driver used by
the recorded run, SHA-256
`beeacdc9df49136bbbcf11a75f06a11bc5bed4a3465d3c296c4740034ae96a6b`).
It invokes the existing native executable under the Linux dynamic loader,
derives contiguous ranges from the pack's decoded-cost table, requires every
range's successful byte-comparison and preparation summary, and only marks the
receipt complete after its covered image count equals the full catalog. Its
arguments are `--binary`, `--pack`, `--reference`, `--output`, and optional
`--spool-mib` (128 in this run). Run it through `tools/run_logged.py` with the
selected source checkpoint; the receipt records both pack hashes, the actual
native executable hash, every range and the aggregate totals.

The ARM gate executes the integrated ELF with SHA-256
`c8df6b988a2ad0546b251d598f847fdc72ffe6250414110b979acc71ef081c8d`
using the private official-PyPI Unicorn 2.1.4 test dependency. It covers all
65,536 RGB565 values, dense alpha, palettes and indices, sparse empty rows,
240 rows, 320 spans, odd offsets, legacy modes 1/2 and new modes 8/9/10. This
checks the linked instruction/ABI path; it does not replace Firebird execution.

The initial converter attempt reached all 2,126 encoded choices but exceeded
its 600 s allowance. A resumed run then caught a real cross-module integration
failure: the concern splitter's generic reader still rejected logical version
43. `da2ba50` centralizes current host-readable version limits in the
standard-library-only `content_version.py` and adds the complete public pipeline
test. Historical builder output versions and codec-introduction thresholds
remain explicit. Both failed journals are retained; they are not reported as
successful all-locale conversions. The final run above passed. The first native
GCC 13 build also encountered an unrelated compiler internal error in the
existing `world_relics.cpp`; the selected native checks used Clang 18.

## Other measured directions

Separating RGB/alpha fields in already filtered tile groups was rejected: six
byte-weighted sample groups grew from 287,282 to 342,932 bytes even after choosing
the smallest tested variant, **19.37% larger**. It is not part of this output.

Retained sparse field separation was initially promising across eight weighted
groups (184,338 to 171,359 bytes), then implemented and measured across the
complete catalog as reported above. A further adjacent-retained-group merge
experiment is separate: it changes group boundaries and therefore requires its
own raw-slice, concern-boundary and cold-load verification. Do not attribute
sampled or unfinished merge savings to these delivered RGR3 bytes.

## Reproduction and provenance

Run with the same complete private UI18 data directory and the recorded source:

```sh
python3 tools/run_logged.py --checkpoint da2ba50a0d4096f2a58196006a41951cdb7795f4 \
  --label retained-planes-all16 --timeout 1800 -- \
  python3 tools/compact_retained_planes.py \
    --input "$COMPLETE_UI18_DATA" --output "$NEW_DATA" \
    --work "$PRIVATE_WORK" --workers 2
```

The actual private output is `private-ui19-storage/planes/data`, with
`retained-planes-receipt.json`. It has the existing nine shared filenames and
sixteen `nSpire2.<locale>.dat.tns` files. English and Simplified Chinese logical
verification inputs are retained under `planes/work`; the other fourteen full
logical intermediates are not duplicated on disk.

English NSP2/43 logical SHA-256 is
`e58ad92fc84ed9963073f933551d28658aa7291755ac6e111bf668c1304c1b66`.
The common index is
`ea2db4aed2683f956111311c7d7dbf76f62814bcdb8f334e285d350ea2af6d3e`;
English locale is
`9873bb838e939a027f7ebe588dae85ca048ec9abf94fc90f1a8596ff7ec05024`;
Simplified Chinese locale is
`2417ba1912c4ca014e221477dc60f7f4ac57ad989f8f52383983a0827d2f923d`.

The new logical resources require a launcher that supports NSP2/43. Replace
the launcher, common index, all eight concern files and desired locales as one
matched installation. See [content regeneration](content-regeneration.md) for
the distinction between this verified storage conversion and importing a later
original-game release.

The round's public source push was rejected by automatic approval review. These
are local committed source checkpoints and source-only bundles; no remote
publication or readback is claimed here. Private original assets and generated
packs remain outside the public source tree.
