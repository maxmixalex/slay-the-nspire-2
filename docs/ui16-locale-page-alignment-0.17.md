# UI16 locale page alignment

## Behavior and implementation

The final multipart build exposed additional cold reads on the calculator. An
independent native replay separated the unchanged renderer from the two storage
adapters. Its NSL3 overhead came from mismatched page boundaries rather than
cache thrashing: a normal 4,096-byte virtual glyph/string page touched two
outer compressed pages.

For NSL2's raw suffix, `SplitResourceSource::read` maps a virtual address to
`tail + virtual - directory - artwork`. The directory already ends at a
4,096-byte boundary. `align_language_suffix` in `tools/split_content.py` now
places at most 4,095 zero bytes after the directory streams so that
`tail % 4096 == artwork % 4096`. `tools/storage_parts.py::compress_blocks`
applies this alignment before NSL3 compression.

No image, animation, font, text, logical offset or NSP2 byte changes. The
complete logical locale payload is checked while aligning. Repeated conversion
is idempotent. Legacy NSL1 and unpadded NSL2/NSL3 remain readable. The Python
parser accepts only a bounded all-zero gap with the required phase; it rejects
nonzero, oversized and incorrectly aligned padding. Existing runtime bounds
already allow these addresses, so this change adds no C++ code, runtime scan,
cache memory, startup preload or temporary whole-pack allocation.

The receipts retain the original input size and hash separately from the
aligned inner container. Savings are measured against the original input,
not inflated by the new padding.

## Measured locale sizes

These are actual private files produced by commit
`e95f804d2922a3b51567cec6626439ca519fce03`, compared with the first UI16 NSL3
edition. Positive deltas spend bytes to remove cross-page reads. The existing
public artwork split is unchanged by this commit.

| Locale | Previous NSL3 bytes | Aligned NSL3 bytes | Change |
| --- | ---: | ---: | ---: |
| DEU | 811,410 | 811,993 | +583 |
| ENG | 779,181 | 779,312 | +131 |
| ESP | 813,795 | 813,030 | -765 |
| FRA | 830,423 | 831,158 | +735 |
| IND | 783,866 | 784,195 | +329 |
| ITA | 808,150 | 807,242 | -908 |
| JPN | 3,427,690 | 3,428,064 | +374 |
| KOR | 1,560,644 | 1,560,820 | +176 |
| POL | 763,796 | 764,242 | +446 |
| PTB | 816,222 | 816,201 | -21 |
| RUS | 831,700 | 831,603 | -97 |
| SPA | 812,719 | 812,893 | +174 |
| THA | 852,687 | 853,068 | +381 |
| TUR | 815,871 | 815,962 | +91 |
| ZHS | 4,456,976 | 4,456,679 | -297 |
| ZHT | 4,622,935 | 4,623,096 | +161 |
| **All 16** | **23,788,065** | **23,789,558** | **+1,493** |

The largest aligned locale is Traditional Chinese: its inner NSL2 is
5,174,924 bytes / 1,264 pages, within the existing 2,048-page bound.

## Cold read isolation

All four runs use the same current native renderer, canonical Haunted Ship
save/progress fixture, ordinary Continue/End Turn inputs, fixed title/combat
warm sequence and 182 subsequent frames. The first three modes differ only
in their resource adapters. These counts are the action window; they are not
calculator frame-rate measurements.

| Storage edition | File reads | Requested bytes | File seeks | Artwork cold opens |
| --- | ---: | ---: | ---: | ---: |
| Old NSC1 + NSL2 | 17 | 103,971 | 17 | 0 |
| NSM1 + old NSL2 | 20 | 103,971 | 19 | 1 |
| NSM1 + initial NSL3 | 26 | 116,124 | 19 | 1 |
| NSM1 + aligned NSL3 | 22 | 102,505 | 19 | 1 |

All 182 RGB565 frame hashes agree across the four modes. Alignment removes
four extra reads and 13,619 requested bytes in this replay, with zero added
runtime memory. The remaining two locale reads split one 28,129-byte request
across compressed / literal / compressed pages. The separate three multipart
reads and rooms-file cold open arise from short interleaved artwork spans;
those belong to the separate concern-layout fix.

The physical trace also ruled out a simple 128- or 256-byte minimum compression
saving as a remedy: the four extra paired pages save between 631 and 1,584
bytes per block. Making those pages literal would give up compression without
fixing the general address-phase error.

## Validation and limits

- Public split conversion suite: **11 tests passed**, including new legacy
  preservation, logical-byte equality, idempotency and padding-rejection cases.
- All **16 actual aligned locales** fully reconstruct to the exact same NSP2
  bytes as their preceding editions. The private receipt records old/new pack
  SHA-256, logical resource size and logical SHA-256 for each language.
- Production C++ storage readback compares **76,368,876 English bytes** and
  **80,793,488 Simplified Chinese bytes** against the original NSP2 fixtures;
  **8,000 / 8,134 checks passed**, including reads while `ResourceTransferLease`
  owns the shared image workspace.
- The production locale/font/startup suite on the aligned files passes
  **89,816 checks, 53,072 descriptions, zero failures**. This includes the
  existing 90 selected-language corruption/failure recoveries to English.

Private artifacts are in `private-resources/ui16-aligned`, with
`locale-alignment-receipt.json`; source-only evidence is checkpointed before
the runs. The common index and eight artwork files in that validation folder
are links to the preceding edition, so release assembly should take the
16 language files and combine them with the separately validated final
artwork edition. The original UI16 files remain preserved.

The earlier device result that motivated this change remains valid evidence:
its new storage edition had 31 reads versus 19 and a frame above 250 ms.
Native replay counts differ because its warm trajectory differs. The aligned
pack must be measured in the same device scenario before claiming that the
observed calculator regression is resolved.
