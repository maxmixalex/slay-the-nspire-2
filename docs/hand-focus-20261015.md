# Hand focus source checkpoint

The usable 0.13 recovery has been promoted to main at `203c9dd004f3c674a8650b65d92a192b0469792f`. This next stage changes only presentation and input focus; no game rules, registry counts, save format or asset profiles change.

At rest the hand stays entirely on the bottom arc. Hovering a real hand region or navigating cards with left/right raises the focused card. Moving off the hand, ending an action, cancelling a pointer capture, or opening an overlay clears the presentation focus. Keyboard target selection preserves deliberate focus. Raised preview regions exist only while focus is active. The stable lower rail and upper collapsed card faces remain selectable.

The source includes focused/neutral transition regression cases for hand sizes 1-10, keyboard target changes, modal transitions, cancellation, instance-based drag completion and optional actual-runtime PPM captures. Validation of this new checkpoint is pending; previous 0.13 validation does not certify these edits. Persist and read the work-branch ref before executing the tests.

This fixes an always-raised-card presentation defect. It is not an assertion of original-PC pixel or animation-curve parity. CI stays disabled. No content additions are counted in this UI stage.
