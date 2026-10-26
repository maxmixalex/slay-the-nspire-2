# UI14 room corrections

Reference: the user's local v0.111.0 / `41cef1ea` assembly and scenes. Original
source, text and art remain private. This document records behavior and the
port's implementation decisions; validation results are added after the
checkpointed build.

## Source behavior

`NRewardsScreen.RewardCollectedFrom` calls `UpdateScreenState`. An empty
terminal rewards list fades its Rewards node over 0.25 seconds, hides the
backstop, enables/pulses Proceed, and emits Completed. It does **not** open
the map. `OnProceedButtonPressed` calls the run manager's terminal-rewards
continuation. An empty nonterminal rewards overlay instead removes itself
and resumes its parent. UI14 keeps those separate flows. The grouped
terminal set's transition into the empty room reward phase now preserves
the same 250 ms fade rather than resetting its opacity immediately.

`NDeckTransformSelectScreen.OnCardClicked` opens its transformation preview
at MaxSelect. Its grid confirmation is enabled only when MinSelect differs
from MaxSelect and the minimum is satisfied. Preview confirmation is a
different control. UI14 removes the unneeded disabled grid control for
fixed-count selectors and retains the preview and its confirmation.

The rest button scene uses KeepAspectCentered for both icon and outline.
The 256 by 169 icon fits into a 250 by 153 control. The separate outline
fits a 248 by 173 control and has its own 0.9-to-1 focus scale. The old LCD
code stretched a padded 80 by 57 icon to 47 by 29, making its visible width
greater than the outline. Its corrected packed-texture projection is
44 by 31 in the same 47-pixel holder; the focus outline keeps its source
projection and independent tween.

The merchant dialogue scene centers its 280 by 130 text control below the
texture center. The bubble's 420 by 285 texture includes the tail above its
body. UI14 preserves that aspect ratio at 128 by 87 and projects the text
control through the same opening scale, including its approximately
11-pixel downward center offset at full size.

## Runtime changes

- Event buttons with no secondary description use only the label's required
  height. Wrapped labels retain sufficient room; description buttons remain
  at the existing readable height. Body scrolling and hit regions use the
  resulting variable-height stack.
- Event card previews follow the hovered option vertically. Associated
  keywords stay with the card and remain inside the screen, moving above
  the card for low options so that reserving their text does not push the
  actual preview away from the option. This corrects
  Wood Carvings' Torus preview being pinned near the top of the display.
- Event prose, Architect dialogue and reward scrollbars use the shared
  hover enlargement. Room compact paragraphs retain their full text and
  scroll by pixels when their fixed viewport overflows.
- The loot title sits four pixels lower on its ribbon. Collected terminal
  rewards wait for the player's Proceed action before opening the map.
- History room tooltips use a 174-pixel column instead of a 272-pixel panel.
  The existing full-entry scrolling remains available.
- Merchant inventory opens immediately with its original rug tween.
  Incremental image/glyph preparation no longer inserts a three-dot loading
  screen, freezes the room, or restarts the opening tween. Cancel remains
  available while preparation completes. Removing the preparation backdrop
  also removes 139,520 bytes of static RGB565 storage.
- Merchant cards fit their 60-pixel columns at rest, grow to 74 pixels on
  focus, and preserve the focused-card draw order. The lower card row moves
  four pixels down; prices and hit regions follow the same layout.

## Closed-merchant frame representation

The ten regular merchant clips at directory indexes 1695 through 1704 keep
their complete 1,817-frame source catalog, dimensions, pivots, frame order
and clocks. The UI13 pack stored these as full tiled frames. In particular,
the 145-frame bottom layer repeatedly included identical room pixels, and
new frames could require several compressed tile groups before being drawn.

`merchant_ui14_assets.py` decodes the owner's existing logical pack and
compares every frame of each clip. A pixel becomes static only if its
RGB565 color and alpha are identical in all frames and its alpha is nonzero.
All other pixels stay in that frame's moving plane. The static layer and
every moving layer have disjoint coverage, including partially transparent
edges, so drawing them consecutively performs exactly one source-over
operation at every visible pixel. The renderer uses the original clip pivot
for both. No new framebuffer, interpolation, frame skipping or resampling is
involved.

The ten static layers occupy auxiliary IDs 5265 through 5274. Moving layers
replace their existing image IDs. Every
output plane is independently decoded again and recomposed against the
original RGB565 and alpha planes; the private manifest contains per-clip
pixel identities and encoded/decoded storage counts. The release converter
can then reclaim tile pixels no longer referenced by any live image.

The independently encoded intermediate moving planes increased final common
storage, despite reducing decoded cache pressure. UI14 therefore packs the
moving planes into existing RGR1 retained groups, each containing at most
eight consecutive frames of one clip and no more than 64 KiB of decoded
bytes. One cold frame requests only its small neighboring group; decoded
individual frames still enter the ordinary image cache. This uses the existing
codec and cache without adding runtime memory. Groups are used only when
their complete payload plus descriptors is smaller. The relocation pass
physically drops the replaced independent blocks.

An all-frame private measurement at stage4 compares the full 1,817 moving
frames: independent encodings occupy 8,247,744 bytes, versus 2,033,340 for
eight-frame/64-KiB RGR1 groups. The largest resulting group is 64,239 decoded
bytes. Four-frame groups occupy 2,436,922; expanding to sixteen-frame/128-KiB
groups reaches 1,665,386 but increases cold decode work, so UI14 keeps the
smaller bound. RGB-decorrelated RGR2 mode2 saves only another 31,490 bytes
and adds a restoration pass; the merchant path retains plain RGR1. These
measurements are compression and byte-exact readback evidence, not device
frame-rate measurements.

The public small-pixel tests cover opaque black invariants, partial alpha,
changing alpha, disjoint composition, independent encoded readback and
rejection of altered frame dimensions. Retained-group tests additionally
cover frame/byte bounds, clip boundaries, cheaper independent fallbacks,
physical removal of old blocks and preservation of foreign tile, glyph and
localized-string data. The actual private conversion and Firebird
cold/sustained measurements have separate receipts below.

At checkpoint `97a63d78979daba7d12fa8475b305e836fc9503c`, journal
`merchant-ui14-exact-split-3cfddc4f2303` closes PASS with unchanged source in
32.72 seconds. All 1,817 encoded moving frames and ten fixed planes recompose
exactly. Total decoded sparse bytes fall from 15,482,673 to 11,571,712,
including 44,267 fixed-plane bytes, a reduction of 3,910,961 bytes (25.26%).
Moving visible pixels fall from 5,440,099 to 3,148,513. These are complete
catalog totals across all five characters; one room displays only its selected
character. With 4 KiB allocation rounding, a complete closed-room cycle uses
1,468 to 1,946 image pages, or 1,759 to 2,237 with both hover skins retained,
before the small backdrop/light/UI set.

The stage6 final resource assembly at
`67e2822ef369dc62428dcc0698c0bda62586756f` closes PASS in
`ui14-final-resources-0c2a8ba4cd52`. Its merchant groups physically remove
6,215,294 bytes, including two-byte alignment, from the intermediate pack.
Final common artwork is 72,131,134 bytes, compared with the stage4
independent-frame result of 78,354,908. It remains 1,471,828 bytes larger than
the previous release's 70,659,306-byte common file, including the other UI14
art additions. The complete release must report that net increase accurately.

Journal `merchant-ui14-final-pixel-proof-2cd1e0f585cc` independently decodes
the final English package and the original UI13 source. All 1,817 frame
pairs recompose from the final ten static layers and final retained moving
planes with zero RGB565 or alpha differences; dimensions, pivots, order and
clocks are also identical. The final logical-pack SHA256 is
`e5523824d43abd94a4bdf25fc0f71502bfd32bd022382ff9060b60c79076e66a`.
Six public factoring/grouping tests pass at this same checkpoint. Ordinary-OS
Firebird cold and sustained timing is recorded independently below.

## Validation scope

The terminal reward regression collects the real first encounter's rewards
through the public UI, waits 60 frames on its empty list, and checks that
only Proceed opens the map. The merchant preparation regression checks
immediate opening, cancellation, bounded preparation steps and a stable
opening clock. Ordinary event geometry checks accept the variable-height
buttons while retaining viewport and non-overlap constraints.

The stage7 runtime `0418cf3e63dabb456e4a9aaa49062d32f7ac747c` passes nine
actual-resource gates: English/Chinese event, room, reward and associated
keyword-tooltip tests, plus the full merchant-occlusion comparison. The
private receipt is `rooms-ui14-stage7/receipt.json`; journals use the prefix
`rooms-stage7-`. Captures include label-only Continue, fixed-count Transform,
Torus with its lower preview and keywords above it, closed/open merchant,
dialogue bubble and rest outlines. These correctness checks do not establish
calculator performance.

## Ordinary-OS Firebird merchant measurements

The stage7 ARM is 1,681,232 bytes, SHA256
`88b9c0dcb75b843a94ee0b10ab57df380498d57a80683419f2b0dcc854e9dc35`.
It was read back byte-for-byte from the installed isolated calculator before
measurement. The final Chinese logical resource is 78,904,352 bytes, SHA256
`f33d8871063b18d9392e148fcdc88cdf3b78618a38daa9ab8effc80fe7ace18f`.

The controlled Ironclad merchant fixture uses seed 7443907, floor14, HP80,
gold99 and a matching production progress record marked partial. It is not
a natural run. Both branches start from one fresh application title snapshot,
before any merchant rendering. The cold branch performs Continue and observes
16 simulated seconds. The other branch performs Continue and observes120
seconds, then forks into matched A/B emulator states. A exits immediately;
B runs exactly60 additional seconds and follows the identical exit sequence.
Each branch uses ordinary Pause, Save and Return, confirmed Quit, and OS USB
readback of `state/nSpire2.perf.tns`.

| Room sample | Frames | Interval ticks | FPS | Mean render | Resource reads / bytes | Intervals over50 /100 /250ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| First16s after Continue | 407 | 496,078 | 26.8840 | 21.6001ms | 249 /1,639,165 | 19 /3 /2 |
| First120s cumulative | 3,528 | 3,903,913 | 29.6127 | 16.2341ms | 249 /1,639,165 | 19 /3 /2 |
| Matched additional60s after120s | 1,800 | 1,965,896 | 30.0028 | 15.5379ms | 0 /0 | 0 /0 /0 |

The clock is the emulated CX foreground SP804 timer at32,768Hz, hardware1.
Actual Room-classified duration differs from the controller's requested time
because Continue includes the original title/departure lifecycle. This is
Firebird emulated-device timing, not physical-handheld FPS. The A/B result
subtracts counts and totals; it never subtracts minima or maxima. All reports
have zero resource failures. The unchanged249 reads at16 and120 seconds show
that this complete closed-room cycle no longer repeatedly decodes old tiles.

The cold sample still contains two intervals over250ms, with a longest interval
of1,190.552ms. Its separately aggregated render and resource maxima are
1,125.763ms and894.684ms. They are not a correlated per-frame decomposition
and must not be subtracted to claim CPU time. A read-only native callback
attribution of the first Room frame identifies45 calls /410,493 bytes:
139,935 image-group bytes,171,101 glyph-group bytes,13,441 string-group bytes,
and86,016 descriptor/page bytes. It establishes cold resource ownership,
not device timing. No new loading animation, delayed measurement boundary,
frame deletion or speculative resource prefetch was introduced.

The emulator clone changes only the v3 snapshot's512-byte flash-path field.
All other decompressed snapshot bytes retain the same hash; the paired flash
is a distinct copy. Every branch's `started.flash` was verified before its
measurement. The independent natural-run flash was never opened. Accepted
journals are `merchant-stage7-cold16-40ab630383ca`,
`merchant-stage7-warm120-3c6e254c43f1`,
`merchant-stage7-warm-a-measure-7bd6dae4d0f7`, and
`merchant-stage7-warm-b-measure-e3e0d0365ed6`. All close PASS with unchanged
source; the private aggregate is `merchant-firebird-stage7/receipt.json`.

## Restored original death-to-reward timing

The UI14 historical-difference audit identified the old early-Loot behavior
as a real remaining timing difference. The current source restores
`NCombatUi.ShowRewards`: Normal waits the longest remaining finite death
track plus1 second; Fast waits500ms in ordinary rooms and half the remaining
track for a boss. Actual damage and generated-card command barriers still
finish first. Corpse hold/dissolve stays independent. A held `dead_loop` or
missing ordinary clip cannot extend the deadline, and repeated renders do
not reschedule it. The original four-branch rule was also independently
preserved in the historical `3d6d799` implementation and
[UI6 source timing record](combat-ui6-0.17.md).

The added tests cover all four mode/room combinations, delayed real-command
barriers, fixed deadlines, held loops, absent clips and a real winning-card
canonical comparison. This source correction is later than stage7; its
executed gate must retain the next integration checkpoint's identity.
