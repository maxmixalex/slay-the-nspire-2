# Reward presentation, UI5

Pinned reference: v0.111.0 / `41cef1ea`, as identified by
`config/reference-release.json`. Only implementation metadata is recorded here;
the owner's extracted/decompiled source and visual assets remain private.

## Implemented contracts

- `NRewardsScreen.AfterOverlayShown`: window alpha rises linearly for 500 ms;
  vertical position enters with BackOut from 100 original pixels below. The
  calculator uses a 20 px offset. Collecting removes the actual row immediately;
  a terminal empty window fades for 250 ms and leaves Proceed available.
- `NRewardButton.GetReward` invokes the inventory acquisition animation from
  its icon position after a successful claim. Potion movement is 350 ms
  QuadOut (`NPotion`); relic movement is 350 ms SineOut
  (`NRelicInventoryHolder`). The real destination icon is withheld while its
  one moving representation is visible. Full-belt failures create no flight.
- `CardReward.OnSelect` explicitly reparents the selected holder's card into
  `NCardFlyVfx` targeting `PileType.Deck` after `CardPileCmd.Add` succeeds.
  The flight retains the source acceleration/duration ranges, quadratic
  Bézier direction, early shrink/darken and final shrinking tail. Cosmetic
  parameters come from an independent presentation hash, never a gameplay RNG.
  Rendering uses the newly acquired deck slot for enchantments, permanent
  Genetic Algorithm/Scythe growth, portrait type and energy offsets. The hand
  cache key still contains the resolved portrait/frame/text/cost; no new raster
  allocation is introduced for pickup cards.
- `NTopBarGold.UpdateGoldAnim` supplies the count popup; there is no invented
  coin-flight effect. It enters over 250 ms, waits 150 ms, consumes the delta
  in the source 75/10/1 increments, waits 250 ms and withdraws. Large uniform
  ranges are skipped arithmetically, leaving at most 50 one-unit steps per
  query rather than work proportional to the gold reward.

The UI observer snapshots only potion identities, counts, acquisition origins
and the current reward context. A serial suppresses duplicate observations
from nested pointer/key/action handlers. Rendering, hover, failed claims and
save loading cannot manufacture a reward or advance game RNG. Twelve bounded
flight descriptors reuse the existing hand-card raster cache; they do not add
a second framebuffer or another card image pool.

## Adaptations and remaining differences

The calculator has three visible potion slots per page and fifteen visible
relic slots. An off-page bottle flies to the page affordance; overflow relics
land at the existing additional-relic indicator. Window/banner geometry is
compressed below the 22 px HUD and relic strip, leaving Proceed at the bottom.
Card subpage headings also begin below the relic strip. Eight-card compact
offers reserve separate bounds for both card rows and their alternative/footer
controls; three-card pages retain the larger card faces.

The Spine/particle card trail and inventory flash shaders are not reproduced
by the integer raster flight. Its card tangent uses a direct integer angle
rather than Godot's frame-dependent rotational smoothing. Special-card grants
without a visible card holder are not assigned a fabricated reward-card
origin. Card selection entrance/fade timings and reward selection reticle
artwork still require their own parity work.

The legacy prototype reward route remains separate from the native world
reward list. Statements about native reward parity must not imply that every
prototype screen was replaced.

## Verification

The reward UI regression checks window entry/empty fade, source potion/relic
timings, bounded gold stepping against the literal delay sequence, exactly one
pickup per real claim, failed-belt behavior, and unchanged serialized state
through animation. Existing reward ownership, card facts, reroll/alternative,
terminal Proceed and event-combat extra-reward tests remain in place.
The stage 3 runtime and actual English/Chinese v22 packs each pass 2,915
reward UI checks, including fixed-clock pixel changes/restoration for newly
acquired Genetic Algorithm, Scythe, Mad Science and enchanted cards. This is
recorded in [frontend evidence](validation-ui5-frontend-0.17.json), separately
from earlier stage 2 results and later resource-layout comparisons.
Private capture names include `reward-potion-flight` and `reward-relic-flight`.
Those two single-item fixtures return to the legacy prototype map; their
background is not a native-map acceptance image. `rewards-combat-extras` uses
a real native event-combat reward route. `reward-card-genetic-flight` isolates
the acquired card's stored facts at a fixed presentation clock.
Desktop captures and synthetic timing checks do not establish physical
calculator cold-I/O latency or touchpad frame pacing.
