# UI11 room and acquisition effects

Reference: **v0.111.0 / 41cef1ea**. The implementation uses the existing
original UI7 soft-light/speck masks, coin image, inventory icons and the already
installed room/actor animations. It adds no textures, image IDs, resource-pack
sections, saved fields or gameplay RNG calls.

The runtime implementation is in `src/room_secondary_vfx_ui11.inc` and the
existing camp, treasure, merchant and reward presentation files. Small masks
and bounded deterministic particles adapt the source effects to 320 × 240;
this is not a general GPU particle/shader implementation or a claim of
pixel-identical Godot output.

## Source contracts and implemented behavior

| Source | Behavior retained in UI11 |
|---|---|
| `HealRestSiteOption.DoLocalPostSelectVfx`, `NRestSmokeVfx`, `rest_smoke_vfx.tscn` | A successful Heal starts rising smoke once. Emission occurs during the first 0.6 seconds, particle lifetimes vary from 2.25 to 3 seconds, and the transient presentation expires by 4 seconds. Existing normal/fast choice waits remain 2.5/1.5 seconds. |
| `NDesaturateTransitionVfx.Animate` | One second toward brightness 0.1, saturation 0.25 and contrast 0.7; another second toward contrast 0.8; a final second back to normal. The LCD top bar remains legible and outside this room-only color treatment. |
| `NRestSiteRoom.ExtinguishFireIfAble`, original per-act rest scenes | Ambient light and embers follow each act's registered fire location and disappear when all rest choices are exhausted, alongside the already implemented dark room/actor variant. They do not restart a completed Heal after loading. |
| `merchant_room.tscn` | Candle cinders and the distinct warm, violet and blue light nodes supplement the already animated original flame planes. The room camera and rug/item layout remain unchanged. Settled inventory culling uses the actual opaque rug mask, including the full possible ember bounds. |
| `NTreasureRoom.OpenChest`, `treasure_room.tscn` | Opening a real chest emits coins only when that opening awarded gold. Their original 2.5-second particle lifetime and 1.8 speed factor are represented with a bounded burst and a reflected bounce. Restored open chests emit no new burst. |
| `NTreasureRoomRelicHolder.Initialize`, uncommon/rare glow scenes | Uncommon relics receive four cyan glows on a 3-second cycle; rare relics receive three gold glows on a 1.8-second cycle. They use the existing holder position, scale, opacity and entrance movement. Common relics receive no invented rarity glow. |
| Empty-label smoke in `treasure_room.tscn` | A newly shown empty chest receives a brief smoke puff. The effect follows the existing collection visibility and is not inferred from a restored empty room. |
| `NCardFlyVfx`, five `card_trail_*.tscn` scenes | A card's existing acquisition curve receives its character-colored ribbon and bright inner specks, sampled from the same flight. The trail fades at arrival and uses no independent movement/gameplay state. |
| `NPotion.AnimateObtained`, `NPotionFlashVfx`, `vfx_potion_flash.tscn` | The real belt icon reappears at the unchanged 350 ms arrival. A separate one-second additive flash expands over it. |
| `NRelicInventoryHolder.AnimateObtained`, `NRelicFlashVfx` | The real relic icon reappears at 350 ms, followed by three expanding/fading flashes starting 0/200/400 ms apart. These visual tails do not keep the inventory icon hidden. |

Treasure acquisitions now enter the same existing input-scoped observation as
shop/event/reward acquisitions. Only a successful action's actual inventory
or gold change produces a flight or counter transition. Hovering, rendering,
resetting presentation and restoring saves cannot manufacture an acquisition.

## Calculator bounds

Camp embers are capped at eight visible specks. Each merchant candle has four;
there are four candle emitters and eight light nodes. Heal smoke uses at most
twelve clouds, each assembled from three existing small soft masks; empty-chest
smoke uses six. Coin bursts are capped at twenty-four coins. Card ribbons use
eight historical curve samples per existing flight. All acquisition effects
share the existing twelve-entry flight pool and recycle its oldest entry when
necessary; there is no growing particle list.

The only added resident room state is a Heal start clock and active flag.
Particle phases derive from presentation clocks and fixed hashes, without
reading or consuming canonical run randomness. A small stack tile is used for
inventory flash color treatment; it is not a new image cache or persistent
atlas. Full-room color treatment runs only during the three-second Heal
transition.

The source GPU particle counts, exact stochastic trajectories, HDR bloom,
continuous subpixel rasterization and smoke texture silhouette are adapted to
the LCD. Primary authored room textures, character/merchant/chest animation
frames and timings are reused as provided by the finalized resource pack.

## Verification

`room_secondary_vfx_regression` checks the exclusion of the top bar from the
Heal color pass, effect expiry, restored-room suppression, actual Heal/chest
action triggers and canonical save/RNG immutability during visual sampling.
With a final optional-language directory, it additionally uses the real
installed masks/icons and verifies exact pixels for culled versus complete
merchant effects beneath the rug. It checks that the actual inventory icons
become visible at arrival while their independent flashes continue.

Run from a frozen source checkpoint:

```sh
room_secondary_vfx_regression /private/final-editions
locale_editions_regression /private/final-editions
```

These native gates verify rendering and data safety. Firebird remains the
acceptance environment for the ARM launcher, UI interaction, memory and
calculator performance. Test completion and emulator observations must be
recorded against the checkpoint that was actually executed.

At checkpoint `4e4ce0692a01132de613c01e3cb6a8a56f0d001c`, the final installed
common art and English edition pass **105 checks** in
`room_secondary_vfx_regression`. This includes successful Heal/open-chest
actions, restored-room suppression, effect expiry, top-bar clipping,
canonical save/RNG invariance and exact merchant pixels with/without rug
culling. The receipt is `ui11-room-final-asset-canonical-i-318ee8e5334f.json`,
with `source_changed=false`; the matching build receipt is
`ui11-clang-locale-room-build-i-943d0506436d.json`.
