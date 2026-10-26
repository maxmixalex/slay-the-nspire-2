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
