# Potion interaction in UI5

The reference is the owner's private v0.111.0 / `41cef1ea` assembly and PCK,
identified by `config/reference-release.json`. Source evidence comes from
`NPotionHolder`, `NPotionPopup`, `NPotionPopupButton`, `PotionReward`,
`PotionCmd` and `FoulPotion`. No private assembly or decompiled source is
included in this repository.

## Changes

- Each top-bar bottle opens its own anchored two-action popup. Hover does not
  choose another bottle. Empty slots have no use menu. Original popup artwork
  is appended by the v22 converter; older resources retain a compact fallback.
- Use and discard use the original localized labels. Targeted and all-enemy
  bottles use the throw label; only an actual targeted bottle enters a second
  target-selection step. Cancelling the menu or target leaves canonical save
  bytes unchanged. Automatic bottles disable manual use and focus discard.
- Targeting reuses live creature geometry. Foul Potion outside combat targets
  the visible merchant, including the fake merchant's scene-derived rectangle.
  Opening the shop inventory disables that merchant use path. Combat Foul
  Potion remains an all-enemy effect and needs no individual target.
- The popup appears in 100 ms with a 150 ms Sine/In positional entrance. Used
  bottles shrink over 200 ms; discarded bottles move upward over 400 ms using
  the original Back/In timing. Placeholder appearance starts after 200 ms.
- World and event reward lists no longer replace an existing potion when the
  inventory is full. The attempted reward remains available and unclaimed.
  The player can use an allowed anytime potion or discard a bottle, then take
  the reward. Sozu's failed-procurement result follows the original claim rule.
- Non-combat potion/HP hooks with no applicable listener no longer accumulate
  suspended frames behind an open reward list. Suspended combat callbacks are
  retained for their original resume point.
- Confirmed combat use starts the same bounded command projection used by
  cards, so the damage and status display follows the command cues.

## Calculator adaptations and remaining differences

The 320×240 top bar exposes three bottle slots at a time. Additional physical
slots use a small page affordance; keyboard navigation can reach every bottle.
The core still owns all sixteen possible slots. This pagination is a device
adaptation, not a claim about the original PC layout.

The target arrow currently uses the existing calculator arrow rendering. The
original target arrow's texture/particle appearance, popup exit fade, bottle
hover bounce and the complete hover-tip set are not fully reproduced here.
Direct scripted event potion exchanges outside the reward-list controller
retain their previous behavior until their original paths are separately
audited.

## Verification contracts

`potion_ui_regression` covers exact bottle selection, hover purity, pointer and
keyboard cancellation, target confirmation, automatic-use disabling, immediate
self-use, empty slots, expanded-slot navigation, merchant targeting and the
original encounter's visual damage projection. The existing core and reward
suites cover full inventory rejection, clearing slot 15, retrying the retained
reward, Sozu, blocked event choices and suspended combat callbacks.

Test execution and actual-resource results belong in the current WORK_STATE;
the presence of these cases alone is not a pass result.
