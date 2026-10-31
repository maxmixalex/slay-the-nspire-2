# UI22 delivery: source precision, storage and performance

The calculator release is **nSpire 0.1.0-ui22**, with matching **NSP2/46**
resources: 58,613 image entries, 1,783 clips and 7,212 auxiliary entries.
The original reference is the owner's Slay the Spire 2 **v0.111.0 / 41cef1ea**.
Original artwork, derived resources and private captures are not public source.

UI22 restores sampled source colors and repairs insufficient animation density.
Its accepted storage changes preserve the resulting RGB565 words, source alpha8,
sample counts, clocks and registrations. The required bilingual installation
is **99,526,983 bytes before saves**. It is smaller than the control with the
same restored precision and active content, but larger than historical UI21.

## Deliverables and exact size definitions

ZIP bytes measure the download. Extracted bytes include documentation and
licenses. Required `.tns` bytes measure the launcher and resources used by the
calculator; mutable saves and preferences are additional.

| Archive | ZIP bytes | All extracted file bytes | Required or optional `.tns` bytes |
| --- | ---: | ---: | ---: |
| `Slay-the-Nspire-2-0.1.0-ui22-bilingual-calculator.zip` | 98,728,539 | 99,650,627 | 99,526,983 required |
| `Slay-the-Nspire-2-0.1.0-ui22-optional-languages.zip` | 17,321,125 | 18,855,677 | 18,854,842 optional |

Archive SHA-256 values:

- Bilingual: `e75fa7e3c8e4bec3e4bc2f4381b4fb760aead6ab55b3236c5c24daa5196f8e9f`.
- Optional languages: `bda9f42a2dcf62f0afa9799ef2b0626d204a11a5c49b3da07facb709bf2defc6`.

The complete bilingual package includes the launcher, resource index, all eight
shared artwork parts, English and Simplified Chinese. The optional package
contains the other fourteen original language editions. Install only the extra
languages wanted; they use the same required artwork parts and English fallback.

The logical packs reconstructed from those physical files have these identities:

| Logical pack | Bytes | SHA-256 |
| --- | ---: | --- |
| English | 94,924,498 | `e2fc2beb4fe1e4d9ff5e54188f0c134bc9da2ee680ef2acc06ad5f7cd6784147` |
| Simplified Chinese | 99,349,212 | `e1add5f33fbbf8f2093fcf3676200518a0550808ad575bf6e9b0071b3ce7b3d6` |

Logical-pack size is not the physical bilingual installation size: shared
artwork is stored once, while both locale files and the launcher are required.

## Source and executable identities

- Measured production runtime and ARM build: `b9425530b24b0d851b57892872fc07865859cea1`.
- Final accepted resource tools and packager: `1128b795557fee3ee31444e323f58f073bbcd922`.
- Independent opaque-alpha oracle correction, test only: `dc175a9ca060478298e08abdaa9f35a22ca65a33`.
- Launcher: **1,688,294 bytes**, SHA-256 `5d07224f6b5dc92041e9edba3eba7e328dc024deda8e30f9f3ddceefd5d03362`.
- Linked ELF SHA-256: `e749c0dd19bb5797186be0d6e5391c11bec1d4790bd8f77d14a07f1c1635202e`.

These identify the executed runtime and generated artifacts. They make no claim
about an additional hosted CI run or a later documentation commit's build.

## Two storage comparisons

| Required launcher plus bilingual resources | Bytes |
| --- | ---: |
| UI22 restored-precision/native-density control | 107,729,436 |
| Final UI22 | 99,526,983 |
| Historical UI21, containing earlier lossy colors | 73,295,445 |

Against the same-quality active-content control, UI22 saves **8,202,453 bytes**.
The English logical comparison is **103,133,990 → 94,924,498 bytes**, a reduction
of 8,209,492 bytes. Preparation and final catalogs are not literally identical:
the final catalog adds selection/retirement metadata and retires proved
unreachable legacy payloads. Source precision, visible samples and active
content are the controlled quantities.

Against UI21, the required installation **increases by 26,231,538 bytes**.
UI21's reversible codecs preserved inputs that already contained historical
`444a6`, `555a6` or `454a6` color/alpha reduction. UI22 restores information that
those inputs had lost. That restoration is not counted as compression savings.
The precision target is native **16-bit RGB565 plus eight-bit source alpha**;
desktop RGB888/HDR shading remains outside this target-raster comparison.

The earlier 65–70 MB ambition was a historical soft target. No particular amount
of free calculator storage was established for this release. Use the explicit
installation counts above, rather than ZIP size or a resource-format limit, to
assess space requirements.

## Feedback addressed

| Feedback | Delivered behavior |
| --- | --- |
| Continue metadata covers Abandon Run | Placement measures the complete localized menu, focus padding, logo and profile bounds; a clear-gutter fallback keeps controls unobscured. |
| Reward potion/relic tooltips disappeared | All seven reward families route to the actual item tooltip. Long descriptions have a modal paginated reader with preserved focus and complete text. |
| Character selection drops frames | Bounded exact key/copy replay, smaller clip-local index groups and cache reuse remove recurring background resource reads after warming. Reused App addresses invalidate stale scene ownership. |
| Necrobinder death and other animations lose detail or color | Official-source native-density sampling, restored RGB565/alpha8 values and one shared sampling grid preserve detail and prevent seams between cropped pieces. |

The [UI acceptance report](ui22-ui-acceptance.md) records actual input routes,
localized captures and remaining presentation boundaries. Osty's growth, death,
dead-loop and revival sequence was exercised with the corrected art; its
pre-existing timing is not presented as a newly added UI22 feature. The exact
40/7-second affliction phase clock also fixes accumulated sampling drift.

## Adopted storage and rendering changes

The five selection backgrounds keep all **540 source samples**. Frame zero is
complete; every eighth frame is a checkpoint relative to zero, with at most
seven intervening copy patches for a jump. Every reconstructed 320×240 frame is
compared exactly with the full-precision control. Eight-frame index groups stay
within their character clip, and same-tile span merging reduces metadata while
preserving each frame's active tile set and decoded-page budget.

The existing **12.75 MiB** image/tile allocation is repartitioned, not enlarged.
Silent and Defect use 8.5 MiB decoded-image and 4.25 MiB tile caches; ordinary
scenes use 6.75 MiB and 6 MiB. Scene-owner invalidation prevents stale raster
reuse after App reconstruction. This is separate from source-quality recovery.

The dense TGR inverse processes two RGB565 words together where alignment
allows, with exact scalar handling for tails and unaligned cases. Existing
pixel/index compression modes and profiles are selected only when the actual
complete linked ARM decoder is no slower than its declared control. A smaller
LZMA stream alone does not qualify. No new color quantizer is enabled.

Ordinary **FFFD implicit geometry** regenerates original sparse spans from
their exact alpha values, using the original active-tile bitmap and a bounded
230,400-byte scratch area. Only byte-identical geometry is eligible; other
records retain their original representation. It converts 31,479 records and
saves **6,053,888 bytes** at its measured stage. NSP2/46 makes older launchers
reject the representation before decoding an unsupported marker.

The guarded legacy retirement replaces exactly **1,598 unreachable image IDs**
with same-dimension empty placeholders, preserving all live actor, merchant,
Kaiser and Vantom references. Its measured stage saves 1,454,876 bytes. Retained
slice cleanup saves another 77,824 bytes without changing any referenced slice.
These stage figures are not substitutes for the complete installation delta.

The final pass removes **988 index records with no image-descriptor consumer**
from 21 groups, retaining every live record byte and remapping its raw offset.
It saves **249,856 bytes**. Complete final-ELF LZMA2 plus index inverse work falls
**96,820,329 → 11,989,047 instructions**, with every affected group improving.
All image IDs, active pixels, clocks and per-image cache costs remain unchanged.

## Candidates deliberately excluded

The FFFC selection-bitmap candidate was exact but saved only **163,840 bytes**.
Same-core host comparisons measured Defect/Necrobinder opening CPU increases
of **2.86% / 4.43%** and rapid switching **+3.86%**, with noise also present in
unchanged controls. Its benefit did not justify adoption. **Final data contains
no FFFC records**; release validation rejects them. Its offline converter needs
explicit `--experimental-candidate-only` opt-in and marks its receipt
`not_release_approved`. Bounded unused reader support is not a released saving.

Random-particle candidates failed whole-frame performance comparisons and were reverted.
No Random speedup is claimed. XOR, masked re-dicing, temporal residuals,
bitplanes, tile atlases, larger dictionaries and the retained-image candidates
also produced no accepted release improvement. They do not justify dropping
particles, samples, colors, alpha precision or source density.

## Executed acceptance and performance limits

- **162/162 native CTest targets** pass on the frozen production core.
- Final English/Chinese data passes **18 actual UI/Osty runs**, alongside the
  dedicated filesystem, tooltip, input-mode and long-locale checks.
- **58,613 images × memory/ranged/incremental-preload** match the independent
  old-format reference: **1,065,442,263 source bytes per reader**. The final gate
  reports **200,421,907 checks**, 28,655,100 preload steps and zero source reads
  after Ready. Its only format equivalence is the old runtime's lossless removal
  of 33 all-255 alpha planes per reader, with every original value checked.
- All sixteen locales reconstruct exactly, preserving **344,590 glyph entries
  and 139,264 string entries**, including their compressed font/text tails.
- All 58,099 inherited image provenances are classified. The density-repair
  subset has **427 actions / 11,051 frames / 11,134 pieces**, freshly compared
  with official source at zero RGB565 and alpha8 error.
- The production Canvas source gate checks **5,106,124,800 LCD pixels** across
  61,816 views, including noninteger projection, opacity, additive and grayscale
  rendering. Its `b05ecd…` candidate is not the release. The same renderer,
  exact live-record transformations and final `e2fc…` complete original-byte
  gate establish final equivalence; the large Canvas run was not relabeled.
- **4,945 linked ARM vectors** cover dense/sparse inverses, LZMA2, memory guards
  and ABI behavior. A 33-symbol, 45,556-byte machine-code/constant/relocation
  closure binds prior complete-stream measurements to the final ELF.

In the final same-core ABBA comparison, ordinary FFFD is tested against explicit
geometry using `b942553` on both sides. All LCD fingerprints match, and selection
advance performs zero resource reads. Seven combat preparations total
**554.627 → 488.729 ms** and **20,162,400 → 19,528,978 requested bytes**;
6,300 advance-plus-render frames total **3,705.156 → 3,527.798 ms**, with
**1,422,556 → 1,364,734 requested bytes**. Initialization, render, advance,
hashing and logging are measured separately. Warm no-read windows vary both
ways; the measurements do not establish a universal frame-time improvement.

These are host process CPU and resource-reader measurements, not calculator
FPS or flash latency. ARM stream gates measure the complete named decoder
helpers. FFFD alpha-to-geometry reconstruction is covered by native exact-resource
and actual-App gates, but not a complete ARM `Content::image` timing fixture.
No new physical-calculator or ordinary-OS Firebird execution is claimed.

Formal ARM allocation is **22,535,324 bytes**, including the new scratch, leaving
**2,630,500 bytes below 24 MiB**. This linked bound is not a measured peak-memory
trace. Audio, multiplayer, Timeline, finite animation sampling and established
CPU material/scene substitutions remain explicit platform boundaries.

## Upgrade and reproduce safely

1. Exit the game and preserve the **entire existing `state/` directory**.
2. Replace the launcher and complete `data/` with the matching UI22 bilingual set.
   Keep the index and all eight shared artwork files. Replace previously
   installed optional locale files with their matching UI22 versions too.
3. Copy only wanted additional language files into that same `data/` directory;
   retain English for fallback. Do not overwrite live saves with an empty folder.

Use the default original-precision build chain in [source precision](ui22-source-precision.md):
restore and register source art, exact-import it, finish bounded selection,
apply input-bound ARM-gated streams and guarded retirement, then ordinary FFFD,
ARM-gated orphan-record removal and all-locale refresh. FFFC is excluded.
The matching resource and launcher release guards are required before packaging.
Keep source/receipt hashes, lossless defaults, typed references and the allocation
bound. [Selection measurements](ui22-selection.md), [UI acceptance](ui22-ui-acceptance.md)
and [WORK_STATE](WORK_STATE.md) contain the detailed results and continuation rules.
