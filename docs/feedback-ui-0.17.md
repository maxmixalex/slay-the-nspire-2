# UI feedback corrections after source38

Reference remains **v0.111.0 / 41cef1ea**. All original textures, private IL
inspection, generated animation frames and screenshots stay outside Git.

## Original evidence and resulting behavior

| Concern | Pinned source | Change |
| --- | --- | --- |
| Merchant placement | `scenes/rooms/merchant_room.tscn`, `merchant_button.tscn` | One 1920×1080 camera covers the 320×218 room. The bottom skeleton has origin (−10,20), scale .505; top has origin (83.3,71.32), scale .470095. Independent sprite bounding-box fitting is removed for NSP2/18. |
| Player in shop | `scenes/merchant/characters/*_merchant.tscn`, `NMerchantCharacter._Ready` | Dedicated `*_shop.atlas` with `relaxed_loop`, original positive scale .47 (.23 for Defect), at CharacterContainer (564,766). |
| Opening shop | `NMerchantButton.OnRelease`, MerchantButton bounds (1206,468,270,330) | Click the merchant's body. The extra goods button and name caption are removed. The shared original corner reticle indicates focus; Enter still opens the shop. |
| Proceed | `scenes/ui/proceed_button.tscn` | Shop uses the existing original `common.PROCEED` atlas arrow rather than a reward-skip skin. Preserve aspect ratio and transparent margins; keep the whole image within the small viewport. |
| Energy | `scenes/combat/energy_counters/*`, `NEnergyCounter.RefreshLabel` / `_Process` | Five actual orb layers per class (three for Necrobinder); rotating layers 30/60 degrees/s, 5/10 at zero. Counter shows Energy/MaxEnergy. Cream/class outline normally, red/dark orb at zero. |
| Attack intent | `scenes/combat/intent.tscn`, `AttackIntent.GetTexture` | Retain the original total-damage icon thresholds. Damage is cream with a dark outline; the repeated-hit `xN` suffix uses the smaller 18/22 size relationship. |
| Turn banners | `player_turn_banner.tscn`, `enemy_turn_banner.tscn`, corresponding Display methods | Gold title, blue turn counter, original opposite title/counter motion and independent fade/zoom clocks. First turn does not create the player banner. An enemy banner can still be fading when the next player banner begins. |
| Enemy cue | `CombatManager.StartTurn`, `ExecuteEnemyTurn`, `NCreature.PerformIntent` | Normal-speed banner lead-in is .8 seconds, then intent lead-in .4 seconds. The first move begins during the 2.3-second banner. Enemies retain sequential order. A scheduled command is no longer stalled by an unrelated previous non-loop clip; elapsed time is measured from its recorded cue. |

The camera, sprite scale and orientation are from original scene transforms.
The 42-pixel energy orb, 14-pixel maximum intent icon and bitmap text sizes
remain calculator readability adaptations. They are not pixel-for-pixel
claims about the desktop viewport.

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

The source stage passes C++ syntax checking. Targeted turn-banner and actual
pack UI regressions must be rerun after the source checkpoint; source review
and resource metadata alone do not establish visual completion.

Remaining limitations are explicit: merchant candle GPU particles and the
Necrobinder flame shaders are not prebaked; per-monster command-to-hit timing
still uses the existing clip metadata and synchronous simulation, so this is
not a complete action/effect replay system. Fast mode retains the existing
presentation-clock acceleration for enemy animation/banners. Device frame
rate requires actual calculator/emulator measurement rather than host timing.
