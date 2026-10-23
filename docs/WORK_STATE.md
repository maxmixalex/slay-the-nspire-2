# Work state — 2026-10-24

## Current: UI4 implementation, ordinary emulator acceptance and delivery closed

Continue original UI fidelity and calculator usability first, runtime speed
second, then storage reduction. Reference remains **v0.111.0 / 41cef1ea**.
The final production source is
**`76f0a01cb506fde7ac8b427e857462a35933ade3`**; final test/CI source is
**`6b7ca242f5938b5008cd3fc47b92c2ab90f98a79`**. The latter corrects two new
fixture assumptions and changes no production source. Private v21 resources
were converted at **`65baee5b44c11e1fa1aad4f603024aa8189a7e1a`**. This closure
adds reviewed source-only metadata. Main synchronization uses a fast-forward;
keep the two archive branches and `checkpoint/ui4-cache-isolated`, whose
unique comparison history must not be discarded. Original assets stay private.

Three parallel implementation worktrees covered bounded text caching and
cold range reads, original title composition, and combat card movement/command
waits. Follow-up review independently checked composite-intent pixels, actual
ARM screenshots and final HTML inputs. No integrity-only startup scan was
reintroduced. Developer-time comparisons are not game startup work.

### Implemented UI and the ARM regression that was actually fixed

The title layers original background, 200 stars, two groups of 40 fog particles,
foreground, original gold/teal Logo and menu. Continue shows the current act,
colored floor/HP/gold facts and omits a zero Ascension badge. Three shared images
add 138,222 installation bytes; existing artwork and animation payloads stay
unchanged. Background Spine remains the established sampled static pose.

The earlier **9a** candidate failed real calculator usability despite passing
native/browser builds: 146 title render samples averaged **226.867 ms**, with
205.444 ms minimum / 243.652 ms maximum and zero resource I/O. Normal 80 ms
keypresses were missed. It was rejected for delivery and its failure report
and working NAND are preserved. The fix visits only visible scaled destination
columns and increments inverse nearest-sampling quotients/remainders, removing
per-source-pixel division and unnecessary atlas traversal. It adds no cache
pool and removes no stars, fog, layers or pixels.

The final ordinary Chinese Firebird route records **350 title render samples
at 42.236 ms mean / 38.605 ms minimum / 42.999 ms maximum, with zero reads**:
about 81% below that rejected candidate's mean. Original 80 ms inputs complete
Continue, collection, focus, details and save/quit. Canvas verification passes
377,265 checks across 187,602 cases; 18 actual bilingual title/Continue/confirm/
pause frames remain byte-identical to 9a. These are emulator render times and
native pixel comparisons, not physical-device FPS. The English route's title
mean is **48.376 ms** and maximum **279.449 ms**, including resource reads when
returning from combat; do not describe every title frame as under 43 ms or
I/O-free. See `validation-ui4-title-0.17.json`,
`validation-ui4-title-arm-blocker-0.17.json` and the final Firebird report.

Actual combat draws now enter sequentially. Manual play, return and discard
follow visible flight/tweens; focus, hit regions, details and pile counts agree
with those displayed poses and arrival times. Toasty Mittens' selection waits
for the original awaited draw commands. Audited Leaf Slime S/M and Damp Cultist
moves use original Normal/Fast command boundaries without waiting for unrelated
animation tails. Other model-specific waits remain separate work.

Composite enemy intents independently place each icon's number. Mecha Knight
now shows damage **8 / 12 at A9** and **4 status cards**, Soul Fysh **7 and 1**,
and Vantom **26 and 3**. Multiple enemies and the smaller **3×3** multiplier
remain legible. Actual English/Chinese resources each pass **20,695 world-UI
checks**, and synthetic resources pass 6,885. The corrected same fixture linked
to the preserved old 9a runtime fails at the first independent-number assertion,
as expected. The initial wrong opening-move and opaque-glyph test assumptions
are retained in `validation-ui4-intent-values-0.17.json`; exact alpha-composited
pixel comparison remains strict. Independent full-screen review found no new
number/icon/creature overlap.

### Performance: stable card pages fixed; cold entry still slow

The bounded text cache removes the prior Chinese selection page's **15,448
reads / 63,275,008 bytes over 180 stable frames**. Both languages and all three
card scenes have zero stable-window I/O. Compressed images avoid the repeated
header read; adjacent cold pages coalesce only when the existing replacement
order permits it. No new persistent pool, font pre-read or whole-resource
validation pass was added. Six scene pairs preserve exact pixels and canonical
saves. See the card-cache and cold-range reports.

The isolated cache checkpoint **`ebf1212e71eb39f85e7fe282851b5ac05fc510f8`**
reduced the largest ordinary-emulator collection render sample from **1,646.820
to 1,508.392 ms** (8.4%). Four collection/focus/detail captures and the save are
byte-identical; scene aggregate reads fell from 151 to 76. Baseline used a
prepared pre-app OS snapshot, candidate a normal cold boot/Ndless install;
identical OS-cache histories and repeated device trials are not established.

The integrated final Chinese route's largest collection render sample is
**1,273.224 ms**, with a largest resource-service sample of **1,224.335 ms**.
These are independent maxima, not a per-frame subtraction. This route is not
an isolated causal comparison with the earlier 1.5-second result. The first
card page still visibly stalls. `ui4-next-work-0.17.md` records bounded visible
image-read batching and glyph/string payload locality as concrete next work;
neither proposal is counted as implemented.

### Executed acceptance and saved delivery

[Actions run 39999119491](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/39999119491)
passed **74/74 ASan/UBSan suites**, **135 + 53 Python tests**, and ARM/WASM builds
at 6b. The final launcher is **997,748 bytes**, with **16,328,960 bytes** of
allocation (448,256 below 16 MiB); WASM is **5,489,541 bytes** with fixed 16 MiB
memory. Both actual languages pass **23 scenarios / 649 exact native-WASM
frames / 30 exact saves**. The historical 9a linked-ARM division test retains
its own source identity; it is not relabeled a test of the final ELF. Earlier
failed builds, fixtures and emulator batches remain documented.

The final offline HTML passed ordinary file-URL start, S/O, reload Continue,
cross-language continuation and language cancellation, with zero page errors
or external requests. Its visible warm collection measurement took 300 render
samples at **0.8313 ms mean / 2.3 ms maximum**, with zero reads. These are desktop
browser CPU timings, not display or calculator FPS. Root inspected the final
English title and Chinese Continue images. See the final browser report.

Both ordinary final Firebird routes are closed passed with unchanged source
and zero event errors. Chinese continued the old run and exercised collection,
focus, details and save/quit. English continued that same saved run, took a
normal map path into the next fight, played Strike and Bash, observed enemy
Banner/empty hand and subsequent sequential draws, then saved and exited
normally. Title then showed floor 3 / 58 HP. No core/RAM state injection was
used. Static frames alone do not prove every enemy's original timing. English
startup/first-render were **622.345 / 657.471 ms**; Chinese **537.384 / 1,036.224
ms**. Final victory/reward coverage and physical-calculator performance are
not asserted for these routes. See `validation-ui4-firebird-0.17.json`.

The **76,817,608-byte calculator ZIP** and **115,708,680-byte offline HTML** were
saved successfully as authorized. Archive members and embedded WASM/common/
locale bytes match the selected inputs; complete notices and licenses are
included. Common resources occupy **74,894,324 bytes**, English **1,780,951**,
Chinese **4,563,650**. Bilingual installed size is **82,236,673 bytes before
saves**, 152,288 above UI3; resource bytes remain 12,544,438 below v19. The
larger title restoration is explicitly accounted for. Save format remains 15
and resource format is 21; preserve the user's save and language preferences
when installing the matching set. See `delivery-ui4-0.17.json`.

Next UI work: energy explanation on focus, independent intent fade/floating,
original Banner type/outline, then energy entry/exit and gain effects. Opening
deal, exhaust/shuffle/generated-card entrance, autoplay, other model-specific
waits and full title Spine animation also remain open. The 30–40 MB installation
target and overall 95% original UI acceptance are not reached. Continue these
concrete changes in parallel with the measured cold-page bottleneck; do not
restart bulk content expansion or count a diagnostic as a feature.

## Verified UI3 delivery: native, sanitizer, browser and emulator acceptance closed

The delivered runtime is **`ead747dcdffded08d641445a811af48a4fafd1de`**;
final resources use converter **`3843e8e626317da0c1508fab02a2ed4a627f9fdd`**;
packaging and the browser acceptance script use
**`d6b3d31b9f00ecd144ab1b4a1748fa1c2e731c5a`**. Later closure edits are
source documentation only; they do not change the tested runtime or resources.
The verified integration and this closure are synchronized to `main` by a
fast-forward, preserving history. `work/0.17-world-parity` remains available
for the next coherent stage. Two unique historical checkpoint branches are
preserved under `archive/`; merged obsolete work branches were removed.

The complete native Release build passed **73/73 CTest suites**. GitHub
[Actions run 39977430306](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/39977430306)
then passed **73/73 ASan/UBSan suites**, **132 + 53 Python tests**, and the
ARM/WASM portable builds at d6b3d31. The earlier 72/73 failure was a test-only
stack overflow: in-place initialization and an independent serial round-trip
workspace removed large temporary States without changing the 292 CHECK
expressions, production code, sanitizer flags or stack limit. Its targeted
34,202-check run also passed with the normal 8 MiB stack. Both the original
failure and the ptrace/LeakSanitizer environment failure remain recorded.
See [runtime acceptance](validation-ui3-runtime-0.17.json) and
[targeted sanitizer receipt](validation-ui3-potion-sanitizer-0.17.json).

The ARM launcher is **983,682 bytes**, with 16,305,200 bytes of allocation
(472,016 below the 16 MiB ceiling). WASM is 5,456,024 bytes with fixed 16 MiB
memory. Actual final English and Chinese resources each passed **23 scenarios,
649 exact native/WASM frames and 30 exact saves**. These establish the exercised
port behavior and backend agreement, not original-PC fidelity or physical-device
frame rate.

Ordinary Firebird execution found and led to a real fix for next-player cards
appearing during the enemy banner. The hand view records only ten instance IDs
per cue on the existing effect clock. Discard, Retain, drawing, focus, details,
keyword tips and hit regions now agree with the visible time. Final bilingual
world-UI runs passed 16,229 checks each, including Normal/Fast, discard/Retain/draw,
load/reset, overflow and exact saves against direct core settlement. Final ARM
captures show an empty hand during the enemy action/banner and new cards only
at the player boundary. Full pile/card-effect animation and unaudited enemy
command delays remain separate fidelity work.

Final ordinary Firebird batches used the final launcher and all three matching
resource files, without core, OS or RAM patches. English play reached combat
victory, gold/card rewards, save/quit and normal OS exit. Chinese cold launch,
title Continue into that same run, eight-card collection, focused Block tooltip,
card details, save/quit and normal exit were confirmed. Three retrieved
230,520-byte gameplay saves match exactly. The final confirmed Chinese startup
open/init interval was 778.9 ms, and the first render 884.8 ms on the emulator
SP804 clock, with no read failures and zero integrity/image/tile-index/glyph/
string pre-scan samples. The collection still had a **1,646.8 ms cold render**;
67 render samples averaged 52.9 ms (including that cold frame), minimum 22.9 ms.
This is an unresolved first-entry pause, not a warm-frame average or handheld
FPS. The earlier incomplete long event log and failed emulator snapshot restore
remain separate limitations; completed short batches provide final evidence.
See [final Firebird acceptance](validation-ui3-firebird-0.17.json).

Actual final-pack merchant checks found and fixed a version-equals-19 condition
that disabled the original focus outline and candle layers for v20. Both languages
then passed the unchanged five-character, six-second phase, body-click,
state-preservation and v18 fallback assertions. Two pre-fix failures are retained.
Root and two independent reviewers inspected representative merchant images.
See [merchant acceptance](validation-ui3-merchant-0.17.json).

The title uses the original gold lettering, teal II, ten fire frames and the
authored single additive glow. The glow adds **15,602 shared bytes**, preserving
the old 32,274-byte atlas as an exact prefix. Original curve, old/new bilingual
atlas and native/ARM visual checks passed. Separate background GPU stars and
fog remain absent. See [title acceptance](validation-ui3-title-0.17.json).
All unnecessary runtime integrity and eager payload scans are removed; format,
decoding and selected-range bounds remain. Actual bilingual text checks covered
935 bindings, 200 dynamic routes and 230 model/UI calls, with zero unresolved
fields. This is systematic implemented-UI coverage, not every original text-key
and numeric-state combination.

The final split resources total **81,100,703 bytes**: common 74,756,230,
English 1,780,887, Chinese 4,563,586. Including the launcher, the bilingual install
is **82,084,385 bytes before saves**, 12,682,660 resource bytes below v19.
English-only and Chinese-only installs use 77,520,799 and 80,303,498 bytes.
Both complete packs reconstruct exactly; other pixels, animation payloads,
glyphs, strings, catalogs and timing are preserved. Common cards/UI retain fast
individual encoding; the smaller all-group candidate regressed first-visible
card pages and was not selected. Historical host timings retain their actual
source/resource identities. See [resource receipt](validation-ui3-glow-resources-0.17.json).

The final **76,697,282-byte calculator ZIP** and **115,479,696-byte offline HTML**
were saved successfully as authorized. ZIP contents and the HTML's embedded
WASM/common/locale bytes match their selected inputs; license and notice texts
are complete. The real file-URL browser flow passed start, S/O, reload Continue,
canonical save agreement, cross-language continuation and language cancellation,
with zero page errors or external requests. A warm eight-card collection
rendered 300 samples at 1.049 ms mean / 6.5 ms max with zero resource reads.
That is desktop-browser CPU timing; Canvas submission intervals are not display
or calculator FPS. See [browser acceptance](validation-ui3-browser-0.17.json)
and [delivery record](delivery-ui3-0.17.json).

Next work should prioritize the measured cold card-page pause and original
animation/layer gaps, followed by further storage reduction. Overall 95% original
UI acceptance, physical-calculator frame rate, title GPU stars/fog and all enemy
command offsets remain unestablished. The 30–40 MB installation target remains
unmet. Continue from these verified sources, preserve original gameplay/save
semantics and do not reintroduce integrity-only startup work. Public source
contains no original resources, embedded HTML, screenshots or gameplay saves.

## Historical b4f7ba0 stage: original title, on-demand loading and hybrid compression

The active source is persisted on `work/0.17-world-parity`. The verified
`b4f7ba06164734028af2c673f38fa918120d1341` checkpoint contains the new runtime,
combat status playback, original title atlas conversion, converter cleanup and
hybrid retained-image policy. The next integration adds the restored frontend
and map changes plus their meaningful UI fixtures. Do not relabel partial
validation below as final artifact or physical-device acceptance.

Implemented runtime behavior: no whole-pack FNV, split header digest, LZMA2 CRC,
or eager image/glyph/string/tile-catalog scans. Read only the directory and
language marker at open; decode selected data with its actual bounds. Save
version remains v15, with the unused footer written as zero and ignored on
read. Old saves are readable; old launchers may reject new zero-footer saves.
The frontend integration also removes repeated complete save encoding/FNV and
its 256 KiB buffer, using ordinary save association and relevant run identity.

The title now uses the pinned scene's gold text, teal II and ten fire drawings,
packed into one 32,274-byte sparse atlas in image 4. No image/clip IDs are added.
The separate glow and particles remain outside this implementation. Menu order,
mirrored focus marks, Continue panel layout and click regions are being checked
with real packs. Combat status icons, amounts and descriptions now follow the
same presentation clock as actions, including retained enemy identity. This
does not change canonical state or saved RNG.

Actual resource conversion at `b4f7ba0` preserves all retained pixel bytes
except the intentional Logo replacement, all 42,189 tile image descriptors,
1,184 encoded animation groups, clip timing, glyphs and localized strings.
The chosen `--fast-images ui` policy keeps common cards and UI in their existing
single-image encoding and groups other retained animation pictures. Shared
resources total **81,085,101 bytes**: common 74,740,628; English 1,780,887;
Chinese 4,563,586. This is 12,698,262 bytes below v19, before the launcher.
An all-group candidate saved 15,146,852 bytes but regressed first-visible card
screens; it is retained as comparison evidence, not the delivery default.

The paired 180-frame host benchmark used the completed b4f7ba0 binary. First
visible medians for v19 / all-group / chosen hybrid were: encyclopedia
2.334 / 15.063 / 2.022 ms; removal 2.617 / 13.419 / 2.801 ms; draw selection
5.280 / 37.070 / 3.680 ms. Hybrid warm windows had zero resource reads. Draw
selection preparation still cost 6.281 ms versus v19's 1.240 ms; these host CPU
intervals are not calculator frame rates. The unchanged v18 English resource
opened with 4 callbacks / 892,656 requested bytes, versus ed5caa1's 9,197 /
137,046,669 bytes. The observed host times were 0.166 / 778.478 ms, without
controlled host caches and excluding initialization and first rendering.

The b4f7ba0 native build passed. Its complete 73-suite run passed 70; remaining
failures were obsolete checksum/padding and covered-pixel fixture expectations,
being corrected at the next source checkpoint. Actual v20 English/Chinese
world UI passed 4,758 each, world text 6,963 / 6,746, and UI text 247 each with
zero unresolved fields. Those runs remain b4f7ba0 evidence. New ARM/Web builds,
final integrated regressions and Firebird acceptance follow the frontend merge.

Repository maintenance: main was advanced to ff5ef027 with Actions enabled.
The first main Actions run completed 70/72 native suites, portable ARM/WASM
passed, and old conversion assertions failed; the updated fixtures are now in
the work branch. ASan/UBSan remain enabled with -O1 to avoid the observed
20-minute unoptimized test run. Five already-merged work branches were removed;
the two branches with unique commits were renamed to
`archive/bulk-checkpoint-20261015` and `archive/bulk-recovery-20261013`.
Their history is preserved. Final verified source still needs synchronization
to main after the integrated gates close.

Original PCK/assembly, prior v18 bilingual resources and the user's Firebird,
CXII, injected boot and Ndless r2022 inputs are restored. The original NAND
must remain unchanged while a separate working copy is used for execution.
Earlier unsaved ui2 deliverables were pruned; build a new matching ui3 ZIP and
offline HTML, then save them with the user's explicit authorization. Never
claim the recreated artifacts are the old byte-identical ui2 files.

## Repository transfer and resumed work

The user transferred the repository to `maxmixalex/slay-the-nspire-2`, explicitly
authorized GitHub Actions and synchronization of verified progress to `main`,
and requested branch cleanup without discarding unique work. The recovered
development head is `a5391bbcfe42bfa656e63ec23c8daed4592c2011`; old main is
`efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0`. The old disabled-CI policy is superseded.

The user also explicitly authorized saving the two completed ui2 artifacts to
their ChatGPT Library. Environment maintenance removed the previous scratch
checkout, private inputs and unsaved ui2 files before this turn. Source history
is recovered intact from the transferred repository. Private asset recovery is
in progress; the previous checksums remain historical identities and are not
evidence that those old files are currently available. Do not claim a recreated
artifact is byte-identical until actually established.

New runtime policy: remove integrity/hash checks that are not original-game
requirements or functional prerequisites. Full-file FNV, integrity-only CRC and
eager catalog-wide validation are being removed. Preserve real parsing/indexing
and decoder bounds, gameplay RNG and functional lookup/cache hashes. Previous
documents explaining retention are historical evaluations, not current policy.

Parallel work is split between startup/runtime checks, original title Logo and
frontend behavior, private asset recovery and lossless storage improvements.
The integration owner handles Actions, source/main persistence, branch history
and combat status presentation. Native/portable CI runs without private data.
Actual UI and resource checks follow recovered inputs; no new acceptance is
claimed at this initial continuity checkpoint.

## Earlier ui2: runtime verified and packaged; upload was blocked before authorization

Final report and packaging checkpoint
`46f7ab9ed5292e07212a5b423c43e951f7d97a23` is pushed, read back and verified
against all nine changed blobs/modes. Packaging journal
`ui2-delivery-package-d6a9e9dfa878` is closed passed with unchanged source and
an independently matched log hash. All 12 ZIP members were read back; all
embedded HTML inputs match their executed runtime/resource files. The HTML
repack changes only the EOF license comment and preserves every inline script
and all other bytes. Conservative ARM/Web inputs at the report checkpoint are
byte-and-mode identical to actual runtime 4f50193b (396 / 395 files).

| Verified local artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `Slay-the-Nspire-2-0.17-ui2-bilingual-calculator.zip` | 81,484,351 | `a322d31e641794be211839981eb4eeec20d0413f3aa6e3f6a687a7579c03e7a1` |
| `Slay-the-Nspire-2-0.17-ui2-bilingual-offline.html` | 132,299,895 | `d9c325a77b53ac6546f2fa40e532d7c4a679f2d581ba733f3634be37421123c9` |

Persistent artifact storage is **not completed**. Automatic approval review
rejected the upload before the process started because these artifacts contain
private original game assets and require explicit user authorization for the
ChatGPT Library destination. No upload retry or alternate route was attempted.
The next delivery action is to obtain authorization for these exact two files
and that destination, then save them and verify both ordered results. See
`delivery-ui2-0.17.json`; keep runtime 4f, conversion 06c7 and packaging 46f7
identities separate. Source-only archive `feedback-stage13.tar` is 11,591,680
bytes, SHA-256 `0678b49cabb5d2cf3aa04b9c09ce2bcd8f406498bc9b576f262ee1b17d61ba6b`.

The second implementation stage is saved and executed at
`06c7fd16e6d4d47dc25b628913ecdeef3a4fe76d`. Test/report checkpoint
`5c884a2f1108c1c42e2114716f3a5a3192431fdf` is also pushed, read back and
verified against all changed file blobs/modes. Main and disabled CI
remain unchanged. Continue parallel work; original UI fidelity remains first.

Implemented at `06c7fd16`:

- Combat selections wait for their own effect-stream cue before rendering or
  accepting keyboard, pointer or direct UI actions. Late answers delay only
  unexecuted continuation. Original Knowledge Demon selection waits are
  1,000 ms Normal / 250 ms Fast. Save and gameplay RNG behavior are unchanged.
- Intent tooltips use the same displayed damage, repeat count and actor identity
  as the projected icon, including the battle visible before a pending choice.
- Append-only v19 resources add the original merchant outline skin and sampled
  behind/front candle layers. Old indices and payloads remain unchanged; v18
  fallback is retained. New encoded artwork is 535,140 bytes, with a 4,732-byte
  largest decoded new frame and no larger resident caches.
- Startup reuses spare directory space for descriptors and bounded geometry
  summaries. Full FNV, startup structure/alias checks and first-use pixel
  validation remain enabled. No new Content allocation is introduced.

Closed execution at `06c7fd16`: world UI passes 4,497 synthetic checks and
13,951 for each of English/Chinese v18/v19. World text passes 10,541 synthetic,
6,963 English and 6,746 Chinese checks for both resource versions. Boot locale
passes 498. v19 UI template routes pass 34,723 checks per language, covering
935 bindings and 200 dynamic bindings with zero unresolved values. Actual
WASM v19 split/full frames match in ten pairs; both v17/v18 languages load,
and wrong-language plus common/locale corruption are rejected. These results
retain their original execution identity. Final runtime `4f50193b` subsequently
passed actual offline Chromium acceptance with its own v19 HTML.

Merchant conversion, preservation/split and actual-asset rendering gates are
closed. The independent original shader-graph comparison measures at most
0.500093/255 per-channel error including output quantization; it is not a
Godot framebuffer/HDR or complete particle-system comparison. The finite
six-second sampled loop is explicitly not equivalent to unbounded shader
TIME. Cinder/additive particles and the Necrobinder-specific flame are open.

The v19 split is 87,405,187 bytes common, 1,801,249 English and 4,576,927 Chinese,
93,783,363 bytes total. Both reconstructed full streams are byte-identical to
the converted packs. Conversion remains `06c7fd16`; save format remains v15.

An independent review found and fixed the visible slot-reuse defect: a new
enemy identity could inherit the old occupant's projected intent. Runtime
`4f50193b212bb5d27e4a7e82deaa7622a5d4757d` clears only that stale slot intent,
preserving final convergence and adding no fields. Its native/ARM/WASM builds
and final bilingual world-text, WASM compatibility/corruption and actual
Chromium language/collection flows are closed passed. ARM is 978,599 bytes,
SHA-256 `878fee0120b0c6e2edb4c652021911403f58bf476055370a9e1552d5b0bd3a74`,
with allocation 16,513,432 bytes and 263,784 bytes of margin below 16 MiB.
WASM is 5,388,416 bytes, SHA-256
`2e025eddd673e2b2d28f72bd567cecb4285e4969e1e12e774eea76cb76da0072`.

The real Fogmog/Eye slot fixture passes Normal/Fast intent, hit-region,
Tooltip-title and unchanged-save checks in both v19 languages. Its initial
synthetic rename-pixel assertion failed because the synthetic profile has no
glyphs. Test-only checkpoint `5c884a2f1108c1c42e2114716f3a5a3192431fdf`
requires rendered title change with actual packs and explicitly checks the
synthetic absence of glyphs, retaining every other assertion. It links the
unchanged 4f core after verifying all 512 other recorded source inputs. Final
synthetic/English/Chinese runs pass 4,598 / 4,614 / 4,614 checks. No screenshots
were requested in those runs, so counts omit capture I/O checks and cannot be
compared directly with the earlier capture-enabled totals. Keep the initial
failed journal and the independently passed pre-fix actual-pack runs.

Final v19 bilingual Firebird Continue/save/80 ms Enter exit is closed passed
with the exact 4f launcher on CX II CAS / OS 6.4.0.74 / Ndless r2022. Both
languages continued the original Neow save, saved normally, used Save and Quit,
then held Enter for 80 ms at the title Quit confirmation. Both remained in the
OS document browser seven simulated seconds after release. All four installed
files were completely read back over USB and matched their source bytes.
The original save and both final 230,088-byte v15 exports are byte-identical,
SHA-256 `9965d829dd1231146cdc7ee75c1d0e6a0ce07556dc1819b4f9d3ed0c46911d4c`;
the exact 4f core also loaded, validated and re-encoded all three identically.
Language preferences are exactly `NSL1zhs\n` and `NSL1eng\n`.

An ordinary USB replacement of the large common resource was rejected by
TI OS for temporary storage space before transfer. The failed attempt is
preserved. Removing the old resources in the isolated working image before
upload allowed the complete update; existing save/preferences were retained.
Installation instructions explain this observed upgrade case. A first save
validator build failed to launch because unversioned `clang++` was absent;
the separate Clang 18 retry and all three save roundtrips passed. The original
baseline NAND image is unchanged. None of these emulator results is physical
calculator acceptance.

Final v19 initialization observed 49.774567 simulated seconds in Chinese and
48.061584 seconds in English, with zero resource-read failures. Chinese full
FNV validation alone took 23.175323 seconds. USB readback had warmed host
caches, English followed Chinese, and neither cache state was controlled.
The timer covers selected-resource validation and App initialization only;
it excludes OS boot, Ndless installation, initial file opening, language
selection waiting and the first frame. These observations are not a cold A/B
or evidence of physical-device acceleration. Full execution identities,
closed journals and retained failures are in
`validation-platform-ui2-0.17.json`. Final artifact packaging passed as recorded
above; persistent storage alone awaits authorization.

The old content cache-pressure fixture failed at `06c7fd16` and reproduced at
baseline `61970600`: it created 159 keys against today's 320-entry cache. The
`89dc0f80` test-only correction uses public capacity plus 32 and asserts actual
eviction before testing failed reads. Its 4f runtime execution is closed passed
with 1,554 checks (`startup-content-fixture-fixed-82aa9247e26c`); neither earlier
failed journal is changed to passed. The
cache-age test's initial non-executable output was separately repaired by a
mode-only change, preserving its binary SHA, and its retry passed.

Controlled same-v18 startup A/B now has a closed, independently verified
retry: `startup-v18-ab-complete-47743de9d363` contains all 28 measured runs and
four excluded warmups, complete inner status and matching outer log SHA. Warm
host open medians were 796.041 to 764.588 ms Chinese and 769.710 to 743.533 ms
English (3.95% and 3.40% lower); overlapping ranges limit timing conclusions.
Tile-index FILE requests fall deterministically from 1,382 to 1,155 and
10,476,206 to 10,008,974 bytes per language. FNV/retained-image requests are
unchanged. These are requested FILE I/O counts, not physical storage traffic.

The earlier outer journal reported success while its inner report contained
only 27 of 28 measured runs and its log hash did not match. That attempt is
incomplete and supplies no accepted speed result; preserve it unchanged. The
historical Firebird target timings below are not updated from host medians.
No validation bypass, physical-device cold-start result or whole-game fidelity
percentage is claimed.

A separate fresh-OS 06c7 + unchanged-v18 Firebird observation is closed passed,
with zero resource-read failures and normal 80 ms Enter exit. It observes
49.394989 simulated startup seconds: FNV 23.056000, retained images 12.463196,
tile indices 12.157898. The host caches were not made cold; it is one emulator
observation, not a strict cold A/B or a causal speedup against the old 619 run.
See `validation-startup-reuse-0.17.json` for both exact identities.

The first feedback calculator ZIP and offline HTML were already saved and
provided to the user from report checkpoint `3bda3cf5`. Their exact identities
remain in the historical delivery reports; the next version must preserve
actual build/conversion/execution identities and verify all packaged members.

## Delivered baseline: final runtime and bilingual target flow verified

The integrated runtime is **`61970600a8d3f7fb2833af13f82f09c86105ebec`** on
`work/0.17-world-parity`. Each coherent stage was actually pushed, its branch
read back and changed blobs/modes verified before long builds or execution.
Its source-only archive SHA-256 is
`89d30cd066189065f2842f3c1377e178d20fa73e3c90f7ceace8fe3ed444aa91`.
The pinned reference remains **v0.111.0 / 41cef1ea**. Original UI fidelity stays
first, performance second. Main remains `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0`;
remote CI remains disabled. Audio, multiplayer and Timeline remain excluded.

### Implemented feedback corrections

- Event options now expose original-derived keyword/card/relic/potion hover
  associations, keyboard/pointer focus and paged V details. Dynamic choices
  use already prepared offer/state data without generating new RNG outcomes.
  The audited rows cover 57 event models, 147 relic rows and 39 extra potion
  rows. The DARK_ORB identifier and upgrade-preview title semantics are fixed.
- Selection/removal headings use the central localized rich-text formatter.
  Amount, HotKey/Hotkey and other known fields are bound through their real UI
  routes; unknown fields are preserved rather than silently fabricated as zero.
  Six live-value models and independent Rolling Boulder instances are covered.
- Scaled card focus outlines follow the real face geometry, scale and rotation.
  Small-card exact-background caching and resident glyph metrics reduce dense
  grid cost without freezing animation or changing cached-render output.
- Five original class energy counters show current/max energy and zero-energy
  appearance. Intent icons use original thresholds with outlined damage and
  smaller repeated-hit counts. Banner motion, colors and independent fades
  follow the pinned scenes. The original Proceed art is used in the shop.
- Merchant placement, positive scene scale and class shop animation loops are
  preserved. Clicking the merchant body opens the shop; the extra goods button
  is removed. Original GPU particles, focus skin and flame shaders remain open.
- Enemy actions begin during the enemy banner. Audited Nibbit/Louse command
  waits advance the next actor while prior animation tails continue. Fast now
  chooses explicit short command waits on one real 33 ms display clock; it
  no longer doubles every banner, skeleton and health-bar clock.
- A bounded App-only 6,032-byte CombatFeedback projection displays HP, block,
  Osty HP, energy, old intents, damage and lethal/removal/reused-slot feedback
  at action cues. It neither copies the full State nor changes saves or RNG.
  A shared input guard blocks cards, potions and EndTurn while effects are
  visibly unfinished; actual suspended choices remain operable.
- Shared image/animation data is stored once in a common resource, with separate
  Chinese/English text and glyph files. All boot phases, errors, preparation,
  language controls and the browser shell have bilingual bindings. The small
  Noto boot glyph subset has its separate OFL attribution.
- Ndless shutdown finishes resource close/report I/O before releasing the
  foreground and skips a discarded post-Quit render. The original 80 ms Enter
  trigger now returns to the OS instead of re-entering the launcher.

These are implemented behaviors, not an overall 95% visual-parity claim.
The authoritative simulation is still synchronous. Status/power strips,
card/pile state and actual choice layers still read canonical state, so a
choice can appear before the enemy presentation finishes. Intent tooltips are
suppressed during the active projection to avoid showing next-turn values.
Unreviewed monster effects retain the existing move-start/timing fallbacks.

### Closed native and Web execution

| Gate | English | Chinese | Actual execution checkpoint |
| --- | ---: | ---: | --- |
| World UI, complete clock/feedback/input/HUD/merchant suite | 11,610 | 11,610 | `61970600` |
| Map UI, actual preparation/render/fade lifecycle | 8,598 | 8,598 | `61970600` |
| Frontend settings/save lifecycle | 567 | 567 | `14a20900` |
| Event reader/shop interactions | 1,462 | 1,462 | `14a20900` |
| World event layout | 484 | 484 | `14a20900` |
| Event tooltip actual v18 pack | 402,632 | 402,524 | `14a20900` |

The standalone banner suite passes 21 checks and synthetic event hovers pass
21,734 at `14a20900`. At `9d8cb1fd`, all four v17/v18 Chinese/English template
audits pass 935 bindings, 200 dynamic bindings, 230 real UI routes and 34,723
assertions per pack, with zero unresolved fields. Standalone text (167), codec
(1,261) and map-travel/save observer (1,221) also pass there. Keep these actual
identities; later documentation or platform-only commits are not new executions.

The latest six native journals are closed passed with unchanged frozen source.
The independent native receipt audits 27 journals, including retained failures,
with log and source-inventory hashes. See `validation-feedback-native-0.17.json`,
`feedback-ui-0.17.md`, `fast-mode-clock-0.17.md`, `event-option-tooltips.md` and
`ui-template-audit.md` for coverage and limits. Private screenshots were visually
inspected for the five class HUD/shop styles, both languages, and effect timing.

Actual WASM split/full/legacy loading, corrupt-input rejection, bilingual HTML
and Chromium language/collection interaction pass at
`14a20900035b5e554e076263fc22fe7611d2e410`. A later closed equivalence gate proves
all 305 conservative Web compilation/packaging inputs byte- and mode-identical
at runtime integration `61970600`; all embedded WASM/common/locale inputs match.
The final attribution update intentionally changes NOTICE only, so a delivered
HTML must prove exact equality outside that EOF documentation footer and retain
`14a20900` as its actual Web execution identity.

### Performance, validation policy and resource size

The controlled warm-host card-rendering A/B remains the `069f6d51` execution against `e8444322`,
using identical v17 packs, 180 advancing frames per window and unchanged save
bytes. Mean update+render time fell from 1.047/1.275/2.056 ms to
0.549/0.639/1.585 ms for Chinese library/removal/choice, and from
1.894/2.075/3.045 ms to 0.615/0.655/1.599 ms for English. Static Chinese library
and removal windows made zero resource callbacks. These are host measurements,
not physical calculator FPS or a claim that every scene is equally accelerated.

Actual v18 split resources are 86,870,047 bytes common, 1,794,081 English and
4,569,759 Chinese: **93,233,887 bytes** together. The equivalent two full packs
would total 180,103,614 bytes, so sharing saves **48.23%**. Payload reconstruction
and stable old indices are verified. This does not reach the 30–40 MB goal.
The resource conversion checkpoint remains `069f6d51`; save version remains 15.

The final `61970600` Firebird Chinese startup profile observes 50.472626
simulated seconds: full integrity 23.055786 (45.68%), retained images 12.463165
(24.69%), tile indices 13.376617 (26.50%). This replaces the old candidate as the
current target observation, without relabelling it as physical-device cold
performance. It excludes language-selection waiting and pre-selection opening;
USB transfer and host caches were not made cold. No per-stage I/O/CPU split was
captured. The first device journal failed on stalled USB export, but its later
normally exported profile was independently matched against the fresh flash
record; the failed journal remains failed.

**Full-file integrity and startup structural checks run on every startup;
the existing first-use payload checks remain enabled.** FNV is a substantial target cost, and it is not
covered by all downstream paths: retained LZ4 images and text/glyph payloads
have no equivalent independent CRC. Comparing split SHA labels does not rehash
the common file. Next implementation should reuse a sequential encoded-byte
stream for the existing hash and validators, removing repeated reads while
requiring every existing startup check before publishing Content. This is evaluated
follow-up work, not an implemented speedup or permission to disable validation.
See `rendering-performance-0.17.md` for the concrete alternatives and bounds.

### Target identity, failures and delivery state

The final ARM launcher is **974,608 bytes**, SHA-256
`6037aaac82d9a70daaad107008079e2bc2ff083624a77ff332f007b6f64c0cab`.
Its Zehn allocation is **16,507,640 bytes**, leaving **269,576 bytes** below
16 MiB. WASM is 5,384,458 bytes, SHA-256
`6de70bc1842f3f24a80662d401bf3ba0af7bffde41aae9266ad612f37fbfe9d0`,
with its actual runtime identity `14a20900`. Build and execution are separate.

The final ARM ran on a freshly rebooted isolated CX II CAS/Firebird image with
Ndless r2022 and a matching new snapshot, not an older RAM snapshot. Two actual
80 ms Enter Quit confirmations retained the foreground while held and returned
to a stable OS document browser after release; an alternate key was not used
to substitute for this trigger. A new Chinese run produced an actual 230,088-byte
v15 save exported over emulated USB; original-core decode, validation and
canonical re-encoding are byte-identical. Direct Chinese and English Continue
then each reached the same Neow event with 80/80 HP, 99 gold and the same options.
Each used normal save, Save and Quit, and the original 80 ms Enter Quit trigger.
After seven seconds the OS document browser remained visible without re-entry.
The new-run, clean Chinese-continued and English-continued saves are byte-identical,
SHA-256 `9965d829dd1231146cdc7ee75c1d0e6a0ce07556dc1819b4f9d3ed0c46911d4c`.
The English language preference was normally exported as `NSL1eng` plus newline.
The bilingual device journal `final-firebird-save-continue-0158306e39a1` is
closed passed, exit 0; the earlier cancelled-USB journal remains failed.

Earlier failures remain visible: `9d8cb1fd`'s Fast second-hit banner assertion
found the real global-clock bug; `14a20900`'s Toasty fixture had not resolved an
acquisition continuation; its map fixture had not rendered the mandatory
loading page. The final fixtures exercise the real lifecycle without weakening
assertions or changing production map timing. Permission-only launch failures,
stalled USB exports and an ambiguous short-key menu sequence are also recorded
separately from their successful retries. See `ndless-exit-handoff-0.17.md` and
the platform validation receipt for exact target acceptance boundaries.

Next: persist the final reports and attribution checkpoint. Package one bilingual
calculator ZIP and one offline HTML from the exact executed ARM/WASM/resource
inputs. Read every ZIP member and embedded HTML input back; keep actual runtime
identities separate from the packaging/documentation commit. Save both private
deliverables before presenting links. Original packs, assembly/IL, calculator
images, captures and embedded HTML remain outside the public Git repository.

# Work state — 2026-10-21

## Current: bilingual delivery generated and verified — source38 receipt

The two calculator ZIPs and two offline HTMLs have been generated and read back.
The private delivery manifest is `packaged_and_verified`; the final independent
verification and inline-JavaScript syntax journals have both completed with
`passed`, exit code 0 and unchanged frozen source. All five formal packaging and
verification journals are closed. No compilation or resource conversion was
performed while assembling this delivery.

| Deliverable | Bytes |
| --- | ---: |
| `Slay-the-Nspire-2-0.17-zhs-calculator.zip` | 82,126,673 |
| `Slay-the-Nspire-2-0.17-eng-calculator.zip` | 79,352,063 |
| `Slay-the-Nspire-2-0.17-zhs-offline.html` | 115,124,029 |
| `Slay-the-Nspire-2-0.17-eng-offline.html` | 111,423,361 |

The [public delivery receipt](delivery-0.17-bilingual.json) records each filename,
complete byte count and SHA-256, plus the five journal basenames, hashes, actual
checkpoints and source-stability results. The delivery manifest SHA-256 is
`e345adb7549576d29538b12e585cb05300c69a8470a8d4cb00f0e05e0c54e6b5`.
Every member of each 12-member calculator archive was read back and checked;
both HTMLs' complete embedded WASM and resource inputs match their bound files.
The final independent verification also checked the manifest artifacts and
SHA256SUMS. Both emitted inline scripts passed `node --check`; that operation
only parses JavaScript and does not run a browser, DOM, localStorage or WASM.

Execution identities remain distinct: source36
`127605b7621c424e7990fc7bdde6bfb2a2fa7f5e` is the ARM release and final Chinese
Firebird checkpoint; source35
`486fd83f6c7838cbab7590ac8619208d7aa86d09` is the actual native/WASM execution;
source33 `2d5c6384cd6a5cab14877c1f0371a335760b00e0` produced both resource packs.
The remotely preserved source37 report checkpoint is
`27e762c00e3431cd8daf85d2020513b419390433`; its identity proof confirms all 488
declared runtime inputs and both resource packs are unchanged from source36.
This source38 receipt changes only documentation and does not relabel previous
tests or packaging as new runtime execution.

The packages are prepared for private delivery and remain outside the public
repository. This repository records metadata, not resource payloads, embedded
HTML, screenshots or device images; this entry makes no upload-completion claim.
Both calculator editions install `nSpire2.tns` and `nSpire2.nsp.tns` together
with their exact names and require Ndless r2022 or newer. Saves still use v15;
normal archives include neither saves nor profiling markers/reports.

The [feedback report](calculator-feedback-0.17.md) and its
[validation metadata](validation-calculator-feedback-0.17.json) retain the actual
UI, gameplay, emulator and performance evidence, including failures. Packaging
and syntax checks add no original-engine parity, browser-runtime or physical
calculator FPS result. Original UI 95% acceptance, the 30–40 MB resource target
and cold-start acceleration remain unestablished. English Firebird was not run
in this round. Main and disabled remote CI remain unchanged.

Next: continue the remaining original UI composition, animation and keyword
coverage work, and measure cold startup and battle-entry costs on physical
hardware. Preserve the current paired artifacts while investigating further
bounded resource reads and storage reductions; do not treat fewer bytes read
or visible progress as a measured FPS or cold-start speedup.

## Previous checkpoint: source36 runtime gates completed; bilingual delivery packaging — source37

The executed release is source36,
`127605b7621c424e7990fc7bdde6bfb2a2fa7f5e`. All 572 source files and the two
changed contents were read back from the remote branch before its ARM build.
The source-only archive SHA-256 is
`837d04f86808f2e7c3c85d5c75a877bb9f2433e1712f4b85d9d7dfb78dc1022e`.
The final launcher is 939,953 bytes, SHA-256
`0662043c138649a790e7e6047cc2dc9dd0cbcd3dd7a1648edc14741612c1f2fd`.
Its Zehn allocation is 16,311,912 bytes, leaving 465,304 bytes below 16 MiB.
Source35 remains the actual native/WASM execution checkpoint; source33 remains
the resource-conversion checkpoint. The target-specific proof verifies all
488 declared inputs and identifies only the Ndless main change, with native
and web dependency/tool/object identities unchanged. Do not relabel those
earlier executions as new source36 tests.

The exact final launcher was installed in an isolated Firebird CX II CAS
session and downloaded back over emulated USB with identical complete bytes.
It completed all eight startup stages, continued the saved B run with all
three seed fields intact, and re-saved its 230,088-byte v15 save byte-for-byte.
The same 80ms ENTER Quit that caused the earlier re-entry now passed: while
the key was held, app.quit was true and the program retained the foreground;
five simulated seconds after release the OS document browser was visible,
without automatic reopening. No shorter press, alternate confirm key or
forced emulator exit was used for this final regression. The underlying
Ndless/OS queue mechanism is not established by that one passing case.

The normal 4,393-byte performance report was exported through USB; startup,
first-render and first-present totals exactly match paused read-only counters,
and resource failures are zero. The final session journal completed with
status passed and unchanged frozen source. Its summary SHA-256 is
`18af0a4854b8964fdebe60db7501dcac79e09fb675bc17efb327a5a32ebe312e`.
Its 41.741-second startup inherits the previous OS/filesystem cache snapshot,
so it is not evidence of acceleration from source33's 52.353-second cold run.
All emulator sessions ended normally. No physical-calculator FPS is claimed.

The broader source33 session separately verified visible startup and combat
preparation, a killed enemy disappearing, a power amount, keyword hover/detail,
player-turn banners and process-restart seed behavior. Run A and Continue used
E80C6A8B; a normal new run B used 0651E28D. Preserve that actual execution
version and the first-profile snapshot-recovery provenance. English, dense
statuses and rich paragraph wrapping have source35 actual-pack native/WASM
evidence, not an English Firebird run.

The consolidated [feedback report](calculator-feedback-0.17.md) and
[validation metadata](validation-calculator-feedback-0.17.json) cover the
nine user reports, the 18 affected sanitizer executions, 48 reviewed frames,
both language packs, controlled I/O measurements and retained failures.
Original UI 95% acceptance and the 30–40 MB target remain unachieved. Main and
disabled remote CI are unchanged; private resources remain outside Git.

Next: freeze this documentation-only checkpoint, verify its unchanged runtime
inputs, create the exact paired bilingual calculator ZIPs and offline HTMLs,
read all members and embedded inputs back, and record final delivery hashes.
No further compilation or resource conversion is needed for this pair.

## Previous checkpoint: final bilingual UI and bounded Ndless exit handoff — source36

Source35 is remotely preserved at
`486fd83f6c7838cbab7590ac8619208d7aa86d09`, with all 572 files and four
changed text contents read back. Its source archive SHA-256 is
`b7f843e81f002f74f987b98b405b56bcf15f5055f23e493082465fba27720268`.
The ten affected fallback ASan/UBSan regressions and eight actual-pack
Chinese/English world-text, world-UI, keyword and device-feedback executions
all passed. LeakSanitizer remains disabled for the documented sandbox limit.
Both languages also passed 18 card/HUD/selector captures; all 36 frames and
12 separate keyword/status/banner images were visually reviewed. The dense
status label no longer covers or intercepts a power, English keyword paragraphs
wrap whole words, and the unrelated invalid-seed fixture notice is gone.
This establishes the inspected behavior, not a numerical original-UI fidelity.

Source35 ARM and WASM builds passed. The ARM launcher is 939,389 bytes,
SHA-256 `493e5fb8df0524fb90bafb714009f875ef4cbb3a620b753faa2e136a975e8778`;
allocation is 16,311,248 bytes, leaving 465,968 bytes below 16 MiB. WASM is
5,200,648 bytes, SHA-256
`b4e6ccd0a9c9c4b8db49c90bb59a60e28198a3f1d50192067eb8eb7a35cafd4a`.
Verified object reuse includes dependency/tool hashes and byte-identical
re-linking of the preceding ARM/WASM artifacts. An initial WASM proof failed
because its temporary output basename changed only the name section; retain
that journal. Repeating in an independent directory with the original basename
passed the full-file comparison without weakening the check.

Source35 actual Chinese and English native/WASM replays passed all five sampled
RGB565 frames, all stage read counts/bytes, unchanged canonical saves, zero save
callbacks and fixed 16 MiB WASM memory. A single same-v16 control observation
also matches source33's stage I/O exactly; the historical ABBA 91.026% read
reduction remains applicable to this one controlled replay, not to FPS.
Both private language packs remain the unchanged source33 v17 artifacts.

Source33's real Firebird sequence confirmed a killed enemy disappears while
other enemies remain, Block keyword hover/detail and the player-turn banner.
After an actual process re-entry, Continue preserved seed text E80C6A8B,
run_seed 2855106481599043927, turn 2, player HP 76 and the killed enemy's state.
The first profile was recovered byte-for-byte from the frozen snapshot, matched
to startup/first-render/first-present counters and cross-checked against NAND
data-page bytes. This is snapshot recovery, not a USB export. Cold startup is
52.352966 simulated seconds; no physical-device speed is claimed.

One 80ms ENTER Quit confirmation was followed by an unintended new invocation.
The r2022 loader already has an instantaneous no-key check, so a missing local
check alone is not a proven root cause. The next platform-only source stage
adds a bounded foreground handoff: sample all matrix rows and touchpad contact/
press at most every 10ms, require 100ms continuous release, and stop after two
seconds before the existing loader handles return. It changes neither the OS
input queue nor interrupt/timer configuration. ARM syntax and six extracted
helper probes passed, including held input, bounce and counter wrap. The same
80ms Firebird exit must still be repeated before claiming this fixes re-entry.
New-run seed and final launcher execution/packaging remain in progress.

## Previous checkpoint: dense status overlap and English word wrapping — source35

Source34 is remotely preserved at
`c466cbb46b65cf3805997c6bc05c67991c3ee45d`, with all 572 files and five
changed text contents read back. Its archive SHA-256 is
`bea336668a5fcbdfe1c2632cddf45c855f5e31657444161a21f7e2bd0f5471a6`.
All 325 declared runtime/build inputs remain byte-identical to source33.
The three corrected/extended fixtures passed all nine fallback/Chinese/English
executions under ASan/UBSan. The public WASM profiler also passed its actual
three-enemy save: 59 preparation updates, first visible render recorded
separately, then 360 combat frames with 20,480 bytes of subsequent reads.

Actual dense-status images exposed a production issue that hit-count tests
alone missed: the fixed `I Detail` label and its later command hit rectangle
overlapped the third power row. The next source stage checks actual rectangle
intersections, tries the opposite gutter and omits this redundant label/hit
when both gutters are occupied. Every power remains directly visible and
interactive; the keyboard shortcut is unchanged. The dense fixture now checks
the effective `hit_at` result for every status center and clears the unrelated
invalid-seed notice from its previous fixture before making screenshots.
English rich paragraphs also gain whole-word wrapping, retaining explicit
newlines, long-word fallback and rich styles. Chinese paragraph layout and
both private resource packages remain unchanged.

Source33's exact final Chinese pair has reached the title and a new run in
Firebird. Observed resource-open/App.init time is 52.352966 simulated seconds;
first render is 0.456818 seconds. This excludes Ndless/Zehn loading. Real
progress captures and all eight completed stages are available. Its first
new-run battle prepared 209 images/638 pages in 53 updates, and the observed
preparation scene occupied 54 complete frames/6.524841 simulated seconds,
including the visible transition. The player-turn banner appeared. These are
current-only emulator observations, not a same-pack startup A/B or physical
FPS. Seed/restart verification is still running. The next renderer must be
built and rechecked on the installed final pack before final packaging; do
not relabel source33 execution as source35 execution.

## Previous checkpoint: source33 runtime and source34 fixture validation

Source33 runtime is remotely preserved at
`2d5c6384cd6a5cab14877c1f0371a335760b00e0`: all 572 tree files and all
12 changed contents matched the read-back. Its source archive SHA-256 is
`cbb41b17d5b0eb319f07772f39731a069981c6767543391e34e4098299869964`.
The final ARM launcher is 939,160 bytes, SHA-256
`e0c4778626f5838b769343597d0267f7c81625feb38d325ab0517f6e1410b46b`.
Zehn allocation is 16,310,280 bytes, leaving 466,936 bytes below 16 MiB.
The final WASM is 5,199,273 bytes, SHA-256
`12f58985a1180b623e9860de3e63f86fd8fe06d628e76783ecc1c070c47ce577`.

The actual v17 Chinese and English packs are respectively 81,124,916 and
78,349,238 bytes. Their SHA-256 values are
`470a8e2a79338a3bd92d7cf9904b5aa1818f5b6a1ce23b849499e951cbab8f43`
and `7cdf55f9a909a6070b968aa8fbaafaebdad12f46e241a5db8451b4aef934ca7b`.
Both contain 45,785 images, 1,695 clips, 558 cards, 8,192 strings and 1,236
auxiliary entries. Original graphics payloads match the preceding pack;
Chinese has 11,900 glyphs and English 570, with all text/runtime codepoints
present at all five sizes. English resource strings contain no CJK characters.
Of 935 UI literal bindings, 620 map directly to original localization keys;
315 are port controls, diagnostics or legacy fallback prose, not original text.

Source33's ten affected ASan/UBSan regressions passed. Source31's fifteen
related regressions also passed after disabling only LeakSanitizer: its initial
invocation aborted because this sandbox cannot enumerate `/proc/.../task`.
Retain that failed invocation; do not claim leak checking or a single green
full-suite run. The source33 actual packs separately passed world UI checks
(5,820 each), all localized glyph/string metadata, the selected 48 appended
image decodes and 40 identical memory/range UI frames. They also produced
18 card/HUD/selector captures per language. Screenshots require visual review;
their existence alone is not original-engine parity.

Actual-pack keyword and world-text regressions exposed fixture assumptions
that synthetic content did not exercise. Keyword setup must finish the new
preparation scene before querying hand hit targets. Dusty Tome's original
English description does not append the Chinese template's `+`; retain correct
card-reference and save-stability assertions without inventing punctuation.
The Chinese world-text canonical-blade fixture also forced Combat while turn
was still zero; establish combat through the runtime entry point instead.
Keep these failed journals and rerun only affected paths after the next remotely
verified checkpoint. An actual runtime text-buffer observation confirms the
STAR_COUNT title and Divine Right description use 辉星. A small screenshot
was initially misread; no production string or pack change is warranted.

The final same-v16-pack source27/source33 ABBA run confirms 24,097,642 to
2,162,452 bytes across preparation, first visible render and 720 subsequent
frames (91.026% less), with all canonical saves unchanged. Preparation is
59 updates, maximum four cold images per update, versus source31's 236.
At the harness's 33ms schedule that is a 1,947ms waiting floor versus 7,788ms;
it is not observed device elapsed time or browser RAF timing. Actual Chinese
and English v17 observations read 2,224,072 and 2,113,480 bytes respectively.
Native/WASM match all five sampled RGB565 frames and all stage I/O counts in
both languages, with fixed 16 MiB WASM memory and no save callbacks.

Cold-start read callbacks decrease from 25,830 to 7,290, while bytes remain
approximately 117 MB. No startup speed improvement is established. Stage
timing identifies full animation-index decoding/validation as the remaining
hotspot; integrity and malformed-data rejection remain enabled. The public
WASM profiler is being updated to drain and separately report preparation so
that diagnostic input replay and visible-window timing respect the new scene.

Firebird execution of the exact final launcher/Chinese pack is in progress,
including real startup progress, new run, combat entry and seed persistence
across process restart. It is not yet accepted, not a physical calculator FPS
measurement, and should not be inferred from ARM compilation. Final packaging
and delivery remain pending. Main and CI are unchanged; 30–40 MB is unachieved.

## Previous checkpoint: bounded preparation batching and bilingual delivery

Source32 is remotely preserved at
`26e5407a6e497ef8db76a22881ef853621f249e1`, with all 571 tree files and
11 changed text contents read back exactly. Its source archive SHA-256 is
`7e81f764644b65d519b6441a5d0ba5dbe0102c826a7f6aa230a84828deede324`.

The source31 same-v16-pack, same-save ABBA benchmark completed with canonical
save state unchanged in all four processes and no save attempts. Including
preparation, first visible render and 720 subsequent frames, read traffic fell
from 24,097,642 to 2,162,452 bytes (91.026%). This is native I/O evidence,
not calculator FPS. One-image-per-update preparation also imposed 236 updates,
or a 7,788ms minimum at 33ms/update. The next source stage therefore batches
at most four cold images per update and asserts that bound in the benchmark;
the new waiting cost must be measured again. Startup read calls fell from
25,830 to 7,290, while bytes remained approximately 117MB. Shared-host timing
was noisy and does not establish a startup speed improvement.

Source31 ARM compilation passed: 937,765-byte launcher and 16,307,296-byte
Zehn allocation, 469,920 bytes below the 16MiB limit. This is not execution
evidence. A new isolated Firebird validation is being prepared for the final
runtime/resource pair, including actual progress-screen captures.

Source33 also preserves every runtime-only Unicode symbol in both language
font subsets (including checkmarks and arrows); isolates canonical/detached
card text from live Parry, Sword Sage and targeting modifiers; and shares that
same canonical text with reference-card miniatures. Browser packaging now
accepts explicit language, pack and destination arguments, with an English
control/help/error shell as well as the original Chinese shell. Both templates
passed JavaScript syntax checks. The final packs, UI captures, native affected
tests, ARM/WASM builds and device measurements remain pending.

## Previous corrections: bilingual and layout — source32

Source31 is remotely preserved at
`62d310779e796a60d3b247589218a7a9cd76f9ca`: all 571 tree files and all 83
changed text contents matched the read-back. Its frozen source archive SHA-256
is `510264cf5543d1768d96b9254050abd5da20cf255a4af3017dfe70fb9cd38d71`.
Native affected-path and ARM/performance work has begun from that snapshot;
no result is accepted merely because the work was launched.

The next small source checkpoint corrects confirmed review findings before
the final bilingual conversion: preserve the portrait's 36 x 40 transparent
canvas instead of compressing its visible 27 x 40 area again; use original
character-pool icons in the library with full-name hover labels; render card
references in keyword tooltips through the same scene-based card face.
Original Nightmare text now binds each stored card and its three copies;
canonical card defaults retain detached growth/enchantment snapshots.
Missing v17 text keeps v16 title fallbacks, and legacy event text folds the
single-player conditional before packing. Short syntax, 1,020 standalone
template checks and five synthetic localization checks passed. Runtime and
actual Chinese/English rendering checks remain outstanding. Startup and
combat measurements must include preparation and any imposed frame waits.

## Previous implementation: calculator feedback, bilingual UI and cold preparation — source31

The owner reports approximately 70% UI fidelity on a physical calculator and
specifically identifies missing card framing, character focus motion, death
removal, visible power stacks, turn banners, repeated startup seeds and long
cold/entry stalls. These reports supersede earlier aspirational percentages.
UI fidelity remains priority one; measured performance remains priority two.
The reference stays v0.111.0 / 41cef1ea. Main and remote CI are unchanged.

This coherent source stage is being checkpointed **before** long compilation,
resource conversion or emulator work. No new runtime/pack pair is accepted
until its frozen-source checks complete. The prior accepted pair and its
measurement limits remain recorded below.

Implemented source changes awaiting those checks:

- A common card face follows the original 300 × 422 scene's portrait, frame,
  border, type plaque and description rectangles. Energy/star costs do not
  push the portrait downward. Readable 8px text and final-line ellipsis are
  explicit calculator adaptations. Hand, rewards, merchant, detail and deck
  confirmation use the same renderer. Original top-bar portrait, backdrop and
  potion-slot assets extend the v17 auxiliary catalog to 1,236 entries.
- Character focus restores the original 1.1 scale and 500ms ExpoOut release,
  with the 500ms information-panel slide. Pointer focus changes the preview;
  beginning a run still requires the separate confirmation.
- Every collected creature power receives its own visible icon/amount and
  hover/click target, with compact wrapped rows. Amounts are no longer capped
  at 99+ or folded into a +N icon. Dense arrangements on a 320 × 240 screen
  still require visual checks. Original power descriptions replace old
  paraphrases through the locale bindings.
- Removed enemies finish their death track and leave the renderer; downed or
  reviving actors retain their body. The constrained renderer uses a bounded
  opacity fade in place of the original death noise shader/particles, and
  does not claim pixel-equivalent death VFX. Dedicated checks distinguish
  permanent removal, restored corpses, Test Subject revival and detached
  Decimillipede segments.
- Player/enemy turn text comes from original gameplay_ui keys. Presentation
  uses the original banner tween durations, label motion and enemy 2→1 zoom,
  and queues the next player label behind the observed enemy animation clock.
- Platform-provided new-run entropy fixes the process-local seed counter.
  The auto-generated displayed seed can be entered manually to reproduce
  both legacy and named streams; explicit seeds and loaded saves do not draw
  fresh entropy. A missing/zero host source keeps deterministic test replay.
- Separate Chinese/English v17 packs reuse all existing animation/tile
  payloads and rebuild localization/glyph directories, with original keys for
  cards, powers, potion and world text. Regent's resource uses STAR_COUNT's
  original title, 辉星. Public source contains bindings and keys; original
  localized payloads, images and fonts remain private.
- Cold startup exposes real per-phase work progress using a tiny built-in
  display before pack fonts/images are trusted. Integrity hashing uses
  bounded 64 KiB reads; repeated descriptors avoid duplicate validation.
  Decoded-image entries increase from 256 to 320 within the same 748-page
  pixel pool. Planning APIs do not perform I/O or alter cache ages.
- Combat preparation selects only current/next-idle tracks within 288 entries
  and 640 pages, removes exact aliases, reads in stored frame order, and yields
  after each cold image. The animation clock freezes until preparation ends.
  Diagnostic reports separate preparation from visible combat. Progress alone
  is not a speed claim: the new benchmark includes preparation, first visible
  frame and subsequent windows, and verifies the save remains unchanged.

Next gates: remote tree/content read-back; frozen native affected regressions;
strict original-locale template audit and both v17 conversions; actual-pack
Chinese/English captures; same-pack source27/current cold preparation A/B;
ARM allocation/build and bounded Firebird execution of the new pair. Retain
all failed journals. Native agreement, modeled cache savings and Firebird
update rates remain distinct from physical-device speed and original-engine
visual acceptance. The 30–40 MB storage target remains unachieved.

## Previous: source27 UI/runtime and final-v16 device gates completed — 2026-10-21

The built runtime remains **source27**,
`9d1e2a90ef5e942bd93fe304cd47eda0d88d3f69`. Source28 is remotely preserved at
`c92b1733f1b54bff820583d8f6ab2d62fb56e824`; its 540 tree files and two changed
contents matched the read-back, and its source archive SHA-256 is
`94fd5a90eb00c94f7743bc85ea71a395440c8a39a42fd58d27411199e280a747`.
It changes a pressure-test fixture and documentation only, with 274 runtime
input files byte-identical to source27. Later report commits do not identify
new runtime binaries. See the consolidated
[execution report](source27-ui-performance-20261021.md) and
[machine-readable receipt](validation-source27-ui-performance.json).

The source25 native completion chain is now closed: all **64 registered tests**
have a completed passing run from the same frozen sanitizer build. The initial
full invocation passed 60 and timed out four at 300 seconds; only those four
were rerun with longer budgets. The initial failure remains retained, and this
is not a single green full CTest invocation or a full source27 rerun. Source28's
four affected ASan/UBSan tests and corrected Release fixture also passed. The
original source27 fixture assertion failure remains preserved, with no
sanitizer diagnostic. Runtime source did not change to fix that test.

Source27 ARM and WASM built successfully. The launcher is **911,249 bytes**,
SHA-256 `79a095948a2297ea9a3ae4e2dba6c11a6c4743c63ec5a7993b792c17e0a66789`;
linked allocation is **16,245,852 bytes**, within the existing 16 MiB bound.
WASM is **5,145,486 bytes**, SHA-256
`8688714c7d6c23dbec25e9729e14596242c0a84ff5a3dc869d5a660f270136bd`.
The fresh native 23-scenario capture preserves all **480 frames, 30 saves and
23 fixtures** from source25 exactly. Fresh WASM replay matches the native
frames and saves with the complete v16 file. The selected original clip oracle
and repeated-cycle cache check passed; they do not represent all 45,737 images.

The same-pack v15 WASM comparison reduces the real three-enemy save's second
180-frame window from **20,064,764 to 662,603 bytes** (96.70%). Neow's matching
window falls from **46,448,640 bytes to zero**. An additional source27-only
360-frame window makes zero callbacks in each of three declared scenes. A
separate final-v16 run of 720 frames per scene has the same window read counts,
explicitly checking that the new UI images do not defeat the cache in these
scenarios. These are resource callbacks, not browser or calculator FPS.

The actual source27/v15 Firebird comparison has completed with unchanged source
and a normal exit. The same saved battle crosses the authored idle-duration
threshold after 50 simulated seconds, versus 60 for source26. The following
matched ten-second window observes **13.6 update starts/s**, versus **1.2**;
maximum gaps are still **1,350 / 1,300 ms**, so cold stalls remain. Two later
current-only ten-second windows reach **28.4 and 30.0 starts/s**, with maximum
gaps **320 and 40 ms**; the last has zero reads, bytes and seeks. These are not
matched 25-fold FPS results. App startup remains **44.09 simulated seconds**,
effectively unchanged from 44.08. The source25, source26 and source27 retrieved
saves are byte-identical: 230,356 bytes, SHA-256
`c60baca9345faec5f106dad220c9249d5c414c427c697af789dc0d1af131c5ca`.

The independent fresh source27/v15 new-run route also completed Neow, Fishing
Rod, map, save, title and normal OS exit. Its initial Neow five-second window
observes **3.4 starts/s** and the next **28.2 starts/s**; the map entrance/display
window observes **13.4 starts/s**. Production save loading verifies the same
seed, generated first-act map and all 143 current-act layout slots as the
source25 run. The other two stored map records are still ungenerated; this
does not verify three generated acts. Different map-camera phases, entry
delays and profiler activation prevent a strict steady-map twofold A/B claim.
The corrected current report retains its earlier wording as a historical v1;
the executed journal, logs, saves and raw profiler output remain unchanged.

The final source27 launcher and full v16 file also completed a normal fresh
run through Neow, first-act map and three-enemy battle. Actual 320 × 240 images
show health bars, native buttons and a Defend card with nonempty Block hover
and V keyword details, operated by ordinary emulated keys/touchpad. The final
USB save remains the same 230,356-byte canonical file above. The completed
journal is passed / exit 0 / source unchanged; the backing flash file is
unchanged. This closes the final runtime/resource-pair execution gate.

The final-v16 reinstallation starts in **59.006317 simulated seconds**, with a
separate 0.836823-second first render; it is not the retained-v15 startup A/B.
An overlong automation confirmation key appears to have caused the OS to reopen
the launcher after its first normal exit. The complete first profiler record is recovered
from the preserved snapshot, with independent two-page NAND content matching
and earlier counter agreement. There is no full FTL/directory-path proof and
it is not called a first-run USB export. The second, title-only invocation's
USB profile is separate. Its shorter key confirmation returned normally to OS,
and final USB save retrieval completed. Keep the incident, both report
identities and recovery limits intact. The mixed first-run overlay sequence
records 236 frames / 497 reads / 20,514,241 requested bytes; it is not a steady
overlay FPS measurement or a continuation of the v15 30.0/s warm window.

UI fidelity remains priority one; measured performance is priority two. The
full v16 resource is still **80,879,519 bytes**, and its pair with the launcher
occupies **81,790,768 bytes** before saves. Neither 30–40 MB storage nor a
95% original-engine UI acceptance percentage has been achieved. Firebird is
not cycle accurate, no physical handheld was available, and cloud-browser
private-content testing remains blocked by the recorded automatic approval
review. Local Node/VM host logic and actual WASM execution have their own
completed evidence; no browser compositor FPS is claimed.

Next: continue original UI effects/timing and matched original-engine visual
acceptance first; optimize cold startup, first-use and overlay work second,
with a controlled map-camera comparison before further map-speed claims.
The final source27 delivery must bind the exact launcher/WASM and v16 pack
above, retain completed device receipts and exclude diagnostic markers/saves
from the normal installation ZIP. A separate bounded measurement archive may
contain JSON reports and logs, never ROM/flash/snapshots or resource payloads.
Preserve private art, ROM/OS, captures and embedded HTML outside Git; main and
disabled remote CI remain unchanged.

## Previous: source28 cache-pressure fixture correction — 2026-10-21

Source27 is remotely preserved at `9d1e2a90ef5e942bd93fe304cd47eda0d88d3f69`;
all 540 tree files and 14 changed contents matched the read-back. The source
archive is 10,004,480 bytes, SHA-256
`adb77cf4b17f9d0134dac0db4729755e6729ee358d5a06429cd43e1c843a7c22`.
ARM/WASM builds and exact scene checks are in progress from that frozen tree.

The affected ASan/UBSan run passed the reader's 8,572 checks, then correctly
exposed an obsolete assumption in the no-bitmap failure-injection fixture:
its 300 × 806-byte tile blocks fit in approximately 60 reader pages. Evicting
the decoded group cache no longer forces provider reads with a 64-page reader,
so the injected I/O failure remains hidden by valid resident data. This is
expected runtime behavior for an immutable source, not a decoder fault.
The original failed journal `source27-resource-map-asan-18eef84a4b7c.json`
is retained. No sanitizer diagnostic accompanied the failed test assertion.

Source28 aligns only the 300 referenced fixture groups to separate 4 KiB
pages and asserts that this working set exceeds the reader capacity. It keeps
the failure and invalidation assertions intact; unrelated groups remain
compact. The runtime, pack, compiler limits and source27 ARM/WASM inputs are
unchanged. Preserve this test correction before rerunning the affected gate.

The longer source25 events and relic-combo tests have now passed (759.22 and
777.64 seconds). Together with the earlier core rerun, 63/64 suites have
completed pass evidence; character-UI is the only still-running item. Its
original 300-second timeout remains documented with the initial full-suite
failure. No completed earlier check is being rerun unnecessarily.

Next: freeze the fixture correction, complete the four affected sanitizer
tests and the source27 native/ARM/WASM scene/device gates, then publish a
coherent validation receipt and matching development package. Runtime
executions remain attributed to source27; the corrected test is source28.

## Previous: source27 measured rendering bottlenecks — 2026-10-21

Source26 is remotely preserved at `8536cadf8f8e3d88b6856c53e3970170cfd95d63`:
539 tree files and all three changed contents matched the read-back. Its ARM
build passed; the fresh WASM is byte-identical to source25, with an explicit
common-source inventory comparison. Actual Firebird execution confirms the
marker enables profiling and that Continue loads the source25 first-battle
save. Normal exit produced the diagnostic JSON; the re-saved file matches the
source25 save byte-for-byte. Startup inside the app is 44.0833 simulated
seconds. Across 59 completed battle frames, rendering takes 77.113 simulated
seconds, with 43.184 seconds (56.0%) inside resource calls and 42,621,686 bytes
read. No resource failures were recorded. The final ten-second battle window,
after the animation clock crossed a complete idle duration, still reaches
only 1.2 observed updates/s. This is not a guarantee that every source frame
was accessed: the slow renderer advances animations in clamped steps.

Source25 actually completed new run → Neow → map → first battle → save →
return to title → normal exit → USB save retrieval. In declared simulated
windows title and character each reached 30 updates/s, Neow 10.2 and map 6.6.
Initial three-enemy battle windows reached only 0.6–0.7 updates/s. These count
observed loop updates, not physical handheld FPS. Slow render clamps also mean
ten simulated seconds do not warm ten seconds of animation. Do not label the
initial battle windows fully warmed. The UI remains priority one; this measured
rendering problem is priority two, following the corrected entry/save fault.

Source25 deterministic resource traces explain two distinct cache problems.
Neow repeats 63 small page reads / 258,048 bytes on every observed frame, with
44 distinct pages in the miss stream and no tile/index reads. The actual
three-enemy first-battle save reads 20,064,764 bytes in its second 180-frame
window; tile/index groups account for 96.51%. Constantly used groups are
repeatedly evicted. A separate controlled Nibbits fixture becomes read-free,
so it must not stand in for this real slow encounter. The map PC sample instead
places 61.8% of its samples in the ARM division bucket. See
[native-ui-performance-0.17.md](native-ui-performance-0.17.md).

The next coherent source stage expands the fixed small-read cache from 8 to
64 pages and computes unrotated map inverse coordinates once per row/column,
preserving the old pixel-center arithmetic and all painting rules. The selected
tile pool is 40 × 128 KiB (5 MiB), while decoded pictures retain their existing
748 × 4 KiB pool with the entry ceiling raised from 90 to 256. The actual
access trace has 256 different picture keys occupying only 680 pages; the old
entry ceiling defeats the available pixel capacity. The choice stays within
the existing 16 MiB target bounds, subject to actual ARM/WASM linking. These
changed-code results remain pending until
this stage is pushed, read back and frozen. No asset, frame, timing, color or
save-format reduction is part of these performance changes.

The source25 complete ASan/UBSan run has now closed: 60/64 passed and four
timed out at 300 seconds. The isolated core rerun passed in 394.29 seconds
(103,307 checks / 9,885 generated transitions). Only relic-combo, events and
character-UI are being rerun with longer limits. The original failed journal
is retained; this is not yet a completed 64-suite pass. All actual source25
UI/world/WASM gates below remain attributed to their original checkpoint.

Next: preserve source27, build ARM/WASM and
affected native targets, verify exact map pixels and existing 480 world frames /
30 saves, repeat the actual resource traces, then compare the same saved
encounter in Firebird. Preserve the source26 diagnostic baseline and close the
three incomplete sanitizer tests. Cloud-browser private-content approval is
still blocked; browser compositor FPS and physical-device FPS remain unknown.

## Previous: source26 reliable calculator profiler activation — 2026-10-21

Source25 is preserved at `fc769a017d83051e34592e661d02c92aa0836b20`, with all
539 remote tree entries and 34 changed contents verified. Its ARM/WASM builds
passed. ARM own-frame sizes are now new-run 80 B, save 56 B and load 64 B,
plus their called helpers; the 198,112-byte codec workspace is in BSS. The
full 64-suite sanitizer run is still active and cannot yet be marked passed.

The corrected private v16 conversion passed: 80,879,519 bytes, SHA-256
`89367cefcf3103d9bd834c47f1eb33e90119d3ab7cbb13b56e952501d2afc342`.
Actual keyword/text/new-image gates passed. Native world capture produced
23 cases / 480 frames / 30 saves: all fixtures/saves match source24 exactly,
474 frames match exactly, and the six changed frames contain only the scoped
Crystal Count correction. The actual WASM replay also passed all 480 frames
and 30 canonical saves against that new native capture.

Actual source25 loading exposed an instrumentation issue: while CTRL was held,
the PC remained in Ndless before `main`; after release it entered this binary's
`ResourceReader::fnv1a`, with profiling disabled. Source26 therefore adds a
read-only sibling marker (`nSpire2.profile.tns`) to opt into startup/per-scene
diagnostics without relying on loader key retention. The source25 common game,
UI, resource conversion and web runtime sources are unchanged. Preserve this
minimal source stage before the next ARM build and profiled Firebird run.
The existing source25 session is an unprofiled execution, not a diagnostic pass.

## Previous: source25 calculator stack and native UI correction checkpoint — 2026-10-21

Actual Firebird execution of the historical source22 calculator package exposed
a blocking ARM stack fault when confirming a new expedition. The compiled
`new_run`, `save` and `load` functions reserve **198,344 / 198,168 / 198,312
bytes** respectively, including saved registers. A full `State` temporary in
each path exceeds the calculator's available stack. The emulator captured an
unmapped stack page at the new-run fault; this is an executed failure, not an
inference from a host build. **Do not treat the old source22 calculator ZIP as
having passed new-run, map or battle execution.** Existing host/codec results
remain valid within their documented scope.

Source25 removes the three full-State stack temporaries while preserving
default member values, immutable save inputs, historical save bytes and load
failure atomicity. Related event/combat aggregate resets use the same bounded
mechanism. A separate agent prepared the emulator route from
the retained installed snapshot. All changed-code builds and execution remain
pending until this source stage is pushed, read back and frozen.

The baseline actually reached the title and character pages. In separate
10-second and 5-second simulated windows they produced **30.0 and 26.8 loop
updates per simulated second**. ENTER-to-first-observed-loop took about
**44.81 simulated seconds**. These are Firebird observations, not handheld FPS;
the emulator is not cycle accurate. No baseline map/combat rate was obtained.
Original UI fidelity remains first priority; performance is the second priority,
with the crashing entry/save paths a prerequisite to any useful battle test.

Source24 **`59b678675ecddd1472a337c3407d38cb2f3c8dd4`** is now remotely
preserved: **534 files / 40 changed contents** matched the read-back. The
source-only archive is 9,922,560 bytes, SHA-256
`531b9ebb52c2573a98c0fae4258529a4621d6e270e577d66a1f7109166a10733`.
Its logged private UI append passed: **13 original UI images**, **33,060 added
bytes**, output **80,879,118 bytes**, SHA-256
`092d58ac18c57873de6491945c7ed3527eec52174b1ccb0450d338b9dcf14570`.
All historical compressed payloads/catalogs and relocated offsets were checked.
The frozen source24 native UI baseline then captured **23 scenarios / 480
frames / 30 saves** and passed the synthetic production health test (467,441
checks including capture writes). The actual all-card keyword gate **failed**:
the old text catalog has no `orbs.*` table. A complete readonly formatter scan
found 52 empty results among 1,247 generated hover outputs, all belonging to
the five missing orb title/description pairs. Source25 appends those ten exact
private locale strings after the historical 6,100 entries; no text is invented
or copied into public source. The old v14/v15 profile and early v16 profile
remain readable. This failed gate must be rerun on the corrected pack.

Independent image review also found a literal `{Count}` on the crystal board.
Source25 binds only `CRYSTAL_SPHERE.minigame.divinationsRemain` to the saved
crystal charge count. The unchanged source24 frames and canonical saves form
the comparison baseline for the direct RGB565 clear/background fast paths;
the deliberate crystal text and no-playable-card glow changes are separate
visual corrections. No physical-device or browser performance result exists.

The end-turn control now uses the existing original glow texture when no card
is playable, including its authored center, additive draw order, 0.8-second
BackOut activation, 1.5-second pulse and 0.5-second ExpoOut fade. Its state is
presentation-only and reset per combat. Short sanitizer/formula comparisons
passed; production captures and calculator execution remain formal gates.
New bounded-stack/codec tests cover all five characters, historical saves and
a 64 KiB thread stack; their full checkpoint runs are still pending.

Next: preserve source25; compile native/ARM/WASM; verify bounded stack frames,
codec atomicity/defaults, v14/v15/v16 compatibility, health/keyword/button UI,
actual native/WASM scenes, then CTRL-launch the corrected ARM binary in
Firebird and run new game → map → battle → save → restart → continue. Record
complete per-scene diagnostics before selecting further performance changes.
Cloud-browser private-HTML approval remains blocked as described below.

## Previous: source24 native UI and performance checkpoint preparation — 2026-10-21

The owner identified inaccurate health bars, keyword hover tips and buttons,
and reported a slow browser build. Three agents implement health components,
keyword interactions and actual Firebird/device measurements; integration adds
an append-only NSP2/16 UI directory and host profiling. Source24 is being
preserved **before** formal builds, private conversion and changed-code emulator
runs. No source24 delivery, browser FPS or completed fidelity acceptance is
claimed yet. See [native-ui-performance-0.17.md](native-ui-performance-0.17.md)
for component scope, exact resource compatibility and measurement boundaries.

The original uploaded CX II CAS flash, injected boot image, Firebird and Ndless
were recovered; working-copy boot, installation and source22 USB transfer have
actually run. Native physical-device FPS remains unknown. Cloud-browser use of
the private embedded resource HTML was denied by automatic approval review;
that route needs explicit authorization, and no alternate browser workaround
is being used. Local WASM execution and the actual emulator remain available.

Next: verify the remote source24 tree and contents, archive/freeze it, then run
component tests, old/new pack compatibility, actual private UI conversion,
matching ARM/WASM builds, native/WASM scene replay and real emulator timing.
Use failures to choose a measured second-priority performance fix. Preserve the
source22 deliverables until a coherent replacement passes these gates.

## Previous: source22 verified and packaged; source23 records results — 2026-10-21

Runtime checkpoint **`5d177be78c507c9cd4b3c9d38a4600e84a44c02e`** is preserved
remotely on `work/0.17-world-parity`: all **509 files / 12 changed contents**
matched the read-back at **09:35:26 UTC**. Its source-only archive is 9,625,600
bytes, SHA-256 `9e4039d3788ccf4a2a4bea798934d43202bab2cef79562976d7793994565301a`.
The frozen runtime fingerprint is
`1742987f36dc2f9cb37ddf7209922de4d8d692ad2aa315f2205c8f2c65ec893c`.
This next source23 checkpoint changes documentation/evidence only. The binaries
remain source22; no source23 rebuild or new runtime test is claimed.

The shared **748 × 4 KiB / 90-entry LRU image pool** is implemented and
validated within the old total cache budget. Actual clip 967's 23 distinct
medium frames previously thrashed eight slots despite needing only 1,557,577
decoded bytes. Its second source22 cycle now has **23/23 accesses with zero
resource callbacks and zero reader-page activity**, versus source21's 216
calls / 11,671,220 callback bytes. First-cycle traffic is unchanged at 265
calls / 15,471,204 bytes. Cold startup still scans the full file and indices;
cache misses can still stall. Other scene actors compete for the pool, and
these host access statistics do not establish calculator FPS.

Source22 passed all **147 native build steps**, ARM (**885,386 bytes;
14,714,112-byte linked allocation; 28,595 relocations**), WASM (**4,881,267
bytes**), **5,249 linked division/ABI + 28 decoder vectors**, and **ten
affected ASan/UBSan suites in 37.53 seconds**. Final tiled-content tests report
**10,426,953 checks**, including fixture assertions. Fresh native/WASM captures
match **23 scenarios / 480 frames / 30 saves**. Selected actual-file checks
compare all **3,535 retained images** and the **23 clip-967 images**, each
through both new backends against original v14 bytes; each command also checks
all metadata and the same 40 three-owner UI frames. This is not a complete
source22 catalog rerun. The last complete registered-suite baseline remains
source17's 52 suites; ten of the current 56 were rerun. All 14 source22 runtime
journals passed with unchanged source. See
[validation-source22-0.17.json](validation-source22-0.17.json).

Source21's long **complete** oracle has also finished: both new backends
matched **45,724 images / 994,105,847 decoded bytes each**, full metadata, 40
UI frames and failure/reopen boundaries in **1,551.05 seconds**. Its
182,923,958 checks include fixture assertions. There are 21 passed journals
and two retained failed build attempts, followed by the corrected Clang
Release build. This historical full proof remains attributed to source21;
see [validation-source21-0.17.json](validation-source21-0.17.json).

The actual resource file is unchanged at **80,846,058 bytes (77.100809 MiB)**.
A matching private **0.17 development ZIP** and **offline HTML** are now
packaged and independently checked: all 12 ZIP members, HTML's complete
embedded resource/WASM payloads, manifest and checksums match. The additional
HTML packaging journal passed, giving 15 gates in the delivery manifest.
Calculator files total **81,731,444 bytes before saves**; ZIP is 81,791,483
bytes and HTML is 114,320,767 bytes. Binary/source/resource digests are in
the public source22 validation JSON; original-derived deliverables remain
outside public git. The previous accepted 0.16 pair is retained.

**30–40 MB is not achieved.** Investigation covered pinned Sprite Dicing,
complete tile transparency/bounds census, an exact-crop sample, eight
reversible representations on 8 real groups / 5,024 tiles, and all retained
images/glyphs. Every transform reconstructed exact bytes; none improved the
8-group tile baseline, and two native-channel layouts exceeded the current
raw group bound. Grouped retained/glyph models suggest roughly **8.43 MB**
of savings, still around **69.1 MiB** if the rest is unchanged. New global
metadata, runtime format and read costs are not implemented or included.
See [resource-budget-0.17.md](resource-budget-0.17.md) and
[validation-resource-research-source22.json](validation-resource-research-source22.json).

Next actionable work: implement and verify bounded grouped retained-image/glyph
loading without increasing the memory limit, then measure actual cold/random/warm
access rather than assuming modeled size is sufficient. Research animation
representation changes only with exact visual and CPU-budget evidence. Continue
the original UI's remaining dynamic layers, label clocks and temporal-sampling
acceptance; **95% original fidelity is still unestablished**. Firebird or real
hardware startup, peak memory and frame pacing remain unmeasured. The native
map/enemy and five-character/card/potion implementation remain in the same
verified runtime; this cache change adds no lossy sampling or pixel conversion.
Main and disabled remote CI remain unchanged.

## Historical checkpoint preparation records

The entries below retain each stage's state at preparation time. Then-pending
source21/source22 gates are resolved by the current record above.

## Source21 integrated gates; source22 shared image pool and 30–40 MB research — 2026-10-21

Source21 `d5738010f2d7ebc6139a2fa181ff87820d344bbb` was pushed/read back with **505 files / 13 changed contents** matching. Its frozen stage passed all **147 native build steps**, ARM (**885,145 bytes; 14,713,908-byte allocation**), WASM (**4,879,462 bytes**), **5,249 linked ARM division/ABI + 28 decoder vectors**, and **ten affected ASan/UBSan suites in 50.61 seconds**. The final tiled-content suite reports **5,455,291 checks**, including fixture assertions. Actual native/WASM agreed on **23 scenarios / 480 frames / 30 saves**; actual act-entry UI passed **43,364 checks** and four captures. The optimized actual-file oracle's complete new-memory traversal matched **45,724 images / 994,105,847 decoded bytes** against the old v14 pack. Its complete range traversal and subsequent metadata/40-frame checks remain running at source22 preparation; do not mark that command passed until its own final journal completes. GCC 13 Release hit an internal compiler error in unchanged relic code; the first Clang configuration had an invalid C compiler alias. Both failures remain; corrected Clang 18.1.3 `-O2` targets passed. These optimized host runs have no sanitizer instrumentation.

Real source21 access measurements exposed a cache issue: clip 967 has 23 distinct medium frames totaling **1,557,577 decoded bytes**, but the old medium tier holds only eight pictures. Its second host cycle still issued **11,671,220 callback bytes**. Source22 shares the old total pixel-plus-bookkeeping allocation through **748 contiguous 4 KiB pages / 90 LRU entries**, with stable surviving views, exact alias keys, and no heap or compaction. Unpublished reservations are reclaimed and failed owners cleared. A **22.075-second pre-checkpoint Content-only sanitizer probe**, including compilation, passed **361 alias checks** and **7,411,030 tile/cache checks**; this is a selected short diagnostic, not a complete registered-suite pass. Independent review found no blocking issue. The new actual clip-967 result and cross-target gates still require the frozen source22 build.

The owner now asks about Sprite Dicing/streaming and **30–40 MB on the calculator**. The existing complete pack remains **80,846,058 bytes / 77.100809 MiB**, unchanged. Together with source21's launcher it is **81,731,203 bytes** before saves; a 40 MB installation still needs roughly **51.62%** less resource storage. Fixed 16px global deduplication and range reads are already present. Inspection of pinned Sprite Dicing `fd5a865525ae257bf35fc05bd5f16b0a78ee0e5d` confirms overlapping fixed-grid deduplication, not general motion compensation. A 13-group / 8,424-tile exact-crop sample grew **926,244 → 944,638 bytes (+1.986%)**. New public reversible-tile and retained-image/glyph probes await actual frozen-source measurements; their output is a diagnostic/model, not a production codec or full-file claim. See [resource-budget-0.17.md](resource-budget-0.17.md) and [source22-evidence-20261021.md](source22-evidence-20261021.md).

Next push/read back/freeze source22, rebuild targets, run final cache/failure suites, compare actual animation bytes/callback costs and capture fresh native/WASM scenes. Keep the existing source21 complete-byte oracle running independently. Record actual bounded compression results without extrapolating a sample or silently changing pixels/frame sampling. **95% original UI fidelity and physical-device performance remain unestablished**; main, disabled CI and the verified 0.16 pair remain unchanged.

## Source20 verified; source21 staged resource startup — 2026-10-21

Source20 `3f66da839e056fe291c27ca5e93260b3833bd7de` was pushed/read back with all **502 files / 37 changed contents** matching. Its frozen source passed all **145 native build steps**, ARM (**883,404 bytes; 14,710,272-byte allocation**), WASM (**4,875,740 bytes**), **5,249 linked ARM division/ABI vectors + 28 linked decoder vectors**, **ten affected ASan/UBSan suites**, and **24 Python encoder tests**. The final tiled-content suite passed **3,565,023 checks**; these include fixture assertions and are not a count of independent scenes. All nine formal journals passed with the same runtime fingerprint and unchanged source. See [validation-source20-0.17.json](validation-source20-0.17.json).

The complete private **NSP2/15** file is now real: **80,846,058 bytes (77.100809 MiB)**, SHA-256 `407589a297a6e1bf935b6f2a5024ad532efd84423b0374db13c080e6671ba7dd`. Its full inverse validation compared **28,910 canonical frames / 443,359,210 original sparse bytes**, all **42,189 timing aliases**, 3,535 retained images, glyphs, strings and references. All 1,184 reused compressed groups were actually decoded and checked. The old 231,312,495-byte input remains unchanged. Storage now fits the preferred 80 MiB resource target; this is not target-device performance evidence.

Source20's eager image validation still models about **30.8 GB** of startup group decoding even after the 90 image-cache slots. Source21 instead validates all index streams, descriptors, geometry and group headers during open, then fully validates every tile of an image before its first successful publication. Exact-key alias hits may inherit validation; owners/reopens cannot. Temporary descriptor lists and an optional at-most-8,192-byte bitmap borrow the unused 2 MiB directory tail, with sequential/full-validation fallbacks when space is insufficient. Two independent short Content-only sanitizer probes passed (2,948,034 and 2,351,298 checks; not added into a full-suite claim). An O0 sanitizer host smoke opened the actual new pack in 4.292 s and verified its first tiled image against the old bytes. This is a pre-checkpoint diagnostic, not a formal integrated or ARM performance result.

Next preserve/read back/freeze source21, rebuild all three targets, run the final first-touch/cache/fallback suites, and execute complete new-memory/new-range/original-v14 byte comparisons plus fresh 23-case native/WASM captures with this actual pack. The all-image proof uses two sequential complete traversals against the same reference to avoid artificial alternating-owner cache thrashing; all bytes still compare and the 40-frame UI oracle continues to alternate three owners. A dedicated host benchmark separates Content open from first-image costs. See [source21-evidence-20261021.md](source21-evidence-20261021.md). Original UI **95% remains unestablished**; no new Firebird or physical-device execution exists. Main, disabled CI and the verified 0.16 pair remain unchanged.

## Source19 verified; source20 lossless sparse-tile storage — 2026-10-21

Source19 `00c6b9dda80333c3aa260d82e650b079f9fe76a3` was pushed/read back with all **479 files / 35 changed contents** matching. Its frozen source passed all **138 native build steps**, ARM (**870,008 bytes; 10,466,576-byte allocation**), WASM (**4,852,522 bytes**), **5,249 linked ARM vectors**, and **31 affected registered suites**. These include **38,001 character checks / 860 variants**, **48,111 enemy checks / 174 variants**, the new act-entry suite and retained death/cache boundaries. Actual memory/range loading passed **317,981 checks / 40 frames**, and actual act-entry UI passed **43,364 checks**. The expanded same-source native/WASM run passed **23 scenarios / 480 exact frames / 30 exact saves**. The first native invocation lacked its output directory; its failure remains alongside the separately logged corrected passes. See [validation-source19-0.17.json](validation-source19-0.17.json). The earlier complete baseline is still source17's 52 suites; no complete source19 54-suite run is claimed.

Source20 adds the NSP2/15 sparse-tile writer/reader and the unmodified public-domain LZMA decoder behind a fixed arena. A 4 MiB contiguous page cache reconstructs the exact original format-4 bytes into the existing image tiers. Complete diagnostic accounting measured **80,846,058 bytes (77.10 MiB)** for the 512 KiB grouping and verified all **28,910 canonical frames / 443,359,210 sparse bytes / 42,189 timing aliases**. Smaller groups grow beyond the preferred storage budget. This remains a diagnostic result until the new production file is built and checked. Startup and frame read/decode costs are material remaining gates, not implied by the size result. The independent reader/decoder probes passed; the final merged suite, full App ABI, actual ARM/WASM memory and new-pack equivalence await source20 push/read-back/freeze. The map top bar also now shares the original act-title localization with its banner. See [source20-evidence-20261021.md](source20-evidence-20261021.md) and [content-tiles-0.17.md](content-tiles-0.17.md).

Next preserve source20 remotely, freeze it, build all three targets, run the final codec/content and affected UI suites, repack privately, compare all old/new content bytes and actual UI frames, and measure startup/access before publishing a matching pair. Original UI **95% has not been established**, and no new Firebird or physical-device run exists. Main, disabled CI and the verified 0.16 pair remain unchanged.

## Source18 verified; source19 act entry, escapes and cache identity — 2026-10-21

Source18 `5147ca97b377f662f363c61124e5022fd0277143` was pushed/read back with all **471 files / 12 changed contents** matching. Its frozen source passed all **134 native build steps**, ARM (**863,848 bytes; 10,456,808-byte allocation**), WASM (**4,840,209 bytes**), **5,249 linked ARM vectors**, six affected sanitizer suites and **1,759 actual world-text checks**. The new WASM matched the already verified source17 native captures for **18 scenarios / 357 frames / 25 saves**; those older cases do not include Colossal Flower. Separate actual Colossal captures verify its parameterized rainbow rendering and save invariance. The first backend launch failed because Node was missing from a restricted PATH; its journal remains, and a separately logged corrected run passed. See [validation-source18-0.17.json](validation-source18-0.17.json). The complete 52-suite baseline remains source17, not an invented source18 full rerun.

Source19 adds the original first-open map camera/banner lifecycle, interpolation, interruption and hidden-node pause, plus repeat completed-event Proceed. Its pure helper passed **61,072 checks**; the existing window helper passed **106,533**. Explicit FTUE/load/font/viewport boundaries remain documented. Two new registered map suites and four fresh-run backend cases await the frozen App build. The backend driver also adds actual Colossal Flower, for **23 planned scenarios**. Three actual enemy exit paths no longer fire a false AfterDeath; saved pending real deaths and revivals are preserved. Its focused sanitizer diagnostic passed **683 checks**. Image and glyph cache keys now distinguish payload descriptors/ranges without extra cache memory or hit I/O; **392 short sanitizer checks** passed. These are pre-checkpoint diagnostics, not a rebuilt complete source19 result.

The full **231,312,495-byte** private pack still does not fit documented calculator storage. A byte-exact 16-pixel tile sample measured **5,203,414 bytes versus 9,035,482 bytes** for its raw-group baseline, with **71,792,715 reconstructed pixels** checked in sequential and shuffled access. The complete 28,910-frame size and read-cost trial is in progress; no sample extrapolation or production codec claim is made. Dense tile output would change image cache size classes, an additional runtime gate. See [source19-evidence-20261021.md](source19-evidence-20261021.md) for scope, retained failures and the next checkpoint/build/actual-pack gates. Original UI **95% has not been established**; temporal animation sampling is a separate performance adaptation, not part of the allowed readability exception. Main, disabled CI and the verified 0.16 pair remain unchanged.

## Source17 complete verification; source18 original rainbow text — 2026-10-21

Source17 `26a9097521a971b69a4b58494985c6062a24017e` and all **466 source files / 21 changed contents** were remotely verified. Its one frozen source passed all **52 registered native suites**: a 50-suite ASan/UBSan batch in **1,668.31 s**, plus separately logged original-events and world-text suites. The exact 52-name set matches CMake registration. This includes **38,001 character checks / 860 variants**, **47,428 enemy checks / 174 encounter-ascension variants**, original map/lifecycle and all five-character/card/potion foundations. All **134 native build steps**, ARM (**857,290 bytes; 10,446,400-byte allocation**), WASM (**4,829,363 bytes**) and **5,249 linked ARM division/ABI vectors** passed. The actual 18-case native/WASM comparison passed **357 exact frames / 25 exact saves**. Actual event, room, map-window and text checks also passed. See [validation-source17-0.17.json](validation-source17-0.17.json) for exact immutable evidence.

Source18 fixes the reachable parameterized Colossal Flower rainbow tag using pinned Godot OkHSV math, bounded nested parsing and a 7,582-byte integer table. Standalone parser/color **1,012 checks** and **393,216 official-function channel comparisons** passed; independent review found no blocking parser/arithmetic issue. The new complete App-linked actual/synthetic tests await source18 push/readback/freeze and build. The original label-clock phase difference remains explicit and is not counted as a screen-readability adjustment. MIT attribution accompanies the source and offline HTML.

The complete **231,312,495-byte** resource pack is valid but **does not fit the calculator's documented 90+ MB storage**. Exact lossless raw-group trials yielded **117,396,313 bytes** (16 frames/1 MiB) and **107,167,402 bytes** (64 frames/1 MiB), including all retained payloads and metadata; neither fits. The first interrupted serial journal remains unfinished, while the separately logged parallel trial validated every decoded group. The bounded LZMA2 SDK prototype is still outside production. Pixel-preserving compression research continues; no incompatible 0.17 calculator pair is delivered. Original act-entry camera/banner can proceed with existing resources. See [source18-evidence-20261021.md](source18-evidence-20261021.md) and [rich-rainbow-0.17.md](rich-rainbow-0.17.md). Main, disabled CI and the verified 0.16 pair remain unchanged; **95% original UI fidelity has not been established**.

## Source16 executed; source17 conditional text and treasure clocks — 2026-10-21

Source16 `9cf375893b9ddce526b6a0fdb85a4f3c11cc6b83` was pushed/read back with all **463 paths/modes/blob IDs** and all **45 changed contents** verified. Its frozen source passed all **132 native build steps**, ARM packaging (**855,071 bytes; 10,442,840-byte allocation**), linked ARM division/ABI checks (**5,249**) and WASM linking (**4,825,157 bytes**). Enemy execution passed **47,428 checks / 174 encounter-ascension variants**; original map core passed **1,070** and retained-room lifecycle passed **287**, including exact loading/resaving of a real source15 v15 save.

The complete new private pack is **231,312,495 bytes**, SHA-256 `d9adddc61c4f4f61b2ce50ee90960b112d752a694595ffa5f0feb2a6d9b204fb`. Independent full canonical, prefix, clip and glyph checks passed. Only the two Ancient button images differ from the earlier complete pack. Actual memory/range passed **317,981 checks / 40 frames**; actual Ancient/event UI **3,656**, synthetic Ancient color **4,921**, and actual map-window UI **82,682** passed. Real screenshots confirm restored Ancient contrast. The focused source16 UI group retains environment failures and a genuine obsolete empty-chest assertion; the 18-case driver retains its Fast-map close-tail failure. No complete native or backend pass is claimed for source16.

Source17 fixes the original locale's string/boolean and multi-comparison `cond` grammar, known empty StringVars, upgraded card references and Pael's paired text effect. A private original-text probe passed **76/76 cases** across all **19** cond entries; production binding tests still await the new frozen build. Empty treasure and Skip now use independent **1,000 / 2,500 ms** timers in both Normal and Fast, blocking early map travel without changing saves or gameplay RNG. The backend driver now distinguishes keyboard release from the separate map close tail. See [source17-evidence-20261021.md](source17-evidence-20261021.md) for source16 evidence, retained failures and the next full-suite/cross-target gates. Act-entry camera/banner and remaining dynamic scene effects still require work. Main, disabled CI and the verified 0.16 pair remain unchanged; no 95% result is claimed.

## Source16 retained native rooms, map window and enemy death repairs — 2026-10-21

The previous source15 checkpoint `7225c880decfb7ff635f960d03ebecd4b0cdd1e2` completed all **126 native build steps** and the full character UI regression: **38,001 checks / 860 variants** in 445.77 seconds. Its other 47-suite batch was interrupted after 15 passes; it is not a complete pass. A complete source14 pack was found despite its unfinished conversion journal and independently validated: **231,314,177 bytes**, **45,724 images / 1,695 clips / 11,915 glyphs / 6,100 strings / 558 cards**. Actual memory/range loading passed **317,981 checks / 40 identical frames**. The failed Crystal Sphere capture and unfinished old journal remain failures/unknowns, not retroactive successes.

Source16 integrates the original map as a retained-room overlay, with layered entry/exit timing and guarded travel; completion is counted once while the previous merchant, rest, event, reward or treasure scene remains available. Shared travel eligibility and persisted `room_open=2` protect outstanding choices and repeat Proceed. Original treasure evidence corrects the earlier unopened-chest behavior: a route requires relic selection or explicit Skip. Reviving-enemy shared debuff cleanup and final-primary/minion death dispatch now preserve real death hooks and suspended saves. Ancient option tint, outline and border scaling are corrected from actual screenshots. Three agents worked in parallel on map presentation, enemies and resources; root integrated the saved room lifecycle and backend driver.

The source has only short pre-checkpoint verification so far; it still requires its own push/readback, immutable build, affected regression runs and actual-pack screenshots. All enemy/rest cache closures remain identical, so only two static Ancient entries need rebaking. See [source16-evidence-20261021.md](source16-evidence-20261021.md) for exact boundaries, evidence and next gates. Original conditional text, empty-treasure presentation timing, act-entry/map effects and final visual acceptance remain open. Main, disabled CI and the verified 0.16 delivery pair remain unchanged. No 95% fidelity claim is made.

## Source14 map gates passed; source15 full character UI regression repair — 2026-10-21

Source14 `a205a6bd566dd2f5d7f60a1d3c7a06f2e1da9430` was pushed/read back at **13:05:26 Asia/Shanghai**: all **453** paths/modes/blob IDs and **31** changed contents matched. The frozen source passed the three map/actual-travel/transition native suites and all **112 distinct Python resource/cache tests**, generated headers and the complete directory plan. An initial environment configuration failure from missing explicit X11 paths is retained; corrected configuration and targeted compilation passed. Its source-only recovery archive is verified. ARM has also completed; the final cross-target receipt follows after WASM finishes. These are not a complete native suite or physical-device result.

The full **200-frame** original/reference versus batched **Osty ACT3 dark** rest comparison passed byte-for-byte for RGBA, encoded frames, complete metadata and payload. Sampling/rendering CPU was 138.136 versus 113.857 seconds; setup and encoding are separate. This is one complete actor row, not all 48 rows or original GPU equality. The final read-only enemy-cache scan also passed **1,600 clips / 33,210 frames / 109 models** with complete keys, scopes, payload and frame validation. The 327 enemy closures remain unchanged through source14. The sole full builder and rest prefetch are being switched to frozen source14 with preserved journals and exact checked cache reuse; a complete new pack is still pending.

Source15 changes only the character UI test and evidence. A bounded diagnostic identified 27,928 repeated page clicks in the old 860-variant suite. The replacement first verifies all **70 catalog pages** in both directions using real pointer presses/releases and disabled boundaries, then positions each isolated model fixture directly. All **858 character variants + two GiantRock variants**, the original 88 CHECK expressions, real acquisitions, descriptions, validity and save/load checks remain; the extra navigation run preserves the old seed sequence. Stage/model/upgrade progress now flushes. A bounded old-library diagnostic passed 2,070 checks and eight representative variants; the complete new 860-variant test still requires source15 preservation and execution. Historical timeouts stay recorded.

See [source15-evidence-20261021.md](source15-evidence-20261021.md). Next: push/read back and freeze source15, build all native targets and run the full character regression plus affected suites; finish the matching original-asset pack, actual-pack parsing, all 18 native/WASM cases, screenshots and matching calculator delivery. An original-source map architecture audit confirmed that ordinary map opening must retain the previous room below it; the existing Phase::Map replacement is still an open implementation difference. Main, disabled CI and the verified 0.16 pair remain unchanged. No 95% fidelity result is claimed.

## Source13 formal gates recorded; source14 original map travel — 2026-10-21

Source13 `a21fd47e38af598ab14cfc8aeac398dfb9abbfb2` was pushed and read back at **12:21:27 Asia/Shanghai**, including all **445** file paths/modes/blob IDs and all **20** changed contents. Its frozen source passed all **122 native build steps**, ARM packaging (**841,732 bytes; 10,421,136-byte allocation**), WASM linking (**4,799,333 bytes**) and **95 distinct Python asset/cache tests**. The UI batch finished **10 passed / one failed**. The failed reticle frame test rewound its animation clock after resetting presentation, leaving an apparent future combat linger. A bounded probe with the same frozen library reproduced the 178 differences, then verified all **76,800 pixels** after correcting the fixture's clock/reset order. The failed full batch remains recorded; this probe is not a retroactive suite pass. Source14 contains the precise fixture correction.

Earlier long checks have also finished: source12 has **19 passed / one character-UI timeout**; source10's same-source recheck passed events but timed out again in character UI. Source10 therefore has **45 distinct suites passed and one unresolved**, with the initial and recheck failures retained. A separate bounded progress probe is investigating the character test's repeated real catalog navigation before choosing a further rerun. Do not aggregate these different source versions into a full pass.

Source14 implements App-only delayed map selection, original circle/path/marker timing and the two original room-transition layers. Canonical State stays at departure until one guarded controller submission after **1,100 ms Normal / 600 ms Fast**. Keyboard/controller and pointer gates follow their different original timings; top-bar mouse overlays remain usable during the wait and close at room entry. Full canonical-save fingerprints, identity/revision checks and load/reset/exit cancellation prevent stale submission, while complete-byte test oracles verify gameplay/RNG results. Entry feedback is reset before observing the new room, and damage notifications establish newly created enemies' visual identities before their hurt cue. Four early circle frames append without changing the terminal circle or reticle IDs. Unscaled map milliseconds also correct source13's unintended Fast-mode reticle acceleration.

The planned profile is now **45,724 images / 1,175 auxiliary entries / 1,695 clips**, with a **1,345,460-byte** directory bound. Actual decoding checked all five original circle textures and their full logical margins. Pure motion/transition helpers and short syntax checks passed; the changed App suites still require the source14 push/readback/freeze. The actual-pack backend driver now has **18** cases, adding pending travel, intermediate frames, a deck overlay and separate Fast fade-in input release.

A measured byte-identical original Osty frame justified a rest-only bounded raster wrapper. Its 31 short batch/cache/raster tests and independent review passed. All **327 enemy closures and seven other existing render scopes** remain unchanged; only the normal rest scope changes, and its exact original reference scope remains available for strict checked migration. No new bulk conversion may use the wrapper until its frozen complete **200-frame** reference comparison passes. Existing conversion jobs remain on their original verified sources. At **13:00:36 Asia/Shanghai**, the reverse enemy prefetch had strictly verified at least **1,478 / 1,600** clips, and the original rest prefetch had completed **6 / 48** clips / **1,200** frames; these are progress observations, not complete packs.

See [source14-evidence-20261021.md](source14-evidence-20261021.md). Next: preserve and read back source14, freeze it, execute affected native/ARM/WASM and asset gates plus the complete rest raster comparison, finish the matching full pack, run actual-pack parsing and all 18 native/WASM captures, inspect screenshots, then package a matching calculator runtime/content pair. Ordinary map-open/act-start camera effects, additional original room effects and physical-device performance remain open. Main, disabled CI and the verified 0.16 delivery pair remain unchanged; no 95% claim is made.

## Source12 builds passed; source13 original map selection and rest prefetch — 2026-10-21

At **11:49:58 Asia/Shanghai**, work branch `6ec81e3b6b6aac1198d168623ae557b4fe01deb0` and tree `172c6d9ad227fed0339f9d96a99e1921cd6ff6ea` were read back: all **442** paths/modes/blob IDs and all **11** changed contents matched. Frozen source12 passed all **122 native build steps**, ARM packaging (**840,339 bytes; 10,417,972-byte allocation**) and WASM linking (**4,796,029 bytes**). Its affected UI batch has **19 of 20 passed**, including the complete changed room suite (**7,637 checks**), blend and frontend; the long character-UI suite is still running. Source12 is preserved in a verified source-only recovery archive. Main and CI remain unchanged.

The initial source10 full native batch finished **44 passed / two timeouts**, retaining its failed journal. A same-source sequential recheck has now passed the events suite in 1,071.96 seconds; the character-UI recheck remains running. This is **45 distinct suites passed on source10**, not a final 46/46 or a source13 result. After an actual byte-identical frame probe showed lower CPU cost with one BLAS thread, the private converter and enemy prefetch were interrupted cleanly and resumed with their exact frozen render closures and intact cache. The strict restart scan verified **784 of 1,600** enemy clips. The serial builder and four-worker reverse enemy prefetch are still running; no complete payload or new delivery pair is claimed.

Source13 replaces drawn map brackets with the original colored 48×48 reticle, mirrored corners and 0.35 shadows. It restores directional-navigation-only focus, original 500 ms EXPO_OUT select scaling and 200 ms SINE_OUT independent deselection fades, retaining rapid changes per node. App-only clocks use full seed/act identity; pointer mode and reset/load have explicit lifecycle handling. State, codec and gameplay RNG are unchanged. One appended auxiliary entry preserves every old index; all 335 checked render scopes and all existing clip jobs remain compatible. The planned directory is now **45,720 images, 1,171 auxiliary entries and 1,695 clips**, with a 1,345,380-byte conservative directory bound.

Three changed translation units and extracted production timing/raster helpers passed short diagnostics, including 384 full synthetic frames; the complete changed App suite awaits freezing. A new one-worker-by-default rest prefetch tool prepares the existing 48 clips/8,340 frames using the unchanged renderer and strict cache protocol. Its 12 asset-free tests and independent review passed; real conversion must wait for source13 remote preservation. The 16-case backend driver now replays real pointer movement and directional-reticle timing in its map case.

See [source13-evidence-20261021.md](source13-evidence-20261021.md) for exact checkpoint, journal, hash and adaptation boundaries. Next: commit/push/read back source13, freeze it, execute its affected runtime and asset gates, start the bounded rest prefetch, complete matching canonical pack assembly, compare actual-pack native/WASM frames and saves, inspect screenshots, and package the matching runtime/content pair. Circle/marker/travel/map-entry effects and other documented original GPU/UI differences remain unfinished; no 95% claim is made. Keep the verified 0.16 pair available.


## Source11 ARM and asset gates passed; source12 treasure holder and backend coverage — 2026-10-21

At **11:32:24 Asia/Shanghai**, `work/0.17-world-parity` read back as `713ab3be32c5e1331da132581d6ecad3c9d31e0e`, tree `3c3cf590656ccbdf91030bb85214ce44bc85a34e`: all **441** committed file paths/modes/blob hashes and all **7** uploaded contents matched. The original equivalent local source11 commit remains on its named checkpoint. The frozen source11 ARM build passed: **839,822-byte launcher**, **10,417,060-byte allocation**, 28,471 relocations. The actual linked production ELF then passed all **5,249** ARM926 division/ABI vectors. This is instruction-level validation, not Firebird or calculator performance. All **81** distinct Python asset/cache tests, generated tables and the complete directory plan passed with unchanged source. A source-only recovery ZIP contains and verifies all 441 files.

The source10 complete 46-suite ASan/UBSan batch is still running at the source12 cutoff. **43 suites have passed**, including the complete enemies, world-map/Dolly, room UI, map/reward/game-over/frontend and blend suites; three older long suites remain unfinished. This is not yet a 46/46 result. Source10 and source11 have 298 identical native/WASM runtime/build/test inputs, explicitly checked; that identity does **not** extend across the new source12 frontend edits. The complete private source11 pack conversion and cooperating reverse enemy prefetch are running with strict cache validation. No complete pack, actual payload size or matching new delivery pair is claimed yet.

Source12 adds the original single-player treasure holder's 200–400 ms cosmetic delay, 600 ms BACK_OUT entry, delayed 200 ms black-to-white modulation and child 250 ms transparent-black fade. Only the pointer waits for the completed entry; focused Confirm retains the original early claim. A press begun before the hit exists cannot claim by releasing after it appears. Cosmetic randomness belongs to an independent App-only stream, sampled once per fresh nonempty holder and never by drawing or restored offers. Its deterministic clock-seeded xorshift32 is an explicit platform adaptation of Chaotic randomness. State, codec and gameplay RNG are unchanged. Canvas source RGB is modulated before normal/additive composition without changing alpha holes, with neutral-white default pixels preserved.

The six-file implementation passed independent review and short syntax/UBSan diagnostics. Its complete changed room suite still needs execution after source12 preservation. Actual-pack native/WASM capture coverage expands from six to **16** controlled scenarios, adding all five rest characters across four regions/three acts, plus keyboard/pointer/reload/Dolly/empty treasure flows. These fixtures remain internal backend agreement checks, not original-PC comparisons. See [source12-evidence-20261021.md](source12-evidence-20261021.md) for exact receipts, timing adaptations and unfinished effects. Push/read back and freeze source12 before its new C++/WASM/ARM builds. Retain the active source11 asset job and completed 0.16 delivery pair; main and disabled CI remain unchanged.

## Source10 verified and validating; source11 ARM division and rest tint — 2026-10-21

At 11:20:43 Asia/Shanghai, `work/0.17-world-parity` read back as `1d4133ff49c6a50720c9a8ffa29cdaa3f936d040`, tree `f8ad9f8a63bf5fb58de12a0679c8de51ea870cd2`: all 439 blobs and all 15 uploaded contents matched. Its equivalent local commit `cff19391844521dd79e90d6da76d386ecb1be36f` remains on `checkpoint/0.17-freestanding-room-source10`; main remains unchanged. Frozen source10 passed all 122 native ASan/UBSan build steps, 75 distinct Python converter/cache/profile tests, generated tables and WASM linking (4,795,012 bytes). The complete 46-suite native run is still in progress at this source stage, with a 1,200-second per-test limit and 7,200-second journal limit; it is not yet a completed suite result. Initial Python attempts missing the existing lz4 dependency remain failed in their journals; reruns with the documented dependency path passed.

ARM progressed past compilation but failed linking because the new full-period animation arithmetic needs `__aeabi_uldivmod`. Source11 supplies that ARM926 EABI helper without changing C++ timing, State or codec. An isolated linked-helper diagnostic executed 5,249 unsigned vectors against Python integer division, including high-bit divisors, real animation periods and tick extremes, and checked r4-r11/SP preservation. This short instruction-level check is not a linked production runtime or physical-device test; the complete ARM build and execution against its linked ELF follow remote preservation.

Source11 also applies the original extinguished-rest `Character_1.Modulate = DarkGray` before individual Spine draws and ordered-plane blending. The exact Godot binary32 color is inherited by all six actor chains; attached Necrobinder/Osty fire shaders retain their authored COLOR override. This changes the rest-actor cache closure while retaining all 327 enemy closures and other image/map/treasure categories. Fifteen targeted asset-free tests passed and an independent review found no blocking issue. Enemy prefetch continues from its unchanged frozen source9 input. Preserve and read back source11, then rebuild ARM and convert the complete private pack before actual-pack UI/backend validation. See [source11-evidence-20261021.md](source11-evidence-20261021.md).

## Source9 remotely preserved; source10 cross-target, room and acquisition repairs — 2026-10-21

GitHub writing recovered. At 10:50:57 Asia/Shanghai, work ref `7efafe07e956bba6a49b2c9f0cca85100c7e9598` and tree `05a8e3db7ae31e5ca897e309ffc404d749599db0` were read back: all 438 files' paths/modes/blob IDs and all 49 uploaded contents matched local source9. The original local `aa3badfd` history remains on its named checkpoint; the API commit has the same tree and the existing `6b2e83c6` remote parent. Main and disabled CI remain unchanged. The previously recorded connector write blocker below is historical, not the current gate.

Frozen source9 completed all 122 native ASan/UBSan build steps. Its nine-suite risk group finished seven passed / two failed, with no source changes or timeouts. ARM and WASM both failed at the new room header's `<cstdint>` include in the freestanding toolchain. Source10 replaces that include in all three affected runtime headers and both generators with `<stdint.h>`; generated-header and both target syntax checks passed, while complete cross-target rebuilding remains required.

The room assertion exposed a real presentation defect: CamelCase original region keys missed uppercase image keys, so Camp silently displayed the historical background and skipped the new actor planes. Ordinary combat region backgrounds had the same mismatch. Source10 normalizes only presentation lookups, preserving original region identity and boss-specific precedence, with four-region lit/dark and combat coverage. Separately, the Hopper regression used a persistently discounted card with no corresponding enchantment, which was already invalid before theft. Its fixture now creates real Tezcataras Ember and checks captured/returned metadata without weakening runtime validation; 289 targeted assertions passed with the frozen source9 core.

The corrected frontend reached a later genuine state-validation failure: an acquired native chest relic may await its pickup card selection while the claimed Treasure room remains open. The old generic Treasure-phase predicate rejected that legal continuation. A frozen-core probe confirmed that both native room/relic validators and both relic bags were already correct. Source10 permits only this claimed native continuation returning to Treasure; it retains all other validators and leaves State, codec and RNG unchanged. Added map coverage includes pending save/load, no double grant, completion/exit, named RNG preservation and invalid-state negatives. Short syntax checks passed; these new cases await the formal corrected build.

The remaining 37 suites were held for the corrected checkpoint. Enemy resource prefetch has resumed under the verified source9 journal, using two workers and the existing checked cache; the starting scan confirms 122 reusable / 1,478 missing clips. Its running status is not a completed conversion. See [source10-evidence-20261021.md](source10-evidence-20261021.md) and the validation JSON for exact source identities and failure evidence. Finish and preserve the source10 repairs, push and read back the corrected checkpoint, then run the complete 46-suite native regression and full ARM/WASM builds before assembling and verifying the new private resource pack.


## Source9 rest actors, room shaders and treasure transitions — 2026-10-21

Source9 continues from the locally preserved source8 commit `81a0dd913afc55ee7176fa8f3a37408155e509b4`. Three agents implemented rest actors, reviewed fire/light evaluation and treasure transitions; a subsequent independent integration review checked Canvas paths, append-only IDs and cache closures. This is a source checkpoint, not a new playable release or a 95% visual-fidelity result. Source8 and source9 still require a verified remote branch ref before long validation under this repository's checkpoint rule.

Rest presentation now registers the five characters and Osty to the original single-player scene hierarchy and common room camera. Three act-specific loops and lit/dark track states append **48 clips / 8,340 planned frames**, with each actor's full period retained. Defect's additive glow remains between its back/front normal planes. The original Regent scene explicitly replaces its skeleton's multiply material with a default-MIX CanvasItemMaterial; the converter checks that override rather than inferring pixels from the slot enum. Necrobinder and Osty keep independent 21,333 ms and 10,000 ms periods. Runtime frame selection normalizes by period instead of the rounded directory frame interval. The description uses a readable right-hand column so it does not cover the sampled left-side actors.

A bounded room-specific evaluator supports four exactly fingerprinted original light/fire shaders, including generated float CurveXYZ textures and gradient sampling. Unknown shaders and unsupported scene elements remain explicit diagnostics. The only static scene comparison rendered overgrowth before/after: 10 existing draw nodes became 23, all existing transforms/rectangles stayed equal and the 13 new light/fire layers produced pixels. Eleven exploratory actor frames checked transforms, selected materials, ordered planes and attached dark fire. These limited samples do not establish full-period correctness or GPU pixel agreement. Shader time remains fixed at zero; dynamic noise, particles, foliage, Chaotic animation offsets and original GPU filtering/quantization remain visual gaps.

Treasure presentation adds the original 400 ms linear opening fades, 300 ms closing fades, EXPO_OUT banner alpha and BACK_OUT banner offset. Closing captures the displayed alpha, keeps the independent chest opening/shine clock, hides the claimed holder immediately and can continue during Dolly's Mirror's follow-up choice. An empty collection closes visually after one second. These clocks and opacity fields belong only to App; State, run RNG and the codec are unchanged. Original randomized holder delay/motion/input gating and particles remain unfinished.

The complete planned directory is now **45,719 images, 1,695 clips, 1,170 auxiliary entries and 6,100 strings**. Its conservative directory bound is **1,345,360 bytes**, below 2 MiB; payload bytes remain unknown until complete conversion. Existing image/text/clip IDs are unchanged. Actor and shader sources are included in their new cache scopes. All 109 enemy models' bounds/reference/batched renderer closures remain identical to source8; this scope comparison does not replace payload integrity checks or advance the 122/1,600 verified enemy-clip cache count.

The previously interrupted frozen `0f750373` baseline's 24 remaining suites completed in **1,067.58 seconds: 22 passed / 2 failed**. These were the Havoc/Armaments forced-exhaust assertion and the five-character UI's obsolete fixed title-menu practice row. Both fixtures were corrected against the original/current flow; isolated tests linked to the old frozen library passed 4,407 and 2,703 checks. The original failing journal remains failed. These diagnostics do not verify source8's newer save guards or source9's runtime. No tests from different checkpoints are combined into a new full-suite pass.

Short source diagnostics passed 75 Python checks, generated-table validation, selected C++ translation-unit syntax and a standalone UBSan RGB565 additive test. The independent review also exercised actual Canvas picture paths with synthetic images and preserved the old default pixels. Its newly discovered room-fixture actor-placeholder issue was repaired with a transparent actor frame and targeted layer-order assertions; that new engine test is source-ready, not formally executed. See [source9-evidence-20261021.md](source9-evidence-20261021.md) for the separate receipts and their exact scope. CMake now registers 46 native suites; no new complete native/ARM/WASM build has run.

The last read-back work branch remains `4b9dad9645f531b3cda3884c2a906bc425570ca6`; the GitHub write problem and unavailable CLI authentication described below remain unresolved. Preserve source9 on `checkpoint/0.17-room-presentation-source9`, with its complete source-only archive and incremental Git bundle. Next: restore a working GitHub write path, push and read back the actual ref/tree/blobs, freeze that verified source, run the 46-suite sanitizer and cross-target builds, then resume conversion with the checked cache. Keep the completed 0.16 artifact pair, main and disabled remote CI unchanged.


## Source8 preserved; remote branch write blocked — 2026-10-21

The complete next runtime source is ready for a checkpoint: native Camp/Proceed and unopened-treasure leave rules, original room presentation and its interaction suite, the Gremlin creation/add-order repair, the aggregate-initialized ARM frontend workspaces, and guarded saved-checkpoint association are all present. The frontend now fingerprints the complete canonical save, distinguishes the 64-bit native seed/ascension/initial region, rechecks a title abandonment's confirmed checkpoint, and refuses to mistake a known checkpoint's temporary read failure for absence. Hosts without a deletion callback save a forced terminal probe successfully before replacing the visible run. These are source changes, not a formally rebuilt or fully tested new runtime. No State or codec bytes were added by the room or frontend changes.

Independent room/enemy review found two further source defects and repaired them before freezing. Surprise creates Fat before adding Sneaky, so the not-yet-added Fat must not constrain Sneaky's unique-HP candidates. The narrow pending-add exception preserves creation IDs, two Niche draws, add order and the codec; new seed-2 checks expect Fat/Sneaky HP 13/13 at A0 and 14/14 at A8, including the complete RNG state, saved turn IDs and independent Heist gold. The treasure presentation identity now compares and records the complete native run seed, with a stale-clock regression that preserves every canonical save byte. Short syntax/helper diagnostics passed; these new engine regressions have not run.

Three additional agents independently reviewed frontend state, room/enemy integration and the validation evidence after the tool interruption. The frontend's three requested translation units passed a short C++17 syntax check without additional changes. The latest asset-free converter check passed 51 tests in 1.160 seconds. Those checks do not replace native linking, the complete regression suite, ARM/WASM builds, actual-pack parsing or image review.

The GitHub branch-ref mutation calls stopped returning. Two attempts to update the work branch did not produce a confirmation; subsequent readback still found `work/0.17-world-parity` at `4b9dad9645f531b3cda3884c2a906bc425570ca6`. A separate create-branch attempt was bounded at 20 seconds, and the proposed `checkpoint/0.17-world-resources-6b2e83c6` ref read back as 404. A bounded 20-second attempt to save the work-state document through the separate contents API also did not return; the subsequent work-branch readback remained unchanged. Repository metadata still reports push permission and a non-archived repository. A direct Git dry-run reached GitHub after approved network escalation but failed because this CLI has no available GitHub username/credential; no remote ref was changed. No permission-rejection reason was supplied by the connector, so this is recorded as an unresolved connector write failure and unavailable CLI authentication, not an authorization denial.

The resource-stage commit object `6b2e83c651055480a3150e545241769be99459e2` and tree `fbc6fe1a2db3819c02232ffcd147aa16c8e25d03` exist remotely and match the local index/source archive, but **a commit object without a verified branch ref is not a completed push**. Preserve the full integration source in a local commit, source-only archive and incremental Git bundle, and retain any remotely verified commit object as an additional review reference. Do not claim that the new runtime is published or validated. Do not repeat the same hanging ref-update request indefinitely.

The read-only cache audit verified 122 of 1,600 enemy clips, 2,639 of 33,210 frames and 14,167,819 payload bytes against receipts and the current retained reference-renderer inputs. Another 1,478 clips remain. All 1,144 distinct bounds receipts are valid; 456 other clip jobs reuse the shared Kaiser layout. No complete new pack or matching HTML/ZIP exists. See [source8-evidence-20261021.md](source8-evidence-20261021.md) for precise build, regression, cache and artifact evidence.

The next gate is to restore branch-ref writing or apply/push the complete source checkpoint from a functioning Git client, then read back the ref, tree and changed blobs. Only after that gate may the next long native build, 45-suite regression, ARM/WASM build and private resource conversion run under `tools/run_logged.py`. Continue from checked cached frames rather than restarting extraction. All earlier failed and interrupted journals remain retained. The completed 0.16 artifact pair remains the delivered runtime; no new 0.17 HTML/ZIP pair or 95% UI-fidelity result is claimed.

## Verified frontend build, interrupted jobs and raster batching — 2026-10-20

Checkpoint `0f750373da39b980726b543b993f3bca49c7cf2e` completed all 118 native ASan/UBSan build steps, a 4,600,560-byte WASM runtime and 38 Python converter tests. Its ARM packaging failed because the frontend's static saved-state probe emitted an unsupported constructor entry. The aggregate-initialization repair is in the next runtime source; a short ARM object check found no constructor entry, but a complete repaired ARM build is still required.

The execution service disconnected during the 44-suite run. Its partial log recorded 16 passing suites, but that run and the initial four-suite run remain **interrupted/unknown**, not passing. The serial pack conversion and two-worker prefetch were also interrupted; checked cache records and source files survived. Original unfinished journals are retained unchanged, and [validation-world-0.17.json](validation-world-0.17.json) records their observed log hashes and explicit status. Do not silently count a running journal as a pass or continue polling its lost process ID.

A fresh frozen four-suite run completed: card-result and frontend tests passed, while enemy and game-over UI tests failed. The game-over failure was a synthetic 2 × 2 Proceed image drawn with an incorrect aspect ratio over the gold row; its corrected 104 × 51 fixture passed 4,693 checks in a single-TU diagnostic linked to the frozen library. The enemy failure exposed Gremlin Surprise's creation-before-add ordering: Fat receives an earlier combat ID, but Sneaky is added first. A source repair preserves original RNG/creation IDs and the actual add order, including saved turn identities. Both repairs still need the next complete frozen runtime build.

The independent converter checkpoint `4b9dad9645f531b3cda3884c2a906bc425570ca6` compared the retained rasterizer with bounded batches of the same NumPy solves, interpolation and blend operations. Five full original clips (Corpse Slug, Spiny Toad, Inklet, Ceremonial Beast and Zapbot), totaling 69 frames, had **zero differing RGBA bytes**. Reference rendering took 37.275 seconds and batching 13.302 seconds, a 2.80× improvement in this sample. This verifies those implementation comparisons, not every enemy frame or an original-game display oracle. The subsequent converter source selects batching for new enemy/map pixels, binds its own code into new cache scopes and imports only completely hash-checked reference pixels with matching geometry, encoder, model and job inputs. Bounds and original scene scopes are unchanged. Fifty asset-free Python tests passed as a short source check; the integrated checkpoint still requires frozen execution.

Room resource source adds four regional lit/dark rest scenes, three treasure skins and six original opening/shine clips (99 frames), preserving previous directory IDs. The new plan is 37,379 images, 1,647 clips, 1,170 auxiliary entries and 6,100 strings, with a maximum 1,211,152-byte directory. These remain planned counts until a complete pack is assembled and checked. Rest fire/light shaders, particles and correctly placed regional character/companion layers remain explicit visual gaps. Treasure fade interpolation also remains incomplete. Do not infer 95% UI fidelity from this source or its resource registry.

The next runtime source also contains native Camp/Proceed and unopened-treasure leave semantics and a new room interaction suite. Frontend review reproduced wrong native-seed identity, changed-save confirmation and transient-load failure cases; their guarded save association repair is in progress. Next: freeze and verify that source, run the new/failed suites and complete cross-target builds, resume conversion using checked caches, then review actual-pack frames and compare native/WASM saves and pixels. Keep the completed 0.16 downloadable pair, main and disabled remote CI unchanged until matching new artifacts pass.

## Frontend, result-location order and ending flow — 2026-10-20

Verified checkpoint `14fcf1e4478a582d39024ba8cb96a466c0bb8dd4` completed all 112 native ASan/UBSan build steps, a 4,563,409-byte WASM runtime, and an 820,236-byte ARM launcher with 9,921,688 bytes of allocation and 28,173 relocations. These are compiler/link results; no new physical-calculator performance measurement is implied. Its six-suite focused run passed world relics, event UI, map UI and reward UI. The two failures were an event-combat fixture using an event key instead of its encounter name, and a Test Subject death fixture overlooking the third form's real Intangible. Both fixtures are repaired in the next source. The revised world UI test alone passed 3,087 checks when linked diagnostically against the frozen checkpoint library; this is not a rebuilt next-checkpoint pass.

The separately executed enemy suite progressed past Flyconid and exposed a real Knowledge Demon suspended-choice validation bug. Original virtual card offers were incorrectly treated as combat-instance IDs. The next source accepts the exact validated choice shape, including owner, stage and generated models, and adds rejected-corruption cases. Card result locations now capture modifier acquisition order once before replay count generation; previews remain read-only, and the saved continuation uses its captured pile and top/bottom position. The v15 suffix adds 17 encoded bytes. The deliberately conservative native-tail storage bound is 255,992 bytes within the 262,144-byte save buffer; it is not a measurement of actual encoded run length.

The frontend now follows the original continue/abandon versus new-run menu visibility, probes saves without overwriting the visible run, and restores only validated unfinished runs. Pause has Resume, Settings, Compendium, Give Up, and Save and Quit to the main menu. Save failures retain the active run. Confirmed abandonment removes the matching save and executes forced player death in a live run; completed runs cannot leave an earlier matching save resumable. Practice or a different saved expedition is protected. Browser deletion clears v3–v15 fallback keys; calculator deletion also clears temporary and backup files. Settings exposes a real presentation-only speed switch and the existing controls/practice/reference utilities. Its preferences are session-local, and it is not the full original desktop settings screen.

The new ending page uses original localization and a Continue → summary → main-menu flow. It shows saved floor, character, ascension, seed and current balances; no unsupported score, elapsed time, accumulated gold, unlocks or kill counts are fabricated. Fifteen original component images add common controls, character selection framing, sale tag, menu selection/continue panel and summary decoration. The character highlight is correctly behind the portrait, with the original portrait proportions. The planned pack is now 37,263 images, 1,641 clips, 1,153 auxiliary entries and 6,100 strings. Existing directory IDs are preserved.

The frozen previous private conversion has passed all 799 original world-image jobs and is still baking the enemy clips. A complete matching pack, actual-image review, full next-checkpoint regression, native/WASM comparison and final ARM build remain pending. New frontend, game-over and result-order regression suites are source-ready for the next frozen build. The previous completed 0.16 downloadable pair remains available; main and disabled remote CI remain unchanged. No 95% visual-fidelity claim is made.

## Recovery and integration checkpoint — 2026-10-20

The active branch was recovered exactly at `85794331b1cbe6fc8e13d516ce4042d8fbc4d95b`; all 315 source blobs and its tree were checked against GitHub. The 0.14–0.16 branches are ancestors of this work branch, so their implementation was retained rather than re-created or cherry-picked. The owner's pinned DLL, PCK and completed 0.16 private pack have been recovered and their recorded SHA-256 values verified.

This checkpoint preserves resumed source work, **not a buildable release or a new passing runtime**. Three agents are implementing original enemies/encounters, events, and original run routing in parallel. New source includes encounter composition, move/number data, enemy and event codecs, 43 of 66 event rule implementations with explicit remaining gaps, original reward-group state, three-act routing/queues/room logic, original seven-column map entry, and shared save/choice integration. The original map/run translation units passed a short syntax-only check; no new integrated native, WASM, ARM or visual run has been performed.

Root has widened animation/damage presentation arrays to all eight enemy slots, added bounded multi-enemy geometry and overflow-safe billion-HP bars, connected the new game entry to the original run, kept historical save widths, serialized new relic continuations, and added original-world selection input. Missing work includes the rest of enemy move/power execution, 23 complex events, relic combat/run hooks, complete new resource conversion, selection/reward display, full shared hook timing, integration validation and visual comparison. The old NSP2/13 profile cannot yet satisfy the appended 558-card profile. No 95% UI-fidelity result is claimed.

Next: complete the explicit missing modules, persist and read back that source stage, then freeze it before long build/conversion/validation with `tools/run_logged.py`. Keep the completed 0.16 downloadable pair until a matching new runtime and private content pack pass relevant verification. Main and disabled remote CI remain unchanged.


## Integrated world source checkpoint — 2026-10-20

The next source stage joins executable enemy move/power/summon/death modules, all 66 event rule modules, original three-act routing, native room entry and reward/camp/shop hooks, and the original-world choice UI. Original relic acquisition, combat/run listeners and dynamic descriptions are now present; remaining nested acquisition continuations are under active review. These are implemented-source statements, not passing content or fidelity counts.

The new UI supports the complete relic directory, sixteen potion slots, per-instance card offers, independently claimed event rewards, actual camp choices and Crystal Sphere's 121-cell board. Crystal retry/subscription bounds require 825 reveal records, 770 rewards and 495 offered-card records; indexes and the v15 suffix retain those values. Canonical resource metadata covers new cards, events, relics, power icons, intents, rest choices and scenes. The private NSP2/14 pack and actual original-enemy/scene animation integration are still being built. Complete Spine 4.2 runtime inspection loaded all 102 distinct skeletons; no conversion pass or 95% visual result is claimed.

Short syntax checks pass for the current runtime and UI source. This checkpoint is to be frozen for the first integrated ASan/UBSan build and regression execution. New source tests cover enemies, original events, map routing, relics and bounded localization; runtime outcomes are still pending. Keep failures explicit, complete nested acquisition continuations and the new resource converter, then checkpoint each repair before long validation. The completed 0.16 downloadable pair, main and disabled CI remain unchanged.

## Awaited action and UI follow-up — 2026-10-20

Source now corrects the shared victory lifecycle, extra-player-turn enemy intent preservation, saved damage/death continuations, Fairy before Lizard Tail, potion-used hooks, permanent-card remove/append transforms and batched relic acquisition. Native combat main rewards and event/relic extras share one selectable reward group. Driftwood rerolls retain creation options and one-use state; Pael's Wing remains a separate alternative. New regressions cover these boundaries, 66 initial event dispatches, large Crystal reward indexes, nested full-potion rewards and exact save/resume. These suites are source-ready and have passed short syntax checks; their execution is pending the next frozen build.

UI work adds original dynamic relic and event references, per-instance counters/disabled display, card enchantment/affliction descriptions, Mad Science frames and portraits, native run seed/ascension/first-region selection, complete potion paging and locked-event guards. Reward-kind mapping is corrected, reward rows claim directly, and reroll/sacrifice use explicit core actions. All 109 enemy visual model directories now have source metadata and full-runtime sampling/Drive support. New enemy clip-table generation and private world-pack conversion are still in progress and are not part of a delivered runtime.

Next: freeze this source, verify the remote ref and every uploaded blob, then build/run the full native sanitizer suite. Preserve failing journals and repair concrete failures on a later checkpoint. Complete the original resource pack and scene/animation UI, followed by actual-pack/native/WASM/ARM and visual verification. No original-game differential or 95% UI similarity result exists yet.

## Runtime repairs and original animation wiring — 2026-10-20

The verified source checkpoint `c6895eeae34c2f28d761454c3b603809e57b6f89` built successfully through all 102 native ASan/UBSan build steps, including X11. Its complete 38-suite regression run finished with 22 passing and 16 failing suites in 852.75 seconds (`world17-ctest-b43faf017eb3`). Those failures are retained, not presented as a passing original-world release. Concrete findings include early consumption of Feral's return quota during a result preview and rejection of structurally valid saved Combat continuations, plus outdated test fixtures for new card identity, save version, directories, event completion and native entry. The last character UI failure was the old final-card expectation after appending 61 original event/status cards; it is updated to the actual terminal model, Writhe.

The next source stage separates Feral's captured result from pure previews, preserves normal Clone versus generated Dupe behavior, accepts validated suspended Combat effects, and adds exact ordered native extras for The Hunt, Swipe, Heist, Royalties and Forbidden Grimoire. The reward bounds are now 128 entries per group store and 96 enemy extras. A source codec calculation gives a 250,224-byte conservative full writer bound within 262,144 bytes; a deliberately looser native-tail bound also fits. These remain fixed calculator limits, with explicit capacity failure; they are not unlimited original-game capacity.

Presentation now observes actual enemy move starts without changing saved state or RNG, selects original animator transitions and phase/skin clips, keeps terminal frames, and renders the shared Kaiser Crab once between the background planes. Native intent icons support combined actions, exact status-card counts, damage-based attack icon levels and readable descriptions. Card detail/merchant fees use the offered instance facts; Sea Glass titles use prepared or owned character facts; original power displays use their actual display counters. Ancient dialogue currently uses the exact first-visit line as an explicit historyless fallback; a persistent cross-run AncientStats profile and repeat dialogue selection remain missing.

The private converter is source-ready with 1,600 new enemy clip variants, 33,074 new animation frames, 737 auxiliary images and 4,341 original locale records appended after the preserved NSP2/13 payloads. This plans 36,510 images, 1,638 total clips, 1,076 auxiliary entries and 6,080 strings. Complete model cycles, common model scale/pivots, shared scene coordinates and checked resumable caches are implemented. These counts describe the conversion plan; a finished pack, its actual compressed size, and rendered visual acceptance have not yet been established.

Next: push/read back this stage and all uploaded blobs, freeze it, then run the repaired native suite, actual private conversion and ARM/WASM builds with journals. The original-world UI and generated content still require actual-pack integration and scene review. No 95% fidelity or original-game differential result is claimed. The previous 0.16 download pair, main and disabled remote CI remain unchanged.

## Original event composition and conversion repairs — 2026-10-20

Checkpoint `5f5f652f8e38e0a5ca3d9036fe6a1fd7d67228e7`, tree `f6111c488b50c976e9906210de0d7058277ad222`, was pushed/read back with all 83 changed blobs verified before execution. All 102 native ASan/UBSan build steps passed with unchanged source. Its full CTest run remains in progress at this source preservation point. The repaired Feral, original-event continuation, five-character integration, five-character UI and colorless completion suites passed; remaining failures have explicit retained journals. Two UI fixtures substituted card models while retaining their old permanent origins, a potion fixture checked all eight storage slots instead of the active enemy count, an intent fixture selected the wrong Test Subject move, an observer test compared object padding, and a History Course fixture wrongly fixed an automatic replay's target. Corrective tests preserve their actual gameplay and serialization assertions.

The run also exposed a real modal bug: drawing the event under a potion panel clamped the shared scroll field and hid its second page. Rendering now preserves the active panel's scroll; the test uses actual potion-key, wheel and selection inputs to reach slot 16. SpecialCardReward details now retain their enchantment, price and Genetic Algorithm/The Scythe growth snapshot, and Mad Science's detail type follows the offered instance. Actual-pack fixture readers measure the complete file instead of silently truncating it at the historical 13 MiB limit.

The first full private conversion stopped at Fake Merchant's inline Shader resource, before enemy baking. The scene parser now treats multiline quoted shader text as data and supports its scoped resource identity. Its reviewed UV distortion remains an explicitly reported static approximation; unknown shader omissions remain diagnostic entries. The targeted Fake Merchant scene rendered, and 17 scene tests passed, but neither is a full-pack result. Resource caches now independently bind texture, scene, enemy bounds and enemy raster dependencies, and strictly checked migration can reuse unchanged completed jobs from the previous converter.

The private profile now plans 37,112 images, 1,641 clips, 1,138 auxiliary entries and 6,080 strings. It adds 60 original map images and three original Boss map cycles (540 frames), plus the two Ancient option button layers. Ancient offers retain the centered three-row hierarchy and original acquisition text precedence. Fake Merchant uses six individually priced and sold-out stock positions in the authored 2/3/1 arrangement; its completed page exposes Proceed instead of a retained relic. Independent event UI and native/WASM world capture drivers are source-ready. Actual map rendering, private pack size, full animation baking and visual acceptance remain pending.

Both first cross-target builds stopped at a new `string.h` dependency in the card factory. Its two key comparisons now use a local unsigned-byte comparator suitable for freestanding ARM and WASM, with matching sort semantics. The repaired source passed a short ARM syntax check; no successful new cross-target link or emulator execution is claimed. Next: publish/freeze this repair stage, resume private conversion, rerun the failed and new suites, and complete ARM/WASM plus actual-pack rendering. The completed 0.16 artifact pair, main and disabled remote CI remain unchanged.

## Map composition, reward flow and execution repairs — 2026-10-20

The full regression run for `5f5f652f8e38e0a5ca3d9036fe6a1fd7d67228e7` has now finished: **31 passed, 7 failed out of 38**, in 1,237.15 seconds. The subsequent verified checkpoint `c6663cffffac19de1d50de9cadc95a55f6625f91`, tree `c20a9db4fa956944a8c9fd27116c0974c0c1521d`, completed all 108 native ASan/UBSan build steps and compiled a 4,523,730-byte WASM runtime. Its focused rerun passed six of eight suites: Silent UI, shared UI, colorless/potions, world map, world UI and the new Ancient/Fake Merchant UI. Exact retained journal/log hashes and failures are recorded in [validation-world-0.17.json](validation-world-0.17.json).

The remaining enemy failure is an actual Flyconid action-selection bug, reproduced against the frozen library: when all branch weights are zero, the original still consumes one `NextFloat(0)` sample and selects its first branch. The current repair preserves that behavior and tests the exact RNG delta. The other failure is the Earring/Stardust fixture, which set Black Hole without its required ordered listener registration. A corrected single test translation unit linked to the frozen library passes all 743 relic checks; this is a focused diagnosis, not a full rebuilt checkpoint pass. The ARM link failed because an encounter gold proportion used native float subtraction; it now uses the existing software arithmetic with both original binary32 rounding boundaries. The private conversion stopped at an embedded `VisualShader`, a Shader subclass incorrectly rejected by the importer. The narrow fix passed 18 scene tests and a real Insatiable background smoke render. Neither failure has been hidden or counted as a successful release.

The next source stage connects the four original map regions, authored node/outline images, three animated boss markers, current-character marker, dotted routes, read-only deterministic map jitter, foldable legend and keyboard/pointer routing. It appends 429 bytes of visited-room facts so resolved unknown nodes retain their first actual room after event-owned combat. Map drawing does not consume gameplay RNG. [world-map-ui-0.17.md](world-map-ui-0.17.md) documents the remaining cosmetic transitions and the independent map UI regression. A real Miniature Tent soft lock is also repaired: completing a rest option can retain the room, and an explicit core-validated Proceed action now permits leaving it.

The reward menu is being separated from general multi-selection: original panel/banner, single-column immediate claims and a separate card-reward row retain each core reward identity, alternatives and parent continuation. Enemy presentation gains an optional process-local animation callback at actual power/action boundaries, Test Subject's two knockout/revival cycles, synchronized Decimillipede strikes and restoration of settled loops after direct tracks. New visual regressions check those transitions while comparing complete save bytes. Character selection now uses original localized descriptions and starting relic text and shows the pinned starting gold; the native merchant uses actual offered card titles and the original sale-price color.

These source changes still require their own pushed/read-back checkpoint, frozen build and execution. Next: preserve that stage, run all native suites, resume the private conversion, then verify actual-pack rendering, exact native/WASM frames and saves, ARM allocation and fresh-process persistence. New original art-backed menu/rest/treasure composition and several visual timing details remain under review. No 95% fidelity measurement or original-game oracle result exists. Keep the completed 0.16 artifact pair, main and disabled remote CI unchanged.

## Active 0.17: original events, enemies, maps and UI

The user requested the following order: complete all original single-player events and enemies, then integrate the original map, then restore original UI with a 95%+ fidelity target. Scaling and layout adjustments required by the 320 × 240 calculator screen are permitted. Work continues on **`work/0.17-world-parity`**, branched from the remotely verified completed 0.16 documentation head **`4abacfa4cd3cfbb3d21f5065d8134edb2aca681e`**, tree **`fd34b8092ef7526335ce87cf924418028ddf1583`**. The previous runtime and its delivered artifacts remain the completed historical baseline below.

Three parallel agents own enemy/encounter behavior, event behavior and card generation, and all original relic behavior; root owns shared state, integration, migration, reference-run routing and checkpoint publication. Initial inspection found hard-coded two-enemy storage, a 12 × 3 expedition, five adapted events and a separate seven-column original-map subsystem. These are integration gaps, not completed original-world support. The 13 MiB resident resource pack has only 720,710 bytes free and ARM static allocation only 602,932 bytes below its 16 MiB ceiling; full-world assets require bounded random-access resource loading rather than another unchecked resident-buffer increase.

Reachability analysis identifies 107 original single-player combat monsters plus the Architect display model, 87 combat encounters (80 act encounters and seven event encounters) plus the Architect display encounter, 66 events including eight Ancients and the Lantern Key forced event, and 297 single-player relic models. Legacy Nibbit remains a separate compatibility model. These are implementation targets, not passing-execution counts. Enemy storage must handle at least six simultaneous original creatures; the shared engine will provide eight slots. Nested event/relic rewards and enemy status cards extend the append-only card directory to at least 558 models. The task ledger and acceptance details are in [world-parity-0.17.md](world-parity-0.17.md).

### Resource storage source stage

The first coherent runtime stage replaces production resident pack arrays with bounded random-access resource loading in Ndless, X11 and WASM. Directory, decoded picture, glyph and string caches have fixed limits; the file remains immutable and its complete checksum and payload formats are checked at open. The browser keeps the embedded pack outside WASM memory. Historical memory-backed parsing remains available, and old resource directory counts are frozen explicitly for versions 3–13. New reader and actual-pack memory/range comparison suites are prepared. Details are in [resource-source-0.17.md](resource-source-0.17.md).

The resource stage **`5b665a8b81d70df9215dff31e74a705d7cfeb121`**, tree **`8392667a5d6757df86fdc4d981a71bcf7ec19f70`**, was pushed/read back and frozen before execution. All seven journals passed with unchanged source. Actual memory/range content comparison passed **39,164 checks**, including every image, glyph, string and clip and **40 rendered UI frames**. The new WASM range host preserved **384 exact RGB565 frames and 23 v14 saves** against the completed 0.16 reference captures. ARM produced a **288,654-byte launcher**, **8,434,016-byte allocation** and 5,606 relocations. WASM is **2,954,751 bytes** with fixed 16 MiB memory. [validation-resource-0.17.json](validation-resource-0.17.json) retains log hashes. These passes establish storage preservation, not new original-world behavior or original-UI fidelity.

### World source preservation stage

Shared integration now appends eight enemy slots, 32-bit enemy HP for the Waterfall Giant's special second phase, 16 possible potion slots, 558 card models, original named RNG streams, event/enchantment/affliction facts and a v15 suffix. Attack boundaries capture Fatal permission before damage and retain kill results across revival. Three agents are completing independent event, enemy and relic continuations, choices, codecs and regression suites. Their currently saved source is preserved on this work branch while integration continues. This is **an in-progress source checkpoint, not a runnable or validated release**; some referenced module files and final call sites are still being written. Do not compile this preservation snapshot. The completed 0.16 download pair remains the delivered runtime.

## Completed 0.16: all five single-player character pools

The requested **Defect, Regent and Necrobinder single-player card pools and their character mechanisms are implemented and verified**, retaining the completed Ironclad, Silent, colorless and potion content. Three implementation/resource agents worked in parallel; root integrated shared behavior, reviewed the final scenes and packaged the matching outputs. Reference remains **v0.111.0 / 41cef1ea**, with audio, multiplayer and Timeline excluded.

The exact tested runtime is **`8c6977d3cc1dc9f17926a480127bc1067abd0286`**, tree **`db96da21a4f39ae8612857fab5c2ea1b4e1295a3`**, pushed and read back on **`work/0.16-five-characters`** before validation. Later commits update documentation and receipts only. The frozen build root and source-only archive are `nspire-stage-8c6977d3` and `source-8c6977d3.tar`. Main and disabled remote CI remain unchanged.

| Final verification | Executed result |
| --- | --- |
| Native ASan/UBSan | **31/31 suites; 929,474 checks/assertions**; all 81 build steps, including X11, passed |
| Defect completion | **16,190 checks / 172 base-upgrade variants** |
| Regent completion | **9,892 checks / 181 base-upgrade/token variants** |
| Necrobinder completion | **19,494 checks / 174 base-upgrade/token variants** |
| Python / actual parser | **30/30 tests; 3,736 actual NSP2/13 parser checks** |
| Actual character UI | **66,478 checks; 860 character/Giant Rock acquisitions** |
| Actual five-character mechanisms UI | **7,173 checks** |
| Actual colorless/potion UI | **7,375 checks; 106 variants / 64 potions** |
| Backend comparison | **384 exact RGB565 frames and 23 exact saves** across nine room/combat scenarios |
| Fresh-process X11 persistence | v14 **88,764-byte** ordinary and **109,012-byte / 512-instance** files load/re-save exactly through production file callbacks |
| Migration / full capacity | Genuine v13 **41,820 to 88,876 bytes**; v14 full fixture **109,012**, conservative **109,064 / 131,072** |
| ARM | **284,458-byte** launcher; **16,174,284-byte** allocation; **5,597** relocations; unchanged 16 MiB allocation ceiling |
| WASM / HTML | **817,288-byte** WASM; fixed **16 MiB** memory; **18,319,349-byte** self-contained HTML |
| Visual review | **40 formal captured scenes plus two Osty animation-end scenes** reviewed by root and the UI agent; no blocking findings |

The native count includes repeated assertions and is not an independent-scenario count. The three actual UI suites total **81,026 checks**. An additional frozen-engine Osty clip-end diagnostic passed 540 checks; it is recorded separately and is not added to those formal counts. All **19 final journals** are `passed`, `source_changed: false`, with source fingerprint **`9fadd7740d7abde1d3144e293f22a6846476d84b6cefe74927f3065182a5ea2e`**. The full native journal is `native016-ctest-54a1d9ac8e6f`; [validation-0.16.json](validation-0.16.json) records all final scopes, file hashes, journal log hashes and the screenshot evidence. All earlier failed journals remain retained.

The directory now has **497 models**: 85 Ironclad, 86 each Silent/Defect/Regent/Necrobinder, 53 colorless and 15 generated/status models, plus 64 potions and 33 relics. This stage adds 258 character models and 11 generated/status models after the unchanged old IDs 0–227. The core includes five orb types/Focus; stars, independent star/energy prices and Forge; Osty, Soul and Doom; exact bounded payment/trigger/choice/replay sequencing; complete temporary-cost and Nightmare snapshots; and permanent Genetic Algorithm/Scythe growth.

The final private **NSP2/13** pack is **12,910,778 bytes**, SHA-256 **`4b83320460798a5dd57d4faeee3cdf9613bef7413259ac57b61fb74d109a5917`**. It contains 1,556 strings, 339 auxiliary entries, 2,638 images, 7,215 glyph records and 38 clips. All 1,622 prior image payloads, 6,265 prior glyph payloads and old card/string/auxiliary/clip records are preserved byte-for-byte. Full original selected cycles add 641 animation frames; all text has glyph coverage at five sizes. To preserve those frames and dimensions after maximum lossless compression, the resource buffer is 13 MiB, with 720,710 bytes free beyond the actual pack. ARM allocation and WASM linking verified the resulting bounds.

The matching release files retain the canonical names **`Slay-the-Nspire-2-calculator.zip`** and **`Slay-the-Nspire-2-offline.html`**. The ZIP contains the paired launcher and pack, Chinese instructions, build metadata, validation receipts and license notices. The HTML embeds the exact verified WASM and identical pack; ZIP CRC and embedded payloads have been checked. Private game-derived files remain outside Git. Source and validation documentation persist in the verified work branch.

### Next work and evidence limits

The current card-pool task is complete to this recorded implementation and regression scope. Complete original encounters/runs, original special-potion acquisition routes, further original-UI fidelity, and new Firebird/physical-device execution remain separate work. The playable twelve-floor route, Nibbit encounters and five events are adaptations. Original rules were inspected from the pinned installation; the original game was not run as a differential oracle. Native/WASM agreement uses the shared storage adapter and does not establish browser DOM/localStorage behavior. X11 persistence used production headless actions, not display-server interaction. The visual review covers the 42 recorded scenes, not an individual review of every card portrait. Older saves preserve encoded facts and documented historical-order policies; missing histories cannot be reconstructed.

Continue from the current work branch. Preserve the tested runtime SHA versus later documentation-head distinction, push/read back changed source before long execution, and do not repeat this completed validation without a concrete new change or risk.

## 0.16 development history: recovery and integration

The following entries are historical. The completed stage above supersedes their earlier pending and failed states; failures remain recorded for continuity.

The user explicitly requested **all single-player Defect, Regent and Necrobinder cards and their mechanisms**, continuing the completed 0.15 stage. Source work uses **`work/0.16-five-characters`**, based on exact restored commit `7f486bdf757fb9af9dff89cdc8a785f049a404fc`, tree `58c6cdbb729c2822c52f74fb01028262670b8e1e`. All 203 source blobs and the complete tree were verified during recovery. The previous scratch files had expired; the source and original inputs have now been recovered instead of restarting from memory. Original DLL, PCK and the 0.15 NSP2/12 input pack all match their recorded hashes. The last distributed verified runtime is still 0.15; no new 0.16 behavior is validated yet.

The reference stays pinned to **v0.111.0 / 41cef1ea**. Each remaining original pool declares 91 cards and excludes five multiplayer-only models, giving **86 single-player cards per character / 258 additional pool cards**. Defect IDs append at 228–313, Regent at 314–399, Necrobinder at 400–485; required generated/status models follow. Preserve all prior IDs 0–227. Do not equate these scope counts or newly authored code with passing execution results.

The Defect agent owns the isolated `defect_*` state, operations, choices, rules, definitions, codec, validation, audited config and completion suite. The Regent agent owns corresponding `regent_*` files. The recovery agent, after recovering all private inputs, owns `necro_*` and the Necrobinder audit/suite. Root owns all shared files and integrates hooks, CardKind/token width, five-character entry, resources, rewards, save v14, cross-character tests and verified GitHub persistence. Agents must not compile unpublished mutable source. The next long validation must run from an immutable archive of a pushed and read-back checkpoint.

Mechanisms include Defect's actual five orb types and Focus; Regent's stars, independent star costs, Forge and existing Sovereign Blade; Necrobinder's Osty, Soul and Doom. Reuse the existing potion/colorless state where those mechanics already exist. Inspect the actual original triggers and targets: for example, the pinned Defect commonly evokes the rightmost orb, unlike assumptions imported from earlier games. New source must preserve bounded queues, choices, nested effects and explicit serialization; new output will use save v14 and the NSP2/13 resource profile. The completed 0.15 calculator ZIP and offline HTML remain the downloadable pair until a fully verified matching replacement is ready.

Private inputs are `{sts2.dll,SlayTheSpire2.pck,release_info.json,nSpire2-0.15.nsp.tns}`. Private audit tools and dependencies are `nspire-il.py` and `nspire-python-deps`. The recovered installation has unrelated mod PCKs; only the hash-locked root game PCK is a reference. Never publish those inputs, extracted original text/art, private IL or generated packs/HTML in Git. No image generation is authorized. Audio, multiplayer and Timeline remain excluded, and remote CI remains disabled.

Next: finish the three audited modules and shared infrastructure, push the coherent source stage, then build and validate all five characters plus the retained content. Create a genuine v13 continuation fixture from the unchanged restored 0.15 writer before testing migration. Record actual failures and push repairs before rerunning changed source.

### First 0.16 source checkpoint: integration in progress

All three isolated modules now contain explicit executable branches for their 86-card single-player pools and required generated cards, plus literal base/upgraded numeric suites. Shared integration now contains five-character entry, append-only models through 496, wide model tokens, independent star/energy payment, ordered before/body/after/late continuations, all five orbs, Osty/Doom damage hooks, permanent deck growth and cross-character snapshots. Four generated-card powers share acquisition order with the bounded power directory. The UI and resource agent source is being checkpointed while its final call sites/tests are still being completed.

This is an **in-progress work-branch preservation checkpoint**, not a release or successful build. No 0.16 build, conversion or regression has run. The next step is to finish the shared validator and UI call sites, publish the next frozen source stage and run the pure core regression suites, followed by the real content/UI and native/WASM/ARM verification. The existing 0.15 downloads remain the verified pair.

### First resource execution and final shared integration

The source checkpoint `2eabce11a5ad5e5f2460e67eb5f9713c8ea424eb`, tree `2ddafe278900b663756127ef8d118c2f55f9d89a`, was pushed and its branch/commit tree read back. The immutable resource conversion stopped on No Escape's original `IsTargeting` description expression. Journal `pack16-preservation-34ae4b4084d1` retains that failure with `source_changed: false`; no pack was produced. The strict converter and runtime description parser now support only the two observed target conditionals, with explicit missing/dead/out-of-range target regression coverage. Publish this repair before its second conversion.

The genuine old v13 writer at unchanged `7f486bdf` produced the 41,820-byte suspended Nightmare/Havoc/Headbutt fixture (`6d20885d2d6536fe10d9c8389bbeac80b7d24beed99476eb0700fda24ed92b00`). Its old writer build and successful production journals are retained; a first LeakSanitizer failure caused by ptrace was resolved by disabling leak detection, with ASan/UBSan still enabled. The restored official X11 headers/libraries also passed old-source configuration. These are recovery/fixture results, not new 0.16 runtime passes.

Final shared source adds ordered Envenom/Monarch's Gaze/Reaper Form damage listeners, seven separate temporary Strength sources, Misery snapshots, expanded v14 validation and migrated old listener baselines. Original IL establishes that BulletTime's energy and star zero-cost modifiers expire at physical play completion or turn end. Whole-turn Flatten remains a separate layer; all local Add/Set modifiers precede global free/extra-cost hooks and final clamping. The Regent agent is completing those runtime/free-choice call sites while this preservation checkpoint allows independent resource conversion. Two star-price flags and complete codec/Nightmare paths are present. The next complete C++ build must use the subsequent coherent candidate after that agent reports ready.

Root also added correct automatic result-pile handling for unplayable status cards without inventing a CardPlay, and expanded the native/WASM oracle from four room sequences to five additional character combat sequences. All new C++ behavior remains unexecuted at this point. Next: finish the fee call sites and literal cases, push/read back the full candidate, run all native suites and actual-pack UI, then build ARM/WASM/X11 and replace the same two downloads only after matching validation.

### First complete build candidate

Checkpoint `865a92783a825d95d05c20689aca90eeceee8723`, tree `9b55ed92d2abb7eee9eabc8d106c2f182c8eef52`, was pushed/read back before its resource work. Python executed 25 tests: 24 passed, including all eight new resource cases; the sole failure was the historical test still expecting writes to the v13 browser key. It now checks v14 writes and read-only v3–v13 imports. Conversion then passed all 258 new character descriptions but stopped at Debris, which has no original independent description. Its original Exhaust keyword is now the explicit fallback, alongside the two previously confirmed models. Failed journals `python16-all-fc7c7b030c5b` and `pack16-preservation-6ef55eadb633` both retain unchanged-source evidence.

All agents now report the C++ candidate ready for its first build. Independent energy/star costs, local Add/Set order, whole-turn Flatten, all-resource versus energy-only free callers, actual choice previews, clones, Nightmare, old BulletTime migration and expiry paths are complete in source; [shared-costs-0.16.md](shared-costs-0.16.md) records the pinned method evidence. Automatic unplayable cards retain ExhaustOnNextPlay until the actual cleanup boundary, without card-play histories; active ancestors cannot be recursively re-entered from any pile. Event/merchant removal and transformation entrances require eligible non-Eternal cards, including Grove's two-card requirement. CMake registers **31 native suites**. Push/read back this full candidate, freeze it, then execute native ASan/UBSan/X11, all Python tests and the repaired conversion in parallel. No new runtime pass is asserted yet.

Candidate `7b61bed6326639d137137af453451820b973310e` / tree `e0a1e32a89eff097cab1e2821250e56a25377083` was pushed/read back. All **25 Python tests passed** (`python16-all-acd085dbd0ce`). Native X11 configuration passed, but the first full compilation stopped at the missing separator between the historical and appended rarity arrays; 47 independent objects compiled, including the tests and application, while the core object failed. CTest did not run. The build also identified two legacy test masks using 32-bit shifts for 33 relics. Source repairs add the separator and use `RelicMask(1)` in those assertions. Journal `native016-build-b99fc354b012` preserves the failure and unchanged source. Publish repairs before rebuilding; resource conversion from this candidate continues independently.

### First complete native run and original animation repair

Checkpoint `cc6e3123ede03b72006b1b6e1c57643814343f5a`, tree `d895812dbbb32286726e3adb20ea4382bfb5a22f`, was pushed/read back before building. All 81 native build steps succeeded, including all 31 regression executables and the X11 host. The first complete ASan/UBSan CTest run passed **23/31 suites**, with **775,981 checks/assertions reported by the passing suites**. The combined five-character UI acquired all **860 variants / 62,572 checks**. Genuine v13 migration passed: **41,820 to 88,876 bytes**, preserving and resuming the suspended payload. The fully bounded v14 fixture is **109,012 bytes**, with conservative ceiling **109,064 / 131,072**. Automatic status result-pile handling passed 88 checks. The full native journal `native016-ctest-920f1bf9e1b4` retains the eight failures and unchanged source; its log SHA-256 is `522b3f781f96baa4b59f92f4c0ccd984003540b68bd5e19f497d3ff1b02de350`.

ARM compilation also passed at this checkpoint: **284,355-byte launcher**, **15,125,472-byte allocation**, and **5,596 relocations** (`arm16-build-ec8b0818ca85`). This establishes compilation only. Resource conversion from `7b61bed6` passed all appended descriptions, portraits and Defect clips before stopping at Regent's unsupported NoRotationOrReflection inheritance (`pack16-preservation-342a84f747db`). No complete NSP2/13 pack or 0.16 replacement download exists yet.

The next repair stage corrects the actual shared entry issues: practice card creation now drains its generated-card notifications; rejected star-cost plays preserve state, including the relic flash; Solar Strike and Shining Strike receive their original Strike tags. Legacy tests are updated for 33 relics, five-character Splash generation and the real relic-page extent. New-module fixtures correct an illegal save attempt inside a transient effect queue, four original Necrobinder numerical/keyword expectations, and a Hellraiser fixture that incorrectly auto-played non-Strike Headbutt/Flak Cannon. The replacement Scrape scenario naturally pauses on Sculpting Strike; an explicitly identified queued-operation fixture separately checks Flak's active-ancestor status snapshot.

Private diagnostics linked to the unchanged `cc6e3123` engine isolated all 181 Regent variants/tokens and 174 Necrobinder variants, plus their interaction groups, with no remaining numerical failure after identified fixture corrections. All 172 Defect variants and four of its five interaction groups passed; its last group exposed the invalid Hellraiser premise described above. These diagnostic drivers are not a replacement for the next formal suites. Original animation inventory identified four actually used additions: inheritance mode 2, a relative-world transform constraint, one-bone IK stretch, and clipping attachments. The source now implements these using the official Spine 4.2 conventions and adds five independent matrix/reach/area test methods. Publish/read back this repair stage before rerunning all changed source, then complete actual-pack UI, native/WASM agreement, fresh-process persistence and both replacement artifacts.

### Repaired card suites passed; measured resource capacity

Repair checkpoint `79b17017674cbbc02e8255e9bdc714c87472dfe6` / tree `12c6ee0018a966843d363b7ac9b29f7e4ad7d2bf` was pushed/read back before execution. All **eight previously failing formal suites passed** (`native016-targeted-78c63eab0b28`), including Defect **16,190 checks / 172 variants**, Regent **9,892 / 181 variants**, Necrobinder **19,494 / 174 variants** and five-character UI **1,687 checks**. The complete 31-suite run continues independently. All **30 Python tests passed**, including the five new geometric methods (`python16-all-42539a43c098`). ARM built at **284,457 bytes**, **15,125,708-byte allocation** and **5,597 relocations** (`arm16-build-65a1fafc6467`). Every completed journal reports unchanged frozen source.

All **21 new original animation clips** now sampled successfully. The final encoded full resource pack measured **12,910,778 bytes**, exceeding the 12 MiB limit by **327,866 bytes**. The encoder already uses maximum LZ4 HC compression; the conversion correctly rejected the over-limit pack (`pack16-preservation-058304a1e28b`, log SHA-256 `40c4827f38a967ca4e0a90c7b5299ee58419b617f7ed2eae29a6bf6cb7ab7ecd`). This is a size failure, not a successful delivered pack. To preserve every selected frame and existing payload, the next stage raises only the resource capacity to **13 MiB**, leaving **720,710 bytes** beyond this measured pack. The ARM allocation is expected to remain below its existing 16 MiB ceiling; the actual ARM link and unchanged 16 MiB WASM limit must verify that expectation. No sampling rates or dimensions are reduced.

Publish/read back the capacity adjustment before conversion and platform builds. Use the next single source identity for final full regression, real-pack UI/parser, native/WASM frame/save comparison and fresh-process X11 persistence. Retain all failed journals and keep the existing downloadable pair until its matching replacement passes those gates.

## Completed 0.15 release stage

The complete **85-card single-player Ironclad pool and 86-card single-player Silent pool** are implemented and regression verified, together with the retained 53 colorless models and 64 potions. The current directory has **228 models**, including Shiv, Soul, Sovereign Blade and Giant Rock. Reference remains **v0.111.0 / 41cef1ea**. The three implementation/resource agents completed their separate audits, and the root integrated shared hooks, reviewed the final visuals and collected the release evidence.

The exact tested runtime is **`4df76b04e2a07e6be9a898793f03fd0ab8df4aae`**, tree **`91496dce7dfb6580acea24a9ee860ce5e4f36b42`**. It was pushed to **`work/0.15-full-characters`** and the remote branch and complete tree were read back before testing. Later commits update documentation and validation metadata only; the release binaries stay tied to that runtime SHA. Main and the completed 0.14 branch are retained unchanged, and remote CI remains disabled.

| Final verification | Actual result |
| --- | --- |
| Native ASan/UBSan | **25/25 suites, 748,629 checks/assertions passed**; these are repeated assertions, not unique scenario counts |
| Ironclad completion | **4,390 checks**, 170 base/upgraded pool variants plus both Giant Rock variants and interactions |
| Silent completion | **17,753 checks**, 174 variants across 86 pool models and Shiv, including 864 exact integer-rounding reference combinations |
| Retained completion suites | Colorless **8,961 / 106 variants**; potions **34,202 / 64 models**; combined **7,897** |
| Python / actual parser | **17/17 tests**; actual NSP2/12 parser **1,667 checks** |
| Actual character UI | **22,225 checks**, all **344 acquisitions**: `(85 Ironclad + 86 Silent + Giant Rock) × 2` |
| Actual colorless/potion UI | **7,374 checks**, all **106 variants and 64 potions** |
| Backend room comparison | Four controlled scenarios, **84 exact RGB565 frames and eight exact saves** between native and WASM |
| Fresh-process X11 persistence | v13 ordinary **44,720 bytes**; **384-instance / 48,908-byte** save reloads and re-saves exactly |
| Save boundary / legacy migration | v13 full bounded fixture **61,956 bytes**, conservative **62,008 / 65,536**; genuine frozen v12 writer fixture passes |
| ARM / WASM / X11 | All three built; ARM **195,304 bytes**, allocation **10,659,088 bytes**, **3,192 relocations** |
| Visual review | Root reviewed all **18 final scenes** in the two contact sheets; no blocking findings |

All **13 final journals** ended as passed, with `source_changed: false` and common source fingerprint `ba9286b978111d5eb38b0639b7497d1026d6115d46d9a4d5e3a8f145253a93ba`. The native journal is `native-final-c5b65fe7bbf4`. Full public source/artifact hashes, journal log hashes, exact scopes and limitations are in [validation-0.15.json](validation-0.15.json). The local complete receipt is `nspire-validation-4df76b04-receipt.json`; the immutable test root is `nspire-stage-4df76b04`, with source-only backup `source-4df76b04.tar`.

The final **NSP2/12** pack is **7,511,131 bytes**, SHA-256 `2699415023cb3b825a84e0f3645800c6690ea258854583a02bbb28cadf318f66`. It contains 228 card records, 749 strings, 233 auxiliary entries, 1,622 images, 6,265 glyphs and 17 clips. All 1,460 prior image payloads were retained; 877,477 bytes remain below the 8 MiB resident resource limit. Sidestep uses the actual original beta portrait because the pinned installation has no final portrait. The older expansion UI success-line text still says NSP2/11; the supplied pack, parser and actual UI profile were verified as **NSP2/12**. That stale log label does not change the test or loaded content version.

Use the existing canonical release names **`Slay-the-Nspire-2-calculator.zip`** and **`Slay-the-Nspire-2-offline.html`** for the matching 0.15 pair. The ZIP includes the launcher and resource pack together, Chinese instructions, exact build information, validation receipts and license notices. The self-contained HTML embeds the same pack and the WASM built at the tested commit. Keep original assets and generated releases out of the public source repository.

The repairs validated in this final runtime include ordered after-play listeners, captured before-play amounts, nested continuation order, legacy in-progress attack migration, correct X-cost expectations, actual-instance cost previews and ARM arithmetic without an unsupported 64-bit division helper. Save **v13** records character modules plus a bounded shared power order; it reads project v3–v12 saves. New historical facts cannot be reconstructed: missing earlier histories remain zero, except visible pending attacks, and old after-play powers retain the former Panache-index-before-Calamity policy.

### Remaining scope and next handoff

The requested two single-player card pools and the preceding potion/colorless stage are complete to the recorded implementation and regression scope. The other three character systems, complete original encounters and runs, special potion acquisition routes, and exact original UI fidelity remain future work. The existing playable twelve-floor route and Nibbit encounters are adaptations. No Firebird or physical-device execution was performed; native/WASM agreement uses the project storage adapter and does not validate browser DOM/localStorage or original-game runtime parity. Audio, multiplayer and Timeline remain excluded.

Continue from the latest verified work-branch head, preserving the tested runtime/artifact distinction. Before a new implementation batch, read the two character audits and append-only card/save contracts, then push and read back the next coherent source checkpoint before any long build or validation. Do not repeat this already-passing batch without a concrete change or remaining risk.

## 0.15 development history

The following checkpoint notes are historical. The completed result above supersedes their pending and failed states; the failed journals remain preserved.

The user requested continued development followed by the **complete Ironclad and Silent card pools**, with parallel agents. Continue on **`work/0.15-full-characters`**, created from and read back at `9c4d47e543a2ee9af58fe075700ff50cf205740b` (tree `949fc6b2b5e748ac1d7b4668252d87322c9299b4`). The completed 0.14 work branch is retained at that commit; its tested runtime remains `b4b67e714ca76e7afb72f3a170789ad90047e6f4`. At the opening of that stage, no 0.15 behavior or build had passed.

The locked assembly declares 90 Ironclad and 91 Silent models. Applying its multiplayer constraints leaves **85 Ironclad and 86 Silent single-player models**, including Silent's Ancient-rarity Suppress and WraithForm in the compendium/practice pool. Ordinary rewards and generated cards must exclude Ancient rarity. Existing character IDs remain unchanged; append the 56 missing Ironclad models at IDs 114–169, the 57 missing Silent models at 170–226, and generated GiantRock at 227. Inky is an enchantment, not a new card token. Do not import unrelated STS1 status cards from memory.

The root owns shared CardID/pool dispatch, combat hooks, status UI, save v13 integration, tests/build registration and verified source persistence. The `colorless` agent owns the isolated Ironclad state/operations/definitions/core/codec/validation and its audited config/regression; `potions` owns the corresponding Silent modules. `recover_workspace` owns original resource conversion, fixed historical resource profiles, descriptions, card-detail type/cost rendering and real-content UI coverage. Source state stays explicit and bounded; save v13 appends character modules after the frozen v3–v12 encoding. The intended content profile is NSP2/12. The practice deck capacity will grow to 192 to hold each full character pool plus all 53 colorless cards.

The two existing downloadable artifacts are the completed 0.14 calculator ZIP and offline HTML. Keep them available until a fully verified matching 0.15 pair replaces them. Preserve every prior image payload while appending original card and power art; Sidestep may use the actual original beta portrait with that source recorded explicitly. No image generation is authorized.

### First 0.15 implementation checkpoint (not yet validated)

The first source batch contains executable modules for the 85 Ironclad and 86 Silent single-player models, GiantRock, bounded nested play and per-instance modifiers, dynamic target/cost/description hooks, character status UI, save v13 and the NSP2/12 append converter. The retained 53 colorless models and four generated tokens bring the directory to 228 cards. CMake now registers 25 suites, including both complete character variant suites, actual UI acquisition and a separate historical migration/capacity check. Python discovers 16 resource/journal tests. These are source coverage statements; no 0.15 build, conversion or regression has run yet.

The migration fixture `tests/fixtures/v12-entropy-choice.hex` was produced with the unchanged, previously verified `b4b67e71` v12 writer. It contains 12,756 binary bytes (SHA-256 `862e5d176f6a627bcc2e59d254e925b290c0406e8980327ed11356f3494c110b`) suspended at Entropy before a later Boulder and TurnRelics operation. Its producer journal `v12-frozen-fixture-nonpie-9f8ba72ee696` passed with unchanged frozen source. The initial private fixture-driver link failed because the old ASan library is non-PIE; adding matching non-PIE compiler/linker flags resolved that driver issue without altering historical engine source. All older fixture bytes remain unchanged. Old v3–v12 saves initialize new character histories to zero because those writers did not record those facts; the migration preserves the existing encoded payload.

Next: push this source batch and verify the remote ref/tree, then run all suites, original resource conversion, ARM/X11/WASM builds, actual-pack UI, backend comparison and fresh-process v13 persistence against an immutable snapshot. Keep discovered failures explicit and push any repairs before their verification.

### First execution results and follow-up repair

Checkpoint `575b3fce9e12aaa455efc9156b3f904f08780b85` was pushed and read back with exact tree `d382ada89332aa99012d8233b5cb7e3ee41107b4`. Its source-only archive and frozen `nspire-stage-575b3fce` are retained. Native, ARM and X11 compilation all stopped at the same missing comma between the historical and appended status arrays; no C++ suite ran. The converter stopped on the first previously unsupported localization form, `Cards:plural:牌|牌`. All 16 Python tests passed. Every completed journal reports unchanged frozen source. These failures are preserved under `native-final-206dfc24ab7d`, `arm-final-3ec729f66d7e`, `x11-build-ea1756267bda` and `pack-final-f524a467284f`.

The follow-up source repairs that separator, supports the three additional expression forms found by enumerating all appended card descriptions (17 Python tests are now registered), and makes Unmovable block previews distinguish the current suspended CardPlay from another card instance. The latter has a numerical regression paused in Armaments' upgrade choice. Character audit documents are now present. Push these repairs before rerunning; no successful 0.15 runtime result is established yet.

### Second execution results and semantic repair

Checkpoint `433dc2c6920914d67e751417fdee7b93a441cf7f` was pushed/read back with exact tree `994e2b0220e476c030d12f84ebeb66ea7c1357ab`. Its first full native run passed **20/25 suites**: Silent completion passed **16,811 checks across 174 variants**, the historical migration/capacity suite passed 60 checks, and the combined colorless/potion and potion completion suites passed. Ironclad's new suite did not compile because its test used a nonexistent potion helper; the two UI suites retained old cost-sentinel/disabled-button assumptions; Discovery's old test assumed every generated card had a fixed energy cost. The fifth failure was real migration behavior: a suspended v8 Attack was loaded with zero new started history, then incremented finished history on continuation and became invalid. A diagnostic linked to the unchanged frozen library confirmed `started=0, finished=1`; setting the visibly pending started count restored validity. The repair seeds only visible in-progress attacks in historical saves, leaving unrecoverable earlier history at zero.

The second source passed **17 Python tests**, NSP2/12 conversion, X11 and WASM compilation, the real parser (**1,667 checks**), 84 exact native/WASM room frames and eight exact saves. Fresh X11 processes reopened/re-saved v13 ordinary **44,632-byte** and 384-instance **48,820-byte** saves. The first capacity test reached **61,868 bytes**, with conservative bound **61,920 / 65,536**. Actual-pack UI reached all 344 character variants and all 106 colorless variants/64 potions before its known later fixture assertions failed; those partial runs are not full UI passes. The ARM link failed on the new signed 64-bit division helper. The repair keeps the exact combined multiplier and performs bounded unsigned shift/subtract division, with 864 host-reference cases. No target ABI helper is assumed.

The converted pack has **228 cards, 749 strings, 233 auxiliary entries, 1,622 images and 6,265 glyphs**: 7,511,131 bytes, with all 1,460 historical image payloads retained and 877,477 bytes below the resident 8 MiB limit. All second-stage journals report unchanged frozen source. The source-only archive and `nspire-stage-433dc2c6` remain available; the native journal is `native-final-99adca70f281`.

Original listener enumeration also exposed a normal character/colorless interaction: earlier Panache must resolve before later Rage/Juggernaut or Serpent Form. The next source adds a bounded 22-entry player-power acquisition order and serializes each AfterCardPlayed listener in the existing queue. Player powers run in acquisition order, then relics and card-model hooks, then enemy powers. Before-play captures remain separate, and each listener's nested actions finish before the next. Save v13 appends this order; historical saves retain their old Panache-index-before-Calamity policy because they did not record the true acquisition order. Both acquisition directions have numerical and save/resume regressions. New source must be pushed before testing these repairs.

## Completed 0.14 delivery

Runtime source **`b4b67e714ca76e7afb72f3a170789ad90047e6f4`**, tree `5c97f84b8357176ddae5deec5b1911c7b0ff7aa8`, was pushed and read back before final validation. All **21 C++ suites and 10 Python tests passed**. All 12 final journals completed with unchanged frozen source. The build/artifact receipts and scope limits are recorded in [validation-0.14.json](validation-0.14.json). Later changes only update audit/result documentation; distributed binaries are identified by this tested source commit.

## Active branch and recovery receipt

The completed 0.14 branch is `work/0.14-colorless-potions`; new development uses the 0.15 branch above. The user prioritizes GitHub persistence before long validation and parallel agents. Main remains `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0` (historically tested 0.13). CI remains disabled. Reference remains **v0.111.0 / 41cef1ea**. Audio, multiplayer and Timeline are excluded.

The accessible scratch workspaces did not contain the earlier checkout. All 163 source files were restored from work-branch commit `50a58c9b876c3038e549b97e7e7d245224d717d8`. Every Git blob and the complete tree `2fb04a1bb26b50d96d6741cbe2cba17f4a9366cc` were verified, and the exact shallow Git commit was reconstructed. The current working directory is `slay-the-nspire-2`.

Recovery checkpoint `0215379bff2dbb5478def539dd8aa670d35e4e93` and completion checkpoint `b0945483e4f50ca065c0b7c3a0d9f917a2b375e6` were pushed and the work-branch ref read back. The latter's remote tree `554cde3772af4b602c11b9d65abd8d758687211f` exactly matches the staged local source. Source-only archives and isolated test copies are retained locally. Always repeat this verified checkpoint cycle before testing changed source.

Original-rule checkpoint `8adeaeebe4a0380b92ceceff154e30ab5069696c` was subsequently pushed and read back; tree `9c5d5f0f3c5e96c6f96457dc604518b882c7bf28` matched the complete staged source. Its frozen test/build root is `nspire-stage-8adeaeeb`. Follow-up checkpoint `afe33815cfc9f9ab24bbf457cdaeed0cd0e3f2df` was pushed and read back; its complete remote tree `67a0b819cee92d14fec31bd5a8045ec8993cb38d` matched local source. It adds batch auto-play reservations, full-hand pile selection, Touch eligibility and Doom/Demise ordering. Frozen validation root: `nspire-stage-afe33815`. The final tested `b4b67e71` stage records turn-start acquisition order explicitly in save v12 and fixes temporary costs before enemy-side choices.

## Completed 0.14 implementation scope

The engine registers 53 single-player colorless card models, all 64 active potion models, and generated Soul/SovereignBlade tokens: 114 card IDs total, 30 relics and two partially implemented playable characters. These are implementation counts, not a claim that the entire original game is complete. Nine other-character potions and four special-acquisition potions can be exercised explicitly in practice; the other three playable character systems and original special event routes remain outside this stage.

The source writes save v12 (reading v3-v11) and requires NSP2/11: 114 cards, 407 strings and 185 auxiliary images. Historical v3-v9 fixture bytes have not been changed. Extended selections use model tokens for offers and instance IDs for pile choices, and support suspended save/resume. Practice now has a direct HUD compendium entry, card/potion acquisition buttons and accurate full-hand feedback. Acquisition is guarded against use from non-practice runs, the title screen or a paused game.

At `b0945483`, temporary/permanent free-cost precedence and queue-capacity checks were corrected. Old test expectations were migrated to the expanded practice/content/save profiles, and separate colorless, potion and UI acquisition regressions were added.

The `8adeaeeb` source stage corrects random card/potion generation restrictions, distinct generated batches, original potion rarity weights, BeatDown's captured discard selection, Mayhem's turn-start ordering, Anointed's hand capacity, temporary costs expiring on the first completed play, X-cost precedence, Plating's turn-end-only block, and damage-copy effects retaining overkill. Fairy now participates in lethal event HP loss as well as combat. The three new regression suites passed on that checkpoint; the older combined suite retained three obsolete numerical expectations that the next stage corrects against the verified original rules.

The final semantic stage adds one acquisition-boundary field: Rolling Boulder instances before Entropy execute before its choice, later ones afterwards. Mercury Hourglass follows those powers; Blood Vial uses the later hook; early energy/block relics execute before hand draw. The frozen v11 prefix is unchanged and v12 appends the boundary. Old v11 saves retain their former all-Boulders-before-Entropy policy because acquisition history was not stored. Browser saves use an isolated v12 key with v3-v11 import fallback. The X11 reader accepts the full expanded save size, and the full content-build entry point includes the final NSP2/11 conversion.

## Fresh validation results

All native commands run through `tools/run_logged.py` against a verified frozen source snapshot. GCC AddressSanitizer and UndefinedBehaviorSanitizer remain enabled. LeakSanitizer alone is disabled because it cannot inspect the sandbox's process/thread information.

| Source checkpoint | Result | Meaning |
| --- | --- | --- |
| Recovered `50a58c9` | All 18 C++ regressions compiled; 8 passed, 10 failed at runtime. | Failures exposed old save/profile assertions and stale fixture/driver setup; this was not a clean baseline. |
| `b0945483` | All 21 C++ regressions compiled; 16 passed, 5 failed at runtime. | Four older random-play drivers did not resolve ExtendedChoice; the new colorless fixture also used enemy HP outside the engine's legal encounter bounds. These failures were repaired in the later checkpoints below. |
| `b0945483`, potion completion | 7,573 checks passed across 64 model policies. | Numeric effects, availability, selection, acquisition, continuation and save/resume within this engine. |
| `b0945483`, expansion UI | 4,852 checks passed. | All 106 colorless variants and 64 potion entries can be acquired through the actual UI handlers; shop/reward transactions and guards are exercised. |
| `b0945483`, combined content behavior | 7,832 checks passed, 106 card variants and 64 potions. | Initial model-level assertions; not an original-game replay comparison. |
| `8adeaeeb`, full native suite | 20/21 passed; 642,869 checks/assertions reported by passing suites. | The old combined suite first failed on its former overkill-block expectation; its related copy-damage and Plating expectations also require migration. This was not a full pass. |
| `8adeaeeb`, new completion suites | Colorless: 8,697 checks / 106 variants. Potions: 33,831 checks / 64 model policies. Synthetic UI: 4,905 checks. | Original-rule cost, stacking, generation and event-revival additions actually executed under ASan/UBSan. |
| `8adeaeeb`, real content/UI | Parser: 863 checks. UI: 7,374 checks, 106 colorless variants and 64 potion acquisitions. | Nine actual-pack screens were visually reviewed. A detail-cost outline improvement is pending in the follow-up source. |
| `8adeaeeb`, Python | 10/10 event-pack and journal tests passed. | An earlier root invocation lacked the private lz4 dependency path; the complete dependency-configured run passed. |
| `afe33815`, full native suite | 20/21 passed; 616,993 checks/assertions from passing suites. Colorless 8,820; combined 7,832; UI 4,905. | Potion completion stopped at a new batch numeric expectation. A frozen-library diagnostic confirmed Flash of Steel deals 5 and Twin Strike deals 5 twice (total enemy HP 77); the test incorrectly expected 79. Its later single-Flash expectation likewise changes from 89 to 87. |
| `afe33815`, builds and UI | ARM, WASM, X11, content conversion, parser 863, real UI 7,374, Python 10/10 passed. | All 106 colorless variants and 64 potion entries exercised with real art. The detail-cost outline was visually confirmed across the nine reviewed screens. |
| `afe33815`, persistence and backend comparison | Four room scenarios: 84 exact RGB565 frames and eight exact saves. X11 fresh-process v11 reopen succeeded at 14,340 and 19,424 bytes. | The larger case contains 384 card instances and exceeds the former 16,385-byte reader limit; loading and re-saving produced identical bytes. |
| **`b4b67e71`, final native** | **21/21 suites; 651,275 checks/assertions.** Colorless 8,961 / 106 variants; potions 34,202 / 64 models; combined 7,897; synthetic UI 4,905. | ASan/UBSan enabled. Includes real v11 fixture migration, exact acquisition order during suspended choices and costs clearing before enemy-triggered choices. |
| **`b4b67e71`, final platforms and content** | ARM, WASM, X11, NSP2/11, Python 10/10, parser 863, real UI 7,374 all passed. | 106 variants and 64 potions exercised through actual UI. Nine final raw screenshots exactly match the visually reviewed outlined-cost screens. |
| **`b4b67e71`, final persistence** | 84 native/WASM frames and eight saves exactly match; X11 v12 saves of 14,344 and 19,428 bytes reopen in fresh processes. | The large save contains 384 instances and re-saves byte-for-byte. Original-game runtime parity and browser DOM/localStorage remain separate, untested claims. |

The completed baseline, `b0945483`, `8adeaeeb`, `afe33815` and `b4b67e71` validation/build journals report `source_changed: false`. The latest native journal is `native-final-6173a9899340.json`; the reviewed local summary is `nspire-validation-b4b67e71.json`. The final v12 stage passed at `b4b67e71`. A genuine v11 fixture was produced by the unchanged `afe33815` writer under journal `v11-frozen-fixture-3c145f292253`: 12,580 binary bytes, SHA-256 `39469ff1e754da978689eab2e849f85f32fbab61138b2bdfc10bee5664c68a4f`. The passing migration regression reads it instead of regenerating historical data with the new codec. Test source alone does not establish a passing result, and agreeing native/WASM outputs would not establish original-game parity.

## Recovered original inputs and build work

The old missing-volume blocker is resolved. Four complete split ZIP volumes were recovered and streamed through ZIP extraction without constructing another combined archive. The actual release metadata identifies `v0.111.0`, commit `41cef1ea`, and the following extracted inputs match the project's locked SHA-256 values:

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `sts2.dll` | 9,757,184 | `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9` |
| `SlayTheSpire2.pck` | 1,990,705,700 | `c60f672ee7804e6aefa1e19a582fa1c80b126a7b0eef4d084d3abf110df2eab7` |
| Existing NSP2/10 base pack | 6,057,527 | `e157ee346a4eb1fc2849bc2d18e5dc1f4ccf364b85386d92dc7306a1430908a5` |

These private inputs are outside the Git checkout under `reference-inputs`. Do not upload original archives, PCK data, assemblies, fonts, game-derived packs or generated HTML with embedded original art to the source repository. The old NSP2/10 pack is only a conversion input, not a compatible resource pack for the new launcher.

The real NSP2/11 conversion passed at frozen `b0945483` without changing conversion source. The new pack is **6,584,524 bytes**, SHA-256 `7add6827c5a8a271f0f411f1797a2a75cb6b9db36f450a7402f667ff789a12b4`: 114 cards, 407 strings, 185 auxiliary images, 1,460 images, 5,770 glyphs and 17 clips. All 1,334 earlier image payloads are preserved; 1,804,084 bytes remain below the resident 8 MiB pack limit. The conversion journal `real-content-pack-19a1867ba47b` and actual-pack native Content/App rendering journal `real-content-runtime-a2781fd4f4c7` passed with unchanged source. Synthetic packs remain parser/UI test fixtures only.

LLVM 21.1.8 was obtained from official Noble packages, with signed-index and per-package SHA verification, and unpacked privately under `nspire-toolchain`. At final `b4b67e71`, ARM compiled to a 152,721-byte launcher with 10,564,912-byte allocation and 2,124 absolute relocations. WASM compiled to 493,402 bytes and the X11 host to 498,800 bytes. The self-contained offline HTML is 9,452,257 bytes. The content conversion repeated with the same NSP2/11 hash.

Four existing room scenarios produced 84 exact native/WASM RGB565 frame matches and eight exact save matches. This checks the shared engine/storage adapter, not browser DOM/localStorage or the original game. At `afe33815`, X11 fresh-process save/reopen passed with both a 14,340-byte ordinary save and a 19,424-byte / 384-instance expanded v11 save. Both checks passed again at `b4b67e71` with the actual v12 writer: ordinary 14,344 bytes and expanded 19,428 bytes, including exact re-saved bytes in a fresh process. No new Firebird execution or physical-device performance result has been established.

## Handoff and remaining development

The requested 0.14 colorless/potion stage has passed its defined validation and has matching calculator and offline-browser builds. The work branch contains source and audit evidence; private game-derived outputs remain outside Git. Main and disabled CI have not changed.

Further development should begin from this verified work branch: expand the three missing character systems and full character pools, implement original special-potion acquisition events, replace adapted encounters/run structure, and validate a future build in Firebird and on physical hardware. Continue the checkpoint-before-build cycle for any new source changes.
