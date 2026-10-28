# Original enemy death and revival boundaries — 0.17

This stage follows source15 `7225c880decfb7ff635f960d03ebecd4b0cdd1e2`.
Reference remains the owner's pinned v0.111.0 / `41cef1ea` installation.
The findings below come from its private original method contracts and bounded
execution against the frozen source15 native library; no original source or
assets are included here.

## Executable coverage and its limits

The current registry has 107 original solo combat monster models, the Architect
display model and the separate legacy Nibbit compatibility model. Its 87 combat
encounters have an existing regression that executes up to ten complete turns
at ordinary and deadly ascension, with save/load continuation checks: 174
encounter/ascension cases. This is executable encounter coverage. It does not
exhaust every random move branch, card interaction or original-game replay.

This stage adds targeted source-derived checks at death boundaries that the
turn-only coverage did not expose.

## Shared powers must follow the original death policy

The pinned `IllusionPower.ShouldPowerBeRemovedOnDeath` removes ordinary debuffs
and preserves buffs and temporary-power wrappers. `Creature.RemoveAllPowersAfterDeath`
also honors powers that explicitly survive owner death, including Adaptable,
Reattach, Steam Eruption, Painful Stabs and Minion.

The previous enemy cleanup covered the native enemy power directory, but skipped
shared character/potion fields for illusions. A bounded source15 probe killed
and revived an Eye With Teeth after applying those powers through the production
effect dispatcher. Poison, Doom, Demise, Shrink from Beetle Juice, Strangle, Hang,
Oblivion, Sic 'Em and Conqueror incorrectly remained. Weak, Vulnerable and
Debilitate were already cleared by the native power path.

A second probe killed and revived Test Subject with temporary Strength loss.
Strength itself correctly became zero, but the shared restore total and
Dark Shackles wrapper remained at three. The next enemy-end restoration could
therefore grant Strength that the new form should not receive. Shrink and
Conqueror also remained in that case.

Cleanup now applies to these shared fields as well as the native directory.
Removing Strangle or Oblivion also discards its captured per-play listener
values. Illusions preserve both their native buffs and the individual temporary
Strength wrappers; other death policies clear the shared wrapper amounts and
restore total. This changes no saved field, codec version or random stream.

The new regression covers Eye With Teeth and Parafright, plus Test Subject,
Decimillipede and Waterfall Giant. It checks the twelve shared debuff categories,
positive powers, two distinct temporary Strength wrappers, actor identity,
gameplay RNG and a save taken between lethal damage and the queued death.

## Last-primary death must execute minion death hooks

The pinned `CreatureCmd.KillWithoutCheckingWinCondition` kills surviving
secondary teammates when the dead primary creature has no surviving primary
teammate. Those are real deaths, including `AfterDeath` listeners. A minion's
classification itself survives death.

The previous engine entered rewards while leaving those minions alive. The
same frozen-library probe produced these results:

| Encounter | Living minions before leader death | Living minions in rewards | Gremlin Horn energy | Melancholy cost change |
| --- | ---: | ---: | ---: | ---: |
| Fogmog | 1 | 1 | 1 | -1 |
| The Obscura | 1 | 1 | 1 | -1 |
| Fabricator | 2 | 2 | 1 | -1 |
| Ovicopter | 3 | 3 | 1 | -1 |

The new path queues secondary deaths after death-triggered replacements have
been created. It retains the original creature ordering and uses the existing
saved death operations. Pending death flags keep the encounter open until those
listeners finish. Minion powers remain attached after death. Illusion minions
still run their ordinary death/revival hook; their zero HP allows the encounter
to finish once no primary remains.

The regression covers all four encounters, their exact death count, Gremlin Horn
energy and Melancholy cost changes, and save/load at the leader's pending death.
A separate negative case confirms that another surviving primary prevents the
cascade until that creature also dies.

## Validation status at source handoff

The two changed C++ translation units passed syntax checks. The new shared-power
regression linked against the unchanged source15 library fails on the first
illusion's retained Poison, providing a pre-fix regression check. The independent
bounded probes reproduced both defects with AddressSanitizer and
UndefinedBehaviorSanitizer enabled; their raw logs remain local.

The corrected source still requires its own pushed/read-back checkpoint and
frozen native execution. Run the complete `enemies_regression`, related
character/potion/save suites and target builds after that checkpoint. No new
full-suite, original-PC parity, UI-fidelity percentage or device-performance
result is claimed by this source handoff.
