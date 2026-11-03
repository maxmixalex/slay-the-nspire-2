# Development entry point after UI25

Read this file and [AGENTS.md](../AGENTS.md) before continuing work. Repository:
`maxmixalex/slay-the-nspire-2`; original-game reference: **v0.111.0 / 41cef1ea**.
The complete preceding 108,104-byte work log is preserved unchanged in
[WORK_STATE-through-ui25.md](WORK_STATE-through-ui25.md), in the same directory
so its relative links retain their meaning. [WORK_STATE-ui19.md](WORK_STATE-ui19.md)
retains the earlier archive. Historical checkpoints and pending statements in
those logs describe their own stages, not the current release.

## Source and evidence identities

| Role | Identity and scope |
| --- | --- |
| Accepted ordinary calculator runtime | `ed5dd2564625779313aa2c71046079dee7506e76`; final UI25 guest, ARM and native evidence |
| Corrected resource producer | `e98a6cb4ed29cd4e3bcaa597cff9ad089c6854f7`; actual corrected full/reduced tails and all-language publications |
| Published follow-up checkpoint | `6548e6777b563c8032ba100196d72c9e5f2755fe`; runtime C++ unchanged; two ARM Python build scripts have AST-equivalent formatting; public launcher CI artifact added |

The [UI25 release record](ui25-release-validation.md) and
[numeric/hash receipt](ui25-release-validation.json) are the authoritative package,
resource and executed-test records. Later documentation or build publication does
not reattribute those measurements to a newer launcher. Read the
[player-feedback reconciliation](ui25-player-feedback-reconciliation.md) for
current implementation paths, historical supersessions and unresolved input scope.

Use the actual current remote refs and local status to choose an isolated task
branch; do not assume an old integration branch is still active. Preserve and
read back each coherent source checkpoint before long work, following AGENTS.
Synchronize verified progress to `main` within the existing authorization. Keep
unique branch history reachable or archived before any cleanup.

## Fixed private packages and hard limits

The three accepted UI25 ZIPs are fixed; do not rebuild them for documentation-only
follow-ups. They contain no user saves or desktop converter. Original resources,
firmware/NAND, fonts, PCK/assembly data and embedded-content HTML remain private.
The public launcher artifact contains executable, instructions/commit/hash manifest
and required licenses only; it is not a complete resource installation.

| Package | All extracted members | Boundary |
| --- | ---: | --- |
| Full animation, English + Simplified Chinese | **69,839,495 bytes** | **160,505 bytes** below 70,000,000 decimal bytes, including instructions and licenses |
| Reduced actor animation, same languages | 29,811,189 bytes | Matching reduced resources required |
| Optional fourteen languages, both profiles | 32,244,564 bytes | Separate optional package; use the matching profile |

The accepted launcher is **1,737,025 bytes**, SHA-256
`9c578a2cb0ca079a3aea523e8fcfadf0aecef3e4aee5ca557423c9c238932b1f`.
Its formal allocation is **22,288,920 / 22,291,016 bytes**, leaving **2,096 bytes**.
Do not enlarge heap, caches, arenas, preparation bounds or the formal ceiling.
The first-action preparation remains bounded to **288 images / 640 pages**.
Ordinary validation uses the supplied **64 MiB CX II CAS, OS 6.4.0.74, Ndless**
Firebird path at its recorded clocks; no RAM, clock or snapshot-header bypass.
Linked allocation is not physical-device peak-memory evidence.

## What the final runtime evidence establishes

Final full and reduced USB readback verifies each edition's twelve launcher/data
files before fresh ordinary OS launch. The scoped Necrobinder/Nibbits first-Strike
pair adds **zero combat reads, bytes or resource time** relative to idle. Full and
reduced Strike/Osty actions, joint death with both corpses visible at 10.1 seconds,
save/exit/relaunch/Continue, and OS return at 20/30 seconds pass. English and
Simplified Chinese Settings/visual/readback checks pass. Saturated encounters can
still leave optional actions cold; this is not all-attack or all-encounter coverage.

The final zero-I/O scene windows report:

| Scene | Full FPS | Reduced FPS |
| --- | ---: | ---: |
| Ironclad selection | 30.000153 | 30.002197 |
| Random selection | 27.754131 | 30.006227 |
| Neow | 29.752381 | 30.007326 |
| Tezcatara | 26.927660 | 30.007326 |

Full Ironclad uses an additional 60-second settle. Its retained initial
5-second-settle / 30-second first cycle is **28.204384 FPS**, with **17 reads /
1,199,150 bytes** and **five intervals over 250 ms**. Cold startup/transitions
still stall. Reduced Random uses a static background; the Random fixture uses an
explicit synthetic legacy-compatible profile with zero obtained Epochs/timestamps.
The table is scoped ordinary Firebird evidence, not universal 30 FPS, natural
Timeline progression, or physical-handheld performance.

## Remaining work and evidence boundaries

- A supplementary ordinary ed5/e98 full-edition check passed **touchpad Custom
  Start → Neow → Pause → Save & Quit → title → Quit → Yes → TI OS**, using
  isolated Profile 2 with a production-codec empty NSH2 legacy-compatibility
  input and matching USB readback. Initial profile selection used the keyboard;
  the tested route used actual relative touch movement and central press/release.
  The first fast Pause movement missed; a settled reposition reached the control.
  Yes was held from 541,190 to 541,590 simulated ms, then the finger lifted at
  541,990 ms. From 543,990 to 573,990 ms, the initial OS frame and thirty
  one-second samples were pixel-identical, with no relaunch. This is not the
  recovered original failing profile/gesture or a root-cause fix. The historical
  trigger and cause remain unknown. See the [input evidence and fixture scope](ui25-player-feedback-reconciliation.md#touchpad-custom-run-check-and-historical-exit-issue).
  The earlier receipt's `remaining_scenarios=[]` retains its original gate scope.
- Corrected e98 production passed resumed full/reduced tails and all-language
  publication, not a new empty-work cold conversion. The completed historical
  5a21 cold run took 13,087.111953 s with 2,279,501,824 allocated whole-TMP bytes,
  but its rejected slash output prevents corrected-output acceptance. Two planned
  whole all-language warm runs did not finish. Tail workspace peaks and the later
  host synchronization ENOSPC incident are not whole-TMP cold measurements.
- Windows frozen/source/self-tests do not establish Windows original-game
  conversion. Portable WebAssembly compilation is not interactive browser
  validation. Do not run manual WASM/browser validation without a renewed request.
- Audio, multiplayer, hosted/cloud services, timezone/DST data and calculator IME
  remain unimplemented, without a user waiver. Source Early Access Timeline
  placeholders remain. The seven-Affliction extreme has no new final optimization
  or measurement; exhaustive fidelity and physical-handheld measurements remain
  unestablished.

Timeline/57-Epoch progression, long-ASCII seed import, paged history and the
five-character foundation remain implemented. Their detailed historical scope is
in [Timeline](ui24-timeline.md), [ASCII seeds](seed-ascii-ui24.md),
[paged history](ui23-history-archive.md), and the release record above.

## CI and the public launcher

The accepted ed5 [core run 41495670719](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/41495670719)
passes all three jobs: 183/183 ASan+UBSan native suites, 652 Python discovery tests,
a separate 56-test supplemental command, and portable builds. These are separate
command scopes, not an aggregate independent-test total. The ed5
[Windows run 41495670730](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/41495670730)
passes both jobs.

Follow-up checkpoint 6548's
[core run 41637348636](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/41637348636)
passes portable and conversion (652 discovery tests and a separate 56-test
command); [Windows run 41637348613](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/41637348613)
passes both jobs. Both are push runs for the exact 6548 commit. The actual
[public launcher artifact 13231387011](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/41637348636/artifacts/13231387011)
was downloaded and independently checked: 13 exact allowed members, matching
COMMIT/SHA256SUMS, and README/licenses equal to the published source. Its
1,737,025-byte TNS has the **same SHA-256 as the accepted ed5 launcher above**;
this is byte identity, not only equal size. The ZIP is 1,804,781 bytes, SHA-256
`81f7d8658c482575991725d4019692c9e1a7c52531f482474a2c0a288dae4b8b`.
The 6548 native job was still running at this artifact review; its eventual
result belongs to the linked core run, not an assumed complete-green claim.
This follow-up does not add gameplay, physical-device or original-data conversion
acceptance and does not replace the fixed private packages.
