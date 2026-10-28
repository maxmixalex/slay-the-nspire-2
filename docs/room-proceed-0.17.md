# Rest and treasure room completion — 0.17

Reference: the owner's pinned **v0.111.0 / 41cef1ea** assembly. Original source and assets remain private.

## Rest selection and travel are separate

`RestSiteSynchronizer.ChooseOption` awaits the selected option. A cancelled option leaves the remaining list intact. A successful option normally clears the remaining list; `MiniatureTent.ShouldDisableRemainingRestSiteOptions` instead removes only the chosen option. `NRestSiteRoom.AfterSelectingOptionAsync` completes the option presentation, updates that list and reveals Proceed in both cases. Its Proceed handler opens the map. The original fire is extinguished when the remaining list is empty, including options that are visible but currently unusable in that count.

The native controller therefore retains `Phase::Camp` and the open room after every successful rest option. `world_rest_proceed` is the explicit transition to the map. Pending card, relic or potion choices must resume the option before Proceed becomes available. Cancelling Smith or Cook neither consumes the option nor unlocks an initial Proceed action.

The existing `rest_used` mask now explicitly records slots removed from the original remaining-options list. Ordinary completion marks every current slot; Miniature Tent marks only the selected slot. Slot numbers remain stable for input and saved continuations. `world_rest_option_visible` reports list membership, while `world_rest_option_enabled` additionally checks usability and pending choices. This persists the original list update without changing the save layout or depending on a later live inventory check.

## A closed treasure may be left

`NTreasureRoom._Ready` exposes the ordinary Proceed button before opening the chest. That action uses the terminal-room travel path. `OpenChest` is the separate operation that generates normal gold, awaits extra rewards and exposes the prepared relic selection. The relic itself was already pulled by `TreasureRoomRelicSynchronizer.BeginRelicPicking` during room entry.

For a native treasure room, `leave_treasure` consequently accepts an unopened chest. It closes the room without invoking the chest-opening rewards or acquiring its prepared relic. Existing historical expedition behavior remains unchanged. The original 2.5-second delay before the opened chest's Skip button is a presentation detail, handled separately from this room-state change.

## Original room presentation

Camp selects the original lit or extinguished background for its region and projects the remaining original option hierarchy into four readable choices per page. Input keeps stable option indices after Miniature Tent removes an entry. A successful ordinary option leaves the empty room with an explicit Proceed action. Treasure selects the original closed, outlined and open chest for the act, followed by the 1,133 ms opening and 500 ms shine clips. The opened-relic Skip delay is 2.5 seconds after any extra reward selection has finished.

These clocks and their complete 64-bit native run seed, act and map-point identity belong to `App`, not `State` or the save codec. Loading an opened chest displays its final pose. Replacing a room with a different identity invalidates its old clock. Presentation updates must preserve the complete canonical save bytes, including all RNG state. The normal title, pause, settings, confirmation, card/relic detail and map modals capture room input before actions can reach the core.

The resource source appends 17 static rest/treasure images and six opening/shine clips, preserving existing directory IDs. Source9 appends another 48 actor-plane clips (8,340 planned frames). The five characters and Osty use the original single-player container and scene root override, the act's loop and a separate light-off track. Necrobinder and Osty retain independent full periods. Defect's continuous additive glow slots are kept between the two normal planes. Regent's authored default-MIX material overrides its skeleton's multiply slot, so the final render follows the scene material. The runtime uses a shared camera and normalized loop duration; long clips reduce sampling rate to meet the 256-frame bound without truncating their period.

The room-specific renderer evaluates four fingerprinted light/fire shaders, including their curve/gradient textures. Static rest backgrounds and the actor's attached fire use shader time zero. A time sample does not imply a loop period. Dynamic shader time, Chaotic starting phases, foliage wind, particles, GPU filtering and per-draw quantization remain explicit gaps. A limited overgrowth comparison and eleven actor frames are development diagnostics, not an original-game visual comparison. The calculator moves the description to x143/y142 in a 164-pixel column with up to five lines to leave the observed left-side actor area readable.

Treasure's collection/chest opening alpha uses a 400 ms linear tween; closing uses 300 ms. The banner begins on the opening click, including while extra reward choices are pending, with the original 400 ms EXPO_OUT alpha and BACK_OUT offset. Closing captures the current chest/banner alpha and position, so early acquisition does not jump or restart Spine. The original single-player holder becomes invisible immediately after acquisition; it is not painted during its parent's closing fade. Empty collections close visually at the first presentation tick at or after one second. Restored offers display the terminal chest pose and use a separate closing mode when claimed.

The integer presentation helpers saturate elapsed time before multiplication and use explicit 33 ms samples of the original easing formula, including the EXPO_OUT 1.001 correction. These are App-only clocks and Canvas opacity, not new saved state. Randomized holder delay, black-to-white tint, BACK_OUT holder travel and corresponding input waiting are still incomplete, along with the empty-label spring and reward/particle motion. The readable calculator controls currently remain usable during the collection fade. This stage does not establish 95% UI similarity.

## Verification scope

`tests/world_map_regression.cpp` adds ordinary Smith cancellation, completion, save/resume and one-time Proceed; a real Dream Catcher reward pause before Camp completion; and travel through the original map into an unopened treasure room followed by leaving it without gold, relic acquisition or any named RNG draw. Existing Miniature Tent and two-card Cook checks remain intact and now check visible slot membership.

`tests/world_room_ui_regression.cpp` additionally covers real rest/treasure routes, keyboard and pointer actions, modal capture, one-time rewards, loaded chest poses, stale complete-seed identities, visual-clock purity and pending relic acquisition. Source9 also checks fade endpoints, early claim continuity, extra-choice timing, empty collection timing, unsigned tick wrap and loaded terminal poses. Short syntax, asset-free converter and isolated Canvas/easing diagnostics are separate from the full engine suite, which still requires a pushed, read-back frozen checkpoint. It is not an original-game differential result or a visual fidelity measurement.
