# Original map window and retained rooms

Reference: the owner's pinned v0.111.0 / 41cef1ea installation. This stage
extends source15's map travel controller. Its purpose is to make an ordinary
map opening an independent window over the current room, including when
Proceed has been pressed, without restoring an old State or a screenshot.

## Controller and App boundary

The saved room remains the actual merchant, rest site, treasure room or event
after ordinary Proceed. `world_can_travel` represents the room's permission to
select a destination; visibility of the map is independent. The controller
keeps its canonical offers and completed actions until `world_choose_point`
commits the destination. `room_open == 2` remembers an explicit Proceed so
repeating it cannot increment the completed-room counter twice. Existing
0/1 values and the save layout are preserved.

The App marks the new window open after a same-room 1-to-2 transition, or after
an explicit repeated Proceed. It does not automatically open when a rest
option merely enables travel. Closing the window makes the same room
interactive immediately. Merchant purchases and an unused Miniature Tent rest
option remain available. Treasure travel remains locked until its relic
selection is claimed or explicitly skipped; a skipped relic cannot be reclaimed.
The settled chest and its room remain visible after closing the map. Room
cursor and text scroll are retained separately from map navigation, so drawing
the underlying room cannot change the selected route each frame.

The existing asynchronous travel controller accepts both the initial act's
standalone map and a travel-enabled window. Its saved-state fingerprint,
revision and full run/act/point identity guards remain in force. One actual
destination submission follows the existing Normal/Fast transition. A reset,
load, replacement run, room replacement or title exit discards stale window
presentation. Loaded saves restore the retained room; opening state and tween
clocks are not serialized.

## Treasure permission timers

A fresh empty collection permits Proceed and map travel only after 1,000 raw
milliseconds. A nonempty collection reveals its Skip permission after 2,500 raw
milliseconds; reaching that deadline alone does not resolve or abandon the
relic. Both timers start when the chest's own collection appears, after any
extra room rewards. They continue under a map or covering panel and do not
accelerate in Fast mode. The callback/permission completion is latched, so a
later millisecond wrap cannot disable an already available action. This change
does not retime the existing chest, banner or holder visuals.

The private source evidence is `NTreasureRoomRelicCollection.AnimIn`'s 1.0-second
empty callback, `NTreasureRoom.OpenChest`'s travel enable after
`RelicPickingFinished`, and `EnableSkipAfterDelay`'s `Cmd.Wait(2.5, token)`.
`Cmd.Wait` uses the same ordinary scene-tree timer in Normal and Fast; only the
separate Instant setting bypasses it. The port exposes Normal/Fast only.

Loading the port's mid-room save keeps the exact canonical opened chest and
settles presentation, including these timers; it does not replay rewards or
RNG. This is the existing calculator save compatibility choice, not a claim
that the original resumes the same mid-animation save. Original
`RunManager.ToSave` contains no chest animation clocks, its load path re-enters
the latest map coordinate, and `TreasureRoom.EnterInternal` rejects room-stack
reconstruction. Reset/load clear every collection timer field. Fresh openings
still receive their full delay, including after a previous chest was loaded.

## Original ordinary window timing and composition

The map and its black backstop open over 250 ms. The Points and Legend have
their separate 100 ms delay; DrawingTools timing has its authored 200 ms delay.
These are raw milliseconds and do not accelerate in Fast mode. Parent RGB and
alpha modulation are applied to each child draw; Points multiply the additional
child alpha. The authored Backstop alpha and its 0.85 modulation multiply at
the final coverage boundary. This is deliberately not a single fade of an
opaque flattened map image.

Closing immediately releases room controls while keeping the visual tail for
350 ms. Points, Legend alpha and Backstop fade over 150 ms; TheMap fades over
250 ms after a 100 ms delay. Its 200-source-pixel CUBIC_OUT translation and the
Legend's 120-source-pixel translation use the existing 0.22 map scale. The
post-tween scrolling tail follows the source node-before-tween process order.
This is a source-based implementation, not an observed original-GPU timing
comparison.

A covering deck, log, relic or potion panel retains the map instance. Closing
that capstone returns to the same window and does not restart its tween or
marker. The TopBar Map action closes a covering capstone first. During a close
tail it follows the original Visible-based toggle, so repeated clicks cannot
reopen or restart the tail before it disappears. The room's hit regions are
replaced while open and restored immediately when closed; stale press/release
pairs cannot cross the window boundary.

## Verification scope

`world_map_window_regression` compares the Q12 samples with independent
continuous equations, including every millisecond around opening and closing,
interruption at multiple open ages, immediate close, reopen and clock wrapping.
A bounded UBSan diagnostic passed 106,533 checks before checkpointing.

`world_map_window_ui_regression` exercises actual key/pointer handlers and
complete saved-state equality. It includes merchant reopening and purchases,
Miniature Tent's remaining Smith option, a claimed chest and the unopened
chest's read-only map, unchanged RNG/save
bytes during presentation, covering capstones, reset/load, and one delayed
route commit compared with a separate controller State. It accepts either its
explicit synthetic profile or the complete private content pack. The existing
map/travel/room/reward fixtures are updated to the retained-room contract.
The follow-up treasure cases check 999/1,000 and 2,499/2,500 real-millisecond
boundaries in Normal and Fast, clocks wrapping through zero, stale pointer
presses, read-only map routes before selection, and exact save bytes before
and after a mid-delay load. These follow-up tests require their own preserved
source and full execution; source16 results do not validate later source edits.
Full native/ARM/WASM results must be recorded only after the new source has
been pushed, read back, frozen, built and executed.

This stage does not establish the requested 95% visual acceptance. The initial
act's separate camera introduction/banner, original-PC image comparisons and
physical-calculator execution/performance remain independent work.
