# UI feedback corrections after source38

Reference remains **v0.111.0 / 41cef1ea**. All original textures, private IL
inspection, generated animation frames and screenshots stay outside Git.

## Original evidence and resulting behavior

| Concern | Pinned source | Change |
| --- | --- | --- |
| Merchant placement | `scenes/rooms/merchant_room.tscn`, `merchant_button.tscn` | One 1920×1080 camera covers the 320×218 room. The bottom skeleton has origin (−10,20), scale .505; top has origin (83.3,71.32), scale .470095. Independent sprite bounding-box fitting is removed for NSP2/18. |
| Player in shop | `scenes/merchant/characters/*_merchant.tscn`, `NMerchantCharacter._Ready` | Dedicated `*_shop.atlas` with `relaxed_loop`, original positive scale .47 (.23 for Defect), at CharacterContainer (564,766). Shop loops use real time independently of the combat Fast setting. |
| Opening shop | `NMerchantButton.OnRelease`, MerchantButton bounds (1206,468,270,330) | Click the merchant's body. The extra goods button and name caption are removed. The shared original corner reticle indicates focus; Enter still opens the shop. |
| Proceed | `scenes/ui/proceed_button.tscn` | Shop uses the existing original `common.PROCEED` atlas arrow rather than a reward-skip skin. Preserve aspect ratio and transparent margins; keep the whole image within the small viewport. |
| Energy | `scenes/combat/energy_counters/*`, `NEnergyCounter.RefreshLabel` / `_Process` | Five actual orb layers per class (three for Necrobinder); rotating layers 30/60 degrees/s, 5/10 at zero. Counter shows Energy/MaxEnergy. Cream/class outline normally, red/dark orb at zero. |
| Attack intent | `scenes/combat/intent.tscn`, `AttackIntent.GetTexture` | Retain the original total-damage icon thresholds. Damage is cream with a dark outline; the repeated-hit `xN` suffix uses the smaller 18/22 size relationship. |
| Turn banners | `player_turn_banner.tscn`, `enemy_turn_banner.tscn`, corresponding Display methods | Gold title, blue turn counter, original opposite title/counter motion and independent fade/zoom clocks. First turn does not create the player banner. An enemy banner can still be fading when the next player banner begins. |
| Enemy cue | `CombatManager.StartTurn`, `ExecuteEnemyTurn`, `NCreature.PerformIntent` | Banner lead-in is .8 seconds Normal / .5 Fast, then intent lead-in .4 Normal / .25 Fast. The first move begins during the banner. Enemies retain sequential order. A scheduled command is no longer stalled by an unrelated previous non-loop clip; elapsed time is measured from its recorded cue. |

The camera, sprite scale and orientation are from original scene transforms.
The 42-pixel energy orb, 14-pixel maximum intent icon and bitmap text sizes
remain calculator readability adaptations. They are not pixel-for-pixel
claims about the desktop viewport.

### Audited command occupancy

Animation duration is not action occupancy. `AttackCommand.Execute` awaits
its animation delay and `CreatureCmd.Damage`; block and power commands add
their own waits. It does not wait for the whole skeleton animation to end.
The following complete solo move bodies were inspected in the pinned DLL:

| Move | Normal wait | Fast wait |
| --- | ---: | ---: |
| Nibbit Butt | 350 ms | 175 ms |
| Nibbit Slice | 600 ms | 275 ms |
| Nibbit Hiss | 850 ms | 350 ms |
| Louse Web Cannon, uncurled | 650 ms | 300 ms |
| Louse Pounce, uncurled | 400 ms | 200 ms |
| Louse Curl and Grow | 750 ms | 325 ms |
| Conditional Louse Uncurl prefix | +900 ms | +250 ms |

These waits advance the next actor's cue without shortening the current
actor's clip. For example, Nibbit Hiss keeps its 1,633 ms animation tail while
the next enemy begins its intent/move. Uncurled Louse skips the conditional
prefix. The `curled_loop` animator state persists until Uncurl; the default
initial idle state no longer overwrites it during rendering.

Waits are rounded up to presentation ticks. Fast waits are independently
converted to the existing accelerated clock. The original animator's
conditional triggers remain conditional: a Fast Attack that arrives while
Uncurl is still active is not turned into a fabricated any-state transition.
No occupancy table was extrapolated to other monster models. Power/death
hooks and effects beyond these base move bodies still need a full replay
timeline before all combat effects can claim original timing.

## Resource extension

`include/feedback_assets.hpp` defines an append-only NSP2/18 profile:

- Aux 1236–1260: Ironclad, Silent, Defect, Regent, Necrobinder, five slots each.
  Regent repeats source layer 5 in its last two slots, as its scene does.
  Necrobinder's last two slots are transparent.
- Aux 1261: merchant background through the scene transform.
- Clips 1695–1701: bottom, top, then the five merchant character loops.
- Every NSP2/17 card, text, image, auxiliary and clip index stays stable.
- New compressed images are inserted before the locale payload. Chinese and
  English use the same generated art in the same physical order, allowing
  NSC1/NSL1 to share the entire art segment. Existing compressed payloads are
  copied exactly; glyph/string, image, group and descriptor pointers relocate.
- Legacy packs retain their former merchant art fallback while receiving the
  text, intent, banner and click-flow corrections.

`tools/patch_feedback_assets.py` checks the exact PCK SHA-256 and the relevant
scene transforms. Its private receipt records old-payload equality, old-index
stability, all relocated offsets and a decode/hash roundtrip for every new
image. Conversion must run through `tools/run_logged.py` after a verified
source checkpoint.

## Verification boundary

Verified source checkpoint `069f6d51f37aef3f31d11a6282bd02a61d7afdef`:

- v18 conversion completed for both actual language packs. All 1,398 appended
  images decode/hash roundtrip, old payload/index equality and pointer
  relocations passed. New common art is identical in both languages.
- Targeted native UI build passed. Both actual-pack world UI runs passed
  9,672 checks each, including all five energy orbs and merchant characters.
- Chinese event/shop UI passed 1,462 checks. The English run exposed an old
  reader-line test helper that omitted English word wrapping; its corrected
  helper and the later command-occupancy tests require the next checkpoint.
- Private actual-pack screenshots were inspected for merchant placement,
  original Proceed art, Energy/MaxEnergy and action/banner overlap. These
  screenshots and converted packs remain outside the source repository.

Follow-up source checkpoint `573cb816bbf6b57c8d677239060414c9fc124f72`
also completed its logged native build and actual-pack validation:

| Regression | English v18 | Chinese v18 |
| --- | ---: | ---: |
| World UI, including Normal/Fast command occupancy, two-enemy handoff, retained Hiss tail, conditional Uncurl, five energy/merchant profiles | 10,100 checks | 10,100 checks |
| Event reader and merchant body/purchase/back/proceed interactions | 1,462 checks | 1,462 checks |
| World event UI, six shop slots and eight ancient layouts | 3,656 checks | 3,656 checks |

The independent banner regression passed 20 checks, including both directions
of player/enemy fade overlap and the distinct Fast lead-in values. The corrected
English word-wrap helper passed without removing its scroll assertions. New
screenshots were inspected again after these changes. No original running-game
capture or calculator-device measurement was available to turn these results
into an overall UI parity percentage.

Merchant direction was checked against both original scene transforms and
`NMerchantButton._Ready`'s nested callback: it starts `idle_loop` and switches
default/outline skins on focus, with no flip/scale operation. `NSpineAutoPlayer`
also starts the authored loop without a mirror. The converter preserves the
bone/attachment transformations and positive scene X scale.

Later source changes must be rerun after their own verified checkpoint. Source
review and resource metadata alone do not establish visual completion.

Remaining limitations are explicit: merchant candle GPU particles and the
Necrobinder flame shaders are not prebaked. The simulation resolves enemy
damage, HP, block and next intents synchronously before the visual queue
plays, so those HUD values can change before the visible hit. The delayed
hero hurt pose does not fix that missing effect replay. Unreviewed models
still use clip duration for action occupancy. Fast mode retains the existing
presentation-clock acceleration for enemy animation/banners. Device frame
rate requires actual calculator/emulator measurement rather than host timing.
