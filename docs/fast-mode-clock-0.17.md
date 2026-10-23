# Fast mode clock audit — 2026-10-23

## Conclusion and execution status

The pinned original selects shorter **command waits** in Fast mode. It does not
apply a blanket 2× speed to the enemy/player turn banners or creature Spine
tracks. The stage5 port combines explicitly selected Fast command durations with
a globally doubled presentation clock. Its wait conversion compensates for that
clock, but its banners and skeleton tracks do not. Consequently the command
cues retain approximately the intended Fast wall time while the banner and
animation finish too early relative to those cues.

The minimal correction is a real-time 33 ms presentation clock, with Fast
selecting the explicit Fast duration in `wait_ticks` without a compensating
multiplier. The UI owner has prepared this paired change for stage6. **This
document records completed reference inspection and a source review; the
stage6 wall-time execution gate is still pending.** No build, test or original
game capture is claimed here as evidence that the prepared correction ran.

The reviewed incorrect implementation is frozen checkpoint
`9d8cb1fd58d2ea73900449415c925592776a5584`. Earlier passing command tests establish
their tested cue arithmetic and save isolation, not original Fast-clock parity.

## Reference identity and inspection method

Reference: owner-supplied v0.111.0 / `41cef1ea`.

Original `sts2.dll` SHA-256:
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.

The metadata scan considered all 51,602 method definitions. All **50,816 methods
with bodies parsed successfully**, with zero parse failures. It collected static
uses of the 26 field/method symbols containing `FastMode`, `TimeScale`,
`SpeedScale` or `AnimationSpeed`, then examined the relevant callers. A separate
literal scan checked dynamically named `time_scale` and `speed_scale` accesses.
Its four matches are the declaration/use pairs of the ordinary MegaSpine
animation-state and track-entry `set_time_scale` bindings, which are included in
the call-site review.

The private metadata-only scan is 30,960 bytes, SHA-256
`d10441f9e83a37ff016257acb6e7fc4c760341d4feaefd13802a6d4399bfd41c`.
Original IL, original scene contents and the original assembly remain private.
This static analysis covers the pinned managed game implementation and the
reviewed scene properties; it is not a recording of the unmodified running game.

The relevant scenes have no authored time/speed multiplier:

| Original scene | SHA-256 |
| --- | --- |
| `scenes/combat/enemy_turn_banner.tscn` | `72cc1ee9bb067e0ed58d41ecf49a9dc6d5296480ffd03373e1021264ca0f6bc7` |
| `scenes/combat/player_turn_banner.tscn` | `cdd0b2b45a1f8e22caa3df27b43349f8b8dc96272619bf5c84a80cdeb4e8d9ff` |
| `scenes/creature_visuals/nibbit.tscn` | `7fd3b6e0d75e56ceaa7420ca92afd85c700d0aa52c00701d4f9238df8e9a9540` |
| `scenes/creature_visuals/louse_progenitor.tscn` | `8b8208d90c74a9dfa60405b946e739cd1500118f4100a8825dccc7dc0927e3b1` |

## Original methods and timing contracts

Tokens below are metadata identities in the pinned DLL. For async functions,
the token identifies the actual state-machine `MoveNext` body.

| Original method | Token | Observed behavior |
| --- | --- | --- |
| `PrefsSave.set_FastMode` | `0x0600068e` | Stores the enum field only; no clock or animator update |
| `NFastModeTickbox.OnTick` / `.OnUntick` | `0x06006c98` / `0x06006c99` | Select Fast value 2 / Normal value 1; separate Instant value 3 exists |
| `Cmd.CustomScaledWait` async | `0x0600b2d0` | Selects `fastSeconds` for Fast and `standardSeconds` for Normal, then calls ordinary `Wait`; Instant skips |
| `Cmd.Wait` async | `0x0600b2d2` | Passes the supplied duration to a scene-tree timer; does not halve it again for Fast |
| `CombatManager.StartTurn` async | `0x0600b3d4` | Creates the side banner, then requests 500 ms Fast / 800 ms Normal |
| `NCreature.PerformIntent` async | `0x0600c7b2` | Plays/freezes the current intent, starts its 400 ms hide tween, then waits 250 ms Fast / 400 ms Normal |
| `NEnemyTurnBanner.Display` async | `0x0600c7dd` | Ordinary tweens: scale for 750 ms and fade in for 1,300 ms in parallel, followed by 1,000 ms color/fade-out; total 2,300 ms |
| `NPlayerTurnBanner.Display` async | `0x0600c836` | Fade in for 1,000 ms alongside 1,500 ms movement, then hold 400 ms and fade out for 300 ms; total 2,200 ms |
| `CreatureAnimator.SetNextState` | `0x06005a22` | Starts the named Spine animation and its queued successor; no Fast branch or global speed assignment |
| `CreatureAnimator.OffsetLoopingAnimation` | `0x06005a27` | Independent cosmetic loop speed variation from 0.9 to 1.1 and a small track-time offset, without consulting Fast |
| `NCreature.AnimDie` async | `0x0600c7b0` | Waits remaining animation time plus 500 ms, capped at 20 seconds, using ordinary `Cmd.Wait`; no Fast halving |
| `NHealthBar.AnimateInBlock` | `0x06008a8d` | Uses 500 ms position/alpha tweens; its speed-mode shortcut is Instant, not Fast |

Neither banner calls `CustomScaledWait`, consults the preference, nor sets tween
speed. Its ordinary tween durations therefore remain the same in Normal and
Fast. The first enemy move can begin while the banner is still present: its
base lead-in is 800 + 400 = 1,200 ms in Normal and 500 + 250 = 750 ms in Fast,
before additional model or hook delays.

### Why there is no inferred global Fast multiplier

The complete managed call-site scan finds Engine time-scale writes in
`NGame.DebugModifyTimescale` (`0x06005acf`) and `NHitStop.SetTimeScale`
(`0x06006467`). Their behavior is debug adjustment or temporary hit stop.
The Fast preference setter does not call either, and the Fast checkbox only
changes the preference.

The Spine time-scale call sites are the independent cosmetic loop offset,
Doom animation, treasure-button animation, fake-merchant animation and the
generic Spine wrapper. There is no Fast-driven speed assignment in the reviewed
creature animator or visual initialization. These explicit local uses must not
be conflated with a global gameplay preference multiplier.

`settings_ui.FASTMODE_DESCRIPTION` describes faster combat actions and screen
transitions; its Chinese localization uses broader animation wording. The port
should retain the original localized key. That prose does not establish a
uniform 2× time base, whereas the actual timers and animation paths distinguish
their durations explicitly. The port exposes Normal/Fast, not the separate
original Instant shortcut.

## Command waits remain independently selected

The previously audited Nibbit/Louse command totals remain applicable. They are
action occupancy, not the duration of a skeleton track. Their source inspection
and model boundaries are recorded in [feedback-ui-0.17.md](feedback-ui-0.17.md).

| Action | Normal wall time | Fast wall time |
| --- | ---: | ---: |
| Side-start wait | 800 ms | 500 ms |
| Each enemy's intent wait | 400 ms | 250 ms |
| Nibbit Butt | 350 ms | 175 ms |
| Nibbit Slice | 600 ms | 275 ms |
| Nibbit Hiss | 850 ms | 350 ms |
| Louse Web Cannon, already uncurled | 650 ms | 300 ms |
| Louse Pounce, already uncurled | 400 ms | 200 ms |
| Louse Curl and Grow | 750 ms | 325 ms |
| Conditional Louse Uncurl prefix | 900 ms | 250 ms |

For example, Hiss retains its authored 1,633 ms clip while a later actor's
intent/action begins. Fast does not halve that clip. Louse's short Fast Uncurl
command delay can finish before its Uncurl track; the following Attack remains
subject to the original animator transition predicate. Neither case justifies
waiting for every skeleton to finish or inventing an any-state transition.

No command table is inferred for the remaining monster models. Their existing
clip-duration occupancy fallback is still a limitation, even after the common
animation clock is corrected.

## Stage5 failure mechanism and minimal source correction

At the frozen stage5 source:

- `App::advance` in `src/app_input.inc` adds `ms * 2` to the presentation tick
  remainder when Fast is enabled.
- `TurnBannerPresentation::wait_ticks` multiplies each selected Fast duration
  by 2 before converting it to 33 ms ticks. This compensates the clock for
  audited command waits: it is not a second halving of those waits.
- The banner samples `(animation - start) * 33`, and `enemy_frame` uses the same
  expression for the skeleton's elapsed milliseconds. Neither compensates.

Thus the 70-tick enemy banner disappears after approximately **1,155 real ms**
in Fast, while the intended 2,300 ms tween has not elapsed. The 67-tick player
banner similarly lasts about 1,105.5 real ms. A 1,633 ms creature track also
finishes at roughly half its authored wall time. Adjusting a test to expect no
banner at the second Fast enemy's lethal hit would preserve this defect.

The paired source change is:

1. Accumulate the real supplied milliseconds in `App::advance`, without a Fast
   multiplier; keep its existing 250 ms per-call cap and 33 ms tick sampling.
2. Convert the selected Normal/Fast command duration directly to ticks, without
   multiplying Fast milliseconds by 2.
3. Keep original banner/track durations and the explicit per-command Fast
   values. Update the assertions that previously encoded the global multiplier.

There is no need for another global clock, an added State field or a save
format change. Changing only one member of the pair would either double the
audited Fast waits or continue to accelerate the ordinary animations.

### Adjacent clock and input review

The current tree has no `real_animation` field and no invariant that relates
`animation` to `map_clock_ms` through a factor of 2. Merchant room/character
clips, the energy orb's rotation, map motion, map windows, buttons, rainbow and
thinking-dot effects already use `map_clock_ms` directly. Their durations need
no migration. Preparation can intentionally advance that real clock while
holding combat animation, so code must not create a new equality assumption
between the two counters.

`busy` is decremented once per presentation tick. The observer sets its deadline
from `enemy_visual_clock - animation` or the combat-feedback end time in the
same tick domain. Removing the global multiplier and its wait compensation
together preserves that relationship. The existing 8-tick potion tail,
12-tick card flight and legacy practice-enemy delays become ordinary wall-time
durations; no inspected original Fast contract warrants separately halving
those constants during this correction.

Death-track completion and its extra half-second wait also use this domain.
The original `AnimDie` uses an ordinary wait, so restoring real time is
consistent. The port's subsequent bounded opacity fade remains an approximation
of the original death VFX; this clock audit does not establish shader equality.

Two tests outside the battle-specific suite encode the old multiplier:

- `world_frontend_ui_regression::modal_isolation_and_fast_mode`: after two
  66 ms advances around a Fast toggle, animation is 1,004 and the initial
  12-tick busy/card-flight counters are 8, not 1,006 and 6.
- `world_map_ui_regression` reticle comparison: a 99 ms advance produces three
  animation ticks in either mode, while the already-real map clock still
  increases by 99 ms.

Production calculator, X11 and offline-browser loops supply elapsed time to
`App::advance`. The exported legacy `frame()` and scripted direct `tick()` calls
still mean one presentation tick; they do not establish wall-clock timing.

## Required independent wall-time gate — pending

Run the prepared tests only after stage6 has an actual push/readback receipt.
Use real `App::advance` calls in chunks no larger than 250 ms, with a known
initial tick remainder. Do not prove speed behavior solely by assigning
`animation` directly or by computing every expected value with the production
`wait_ticks` helper.

| Contract to check | Independent expected observation |
| --- | --- |
| Clock | Equal real 99 ms advances produce three ticks in Normal and Fast |
| Enemy banner | Both modes retain it at 1,500 ms and through tick 69; it ends at tick 70 / 2,310 ms, the declared 33 ms quantization of 2,300 ms |
| Player banner | Same alpha/motion for equal elapsed time in both modes; ends at tick 67 / 2,211 ms |
| First enemy cue | At or after 1,200 ms Normal / 750 ms Fast, within the sum of the two per-wait rounding errors; the single-clock design computes 38 / 24 ticks |
| Nibbit/Louse occupancy | Preserve the explicit wall-time table above, allowing less than 33 ms rounding per independently scheduled wait |
| Skeleton tail | The same uninterrupted clip has the same elapsed milliseconds in both modes; Hiss retains its 1,633 ms tail while later commands may overlap |
| Health display | Its fixed trail/hold durations do not halve when Fast is enabled; snapshots still converge to the canonical result |
| Input/state | Pending effects keep actions locked; display advancement changes no canonical save bytes or RNG state |

Rounding each wait independently can accumulate a bounded error across the
sequence. This audit does not label the proposed 38/24-tick cues or any pending
test as an executed result. Record the saved stage6 SHA, runner/source identities,
closed journals and actual results before replacing this pending status.
