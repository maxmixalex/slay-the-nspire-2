# UI4: additional original enemy command timing

Reference: the privately supplied v0.111.0 / `41cef1ea` assembly. Original
assemblies and decompilation are not distributed with the port. This record
summarizes the inspected commands and the port's display-only timing changes.

## Audited waits

The two values in each cell are the visible effect time and command completion
time, measured from that enemy's action start, in milliseconds. These are
command waits, not the complete length of a Spine animation.

| Model and move | Normal | Fast | Visible effect |
| --- | ---: | ---: | --- |
| Leaf Slime S: attack | 150 / 350 | 75 / 175 | Damage |
| Leaf Slime M: attack | 150 / 350 | 75 / 175 | Damage |
| Leaf Slime S: Goop | 500 / 1500 | 250 / 1250 | Generated Slimed |
| Leaf Slime M: Sticky | 1000 / 2000 | 250 / 1250 | Generated Slimed |
| Damp Cultist: Incantation | 950 / 1200 | 475 / 575 | Ritual |
| Damp Cultist: Dark Strike | 200 / 400 | 100 / 200 | Damage |

The inspected original `TriggerAnim` caps its Fast wait at 250 ms and otherwise
halves it. Attacks add the original 200 / 100 ms damage-command tail. Damp
Cultist's cast and custom wait precede Ritual application, which adds its own
250 / 100 ms visible power wait. Generated Slimed cards enter from no existing
pile: their preview's lifetime does not turn into an extra card-flight wait.
The generated-card command itself waits 1000 ms in both speed modes.

## Resulting behavior

`enemy_command_timing.hpp` holds the shared audited values. Enemy scheduling
and health/status feedback consult the same table, then quantize to the port's
existing 33 ms presentation clock. A long animation can continue after its
command has completed; the following enemy does not wait for that visual tail.
Neither canonical enemy settlement nor State, saved RNGs or save format changes.

The actual UI regression covers both speed modes, all three models and both
moves, including impact boundaries, next-enemy scheduling, retained animation
tails and exact saves against direct core settlement. The first combined run
later failed at the separately recorded Toasty Mittens fixture. Final frozen
9a554fe runs passed the complete actual-resource world-UI suite in both
languages (19,016 checks each), and Actions 39994866228 passed all 74 native
ASan/UBSan suites. See `validation-ui4-runtime-0.17.json` and
`validation-ui4-combat-motion-0.17.json`; the earlier failure is retained.

Other enemy models retain their previous scheduling. Multi-hit intervals,
hook-dependent waits and individual card VFX need their own original-command
audit; this table is not a claim that all battle timing is now original-exact.
