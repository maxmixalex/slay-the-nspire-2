# UI23 retained differences and recovered behavior

Reference: original v0.111.0 / 41cef1ea. This is an implementation ledger, not
a claim of complete Godot parity. Historical closures remain recorded in
[the UI20 reconciliation](ui20-retained-differences.md). Final package size,
appearance and performance acceptance must be attached separately.

## Ancient scene exception

UI20/UI22 rendered all eight Ancient rooms as static composites. UI23 now has
a production renderer for the two authored skeleton loops: Neow `idle_loop`
(18,000 ms, 546 samples) and Tezcatara `animation` (16,667 ms, 506 samples).
Both retain 33 ms sampling and the original 1680×1260-to-320×240 projection.
The typed tables preserve the existing 1,783 gameplay clips. Reduced-pose
resources hold the exact first scene pose without shortening action clocks.

The original-source render run completed all 1,052 scene frames. Independent
original raster comparisons of the first, middle and final pose of each loop
agree at RGB565 precision. The native production Canvas matches every exact
source frame through two full passes and wrap, seek and shared-raster eviction
cases. This is host correctness evidence, not calculator FPS.

The first exact full-animation representation added 9,966,880 bytes and made
Neow read about 248 MB per loop even after warming. It was rejected. The
production `ee6575e5678a0d49d998e2f324c67619f2c71955` converter instead keeps
all 1,052 samples in existing RGR1/RGR3 copy-patch groups, with checkpoints every
32 samples. Only the new Ancient rooms use that replay bound; selection stays
at eight. Each patch view is consumed before the next image request and remains
evictable through the ordinary cache. No framebuffer or usable cache grows.

The full edition uses cyclic RGB565 color hold bounded to red/blue ±1 and green
±2. Four warm cycles identify 14 Neow and 106 Tezcatara pixels that do not settle;
those pixels remain source-exact at every frame. The resulting seam recurrence
and every frame's error bound are verified. The exact no-animation and exact
quality converter paths disable hold and retain the original first scene pose.

The final production append adds **1,852,322 bytes**, taking the approved
63,287,322-byte logical base to **65,139,644 bytes**. Its SHA-256 is
`f39ea6f0f98e70b2bd0708b6ab50ee03aff2ba7cefa5fe51e0c15ee6b0301bdb`.
Every source-cache frame hash, full-frame patch reconstruction, candidate pixel
hash and exact retained-group inverse passed. Original glyphs, strings,
geometry, clocks, old pixel streams and preparation costs remain unchanged.
These are logical-pack sizes; the package report owns total installed bytes.

The final native Canvas matches all 2,104 candidate frames across two passes,
including wrap, skipped frames, seek and shared-raster eviction. With the actual
App dialogue, three choices and English or Chinese fonts, both Ancient rooms
perform **zero source reads on a complete warm loop**. Neow peaks at 1,188 image
pages / 573 entries, Tezcatara at 665 / 532, below the unchanged ordinary
1,728-page capacity. Cold entry and normal rendering are included in the
separate timing records. The exact reduced edition also passes both legacy
eight-checkpoint and explicit 32-checkpoint table tests at all clocks and seeks.

Restoring these two loops does not restore independent particle emitters or
material clocks. The six other scenes contain painted art and emitters, rather
than another missing skeleton loop. Expanding the authored scene instances and
excluding authored-hidden nodes gives the following concrete remaining scope:

| Ancient | Missing active CPU emitter nodes | Missing active GPU emitter nodes | Other independent material clocks |
| --- | ---: | ---: | --- |
| Darv | 3 | 0 | None identified in this bounded inventory |
| Neow | 11 | 3 | Four stepped additive-fire instances and one water-reflection post shader |
| Nonupeipe | 0 | 13 | None identified in this bounded inventory |
| Orobas | 4 | 5 | None identified in this bounded inventory |
| Pael | 5 | 7 | None identified in this bounded inventory |
| Tanx | 4 | 0 | None identified in this bounded inventory |
| Tezcatara | 90 | 74 | 79 stepped flat-fire and 158 additive-fire instances |
| Vakuu | 5 | 13 | None identified in this bounded inventory |

Counts are scene-node instances, not simultaneously visible particles. The
offline renderer already has stepped flat/additive fire families. The Ancient
runtime does not yet bind those independent clocks or the listed emitter
parameters to room rendering. Resetting them at a skeleton loop boundary or
substituting generic particles would not recover their original behavior.

## Older differences already implemented

The production enemy command programs, Vantom independent root/tail and VFX,
positive Ancient healing, Tablet notification, Smith impacts, Doom, Architect,
Random, Fake Merchant, card trails and source death observers predate UI23.
Their source modules remain present. They are carried-forward implementations,
not newly completed Ancient ambient behavior. UI23 also keeps the corrected
full-screen Ancient viewport, localized UI positioning and save retirement.

## Remaining platform and scope limits

- Audio, multiplayer and Timeline remain explicitly excluded in the product
  scope. Hosted leaderboards, cloud services and desktop service integration
  are not implemented by the local daily/custom modes.
- Output is 320×240 RGB565/LDR with integer rasterization, bounded CPU effects
  and sampled materials. Cosmetic local randomness does not reconstruct an
  unrecorded process-global cosmetic RNG history.
- The RTC path has no desktop timezone/DST database; the calculator has no
  new IME. Seed fields, run history, map notes and decoded caches remain finite.
- The final ee6575 ARM build allocates **22,290,328 bytes**, below historical
  UI21's 22,291,016 by 688 bytes and UI22's 22,535,324 by 244,996 bytes, with
  unchanged usable cache capacities. Its ELF SHA-256 is
  `63c0760f3687a31b8d2692120705aa29948822c8ec3d8c19424da1e0f7bb232f`.
  This linked allocation is not observed physical peak memory.
- Matched-O2 old/new runtime tests preserve complete selection/combat frames
  and read counts. Host timing scatter does not establish an FPS improvement.
  No physical calculator connection or working Firebird firmware is available
  in this session; current device execution and device FPS remain unverified.

The final release must report these remaining differences even when the
resource size, source sampling and native correctness gates pass.
