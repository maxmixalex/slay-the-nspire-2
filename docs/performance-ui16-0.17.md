# UI16 runtime performance and allocation follow-up

This records the runtime work through `d4e41a8` on 2026-10-29. The restored
UI15 baseline is `49040d5`; its ARM launcher is byte-identical to the recovered
UI15 calculator launcher. The rendering/memory candidate used in the device
comparison is `4cf0019`. The later shared-State change is verified separately
below. Final integrated resource-package, scene-cache and device acceptance
belong to the UI16 integration report.

## Implemented changes

* `Canvas::paint` uses the ordinary RGB565 blend when an existing coverage
  pixel is already opaque. This is the exact source-over result and avoids
  three variable divisions. Partial-coverage pixels retain the general path.
* Unshaded affine generated/flying cards have a direct pixel loop. It retains
  the old per-row division/truncation, column steps, clipping, source alpha,
  compositing order and animated overlays. Glow, erosion, brightness/tint,
  color matrices, coverage and occlusion continue through the general path.
* Freestanding `memcpy` and `memset` copy/fill aligned words with a bounded
  head/tail. Differently aligned source/destination pointers stay on the
  byte-copy path. `memcmp` compares words for equality only, then compares
  the first differing unsigned byte; it does not order little-endian words.
  `memmove` retains its existing overlap handling.
* A saved title preview and a synchronous developer-console rollback share
  one `State`. Explicit saved-read, saved-ready and rollback ownership
  prevents a callback or second App from overwriting an active rollback.
  Console entry invalidates the old preview; closing it on the title
  reloads the real save. Returning to the title, profile changes and
  successful initialization already refresh the save. Failed reads never
  become merchant stock. The complete hot picture pool is unchanged.

These are production paths. The benchmark drivers and instruction probes
are verification, not additional gameplay features. No animation is skipped,
no original sprite is made smaller and no particle is removed by these changes.

## Calculator comparison

The restored Firebird device is a **CX II CAS at 396 MHz, OS 6.4.0.74,
Ndless r2022**, using the unchanged upstream core and a working copy of the
user-supplied NAND. It is a different CPU configuration from historical
132 MHz CX measurements; those FPS figures must not be put in the same
before/after comparison.

The fixture starts the real Haunted Ship turn whose ordinary End Turn
applies Weak and generates five Dazed cards into Discard. Both launcher
versions use the same fixture, matching progress and English resources.
For each version, two runs resume its same warm-combat snapshot: one exits
immediately; the other performs End Turn and runs a six-second action window
before following the same exit path. Subtracting the two ordinary performance
reports isolates the action window from earlier loading and warm-up.

| Counter in the paired action window | UI15 baseline | Rendering/memory candidate |
| --- | ---: | ---: |
| Frames | 166 | 168 |
| Recorded interval | 6.023285 s | 6.027222 s |
| Frames / recorded interval | 27.560 FPS | 27.874 FPS |
| Mean render time | 26.511 ms | 25.395 ms |
| Total render timer ticks | 144,206 | 139,802 |
| Frames over 50 ms | 5 | 4 |
| Frames over 100 ms | 4 | 3 |
| Frames over 250 ms | 0 | 0 |
| Resource reads / seeks | 19 / 19 | 19 / 19 |
| Requested resource bytes | 116,259 | 116,259 |

The timer frequency is 32,768 Hz. Mean render time falls **4.208%** in this
pair. The end-boundary frames differ slightly, so the result is a measured
small improvement in this scene, not a universal FPS claim. Maxima cannot be
subtracted; no difference of maximum counters is reported. The captured
screens include the actual Weak effect and five generated cards. Cold
startup and the later State/scene memory sharing were not included in this
rendering comparison.

Private evidence is retained as `baseline-action-difference.json`,
`candidate-action-difference.json`, paired ordinary OS snapshots, performance
reports and screenshots. It includes no emulator timing shortcut or patched
OS execution path.

## Exact rendering and CPU checks

The native status benchmark advances the real generated-card presentation
using a controlled move observation. It is separate from the ordinary
End Turn device flow described above. It asserts unchanged canonical saves
and compares the complete 320 by 240 RGB565 frame for identical input times.

| Executed check | Result |
| --- | --- |
| Canvas independent pixel oracle | 326,081 cases, 2,943,159 checks |
| Flying-card affine poses | 112 poses across seven scales, four angles and four clipped/visible centers |
| Actual English card/HUD regression | 699,418 checks |
| Actual Chinese card/HUD regression | 777,044 checks |
| Four alternating generated-card benchmark pairs | 21,840 matching complete-frame FNV64 pairs; process-CPU median change −9.621% |
| Eight CPU-affinity-controlled generated-card pairs | 14,560 matching complete-frame FNV64 pairs; median paired CPU change −9.275% |
| Random scene | 900 complete-frame hashes identical |
| Seven retained room scenes | 1,897 complete-frame hashes, saved states, cache counts and resource I/O identical |
| Actual merchant rendering | 19,395,167 checks, including 2,376 exact whole frames |
| Actual UI15 combat traversal | 96,046 checks, 1,312 legal move executions |

The eight native CPU pairs range from −22.544% to +4.908%; seven improve and
one regresses under host noise. Native timings do not establish calculator
FPS. The repeated exact-frame and save checks establish that the performance
path does not substitute a simpler visual result.

The unshaded card loop leaves the original overlays outside the direct
pixel loop, so the eight Affliction material phases, thirty Infection frames
and ten Ancient-fire frames remain the same production resources and clocks.

### ARM memory contracts

`tests/arm_memory_regression.py` runs actual compiled ARM code with the
ARM926 Unicorn model. It checks every arena read/write, bounds, alignment,
zero-length behavior, return values, guard bytes, callee-saved registers,
stack restoration and overlapping `memmove`. It flushes translated blocks
after attaching instruction hooks; otherwise Unicorn can miss already
translated code in its instruction count.

The final linked launcher passes 4,799 vectors per version. The actual
production freestanding object linked separately passes 9,908 vectors per
version, including `memcmp`; that unused function is garbage-collected from
the production launcher and contributes no claimed runtime gain there.

| ARM call | Baseline instructions | Candidate instructions |
| --- | ---: | ---: |
| 4,096-byte aligned `memcpy` | 16,388 | 3,344 |
| 4,096-byte `memcpy`, both pointers offset one byte | 16,388 | 3,369 |
| 4,096-byte `memcpy`, different alignment | 16,388 | 16,395 |
| 4,096-byte aligned `memset` | 12,292 | 2,063 |
| 4,096-byte `memset`, destination offset one byte | 12,292 | 2,084 |
| Eight-byte `memcpy` | 36 | 41 |
| Eight-byte `memset` | 28 | 33 |

Instruction counts are not cycles. The bulk same-alignment paths benefit;
short and differently aligned calls retain a small fixed dispatch overhead.
`memcpy` still requires nonoverlapping regions. No unaligned word access or
read past the requested byte range is introduced.

## Shared State storage verification

The `d4e41a8` change removes a second 200,368-byte State. The added code and
owner metadata make the measured Zehn header allocation reduction
**198,768 bytes** in this isolated worktree:

| Isolated runtime tree | Zehn file | Header allocation | Executable/BSS request |
| --- | ---: | ---: | ---: |
| Rendering/memory candidate `4cf0019` | 1,755,472 B | 22,881,288 B | 22,724,308 B |
| Shared State `d4e41a8` | 1,755,618 B | 22,682,520 B | 22,525,452 B |

The small compressed-file increase buys substantially lower runtime memory;
this is RAM reduction rather than a claim of disk-space reduction. The
13,369,344-byte hot picture buffer remains intact. These totals exclude the
other UI16 branch's resource-loader changes and scene-cache sharing.

The new `state_workspace_regression` passes 246 checks with each actual
English and Chinese pack. It covers 24 console-open/close cycles, both close
inputs, complete rollback after a mid-command rejection, blocked reentrant
saved reads, partial failed reads, two App owners, initialization, language
reload and real Continue. A six-suite native run additionally covers existing
debug, merchant preparation, profile and frontend/save behavior.

A separate 369-frame sequence per language compares the old renderer with
the shared workspace: title, console help, title again, console-started
combat, return to title, then Continue into the saved merchant. All **738
complete-frame hashes, canonical-state hashes and resource-read counters**
match. Closing the console on the title performs the designed one additional
save read, changing the sequence's host save-read count from three to four.
Ordinary frames perform no new disk reads. Actual merchant preparation passes
25,516 English and 27,507 Chinese checks, retaining all stock and hover glyph
preparation.

The workspace is independent of the existing foreground map/codec/scratch
union. A debug transaction can still execute map generation and relic hooks
without overwriting its rollback.

## Ordinary OS allocation failure diagnosis

The device independently reproduced a normal UI15 Save and Quit followed by
an immediate second launch failing with `[Zehn] Memory allocation failed!`.
No USB operation is needed. Waiting twenty seconds does not resolve it.
The preliminary full UI16 build also crosses the available contiguous-block
boundary on its first clean launch. A successful ARM build below the old
24 MiB header ceiling therefore did not establish ordinary OS startup.

The audit uses the exact official [Ndless r2022 source tag
`b88e4af2f74f10f3b28c9f7290a79de8fd9108b8`](https://github.com/ndless-nspire/Ndless/tree/b88e4af2f74f10f3b28c9f7290a79de8fd9108b8).
Its [`zehn.h`](https://github.com/ndless-nspire/Ndless/blob/b88e4af2f74f10f3b28c9f7290a79de8fd9108b8/ndless-sdk/include/zehn.h)
confirms the launcher's flags. None requests resident execution.
[`ploaderhook.c`](https://github.com/ndless-nspire/Ndless/blob/b88e4af2f74f10f3b28c9f7290a79de8fd9108b8/ndless/src/resources/ploaderhook.c)
calls `ld_free(base)` after ordinary nonresident entry returns. The SDK CRT's
additional cleanup is for its newlib/libstdc++ allocations, which this
freestanding launcher does not link. The custom entry preserves the required
callee-saved registers and returns normally; changing it to call an unrelated
CRT cleanup routine is not supported by this evidence.

The [Zehn loader](https://github.com/ndless-nspire/Ndless/blob/b88e4af2f74f10f3b28c9f7290a79de8fd9108b8/ndless/src/resources/zehn_loader.cpp)
first allocates the relocation table, flag table and extra metadata separately.
It then requests `header.alloc_size - prefix_size` as one executable/BSS
block. The compressed payload also has a temporary allocation while the
executable is decompressed. The header allocation is consequently neither
the largest single request nor the whole transient loading peak.

Read-only RAM extraction from the ordinary OS snapshots finds:

| Snapshot | Free bytes in the inspected system pool | Largest free block |
| --- | ---: | ---: |
| UI15 before first launch | 29,383,280 | 22,741,376 |
| UI15 warm combat | 6,368,860 | 4,584,860 |
| UI15 after normal exit, before USB | 29,245,424 | 22,728,648 |
| UI15 after failed reopen and OS error UI | 28,970,312 | 22,296,712 |

The live program block changes from allocated to free after exit, and the
complete block list has consistent forward/backward links. This rules out
the suspected unreleased 22.7 MB executable in these captures. UI15's actual
executable/BSS request is 22,722,916 bytes, leaving only 5,732 bytes against
the largest block immediately after exit.

The allocator's next-fit cursor then points at that freed block. Reopening
carves a 156,824-byte allocation from its beginning, exactly the 156,816-byte
relocation table plus the SDK's eight-byte allocation allowance. The loader
frees the metadata on failure, but subsequent OS allocations prevent the
large block from immediately coalescing. The failed-dialog snapshot includes
those later allocations; its extra loss is not all attributed to the loader.

The remedy is to reduce the genuine production allocation requirement. The
shared State above and the separate Random/merchant scene-cache work do so
without shrinking the hot image pool. Final clean startup and repeated
ordinary exit/reopen must be verified on the integrated launcher; this
diagnosis does not substitute for those device checks. A future build budget
must also guard the actual executable/BSS request, with headroom based on
that executed configuration, while retaining the existing total header
ceiling. No such budget can guarantee every arbitrarily fragmented OS heap.

## Storage reachability and retained implementation audit

The actual English logical pack has 31,210 live image descriptors and 500
live index groups. Running the existing pruning operation saves **zero bytes**.
Of 784,936 tiles, the 95,730 unreferenced entries are already empty. The 139
entirely empty compressed groups occupy only 24,261 bytes altogether; this
is not a hidden multi-megabyte artwork saving. Vantom's conversion reuses
old whole-frame image IDs for the layered representation and empties unused
slots. It does not retain another complete old animation payload.

Previously evaluated tile transforms, palette/bounding-box packing and
decorrelation are already recorded in the repository's resource tooling.
The UI15 merchant's independent compressed streams intentionally spend about
189 KB to avoid unrelated cold decoding; reversing that change for its small
disk saving would undo the documented cold-I/O improvement.

The UI14 difference ledger is superseded by
[UI15's reconciliation](ui15-retained-differences-0.17.md). Read-only source
tracing and the actual 1,312-move gate confirm that monster awaits are reached
through the real core observer and command program, and Vantom's 750 ms bone
transition, independent tail, masks, emitters and death clocks are called by
the production renderer. They are not unattached diagnostics. No additional
unwired production path was found among those explicitly retained families.

The accepted omission of full Ancient actor animation remains the agreed
storage exception. Affliction's eight phases, Infection's thirty frames and
Ancient fire's ten frames remain explicit CPU representations. Audio,
multiplayer and Timeline remain excluded. This audit does not assert complete
pixel equivalence with Godot's continuous GPU simulation.

## Rejected work and verification record

Two Random-dot cache experiments remain in the checkpoint history and are
reverted from production. An 8 KB tile-bounds cache preserved the pixels but
made the dot loop roughly 40% slower. A second quad-flags version matched
3,600 frames but made dot CPU time 4.68% worse and whole-screen CPU time 1.13%
worse. Neither is reported as an optimization. The accepted Random renderer
keeps all forty original particles.

Earlier failed validation attempts are retained: one initial build inventory
changed during a benchmark edit; the first combat test invocation used the
wrong input kind; and the first Unicorn timing probe attempted to divide by
a zero count because hooks missed cached translation blocks. Corrected frozen
runs passed. Two shared-State `run_logged` invocations with short SHAs were
rejected by its argument parser before any command ran; the successful runs
use the full checkpoint SHA. These failures are not relabeled as passes.

Key successful local journal IDs:

* `perf-card-pixel-contract-b87c73f7fef4`
* `perf-card-hud-actual-c61ee3699822`, `perf-card-hud-zhs-c5b047d9bfbf`
* `perf-card-merchant-actual-dc70896fb91f`, `perf-card-combat-ui15-f14fda5df487`
* `perf-card-status-ab-fde7220b592b`, `perf-card-pinned-ab-89e1591030b1`
* `perf-card-scene-ab-d82cc370d089`
* `perf-memory-linked-arm-counters-db7cda47ecdc`
* `perf-memory-final-object-arm-d9cbeca97252`
* `perf-storage-reachability-cb7b0635efa7`
* `perf-shared-state-native-build-9d8a016fbd31`
* `perf-shared-state-arm-build-05996cea2f1f`
* `perf-shared-state-core-0748dbc0a20b`
* `perf-shared-state-english-0b15fa45c7dd`, `perf-shared-state-chinese-23802cf5f47d`
* `perf-shared-state-frame-ab-7301d06184f9`
* `perf-shared-state-shop-english-8896d7954b82`, `perf-shared-state-shop-chinese-b6301db008e2`

Private original resources, calculator images and disassembly are not included
in the public source history. Source checkpoints and bundles preserve the
implementation; external publication is handled by the integration owner.
