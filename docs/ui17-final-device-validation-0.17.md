# UI17 final ordinary-OS device validation

## Frozen identity and scope

This record applies to production `d4303baf9a720bbeed58c1bb1859ff63343afcd6`.
The ordinary launcher is **1,766,579 bytes**, SHA-256
`1a9a0fb8de2ba7f99a43ccbef3a21d915e72536c9eb62782634a3abdb6f83459`.
Its Zehn request is **22,269,692 bytes**, comprising **157,220 bytes of metadata**
and a **22,112,472-byte contiguous execution/static block**. The image pool
remains **13,369,344 bytes**. Compared with the restored UI16 launcher,
execution/static demand falls by 253,916 bytes; this is an allocation request,
not a measured physical peak or a guarantee under arbitrary OS fragmentation.

The installed bilingual data is the final `private-ui17-retained/data` set.
`final-ui17-install/receipt.json` gives the hashes of every default resource;
USB directory lengths and an exact launcher readback were checked. The French
locale used for language/error gates comes from the same final set. Original
resource input files were never changed during fault injection. Default
program plus resources totals **79,957,013 bytes**, excluding saves and notices.

Execution uses the owner's ordinary **CX II CAS / OS 6.4.0.74 / Ndless r2022**
installation at **396 MHz**, under unmodified Firebird core
`b10f3b51a9ab8e27d2703444b5e6bd278d4c5e6e`. Only the external command/LCD/USB
frontend is added. This is emulated ordinary-OS evidence, not timing measured
on a physical handheld. The emulator RAM/NAND work copies remain private.

## Measured startup and actual combat

The unchanged UI16 launcher was restored and measured again with this core and
installation. Its SHA-256 is
`cf83955453bc6046e57895a94efdf68ecf00a6c6e4ba2eec5e3a172c94f36158`.
Historical UI15/UI16 reports have a different installation/timing context and
are not substituted into this table. Both versions use the actual Haunted Ship
first-turn fixture; End Turn really applies Weak and creates the five Dazed.
The final action screenshots show both states.

For each version, a warm combat RAM snapshot and its matching NAND are restored
independently for control and action. The control exits immediately. The action
holds Menu for 600 ms, then runs the remaining 5,400 ms before exactly the same
Save & Quit / title Quit path. Subtracting cumulative counters isolates the
action window. Maxima cannot be subtracted. This is one controlled pair per
version; the finite endpoint can include different numbers of complete frames.

| Measured quantity | Restored UI16 | Final UI17 |
| --- | ---: | ---: |
| Startup: selected resource validation + App init, ticks | 49,754 | 36,191 |
| Same startup scope, ms | 1,518.372 | 1,104.462 |
| Action complete frames | 169 | 171 |
| Action interval total, ticks | 196,433 | 196,905 |
| Mean interval, ms | 35.4714 | 35.1407 |
| Derived window FPS | 28.1918 | 28.4570 |
| Render elapsed total, ticks | 137,828 | 138,015 |
| Mean elapsed render stage, ms | 24.8886 | 24.6309 |
| Resource elapsed total, ticks | 8,931 | 6,985 |
| Resource elapsed total, ms | 272.552 | 213.165 |
| Actual provider reads / requested bytes / seeks | 19 / 113,070 / 19 | 19 / 113,079 / 19 |
| Intervals over 50 / 100 / 250 ms | 4 / 3 / 0 | 4 / 2 / 0 |

The clock is the existing foreground SP804 at 32,768 Hz. Startup's reported
scope is 413.910 ms shorter, and action resource elapsed time is 59.387 ms lower.
The action read count is unchanged; reduced syscall count alone is not being
used as evidence of a speedup. Render elapsed time can include synchronous
resource work, so it is not a CPU-cycle profile. The roughly 0.94% window-FPS
increase is an observation of this pair, not an all-scenes or statistically
established FPS improvement. Native replay's different read counts are not
relabelled as these device counters.

The final complete document-Enter-to-first-full-menu observation is
**12,960 ms**, using 20 exact opaque menu-glyph pixels at the core's 10-ms
observation cadence. Down navigation and a normal OS exit are also confirmed
in `final-ui17-launch-watch/`. This is the final installation's absolute
observable launch duration, including the original intro. It is distinct from
the 1,104.462-ms selected-resource/App scope and is not presented as a paired
causal improvement against the isolated experiment's different installed data.

The startup timer still excludes the loader, common-index work before the
selected-resource scope and the original intro presentation. The isolated
same-resource e3 experiment is separately documented in
[deferred artwork I/O](ui17-deferred-artwork-io-0.17.md): it reduces the scope by
308.289 ms and independently reduces Enter-to-visible-menu from 13,380 to
13,060 ms. That isolated result is not reused as a measured complete-launch
improvement for the final repacked installation.

## Lifecycle and default-language gates

All gates below use the final launcher identity above. Screenshots, command
sequences and event logs are preserved with the evidence package. Screenshots
sample the emulated LCD, which can be captured during a framebuffer transfer;
complete-frame pixel/state equality is established by the separate native
oracles, not by interpreting every partial LCD capture as a finished frame.

| Gate | Actual exercised result | Evidence paths within the device evidence package |
| --- | --- | --- |
| Warm combat persistence | Play End Turn, Save & Quit, title Quit, ordinary OS, immediate no-USB reopen, Continue: the resulting hand and piles match. A changed run save is read back after a second normal exit. | `final-ui17-warm-lifecycle/turn2-before-save.png`, `reopened-combat.png`, `receipt.json`, `lifecycle.events.jsonl` |
| Chinese default and repeat launch | Cold reset, normal Ndless installer, delete language preference, first title is Chinese; Quit to OS and immediate no-USB reopening reaches Chinese title again. Preference readback selects `zhs`. | `final-ui17-languages/zhs-cold-title.png`, `zhs-reopened-title.png`, `events.jsonl`, `receipt.json` |
| French warm allocation and default | After two Chinese sessions, change the OS to Français, delete the game preference and upload the final launcher, without resetting OS. Restore that exact prelaunch RAM/NAND and Enter: French title succeeds. Normal Quit and immediate no-USB reopen also succeed; preference readback selects `fra`. | `final-ui17-languages/fra-os-home.png`, `fra-warm-title.png`, `fra-reopened-title.png`, `fra-runtime-events.jsonl`, `receipt.json` |
| Unsupported Dutch default | Cold reset with OS Nederlands, normal Ndless installer, no game preference: English title appears and preference readback selects `eng`. | `final-ui17-languages/dutch-cold-home.png`, `dutch-cold-ndless.png`, `dutch-cold-title.png`, `dutch-cold-events.jsonl`, `dutch-readback-events.jsonl`, `receipt.json` |

The French test exercises the previously failing warm-language-change/upload
pattern in the newly restored installation. It does not claim to reuse an
unavailable historical UI16 RAM image, nor that every possible fragmented OS
heap can launch the program. Here, the final smaller request actually reaches
French title and survives a second launch without an intervening reset.

Two earlier Dutch warm automation attempts did **not** execute the game. One
USB removal request stalled after changing the OS language; a subsequent
fresh frontend session could list/delete but the launcher upload stalled at
50%. Both failed journals and screenshots are retained. The Dutch acceptance
is explicitly the later successful **cold default-language** gate. These
unexecuted launch attempts are neither a game failure nor a passing warm gate.

## Fault handling and preserved failures

The final launcher was tested with a healthy French locale pack and a French
game preference, while injecting each fault into a separate copy of the final
installed NAND:

| Injected fault | First launch and immediate no-USB reopen | Exact persistent result |
| --- | --- | --- |
| Cards NSS1 header has the wrong part ID | Built-in English `RESOURCE LOADING FAILED`; ESC returns to ordinary OS on each attempt | Run (230,352 bytes) and progress (17,591 bytes) are byte-identical to the pre-test fixture |
| Required shared part is absent | Built-in English `RESOURCES NOT FOUND`; ESC returns to ordinary OS on each attempt | The same two files are byte-identical |

The receipts and reviewed `error.png`, `reopen-error.png`, `second-exit.png`
are in `final-fault-bad-header/` and `final-fault-missing-part/`. The unchanged
run SHA-256 is
`ed8a4ddf37fd47f0d8f5ab7bf644a0d033552b6f1025bffb628a0ede5bc6a971`;
progress is
`f1d745136feb3806fbb62bd37b631be4ac310181dea0019b06f1bd7b6fa0e683`.
Readback uses a fresh frontend USB connection on the exact post-exit RAM and
matching NAND. There is no USB operation between either first error exit and
its immediate retry. These are final-runtime checks, distinct from the earlier
a8 branch's historical error exercises.

After both fault gates, the correct final-installed NAND is restored and the
same launcher uploaded. Normal title, actual Down navigation, and normal OS
exit pass in `final-ui17-launch-watch/`. The mutable working copy is left with
correct resources, and the immutable input pack directory remains untouched.

The final integration contains `ResourceRuntimeGuard`: healthy state is checked
around updates, services, input dispatch and renders, and every mutating
persistence callback rejects a latched resource failure. Native tests cover
the preparation/queued-Save window that render-only error checking missed.
The device first-use-header gate complements those tests; it does not pretend
to exhaustively trigger every callback boundary on the calculator.

The directory-enumeration candidate `a8f92dbaad2fcc0a4ecf17020f4e4c8710270832`
is excluded from production. It passed correctness gates but startup was
56.854 ms slower than the selected e3 path. The remaining post-match directory
entry took only one timer tick, so early termination could not close that
regression. Its source identity, raw counters and failure history remain in
the evidence. Diagnostic tracing is absent from the release launcher.

## Evidence boundaries

The device evidence archive contains an explicit SHA-256 allowlist of JSON,
CSV/log/command records, reproduction scripts and LCD screenshots. It retains
failed attempts with their actual status. It excludes ROM, NAND, snapshots,
private original-game payloads, launchers, test run/progress/preference files,
raw full-frame RGB565 and canonical State dumps. Hash receipts for those
private inputs identify the work without publishing the payloads. The root
source-verification package separately covers native/ARM/sanitizer/resource
validation at their exact checkpoints.
