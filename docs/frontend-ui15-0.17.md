# Frontend UI15 implementation and verification ledger

Reference: the owner's v0.111.0 / 41cef1ea original scenes and assembly.
This ledger records source work. An unchecked device or resource gate is not
implied to pass because its implementation or test exists.

## Frontend behavior

| Feedback | Implementation | Verification |
| --- | --- | --- |
| System language by default | Shared ISO language/script/region parser and installed-pack selector: saved selection, system language, English, first installed pack. Desktop uses LC_ALL, LC_MESSAGES, LANG precedence. | Ndless uses the SDK getLangInfo() binding when exported, then the ordinary OS current settings ZIP. Stage4 on CX OS3.1/Ndless2022 has neither newer binding; the real zh_CN settings selected Chinese with no saved language override. Stage5 locale selection/parser gate passes906 checks. |
| Space speeds credits | Holding physical Space multiplies the authored80px/s target by5; releasing, cancelling physical input or closing resets it. The original easing and end behavior remain. | Actual English/Chinese held-Space, release, cancellation and original close timing pass in the integrated frontend gates. |
| Bestiary right column scrolls vertically | Source one-line monster entries retain14px rows and18px region headings. Overflow travels horizontally in the fixed label viewport. | Stage5 actual English/Chinese resource gates pass; fixed14px rows and horizontal overflow are captured. |
| Continue hover is oversized | Shared120px width,66px/79px height; original date, act-floor separator, ascension and health/gold row hierarchy. | Actual English/Chinese long-date Continue captures pass the independent source-layout comparison. |
| Confirm and Back are inconsistent | Shared confirm uses the same40×24 hit size and47×32 source body as Back, including character selection. | Actual original-atlas tick placement and paired control geometry pass; ordinary-OS character-selection captures retain both controls. |
| Short-range scrollbar grows excessively | Grabber artwork is bounded to a square independent of its logical track thumb; character and defeat use fixed10px/12px handles. | Native short-overflow bounds and focus/return cases pass; ordinary-OS checks remain scoped to the named flows. |
| Deck upgrades location | Whole checkbox/label group is centered in its own206–221px footer hit row. | Actual English/Chinese footer and paired-control captures pass. |
| Deck sort fonts and bars | Original left aligned Kreon Bold/substituted Regular labels at readable6px; original250×42 button proportion,50px gaps, separate60px outer strip, image-only material tint. Sparse6px glyphs are explicitly requested. | All16 complete language packs rebuilt; actual English/Chinese font, layout and footer gates pass. |
| Selection eye overlaps cards | Visible eye and hit target move to x5 together. | Actual English/Chinese selection preview captures pass. |
| Long description duplicates tooltip | Card description stays in its own smooth scrolling viewport; only mechanics, relic, power and preview tooltips remain. | Actual long-card pixel and end-window checks pass; the visible tip set equals its mechanic tips and contains no duplicate Description tip. |

The original references include `deck_view_screen.tscn`,
`deck_view_sort_button.tscn`, `bestiary_entry.tscn`, `NContinueRunInfo` and
`NCreditsScreen`. Space acceleration is the owner's requested addition.

## Integrated input and ordinary-OS acceptance

At the integrated source `bd158d205d877f767f59fe2a1eb33874cb925899`, the
32-entry calculator FIFO retains the complete keyboard and touch sample,
including contact origin, movement, physical press/release and original input
time. The1124-byte record is shared by resource reads, CPU decoding and coarse
rendering checkpoints. These checkpoints only collect samples. The ordinary
input loop dispatches them after rendering has released its resources; they
do not advance the game clock or consume run RNG.

An early cooperative version captured keys and pad directions but not the
initial pointer contact. Actual merchant input exposed that omission: the
physical press arrived at the unchanged160,120 cursor. The complete FIFO
fix retains the contact and movement before the click. A second review found
that raw touch jitter could keep the queue nonempty and suppress long-key
repeat. Historical samples now dispatch without repetition, followed by at
most one repeat per frame for the last dispatched held state, provided no
new release is pending. A deliberate old-gate negative control fails; the
real-App positive suite passes267,700 checks. It covers CPU-only decoding,
render callbacks, nested capture, one-shot clicks, drag ownership, cancellation,
queue overflow, retained card selection and120 frames of held-key jitter.

The same source passes native and ASan/UBSan changed-path gates. Seven ordinary
CX OS/Ndless2022 journals pass: system-Chinese default,50ms Continue with its
actual Ironclad wipe,50ms digit/right/single Enter, real Slime damage26→20 and
energy3→2, SaveQuit, normal OS exit, a fresh process and another Continue.
The two saved runs and atomic backup are byte-identical7430-byte files, with
SHA256 `041d9a2c072391cd5c4530a45b6ce37728d41d50d0e104e5f846699db3192179`.
Production loading validates revision16 and enemy HP13/20/9. The merchant's
original100ms contact/100ms movement/180ms press also opens its rug correctly.
These are input correctness results; merchant first-open performance has its
separate remaining gate.

The retained input sample interval is8ms, a minimum interval between complete
hardware polls rather than a hard maximum execution gap. A proposed16ms idle
pad interval was not implemented: the measured complete update phase already
bounds one full read to a small fraction of a millisecond, so it cannot explain
the warm-render difference and does not justify changing the accepted input
ordering. Foreground sampling reads the existing SP804 counter; input and game-animation
clocks retain separate ownership.

## Vantom continuous bone transition

The original Vantom implementation drives `Visuals/ScalingBone`, which drives
`root_scale_adjust`, over 750 ms using SineInOut. Its separately controlled
`tail_target_bone` retains its independent spatial location. A replacement
tween first completes the previous tween before starting. The runtime keeps
that bone clock separate from the creature's formation, health bar and intent
layout; it does not scale the complete actor bitmap.

The converter retains complete original compound animations and each frame's
actual draw order. The original recipes include InkBlot's 150 ms tail delay,
body charge changing into idle while the tail extension continues, the other
moves' before-body tail changes, and the opening `charge_up_1` sequence.
Matching direct cues acknowledge an already running compound animation instead
of restarting its body. The death recipe also includes `NVantomVfx`'s callback:
one second after `dissolve_tail`, track 1 changes to `charge_up_1` and then
`charged_1`, while the body continues dying. There is no claim that arbitrary
body and tail clocks can be spliced without cross-track dependencies.

Each retained source pose is independently sampled through the reviewed Spine
runtime at multiple values of the driven bone scale. Meshes proven to follow
root scaling, one common tail translation, or no scale transform use compact
raster layers. Actual attack/hurt head physics and weighted whip geometry do
not all satisfy those three rigid cases. Those meshes retain their original
triangle/UV topology and source vertex positions. Adaptive scale intervals are
validated against independently sampled intermediate poses; the error limit
is 0.20 LCD pixels at the maximum normal viewport projection, including Q6
vertex quantization and leaving room for runtime rounding. Source
branches refine to adjacent Q10 scale values when necessary, preserving both
states instead of smoothing across a discontinuity. Per-mesh knot reduction
retains only the validated intervals that mesh needs. Four original emitter
transforms are validated on the same source scale grid.

Non-rigid textures are sampled at twice their maximum projected source density,
up to the original atlas density, and shared across repeated contexts. The
calculator uses bounded fixed-point triangle interpolation with bilinear
texture sampling. Its limits are64 ordered layers per frame,114 vertices and
112 triangles per mesh, and65,535 words per geometry chunk. A traversal of
all1,616 retained mesh records and3,232 references proves those per-mesh
bounds; a complete frame can still contain many more vertices and triangles. It copies only
small vertex/index arrays before releasing resource pointers. No actor-sized
runtime bitmap or resident animation pixel pool is allocated. The original
animation sample count and clock are not reduced to fund this path.

The existing 232 endpoint clip identities address 116 private parameter rows
at auxiliary 5288–5403. Layer images and per-frame geometry chunks first reuse
the old 4080 frame slots, after proving those slots are not referenced by any
other clip, card portrait or auxiliary image. Geometry chunks collect multiple
meshes, avoiding an image-directory entry per mesh. Excess unique images are
appended after the parameter rows. Unused old slots become one shared empty
pixel; the master converter reclaims their unreferenced original tile streams.
Parameter and geometry words use the existing lossless image codec and are
never painted. Dense mesh textures and raw word payloads preserve their exact
stored bytes through the master replacement operation.

The first combined-pack attempt exposed 23 mesh textures whose logical UV
dimensions exceeded the native 320-by-240 image contract. Their texels are now
stored linearly in pages of at most 256 by 240 pixels. Twenty-one long, narrow
textures need only a different storage shape; two textures need a continuation
page. Version 3 parameter rows add an explicit continuation image ID. The mesh
still samples its original logical UV coordinates, including bilinear samples
across the page boundary. RGB565 and alpha8 bytes are reconstructed and compared
exactly against the sampled input. No source texel is discarded, no geometry
changes, and the Content buffer and native image limits are unchanged.

The three original shader slots (`tail`, `tail_hook`, `megablade`) retain
separate UV noise masks. Original Godot FastNoiseLite supplies the 512-square
noise field; masks store the sampled noise in alpha8 so RGB565 does not reduce
threshold precision. `src/vantom_vfx_ui15.inc` implements the four original
particle emitters and source events, and `src/vantom_ui15.inc` inserts them at
the current frame's original slot depths. These effects now have concrete
runtime and resource implementations; actual full-pack and device acceptance
are separate gates.

## Historical conversion and early verification

- Twelve synthetic converter and texture-page tests pass. They reject a warped intermediate scale
  from the rigid path, preserve invisible particle boundaries and dynamic draw
  order, retain nonlinear meshes with independently validated midpoints, retain
  adjacent Q10 branch states, preserve UV/triangle/position words, verify the
  private word codec, reject aliased frame ownership, and reuse old directory
  slots before adding images. The page checks also preserve exact RGB565 and
  alpha8 values across a continuation boundary, retain geometry/emitter words,
  and reject a mask whose second page would otherwise be lost.
- Full `src/app.cpp`, including the Vantom mesh and effect members, passes a
  Clang 18 syntax check. This is not calculator execution or a performance pass.
- The production calculator language helper and entry-point binding pass906
  host checks and the complete ARM build. These cover the official `calc_cmd`,
  `TI_MS_MathExprToStr` and OS allocation cleanup behavior, saved language
  precedence and unavailable-language fallbacks. Ordinary CX OS3.1 startup
  selects Chinese from its real settings with no saved override. A separate
  official-r2022 cold boot confirms both optional math bindings are absent
  there, so the settings XML fallback is the path actually exercised.
- The earlier three-frame idle probe was insufficient to establish a complete
  decomposition. Full sampling rejected the original rigid-only and arbitrary
  two-clock assumptions. Those failed runs remain in the private journals;
  their assertions were not reclassified as passes.
- Actual source reconstruction of attack frame 8 at scales 1, 0.75 and 0.5 has
  been rendered and inspected beside the original sampled output. It retains
  the independent blade/whip shape while the body shrinks. This host comparison
  does not establish final RGB565 Canvas output or Firebird frame rate.
- The complete production source conversion passed from immutable local
  checkpoint `24911c1533dc5088b2729a96c9f2e6d1a218a73e`. Its journal,
  `ui15-local-vantom-lcd-v6-5ca8bbc6880f.json`, records exit 0 and unchanged
  source over 347.260 seconds. All 116 compound clips, 2040 original full-scale
  frames and 232 endpoint directory identities were retained. The source proof
  covered 3,750,351 original vertices and used at most 41 independently sampled
  bone scales for a clip. Maximum verified mesh interpolation error is
  0.19998293 LCD pixels; the four emitter transforms remain within
  0.19566429 LCD pixels. Both bounds use the larger bestiary projection, 4/21;
  the battle projection for Vantom is smaller. These are source-conversion
  results, not a calculator frame-rate measurement.
- The source catalog's native dimension failure was fixed in immutable local
  checkpoint `5f8d3608d87ee44da9a74559e49c9791eea3bee3`; journal
  `ui15-local-vantom-pages7-full-996b316a450b.json` records a passed conversion
  with unchanged source. All 23 affected textures reconstruct their original
  native pixels exactly, and all 110 geometry word payloads are byte-identical.
  The final catalog has 7669 unique image/data payloads and reuses all 4080
  old frame slots. Its 3589 excess payloads and 116 parameter rows add 3705
  image-directory entries. Parameter payloads occupy 564,090 bytes and the
  layer, mesh and mask payloads occupy 9,453,443 bytes, before the master pack's
  reclamation of the old flattened tile streams. The maximum actual frame has
  34 layers and a mesh uses at most 20 scale knots. There are 388 unique dense
  mesh textures and 110 shared geometry word chunks.
- A separate final-catalog traversal passed for every retained frame. All
  3232 non-rigid layer references resolve to dense textures and exact geometry
  words, and all 4908 dissolve-mask references have matching dimensions. It
  verifies the emitted resource formats rather than only the source sampler.
  The bounded final catalog SHA-256 is
  `3376958fb0c592d2c2d0cdbe526b08007c6a257b8e37895120e9276541a63848`.
- The complete combined ENG pack also passed a real C++ Canvas visual gate
  using the stage 7 runtime. It rendered attack 1383/frame 8, debuff 1461/frame
  8 and hurt 1471/frame 6 at the bestiary projection and bone scale 0.75. The
  debuff frame exercises 595 source triangles across 16 non-rigid meshes; the
  hurt frame exercises a two-page head texture. Original Spine poses were
  sampled at the same clocks and projection for side-by-side inspection.
  The renderer retains the expected body/tail placement and the paged head,
  without a page seam. RGB565 color precision and the existing rigid-raster
  filtering still differ from the source float renderer; this is not a
  byte-identical image claim. The actual gate journal is
  `ui15-local-vantom-render9-actual-fbb88aec163d.json` and the game source is
  checkpoint `02b8c1cca41c64800de3a75fb08ddd86636f84a2`.
- The actual-art combat regression passed 71,016 checks against the complete
  stage 7 ENG pack. It traverses the real renderer for all 2040 full-scale
  source frames, including all 3232 non-rigid layer references, in addition
  to real Beetle Juice use, the 750 ms Vantom bone tween, the delayed tail cue,
  unchanged formation/health/intent placement, same-run presentation reset,
  and canonical save invariants. Journal
  `ui15-local-vantom-combat7-actual-ef0ea4d1a925.json` records exit 0 and
  unchanged source; the host run took 2.281 seconds. This is a functionality
  gate, not a calculator frame-rate measurement.
- The subsequent integration9 and named resource-delta gates below complete
  compressed-resource validation. Calculator timing remains a separate
  measurement. Full storage reporting includes reclaimed old streams and
  every new layer, mesh, mask and parameter payload; the source catalog's
  gross payload count alone is not the net package-size result.

## Unified integration9 closure

Local checkpoint `a04451c07e1d63fa50cf95466381fce2a0d96ff8` passes all140
native suites and354 Python tests; the portable target compiles. Final
compressed-resource loading passes all16 locales and53,072 descriptions with
zero unexpanded templates. Actual English/Chinese bestiary gates pass
3,095,170/3,094,289 checks; card/HUD gates pass713,342/790,968. Profiles,
Continue, credits and history also pass their real-font/resource cases.
Credits retain the original non-closing end behavior and add held-Space5×
acceleration. Their230,446 checks per actual edition include release and
close timing. These source/resource gates do not imply physical-device FPS.

At integration9 the full-resource Vantom suite passes774,616 checks and its
complete source-frame combat traversal passes77,480 checks. Dense texture
paging, source bone interpolation, source death callbacks and same-run
restoration use that stage's compressed pack. Integration16's Vantom6,596
actual-resource checks preserve the semantic and pixel cases while omitting
PPM file-byte write assertions; the differing counts are not a coverage
percentage. The foreground scratch allocator and bounded mesh arrays reduce
the largest ARM function frame to5,952bytes without removing source layers,
frames or card/effect capacity.

The ordinary-OS legal new-run loop now completes Neow, its first victory,
rewards, Proceed, SaveQuit, visible Continue and a second save. The independent
bd158 short-input/pointer gates and same7430-byte save/backup chain are
complete as detailed above. Final merchant first-open measurements and the verified
shipping package retain their own bd158/strings15 acceptance identity in
[WORK_STATE](WORK_STATE.md) and the [validation manifest](validation-ui15-0.17.json).
Local source checkpoints are not represented as remote CI.
