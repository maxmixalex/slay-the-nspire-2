# Native components and runtime measurements — source24 preparation

The reference remains **v0.111.0 / 41cef1ea**. This stage follows the owner's
priority: original UI fidelity first, then actual slow-path measurements and
performance improvements. A source checkpoint is not a completed validation.

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

`tools/append_native_ui.py` takes the owner's verified v15 pack and pinned
PCK. It writes only to a private path outside this repository. The shader
conversion implements the reference's RGB/YIQ matrix; it deliberately does
not substitute the standard HSV color-space rotation.

## Measurement paths

The calculator's optional foreground profiler is activated by holding **Ctrl
while launching nSpire2**. It stores bounded aggregate counters only, and on
normal exit writes `nSpire2.perf.tns` once. The JSON reports startup, first
render, first present, scene intervals, update/render/blit time and resource
read costs. Frame intervals preserve stalls above 50, 100 and 250 ms before
the gameplay delta clamp. Profiling is disabled in normal runs.
The resource provider tracks its own file position and omits a seek only for
an immediately contiguous exact read; failures invalidate that cursor. All
hash and decoder validation remains enabled. Seek counts accompany read costs.

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
