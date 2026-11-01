# UI24 Necrobinder and Osty animation repair

## Two source-confirmed defects

The actual full-resource calculator run reproduced the white printed slash
placeholder during a normal Strike Necrobinder. The original Necrobinder scene
assigns a separate ShaderMaterial to its `slash` SpineSlotNode. The actor sampler
previously respected SpineSprite blend overrides but ignored this slot material,
so it baked the placeholder atlas into the attack. This is a converter defect;
replacing only the launcher cannot repair existing artwork.

`actor_slot_materials.py` evaluates the authored shader after triangle/UV
interpolation, retaining source clipping and draw order. It reads the original
material uniforms and noise textures, uses the unbound mask sampler's white
default, and replaces the placeholder color/alpha as the shader does. The pinned
attack event starts a 400 ms Quad/In step tween and a 450 ms Quad/InOut opacity
tween. No actor frame count, timestamp, raster density or cache capacity changes.
The converter identity includes the new material and its scene/shader helpers.

The ordinary Offering self-damage reproduction exposed a separate owner/pet
lifecycle defect. The original owner death kills surviving Osty after death
prevention has failed, and game over retains the combat creatures above its
backstop. The port left Osty alive after direct owner death and then omitted it
from the game-over renderer. `death_cleanup` now performs that companion kill
without inventing a second damage event. Combat and combat game over share the
existing Osty draw path, including its die clock and subsequent dead loop.
Non-combat rooms do not acquire a pet visual from this change.

## Executed calculator verification

The actual runtime was built from `b690c582430eb77a97089eeb336dfabf939f30e4`,
published source-equivalent checkpoint
`0e7d6760224e60ab0b87e82ffe7f4932112d1710`. The ordinary release launcher is
1,733,987 bytes, SHA-256
`8db5f9cec3f06b56a41b4bb189bc965ed72d4e0c96aa24c624b53d8c6aeff47b`.
Its formal allocation is 22,286,408 bytes under the unchanged 22,291,016-byte
limit. It has no diagnostic rendering or enlarged heap/cache pool.

Both tests used a private full-resource or reduced-resource candidate derived
from the preceding formal edition, replacing only the Necrobinder attack
artwork/registration. These are explicitly test candidates, not the final
all-locale converter release. The full candidate's eleven resources total
68,183,203 bytes. The final producer and complete ZIP budget remain separate
acceptance gates.

The supplied ordinary 64 MiB Firebird used its existing 396 MHz configuration
and guest 32,768 Hz timer. Launcher and all eleven resources were transferred by
ordinary USB and read back byte identically. Snapshot/NAND pairs stayed in one
private VM path; no header or guest RAM was altered. The action captures have
the console closed and identify the actual Necrobinder/NIBBITS_NORMAL scene.

| Executed scenario | Result |
| --- | --- |
| Full normal Strike Necrobinder | Printed white placeholder gone. A 33 ms capture sequence shows the blue rising arc, complete perforated arc and falling arc at 198/231/297/330 ms after the recorded input boundary, followed by the authored fade. |
| Full normal Offering at 1 HP, living 1 HP Osty | Both deaths animate; Osty remains visible above the game-over backstop and in its corpse loop through the 10.1 second capture. |
| Fairy in a Bottle, normal Offering | Player revives to 19/66 HP; Osty remains alive at 1 HP. |
| Normal Sacrifice, then normal Offering | Osty first dies in combat; later owner death retains the existing corpse without revival or replaying the beginning of death. |
| Non-combat Shop death | The merchant-room character dies at its room origin. No Osty is introduced. |
| Reduced normal Strike and Offering | Start, intermediate and terminal poses remain distinct; both corpses remain after game over through 10.1 seconds. |

The public material/raster/reduced-pose Python checks passed. Clang 18 native
checks passed for the core companion/revival rules, 8,325 synthetic world-UI
checks, 4,175,314 actual-art player-death checks, and 1,814 full/reduced profile,
clock and cache-alias checks. These counts include repeated assertions. They
are not original-application differential coverage or physical-device timing.

## Reduced action timing

All 1,783 established clip directories retain frame counts, clocks and pivots.
The actual candidate tables additionally verify unchanged source registration
and exact aliases to each held reduced pose for the following critical tracks:

| Track | Established samples | Retained indices | Terminal rule |
| --- | --- | --- | --- |
| Necrobinder attack | 12 at 83 ms | 0, 5, 11 | Final sampled pose starts at 913 ms; 996 ms directory duration remains. |
| Osty die | 17 at 78 ms | 0, 7, 16 | Final sampled pose starts at 1,248 ms; dead loop follows the 1,326 ms duration. |
| Necrobinder die | 71 at 33 ms | 0, 34, 70 | Game over uses the original 2,333 ms endpoint selector, not rounded directory duration. |
| Osty dead loop | 3 at 56 ms | 0 | Reduced mode holds its source pose; full mode retains the loop. |

Full and reduced historical color encoding is not asserted byte identical.
The timing proof compares exact reduced held-pose aliases and source
registrations; it does not infer clock equality from visually similar sprites.

## Preserved failed attempts

The first native GCC 13 build hit its existing `gimple_add_tmp_var` internal
compiler error in `world_relics.cpp`; the successful build used Clang 18.
The first synthetic owner-death test called a world-only console command from
a non-world fixture. Test-only `a5a70ead8a40e6987a4b88030554dffe32307cc7`
uses the observed core death operation instead; published checkpoint
`52dafda45718d2f239199a4636370d2f0e9e59d9` contains that correction.

An initial USB overwrite of the 26 MB enemy file failed, with readback detecting
the old bytes. Ordinary USB removal followed by retransfer resolved it; all
final files match. Directly reducing the full candidate correctly rejected its
animated water material. The reduced test therefore starts from the matching
formal reduced edition and preserves its original-source material poses. An
initial private patch omitted its NAF1 profile marker; the profile test rejected
it, and the marked candidate passed before any reduced guest installation.
Failed journals remain preserved and are not counted as successful acceptance.
