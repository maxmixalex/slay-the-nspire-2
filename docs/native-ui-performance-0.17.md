# Native components and runtime measurements — source24 / source25

The reference remains **v0.111.0 / 41cef1ea**. This stage follows the owner's
priority: original UI fidelity first, then actual slow-path measurements and
performance improvements. A source checkpoint is not a completed validation.

## Actual calculator entry fault and corrective checkpoint

The historical source22 ARM package actually booted in Firebird on the owner's
CX II CAS image, then failed when confirming a new expedition. Disassembly and
captured fault state agree: `new_run()` constructs a full `State{}` temporary
on the calculator stack, reserving 198,344 bytes including saved registers and
crossing an unmapped stack page. `save()` and `load()` reserve 198,168 and
198,312 bytes in the same way, so fixing only the visible entry crash would
leave save/continue/startup probes unsafe. These are source25's P0 corrections.
Two event-state resets also reserve about 40 KiB and are being corrected before
the next map/event/battle run. Native default-value and bounded-stack regression
must accompany the actual ARM prologue and emulator checks.

Before the fault, title and character windows showed respectively 30.0 and
26.8 loop updates per simulated second. Opening the launcher to the first
observed loop took approximately 44.81 simulated seconds, at 10 ms sampling
resolution. These figures do not establish physical-device performance or
battle FPS. No battle/map baseline exists from this failed package. The
calculator profiler in the corrected build will distinguish startup/read cost,
update, render, blit and raw frame-interval stalls. The source22 package must
remain labeled historical rather than promoted as tested through a new run.

The source24 private conversion is complete: the 13 skins add 33,060 bytes to
the existing 80,846,058-byte pack. The new NSP2/16 file is **80,879,118 bytes**,
SHA-256 `092d58ac18c57873de6491945c7ed3527eec52174b1ccb0450d338b9dcf14570`.
Existing compressed payloads, metadata and relocated offsets all matched.
The frozen host baseline captured 23 scenarios / 480 frames / 30 saves; the
synthetic production health test passed. However the actual all-card keyword
gate failed because all five orb title/description pairs were absent from the
old localization catalog. Source25 appends these ten exact original strings
after index 6,099. Old string IDs and payload bytes remain unchanged; both the
early 6,100-string v16 profile and the complete 6,110-string profile are read
compatibly. Runtime keyword acceptance requires rerunning the actual-pack gate.

The same image review identified an unresolved crystal-board `{Count}`. The
new binding uses the saved divination charges only for that exact screen key,
with tests that unrelated event Count tokens stay unresolved. The historical
source24 baseline remains available for exact pixel/save comparisons, with
deliberate visual fixes accounted for separately from performance changes.

## Component changes

- Health bars use the original background, stroke, mask, fill, block and
  infinite-health images. The implementation distinguishes block, poison and
  Doom regions and lethal text colors. Creature identity and battle lifecycle
  own a bounded damage/healing trail; repeated renders do not advance it.
  Original damage hold/tween timings are retained. Doom's animated noise
  shader remains a stated gap.
- Card keyword tips follow original model metadata, collect and deduplicate
  related powers, keywords, orbs and generated card previews. Hover works on
  hand, collection, piles, offers and detail views. Held/dragged cards suppress
  tips. On the calculator, detail → V opens the same complete, paginated list;
  arrows scroll and Esc returns to the card. The small-screen font and paging
  are readability adaptations; dynamically granted keyword insertion history
  is not retained by the old state model and uses stable trailing order.
- The wrong wide-stretched confirm-tick background is replaced with the
  original pause/reward plaque for auxiliary text controls. Pause/settings
  buttons use authored normal/focus shader colors. Confirm/back and end-turn
  have bounded focus, press and release motion, modulation, and original art.
  With no playable card, end-turn uses the original additive base/pulse glow,
  shared authored pivot, BackOut activation, QuartOut pulse and ExpoOut fade;
  combat identity and presentation resets isolate it from previous battles.
  Original corner pixels survive narrow-screen text reflow. Some continuous
  shader transitions use interpolation between prebaked endpoint skins;
  this is not pixel acceptance against the original engine.

## Append-only resource format

NSP2/16 retains the v15 tile codec and appends **13** UI images/references
after the historical **1,175** auxiliary entries. It never renumbers old
images, clips, cards, text or auxiliary references. The appender relocates
directory offsets and descriptor header pointers, then compares every old
payload byte, metadata record and new decoded image. Animation groups are
copied unchanged. The runtime continues to accept the old v14/v15 profiles
and uses compatible component fallbacks where the new art is unavailable.
The corrected source25 appender also includes ten orb locale strings omitted
by the old catalog, preserving the historical text IDs and payloads. Early v16
packs remain readable, but the complete new keyword feature requires the
updated resource file.

`tools/append_native_ui.py` takes the owner's verified v15 pack and pinned
PCK. It writes only to a private path outside this repository. The shader
conversion implements the reference's RGB/YIQ matrix; it deliberately does
not substitute the standard HSV color-space rotation.

## Measurement paths

The calculator's optional foreground profiler is activated by placing a small
file named **`nSpire2.profile.tns`** next to the launcher before opening it.
Its contents are unused; remove that marker to disable profiling. Source26
adds this deterministic opt-in after actual Firebird execution showed Ndless
waiting for launch keys to be released before reaching `main`, making the
source24/25 Ctrl-only instruction unreliable. The held-Ctrl fallback remains
for launch environments that preserve it. The marker is opened read-only and
closed immediately; the game never creates or removes it. Timing begins after
this opt-in probe: startup covers resource open and `App::init`, excluding the
Ndless loader and marker probe. The profiler stores bounded aggregate counters only, and on
normal exit writes `nSpire2.perf.tns` once. The JSON reports startup, first
render, first present, scene intervals, update/render/blit time and resource
read costs. Frame intervals preserve stalls above 50, 100 and 250 ms before
the gameplay delta clamp. Profiling is disabled in normal runs.
The resource provider tracks its own file position and omits a seek only for
an immediately contiguous exact read; failures invalidate that cursor. All
hash and decoder validation remains enabled. Seek counts accompany read costs.

Actual source22 PC sampling identified the generic `Canvas::paint` path in
title and character scenes. Both contain an opaque 320×240 RGB565 background.
Source25 directly fills/copies only an unmodulated main surface with no
coverage and, for pictures, normal blending and native dimensions. Every
opacity, tint, alpha, palette, sparse, scaling and coverage case retains the
original compositing path. An independent production-Canvas/paint comparison
covers clipping, pitch, unaligned source buffers and all fallback guards; FPS
improvement remains a measurement gate, not a consequence asserted from code.

The browser host adds an opt-in ten-second measurement panel, bounded per-scene
aggregates, a direct RGB565-to-RGBA lookup, chunked base64 fallback decoding,
and input invalidation coalesced into animation-frame presentation. Resource
copy time does not include C++ decompression; the latter is part of render.
`tools/profile_web_runtime.js` replays actual fixtures using the actual WASM
binary and private pack. Its output is explicitly **Node host execution**,
excluding browser DOM, Canvas2D, compositor and calculator CPU behavior.

The user's existing CX II CAS flash and injected boot image have been restored
into private working copies in Firebird. Ndless installation and the source22
binary/resource USB transfer were actually exercised. The emulator accelerates
some timer polling and does not model a physical ARM cache/wait-state pipeline,
so neither emulator wall time nor simulated FPS establishes handheld FPS.
No physical USB calculator was available during setup.

Cloud-browser access to the embedded private resource HTML was rejected by
automatic approval review. No browser FPS or screenshot acceptance is claimed
from that blocked attempt. Further cloud testing of that artifact requires
explicit authorization for its private contents; local WASM and Firebird work
continue without changing that boundary.

## Verification status at preparation

Short syntax checks and targeted synthetic probes precede the checkpoint.
Formal native/ARM/WASM builds, actual v16 conversion, component regression,
cross-backend replay and emulator timing must run from the remotely verified
frozen checkpoint with `run_logged.py`. Results belong in a subsequent receipt,
including failures. The 95% UI acceptance target and 30–40 MB package target
remain unestablished; no extra image or animation loss is introduced here.
