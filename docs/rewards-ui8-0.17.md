# Reward screens — UI8, pinned v0.111.0 / 41cef1ea

This source stage replaces the enlarged legacy reward layout and pagination.
The reference is `RewardsScreen`, `NRewardButton`,
`NCardRewardSelectionScreen`, `NCardRewardAlternativeButton`, and their scenes.
Coordinates are projected from the original 1680 × 1260 UI to 320 × 240.
The original scene/data files stay private.

## Implemented behavior

The loot window uses its 526 × 640 source rectangle, 652 × 162 banner,
402 × 86 rows, 56 × 56 icons and 10 px source row spacing. A 474 × 484
mask clips the rows. Its 400 px scrolling viewport supports the original
continuous wheel motion, dragging, scrollbar and keyboard focus scrolling.
There are no synthetic Previous/Next page buttons. A partial row's hit area
is clipped to the same visible mask. The original rich text and Kreon Bold
loot heading use actual font glyphs.

Card rewards use the original small 0.8 scale, focused 1.0 scale, 350 px
source spacing, centered row and initial middle focus. The cards spread
over 500 ms with Expo Out and brighten over 1000 ms with Cubic Out. The
screen rejects card acquisition during its first 350 ms. Refreshed offers
restart their visual entrance without restarting the opening input delay.
Unfocus takes 500 ms. Original common-banner motion and the Skip/Reroll/
relic alternative row replace the old enlarged card window. Skip closes the
overlay without consuming the existing card reward. Physical alternative
keys act on release and retain the captured set identity. Inspect uses the
original Confirm binding. Offers exceeding the source's ordinary card-count
range are fitted to one row within 262 display pixels, an explicit adaptation.

Standalone potion rewards now bind the same entering-window state. They
previously inherited a zero-alpha state and could render invisible. Their
button still attempts the actual first empty belt slot; a full belt remains
interactive so the user can discard a potion and retry. No destination-slot
chooser is added.

Pending selections retain the room below the modal: event/ancient, merchant,
rest, treasure, map or combat, using the authoritative room/return phase.
The underlay cannot clamp the overlay cursor, scroll or active hit regions.
The reward room uses the actual combat character anchors and scale. It does
not redraw the withdrawn hand, energy, piles, intents or orbs. Detailed
combat-end transition differences, including frozen power displays versus a
loaded finished room, require separate verification and are not implied by
the static underlay correction.

Crystal Sphere terminal input now reaches its Proceed action. Its background
observation updates the crystal cursor only while the crystal grid is active;
it cannot overwrite a selected reward index underneath a reward overlay.

## Validation boundaries

Stage 4b's native gate passed 90/93 suites. Its failed Crystal terminal and
Crystal reward-index tests exposed the two implementation defects above;
their failed journals are preserved. Stage 5 adds real card-click gating,
release-triggered reroll, continuous list drag/scroll, clipped row hitboxes,
128/770-reward selection and unchanged-save checks to
`world_reward_ui_regression`. Source syntax checks passed. Full immutable
native execution and final ENG/ZHS artwork review are pending at this source
checkpoint; no fidelity percentage follows from test or model counts.
