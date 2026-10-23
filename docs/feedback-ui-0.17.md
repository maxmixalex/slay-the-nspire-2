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

Waits are rounded up to real-time 33 ms presentation ticks. Fast selects
the separately audited command durations, without a global clock multiplier.
The original animator's
conditional triggers remain conditional: a Fast Attack that arrives while
Uncurl is still active is not turned into a fabricated any-state transition.
No occupancy table was extrapolated to other monster models. Power/death
hooks and effects beyond these base move bodies still need a full replay
timeline before all combat effects can claim original timing.

### Fast clock correction

The pinned `NEnemyTurnBanner.Display` and `NPlayerTurnBanner.Display` use
ordinary tweens lasting 2.3 and 2.2 seconds. Neither has a Fast branch or a
speed override. `PrefsSave.FastMode` only stores the preference;
`Cmd.CustomScaledWait` selects the short wait explicitly, while `Cmd.Wait`
uses its supplied seconds. `CreatureAnimator.SetNextState` also has no Fast
speed change. A DLL call-site audit found global Engine time changes only
in debug time control and hit-stop, not in the Fast preference path.

The previous App clock multiplied all presentation time by two in Fast.
That shortened banners, skeleton clips and health-bar trails despite the
correct independent command waits. The follow-up removes that multiplier
and its compensating factor in command-wait conversion. The same single
33 ms clock now drives cues, busy gates, clips, damage values and health
tweens. Map transitions retain their already explicit Normal/Fast durations;
merchant, energy-orb and map animation clocks remain in real milliseconds.

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

## Deferred enemy HUD values (partial formal verification; follow-up pending)

The authoritative simulation remains synchronous. An App-only bounded
`CombatFeedback` projection now observes the settled values after each core
effect and displays them at the existing enemy animation cues. It stores
player, Osty and eight enemy health-bar values, energy, the previous intent
summaries and at most 128 changed-actor samples. It occupies 6,032 bytes on the
64-bit host; a compile-time 8 KiB ceiling
prevents an accidental full-State copy. None of these fields are serialized,
and the read-only observer adds neither gameplay effects nor RNG draws.

Nibbit's attacks release HP/block at its original 150 ms animation delay
(75 ms Fast); Louse Web/Pounce use 200/100 ms after any conditional Uncurl.
Nibbit Slice gains block after its 350/175 ms attack command. Louse Curl gains
block after its 250/125 ms trigger delay. Power/block command tails keep the
already audited cumulative waits, rounded once onto the 33 ms clock. These
times come from the pinned attack/monster/Block/Power command methods listed
above; other models use the existing move-start fallback and do not claim
audited hit or multi-hit timing.

Old intents remain until that enemy's PerformIntent lead-in, then hide until
the next player-side boundary. Energy resets at that boundary. Health-bar
tweens, top-bar HP, player hurt, damage numbers and Osty's HP all consume the
same projection. Canonical enemy removal cannot hide an actor before its
displayed lethal hit. New/reused slots switch identity with their queued
values. A lethal player hit retains the battle scene through the effect cue
before showing the canonical game-over screen.

Loading/resetting presentation discards the trace and reads canonical state.
Suspended card/relic choices keep the trace open for their continuation. If
the fixed queue fills, it retains the final captured values and converges at
the scheduled side boundary. One shared gameplay-action guard covers the
scheduled feedback endpoint, suspended choices and the generic busy counter.
Potion use/discard, card release and EndTurn cannot bypass it through keyboard
or pointer actions. Browsing remains available. Actual pending choices are
handled before this guard and can resume their suspended effect, even after
their old presentation deadline has elapsed. Added regressions exercise
Normal/Fast two-enemy damage,
block/energy/intent stages, byte-identical saves against observer-free core
resolution, pet interception, lethal first/second hits, reflected lethal
damage, summon/reused identity, suspended relic choice and overflow.

Checkpoint `9d8cb1fd58d2ea73900449415c925592776a5584` completed its logged native
build, 20 banner checks, 1,462 event/shop checks per language and 484 world-event
checks per language. Its two world-UI runs passed the sequential HP/block,
energy, intent and save-byte comparisons before stopping at the Fast second
lethal-hit banner assertion. Initial inspection considered relaxing that
assertion, but the independent original-clock audit above identified a real
global 2x clock defect instead. The assertion is retained. The follow-up fixes
the clock and adds actual `App.advance` checks for fixed banner lifetime,
retained Hiss frames and the one-second health-trail hold in both modes.
Retaliation, reused slots,
suspended-choice, overflow and the new potion-input guards still await the
follow-up saved-checkpoint run. Clang syntax checks passed on these follow-up
changes. All formal journals report unchanged source.

Checkpoint `14a20900035b5e554e076263fc22fe7611d2e410` passed its native build
and 21 banner checks. Both actual-language world-UI runs then passed the new
real-time clock/clip/health-trail checks, sequential values, both lethal-hit
orders, reflected lethal damage, summons and reused-slot identities. They
stopped at the Toasty fixture: in-combat relic acquisition still had its
queued continuation, so `end_turn` correctly refused to start. The fixture
now resolves that acquisition and asserts a ready combat before EndTurn.
The actual choice/input/overflow checks remain pending its next checkpoint.
Both actual-language frontend suites passed 567 checks. The English map
suite exposed an older real-pack fixture which advanced without rendering
the mandatory battle-preparation frame; its image cursor correctly stayed
at zero. The follow-up displays that frame, bounds preparation by the declared
image capacity/batch size and checks that the elapsed fade clears without
changing canonical state or submitting another room entry. Production map
durations and preparation code are unchanged.

Remaining limitations are explicit: status/power strips and card/pile state
still read canonical values; this is not a complete replay of every effect.
Intent tooltips are suppressed during the short deferred side because their
formatter still binds canonical parameters. Merchant candle GPU particles
and Necrobinder flame shaders are not prebaked. Unreviewed models still use
clip duration for action occupancy and the move-start hit fallback.
Device frame rate requires actual calculator/emulator
measurement rather than host timing.
