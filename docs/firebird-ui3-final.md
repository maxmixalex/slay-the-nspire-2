# Final UI3 Firebird execution — 2026-10-23

The final runtime at `ead747dcdffded08d641445a811af48a4fafd1de` completed
ordinary English combat, rewards, save and exit, followed by Chinese title
Continue, compendium, keyword tooltip, card detail, save and exit. The compact
hand presentation fix was also observed on the actual ARM launcher: discarded
cards disappear during the enemy turn, and the next player's cards appear only
at the player boundary. These are executed Firebird results, not a physical
calculator benchmark or a claim of complete original-game visual equivalence.

The machine-readable record is
[validation-ui3-firebird-0.17.json](validation-ui3-firebird-0.17.json).
The earlier [b400 report](firebird-ui3-b400.md) remains a separate historical
record of the defect that led to the hand correction.

## Environment and installed artifacts

Upstream Firebird was built with Clang 18. Its core and OS were unchanged. A
separate writable copy of the owner's CX II NAND ran OS 6.4.0.74; the original
NAND was not modified. Ndless r2022 was installed through normal document
opening. The command frontend uses only ordinary keypad/touch and USB APIs,
with a 10 ms simulated scheduling tick. No game state or emulator RAM was
injected to reach these screens.

| Installed file | Bytes |
| --- | ---: |
| ARM launcher | 983,682 |
| Shared common resources, v20 hybrid with title glow | 74,756,230 |
| English locale | 1,780,887 |
| Chinese locale | 4,563,586 |

All three matching resource files and the final launcher were installed by
normal USB transfer. The hybrid policy retains individually encoded common
cards/UI and groups other retained pictures; this execution did not use the
earlier all-group candidate. Original PCK/NAND, resource packs and captured game
images are private artifacts and are not included in the public source tree.

## Observed behavior

The English title displayed the original gold lettering, cyan II, fire and
authored glow. Continue loaded the existing Ironclad run. After End Turn,
captures at 180, 580, 980 and 1,580 simulated milliseconds showed no prematurely
drawn next-turn cards. At 1,580 ms the Crocodile's action overlapped the visible
Enemy Turn banner. This particular action was a buff, so it is not reported as
an attack. The later player-turn capture showed the new five-card hand.

Ordinary card plays and further turns completed the fight. The reward screen
showed 19 Gold and a card reward. Gold increased from 99 to 118, Bloodletting was
chosen, and the empty reward screen displayed Proceed. The hero had 61/80 HP.
S save and pause Save and Quit returned to the title; its Continue panel agreed
on floor 2, 61/80 HP and 118 gold. Title Quit returned to the OS document list,
which remained visible after a seven-second simulated wait.

The first Chinese batch displayed the localized title and compendium, used O
to load the same reward state, and returned normally to the OS. Its early
screenshots were named for intended stages before the slower OS/application
launch had reached those stages. They are not evidence of a captured loading
screen or title Continue. A separate final confirmation therefore booted the
working NAND, installed Ndless normally, waited for the Chinese title, and
pressed Enter on **Continue**. It restored the same empty reward screen.

That final confirmation then opened the eight-card compendium, moved focus
from Strike to Defend, displayed the Chinese Block keyword tooltip, and opened
Defend's Chinese card detail. The focus, tooltip and selected card agreed.
Save and Quit returned to the Chinese title, and title Quit returned to the
OS document list after another seven-second wait. Normal USB reads retrieved
the performance report, save and language preference.

The English save and both Chinese saves were each **230,520 bytes and directly
byte-equal**. The preference was `NSL1zhs\n`. This confirms language switching,
viewing the compendium, Continue/load and save/exit did not change this run.

The actual ARM deck had no Retain card in these turns. Retain is covered by
the final native `world_ui_regression` against both real locale packs:
16,229 checks per language, including actual End Turn with Purity, Normal/Fast
timing, discard/draw boundaries, focus/detail/tooltip agreement, cue overflow
and direct-core save equality. ARM empty-hand timing and native Retain timing
are deliberately reported as different pieces of evidence.

## Performance and remaining delay

The optional bounded profiler used the emulated foreground SP804 clock at
32,768 ticks per second. The values below are neither host wall time nor
physical-device measurements. Startup means selected resource open and App
initialization; it excludes OS/Ndless boot, USB, time on the language selector
and the first rendered frame. The historical JSON field name
`selected_resource_validation_and_app_init` does not mean an integrity pass
still runs.

| Measurement | Final English route | Final Chinese confirmation |
| --- | ---: | ---: |
| Resource open and App initialization | 503.0 ms | 778.9 ms |
| First rendered frame | 445.2 ms | 884.8 ms |
| Resource read callbacks | 157 | 319 |
| Requested resource bytes over the route | 2,283,628 | 2,548,915 |
| Resource seeks / failures | 137 / 0 | 289 / 0 |
| Integrity samples | 0 | 0 |
| Eager image / tile-index / glyph / string samples | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 |

The English and Chinese routes have different scenes and lengths; their total
read counts are not a controlled locale A/B comparison. English combat
recorded 929 render calls, averaging 19.1 ms. The final Chinese title recorded
136 render calls, averaging 33.1 ms. These scene means include transitions and
cannot be converted into steady-state physical frame rates.

**First entry to the Chinese compendium still has a substantial cold delay.**
Across 67 compendium render calls, the mean was **52.9 ms**, minimum **22.9 ms**
and maximum **1,646.8 ms**. The mean includes the cold entry, focus changes and
tooltip display; it is not a warm-frame average. The initial Chinese batch also
recorded a cold maximum of 1,888.2 ms, but only two compendium samples, so its
967.8 ms mean is not a useful sustained-render figure.

The render timer brackets `App::render` itself. Resource I/O, image/glyph
decoding or page preparation performed inside rendering are included. Input
processing and any preparation it performs are in the separate update timer;
the presentation interval additionally includes the game frame gate and work
between frames. Deliberate operator pauses in the command frontend do not
advance simulated time. The aggregated profiler does not identify per-frame
image/glyph/preparation contributions, so no unsupported decomposition of the
slowest frame is claimed. This cold multi-card delay remains a performance
follow-up, even though the UI flow and warm interactions are usable.

## Closed records and limitations

The final install, title, hand and ordinary battle batches are closed with
return code 0, matching ead source, and `source_changed: false`. The complete
bilingual ending is `ui3-firebird-final-bilingual-134ab78552af`; its event log
ends at 43,320 ms with `stopped: success`. The explicit Chinese Continue/detail
confirmation is `ui3-firebird-final-zhs-confirm-e6c01511ae2f`; its complete log
ends at 76,630 ms with `stopped: success`.

One intermediate emulator snapshot, `final-combat-eng-after7.snapshot`, could
not be restored. An incomplete or damaged snapshot is a possible explanation;
the exact cause was not established. The failed startup journal
`ui3-firebird-final-reward-ec06791f4683` is retained as failed. The later accepted
ending replayed the ordinary winning turn from the known-working after6
snapshot, with no intermediate snapshot dependency. The earlier victory image
is a real observation, but a successful wrapper did not establish that the
snapshot could be restored. The misleading file `final-enemy-eng-attack-1580.ppm` depicts the
already defeated enemy and is excluded as enemy-action evidence.

The incomplete long b400 event transcript remains disclosed in its historical
report. The final short batches have complete endings and do not inherit a
claim of transcript completeness from that earlier run. Final scope covers one
ordinary Ironclad combat and the named bilingual screens, not every character,
encounter or event. It does not close the remaining cold compendium delay,
unimplemented title background GPU stars/fog, or physical-calculator timing.
