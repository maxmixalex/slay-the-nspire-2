# UI23 retained differences and recovered behavior

Reference: original v0.111.0 / 41cef1ea. This ledger distinguishes implemented
source, completed runtime/producer checks and final package acceptance. The
runtime through auxiliary/image endpoint **7229/60294** is accepted as d935,
with source-equivalent published checkpoint `1298a1b`: Neow measures 11.2930 FPS
and Nonupeipe 23.0763 FPS on the supplied 64 MiB Firebird, both with zero warm
resource reads. Linked allocation is 22,290,500 bytes against the unchanged
22,291,016-byte ceiling. These are emulator results, not physical-device FPS.

Continuous card materials and paged history are implemented; their specific
precision, capacity and execution boundaries remain explicit below. The actual
bad0168 producer completed all 83 stages for both default bilingual and all-16-
locale output. Final full/reduced installation, current-data guest smoke and
complete extracted package sizes belong to the corresponding delivery report;
this ledger does not predeclare those unfinished checks. See
[runtime acceptance](ui23-runtime-performance.md) and its
[machine-readable receipt](ui23-runtime-validation.json).

This is not a claim of complete Godot parity. Earlier closures remain recorded
in [the UI20 reconciliation](ui20-retained-differences.md).

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
The current v2 consumer keeps a color through the longest consecutive source
interval whose channel-error bounds intersect. Static pixels remain exact;
loop endpoints share a legal color when possible and otherwise each endpoint
is source-exact. All1,052 frame hashes reproduce the reviewed v2 candidate,
which saves449,834 grouped bytes against the preceding same-source hold. This
is a measured intermediate saving, not final physical-install acceptance.
Separate body and light-verification consumers use the same policy; the strict
verifier still reconstructs every frame. The seven original sampler dependency
files and their source-cache identities remain unchanged. Exact-quality and
no-animation conversion disable the hold and retain the exact source pose. The historical loop-only append
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
present. Neow and Nonupeipe have the d935 runtime measurements linked above;
per-scene final-package acceptance remains a distinct delivery-report gate.

| Ancient | Original active emitters | Current implementation | Still missing active family |
| --- | --- | --- | --- |
| Darv |3 CPU |Two source lights and grain stars; source clocks and transforms |None identified |
| Neow |11 CPU,3 GPU |Ten authored light nodes, CPU grain stars, three cloud nodes, four stepped additive fires behind the body, correct vignette ordering and final screen-reflection post |None identified; d935 runtime measured, final-package gates recorded separately |
| Nonupeipe |13 GPU |Five stationary light overlays, six source fog emitters and two sampled turbulence families |None identified |
| Orobas |4 CPU,5 GPU |Four CPU lights, two stationary light plates, three turbulence families and independently clocked water material |None identified |
| Pael |5 CPU,7 GPU |Five CPU lights, four stationary light plates and three turbulence families |None identified |
| Tanx |4 CPU |Two grain/ash families, two scrolling smoke families and source water material |None identified |
| Tezcatara |90 CPU,74 GPU |All90 authored light nodes,74×24 cinders and237 source fire instances (79 flat/158 additive), with original interleave and moving-source inheritance |None identified; final physical/guest gates pending |
| Vakuu |5 CPU,13 GPU |Five CPU lights, first cinder family, three stationary overlays, four turbulence families and five final sparkle emitters |None identified |

The four Neow fire instances now have a source-bound producer and runtime.
All546 foreground poses retain their original cadence. A compact transmission
mask places their additive contribution behind the original animated foreground;
source saturation is permitted only where that foreground is fully opaque.
The mask occupies17,485 bytes inside the existing mutually exclusive cinder
state; the transient contribution uses the existing foreground transfer lease.
Full mode holds transmission within8/255 while the source-bound all-clock
contribution error remains below one green/blue RGB565 code. Reduced mode uses
the source clock0 transmission, with all546 descriptors sharing its real payload.
Both modes reuse the reviewed250×250 triangle field rather than claiming
original full-resolution shader pixels. The actual v2 full append adds 359,382
bytes, including six source channel pages, 546 mask leaves and one parameter
image. This is an intermediate size; final quality/storage and guest performance
are still required. Its shader uses global presentation time with a 200-second
period, independently of the 18-second body loop. The v2 parameter adds compact
all-clock zero-contribution flags. Those flags skip only source-bound integer
shader results that are always zero; they do not discard an animation frame.

The final runtime 789 source was compared at 13 clocks, including entry, the
18-second body seam and 200-second shader wrap. The candidate reads the actual
v2-held body and transmission/shared-noise fire resources; the control uses the
original source body and original-resolution fire shader. Both receive the same
current native foreground particles, lights, vignette, screen post and UI.
The maximum cumulative difference was red/green/blue **[1, 3, 2] RGB565 codes**;
original-size review found no new contour blur, block-like water or visible
loop-seam jump. This bound belongs to those 13 combined comparisons. It is not
the smaller fire-only bound, a universal pixel bound for every independent
material, or reproduction of an unrecorded Godot GPU random history.

Tezcatara fire geometry follows room
elapsed time and its16,667 ms body loop; its shader uses the existing global
visual clock with its own20,000 ms period. Reduced mode fixes both clocks at0.
The eight texture pages use explicit raw/up modes. Only the basic1024×1024
noise and triangle500×500 noise use the reviewed2×2 area mean, rounded to8-bit
channels, yielding512×512 and250×250. Shapes, masks, other noise inputs, all237
source transforms and the401-command interleave retain their source values.
Both full and reduced resources use this same reviewed bounded noise change;
reduced still uses source clock0. The resulting fire pixels and noise bytes
must not be described as identical to the original full-resolution shader.

Up prediction is separately lossless relative to these sampled pages: each page
retains its first sampled row literally and restores later rows once inside its
existing decoded-image cache entry. There is no cross-page dependency, new
buffer or warm reconstruction. The prior18-page raw/up cache proof remains
historical; the new eight-page contract and actual source-module byte proof
validate the new layout. Its grouped payload is176,090 bytes versus494,502 for
the preceding full-resolution up representation, saving318,412 bytes before
final physical publication. Source geometry and independent shader/body clocks
are unchanged. Final physical storage and guest cost remain pending. The two
hidden Neow water-post siblings remain hidden.

The twelve turbulent families retain every authored particle and all45,484
stored position keys, produced by the original float32 force/curl equations.
Their interpolation error is bounded to1/8 LCD pixel. They use a reproducible
source seed bank, not an unavailable GPU random history. The largest table is
stored as320×205 with128 additional zero-padding bytes; it fits the existing
240-row decoder bound. All used table bytes are unchanged. The actual12-table
payload occupies145,062 compressed bytes, not a final installation subtotal.

The separate water/light materials retain their explicit approximation bounds
and exact reduced-source paths;
see [materials](ui23-ancient-materials.md), [stationary lights and sparkles](ui23-ancient-gpu-masks.md),
[Neow particles](ui23-neow-particles.md), [Neow screen post](ui23-neow-screen-post.md),
[Nonupeipe fog](ui23-nonu-fog.md) and [Vakuu cinders](ui23-vakuu-cinders.md).
A full-profile water/light approximation may not be relabeled as its exact
reduced counterpart; the generic reducer rejects that substitution. The Tezcatara
fire noise exception above, including the triangle reused by Neow fires, is
explicitly shared by both profiles. Reduced mode caches the complete
entry pose after background/effects and before dialogue/HUD; dynamic UI remains
active. Persistent framebuffer, particle and decoded-cache capacities do not grow.

## Earlier behavior carried forward

Enemy command programs, Vantom independent root/tail/VFX, positive Ancient
healing, Tablet notifications, Smith impacts, Doom, Architect, Random, Fake
Merchant, card trails and source death observers predate this effects work.
Their runtime modules remain present. Full-screen Ancient projection, localized
UI placement and save retirement are also preserved. Registry/model counts do
not establish arbitrary cross-track Spine composition or full GPU equivalence.

## Explicit non-Ancient reconciliation

The earlier user-requested omissions are not inferred complete from the new
Ancient effects. Their existing implementation and prior acceptance are tracked
separately in [UI20 reconciliation](ui20-retained-differences.md),
[UI19 UI fixes](ui19-ui-feedback.md) and [UI15 reconciliation](ui15-retained-differences-0.17.md).

| Earlier retained item | Current status |
| --- | --- |
| Monster command waits, Vantom independent root/tail and death effects | Source command observers and compound runtime paths are implemented; current original-data actor acceptance is a separate final gate. |
| Low-density charge/action frames and missing Osty death loop | Source-density replacements and typed action/death selection are implemented. No Ancient storage exception applies to ordinary actors. |
| Ancient top gap, tooltip/reward icons, card/label placement and defeat/title transitions | The prior production UI fixes remain present. The new effects preserve their foreground ordering. |
| Abandon/save association, ended no-node map state and terminal checkpoint removal | Prior lifecycle implementations remain present. Current ordinary exit/relaunch verification remains a distinct gate; historical success does not resolve a newly observed guest exit issue. |
| Affliction, Infection and Ancient card-fire materials | New ACU1 packs continuously evaluate all nine materials across the seven Affliction families and render all55 visible authored particles. The former56 phase slots carry fields, curves and particle tracks. Infection's30 PNGs and Ancient fire's10 authored frames remain complete source flipbooks. Historical packs retain their8-phase fallback. See [continuous-card acceptance](ui23-continuous-card-materials.md) for precision, memory and execution evidence. |
| Arbitrary cross-track Spine composition and unrecorded cosmetic RNG history | Existing authored observers, sampled motion and deterministic cosmetic state cover the declared paths. General desktop composition/process-history parity is not established. |
| Hosted services and desktop timezone/IME integration | Local daily/custom gameplay and locale formatting exist; hosted leaderboards/cloud, a timezone/DST database and a calculator IME are not supplied. |
| History capacity | The 32-run, 96-floor and 4,096-notice arrays are bounded working windows backed by the append archive. Older successful archived records remain browsable; migration preserves the existing records but cannot reconstruct facts already erased by earlier versions. Disk space, serial identity and file offsets remain finite. See [paged history](ui23-history-archive.md). |
| Drawing and seed input capacity | Map drawing remains limited to 2,048 points and seed input to 64 UTF-8 bytes plus terminator. These are existing bounded port capacities, not a newly verified desktop-equivalence claim. |
| Audio, multiplayer and Timeline | Not implemented. Earlier project/AGENTS scope excluded them, but the recovered original user requests do not establish an explicit user waiver; this ledger does not invent one. |

The general composition and cosmetic-history limits,
platform/service and capacity differences, and unimplemented features remain
material boundaries even when every inventoried Ancient emitter has a runtime.
No new source-confirmed missing ordinary UI callback was found by this bounded
ledger audit; that is not an exhaustive equivalence proof of the original game.

## Actual execution evidence and remaining limits

The current d935 acceptance and paired measurements are recorded in
[runtime acceptance](ui23-runtime-performance.md). The Pael comparison below
is historical and is not the final full/reduced installation result.

A supplied CX II firmware image has booted in Firebird. Normal Ndless launch,
selection, Neow/map, a card/enemy turn, normal save and exit were exercised with
baseline data. A controlled reduced-Pael test compared the old895 launcher and
460 cache/text-layout candidate using the same saved run and old data, with
**64 MB RAM and396 MHz unchanged**. Across30-second windows, baseline measured
21.094 emulator frames/s and46.995 ms render time; candidate reached29.991
emulator frames/s (30 cap) and29.835 ms, with zero warm reads in both. This is
one emulator scene, not physical-handheld FPS or acceptance of the new effects.
No physical calculator execution has been demonstrated.

- Audio, multiplayer and Timeline are not implemented. Earlier project scope
  excluded them; that classification is not evidence of an original user waiver.
  Hosted leaderboards, cloud services and desktop-service integration are not
  supplied by local daily/custom modes.
- Output remains320×240 RGB565/LDR with integer rasterization, documented
  sampling/noise approximations and bounded particle representations. New ACU1
  resources continuously evaluate all seven Affliction families; only older
  resource packs retain their eight-phase fallback. Infection's30 and Ancient
  fire's10 frames are complete authored flipbooks. This does not establish
  full desktop GPU or unrecorded particle-process-history equivalence.
- The five new history/archive storage diagnostics have Simplified Chinese
  and English text; the other14 locales display English for those messages.
  Original-language content coverage is not a claim that every port-authored
  diagnostic has sixteen translations.
- Cosmetic source seeds do not recreate an unrecorded process-global random
  history. Gameplay RNG and canonical save state remain separate.
- The RTC path has no desktop timezone/DST database and there is no new IME.
  Seed fields, run history, map notes, journals and decoded caches remain finite.
- The accepted d935 linked allocation is 22,290,500 bytes, below the unchanged
  22,291,016-byte ceiling with 516 bytes remaining and unchanged usable capacities.
  Linked allocation is not measured physical peak memory or a guarantee against
  every fragmented OS heap.
- The actual default-bilingual and all-16-locale original-folder CLI runs each
  completed all 83 stages. Final full/reduced real-data installation, current
  guest smoke and the entire extracted 70,000,000-byte installation gate remain
  mandatory and are recorded by the delivery report. The default hard cap and both the core-file and complete-archive
  checks are in `tools/package_variants_ui23.py`; scope metadata does not enforce
  that limit. Source tests and historical packages do not replace these gates.
