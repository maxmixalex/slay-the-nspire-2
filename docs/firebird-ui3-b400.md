# Firebird UI execution at b400 — 2026-10-23

This is execution evidence for source
`b400ab88186ddb618167a8b7c2a8c141d41629e0`, the 977,168-byte ARM launcher and
the first v20 hybrid resource set. It predates the added title glow and the
subsequent hand presentation correction. It is not final acceptance of those
later changes or a physical calculator performance result.

## Ordinary installation and interaction

The owner's original CX II NAND was kept unchanged. A separate writable copy
booted OS 6.4.0.74 in upstream Firebird, built with Clang 18. The original core
and OS were not modified. The command frontend uses normal keypad/touch and
USB APIs; its scheduling is quantized to the emulator's 10 ms periodic tick.
Ndless r2022 was transferred and opened normally. The LCD reported successful
installation. Existing unrelated files were removed only from the copied NAND
to make room for the game. One initial nonempty-directory removal failed; its
setup journal remains failed, rather than being relabeled as a game result.

The launcher and all three matching common/English/Chinese files were copied
by USB. English was selected through the bilingual language screen. Executed
screens include the original title, eight-card compendium page, moved card
focus, card detail and Block keyword tip, Ironclad selection, Neow's Transform
keyword tip, ten-card transformation selection, original Proceed, map and an
ordinary Crocodile fight. The captured `collection-eng-page2` name is inaccurate:
that frame still shows page 1 and is not evidence of a page change.

In combat, ordinary S save and O load restored the original five cards,
3/3 energy and zero block after playing Defend. Two End Turns were executed.
The crocodile's attack pose overlaps the visible Enemy Turn banner. The run
then used pause → Save and Quit; the title Continue panel showed floor 2 and
67/80 HP. Title Quit returned to the OS document list, still present after a
seven-second simulated wait. The 230,512-byte save, language selection and
runtime performance record were retrieved by normal USB reads.

**A functional visual defect was found:** the next player's hand appeared
during the Enemy Turn banner while the old energy and deferred enemy impacts
were still displayed. The canonical turn was already resolved, and the hand
renderer bypassed the health/status presentation clock. That finding is the
reason for the following compact hand-cue correction; this run is not a pass
for hand timing. A complete battle, Chinese UI flow and final title glow still
need their own execution evidence.

## Runtime measurements

The existing foreground SP804 profiler ran at 32,768 ticks per second inside
Firebird. These are emulated timer measurements for this actual route, not host
wall-clock timings or physical-device frame rates. The startup field describes
the selected resource open and App initialization; it excludes OS/Ndless boot,
USB transfer, language selection and the first rendered frame.

| Measurement | Observed |
| --- | ---: |
| Selected resource open and App initialization | 14,649 ticks / 447.1 ms |
| First rendered frame | 11,464 ticks / 349.9 ms |
| First presentation | 7 ticks / 0.214 ms |
| Requested resource bytes over this route | 2,652,899 |
| Resource read callbacks / seeks / failures | 218 / 184 / 0 |
| Integrity phase samples | 0 |
| Eager image / tile-index / glyph / string phase samples | 0 / 0 / 0 / 0 |

The selected virtual English pack was 76,521,355 bytes. The resource record
contains directory and actual scene reads; the zero pre-scan counts above
confirm that the removed whole-pack validation paths did not run.

## Journal scope and limitation

`ui3-firebird-build-a76847b70f8f` and
`ui3-firebird-os-boot-8a6431d9af8d` are closed passed journals.
`ui3-firebird-interactive-861876939c8f` is the intentionally retained failed
setup run described above. `ui3-firebird-game-acceptance-4cd596512d04` closed
with return code 0 and unchanged source.

The long interactive run's events file and captured standard output stop at
83.61 simulated seconds, although the later screenshots, returned save/perf
files and final OS snapshot were written. The cause of that logging loss has
not been established. The closed wrapper journal therefore does **not** prove
a complete later command transcript. The later visual and file results are
reported individually above. Final corrected-game acceptance will use shorter
closed command batches with their own complete logs, keeping this limitation
visible rather than treating this journal as an end-to-end pass.
