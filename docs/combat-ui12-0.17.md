# UI12 combat, cards and rewards

Native correctness and the scoped H/K Firebird sessions below have executed.
The final integrated launcher and matched merchant performance are recorded in
the release acceptance report; historical sessions are not substituted for them.
Reference: owner-provided v0.111.0 / 41cef1ea. Private scenes and assembly
were read locally and are not included in this repository.

## User feedback implemented

- Card cost / Star text has a thin four-sided colored stroke. The title uses
  the source label role and sits higher inside its original banner; compact
  card type labels use the locale-substituted role and 8/10px readable bands.
- Only the actual selected/aimed hand card receives enemy-specific description
  values. Unselected cards and inspectors cannot inherit a remembered target.
- The start-of-combat scene runs Battle Start, followed by Player Turn / 1.
  Source 300ms lead, 750ms scale, 1300ms fade-in, 1000ms fade-out and subsequent
  1500+1000ms backstop tail are retained. Resuming a saved fight suppresses
  the opening sequence once, without suppressing later floors.
- Creature anchors, bars, status and hitboxes move up 14 LCD pixels together.
  Energy and Stars occupy a lower-left rail; End Turn moves up 22px and the
  exhaust button is separated above it. Enlarged piles use 24px art, native
  20px red count badges and 10px count text. Card-flight targets follow them.
- The hand keeps its source fan and neighbour push, compressed horizontally
  to clear fixed controls; enlarged outer focus cards slide inward. Indexes
  sit above the face. The ripple highlight keeps its original distance field
  and clock at half projected width. Empty hands have no placeholder label.
- Failed keyboard or pointer card attempts create the original localized
  thought message. Its smoke/tail composition is a bounded CPU adaptation of
  NThoughtBubbleVfx, with the original smoke sheet; not a generic notice bar.
- Reward text is left aligned at 10px. A selected overflowing row pans its
  full text inside the button. The inset reproduces RewardContainerMask:
  existing reward-panel art, flipped both ways and modulated to 0.310617.
- Reward opening now waits only 8/16 presentation ticks (264/528ms) after the real effect endpoint,
  independent of death-track length. This is the user's requested calculator
  responsiveness adaptation; the original Standard wait is death-track +1s.
  Corpse animations continue in the retained combat room behind Loot.
- Real committed Smith/deck/world upgrades create the source 250ms grow,
  start deck-bound flight at 1750ms, and emit golden sparks. Up to four
  simultaneous upgrades use copied committed card facts. Their whole faces
  are transformed from the existing hand cache, so minimum-size text does not
  spill outside a shrinking frame and no additional bitmap pool is allocated.
- Merchant normal 65x91 cards now enter the existing exact small-card cache.
  The prior size whitelist unintentionally bypassed it for every stock card.

The shared enlarged-button HSV omission is owned by the frontend stage; its
fix restores Proceed's original face and leaves the yellow outline separate.
The calculator's numeric default bindings are owned by the root integration;
the existing Card1..Card10 semantic path already selects its actual slot.

## Current evidence and limits

The runtime stage is `31b7a74f09ee65675f7021454e1676872f719eae`;
subsequent commits through `96cca701b73159e2b71f25bd072c676cd324c198` change
only correctness fixtures. All cited executed journals have
`source_changed=false`. They are local frozen checkpoints, not remote push
receipts. Clang 18 built the entire core and four focused test targets from
source (`f775b442426f`); final fixture rebuilds are `8b72c651104a` and
`e059952b4a8d`.

| Executed gate | Checks | Receipt |
| --- | ---: | --- |
| Source command/selection continuations | 440 | `7db4b82e7cd8` |
| Player/enemy/Battle Start source timing and clock wrap | 43 | `7db4b82e7cd8` |
| Card motions, highlights and five-orb observer invariance | 325,102 | `7db4b82e7cd8` |
| UI12 input, ten card slots, targeting, speech, Smith and reward barriers | 271 | `7db4b82e7cd8` |
| UI12 with actual UI39 common/English resources | 389 | `7ec6bb9e95b3` |
| Secondary effects with actual UI39 common/English resources | 325,127 | `17f1e8f6de0d` |

The actual-pack UI12 gate checks that a Strike description changes 6→9 only
while aiming at a Vulnerable enemy, cancels back to 6, and leaves its neighbour
unchanged. It exercises a real Thunderclap victory over Nibbits: the observed
and direct commands produce identical canonical saves; the reward barrier
expires while corpse presentation is still retained. Smith compares the real
committed upgrade with a direct core command and checks display-only clocks
without altering State/RNG. The merchant 65×91 face enters the existing cache
and its cached pixels exactly match the uncached result.

Initial failures remain recorded. Bare `clang` was absent from PATH. GCC 13
then hit an internal compiler error in unchanged `world_relics.cpp:323`, so
the build uses the installed Clang 18 path. One legacy observer comparison
mistook `Effect`'s byte-1 ABI padding for gameplay state under `-O0`; its final
gate compares every other State byte and full canonical saves whenever the
synthetic fixture is serializable. Real resource fixtures now complete the
production pose preparation phase before pressing a card. A fixture briefly
used an absent model name; `626b4ed4cdcc` failed to compile, and the subsequent
stale-binary runs `e950d41e153c` / `62dd1c4e71b4` are explicitly excluded from
acceptance, as recorded by `9aa7a954f6ed`. The rebuilt gates above supersede
those attempts.

These are correctness results, not calculator frame-time or UI40 visual
acceptance. UI40's appended pile-count and thought-smoke images, refreshed
fonts, final storage/allocation budget and ordinary Firebird interaction are
integration gates. No WASM/browser validation is used.

## Firebird H follow-up: selected-card clipping

Ordinary H calculator entry reached English title, standard Ironclad, Neow,
the map and the first fight. The physical `5` key selected Bash in slot five
without spending energy. This exposed one remaining layout defect: the
selected/aimed pose still used the old source projection at y=210 and .75
scale, which cut off the enlarged face's bottom description. The selected
pose now uses the same 1:1 readable face and y=174 safe area as hover. Its
initial centered sprite is bounded before interpolation, so aiming cannot
briefly reintroduce the cut-off. Free pointer dragging retains its own pose.
Only presentation changes; card/target commands and RNG remain untouched.
The older standalone hand-motion/layout oracles still contained UI9's 69px
face, y=200 focus and prior pile anchors. Their independent source transforms,
motion curves and State-purity checks now include the explicit UI12 LCD
adaptations: 86px faces, 80% horizontal span, hand/scene lifts and new pile
centres. This updates expected presentation coordinates without relaxing
canonical gameplay checks.

The in-progress H emulator journal includes a failed screenshot write when
the host scratch disk filled. The running game stayed intact at the paused
controller boundary; after temporary build cleanup the same screen saved
successfully. This environment failure is retained and is not a game crash
or a passing journal. Final frozen-launcher verification remains required.

## Recovered ordinary-device evidence

H reached the ordinary first battle through New Run, Neow and the map, used
the real numeric selection/targeting path, showed insufficient-energy speech,
won after four turns, collected gold and Rage, returned to the map, and saved.
The production decoder accepted its exact-roundtrip save: floor 2, turn 4,
76/80 HP, 268 gold, 11 deck cards, revision 28. These successful interactions
and the save are scoped evidence; the host ENOSPC controller error still means
the whole H journal is not a pass.

K's closed journal `2d89c8a26a3f` passed with `source_changed=false`. It
continued H's saved run, checked the complete enlarged aimed card, the source
Battle Start / Player Turn sequence and actual left-aligned world-choice Loot.
Its later combat and merchant rooms were created through the real console;
they are fixtures, not claims of ordinary map reachability. The matched Shop
save/progress files were produced by that session. Two subsequent historical
merchant comparison journals timed out before their final receipt/USB return;
those attempts establish no measured speedup.

## Final cache and text corrections

Recovery of the actual seven-slot shop found an additional cache admission
defect: the rightmost 65×91 face was inside the LCD, but its padded 84×108
cache window ended at x=325. `f807731` keeps that fixed window on the surface
and includes its face-relative offset in the cache identity. All eight cache
buffers are reused; the change adds no resource image or larger bitmap pool.
An identical card on a plain background can now move between central and edge
slots without replaying pixels at the wrong inset.

The focused actual-UI40 gate executes every merchant slot with flat and
patterned backdrops, repeats warmed draws, and compares the complete 320×240
result with uncached production rendering. This catches both the previously
uncached edge and invalid reuse after the window changes its inset. The
pre-text-correction stage passed 1,072 checks. Its full merchant occlusion
gate also passed 41,569 checks across 2,376 exact frames, all five characters,
opening/closing/interrupted animations and clock boundaries.

`1deaf3b` / `ae13595` complete the remaining specialized font paths: banner
fill and stroke, health/block/energy labels, merchant prices, floating damage,
intent widths and card-cache cost bounds now retain 1/64-pixel advances,
source pair kerning and glyph bearings until raster placement. Combining marks
cannot reduce the damage mask's preceding ink extent. These changes preserve
the existing text sizes, scene anchors, animation clocks and gameplay state.
The actual resource test compares banner and health-label glyph placement
against ordinary Canvas text, including a kerning-sensitive Latin sample.

The complete native library and test executables were rebuilt from the frozen
`ae13595` combat worktree. Its fixes were separately integrated and pushed by
the root; these receipts describe the named worktree, not unrelated later
frontend changes. All four final journals below passed with unchanged source
fingerprints. They establish correctness and exact cache pixels, not calculator
frame rates.

| Final executed gate | Result | Journal |
| --- | --- | --- |
| Complete matching library / executable rebuild | Passed | `b6016237f5c5` |
| Actual UI40 card/target/Smith/reward/merchant/font boundaries | 1,096 checks | `34d842ab8fde` |
| Five complete combat and canvas suites | 5/5 passed | `c4ac9d4cd5d0` |
| Actual UI40 merchant occlusion | 41,569 checks; 2,376 exact frames | `a2ea930acbc8` |

The five suites cover card-selection continuations (440 checks), banner clocks
(43), secondary card/orb observer invariance (325,102), the resource-independent
UI12 contract (892), and 190,626 bounded Canvas cases (384,270 checks). The
actual merchant comparison retains identical controls, presentation clocks,
canonical saves and all five character cycles while verifying that fully
hidden character frames avoid decoding.
