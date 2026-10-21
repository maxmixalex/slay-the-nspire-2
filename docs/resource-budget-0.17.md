# Resource storage and access budget — 2026-10-21

The current complete file is **80,846,058 bytes = 80.85 MB = 77.100809 MiB**.
It preserves the existing rendered-frame bytes, dimensions, sparse geometry,
timing entries and pivots. This storage result does not settle the separate
original-UI fidelity or calculator performance requirements.

The owner's new goal is approximately **30–40 MB on the calculator**. Source21's
885,145-byte launcher and resource file total **81,731,203 bytes** before saves.
A 40,000,000-byte combined target leaves 39,114,855 bytes for resources, requiring
another **51.62%** reduction of this resource file. Interpreting 40 M as 40 MiB
still requires **49.21%**. Neither target has been achieved or promised.

## What occupies the existing file

The actual production conversion manifest accounts for every byte:

| Component | Bytes |
| --- | ---: |
| Compressed tile and frame-index payloads | 60,642,892 |
| Retained images | 15,734,280 |
| Glyph payloads | 2,649,180 |
| Strings | 295,558 |
| Base directory | 1,011,812 |
| Frame descriptors | 462,560 |
| Group headers and lookup | 47,360 |
| Global tile header | 64 |
| Alignment | 2,352 |
| **Total** | **80,846,058** |

The 452 compressed frame-index groups occupy 6,478,949 of the payload bytes.
The remaining tile payload is 54,163,943 bytes. There are 497,344 unique tiles
and 4,719,668 tile occurrences. The current algorithm already deduplicates
16-pixel tiles globally, keeps original sparse geometry, and compresses bounded
groups with LZMA2 using a 512 KiB dictionary.

## Sprite Dicing: useful reference, overlapping mechanism

The inspected upstream revision is
[`elringus/sprite-dicing@fd5a865`](https://github.com/elringus/sprite-dicing/tree/fd5a865525ae257bf35fc05bd5f16b0a78ee0e5d).
Its [README](https://github.com/elringus/sprite-dicing/blob/fd5a865525ae257bf35fc05bd5f16b0a78ee0e5d/README.md)
describes discarding repeated regions and reconstructing sprites from an atlas.
Its advertised reduction concerns five illustrative visual-novel textures;
it is not a reduction factor to apply again to this already deduplicated pack.

The actual implementation uses a fixed grid and exact pixel identity. It can
omit fully transparent units, crop their alpha bounds, merge units when their
occurrences share the required adjacency, and represent solid regions compactly.
It does not perform general cross-frame motion estimation, rotation matching,
or arbitrary variable-shape deduplication. Its atlas/mesh reconstruction assumes
a renderer different from this CPU RGB565 calculator renderer.

Upstream atlas padding and UV handling also have a different equivalence scope.
The project's byte-level oracle must continue checking original geometry,
pixels and alpha; calling an upstream method lossless does not replace that
check or establish identical calculator filtering at tile boundaries.

### Measured transparent-cropping diagnostic

A read-only census of all 497,344 current unique records found only **101** fully
transparent tiles, **12** uniform full 16 × 16 tiles, and **zero** pixels with
nonzero hidden RGB under zero alpha. Those simple redundancies are already small
after the existing global deduplication.

A bounded experiment kept all original bytes, cropped the exact nonzero bounding
rectangle, included a group-local 32-bit offset table, and used the same LZMA2
parameters. Across 13 declared groups / 8,424 tiles, compressed payload grew
from **926,244 to 944,638 bytes (+1.986%)**. Every reconstructed 770-byte record
matched its input. Smaller uncompressed rectangles did not produce smaller
compressed storage for this sample. No whole-file projection is made.

The raw-spool SHA-256 is
`ea532ca4a5c3d078d68443ac110c55027b6865ec5f028e041ffaf2e7fa9081eb`.
The sampled group IDs are 0, 1, 66, 133, 199, 266, 332, 399, 465, 532, 598,
665 and 731. Original-derived inputs and diagnostic receipts remain private.

## Streaming, validation and caching solve different costs

Range reads avoid making the entire 77 MiB resource file resident. They do not
reduce the number of stored bytes needed for a complete offline catalog.
Current loading still reads every file byte for FNV integrity and validates the
complete metadata/index catalog. It then decodes image pixels on first use,
validates them completely, and caches successful results. See the precise
[NSP2/15 validation contract](content-tiles-0.17.md).

Source21 Clang 18.1.3 `-O2` host measurements observed **1.562–2.230 seconds**
for v15 Content open, versus **4.287–8.089 seconds** for v14 in separate runs.
The callbacks were 118,308,847 versus 733,325,569 bytes. Filesystem page-cache
state was not controlled and other jobs were running; these are host timings,
not calculator startup predictions. Construction and App initialization are
outside this benchmark interval.

A concrete cache problem remains in source21. Clip 967 has 23 distinct images
of 66,749–76,294 decoded bytes, totaling 1,557,577 bytes. All enter the old
eight-slot medium tier even though the total pixel-cache budget is 3,064,768
bytes. Two actual cycles therefore had zero no-read image accesses. They spent
4.748 and 5.114 seconds inside host image access and issued 15,471,204 and
11,671,220 callback bytes. The clip's recorded duration is 1.840 seconds.
This establishes repeated decoding, not a target-device frame-rate estimate.

Source22 replaces those separate tiers with **748 contiguous 4 KiB pages and
90 LRU entries**, preserving the complete old pixel-plus-bookkeeping budget.
Entries do not move while resident. Successful hits update recency; allocation
evicts the least recently used entries until a contiguous extent and an entry
are available. Failed decoding publishes no cache key. Owner resets and exact
descriptor/shape/format identity remain required.

A metadata-only model fits all 23 images in **395 pages / 1,617,920 bytes**,
including 60,343 bytes of internal padding. It predicts 23 second-cycle hits
with no evictions, including after modeling the retained-image startup requests.
This is a model motivating the implementation. The new actual runtime benchmark,
sanitizer regressions and ARM/WASM builds must pass after the source checkpoint.

## Next lossless experiments

Two source-controlled diagnostics support reproducible measurements without
changing a production file:

- `tools/probe_tile_transforms.py` tests reversible tile channel layouts,
  horizontal prediction, exact crop, solid regions and exact indexed palettes. Each
  candidate includes its local indexing, is compressed with the same settings,
  and is inversely reconstructed byte-for-byte. Any transformed group exceeding
  the current raw or encoded bound is reported explicitly. Partial runs remain
  partial, and samples are not extrapolated to full packs.
- `tools/probe_retained_compression.py` compares individually compressed retained
  pictures and bounded groups of pictures/glyphs. Every candidate round-trips.
  Group descriptors and headers are included in the model; an unimplemented
  global lookup/header and changed random-access costs remain explicit limits.

Reversible prediction is a standard compression technique: the
[W3C PNG filtering specification](https://www.w3.org/TR/png-3/#9Filters)
defines reconstruction from neighboring samples. A useful technique still needs
measurement on these actual tiles; reordering can destroy repetitions that the
existing compressor exploits. Earlier complete-frame prediction and temporal-XOR
samples did not improve this pack's corresponding baseline.

If those measurements leave the result far above 40 MB, a more substantial
representation change would need investigation. Keeping texture regions and
animation transforms instead of pre-rendering every frame is one possible
direction; the [Spine runtime architecture](https://esotericsoftware.com/spine-runtimes-guide)
documents that representation. It introduces runtime pose/mesh rasterization,
blending and clipping costs on the calculator. No implementation, size result
or hardware performance result is claimed for that approach.

Frame removal, reduced sampling, quantization and omitted effects would change
the fidelity contract. They are not silently counted as successful lossless
compression or as calculator text-readability adjustments. The existing 95%
original-UI acceptance requirement remains unresolved.
