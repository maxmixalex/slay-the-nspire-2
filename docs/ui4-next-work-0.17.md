# UI4: remaining original UI and performance work

Reference: v0.111.0 / `41cef1ea`. These are scoped follow-up proposals, not
implemented features or new acceptance claims. The accepted production source
for this stage is `76f0a01cb506fde7ac8b427e857462a35933ade3`; its final test
fixture is `6b7ca242f5938b5008cd3fc47b92c2ab90f98a79`.

## Original UI, in practical implementation order

| Work | Original evidence and intended behavior | Boundaries |
| --- | --- | --- |
| Energy explanation | `NEnergyCounter._Ready` / `OnHovered` use localized `ENERGY_COUNT`; both languages already have indexed text. Add the original explanation to the existing focus/inspect interaction. | Use the displayed current/max energy cue; do not reveal future settled state. No new art is required. |
| Intent fade and floating motion | `NCreature.PerformIntent` / `AnimHideIntent` specify a 0.4-second delay followed by a 0.5-second fade; `NIntent._Process` supplies floating motion. | Presentation alpha must not extend the original command wait of 0.4 seconds Normal / 0.25 seconds Fast. Animated intent frames and execution particles remain a separately sized addition. |
| Turn-banner type and outline | Original player/enemy banner scenes use Spectral Bold 96 and Kreon Bold 36 with an outline; the adapted implementation still uses generic 16/10 text and a one-sided shadow. | Convert only the required bilingual labels/digits where possible. Verify 320 by 240 legibility and actual ARM cost before adding a broad font set. |
| Energy entry/exit and gain effects | `NEnergyCounter.AnimIn` / `AnimOut` use 600 ms transitions; `OnEnergyChanged` emits two particle layers. | Implement lifecycle against displayed energy first. Effects must not alter canonical state or introduce extra command waits. |

Composite intent numeric values are already implemented and accepted in this
stage; do not reclassify that bug as pending. See
`combat-hud-gap-audit-ui4-0.17.md` and
`validation-ui4-intent-values-0.17.json` for exact cases and evidence.

Other known visual gaps remain: opening deal, exhaust/shuffle/generated-card
entrances, autoplay choreography, additional enemy-specific waits and animated
title background Spine. The current title restores original layer ordering,
stars and fog but uses the established static Spine pose. None of these gaps
is covered by a general percentage-of-fidelity claim.

## Cold card pages: next bounded changes

The accepted Chinese ordinary emulator route has a largest collection render
sample of **1,273.224 ms** and largest resource-service sample of
**1,224.335 ms**. These are independently aggregated maxima, not a decomposition
of one frame. Do not subtract them and label the remainder CPU time. Warm-page
I/O is already zero in the paired native checks and final browser measurement.

1. **Batch adjacent visible card-image reads using the existing encoded-image
   workspace.** The inspected visible image IDs 7 through 13 have encoded
   sizes 6,765 / 6,207 / 6,090 / 4,866 / 6,286 / 6,798 / 4,893 bytes, two-byte
   gaps and a combined range 1,695,118 through 1,737,025 in the inspected common
   pack. Their descriptors are already resident. A bounded contiguous read can
   serve only the missing images actually needed for that page, then decode
   into the existing image pool. This needs implementation and measurement;
   no speedup is claimed yet. Preserve overflow/fallback behavior and avoid a
   second persistent buffer or speculative whole-page prefetch.
2. **Reorder Chinese glyph and short-string payloads by visible-page co-use.**
   `tools/repack_retained.py` currently writes glyph payloads in directory
   order. Directory order, IDs and every decoded glyph/string can remain exact
   while physical payload ordering improves the existing 64-page cache's
   locality. Validate first-open card collection, multiple character filters,
   removal and selection pages in both languages before choosing a layout.
   This is a new resource conversion and is not included in the current pack.
3. **Remove row-division work in glyph drawing where profiling still shows CPU
   cost.** `Canvas::glyph` can process decoded runs by row instead of dividing
   and taking a remainder per pixel. Keep exact RGB565 output and the current
   bounded memory. This cannot by itself fix the measured resource stall.

The earlier text-stage host trace attributed 26 + 12 + 4 + 5 reads to the
8/10/12/16-pixel glyph sizes (47 distinct 4 KiB pages), 30 reads to images and
7 to strings. It is a **host trace of that earlier stage**, not a classified
trace of the final ARM route. Final cold-range coalescing reduced the same host
page's total from 84 to 75 calls without changing 387,550 bytes. Only 36
contiguous runs exist among those 47 glyph pages, so batching adjacent pages
alone has a limited ceiling; physical locality deserves investigation.

Acceptance should measure the actual click-to-complete eight-card page,
including any preparation work. Moving a stall into a loading step is not a
speed improvement. Retain exact pixels, zero warm I/O and canonical save
agreement, and compare ordinary calculator-emulator routes with their OS/cache
conditions disclosed.

## Storage and startup

The current shared common pack is 74,894,324 bytes; English and Chinese add
1,780,951 and 4,563,650 bytes. Bilingual installed size with the launcher is
82,236,673 bytes before saves. The 30–40 MB target is still open. Language
deduplication is already complete; further savings must come predominantly
from the shared artwork/animation payloads.

Do not repeat the rejected blanket compression choice: the smaller all-group
candidate slowed first-visible card pages. Any next compression change must
keep random access bounded and compare actual first-visible scenes. Repacking
does not justify removing animation frames or changing visible pixels without
explicitly identifying that fidelity tradeoff.

Integrity-only startup hashes, CRC scans and eager image/tile/glyph/string
payload walks remain removed. Keep functional format/range/decoding work,
lookup/cache keys and gameplay RNG. Development-time source/asset comparisons
do not run during game startup.
