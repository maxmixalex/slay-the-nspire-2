# UI19 ordinary-device acceptance and compression gates

This report records the actual calculator-emulator evidence for the UI19
feedback fixes and resource optimization. The UI implementation and native
regressions are described in [UI19 UI feedback](ui19-ui-feedback.md); the
combat/save implementation has its own report. The resource pipeline report
owns the complete image/locale equality proofs and packaged byte counts.

## Runtime and measurement identity

The final production runtime used for every resource-format comparison is
`090023d45ec6211da3b2e5a887cfcc2d1dd68004`. Its ordinary Zopfli Ndless launcher
is 1,724,027 bytes, with SHA-256
`fd40e3badd58366a1b48f9525daa4ef92709510b0c7d89756b0b87349dfd1d52`.
It was not rebuilt or instrumented between format-42 and format-44 samples.

The device environment is the unchanged Firebird controller/core configured
as a TI-Nspire CX II CAS, OS 6.4.0.74, 396 MHz, Ndless revision 2022. Timings
come from the ordinary launcher's foreground SP804 counter at 32,768 Hz.
Each dataset/scenario has one sample, except the final Merchant's two
samples. These are emulator measurements, not
physical-handheld measurements or a claim about every room and encounter.
Scene FPS is the number of recorded frame intervals divided by their total
duration. A scene's maximum frame includes synchronous cold work within that
scene. Preparation, startup and the visible scene remain separate categories.
Resource-read timing can be nested inside update or render timing, so those
breakdowns are not additive categories of total elapsed time.

The first-process startup metric covers selected-resource validation and
App initialization. It does not cover OS boot, the Ndless loader before main,
or all time until a combat room becomes playable. The first runs after
ordinary environment recovery have different OS-cache histories and are not
used to claim a compression-caused cold-start speedup.

## Ordinary behavior checks

The initial UI launcher `3fd43e9` was installed and read back byte-exactly
against original format-42 resources. Its actual 320x240 LCD captures show
the Smith checkbox separated from the clipped card viewport, the raised
treasure title, the left-shifted Continue popover, the complete version label,
and the compact defeat summary with a stable outgoing quote. Independent
native English and Chinese captures cover Random's original unknown HP/gold,
larger introduction padding and the centered floor-11 field. The grid clip
also covers enlarged/hovered cards and their hit regions.

The integrated ordinary launcher `8411656` then passed the behavior flows
with the original resources. A saved Cosmic Indifference selection resumed
through Continue; the selected Wound flew left into Draw, settling at Draw 6
and Discard 2. Save and Quit visibly faded out and back into the title. After
an OS exit and immediate ordinary relaunch, with no USB operation between
them, Continue restored the same pile counts. Osty stood left of the enemy
and his body hover displayed the original Die for You text.

Terminal checkpoint deletion was tested through normal enemy damage. A
production-codec 1-HP Ironclad fixture was loaded through Continue and a
normal End Turn let the enemy kill the character. The title no longer
offered Continue. A subsequent OS directory listing contained the progress,
profile marker, language and performance files, but no active, backup or
temporary run save. Progress remained present. The test never used console
`die`; console state deliberately detaches from an existing checkpoint and
cannot prove this behavior.

The final `090023d` ordinary launcher was also inspected with the real
forged/enhanced Sovereign Blade. Its tooltip showed 47 damage, Sharp 7 and
Retain; the sword stayed fixed while the pointer held it for a further
1,000 ms, then resumed its orbit when the pointer moved away. Native tests
separately cover attack, forging, continuous resume and pointer cancellation.
The first synthetic checkpoint intentionally has timestamp zero, so its
initial Last Saved value is blank. A real Save and Quit subsequently writes
and displays the actual timestamp; the device capture shows 2026-10-30 11:11.

These fixtures are valid production NSR/NSH saves and are loaded by the
ordinary title flow. They are controlled scenario evidence, not a claim of
a naturally completed three-act run.

## Why the first smaller candidate was rejected

The first full-pair format-44 candidate passed all image and text equality
checks, and an ordinary single-sword combat ran at about 29.98 FPS. Additional
cache-risk scenarios exposed a real three-enemy regression. An equal-image
proof and a smooth simple scene do not establish the performance of a larger
working set.

All rows below use the same final ordinary runtime and the same saved scene
and game inputs. Bowlbugs and Decimillipede share game-input SHA-256
`eb161b4c640cbfcce59d655e1b51ba73c28353a82e4215f4cf229a53bcaf719f`.
Merchant uses
`443eec03a2b944d8a4e8738298cbe98bcf90f175c95ae6508720e06dc28eaa90`.

| Scenario | Original 42 FPS | Rejected full-pair 44 FPS | Original max frame | Rejected max frame | Original scene reads | Rejected scene reads |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Bowlbugs, 3 models and one enemy turn | 27.4491 | 17.3680 | 439.026 ms | 907.196 ms | 52 / 1,455,451 B | 208 / 12,343,303 B |
| Decimillipede, 3 segments and one enemy turn | 29.4261 | 29.3967 | 406.433 ms | 401.001 ms | 17 / 190,957 B | 16 / 227,197 B |
| Merchant, closed/open/closed/reopen | 28.0458 | 25.7294 | 1,213.257 ms | 1,472.626 ms | 260 / 1,488,562 B | 148 / 1,656,761 B |

Bowlbugs lost 36.73% of its measured FPS and requested 8.48 times as many
bytes. Frames above 250 ms rose from 6 to 30. This was a new compression/cache
regression. Decimillipede remained similar. Merchant already had a substantial
first-stock cold cost in 42, but the candidate added 259 ms to its maximum
frame and reduced scene FPS by 8.26%. That additional cost also required a fix.

The first correction preserves original 512-KiB dictionary boundaries for
all Bowlbug-family, hero and Osty tracks, retaining the larger groups for
most other assets. On the actual ordinary device, this 37-pair protection
restored Bowlbugs to 1,069 frames / 27.6114 FPS, a 425.232-ms maximum and
55 reads / 1,435,168 bytes. It recorded 14 frames above 100 ms and 6 above
250 ms, with zero resource failures. The original 42 sample was 1,067 frames /
27.4491 FPS, 15 and 6 such frames. The correction therefore removed the
observed working-set regression instead of merely accepting lower FPS.

Merchant's independent retained-image issue required limiting retained groups
to one 128-KiB decode page. That intermediate combined candidate recovered its
maximum frame to 1,200.165 ms and its frames above 100/250 ms to 11/1; the
original had 1,213.257 ms and 10/1. Read traffic fell to 1,299,994 bytes, but
average FPS remained 27.1026, 3.36% below the original sample. This residual
was recorded instead of equating reduced I/O with equal rendering speed.
The final data policy keeps those bounded pages and encodes the 97 Merchant
body groups with supported RGR2, avoiding their additional plane-decoding CPU
cost at a measured 92,820-byte storage cost.

A separate host test of Construct's first charging/attack transition found
32.5 MB of raw dictionary decode work against the original's 7.78 MB. That
specific observed risk added Cubex/Punch-family group protection and one
ordinary device functional/performance check. There is no older format-42
device FPS sample for Construct, and no such device comparison is claimed.

## Final refined resource policy

The final converter policy is source `171f30b`: 59 protected tile pairs,
including the Bowlbug, hero/Osty and Cubex/Punch families, plus the bounded
Merchant groups described above. Its selected English logical pack is
69,923,632 bytes, SHA-256
`775cc5895dbc59fdf4c79b53d78854c3de9e488e659caeb0e530c2fe3dbc26b4`.
The frozen English resource manifest has SHA-256
`263036d36ca971b447912e9550b8356cb235589603fc2f49cbf5b6b388f70cbc`.
The nine common files and English companion total 68,361,944 installed bytes.
All ten data files and the ordinary launcher were read back over USB with
exact size and SHA-256 matches. The index and eight named common parts retain
the existing installation layout. Package-wide/all-language byte counts and
the final all-image equality gates belong to the resource pipeline report.
After the all-16 export completed, `all16-device-english-identity.json`
independently matched those ten device files and the logical English pack
against the final manifest
`463582860bf9a354771cbfd2f9ac5044433b943bcdcc019f55d8a7c1efd569ab`.
The added language companions did not change the installed or measured
English data. The other fifteen companions are covered by host validation,
without a claim that they were all installed or visually reviewed on-device.

Every final scene below was opened through ordinary Continue and visually
reviewed. Each had zero resource-read failures. Values from the earlier
mixed candidate or rejected full-pair candidate are not substituted into
this table.

| Scenario | Final frames | Final FPS | Maximum frame | Scene reads | Frames above 100 / 250 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Bowlbugs, one enemy turn | 1,060 | 27.2301 | 491.577 ms | 59 / 1,615,991 B | 15 / 8 |
| Decimillipede, one enemy turn | 1,144 | 29.4265 | 405.212 ms | 16 / 189,026 B | 2 / 1 |
| Construct, first charge/burrow transition | 1,111 | 28.8794 | 395.691 ms | 29 / 626,642 B | 7 / 2 |
| Merchant, first final sample | 1,113 | 27.3321 | 1,322.144 ms | 161 / 1,245,945 B | 12 / 2 |
| Merchant, same-protocol repeat | 1,119 | 27.4347 | 1,345.398 ms | 159 / 1,224,844 B | 12 / 1 |

Bowlbugs is 0.80% below its single original-42 FPS sample, with a 52.55-ms
higher maximum; its earlier catastrophic 17.37-FPS/12.34-MB behavior is gone.
Decimillipede remains effectively unchanged. Construct's observed HP, block
and charging states match the production core: the hero remains at 80 HP,
Punch remains at 55 HP and gains 10 block, and the two 65-HP Cubex enemies
lose their initial 13 block as they emerge. There is no claimed format-42
device FPS comparison for that encounter.

Merchant was repeated once to retain evidence of its residual difference.
Both final samples are reported, with no selection of the better sample.
Together they contain 2,232 frames over 81.509 seconds, or 27.3834 pooled FPS,
2.36% below the original's single 28.0458-FPS sample. Their maximum frames are
1.322 and 1.345 seconds, compared with 1.213 seconds originally. Final scene
read traffic is lower, but the observed provider/seek/read time is higher;
reduced requested bytes are not evidence of an equal cold-load time. These
samples do not establish a causal explanation for the remaining difference
or statistical confidence bounds. The storage/performance tradeoff is
retained in the final report, not described as zero regression.

The final warm protocol exactly matches the accepted format-42 input hash
`3a6b276bd39faae5ef91e3351c65e49da2fba4ec1ec9b08ab360d23d6bed7308`.
A primer process saves and exits to the OS; the measured process immediately
reopens without any intervening USB operation, continues the same sword
fixture, then displays Random and Ironclad for five seconds each.

| Warm-flow metric | Original 42 | Final refined 44 |
| --- | ---: | ---: |
| Measured startup scope | 753.967 ms | 680.176 ms |
| Combat frames / FPS | 726 / 29.9821 | 727 / 29.9819 |
| Combat maximum frame | 54.352 ms | 54.504 ms |
| Combat resource reads | 0 | 0 |
| Character frames / FPS | 293 / 21.8565 | 293 / 21.8398 |
| Character maximum frame | 736.603 ms | 797.241 ms |
| Preparation total / maximum | 3,039.246 / 250.610 ms | 3,029.968 / 189.789 ms |
| Title FPS | 26.7803 | 26.6743 |
| Overlay FPS | 16.0659 | 16.6060 |

The approximately 30-FPS warmed combat is preserved. Preparation's maximum
frame decreases, while character selection's maximum increases by 60.64 ms;
its average is essentially unchanged. The faster character-selection result
from the rejected full-pair candidate is historical evidence only.

The machine-readable `final-pair-summary.json` and
`corrected44-risk-comparison.json` retain the exact raw-performance paths and
hashes, input hashes, fixture hashes, runtime identity, per-scene totals and
all comparison limits. `full-pair-risk-comparison.json`,
`mixed44-bowlbugs-correction.json` and `combined44-risk-comparison.json`
preserve the reasons for the intervening decisions. All chosen ordinary
flows are complete; no extra scenario or optimization candidate follows
these measurements.

## Setup failures and excluded measurements

All accepted measurements require both a successful journal and an inspected
game framebuffer. A successful controller process alone does not establish
that the OS opened the game.

Some NavNet operations stalled or left a delayed Sent Document notification
over My Documents. Early fixed key sequences then operated on that OS dialog
instead of the game. Those attempts and any stale performance file were
excluded. A fresh unchanged controller connection restored ordinary USB
readback. The accepted warm sequence acknowledges the visible notification,
then leaves USB untouched between its primer process and measured relaunch.

After a large resource replacement, one ordinary launch displayed the OS
Unsupported Document Format message before entering game main. All installed
files and the ordinary launcher read back byte-exactly. A normal OS restart
and the original matching Ndless installer restored the normal launch hook.
No emulator or launcher patch was introduced for recovery; the existing
boot-ROM, controller/core and CPU/memory/scheduler setup was reused unchanged.
Later setup keeps the normal reset, installer, fixture upload and first
ordinary launch in one continuous controller connection; installation is
outside the measured process.

The writable working flash was moved to a task-local temporary directory to
avoid workspace synchronization creating large transient duplicates. Its hash
was preserved. Snapshot relocation changes only the standard flash-path
header; the decompressed payload after the 1,032-byte header was verified
unchanged. Only one canonical writable NAND/controller is used. Large USB
readbacks are hashed and deleted one at a time by the receiving process.

The validation archive includes the final comparisons, raw performance JSON,
input/fixture hashes, controller commands/events, frozen run journals and
selected LCD captures. It excludes NAND images, ROMs, emulator snapshots,
private save fixtures and resource packs. Source stages remain local commits:
automatic approval review rejected publication of the accumulated source
history, and no remote publication/readback is claimed.
