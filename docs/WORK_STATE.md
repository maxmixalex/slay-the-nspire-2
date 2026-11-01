# UI24 original Timeline locale tail and static Neow producer integration

The converter now inserts the verified NWFv3 static-field transform after the
original Neow fire stage, before card materials and all final compaction/ARM
gates. Its module comes from Ancient checkpoint bde4ce45; that task measured
117,114 bytes saved on the actual Neow stage and exact equality for all 17,485
production-shader GB words. The final combined installation is still pending.

Timeline localization is appended during final locale publication, after the
existing locale graft and before exact glyph storage. The shared art is not
changed. The fixed contract adds 257 original strings at 8704..8960 in every
requested language, following the original English fallback only when a key
is missing. New combining graphemes have disjoint private display keys and
expand to the exact original UTF-8. Only missing 10px body/rich and 12px title
glyphs are rendered from the original locale font chain. Existing glyphs,
strings, compressed art, IDs, clocks and the reduced-pose marker are verified.
Twenty-eight focused converter/locale/workspace tests pass. Actual original
ENG/ZHS and all-16 publication/font proofs are the next acceptance step.

# UI24 retained conversion exact-stream proof candidate

The retained-color writer now proves untouched tiled artwork through exact
compressed tile/index bytes, image descriptors/geometry, and codec parameters,
avoiding duplicate reconstruction of unchanged frames in each locale. Modified
retained images, glyphs and strings still decode and compare; final linked ARM
validation is unchanged. Corruption checks cover changed streams, geometry and
codec parameters. Original-data SHA and measured timing acceptance are pending.

# UI24 active repair and optimization

The new task starts at merged main `b775b678c596dab04dccd47a143d46593d35fc76`.
UI23's final official full-animation bilingual installation is 69,968,366 bytes,
and its reduced-pose installation is 29,912,696 bytes. The complete full package
must stay at or below 70,000,000 decimal bytes, and formal ARM allocation must
stay at or below 22,291,016 bytes without expanding pools.

The current user reports that Osty does not animate when Necrobinder dies,
Necrobinder's slash shows missing texture content, and Ancient rooms remain
too slow. The user explicitly permits appropriate simplification of Ancient
effects, and requests faster conversion with substantially less temporary
storage. Full actor clarity and required action clocks remain constraints.
The seed question is whether ordinary PC ASCII seeds are accepted and reproduce
the same locked-version content, not merely whether a string fits an input box.
The drawing-point and seed-buffer limits must not be called OS limits without
evidence.

Three isolated work streams own actor animation, Ancient rendering/seed parity,
and converter scheduling/storage. Coherent source checkpoints will be published
before long validation. No new fixes or performance results are accepted yet.
The existing UI23 measurements below keep their original source/resource scope.

The final UI23 official-resource 30-second guest intervals measured ordinary
seven-card combat at 29.786365 FPS (two reads, 156,125 bytes), three Affliction
families at 18.387354 FPS, seven at 9.429899 FPS, full Neow at 11.201985 FPS,
and reduced Neow at 29.992983 FPS. These used normal USB installation/readback
and the unchanged 64 MiB Firebird OS/Ndless environment; they are not physical
handheld results. The latter four intervals had zero resource reads. This is
the accepted baseline, distinct from the older controlled fixtures below.

# UI24 Ancient calculator effects candidate — guest validation pending

The user permits simpler Ancient environmental effects. This candidate keeps
all source body frames, full-resolution transmission and their 33 ms timeline.
Neow holds one actual source fire-light pose in the existing mutually exclusive
Tez/Neow scene slot, omits screen-space refraction, and samples smooth Ancient
illumination at four-pixel spacing. The full-resolution room, dialogue and HUD
are never rescaled. No resource conversion or larger memory pool is required.

Formal ARM allocation, same-resource 64 MiB Firebird before/after, private
same-clock appearance comparison and focused regressions are pending. The
existing UI23 figures below remain the baseline, not candidate acceptance.
# UI24 standalone seed-code entry candidate

The same converter now accepts --encode-seed and --encode-seed-file before
game discovery, helper loading or resource conversion. The streaming module is
byte-identical to Ancient seed commit a14f7aa and has 918 original-DLL ASCII
vector comparisons reported by that task. CLI tests check the actual oldA and
whitespace vectors, file chunk/trim boundaries, literal prefixes and rejection
of non-ASCII input. Bundle self-test now executes both text and file entry
using the verified oldA oracle. The final frozen executable and Windows CI
still need rebuilding after integration with the current runtime/Timeline work.

# UI24 converter working-set candidate — original-data validation pending

The candidate carries the requested locale set through all locale writers,
removes unused complete logical-edition copies, and reclaims owned stage
outputs/scratch after their last successful consumer. Verified recovery
boundaries and diagnostics survive; a failed stage retains needed inputs.
The new dependency planner resumes through already-consumed generations and
records complete elapsed time plus sampled logical/allocated work-byte peaks.
Existing legacy directories are preserved unless created/owned by this planner.
A bounded RGB565 lookup replaces repeated sparse-span color arithmetic, with
unchanged exhaustive color, geometry and alpha regressions. These are source
and recovery gates, not a completed original-game conversion or speed claim.
Original PCK full conversion, exact byte comparisons, peak temporary space and
updated standalone build acceptance remain pending.

# UI23 accepted runtime and completed producer stages

The runtime is frozen at actual build `d9350b94c234f443c29f12f7efb862f3595a6a57`.
Its published source-equivalent checkpoint is
`1298a1bbbbb9355e8c87ad02542ff14a6154730e`, tree
`402461fe462d4ab4c31322f9096ad28c76ef2e94`; all 641 runtime/build inputs and the
linked ARM helper match. The launcher keeps its actual d935 build provenance.

Ordinary USB installation/hash readback, real scene navigation and matched
30-second warm intervals on the supplied 64 MiB Firebird measure Neow at
**11.2930 FPS / 88.0903 ms render** (339 frames, one over 100 ms) and Nonupeipe
at **23.0763 FPS / 42.9760 ms** (692 frames, none over 50 ms). Both make zero
resource read calls/bytes. Compared with f760 these improve FPS by 7.77% and
17.62%, respectively. These are emulator measurements, not physical-device
performance or a universal 30 FPS claim. The unchanged f760 combat renderer
measures 18.3430 FPS with three Affliction families and 9.43565 FPS with seven
on the explicit small3 TEST ONLY fixture; those are not final-full-package
measurements.

The zopfli launcher is 1,736,356 bytes, SHA-256
`9a00d1f6e3e795839914c0f65929b60eafe856c27c726157b055423e9e60dfae`.
Linked allocation is **22,290,500 / 22,291,016 bytes**, leaving 516 bytes;
no heap, cache or arena capacity grows. The largest individual reported stack
frame is 5,936 bytes, not a maximum combined call-stack claim. Ordinary build
entry points enforce the strict limit and keep render/decode hot paths at
`-O2`, while selected cold control paths use size optimization.

The soft-field comparison covers 1,561 cases / 3,122 complete 320×240 frames.
Against 2825, 24.2061% of pixels differ, with maximum RGB565 deltas R3/G5/B4;
this is finite sampling, not an all-clock scene bound. The independent fire-only
proof covers all 200,000 phases / 6,001,000,000 queries and bounds that component
at R0/G4/B2. Later exact post and option-image changes introduce zero changed
pixels across the same 3,122 frames. Final Canvas regression passes 401,774
cases / 3,094,644 checks. Source geometry, full-resolution masks and clocks
remain intact; native-size peak witnesses were inspected.

The actual bad0168 producer's default bilingual and all-16-locale runs both
completed all 83 stages. The 11 required data files occupy **68,155,584 bytes**;
the 14 additional locale files occupy **17,773,035 bytes**. These are resource
subtotals, not complete extracted installations. The official continuous-card
stage adds 53,580 logical bytes, and published English/Chinese data grows by
54,694 bytes relative to the preceding completed UI23 data. The earlier 47,872
figure belongs to a private candidate. Final full/reduced installation, current
real-data guest smoke, complete package sizes and delivery identities are
recorded by the final delivery report; unfinished checks are not asserted here.

Current details and evidence identities are in [runtime acceptance](ui23-runtime-performance.md),
[the machine-readable receipt](ui23-runtime-validation.json),
[continuous card materials](ui23-continuous-card-materials.md), and
[paged history](ui23-history-archive.md). The candidate and historical entries
below retain their own identities and are superseded where this section supplies
accepted runtime or completed producer evidence.

# UI23 exact smoothstep division candidate

ARM relocation inspection shows three fixed-width unsigned64 division calls
and one dynamic signed64 division call in the continuous material sampler.
This candidate uses a bounded32-bit reciprocal and at most one correction
for exactly the same Q20 quotient. Constant widths fold their reciprocal at
compile time; Smog computes its dynamic reciprocal once per layer preparation
in an existing constant slot. Other widths retain the original division. No
uint128 helper, persistent memory, resource format or source image changes.
Private whole-frame comparison passes460 RGB565 frames and zero warm reads;
actual guest net benefit and the strict ARM allocation gate remain pending.

# UI23 Affliction CPU candidate — guest confirmation pending

The real 64 MiB Firebird seven-card fixture with the continuous ACU1 pack
measures 29.784 FPS without attachments, 10.806 FPS with three families and
4.996 FPS with all seven. The three/seven intervals have zero resource reads;
the remaining failure is CPU work, not decompression or cache misses. These
are runtime 438 measurements with the explicitly test-only small3 pack.

This candidate resolves bilinear corners through one sparse tile/page lookup
when possible and skips pixels covered by later exact-alpha255 hand faces.
The owner map borrows 38,400 bytes from the existing foreground scratch and
adds no persistent pool. Normal hand ranks are 1–10, focus 11 and held 12;
unsupported ranks/counts retain unmasked rendering. Other flying/ghost cards
are conservatively excluded as occluders. The cached base face stays intact;
only its final screen blit and the continuous overlay use the map. Native
axis-aligned origin rounding retains the ordinary draw path.

Private paired comparison passes 460 complete RGB565 frames including hover,
held order and 70 cold-cache rebuilds; all pixels match and scratch leases
balance. Field tests pass 345,660 checks, including separate physical pages,
sparse/invalid tiles, clamp/repeat and channel bounds. Host timing is not a
calculator FPS claim. Production ARM size and actual guest net improvement
remain required before adopting this performance candidate. The frozen
bad0168 resource producer is unchanged; its real zero-argument CLI is running.

# UI23 measured guest layers — next bounded kernel candidate

The ordinary 64 MiB Firebird path measures the first grid stage at 7.18894 FPS,
then exact light-support clipping at 8.193739 FPS (121.678816 ms render). Warm
resource reads remain zero. The actual diagnostic layer means per frame are
fire 36.858259 ms, lights 24.012725 ms, fog 13.944416 ms, post 18.058434 ms,
body restoration 3.083147 ms and vignette 3.377486 ms; the rest of App rendering
uses about 22.3152 ms. These results remain below the desired fluidity.

The next candidate uses exact carry detection for low-saturation light/fog
addition, exposes pinned field sizes to ARM address generation without changing
sampling arithmetic, and skips secondary mip weights at most 8/256. That last
change preserves the full-resolution displacement/sample and bounds each post
pixel difference by RGB565 R1/G2/B1. The source fire proof independently covers
all 200,000 integer shader phases (6,001,000,000 queries) and all 546 source
transmission masks: fire contribution error is bounded by R0/G4/B2 for arbitrary
body entry phase. This is a conservative all-phase bound; the prior 26 complete
App/room captures actually differ by at most R0/G2/B1. New complete frames,
strict ARM allocation and real guest timing remain pending for this candidate.

# UI23 exact light support span candidate

A second runtime checkpoint clips each qualified light row to its proven
native source support before the pixel loop. Unsigned reciprocal division
with one correction computes the same integer entry/exit positions for
positive, zero and negative UV slopes. No texel sampling or channel values
change. The independent row/span regression passes 9,608,737 checks; complete
frame equivalence, actual ARM cost and strict linked allocation remain to be
verified before this candidate can replace the measured baseline.

# UI23 production memory closure — guest timing pending

The production Zehn packer now enforces the unchanged 22,291,016-byte ceiling
including loader metadata. Every build entry applies `-Os` to event rules,
progress serialization and UI input dispatch; renderers and shader kernels
remain `-O2`. A diagnostic linked build measures 22,285,880 bytes; the clean
checkpoint build and actual guest timing remain pending. No arena, heap or
cache capacity has been increased. History decoding clears its existing
output in place instead of creating a 33 KiB temporary stack object.

The bounded Neow fire grid retains the full-resolution transmission mask and
uses exact samples on sharp transitions and quad edges. Actual source packs
at 13 clocks produce 26 full 320×240 App/room images: 0.223% of pixels differ,
with maximum RGB565 channel deltas R=0, G=2, B=1 and mean channel-code error
0.0008013. This verifies bounded visual differences, not guest frame rate.
The baseline remains the historical 5.595 FPS scene until a new ordinary
64 MiB Firebird measurement completes. Failed initial ARM allocation and
missing-Unicorn validation receipts remain preserved; neither is acceptance.

# UI23 current continuation — Neow renderer checkpoint, acceptance pending

The recovered active source is `2825d65`; the older local `5ad3be4` checkout
and its dirty edits are preserved separately and are not the current baseline.
The ordinary 64 MiB Firebird path remains available. The latest historical
Neow result is 5.595 FPS / 178.360 ms; that is an open performance failure.
The linked allocation limit remains 22,291,016 bytes, without a heap increase.

This source checkpoint advances light UV row quotients without per-row wide
division, restores exact source support rejection for qualified light textures,
and skips fully transparent Neow post-process quads. A pinned-field Neow fire
path uses twelve fractional bits per bilinear weight while retaining source
texture bytes, dimensions, Q20 UV coordinates, geometry and clocks. Each field
sample differs from the original normalized filter by at most 513/Q20; complete
shader/frame error and guest performance still require actual-source validation.
Other valid field layouts retain the original equation. Actor raster quality
and action frames are unchanged. No resource/cache/heap capacity is increased.

Focused checks pass 4,800,000 exact light-row comparisons, 250,240 fire table/
sampling checks, 95,492 Neow shader/mask checks and 1,987,948 exact post checks.
The recovered light-support probe historically matched all pixels and reduced
its native thread-CPU interval 8.6%; this is not a new device measurement. Native
real-pack layer profiling identifies fire (~50%), lights (~22%), post (~15%) and
fog (~10%) as the current hot paths. Actual-source visual comparison, production
ARM size, full native gates and measured Firebird improvement remain pending.
Final resource conversion, continuous affliction materials, history storage,
and three matching release ZIPs remain open and are owned by parallel tasks.

# UI23 complete-resource candidate — public full replay passed; performance open

As of 2026-11-01, the complete 7229/60294 Ancient catalog includes the previously
missing Tez and Neow fire families. The actual public Linux standalone789 full
conversion completed all 82 stages for all 16 languages, with source unchanged.
All 25 physical resource files were independently read back: 85,869,477 bytes.
The 11 files needed for English and Simplified Chinese total 68,100,890 bytes
and exactly match the previously inspected private candidate. All 16 logical
roundtrips and final schema checks pass. Full output and its receipts are
preserved independently of the next profile writer.

The previously inspected complete installation totals 69,890,968 extracted
bytes, 109,032 below 70,000,000. That measurement includes the older 78930d5
launcher, all licenses, instructions and its compact manifest. It is a passing
candidate measurement, not the size of a newly rebuilt final package. The
new 35e06b1 launcher is 1,716,916 bytes and final packaging must be measured again.

Actual 64 MiB Firebird execution of runtime 78930d5 with the complete 7229 full
resources measures Neow at 4.09125 emulated FPS, 243.82751 ms mean render time
and zero warm resource I/O. The exact fire optimization in 35e06b1 improves
that interval to 4.57953 emulated FPS and 217.7469 ms, still with zero warm I/O.
Neow remains below the intended performance goal; further faithful hot-path
work is in progress. The 35e06b1 production allocation is 22,264,384 bytes,
26,632 below the unchanged 22,291,016-byte limit. Its actual linked ARM decoder
oracle and formal/native builds pass. The earlier 6,144-output ARM kernel
comparison and all 26 complete RGB565 images remain exact; their kernel
instruction reduction is not presented as the whole-scene frame-rate gain.

This source checkpoint additionally reduces exact shared Ancient light sampling
and Neow post-processing work. The light lookup validates the complete logical
texture before opting in; nonqualifying images retain the existing path. The
new Character cache occupies existing scene-union slack: the native union
remains 230,416 bytes. Corrected tests cover 157 legal colors and rejected
alpha/color/alignment cases; 5,760 complete Ancient frames and 24 raw captures
match the prior renderer. The post-process uses a proved sampling footprint
and shared UV work; all 26 complete full-App/room images match at 13 clocks.
Its measured ARM kernel instruction reduction is 18.2343%, not a whole-scene
FPS result. Two independent reviews cover light qualification/lifetime and
post ROI/mip/stride/temporary-workspace bounds. No resource sizes, actor frame
counts or clocks change. The combined formal ARM allocation and actual guest
frame-rate comparison remain pending; noisy concurrent host timings do not
establish the light path performance gain.

The 78930d5 Nonupeipe interval measures 11.03037 emulated FPS, 90.1956 ms render
time and zero warm I/O. Final-7229 Random selection measures 823 frames in
30 seconds, 27.420697 emulated FPS and 35.823409 ms, with zero warm I/O. Against
the same runtime with older 7227 data (27.452703 FPS), the frame-rate difference
is -0.1166%. This closes that selection scene's resource comparison, not
all-scene performance. These measurements use the supplied 64 MiB emulator
firmware; no physical-calculator measurement is available.

The stable continuous final-7229 Vantom guest path now passes actual Menu to
turn two, normal save/exit and byte-exact saved State after Continue. Its
measured warm interval is 30.0055 emulated FPS, 24.4105 ms render time and zero
resource I/O. The earlier resource-loading failure after restoring an early
combat snapshot, including its cumulative failure counter of one, is preserved
as failed evidence. Continuous controls and the actual save/Continue path pass,
but the cause of the early-snapshot failure is not proved. Native 910-frame and
actual saved-State/four-file-cache controls are supporting evidence, not a
substitute for the completed guest operations or a claim about every battle.

Execution provenance remains explicit. Public full/all16 output was produced
by 78930d5. The 35e06b1 standalone is now running the actual original-folder
reduced/all16 graph with a distinct output; final default-bilingual execution
is held until the runtime performance version is settled. The observed789 full
run is not relabeled as 35e, and partial tails or self-tests are not whole CLI
completion. The preserved matching full common/locale set supplies its own
optional languages; reduced output must supply a separate matching set.

The 789 CI passes 174 native suites, 595 conversion tests plus 56 tooling tests,
the portable gate and Windows build/self-test. The 35e Windows artifact is
independently verified against its exact embedded public source and ARM ELF;
its actual source-mode and frozen linked decoder self-tests, 66 recipe imports,
UTF-8 and Unicode path checks pass. Its explicit self-test faulthandler prints
a nonterminal Unicorn initialization diagnostic before successful real decodes;
the ordinary conversion path does not explicitly enable that handler. Windows
original-folder conversion has not been run. Linux resource-profile execution,
Windows self-tests and emulator measurements retain these distinct scopes.

Narrative corrections are separately bound to 920f7c5, installation/package
documentation to f4f94c1, and a README assertion-only correction to a0d3695. None
relabels an earlier executable. Final release still requires the remaining
public reduced/default completion, whole-archive size/readback and adequate
runtime performance. Existing save data must be preserved. The records below
retain their historical checkpoints and narrower evidence scopes.

# Historical checkpoint: UI23 Tez fire and bounded temporal storage stage

The complete 237 Tez fire instances are implemented in source order with the
90 lights and 74 cinder emitters. Their geometry follows the room/body clock;
shader TIME uses the global presentation clock and its own 20-second period.
The full source catalog is now 7228/59751. The remaining functional Ancient
family is Neow's four fire layers behind its body, including real foreground
transmission and additive clamping; that work remains in progress.

The Tez shader retains original source texture resolution and eight-bit noise
in this checkpoint. A reversible per-page vertical predictor reduces the actual
append from 575,438 to 494,830 bytes. The existing image cache reconstructs each
page once on a cold access, using its metadata flag and no new buffer. Actual
Content reads of every byte, preloaded raw entries, warm repeat requests, aliases,
eviction and reopening pass 6,632 checks for each raw/predicted mode, including
GCC UBSan on the predicted pack. The largest decoded group remains 250,240 bytes.
The 18,332-byte prior ARM margin must be remeasured on this new source.

An independently reviewed temporal hold reduces the same-input 1,052-frame
body append by 449,834 bytes. It retains all source frame times, dimensions and
R1/G2/B1 RGB565 error bounds, leaves static pixels exact, and preserves original
wrap endpoints where a common legal end color does not exist. There is no
additional green quantization, shared spatial palette or runtime operation.
Retained body groups decrease from 43 to 35, and decoded input from 5,360,643 to
4,311,827 bytes, with the same 155,280-byte maximum group. Source-free tests and
all 1,052 accepted frame hashes pass. Separate consumer modules leave every
original sampler dependency unchanged and run the original full-frame validator.

The previously published 7227 baseline now has actual complete bilingual
physical data: 68,571,533 bytes, plus its measured 1,716,856-byte launcher,
70,288,389 bytes before installation documentation. It is over budget and is
not a final release. New hold, fire, future Neow data and final packaging must
be measured together. Both editions still enforce 70,000,000 bytes for every
extracted archive member; no over-budget release override is authorized.

Packaging retains all 64,024 bytes of license notices and complete bilingual
instructions. Full validation records are retained outside the installation;
the installed manifest keeps their digest, length and compact result along with
program/resource hashes. Optional locales must match both the common data from
the same conversion and the animation profile. Existing state is preserved.

Actual full-App testing of the earlier 7227 data executes 8,160 frames over all
eight Ancient rooms. Each room's measured 360-frame interval follows 660 warmup
frames; every interval has healthy content and zero resource reads. These are
host correctness/I/O results, not physical-calculator FPS. New fire/hold and the
final physical data still need that full-scene gate and unchanged-64MB firmware
execution. The standalone Windows converter at d5ae67e passes actual Actions;
that older result will not be substituted for the final rebuilt converter.

## First combined production ARM measurement and CI correction

Published 6cdf8ac links at 22,272,684 bytes, 18,332 below the unchanged UI21
allocation ceiling. The genuine linked ARM literal and predicted streams pass.
This measures the implemented 7227 source stage; the final two fire families
still require another linked allocation and actual scene test. Its source is
frozen independently for native/full-App verification. Follow-up d5ae67e
restores the exact published 346f8ad light sampler and its strict 7217 validator;
the cumulative validator already supplies that exact prefix. All seven sampler
dependencies match, so the 1,052 real source poses are reused without signature
bypasses or resampling. All eight new producers have now executed from the real
7217 input, adding 360,222 bytes before the final physical-pack tail.

The first combined conversion CI ran 578 tests and found one stale Tanx recipe
assertion: it assumed quality followed lights immediately. The recipe now has
eight intervening completed producers. The regression follows and verifies every
input link from quality back to lights while retaining the Tanx and mandatory
final-prune checks. Its focused four-test suite passes; the subsequent full CI
result is still required. The cancelled older Windows job was superseded by a
new source push, and is not reported as a passing Windows build.

# UI23 cumulative Ancient integration — source stage, final acceptance open

The combined source includes the original projected additive light plates,
Orobas and Tanx water materials, stationary GPU sparkle emitters, all 74 Tez
cinder emitters, the Vakuu cinder emitter, six Nonu cloud nodes, twelve turbulent
particle nodes, and Neow clouds, CPU grain stars and screen-texture post effect.
The typed converter/runtime catalog admits exactly the complete prefixes
through 7227/59732, including full/reduced material and post-effect modes.
The remaining 237 Tez fire nodes and four Neow fire nodes behind the body are
still implementation gates; this checkpoint does not label their probes or
unimplemented source descriptors as finished features.

Original actor image dimensions, action clocks and sampled frame cadence are
unchanged. The existing scene union now also holds the cinder state and exact
static-plate cache, without increasing its 230,416-byte host size. Neow screen
capture borrows the existing transfer arena only after loading its source view.
No production image cache, transfer arena or particle pool capacity was raised.
Neow's shader uses the global presentation clock; particle births retain their
room-relative clock. The reduced profile uses its explicit source clock zero.
The final ARM allocation must still be at most 22,291,016 bytes; the combined
source has not yet completed that linked measurement or guest performance gate.

Actual original-source conversion has reproduced the historical 69-stage
bilingual data set. Mandatory real-ARM index/glyph validation then reduced its
physical data from 68,849,266 to 68,047,223 bytes. All six ENG/ZHS label/card/shop
Canvas comparisons match the original and previous accepted pixels exactly.
Adding the measured earlier 1,686,552-byte launcher gives 69,733,775 bytes,
before licenses, instructions and this checkpoint's new art. This is a budget
baseline, not the final complete-install size. The source-derived 1,052 Neow/Tez
body poses add 118,682 bytes over the corresponding same-input older stage;
there is no assumed body-size saving from separating the vignette.

The final trajectory layout stores 145,062 grouped bytes for 392,320 decoded
bytes. One 131,072-byte table is carried in a 320x205 word view with exactly
128 zero padding bytes; its used bytes and every position key are unchanged.
This keeps Content's existing 240-row limit. The earlier 256x256 view was
correctly rejected by the full decoder boundary during source integration. All 45,484 retained position keys invert exactly to the first layout,
and 2,647,470 actual runtime particle queries agree across the two layouts.
Its deterministic source seed bank bounds position interpolation at one eighth
of an LCD pixel without reducing actor art. Projected materials add 119,948
full-profile grouped bytes; stationary masks add 25,692; Nonu clouds add 44,006.
These are component costs, not a physical final-pack sum.

Focused source checks pass: 63 earlier central tests through 7224, then 50
focused tests through 7227 including the final trajectory geometry; 138
native UBSan format/clock checks; three Neow particle format tests; 282,462 independent gravity/gradient/
clock checks with GCC UBSan, 616,971 Vakuu lifecycle/untouched-state checks,
and 1,376,256 exact cloud additive-blend comparisons. Clang's installed UBSan
runtime archive was absent; GCC supplied that sanitizer run. A new test first
missed an initializer-list include, then compiled and passed after correction.
A transient full temporary filesystem interrupted copies/compilation; the one
new zero-byte producer was rewritten atomically and source hashes rechecked.
No failed/interrupted run is counted as passed. Full App syntax is checked
again after all central catalog edits settle.

The cloud fast path uses a 512-byte local table for constant-color source
contributions; its final saturation is exactly the ordinary Canvas equation.
Canvas states with coverage, tint, occlusion or color transforms keep the
ordinary path. This is not a persistent cache enlargement or an FPS claim.
The Neow post comparison retains sharp 320x240 output; the checked kernel
has at most one RGB565 code of difference per channel on the three tested
source body views. The complete fire/fog/light composition remains to verify.

All new original-derived data, comparisons, firmware and recovery receipts stay
private. Final full/reduced all-language conversion, complete scene correctness,
ARM allocation, unchanged-64MB supplied-firmware performance, 70,000,000-byte
whole-install gate and the three matching release ZIPs are still open.

## Actual trajectory samples and lossless layout follow-up

All12original-scene emitters sampled successfully:490particles and329,890
source position samples,392,192decoded bytes in99existing-cache pages. The
first grouped representation stored180,264bytes. A byte-plane layout of the
identical six bytes per key reduces this to145,016bytes (35,248bytes saved),
with no coordinate/color/alpha/frame change, no predictor and no new buffer.
The two groups retain their211,968-byte maximum decoded bound. Block-delta
alternatives stored151,848–170,150bytes and were rejected. Table version2
records the byte-plane layout. Actual final physical-pack append, warm Canvas
cost and guest/ARM allocation remain release gates.

# UI23 bounded source turbulence — source checkpoint, release gates open

An isolated source stage adds the original Godot4.5.1 four-dimensional gradient
and curl equations to the desktop converter helper. It samples all12remaining
Pael/Orobas/Nonu/Vakuu turbulent emitter definitions, preserving their original
490particles, texture alpha, lifetime/preprocess, force/display curves and node
identity. Source sampled motion is compressed to a proved maximum1/8LCDpixel
position error, including integer interpolation, without changing actor images
or actor frames. The calculator uses one existing decoded image view per emitter
for its mask, display ramps and tracks; it adds no persistent simulation pool.

This is a deterministic source seed bank, not an unknown original GPU random
history. Godot's fixed_fps0 preprocess uses30Hz; its unrecorded variable-frame
history is represented at that same30Hz and interpolated on the display clock.
The source Orobas orbit value.0090001 takes the original abs(value)<.01 early
return. Nonu's28.849997 term is radial acceleration, not radial velocity.

Three focused Python source/continuous-error tests,101native format/clock/bounds
checks and Clang18 full-App syntax pass. An initial malformed-record test selected
another valid bank track; the test now corrupts both records and passes. Actual
original-data trajectory sizes, full Canvas visual/cost checks, ARM allocation,
final typed Content/converter/no-animation integration, and the70,000,000-byte
complete installation gate remain open. Planned extension is7224/59729, with
exactly12real emitter data leaves after the revised7223/59717fog predecessor.
Pael/Orobas rendering hooks are included; Nonu/Vakuu source-order insertion is
part of the pending combined integration. No final release claim is made here.

# UI23 integration checkpoint — 2026-11-01, acceptance still open

The active branch remains `work/ui23-integration`; PR #3 is not merged. The
package table below records an earlier fallback and is **not the acceptance
record for the current source**. The current work includes the previously
omitted Ancient particles and materials; their remaining implementations and
actual converted outputs are release gates, not exclusions.

- The user requires the entire extracted full-animation English/Chinese
  installation, including launcher, notices and instructions, to fit within
  **70,000,000 decimal bytes**. Preserve the existing sampled actor dimensions,
  frame cadence and action clocks. Final matching three-ZIP packaging is open.
- The compiled allocation ceiling is **22,291,016 bytes** (UI21), superseding
  the older 24 MiB allowance. Runtime `346f8ad` links at **22,224,488 bytes**,
  leaving 66,528 bytes without increasing production cache capacities. This is
  linked allocation, not measured physical peak RAM.
- Published `29e83e4` adds a reduced-profile Ancient raster cache inside the
  existing scene union. Published `460fae8` losslessly factors text-index
  prefixes and suffixes: all 4,361 text keys and 6,855 registry entries retain
  their mappings, with 69,284 fewer read-only table bytes. The 460 launcher is
  1,686,552 bytes; its physical-device performance remains unmeasured.
- Published `2a9d55b` provides the ARM-compatible Tanx ash/fog stage.
  Published `346f8ad` restores source-ordered Neow/Tezcatara CPU lights. All
  546 Neow and 506 Tezcatara source poses have completed fresh sampling;
  matching final data append, remaining materials and particle systems, and
  full-scene acceptance are still in progress.
- Fresh original-directory conversion is being reproduced from the pinned
  v0.111.0 / 41cef1ea inputs. Historical lossless index retirement and glyph
  savings must be replayed on the physical final outputs before size or
  converter completion is claimed. The same launcher must detect both the
  full and NAF1 reduced profiles; reduced action timing remains unchanged.
- Supplied-firmware Firebird now works at unchanged 64 MB DRAM and normal
  396/198/99 MHz CPU/AHB/APB clocks. Ordinary USB installation and hash-equal
  readback of all 13 baseline files succeeded, followed by ordinary launcher
  startup. Controlled before/after timings and final-build gameplay are open.
  No physical calculator connection is available; host, Unicorn and emulator
  results must remain explicitly distinguished from physical-device evidence.
- Published `31d3ff1` fixes the confirmed Windows frozen-converter failure
  with a source-pinned private PyInstaller bootloader. Actual Windows Actions
  run 41270461958 passes source and frozen execution of both same-ELF ARM
  streams, plus all 56 recipe-module/dependency/helper checks. ASLR, NX and
  high-entropy addressing remain enabled; system policy is unchanged. A full
  original-game conversion on Windows has not been run.
- Actual supplied-firmware warm Pael comparison on identical older reduced
  data and the same saved run: `895ecca` averaged 21.094 emulated frames/s and
  46.995 ms rendering; `460fae8` averaged 29.991 frames/s (30 fps cap) and
  29.835 ms rendering. Each interval is 30 seconds, derived by subtracting
  ordinary-exit counters from matching per-source NAND/RAM snapshots. Both
  have zero warm reads. This is a 36.5% render-time reduction for this one
  reduced-room case, not a physical-device or all-scenes performance claim.
  Normal new-run entry, save-and-return, exit and continue have executed on
  this controlled baseline; final-source/data gameplay remains open.

Original game assets, source-derived packs, private captures, firmware,
conversation attachments and private storage identifiers remain outside Git.
The sections below are historical until replaced by final matching receipts.

# Historical UI23 fallback snapshot — not the current acceptance record

Continue on `work/ui23-integration`; this source has **not yet been merged to
`main`**. UI23 supersedes the historical UI22 lossless-only policy and soft
storage target: bounded color/alpha changes are permitted while preserving
sharp 320×240 output, geometry, sampled animation cadence and action clocks.
The full-animation edition remains the default. The no-animation edition
retains essential source poses and original action timing, detected by the
same launcher through its NAF1 content profile.

## Frozen runtime, resource provenance and memory

- Runtime and full-animation resources:
  `ee6575e5678a0d49d998e2f324c67619f2c71955`.
- Exact no-animation resource checkpoint:
  `7409f1cc4a8f71ad948037b6b9644a6eb1b056bd`; it uses the same final launcher.
- Final launcher: **1,689,275 bytes**, SHA-256
  `77c82e9dec7cbe934419435e478216073897f0daccee4ef7e85ca0c9b3719fc6`.
- Final formal ARM allocation: **22,290,328 bytes**, 688 bytes below UI21
  (22,291,016) and 244,996 below UI22 (22,535,324). Usable cache capacities
  remain unchanged. This is linked allocation, not physical peak memory.
- Package builder:
  `a74fc64edb3816f1dd9160d9dec97ac529dcfb16`; edition receipts record their
  actual resource checkpoint rather than attributing both to the packager.

## Three completed package artifacts

All three ZIPs were built, checked for CRC errors, and read back byte for byte.
Both complete bilingual archives passed the strict **70,000,000-byte** gate
for the **entire extracted archive**, including documentation and licenses.
The required full-animation launcher/data target is 65–70 decimal MB. Save
state is additional. ZIP transfer size and installed size are distinct:

| Edition | ZIP bytes | Launcher + required data bytes | Entire extracted archive bytes |
| --- | ---: | ---: | ---: |
| Full animation, English + Simplified Chinese | 68,903,208 | 69,725,667 | 69,869,712 |
| No animation, English + Simplified Chinese | 28,423,751 | 30,263,570 | 30,408,109 |
| Optional languages, matching sets for both editions | 32,396,898 | Not a standalone installation | 34,598,901 |

ZIP SHA-256 values:

- Full animation:
  `9bc2ef5e48d54151391a4fbde79933cbe29524005dca1684b98070560ab4987c`.
- No animation:
  `4611046f6beb9bc7aa2d60715cc09a3b4c33f90e12291233a4379cb898eeebcd`.
- Optional languages:
  `db2631548dea6bbc33fc2304817386acfb16ce61af66f8b50f716b607fbb84f3`.

The final visual review covers actual 320×240 production rendering. Neow and
Tezcatara now have their authored 546- and 506-frame loops, with bounded cyclic
color hold and bounded 32-frame replay in the full edition. Production Canvas
and full-App English/Chinese checks passed; both Ancient rooms had zero warm
resource reads with normal dialogue and choices, within the existing cache
limit. These are host correctness, resource-I/O and CPU observations. The
isolated final warm-selection comparison did not establish a meaningful CPU
speedup, and no calculator FPS improvement is claimed.

## Remaining acceptance gates and scope

- **Physical device and Firebird:** no physical calculator connection or
  working Firebird firmware is available. Neither native execution nor ARM
  build/instruction checks substitute for device testing; historical runs
  are not UI23 runs. Production memory/cache limits were not enlarged to
  manufacture passing tests.
- **Original-game converter:** the fresh original-source conversion and
  subsequent frozen-executable replay are still running. The stage snapshot
  and resume fix is pushed at
  `68f46e93ba87ce195453038f1105aae9f0fb5ae5`. Linux and Windows bundled
  dependency/helper self-tests passed, but they do not prove a complete
  original-directory conversion. Do not mark that deliverable complete until
  the actual end-to-end output and publication guards pass.
- **Remaining visual parity:** the two authored Ancient actor loops are
  restored, but independent Ancient particles and material clocks remain
  partial, including effects in the six painted Ancient scenes. UI23 does
  not claim full Godot rendering parity. Audio, multiplayer and Timeline
  remain outside the agreed scope.

See [storage and runtime evidence](ui23-color-storage-evidence.md), the
[retained-differences ledger](ui23-retained-differences.md), and the
[converter guide](converter-ui23.md) for detailed proofs and remaining work.
Original source assets, derived packs, private captures and firmware remain
outside this public repository.

The frozen UI22 record below is historical; its branch, version and acceptance
instructions do not supersede this UI23 snapshot.

# Historical UI22 completed matching calculator release

The current version is **0.1.0-ui22**, with **NSP2/46**, 58,613 images,
1,783 clips and 7,212 auxiliary entries. Continue from the verified UI22
source/data on `work/ui22-integration` and its synchronized `main`, not the
historical UI21 resources or early UI22 candidate layouts. The pinned original
reference remains **v0.111.0 / 41cef1ea**. Both complete calculator ZIPs have
been built and read back with matching artwork and all sixteen original locales.
Original data, derived packs, private source captures and firmware stay outside
this public source repository.

## Frozen runtime, resources and deliverables

- Runtime and final ARM build: `b9425530b24b0d851b57892872fc07865859cea1`.
- Final accepted data/converter and packager: `1128b795557fee3ee31444e323f58f073bbcd922`.
- Independent opaque-alpha representation oracle correction (test only):
  `dc175a9ca060478298e08abdaa9f35a22ca65a33`.
- Launcher: **1,688,294 bytes**, SHA-256
  `5d07224f6b5dc92041e9edba3eba7e328dc024deda8e30f9f3ddceefd5d03362`.
- Final linked ELF: SHA-256
  `e749c0dd19bb5797186be0d6e5391c11bec1d4790bd8f77d14a07f1c1635202e`;
  formal allocation **22,535,324 bytes**, 2,630,500 bytes below 24 MiB.
- Final logical English: **94,924,498 bytes**, SHA-256
  `e2fc2beb4fe1e4d9ff5e54188f0c134bc9da2ee680ef2acc06ad5f7cd6784147`.
- Final logical Simplified Chinese: **99,349,212 bytes**, SHA-256
  `e1add5f33fbbf8f2093fcf3676200518a0550808ad575bf6e9b0071b3ce7b3d6`.

| Private delivery | ZIP bytes | SHA-256 |
| --- | ---: | --- |
| Slay-the-Nspire-2-0.1.0-ui22-bilingual-calculator.zip | 98,728,539 | `e75fa7e3c8e4bec3e4bc2f4381b4fb760aead6ab55b3236c5c24daa5196f8e9f` |
| Slay-the-Nspire-2-0.1.0-ui22-optional-languages.zip | 17,321,125 | `bda9f42a2dcf62f0afa9799ef2b0626d204a11a5c49b3da07facb709bf2defc6` |

The complete bilingual launcher and required `.tns` data occupy
**99,526,983 bytes before saves**. Including README, build information and
licenses, the extracted bilingual ZIP totals **99,650,627 bytes**. All fourteen
optional `.tns` locale files add **18,854,842 bytes**; with their README, the
optional ZIP extracts to **18,855,677 bytes**. Install only the desired additional
locales, but keep the complete matching index and eight shared artwork parts.
Exit before upgrading, preserve the whole `state/` directory, and replace the
launcher, complete `data/`, and any optional locales with the matching UI22 set.

## Storage and source precision: two distinct comparisons

Against a control with the same sampled RGB565/alpha8 precision, native actor
density and active content, the required bilingual installation decreases
**107,729,436 → 99,526,983 bytes**, saving **8,202,453 bytes**. The English logical
comparison is **103,133,990 → 94,924,498 bytes**. These catalogs differ in added
selection/retirement metadata and removed unreachable legacy payloads; they
are not claimed to contain identical unused entries.

Historical UI21 occupied **73,295,445 bytes** and inherited UI11's lossy color
quantization. UI22 is therefore **26,231,538 bytes larger than UI21**, while
restoring the source's sampled RGB565 words and alpha8. Do not hide this increase
behind ZIP size, MiB/MB units, or the same-quality control comparison. The earlier
65–70 MB ambition was a historical soft target, not a newly confirmed amount
of free space on the user's calculator. Actual required storage is stated above.

Accepted lossless changes include bounded selection key/copy geometry,
clip-local eight-frame index groups, same-tile span merging, exact legacy and
retained-stream cleanup, CPU-gated existing TGR/TGI2 stream choices, ordinary
FFFD geometry, and removal of 988 index records with no image descriptor.
The last pass really removes those records from 21 groups, retains every live
record and remaps its raw offset; it saves 249,856 bytes. Complete final-ELF
LZMA2 plus index inverse work falls 96,820,329 → 11,989,047 instructions across
those groups, with no group regression.

**Do not adopt FFFC selection bitmaps.** The exact 163,840-byte candidate failed
to demonstrate an acceptable same-core opening/switching CPU result. The final
pack has no FFFC records. Its public offline converter now requires explicit
experimental opt-in and marks outputs not release-approved. Its bounded unused
runtime support is not a claimed released storage gain. Random optimization,
XOR residual, lossy quantization, masked re-dicing and the negative temporal,
atlas, larger-dictionary and retained-image experiments are not shipped wins.

## Completed acceptance and its limits

- **162/162 native CTest targets** execute and pass on the frozen b942 core.
  Final English/Chinese packs pass **18 actual UI/Osty runs**, alongside the
  dedicated filesystem, tooltip, input-mode and long-locale regressions.
- Final **58,613 images × memory/ranged/preload** compare to the independent
  original-reference spool: **1,065,442,263 exact source bytes per backend**.
  The final run reports 200,421,907 checks and 28,655,100 preload steps; after
  Ready, image access adds zero source reads. Thirty-three fully opaque images
  use the old runtime's equivalent implicit-alpha representation, with exact
  original colors and explicit-alpha reconstruction checked independently.
- All sixteen final locales reconstruct exactly and preserve the original
  font/text payloads: **344,590 glyph entries and 139,264 string entries**.
- All 58,099 inherited image provenances are classified. Fresh official-source
  sampling verifies **427 actions / 11,051 retained sampled frames / 11,134
  pieces**; actor Canvas tests include noninteger projection, additive blending,
  opacity and grayscale. Late registration fixes and the Vantom typed assets
  are preserved instead of overwritten with old clip slots.
- Linked ARM passes **4,945 dense/sparse/LZMA2/memory vectors** with guards and
  ABI checks. A 33-symbol, 45,556-byte machine-code/constant/relocation closure
  binds the complete-stream profile measurements to the final ELF.
- Full-precision whole-App comparisons cover cold selection, opening loops,
  fast character switching, Random, and seven combat scenes including
  Necrobinder/Osty/Unleash and Ceremonial Beast/Plow. Every compared LCD
  fingerprint agrees; independent source/Canvas gates establish pixel identity.
  In the final same-core ABBA, selection advance performs zero resource reads.
  Render, advance, initialization, FNV and logging are separated; warm timing
  scatter is retained instead of reported as a universal speed improvement.

No current physical-calculator FPS, flash-latency, or Firebird run is claimed.
Existing stream ARM gates cover LZMA and inverses; the new FFFD alpha-to-geometry
reconstruction is covered by native exact-resource/lifecycle and actual-App
measurements, not by a complete ARM `Content::image` performance fixture.
The allocation figure is a formal linked limit, not a physical peak-memory trace.
Audio, multiplayer and Timeline remain excluded. Other established calculator
adaptations remain documented; registry/model counts do not establish complete
original-game parity.

## Rules for the next iteration

Start from the matching UI22 ZIPs and reconstruct their NSP2/46 resources;
do not repeat the UI10→UI21 recovery or regress to quantized inherited assets.
Keep source RGB565 words, alpha8, hidden colors, clocks, pivots and registered
native density exact. Conversion defaults must stay lossless; legacy quantization
is only an explicit historical-migration operation. Preserve all alias and typed
references when reclaiming resources, especially Vantom's repurposed frame slots.
Any smaller compressed candidate needs complete decoder and relevant cold/warm
scene evidence; size alone is not acceptance. Keep the 24 MiB allocation bound,
matching-language installation, lazy resource reads and absence of startup-wide
integrity scans. Checkpoint source before long work and retain negative results.

Detailed evidence is in [storage/performance](ui22-storage-performance.md),
[selection](ui22-selection.md), [source precision](ui22-source-precision.md) and
[UI acceptance](ui22-ui-acceptance.md). Historical stages below remain separate.

# UI21 historical completed matched calculator release

The historical UI21 release used `work/ui21-integration`, based on UI20
`f4875c2eee50ecdee4793d7ecb365a20d34857dc`. The owner's supplied conversation
attachment was read in full. The accepted UI19 bilingual and optional-language
installers were recovered, and the supplied four-part original game archive
matches the pinned v0.111.0 / `41cef1ea` reference. Both complete UI21 player
ZIPs are built with the matching repaired artwork and all sixteen locales.
Original assets, derived packs, private captures and calculator firmware remain
outside the public repository.

## Frozen source and complete artifacts

- Runtime: **`7b0cd8b4f2fd824eea781e0e0a853011c3474b1c`**.
- Permanent filesystem regression: `c8dbcd16739762b389858e912742dd384f301f96`.
- Final offline selection policy: `9d5f9966bb061cec48f7f6efeef131aa243540c6`.
- Packaging instructions: `01176770c7bcb29c63e03f0bdf302fcd701669ac`.
- Final launcher: **1,679,979 bytes**; formal allocation **22,291,016 bytes**,
  below 24 MiB. The same-toolchain UI20 launcher is 1,679,626 bytes: the new
  sparse inverse adds 353 bytes, not a launcher-size reduction. The older
  delivered UI19 launcher was 1,724,027 bytes.

| Private release ZIP | ZIP bytes | SHA-256 |
| --- | ---: | --- |
| Slay-the-Nspire-2-0.1.0-ui21-bilingual-calculator.zip | 72,473,950 | `172789594bfa5509b4f1567db5843abb90e6402e657ab1b4f4dda359d5f10cef` |
| Slay-the-Nspire-2-0.1.0-ui21-optional-languages.zip | 17,035,233 | `752f0b338622f0f1917cbbd947c26f83ddac9badd4bdf0743b53d6d9e777a7e7` |

Default installation is **73,295,445 bytes** for the launcher and required
`.tns` resources, before saves; the complete extracted bilingual ZIP including
README, build information and licenses is **73,367,990 bytes**. The fourteen
optional locale files add **18,507,815 bytes** if all are installed; users can
select only the languages they need. Preserve `state/` and replace the complete
matching launcher/data set, including any previously installed optional locale.

The comparable UI19 default `.tns` installation was **74,538,517 bytes**.
UI21 is **1,243,072 bytes smaller (1.67%)** while adding **1,105,032 bytes** of
repaired source-derived artwork. Against the same complete artwork, the data
pipeline saves **2,304,056 bytes**. The historical 65-70 decimal-MB target is
not reached: 73.30 MB is 69.90 MiB, and the unit difference is not an extra gain.

## Completed functional and resource acceptance

- Rebuilt complete English artwork is 71,028,106 bytes, SHA-256
  `d823e669731c75bcf7f62f307f699c0212a211af0f236c36383a4b6777cf66b5`,
  reproducing the historical repaired artwork exactly. Eight complete Ancient
  backgrounds, nine native-density enemy clips, Osty's terminal pose and
  rarity-specific reward icons are present in the actual delivered data.
- All **159 native targets** have executed and passed. The new public test
  runs four lifecycle scenarios in twelve independent processes using the
  production Ndless run callbacks and synthetic resources.
- Final English/Chinese resources pass **16/16 production-Canvas UI runs** and
  **20/20 filesystem/icon subprocesses**, including title abandonment, pause
  abandonment, death, act-2/act-3 abandonment and independent restart. Active
  sav/bak/tmp files retire; unrelated profiles and preferences remain intact.
- **58,099 images** pass exact memory/ranged/preload reader comparison. The
  107 groups changed by the final density decision additionally pass complete
  95,118,870-byte record comparison and production-reader image coverage;
  unchanged group streams and image references are byte-identical. Final
  glyphs, strings and clip descriptors retain the matching source semantics.
- The source oracle and final Canvas pass **328 frames / 351 pieces**. Ten
  frame captures remain byte-identical after the final compression and runtime
  changes. Osty's existing death animation completes before its source corpse
  loop, with the existing revive action retained.
- Linked ARM passes **44 sparse vectors**, **4,799 memory vectors** and exact
  ABI/bounds checks. Sparse inverse instruction work falls 10.39-66.92% in the
  twelve controlled windows. Complete decoder samples include low/mid/high
  occupancy improvements of 22.77%, 12.25% and 8.65%; these are instruction
  counts, not device frame rates.

Dense low-benefit sparse conversions and font/text retuning were rejected
after complete ARM decoder measurements showed extra work. The release caps
new sparse selection at 60% occupancy and preserves prior font/text streams.
The 29,614-byte font-profile candidate, color-plane alternatives and cold-TU
compiler candidates are not shipped gains. There are no added resident pixel
buffers or startup-wide integrity scans.

## Evidence and retained platform boundaries

See [ui21-storage.md](ui21-storage.md),
[ui21-ui-acceptance.md](ui21-ui-acceptance.md) and
[ui21-art-and-font-storage.md](ui21-art-and-font-storage.md) for measured
acceptance, rejected candidates and exact resource identities. Continue from
these complete UI21 artifacts rather than the historical launcher-only UI20
checkpoint. No currently recovered Firebird BootROM/NAND was available in this
session, so no new ordinary-OS or physical-calculator FPS is claimed. Static
Ancient compositing, sampled LCD materials, the established display/input
capacities and the existing audio/multiplayer/Timeline exclusions remain
explicit platform boundaries. The preceding UI20 handoff is retained below
for provenance.

# UI20 continuation — historical source and launcher checkpoint

Use the current `work/ui20-integration` history, not the archived UI19 handoff.
The source was recovered from remote UI20 `39fc6f1`; no private resource pack,
PCK, generated UI20 artwork or Firebird image was recovered in this continuation.
Full authorization covers commits, pushes and synchronization to `main`.
There was no callable subagent interface in the continuation session; parallel
subprocesses/CI were not represented as agents.

## Frozen implementation and completed checks

- Production runtime: **`f1d5fcdb702806fb3a0c447b46d3120466e89e8c`**.
- Final native test correction checkpoint:
  **`9342f4f8086df7525fddec3c50a9c9e47aa25088`**.
- Final launcher: **1,675,061 bytes**, SHA-256
  `7946c9225cdf94b668f2709f91044c4f9f0e41492535cdac0f140be0b347c1cb`.
- Same-toolchain baseline launcher: 1,724,397 bytes; **49,336 bytes saved**.
- Absolute relocations: 39,394 -> 32,539. Total Zehn allocation:
  22,289,604 -> 22,274,096. Execution/BSS alone grows 11,912 bytes; both
  independent allocation guards still pass. Do not claim all RAM shrank.
- **158/158 native suites**, **399 converter tests**, **54 additional tests**,
  **4,799 linked-ARM memory vectors**, and ordinary ARM / WASM compile-only
  builds pass. Native suites use public fixtures, not original artwork.
- Packed source-key indices preserve all 6,855 entries; their 57,137-check
  production comparison is part of the native suite, not extra game content.

Details, exact journals and the private rebuild chain are in
[ui20-storage.md](ui20-storage.md). The source-plus-launcher archive is a
checkpoint, not a full game installer. No new whole-installation size or
ordinary-device frame rate has been measured. No Library write was verified.

## Concrete additions in this continuation

`packed_content_index.hpp` / `world_content.inc` remove absolute key pointers
and repeated first-dot domains at compile time, without runtime allocation,
resource reads or catalog scans. The release Zehn encoder chooses the smaller
of its existing and one additional fixed profile, independently verifying the
loaded bytes. Its isolated 2,592-byte gain is included in the total above.

`compact_retained_slices.py` is an implemented offline converter for obsolete
and duplicate retained slices within existing groups. It preserves codecs,
live pixels, group identity and 4 KiB phases, verifies every selected locale,
and declines a candidate unless the actual installation gets smaller. Its
nine synthetic test cases pass; real-game resource savings remain unmeasured.
The earlier pointer-only index candidate and failed test journals remain
historical, not accepted production artifacts.

The two final test changes correct initialization and indeterminate padding
comparisons; they do not alter game rules or relax render immutability. The
raw padding failure also reproduces against the previous runtime, with the
same cards/RNG and identical encoded outcomes.

## UI20 work carried forward, not newly completed a second time

The previous source implements bottom-right version text, status hover icons,
card-only preview anchoring, native-density registered enemy frames, Osty
terminal/dead-loop presentation, rarity-specific reward art, full Ancient
viewport, black standalone next-act maps, compact defeat layout, checkpoint
retirement, Continue information anchoring and the menu fade. The source and
historical private-art records are in [ui20-ui.md](ui20-ui.md) and
[ui20-animation.md](ui20-animation.md). The art-dependent repairs cannot be
activated by replacing only the launcher on an old resource installation.

## Remaining acceptance prerequisites

Recover the owner's complete repaired UI20 resource input (or the accepted
UI19 inputs plus pinned original PCK and official private Spine runtime), run
the documented repaired-art/locale/tile/sparse/retained/stream chain, and
measure the final installation. Verify every image through the production
reader and replay the ordinary Firebird feedback/performance scenarios on
that exact matched data and launcher before describing a full release.

The historical bilingual UI19 total was **74,538,517 bytes**; it is not a
new UI20 measurement and did not meet the 65–70 MB default target. Preserve
`state/` when trying a launcher-only checkpoint; do not apply full-package
instructions that delete `data/` unless a complete replacement is available.
Full static Ancient scenes, sampled LCD materials and platform capacities
remain in [ui20-retained-differences.md](ui20-retained-differences.md).

The prior long handoff is preserved verbatim as
[WORK_STATE-ui19.md](WORK_STATE-ui19.md), including its older source identities,
private paths and measurements. Do not treat those paths as currently mounted.

# UI24 Necrobinder/Osty feedback repair — checkpoint, validation pending

The new report exposes two paths not established by the previous merchant death
screens. Pinned assembly inspection confirms that owner death kills a surviving
Osty only after death prevention has failed, and the game-over screen retains
combat creature visuals above its backstop. The port previously left a living
pet after direct owner HP loss/kill and omitted Osty from the terminal renderer.
This checkpoint restores the companion kill and shares the ordinary Osty draw
path with combat game over, retaining the existing death/dead-loop clock.

The official full actor conversion also baked the Necrobinder slash placeholder
atlas because the actor sampler ignored the authored slash slot ShaderMaterial.
That source-data fix is in progress and will require regenerated actor resources;
replacing the launcher alone cannot fix that texture. No actor dimensions, frame
counts, cache capacity or allocation limit are changed. Public regressions and
ordinary64MiB firmware action captures are still pending for this checkpoint.

The slash repair now evaluates the original slot shader at each interpolated
Spine UV after the source clipping geometry. It reads original PCK uniforms and
textures and follows the pinned attack event's400ms Quad/In step and450ms
Quad/InOut opacity tweens. The shader's unbound mask sampler is white; its output
replaces the printed atlas placeholder, rather than multiplying that placeholder
into the effect. Existing frame counts, timestamps and source rasters remain.
Four source-free tests pass for event endpoints, channel semantics, fragment UVs
and slot-only replacement. Real-art and firmware acceptance remain pending.

# UI24 Necrobinder/Osty actual-action acceptance

Normal full-resource Strike now shows the authored blue slash instead of the
printed placeholder, confirmed in 33 ms ordinary Firebird captures. Normal
Offering kills a living Osty after owner death and retains both death/corpse
tracks above the game-over backstop. Full and reduced guest sequences include
first/intermediate/terminal poses and corpses through10.1s. Fairy prevention,
an already dead Osty, and a non-combat merchant death were also exercised.
Launcher plus all11 resources matched ordinary USB readback in both editions.

The tested release launcher is actualb690/public0e7 with22,286,408B allocation;
its resources are explicit private attack-only test candidates. Final converter
resources and the complete70,000,000B package gate remain separate. Native
world/core/player-death/reduced-clock checks pass after the synthetic test-only
fixture correctiona5a/public52daf. Details and preserved failure attempts are in
[the animation acceptance record](ui24-necrobinder-osty.md).
## UI24 explicit long ASCII seed import — source checkpoint

Normal text retains the original canonicalization and hash, including strings beginning with `raw:` or `SEED64-`. A separate Import seed code mode in Custom Run accepts the same 64-bit seed identity encoded by the converter. World and history records reuse alignment bytes and existing serialized flag bits; their allocation and encoded length do not increase. The entire long ASCII input is streamed through original uppercase/O-I/trim plus xxHash64 on the PC. No source text is truncated, and no code is sent through legacy `old` hashing. New original-DLL vectors include import-looking literals and long-input standard maps. Initial UI24 original comparison passed 386,703 checks / 918 inputs / 13,770 named stream cases / 7,296 standard maps / 912 saves; final updated import/UI/codec validation remains pending. No whole-run/Timeline/Windows-math parity is claimed.

## UI24 baked Neow field — source checkpoint

The optional NWF3 conversion replaces six Neow-only procedural material inputs with the exact existing held-clock0 light field and five empty slots, while retaining every image ID, all 546 transmission frames and all body frames. Shared Tez textures stay intact. NWF1/2 resources retain the current runtime cold build fallback. The new offline integer sampler has an optional direct production-Shader C++ oracle; field equivalence, actual retained-payload reclamation and formal guest cold/warm acceptance are pending at this checkpoint. No installation-size or cold-speed claim is made before those checks.
## UI24 Timeline source foundation — integration pending

The 57 pinned Epoch models now have public numeric metadata, exact EpochState
values, expansion edges, 18 score thresholds and card/relic/potion/event
entitlements. The compact state machine separates obtain dates, reveal state
and legacy content access; score grants are limited to one per run with the
source overflow clamp. A frozen two-word reveal mask supplies pool policy.
The appended localization contract reserves 257 source strings at8704..8960
for all16 locales. This checkpoint does not yet connect normal UI, profile
codec or run creation; those are the next implementation stage. No completion
or calculator allocation/performance claim is made for this foundation.

## UI24 Tezcatara material sampling — source checkpoint

The first formal full-resource guest check found Tezcatara still at 6.372 FPS / 156.408 ms mean render, with all 192 measured frames over 100 ms and zero I/O. Neow's improvements must not be generalized to this different procedural-fire route. This stage shares a 33×33 UV material grid per three source shaders, recomputed at the live shader clock, across all 237 source fire nodes. It interpolates only flame inner color/alpha, keeps all 506 geometry frames and particles, and borrows 13,068 bytes from the existing lexically scoped foreground scratch. Map/renderer lease and nesting regression cases are included. Appearance and formal guest improvement are pending; no frame-equivalence claim applies to this approximation.
