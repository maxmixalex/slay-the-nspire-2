# UI24 release and recovery record

This record separates the tested calculator installation, original-derived
resource production, standalone converter execution and source validation.
The reference remains **v0.111.0 / 41cef1ea**. The companion
[numeric and hash record](ui24-release-validation.json) contains source and
artifact identities without original assets or private download identifiers.

The three prebuilt calculator ZIPs passed their recorded guest and package
gates and have been recovered with their original SHA-256 values. The new
converter with corrected scratch cleanup still requires its own completed
cold/warm runs and final output comparisons; the prebuilt packages do not
substitute for that acceptance.

## Accepted calculator installation

The ordinary launcher is actual
`c8142bd99423a19e82da7aae9b4e33de4ad7c939`, with verified published equivalent
`73e1213810bc28672202a3ca83d87fe32aad149c`. It is **1,736,530 bytes**, SHA-256
`1c455b5f630dee02750f0b54767850770d05e6643ec670427bf535312447eeb1`.
Formal allocation is **22,287,844 bytes**, leaving **3,172 bytes** below the
unchanged **22,291,016-byte** ceiling. No profiling flags or extra heap, cache
pool or framebuffer were introduced. Its matching linked ARM helper ELF is
`dc46a7a03660ffe358971f4ecdcbeb0a7e1cc6d6d745c4930c33e7b0ddaa9199`.

Guest acceptance used the supplied Firebird **TI-Nspire CX II CAS**, **OS
6.4.0.74**, **64 MiB** firmware/OS/Ndless setup. Its boot log recorded CPU/AHB/APB
at **396/198/99 MHz**, with no new clock setting applied. Timing uses 32,768 Hz
guest counters. These are not physical-handheld measurements or estimates for
an older 132 MHz CX; the factory clock setting was not independently established.

| Prebuilt ZIP | ZIP bytes | All extracted members |
| --- | ---: | ---: |
| Full animation, English/Simplified Chinese | 69,000,814 | **69,869,324** |
| Reduced animation, English/Simplified Chinese | 27,997,343 | **29,695,944** |
| Both families' optional languages | 29,468,976 | 32,253,401 |

The full extracted total includes instructions and licenses and leaves
**130,676 bytes** below 70,000,000 decimal bytes. Both main archives contain
23 members, including an empty `state/` directory. The optional archive has
29 members: its instructions and fourteen locales for each resource family.

| ZIP | SHA-256 |
| --- | --- |
| Full | `e15cef0081899b78cd23628987ae36bf5256c57619afcb825f82bc290faa11ab` |
| Reduced | `a5f149f97e261676d0a465dbbd4303b9c4cdf50fb8f03ff3beb40ad3d787f3a7` |
| Optional languages | `e72e7b6348cef8615efc2c953474789cafed90476319fb2652784577d7e4cd4e` |

The independent archive audit streamed every member and checked length and
SHA-256 without importing the packager's validation code. Both sets of twelve
required launcher/data files match ordinary guest USB readbacks; each optional
set matches its own resource family. No test saves, firmware, NAND, PCK or
original game assembly are packaged. Recovery verified the persisted ZIPs;
it did not repeat the earlier guest scenarios or relabel their execution dates.

## Resource families and corrected reduced acceptance

| Family | Bilingual data, 11 files | All 16 locales, 25 files | Recorded producer |
| --- | ---: | ---: | --- |
| Full | 68,052,158 B | 86,019,153 B | Actual 39d28d6, published implementation reference 0e31aea |
| Corrected reduced | 27,878,523 B | 42,163,250 B | Actual cd0d5a7, whole-tree public equivalent 4d75c55 |

The full producer is `39d28d60273a62a722a2f1b0efeaf3083d8b7272`; the final
manifest uses `0e31aea16cec96fc9ed9480efe6ee5cd64477abe` as an implementation
reference. It does not assert whole-tree equivalence. The earlier resource
receipt retains its historical `6ea8bdfc` reference. Corrected reduced comes
from `cd0d5a7dfbef4238abe1da701c82b7874f9a7b14`, whose recorded public tree
equivalent is `4d75c55f7561711ced681806d924f24c65ca4959`.

These prebuilt families came from verified original-derived conversion tails.
The first reduced NWF3 candidate failed startup because generic writers erased
its NAF1 animation-profile header. The repair preserves that header at both
writer boundaries. Corrected reduced was rebuilt from the proper pre-NWF3
source; no failed baked file was patched into the deliverable. All sixteen
reconstructed locales retain NAF1 and 8,961 strings.

The recovered final reduced receipt records twelve exact normal USB readbacks,
continuous Chinese and English Timeline/detail/scroll, Silent 70 HP Continue at
NWF3 Neow, save and normal OS exit. Production decoding retains score 20, the
two actual reveal dates and frozen mask `00000000/00020002`. A separate declared
combat fixture verifies essential Strike poses, no broken slash placeholder,
and owner/Osty corpses still visible after 10.1 seconds. The transient blue arc
is not claimed for the reduced three-pose sample; it is separately visible in
full c814. A 100-frame decoded bridge preserves the tested poses, coordinates
and clocks. See [Timeline acceptance](ui24-timeline.md).

The primary-ID guard correction is separate from the lost NAF1 marker. It
accepts two reduced representations only when they select the same complete
descriptor at the same clip's exact retained sample. Full-mode and retained
IDs remain strict, and the geometry/pose validator still runs. This guard is
also called by standalone `validate-install`. A standard reducer that retains
stable frame IDs need not produce the same raw file bytes as a tail using
direct selected IDs; actual output comparison is required before claiming
byte or runtime equivalence. Optional locale files must come from the same
conversion/family as their installed artwork and common data.

## Ancient performance and seed compatibility

The following historical matched formal full-resource warm windows performed
no resource reads. Build identities are deliberately kept separate.

| Scene | UI23 FPS | UI24 FPS | UI24 runtime and observed window |
| --- | ---: | ---: | --- |
| Neow | 11.201985 | 29.312073 | Earlier b690; 880 frames, no interval over 50 ms |
| Nonupeipe | 22.797750 | 28.283556 | Earlier b690; 848 frames, no interval over 50 ms |
| Tezcatara | 6.388653 | 23.915010 | Final c814; 717 frames, no interval over 50 ms |

Tez mean render falls from 155.985 to 41.457 ms; its cold-containing records still
include approximately two-second stalls. A separate final-c814 NWF3 Neow
post-Continue first-cycle window measures 29.055081 FPS, with 24 reads/511,552 B,
12 intervals over 50 ms, five over 100 ms and none over 250 ms. Its cold-containing
maximum render is 1,604 ms. This is not the earlier fully warm zero-read window.

The scene changes preserve actor frames and action clocks. Neow holds its
expensive radiance field at entry while retaining its 546 animated transmission
frames; NWF3 stores the exact 17,485-word integer field offline. Tez shares three
9×9 material grids and keeps 192 selected cinders at their source 60 Hz trajectories.
The final glow and flame composite is intentionally softer; it is not claimed
pixel-identical to the original composite. The retained
[Ancient report](ui24-ancient-performance.md) states the finite visual and
scratch checks, family bridge and cold limits.

Full-animation actors keep the port's established sample counts, individual
frame dimensions, registered positions and action clocks. Actors are baked
at their displayed density and stored as cropped pieces; they do not all use
one reduced sprite size. UI24's slash repair and Ancient effect work do not
shrink those actor rasters or remove their samples. The inherited full profile
does use bounded RGB565 color reduction and up to four alpha units of error;
it is not an assertion of lossless original rendering. Reduced mode explicitly
holds one source pose for loops and at most three for actions, while retaining
the action duration and terminal pose. The [artwork policy](converter-ui23.md#animated-artwork-policy)
documents the distinct color and sampling contracts.

At the verified original-derived tail, replacing Neow's six unused procedural
input pages with its held-clock light field saves 162,740 bytes in full and
113,558 bytes in reduced. Tile deduplication, sparse encoding, literal-context
retuning and unreachable-index removal preserve the already selected pixels.
The workspace correction below changes temporary-file ownership and lifetime,
not the output artwork policy. These storage changes are separate from the
intentional lighting, flame and particle simplifications visible in the room.

Ordinary PC ASCII text preserves the pinned original normalization/hash. The
64-byte direct editor is a port field, not an OS seed limit. For longer text,
`--encode-seed` or streaming `--encode-seed-file` hashes the entire input and
produces a code for the normal **Import seed code** action. Ordinary `raw:` or
`SEED64:` text is still ordinary seed text; import is a separate mode. The port
stores the explicit import code rather than silently truncating the original
long input. The separate 2,048-point limit is for optional hand-drawn map marks,
not generated map rooms.

The published original-DLL record covers 927 ASCII inputs, 13,905 named RNG cases
(12 global streams plus three named constructors), 7,416 configured maps and 927
save round trips. The original default seed is 12 characters from 34 available
characters. The earlier 2fbe/809c ordinary guest suite covers English/Chinese
input limits, import, save, restart and Continue, with two byte-identical 7,074 B
USB saves and all 15 RNG states/counts retained. A later c814 keyboard Start/
save/exit control also passed. The first touch-start exit relaunched once and
its precise trigger remains unresolved. These checks establish stated seed,
RNG and map behavior; they do not guarantee identical complete runs across
versions, unlock histories, characters, rules or choices. See
[ASCII seed validation](seed-ascii-ui24.md).

For the downloaded standalone files, encode a long ASCII seed with
`./nspire2-convert-ui24-linux --encode-seed-file seed.txt` on Linux or
`.\nspire2-convert-ui24.exe --encode-seed-file .\seed.txt` in PowerShell.
Open the Custom Run seed field, use Up/Down or its **Import seed code** button
to select import mode, enter the returned `SEED64-` code and confirm. The file
must contain the intended seed; interior spaces and newlines remain significant.
This encoder rejects non-ASCII input explicitly. The original text need not fit
the calculator's field because the calculator saves and displays its import code.

## Converter recovery and scratch correction

The earlier actual 94b3ae/public 25c166 Linux executable (`97512e3d...`) was last observed
at stage 63/84 before the execution environment became unavailable. Its recorded
whole-TMP high-water observation was 3,062,857,728 B at that point. No completed
receipt was obtained; neither warm command nor the final 11/25/25 output bridges
had run. This partial observation is not a full conversion time, final peak,
cleanup pass or result for the replacement executable. The older 431790/c822 attempt
was separately stopped after 1,609.918 s because its source lacked reviewed locale
bindings. It is not a deliverable or a completed baseline.

The 94b3ae run exposed an actual cleanup omission: five builtin stages supplied
their scratch directory as positional argument 3, while ownership recognized
only declared outputs and named flags. Public
`5a21f14752dba19e4ceaa43d1b41639dd27d4826` fixes those exact builtin contracts
and cleanup on verified skipped boundaries. It retains last-reader protection,
KEEP recovery boundaries and failed-stage inputs; it does not claim pre-existing
unowned directories or add a recursive clean command. The successful summary
now prints the actual workspace path and sampled retained bytes.

Allocation uses filesystem st_blocks where available. The existing fallback
rounds logical sizes to 512 bytes and is explicitly labelled estimated in both
JSON and console output, including on Windows. Workspace counters exclude the
frozen executable's extraction and other external temporary directories.
See [workspace management](converter-ui23.md#progress-and-resuming).

Seventeen focused workspace tests pass on fixed actual
`6c26a0b904ae1de12f501b53ffdac2366aa3abf8`, tree
`2ccefdcc30d66f693590bbab6eda1c519e457205`. They execute the production recipe,
workspace, receipts and real filesystem with lightweight resource workers.
The old workspace fails the added ownership/boundary cases. Independent review
also checks full bilingual/full all-16/reduced all-16 recipe graphs. This is meaningful lifecycle
acceptance, not a new original-asset conversion claim.

Public `e679005fc507fa03fbe732a39bec13f62ae74ba6` changes only the Windows CI
pattern so both workspace modules are selected: 17 methods instead of 9.
Its production inputs are identical to 5a21. Its actual Windows run found one
test expectation using the short temporary-directory path while the program
correctly printed its resolved long path. Public
`b18b10b7376d7cd6f19bd1c998b71841215cd068` changes only that expectation to the
independently computed `work.resolve()`; production code remains unchanged.
Public `dc8b300dccdaa77da3e1853d9f7c744ca7db34f5` then adds those two test files
to the Windows workflow's push paths, so later test-only edits trigger the gate.

The final Windows dc8 workflow actually passes all 17 methods in 1.478 seconds,
with no skips. Its source and frozen locale gates check 935 UI bindings and 142
extra original keys. Two real worker processes, the one-thread numerical limit,
UTF-8, lock release, temporary-file removal and the matching linked ARM decoder
also pass. These tests do not run a complete original PCK/DLL conversion on
Windows. The replacement Linux keeps actual/public 5a21; workflow and test-only
followups do not change its production inputs.

| Standalone file | Actual/public producer | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Linux | `5a21f14752dba19e4ceaa43d1b41639dd27d4826` | 90,671,856 | `2f549cb9fcf08b12d827d880f5992e082fc778f3c96256bf3e87fc989ec1208c` |
| Windows | `dc8b300dccdaa77da3e1853d9f7c744ca7db34f5` | 70,296,205 | `ae7b9e37d1bfb6fc2df4407f38bf4510e7bd1fc1089a0b774d049b90d9f01f8c` |

The combined converter notices are 365,662 bytes, SHA-256
`a20e6f044227e3ebeadd28466bec7838d0cf52ff3ee08df8571cbf881e6a162c`.
The downloadable names add `-ui24-linux` and `-ui24.exe`; upstream converter
filenames remain `nspire2-convert` and `nspire2-convert.exe`.

The new empty-work Linux run began at 12:01:31.539836 UTC on 2 November 2026.
Its original-asset cold conversion, both all-language warm commands and the
independent final output comparisons remain pending at this checkpoint. Timings will
retain the real shared-environment wall clock, including overlapping checkpoint
exports; an interrupted run on a different host is not a whole-pipeline speed
baseline.

## Source validation scope

Complete CI run **41440911143**, at published
`e679005fc507fa03fbe732a39bec13f62ae74ba6`, passes all **183 native suites** with
ASan/UBSan in **179.69 seconds**, **646 discovered conversion tests** in
**65.668 seconds**, and the portable build. The separately selected 56-test
command is not added to the discovery count. The final Windows **17-test** and
frozen executable gates are run **41442997775** at **dc8**, as described above.
These results retain their actual source labels; moving a branch or adding
release documentation does not relabel an older CI run.

## Evidence recovery and retained scope

The persisted three ZIPs, delivery manifest and Timeline/animation evidence
archive were restored with their recorded hashes. That archive includes
ordinary full/reduced guest receipts, final runtime allocation and the
independent ZIP audit. The separate Ancient/seed tar and its planned later
replacement were not recovered. Previously published Ancient/seed reports and
the contemporaneous recorded results remain historical evidence; this recovery
does not claim to have re-created missing raw oracle logs, screenshots or saves.

Earlier GitHub write outages were handled by an explicit, session-specific
permission to validate committed frozen local sources while publication was
recovered. Those runs retain their actual chronology; they are not relabelled
remote-verified before starting. This did not change AGENTS.md's default
checkpoint-before-long-validation rule. Recovered cleanup source was published
and read back before its new long converter acceptance.

Timeline's 57 source nodes and the normal unlock/save paths are implemented;
the firmware checks did not play 57 entire campaigns. The LCD chronological
list adapts the original spatial presentation. Act2B/Act3B remain original
Early Access placeholders. Audio, multiplayer and hosted services remain
unimplemented, with no inferred user waiver. Simultaneous seven-family
Affliction card materials are a separate heavy rendering case: UI24 did not
newly optimize or finally remeasure that extreme combination. Its earlier
UI23 number is not a current c814 measurement, and the report does not claim
30 FPS for every scene or 95% measured visual fidelity.
