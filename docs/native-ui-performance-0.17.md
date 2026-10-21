# Native components and runtime measurements — source24 through source28

The reference remains **v0.111.0 / 41cef1ea**. This stage follows the owner's
priority: original UI fidelity first, then actual slow-path measurements and
performance improvements. A source checkpoint is not a completed validation.

The sections below retain the sequence of measured faults, source changes and
their then-pending gates. For the consolidated executed results and explicit
runtime/test checkpoint identities, use
[source27-ui-performance-20261021.md](source27-ui-performance-20261021.md) and
[validation-source27-ui-performance.json](validation-source27-ui-performance.json).
The source28 change corrects a test fixture only; the shipped runtime remains
source27. Historical pending statements here do not override that receipt.

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

## Executed source25 results and source27 selection

The complete v16 pack is now **80,879,519 bytes**, SHA-256
`89367cefcf3103d9bd834c47f1eb33e90119d3ab7cbb13b56e952501d2afc342`.
It contains the 13 skins and all ten orb strings. Actual all-card keyword
validation passed 246,626 checks; actual world text passed 1,841. The 13 new
images matched the independent decode oracle with all metadata and 40
memory/range UI frames checked. Synthetic production health validation passed
467,441 checks. The native world capture produced 23 cases / 480 frames /
30 saves; the actual WASM replay matched all frames and saves. Relative to
source24, 474 frames are identical and the other six change only the resolved
Crystal Count text. No paired original-engine capture establishes 95% parity.

Actual source25 ARM execution completed new run, Neow, map, first battle,
save, title return and normal exit. Source26 then actually loaded that saved
80-HP / 3-energy / 5-card battle, with enemy HP 8 / 35 / 14. This verifies the
entry/codec correction beyond the compiled frame audit. Source26's marker
enables the internal profiler. It exits normally, retrieves a diagnostic JSON,
and re-saves the battle to bytes identical to the source25 save. Its startup
counter is 1,444,523 / 32,768 = 44.0833 simulated seconds. Over 59 completed
battle frames, rendering totals 2,526,841 ticks and resource calls 1,415,036
ticks (56.0% of render time), reading 42,621,686 bytes in 837 calls with 770
actual seeks. No resource failure is recorded. In the final ten-second
window, after the animation clock crosses the 9,963 ms idle duration, the
loop still updates only 12 times. Duration coverage alone does not prove
all 81 source frames were visited at that low rate.

| Source25 scene | Observed loop updates per simulated second | Window |
| --- | ---: | --- |
| Title | 30.0 | 10 seconds |
| Character | 30.0 | 5 seconds |
| Neow | 10.2 | 5 seconds after initial rendering |
| Map | 6.6 | 5 seconds |
| Three-enemy first battle | 0.6 | 5 seconds, full animation warmness unproven |

The 10 ms watch counts `map_clock_ms` changes, not completed presentations.
Firebird is not cycle accurate; none of these values is a physical-calculator
FPS claim. About 61.8% of the sampled map PCs fall in the ARM software-division
bucket. Source27 therefore separates the unrotated inverse transform into
320 bounded column entries and one row calculation, leaving rotated sampling,
rounding, alpha, shaders and Canvas composition unchanged. A comparison against
the pre-change direct transform covers clipped/odd sizes and all paint guards.

Actual source25 WASM resource traces use the same v15 file and saved state,
with 360 advances of 33 ms and no gameplay actions. Neow repeats **11,340
callbacks / 46,448,640 bytes in each 180-frame window**. These are glyph and
text pages, with 44 distinct pages in the observed miss sequence; tile/index
traffic is zero. Replaying that miss sequence through 64 pages predicts 44
initial fills and no later misses. This omits the old cache's internal hits;
it is a selection model, not execution of the expanded cache. Source27 sets
the fixed page count to 64 and requires a fresh real trace to verify the result.

The real first-battle save identifies TwigSlimeS, LeafSlimeM and LeafSlimeS.
Its second 180-frame window still transfers **20,064,764 bytes**; tile/index
groups account for **96.51%**. Three repeatedly used half-MiB raw groups are
loaded 31, 32 and 26 times respectively. The full idle-cycle reference set is
11,927,552 page-rounded bytes; that is not a requirement to keep it all resident.
The cache choice uses active tile access and actual image-cache hits, not just
this union. Leaf idle takes 9,963 animation milliseconds, so the 11,880 ms
trace spans only about 1.19 complete cycles. A separate controlled Nibbits
fixture becomes read-free and is explicitly not the slow encounter's baseline.

A full production `Content::image` call trace exposes a second limit: 256
different decoded keys need only 680 of the existing 748 picture pages, but
the 90-entry ceiling forces 594 image misses in 360 frames. The selected
source27 layout keeps those pixel pages and raises only the metadata capacity
to 256 entries; its tile pool becomes 40 × 128 KiB (5 MiB). The exact access
model predicts 256 image misses, with each observed key decoded once, and
second-window encoded group demand falling from 19,349,212 to 665,963 bytes.
The last 58 modeled frames request no groups. This is group demand, including
small index blocks that may be served by reader pages, not measured provider
bytes or frame rate. The model reproduces the old runtime's directly observed
group-read order; fresh source27 native/WASM and ARM execution remain required.

Larger 6/8 MiB tile-only pools would exceed the existing ARM allocation bound
when combined with the expanded reader. A proposed smaller decoded-pixel pool
was not adopted after the entry-count bottleneck was identified. Source27
keeps the 16 MiB linker/memory limits and all original frames, timings and
pixels. Actual linked allocation and same-save scene measurements are gates.

Root receives the private raw trace and corrected group analysis separately.
The raw trace's optional `images.names` used an aux index as an image ID and
must not be cited; the corrected analysis resolves the actual aux directory.
Provider offsets, lengths, byte classes and histogram counts were unaffected.

The complete source25 sanitizer run closed with 60 passes and four 300-second
timeouts. Core passed its isolated longer rerun; the remaining three are still
being closed with explicit longer budgets. Initial failures are preserved and
no incomplete suite is counted as passed.

## Verification status at preparation

Short syntax checks and targeted synthetic probes precede the checkpoint.
Formal native/ARM/WASM builds, actual v16 conversion, component regression,
cross-backend replay and emulator timing must run from the remotely verified
frozen checkpoint with `run_logged.py`. Results belong in a subsequent receipt,
including failures. The 95% UI acceptance target and 30–40 MB package target
remain unestablished; no extra image or animation loss is introduced here.
