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

The first exact full-animation representation adds 9,966,880 bytes and makes
Neow read about 248 MB per loop even after warming. It is **not accepted for
the final full-animation edition**. A bounded color-hysteresis representation
is under measurement; no scene cadence or cache-capacity reduction is claimed.
The exact reduced-pose edition can retain the verified first poses independently.

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
- The c10b8e ARM candidate allocates 22,289,816 bytes, below historical UI21's
  22,291,016 by 1,200 bytes, with unchanged usable cache capacities. Later
  runtime fixes require a new measured final build; this linked allocation is
  not observed physical peak memory.
- Matched-O2 old/new runtime tests preserve complete selection/combat frames
  and read counts. Host timing scatter does not establish an FPS improvement.
  No physical calculator connection or working Firebird firmware is available
  in this session; current device execution and device FPS remain unverified.

The final release must report these remaining differences even when the
resource size, source sampling and native correctness gates pass.
