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
  keywords stay with the card and remain inside the screen. This corrects
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
replace their existing image IDs as independent sparse/LZ4 images. Every
output plane is independently decoded again and recomposed against the
original RGB565 and alpha planes; the private manifest contains per-clip
pixel identities and encoded/decoded storage counts. The release converter
can then reclaim tile pixels no longer referenced by any live image.

The public small-pixel tests cover opaque black invariants, partial alpha,
changing alpha, disjoint composition, independent encoded readback and
rejection of altered frame dimensions. The actual private conversion and
Firebird cold/sustained measurements remain separate acceptance gates.

At checkpoint `97a63d78979daba7d12fa8475b305e836fc9503c`, journal
`merchant-ui14-exact-split-3cfddc4f2303` closes PASS with unchanged source in
32.72 seconds. All 1,817 encoded moving frames and ten fixed planes recompose
exactly. Total decoded sparse bytes fall from 15,482,673 to 11,571,712,
including 44,267 fixed-plane bytes, a reduction of 3,910,961 bytes (25.26%).
Moving visible pixels fall from 5,440,099 to 3,148,513. These are complete
catalog totals across all five characters; one room displays only its selected
character. With 4 KiB allocation rounding, a complete closed-room cycle uses
1,468 to 1,946 image pages, or 1,759 to 2,237 with both hover skins retained,
before the small backdrop/light/UI set. Final installed bytes and frame rate
still require the integrated reclamation and ordinary-OS Firebird gates.

## Validation scope

The terminal reward regression collects the real first encounter's rewards
through the public UI, waits 60 frames on its empty list, and checks that
only Proceed opens the map. The merchant preparation regression checks
immediate opening, cancellation, bounded preparation steps and a stable
opening clock. Ordinary event geometry checks accept the variable-height
buttons while retaining viewport and non-overlap constraints.

These regressions do not establish calculator performance. The reported
closed-merchant frame-rate regression still requires matched ordinary-OS
Firebird measurements, including the first animation cycle and sustained
idle; warmed inventory measurements alone cannot close that report.
