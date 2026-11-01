# UI23 retained differences and recovered behavior

Reference: original v0.111.0 / 41cef1ea. This ledger distinguishes implemented
source from completed release acceptance. The combined implementation through
auxiliary/image endpoint **7227/59732** still requires its final original-data
conversion, whole-scene rendering, allocation, storage and guest gates. It is
not a claim of complete Godot parity. Earlier closures remain recorded in
[the UI20 reconciliation](ui20-retained-differences.md).

## Ancient skeleton loops

UI20/UI22 used static composites for all eight Ancient rooms. UI23 implements
Neow `idle_loop` (18,000 ms,546 samples) and Tezcatara `animation`
(16,667 ms,506 samples), retaining33 ms sampling and the original
1680×1260-to-320×240 projection. Their typed tables preserve the existing1,783
gameplay clips. Reduced resources hold the exact first source pose without
shortening action clocks.

The historical `ee6575` loop-only candidate was sampled from all1,052 original
frames and verified against native Canvas through two complete loops, wrap,
seek and shared-raster eviction. Its first exact storage representation added
9,966,880 bytes and was rejected because of its storage and repeated-read cost.
The accepted representation used existing RGR1/RGR3 copy patches with a maximum
32-sample checkpoint interval. Selection retains its separate eight-sample
checkpoint interval. Patch pointers are consumed before the next image request;
no framebuffer or usable cache grows.

Full animation uses cyclic RGB565 hold bounded to red/blue±1 and green±2.
Nonconvergent pixels remain source-exact, including the loop seam. Exact-quality
and no-animation conversion disable this hold. The historical loop-only append
added1,852,322 bytes; this is not a size estimate for the newer complete effects.
Its English/Chinese App tests made zero source reads through warm loops, with
1,188/665 peak image pages for Neow/Tezcatara below the unchanged1,728-page pool.
Those measurements retain their old source and data identities.

The current authored-light implementation resamples Neow with its intervening
vignette deferred, then composes bone lights, fog/stars, four static lights,
vignette and the final three lights in original order. It does not add a second
546-frame family. Tezcatara preserves its existing506 body samples and
interleaves its original light/cinder nodes. The old loop-only pixel proof must
not be described as acceptance of this newer composition.

## Independent Ancient effects

The original inventory expands scene instances and excludes authored-hidden
nodes. Counts below refer to emitter nodes, not simultaneous particles.
“Implemented” means source producer, typed resources and runtime drawing are
present; combined final-data and performance acceptance remains pending.

| Ancient | Original active emitters | Current implementation | Still missing active family |
| --- | --- | --- | --- |
| Darv |3 CPU |Two source lights and grain stars; source clocks and transforms |None identified |
| Neow |11 CPU,3 GPU |Ten authored light nodes, CPU grain stars, three cloud nodes, correct vignette ordering and final screen-reflection post |Four stepped additive-fire instances behind the body |
| Nonupeipe |13 GPU |Five stationary light overlays, six source fog emitters and two sampled turbulence families |None identified |
| Orobas |4 CPU,5 GPU |Four CPU lights, two stationary light plates, three turbulence families and independently clocked water material |None identified |
| Pael |5 CPU,7 GPU |Five CPU lights, four stationary light plates and three turbulence families |None identified |
| Tanx |4 CPU |Two grain/ash families, two scrolling smoke families and source water material |None identified |
| Tezcatara |90 CPU,74 GPU |All90 authored light nodes and74×24 cinders, including moving-source-node inheritance and original interleave |79 stepped flat-fire plus158 additive-fire instances |
| Vakuu |5 CPU,13 GPU |Five CPU lights, first cinder family, three stationary overlays, four turbulence families and five final sparkle emitters |None identified |

The remaining fire instances must execute in their authored order and on their
independent clocks; registering a descriptor or adding a generic overlay does
not close them. The two hidden Neow water-post siblings remain hidden.

The twelve turbulent families retain every authored particle and all45,484
stored position keys, produced by the original float32 force/curl equations.
Their interpolation error is bounded to1/8 LCD pixel. They use a reproducible
source seed bank, not an unavailable GPU random history. The largest table is
stored as320×205 with128 additional zero-padding bytes; it fits the existing
240-row decoder bound. All used table bytes are unchanged. The actual12-table
payload occupies145,062 compressed bytes, not a final installation subtotal.

Materials retain explicit approximation bounds and exact reduced-source paths;
see [materials](ui23-ancient-materials.md), [stationary lights and sparkles](ui23-ancient-gpu-masks.md),
[Neow particles](ui23-neow-particles.md), [Neow screen post](ui23-neow-screen-post.md),
[Nonupeipe fog](ui23-nonu-fog.md) and [Vakuu cinders](ui23-vakuu-cinders.md).
A full-profile approximate texture may not be relabeled as exact reduced data.
The generic reducer rejects that substitution. Reduced mode caches the complete
entry pose after background/effects and before dialogue/HUD; dynamic UI remains
active. Persistent framebuffer, particle and decoded-cache capacities do not grow.

## Earlier behavior carried forward

Enemy command programs, Vantom independent root/tail/VFX, positive Ancient
healing, Tablet notifications, Smith impacts, Doom, Architect, Random, Fake
Merchant, card trails and source death observers predate this effects work.
Their runtime modules remain present. Full-screen Ancient projection, localized
UI placement and save retirement are also preserved. Registry/model counts do
not establish arbitrary cross-track Spine composition or full GPU equivalence.

## Actual execution evidence and remaining limits

A supplied CX II firmware image has booted in Firebird. Normal Ndless launch,
selection, Neow/map, a card/enemy turn, normal save and exit were exercised with
baseline data. A controlled reduced-Pael test compared the old895 launcher and
460 cache/text-layout candidate using the same saved run and old data, with
**64 MB RAM and396 MHz unchanged**. Across30-second windows, baseline measured
21.094 emulator frames/s and46.995 ms render time; candidate reached29.991
emulator frames/s (30 cap) and29.835 ms, with zero warm reads in both. This is
one emulator scene, not physical-handheld FPS or acceptance of the new effects.
No physical calculator execution has been demonstrated.

- Audio, multiplayer and Timeline are explicitly excluded product scope.
  Hosted leaderboards, cloud services and desktop-service integration are not
  supplied by local daily/custom modes.
- Output remains320×240 RGB565/LDR with integer rasterization, sampled materials
  and bounded particle representations. Existing card material sampling remains
  unchanged; it is not continuous desktop GPU equivalence.
- Cosmetic source seeds do not recreate an unrecorded process-global random
  history. Gameplay RNG and canonical save state remain separate.
- The RTC path has no desktop timezone/DST database and there is no new IME.
  Seed fields, run history, map notes, journals and decoded caches remain finite.
- Final linked allocation must stay at or below UI21's22,291,016 bytes with
  unchanged usable capacities. Linked allocation is not measured physical peak
  memory or a guarantee against every fragmented OS heap.
- Final cold/warm cost, full/reduced real-data rendering, original-folder CLI
  execution and the entire extracted70,000,000-byte installation gate remain
  mandatory. Source tests and historical packages do not replace these gates.
