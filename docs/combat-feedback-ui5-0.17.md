# UI5 combat presentation

Reference: **v0.111.0 / 41cef1ea**. The owner's restored `sts2.dll` matches
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.
The PCK scenes and DLL were read privately; this repository contains only
implementation code and the numeric/type facts needed by that implementation.

## Implemented behavior

| Area | Original evidence | Implementation |
| --- | --- | --- |
| Multiple hits | `AttackCommand.Execute`, `CreatureCmd.Damage`, `CreatureCmd.TriggerAnim` | Each hit advances its own presentation boundary. Distinct opponents in one hit share that boundary. Damage's post-hit wait is 200 ms Normal / 100 ms Fast, and the attacker delay repeats unless `OnlyPlayAnimOnce` is set. |
| Card attack delay | Five character models, their `AttackAnimDelay` getters, and card `OnPlay` overrides | The default is 150 ms, with separately recorded heavy attacks, thin slashes, cast attacks, Osty, beams, Shiv and no-animation exceptions. Fast halves the animation wait with the original 250 ms cap. |
| Monster attack delay | Monster move methods, joined to the existing audited move/node directory | `combat_command_timing.hpp` records 196 numeric move profiles. It covers AttackCommand waits, including explicit waits before a hit and animation-once flags. Earlier dialogue and other awaited commands remain the responsibility of the existing move timeline. |
| Power application feedback | `NCreature.OnPowerIncreased`, `NPowerAppliedVfx._Ready`, `PowerModel.GetTypeForAmount`, concrete power getters | Queued title and original icon feedback follows the same displayed status boundary. Buff titles move up and debuff titles move down over 1.25 s; the title has the original 250 ms entrance and 750 ms exit. The icon grows from 0.4 to 0.8 scale. |
| Star resource | `NStarCounter`, `NCombatUi.Activate`, `combat_ui.tscn`, `star_counter.tscn` | The original star art and number form a control below-left of energy, outside the power strip. The Regent's energy counter is raised. Zero is red/desaturated; gains use a 0.1 s damped count and 0.2 s brightness change; losses update immediately. Once another character gains stars, the counter remains visible for that combat. |
| Star rotation | `NStarCounter._Process` and the three textures in `star_counter.tscn` | The v22 pack adds the original base and two independent layers. They rotate at 30/60 degrees per second while stars are available, and 5/10 at zero. Older packs retain the original composite image. |
| Resource hover tips | `NStarCounter.OnHovered`, `NEnergyCounter` and original `static_hover_tips` | Energy and star controls show the original localized resource descriptions. Clicking stars no longer opens a fabricated power entry. |
| Intent disappearance | `NCreature.AnimHideIntent`, `NIntent._Process` | Preserve the old intent for 400 ms and fade for 500 ms. The fade does not extend command execution. A new reveal replaces the old fade. Retain the original slow bob, scaled for 320 px. |

## Display and save boundaries

The authoritative core still resolves synchronously. No gameplay command,
effect operand, RNG stream or save field was changed. The display observes the
existing `SharedAttackStart` / `SharedAttackEnd`, repeated-hit wrappers and
enemy attack boundaries. A per-depth attack clock preserves auto-play nesting
and sub-frame time; skipped hits against defeated actors do not add a phantom
impact. Existing bounded health/status queues still converge to canonical
values at their endpoint if their intermediate capacity is exceeded.

The effect's presentation boundary is established before its synchronous
hooks run, and the settled values are sampled afterwards. A retained stage-2
reproduction showed Lagavulin waking at input tick 0 while its health impacts
were still queued for ticks 5 and 16. The optional `CoreObserver.before_effect`
callback fixes that ordering for Wake and Flutter's StunTrigger without
changing saved effects or the existing four observer callback fields.

Power feedback stores at most 96 compact `(actor, power, identity, time)`
records. The records can outlive an action's value projection, but are cleared
on presentation reset and combat-identity changes. No title strings, frame
buffers, full State copies or persistent gameplay data are added. Original
power classification is a compact static byte table. Original `Minion` and
`Illusion` suppress power-applied VFX; star, pet and orb resource slots are not
power popups.

The first ARM link exposed unavailable floating-point helper symbols in the
star count animation. `StarCounterMotion` now evaluates the same damped
trajectory with Q14 value/velocity and Q30 reciprocal precision, using the
exact integer form of the original cubic denominator. It adds no software
floating-point runtime. Integer labels are checked against original-float
golden trajectories, including large gains and short/long frame intervals.
All labels match at the tested 16, 17, 33, 50, 100, 250 and 1000 ms intervals.
An artificial 1 ms cadence and a 9999-star gain accumulate a maximum
0.025879-star difference from float rounding; only values near a half-integer
may briefly round to an adjacent label. Losses and zero remain immediate.

After a complete initial snapshot, status capture scans the player and the
currently allocated enemy slots rather than all eight possible enemies on
every effect. Removed slots remain allocated; summons extend the bound and
slot replacements are still sampled. Player resources and auxiliary facts
keep their complete sampling. This removes absent-slot comparisons without
adding memory or omitting any live status.

## Deliberate remaining scope

The effect observer exposes settled state rather than the original
`PowerIncreased(..., silent)` event. Increasing visible power amounts and
negative-capable debuffs are projected; removals and ordinary duration
decrements do not create an application popup. Exact distinction between a
silent amount increase and a normal application still requires a dedicated
core presentation notification. This implementation does not claim that
every non-attack command has its original wait.

The star-gain particle scene, the power flash shader and all 15-fps intent
sprite animation frames are not yet ported. Power feedback uses the original
available icon art, scaled to the original VFX footprint, with a modest text
size adaptation for the calculator. The existing damage-number renderer now
receives each individual health impact; a separate overlapping-number VFX
queue is still a later task.

## Verification

`world_ui_regression` exercises actual Twin Strike, Dagger Spray, Sword
Boomerang, Celestial Might and Refract commands in Normal and Fast modes. It checks
each impact, all-target simultaneity, per-hit health loss, and byte-identical
saves against direct core execution. Lethal first hits skip the unexecuted
remaining attack. Chomper and Axebot exercise repeated
animation versus animation-once enemy attacks. Further checks cover the
power popup's application time/type, intent delay/fade boundaries, the star
resource's deferred value and absence from the power list, and reset cleanup.
Wake and Flutter's stun must retain their prior animation and HP until the
same damage boundary, in both speed modes, with byte-identical canonical
saves. Positive Strength decreases, negative Strength and ordinary duration
ticks check the original counter-versus-duration popup distinction.

The v22 pack relocation test checks all newly appended assets while retaining
the existing image, animation, locale and decoded-cost entries. Source is
checkpointed remotely before the longer builds and real-pack verification;
results belong in the main work log after those checks finish.
