# UI20: hover artwork, registered action frames and reward identity

The reference remains the owner's v0.111.0 / `41cef1ea` installation. All
original images, sampled skeleton geometry, generated frame tables and packs
remain private. This source checkpoint precedes the complete bake/validation;
the final section will record actual results before integration.

## Corrected behavior

- Power hover rows use the same source status image lookup as their combat
  badge. The shared helper is also used by merchant hover preparation.
- A hover containing only derived cards anchors its preview to its real owner
  on both screen edges. It no longer reserves an absent keyword column.
- A terminal core phase does not suppress the visual Osty death edge while
  queued combat feedback is still being presented. His original `die` clip
  now proceeds to the original `dead_loop` when the new assets are present.
- Card reward rows use the original rare, uncommon and special-card artwork.
  The rarity rule follows `CardReward.get_IconPath` (original token
  `0x06000f31`) and `TryGetSingleRarityInPool` (`0x06000ac6`): boss encounter
  rewards are rare; a source pool containing only one rarity uses the first
  generated card's rarity. A mixed source pool retains the ordinary icon.
  `SpecialCardReward.get_RewardIcon` (`0x06000fb8`) supplies its own row icon.

## Animation density and storage

The complete UI19 ordinary-enemy audit found nine action clips, 325 frames,
whose pixels were below the native four-pixels-per-21-source-units density.
The old per-clip fit included the entire travel path in one bounded bitmap.
For example, Ceremonial Beast's `plow` used 16,149 inverse-scale units versus
5,376 for its idle. The runtime had to enlarge those pixels about three times.
Other affected source models are Bygone Effigy, Frog Knight, Mecha Knight,
Thieving Hopper and Waterfall Giant. Vantom already has a separate continuous
registered-layer renderer and is not rebaked by this stage.

The new builder samples the official Spine runtime at the exact previous
frame counts and times. It rasterizes each frame at native density, trims its
transparent boundary and preserves signed world coordinates. Only a single
frame larger than 320×240 is split into bounded pieces. Invisible pieces are
culled before pixel decoding. The tiny registration list is copied before
loading pictures, so cache page replacement cannot invalidate its pointer.

The 325 old frame image IDs are reused for each frame's first piece. Overflow
pieces, Osty's dead loop, 10 private word tables and three reward icons append
to the directory. Existing clip metadata, IDs, clocks and units are unchanged.
NSP2/44 remains the intermediate format; the separate storage stage reclaims
unused tile slots, applies lossless compression and creates locale editions.
The resource reader recognizes the complete 5,422-entry endpoint as well as
the previous 5,409-entry endpoint. No resident pixel pool grows.

The accepted prior 454a6 enemy / 555a6 player colour precision is retained;
this stage raises spatial resolution without lowering colour precision or
removing frames. Runtime pixels still use RGB565 and the existing alpha path.

## Validation pending at the source checkpoint

New checks cover owner-edge hover placement, power-image identity, terminal
Osty impacts, source reward rarity policy, moving geometry at constant pixel
density, exact reconstruction of split synthetic frames and signed private
registration tables. The actual-pack renderer test visits every repaired
frame, validates bounded parts, off-screen culling, loop/clamp endpoints and
distinct reward images. Full results and private artifact paths follow after
the checkpoint is published and the source bake finishes.
