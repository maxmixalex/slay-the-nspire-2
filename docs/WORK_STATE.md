# UI17 local calculator delivery — 2026-10-30 (Asia/Shanghai)

## Current source, runtime and resources

The complete UI16 source/history and delivered resources were recovered from
`1526ede2a84e168cd46d430338e5d5a540ecaf18`; the older public UI14 checkout is
not this delivery's baseline. The final tested production checkpoint is
**`d4303baf9a720bbeed58c1bb1859ff63343afcd6`**, paired with complete resource
conversion **`44b239e5f41208929661ca70c96545ff65903e08`** (NSP2/41, NSL3).
Subsequent commits add documentation and notices without changing the tested
C++/ARM program or data. The source ZIP records the final documentation/history
tip separately in `SOURCE-IDENTITY.json`.

The ordinary launcher is **1,766,579 bytes**, SHA-256
`1a9a0fb8de2ba7f99a43ccbef3a21d915e72536c9eb62782634a3abdb6f83459`.
The default English/Simplified-Chinese program and data occupy **79,957,013
bytes (76.252950 MiB)**; all sixteen editions occupy **98,499,241 bytes**.
Compared with UI16, these save **1,065,701** and **1,077,040 bytes** respectively.
The common index/eight parts occupy 72,957,060 bytes. The earlier 65–70 MB
installation target remains unmet; ZIP compression is not used as an installed
size, and no additional image, frame, glyph or string was removed.

Zehn requests **22,269,692 bytes**, including 157,220 metadata bytes;
contiguous execution/static memory is **22,112,472 bytes**. Total allocation
falls by **253,792 bytes** while retaining the complete 13,369,344-byte picture
pool. The 22,450,000-byte execution and 24 MiB total limits pass. These figures
are not physical peak RAM or a guarantee for arbitrary heap fragmentation.

## Implemented UI17 work and preserved feedback

- Reversible TGI2 frame-index prediction saves **807,004** common bytes across
  496 smaller groups. Existing tile pixel streams remain unchanged.
- Complete retained-image RGR2 filtering selects **608** additional smaller
  groups and saves **262,042** common bytes without another decoder or pool.
- Streamed resident image rows shrink from 16 to 12 bytes; direct-memory rows
  keep their layout. The 1,835,012-byte directory fits all actual locales and
  the existing maximum-image fixture. A retained 256-byte functional address
  table removes repeated NSL3-backed directory reads.
- Exact large constant-color RGB565 blending and hover-list reuse avoid
  repeated per-pixel and inactive-slot work. Foreground polling and decoded
  pixels remain unchanged.
- Selected **e3 stat/deferred-header** I/O keeps required-file presence/length
  checks and validates each functional header before payload use. The a8
  directory-enumeration experiment is preserved in history but **rejected**
  from production because actual OS latency was worse.
- A latched resource-failure guard now blocks subsequent input and actual
  run/history/settings/profile/language write callbacks after a live failure,
  including the same App operation. Built-in technical errors are English.
- Original Unicode seed canonicalization and the separate raw legacy hash
  path are connected to preview, run creation, RNG, save and Continue. The
  field remains bounded to 64 UTF-8 bytes. This closes the earlier ASCII/raw
  legacy omission; it does not add a calculator Unicode input method.

The [feedback ledger](ui17-feedback-acceptance-0.17.md) reconciles all nine
reported items against source and implementation. The index plus eight
resource concerns, language fallback, 8 px deck sort text, tucked card center
(18,78), original locale font roles and treasure-ribbon title correction are
retained. The pointer-only View Upgrades checkbox, distinct Tablet/Messy and
Smith effects, and 20/29 px ordinary/focused map nodes reflect the original
behavior. Previously missing monster-command and Vantom work was implemented
in UI15 and remains present; it is not counted as new UI17 work.

## Executed acceptance

The complete final native suite passes **152/152**, the ordinary ARM build
passes, and changed retained/storage Python coverage passes **21 tests**.
Actual sixteen-language coverage passes **89,816 checks / 53,072 descriptions**.
The independent old/new resource comparison checks all **58,060 images /
1,167,930,140 decoded bytes**, glyphs, strings, clips and complete UI frames.
Final d430 English and Chinese provider replays match complete RGB565/State
bytes through both new providers and against the old UI16 results. Scoped
ASan/UBSan, Unicode, failure-guard and allocation results retain their exact
source identities and instrumentation scope in the component reports.

Matched ordinary-OS Firebird uses unchanged upstream core
`b10f3b51a9ab8e27d2703444b5e6bd278d4c5e6e`, CX II CAS / 396 MHz / OS 6.4.0.74 /
Ndless r2022. Selected-resource validation plus App initialization falls from
**1,518.372 to 1,104.462 ms** (**27.2601%**). The fixed action window changes
from **28.19176 to 28.45701 FPS** (**0.9409%**), with resource elapsed time
falling from **272.55249 to 213.16528 ms**. These are newly matched final tests,
not the historical UI16 setup or a native renderer microbenchmark.

Final warm combat Save & Quit, immediate no-USB reopen and Continue preserve
the saved Turn 2 state. Chinese cold default/reopen and French warm
default/reopen pass; language preferences read back as `zhs` and `fra`.
Dutch cold default opens English and records `eng`. Two earlier Dutch warm
USB sessions stalled before game execution; their failed records remain and
are not counted as successful warm launches. French-requested corrupted-part
and missing-part scenarios show built-in English errors on launch and
immediate reopen. The 230,352-byte run and 17,591-byte progress files remain
byte-identical after both fault sequences.

Cold scene preparation can still stall. No physical-handheld timing,
universal 30 FPS, zero cold stalls, arbitrary fragmented-heap support, natural
three-act victory or measured restoration percentage is claimed. See the
[integrated delivery report](ui17-delivery-0.17.md) for the exact scopes,
package identities and retained representation/history limits. The agreed
Ancient static scene exception and excluded audio/multiplayer/Timeline remain.

## Installation, history and publication

Install the matching launcher, small `data/nSpire2.dat.tns` index, **all eight**
concern parts and desired language files. Keep English for fallback; the
fourteen additional locale files are optional. Preserve the complete existing
`state` directory. For a monolithic older installation, replacing the large
common file with the small index first releases its occupied space. Complete
the full transfer before launching.

The private calculator ZIPs contain no test saves, preferences or profiling
markers. Source is recovered from its final committed snapshot and complete
all-ref bundle; the source-only verification collection preserves original
failures, interrupted commands and unfinished journals with their statuses.
The separate device evidence keeps measurements and screenshots without
original assemblies/fonts/game packs, Boot1/NAND/RAM or user/test saves.

Source publication remains **blocked by automatic approval review**. Personal
Context recovered the user's historical repository authorization this turn,
but the new review did not accept it as current authorization for this outbound
source/history payload and destination. No alternate transport bypassed the
rejection; no new remote main update or remote CI pass is claimed. The complete
local commits, history and reviewable source archive preserve the work.

## Historical UI16 completed local calculator delivery — 2026-10-29

## Frozen runtime, resources and current outcome

The tested runtime is **`4f3c7abfb1c580d37f63a158385ff79a47e87874`**.
Matching complete data is converter checkpoint
**`57cd512e56d3a10f6cb8b424cbae969c8216b2d3`**. Later commits change only
verification launching/documentation and do not alter the shipped production.
The final source/documentation checkpoint is in the source ZIP's
`SOURCE-IDENTITY.json`. Original UI15 source, private selected resources and
complete history were recovered before this work; do not restart from the
older public checkout or repeat rejected experimental branches as production.

Launcher: **1,760,617 bytes**, SHA256
`cf83955453bc6046e57895a94efdf68ecf00a6c6e4ba2eec5e3a172c94f36158`.
Zehn allocation: **22,523,484 bytes**, including 157,096 bytes of metadata;
contiguous execution/static memory: **22,366,388 bytes**. Allocation is
**356,412 bytes below UI15**, with the entire 13,369,344-byte picture pool.
The 22,450,000 execution and 24 MiB total budgets pass. These requests are not
a physical peak-RAM measurement or an arbitrary-fragmentation guarantee.

Default English/Chinese installed program/resources total **81,022,714 bytes**
(77.269281 MiB), saving **888,086 bytes** against UI15. All sixteen with launcher
are **99,576,281 bytes**, saving **6,204,170**. Saves/notices are excluded from
these counts. The index is 1,520 bytes / 89 spans; common index/eight concerns
sum to 74,026,106 bytes. Default resources alone save 893,671 bytes. The prior
65–70 MB installation target remains unmet; no additional texture pixel,
animation frame, string or valid glyph was removed for these savings.

## Implemented changes and feedback decisions

- The large common file is now an index plus interface, cards, characters,
  enemies, bosses, rooms, effects and shared parts. Four live artwork streams
  serve eight logical sources. A matching complete set is required.
- Independent 4 KiB NSL3 locale pages compress all sixteen editions. Inner
  NSL2 tail alignment and complete short A/B/A span folding reduce unnecessary
  reads without changing any logical NSP2 byte. Mixed-page requests coalesce
  through a bounded 32 KiB encoded batch; the added 28,676-byte adapter storage
  is counted in final allocation. Single-probe startup obtains each part's
  length/header together, halving actual initialization opens from 16 to 8.
- Exact opaque/flying-card rendering and ARM memory words reduce rendering
  work. State ownership, Random/merchant/selected-hand raster sharing and
  tighter addressing tables lower static allocation. No image-pool cut.
- Deck sort text is 8 px instead of 6; the tucked played-card center is (18,78)
  instead of (31,88), shared with the moved eye's layout; treasure's single-line
  10 px title uses the projected banner interior.
- Original pointer-only View Upgrades checkbox, distinct Tablet/Smith effects,
  and ordinary map node 20/29 px controls with full atlas margins are retained
  after reference verification. They are not newly introduced bugs or new
  features merely because their tests pass.
- Normal label / explicit rich bold / explicit rich italic roles match the
  original language substitutions at the audited controls. Chinese Sans and
  Serif roles intentionally coexist. All relevant sixteen-language glyphs and
  original-family samples pass. Three pre-existing optional PTB/TUR italic
  glyphs were repaired from their exact source fonts.
- Healthy saved language wins; then system language, English and other usable
  editions are tried. Unsupported calculator languages use English when
  available. Failed startup candidates are removed for the session; manual
  switch failure restores the previous language. Technical errors are built-in
  English and do not depend on a working game font pack.

## Completed gates

All final journals have `status:passed` and `source_changed:false`:

| Gate | Journal / result |
| --- | --- |
| Full ARM build | `ui16-combined-final-arm-59335313de91` |
| Full native | `ui16-final-full-native-9a366dc71d23`: **147/147** at verification checkpoint `99063e30349500b6e845f8a46a2e831fd5d86163` |
| Actual all sixteen editions | `ui16-final-all16-b6743ae37fd5`: **89,816 checks**, 53,072 descriptions, zero failures, 90 English recoveries |
| Full actual English stream | `ui16-final-actual-eng-cbb52fdc06c1`: **12,485 checks**, 76,368,876 logical bytes identical |
| Full actual Chinese stream | `ui16-final-actual-zhs-f0c0169450ed`: **12,619 checks**, 80,793,488 logical bytes identical |
| Final package | `ui16-combined-final-package-f162c3321418`: every archive member read back, matching logical defaults and fourteen optional languages |

Independent changed-path ASan/UBSan checks, exact 182-frame/canonical-state/
logical-request oracles, 2,645 file-cache/header cases, 137,139 boot-language
cases, 3,166 required glyph records, 48 font samples and ARM vector checks are
recorded at their actual checkpoints in [the delivery ledger](ui16-delivery-0.17.md).
Do not combine them into an unexecuted full-game sanitizer claim.

The environment can drop generated ELF execute bits. Final CTest runs the
unchanged programs through their declared ELF interpreter, preserving all
147 registrations, arguments and properties. Earlier BAD_COMMAND failures
remain recorded. The first combined native build completed all 257 build steps
but its helper rejected 147 registrations sharing 146 executables before tests;
that accounting error was fixed and the full 147-suite run passed. No test or
production behavior was removed to obtain a pass.

## Actual Firebird result and material limits

Configuration: **CX II CAS / 396 MHz / OS 6.4.0.74 / Ndless r2022**, with
user-owned NAND/Boot1 and unchanged upstream Firebird core. No physical-handheld
timing claim. Both final lifecycle checks pass: cold Chinese title → Quit →
immediate no-USB reopen, and the original warm Haunted Ship combat → Save &
Quit → title Quit → OS → immediate no-USB reopen → normal Quit.

The unique final six-second five-Dazed control/action pair yields 164 frames,
196,282 interval ticks, 139,380 render ticks and 14,837 resource ticks;
19 reads / 113,070 requested bytes / 19 seeks. Intervals over 50/100/250 ms are
6/4/0. Mean elapsed render stage is **25.93622 ms**, about **2.17% below UI15's
26.51095**. Whole-window FPS is **27.37873 versus 27.55971**, about **0.66%
lower**, not an improvement. Finite exit-boundary frames can differ. Render
elapsed time may include synchronous resource work and is not a CPU-cycle
profiler. Native CPU results do not establish calculator FPS.

The initial split candidate's 31 reads and roughly 288 ms action interval were
corrected; the final window has no interval above 250 ms. Resource time still
exceeds UI15 by about 120.57 ms, despite restored read count and fewer bytes;
that remaining difference is not assigned an unsupported cause. Startup
selected-resource validation + App init is **43,538 ticks / 1,328.674 ms**, versus
UI15 **21,790 / 664.978 ms**. This scope remains slower and excludes complete
Zehn launch. Do not substitute the better old-container renderer experiment.

Earlier def1db3 captures retain their actual identity: the 196-input UI route,
cold French auto-selection, cold Dutch English fallback and missing-required-
part English recovery. These are not relabeled as final-runtime screenshots.
The final source changes do not repeat that whole route; scoped lifecycles and
the controlled action pair cover the final memory/storage changes.

Warm French OS immediately after language change plus USB has an observed
largest free block of 22,145,408 bytes, below the final execution request before
metadata. Cold French passes. Arbitrarily fragmented OS heaps remain unsupported
without restart; failure happens before main, not during French font loading.
The failed old/new candidates remain in [Firebird evidence](ui16-firebird-0.17.md).

## Deliverables, installation and publication

The completed player archive is
`Slay-the-Nspire-2-0.17-ui16-bilingual-calculator.zip`, 80,240,252 bytes, SHA256
`062e30629c14deb4a6a067f55a454e55f1b8d95d482a541be9dec9eae2bf75f5`.
The fourteen-language archive is 17,076,337 bytes, SHA256
`9a959f20709c448e20851974454d861310e0faaa49a6e7ae050b127a7963a840`.
The complete source recovery ZIP includes the committed source/docs, all local
Git history and final verification journals/logs; its own exact identity is
in `SOURCE-IDENTITY.json` and the companion archive receipt. The separate final
Firebird evidence includes prior failed candidates and final lifecycles/raw
pairs, without NAND, ROM, DLL or PCK. Source-only archives contain no derived
resource pack. Player archives contain no test save or profiling marker.

Preserve `state`. For an old monolithic installation, **replace the old
`data/nSpire2.dat.tns` with the small new index first**, releasing its roughly
74 MB, then transfer the complete matching concerns, desired languages and
launcher before launching. Keep English for fallback. Only desired additional
locales need installation; do not require all sixteen to fit together.

Retained fidelity boundaries remain explicit: accepted static Ancient actor/
background animation, ASCII seed editor (Unicode normalization and deprecated
lowercase `old` path unimplemented), LCD/particle/material mappings, independent
cosmetic RNG, UTC and finite history. UI15's actual monster command programs,
Vantom and earlier effect implementations are recovered and retained, not newly
completed by a probe. Audio, multiplayer, Timeline and desktop/network services
are outside scope. No 95% score, universal 30 FPS, zero cold stalls or natural
three-act victory is claimed.

GitHub publication remains **blocked**. Automatic approval review rejected the
source push because sufficient outbound payload/destination authorization
evidence was unavailable. Personal Context is unavailable here; recovered
project instructions only transcribe earlier permission. Do not bypass or
retry without new authorization evidence. No remote ref/main synchronization
or new CI run is claimed. Recover from this completed source archive and its
all-ref bundle, preserving failed experiments as history.

## Historical UI15 delivery — 2026-10-28

## Current source and delivery state

The selected delivery runtime is local checkpoint
`bd158d205d877f767f59fe2a1eb33874cb925899`, with complete strings15 resources
from converter `de0941ebe70a8d92cd124447ea7cb6d30b4f1524`. All functionality,
input, save/Continue and named correctness gates are closed. The exact player
and optional-language ZIPs have been created and read back successfully.
The selected combination's last controlled map→Shop window also completes
actual touch opening, normal OS exit and USB counter cross-checks. This is a
**completed playable delivery with measured performance limits**. Merchant
first-opening stalls remain: saved-Shop peak256.012ms and map-entry
peak549.438ms. No zero-stall, universal30FPS or physical-device claim is made.

The f67+strings15, f67+idle21 and99ea+idle21 experiments passed correctness but
are rejected for performance. V22 reduced saved-Shop text work, but its real
map→Shop opening regressed to808.289ms and four frames over250ms. Its actual
screens, normal OS exit and USB counters confirm the result. A better
saved-Shop result cannot substitute for the normal map path. Production has
been restored byte-for-byte to bd158; all1067 non-document tracked files
match that immutable source. None of the three rejected experimental file versions
belongs in the delivered runtime or converter. Their commits and failure
evidence remain in the local history.

Source-only archives and local Git history preserve the coherent stages before
long jobs. Repeated remote connector writes did not return a successful ref
receipt; `remote_published:false`. Do not repeat those hanging writes or claim
a new remote main/CI result. The source delivery preserves complete source and
available local UI15 history; the externally published UI14 history is a
separate recorded upstream, not a fabricated local ancestor.

The feedback map is [ui15-lcd-followup-0.17.md](ui15-lcd-followup-0.17.md);
exact stage identities and outcomes are in
[validation-ui15-0.17.json](validation-ui15-0.17.json). The complete pinned
v0.111.0 / `41cef1ea` assembly/PCK inputs are recovered. Monster awaited
commands and Vantom's continuous bone transition are actual implementations.
The owner's corrected zero-ascension behavior is applied. No old installation
or obsolete-save compatibility work is part of this change.

## Completed correctness gates

Integration9, `a04451c07e1d63fa50cf95466381fce2a0d96ff8`, passes140/140 native
suites,354 Python checks and portable compile-only. Complete actual resources
pass all16 languages and53,072 descriptions with zero unexpanded templates.
English/Chinese frontend, deck, event, HUD, Bestiary, original screen effects
and complete Vantom traversal pass their named actual-art gates. No manual
browser/WASM acceptance was performed.

The original full ASan/UBSan baseline is136/140: four real failures share one
intermediate out-of-bounds enemy-clip pointer expression. Production and its
generator now evaluate the integer subtraction before pointer addition. At
`8c2d6e9de3a4f2e4a7b7af0275bf275274bd9ad5`,18/18 native and18/18 sanitizer
suites pass, including all four original triggers, with nine actual-resource
paths in each. That clean delta does not relabel the original136/140 sweep.

Integration16, `a94050d53ccd8c437be69dd42ebeb10e0f507b8f`, passes20/20
selected native and20/20 sanitizer suites with nine actual-resource paths in
each: all16 languages/53,072 descriptions, merchant21,323, ENG/ZHS deck
14,672/55,004, Random4,899,908 each, Spark7,366,687, screen effects2,122,392
and Vantom6,596 checks. The latter omits PPM file-byte write assertions from
an older larger count; semantic, texture, attack, shrink, death and restore
cases are unchanged. The42 executed ELF identities and unchanged17 resource
hashes are retained. Portable compile-only passes.

The bd158 changed-path native and sanitizer gates each pass6/6 selected
suites and six actual-resource paths. Merchant, hover resources and complete
merchant occlusion pixels are included. Its formal ARM build and portable
compile-only pass. The pointer regression executes267,700 checks, including
contact origin/movement, press/release, side ownership, held-key repeat during
touch jitter, nested capture and FIFO overflow release. A negative control
retains the former repeat-starvation failure rather than weakening the gate.

The subsequent f67 merchant scheduler passes its synthetic/actual native and
sanitizer gates and portable build, separately from its rejected performance.
Idle21's necessary resource delta passes actual ASan merchant ENG30,063 and
ZHS32,314 checks and all16 languages/53,072 descriptions/38,686 checks. All17
resource files are unchanged across execution. V22's native ENG30,404 and
ZHS33,025 paths pass, including immutable preview state, RNG, progress, pixels
and label clocks. V22 ASan/UBSan passes the synthetic merchant suite and the
same actual ENG/ZHS counts; portable compile-only passes. Four execution
journals retain `source_changed:false`. These correctness gates do not
pre-approve its later rejected ordinary-OS performance result.

## Ordinary calculator OS acceptance

The natural integration9 run starts legally, completes Neow, wins the first
three-Slime fight, takes gold/card rewards and waits for Proceed. SaveQuit,
visible Continue and a second SaveQuit preserve the same7590-byte save:
HP80,114gold,12cards,floor2. Exit returns normally to CX OS3.1/Ndless2022.
This is a natural first victory, not a natural three-act victory. Stack fixes
remove the original first-combat data abort without reducing render/effect
capacities or adding another large pool.

The bd158 ordinary-OS input acceptance has7/7 completed journals. A50ms card
selection,50ms right target change and one50ms Enter hit the intended middle
Slime26→20 and consume exactly one energy. Complete touch contact/movement
captured during foreground decoding now opens the merchant rug using the
original100/100/180ms gesture. Two SaveQuit/normal-OS exits and a fresh Continue
preserve the same7430-byte save and backup, SHA256
`041d9a2c072391cd5c4530a45b6ce37728d41d50d0e104e5f846699db3192179`.
This short-action first-combat save is a different scene from the7590-byte
natural-victory save above; neither substitutes for the other.

Default language selects Chinese from actual OS `zh_CN` with no saved
language override. Official Ndless2022 on OS3.1 lacks the two optional
math-language bindings; the settings-XML fallback reads the real system
locale. Its shared parser/selector/SDK adapter has906 passing checks.

Integration16's controlled Haunted Ship fixture executes the real enemy turn.
Screens show five Dazed growing at2875–3000ms, holding and flying into Discard
at4375–4500ms. Normal SaveQuit/OS exit and production decoding confirm exactly
five Dazed,15combat cards,10deck cards,HP80 and enemyHP63. The7320-byte save is
byte-identical to v14's same action. This is a controlled encounter, distinct
from the natural run and the bd158 short-input save.

The bd158 ARM is1,755,032bytes, SHA256
`8596f9c1ed5778d229944adf0fbde3d9e2f9c22d770b7ce8fef11c6465666555`,
with22,879,896bytes linked allocation and largest function frame5,952bytes.
The rejected v22 candidate was1,756,187bytes with22,883,724bytes linked
allocation. Its changed-path native/sanitizer and ARM gates pass, but that
correctness result does not override its actual map-opening regression.
The selected bd158 main/input/Canvas remain unchanged; input capture does not
alter canonical state, RNG or animation clocks before normal FIFO dispatch.

## Measured performance and rejected experiments

These windows use ordinary Firebird CX132MHz, AHB66MHz, APB33MHz, JIT enabled,
SP80432768Hz, hardware1 and Ndless2022. They are not physical-calculator
measurements. Completed read-only windows are cross-checked against normal
exit USB performance reports. Peak-frame reads and whole-window reads are
kept separate. Merchant comparisons retain the fixed original script:30s
startup/title interval,7s after Continue or committing the Shop route, then
100ms contact/100ms movement/180ms press. They measure the first opening on
that route, not an arbitrary zero-preparation click.

* Cold merchant opening progresses from v8's2,241.669ms peak through1,532.013ms
  with independent card streams,776.093ms at inventory13 and388.062ms at v15.
  V15 has24reads/113,637bytes in its peak frame but167reads/1,082,490bytes in
  the complete first-open window. Its controlled map→Shop path peaks450.928ms.
* Bd158's actual first-open window is11.8485FPS with132reads/919,686bytes and
  a256.012ms maximum (8reads/39,909bytes in that peak frame). It has13frames
  above100ms and one above250ms. Closed-room steady rendering is30.0072FPS;
  a warm opened rug is14.0319FPS,70.2674ms mean render, zero reads. These are
  measured limits, not a universal30FPS promise.
* The final bd158+strings15 controlled map→Shop route completes108frames in
 10.932098s,9.879165FPS, with255reads/1,409,390bytes. Its549.438477ms peak has
 57reads/254,550bytes. The window contains35frames above100ms and three above
 250ms. Actual opening screenshots, normal OS exit and USB counters agree.
  It is better than rejected v22's808.289ms but slower than the earlier v15
  map result450.928ms; the final choice retains the completed short-input and
  pointer fixes. This remaining cold stall is not described as eliminated.
* The f67+strings15 trial regresses to278.534ms,143reads/976,626bytes and
 11.8439FPS. Isolating145 Top frames in idle21 reduces their cumulative cold
  decoding bytes but the same scheduler then regresses to374.329ms,
 178reads/1,141,271bytes,10.9577FPS and two frames above250ms. Both performance
  candidates are rejected; passing resource/sanitizer checks does not accept
  their latency. V22 restores consecutive frame locality and previews saved
  card templates: its saved-Shop window is12.0945FPS/110reads/762,561bytes with
  a256.104ms peak, but its map window is9.8681FPS/279reads/1,490,184bytes with
  an808.289ms peak. That combination is also rejected. Its stable warm-rug
  result of14.0105FPS does not conceal the normal-entry regression.
* A v17 script missed initial touch contact and never opened the rug. Its
 153ms/21.97FPS closed-room window is excluded. The contact/pointer defect is
  now fixed by bd158; that invalid scenario is not rewritten as an improvement.
* Matched earlier partial-resource Random windows are UI14 6.681906FPS,
  immediate pre-span2 8.144128FPS and retained exact span2 9.255827FPS.
  A slower8.547027FPS experiment is rejected. Full inventory13 v14 is
 8.994170FPS; full strings15 integration16 is9.415104FPS (282frames in
 29.951874s),105.2757ms mean render, zero reads, maximum126.5564ms and no frame
  above250ms. These are whole-window observations, not isolated attribution
  of every difference to the exact RGB565 half-black optimization.
* Integration16's full cold status action is11.3063FPS at0–2s,18.7890FPS at
 2–4s and10.1965FPS at4–6s; complete0–6s is13.4592FPS. The first interval
  includes hand discard/turn banner and three frames above250ms, maximum
 293.9453ms. Growth/hold and flight windows have no frame above250ms. Later
  idle frames are not included to inflate the action result.

The historical UI14 20.391FPS Random figure lacks a matching recorded CPU
field. The emulator's132MHz CX versus396MHz CXII split is an audit inference,
not a proven rewrite of the older measurement. Do not compare it directly to
these132MHz windows. Cooperative capture improves input reliability, not FPS.
The minimum hardware sample interval is8ms, not a guaranteed maximum latency.
A proposed16ms idle touch cadence was not implemented: existing whole-update
timings bound any saving too low to explain the warm-rug rendering difference.
No dominant I2C cause or unexecuted A/B benefit is claimed.

## Resource identity and delivery layout

Strings15 converter `de0941ebe70a8d92cd124447ea7cb6d30b4f1524` preserves all
images, glyph pixels and139,264 string rows/8,090,052 UTF-8 bytes. Small strings
and fonts use4KiB cohorts; selected card/inventory images use independent
bounded streams. Its common pack is74,024,394bytes, English1,130,938bytes and
Simplified Chinese5,000,436bytes. All17 SHA256 identities are in the manifest.

Idle21 converter `176eed5b40ddb9d47d87e7747373d9ed8eab78e4` changes compression
boundaries for only145 original Top merchant idle frames. All decoded images,
1,655 animation streams, glyphs, strings and clip timings remain exact. It
adds187,034bytes to common and2,102bytes across all16 locale files, with zero
runtime memory added. Its common is74,211,428bytes, SHA256
`73c2f46e48e4c9b97497934c58632981c7e5d4048f5184b0bf4b927400d91c26`;
English1,131,493bytes; Simplified Chinese4,999,653bytes. Its decoded-resource correctness passes, but idle21 is excluded from the
shipping combination after the actual map-entry regression. Strings15 is the
selected delivery resource edition.

Strings15 common saves2,652,656bytes against the same complete UI15 artwork
before grouping but is1,893,260bytes larger than UI14. Idle21's corresponding
figures are2,465,622bytes saved internally and2,080,294bytes larger than UI14.
These are lossless post-LCD grouping changes, not dropped art or animation.
No net storage reduction against UI14 or completion of the earlier65–70MB
installation target is claimed.

The verified player ZIP contains only `nSpire2.tns`, `data/` and empty
`state/` at its root. English/Chinese are included; fourteen other locales are separate.
No test saves, profiling marker, ROM, snapshots, original DLL/PCK or embedded
browser player belong in the player/source delivery. Audio, multiplayer and
Timeline remain outside the agreed scope, together with the accepted full
Ancient-actor animation storage exception. Ancient gameplay, dialogue,
healing and card materials are implemented.


## Verified package bytes

The calculator-only packaging job uses the immutable bd158 source, reports
`source_changed:false`, and reads back every ZIP member against its actual
input. A second inventory check confirms14 player members,15 optional-pack
members, the exact source/resource identities and empty state directory.

* `Slay-the-Nspire-2-0.17-ui15-bilingual-calculator.zip`:80,205,686bytes,
  SHA256 `0a497a213b8a4ac533afd2f49fbc06c7d05bcfb48b30fa4e158850f26c801813`.
* `Slay-the-Nspire-2-0.17-ui15-optional-languages.zip`:17,016,324bytes,
  SHA256 `9e33741f3aa27efc5e3804665be020c48aba067cec3e2f20e057d13698a97f56`.

The program and three default resources total81,910,800bytes before saves;
including packaged readme/licenses, the ZIP's uncompressed members total
81,975,073bytes. Only the fourteen additional languages are in the optional
archive. Rejected v22 ZIP byte payloads were removed after preserving their
hashes, inventory and readback receipts; they are not delivery artifacts.

The source delivery is a documentation-only successor of bd158, with complete
source plus available local UI15 Git history. Its own commit/archive identity
is recorded externally to avoid a self-referential manifest. No original game
art/font payloads, game assembly, ROM, generated packs or test saves enter
that source archive. No new remote main or GitHub Actions result is claimed.


---

# UI14 calculator delivery — 2026-10-27

The verified production runtime is `af154470daa96563f0695d043178285d26dcaee4`
(stage 16); the resource/converter checkpoint is
`67e2822ef369dc62428dcc0698c0bda62586756f`. Source was pushed and changed
blob hashes/modes were independently read back before long jobs. Later
metadata and documentation changes do not alter the delivered runtime.
The complete feedback map is [ui14-lcd-followup-0.17.md](ui14-lcd-followup-0.17.md),
and exact identities, completed gates, named device windows and remaining
boundaries are in [validation-ui14-0.17.json](validation-ui14-0.17.json).

The final ARM is **1,689,486 bytes**, SHA256
`98d1408183e3bff8d99b6fd0cac7143ad7a25783074a3a2297cf69a04adcc375`,
with **22,676,740 bytes allocated**. It starts in ordinary Firebird CX OS.
All four installed program/resource files were read back through USB and
compared byte for byte. That installation has a complete 104-event trace,
normal stop and no failed operations. The earlier 22,983,964-byte stage 9
allocation failure is why the nominal 24 MiB ceiling is not used as a substitute
for the actual ordinary-OS startup gate.

## Implemented behavior and validation

The console is compiled into every build. The only installation-root file is
`nSpire2.tns`; data and documentation are under `data/`, and all mutable state
is under `state/`. No old filename fallback or alternate debug launcher is
included. Fixed frontend, HUD, deck, defeat, scrollbar, event, reward, card
material, scaling, forge, enchantment, rest and merchant behavior is mapped
individually in the feedback ledger; its coverage is not a blanket claim that
every historical source or performance difference is closed.

The final **138/138 local native suites pass in 37.64 s**, using Clang 18.1.3
Release (`-O3 -DNDEBUG`). This is separate from GitHub's Debug/-O1 ASan/UBSan
job. The final Random checks pass 297,749 synthetic and 610,601 per actual
English/Chinese resource edition, with 8 backgrounds and 4 complete pages
byte-identical to independent earlier renderers. The final combat original-art
gate passes 11,902 checks. The unchanged converter/package Python source has
334 unique passing tests. All 16 final locales pass 38,686 checks and 53,072
descriptions with zero unexpanded templates.

The production runtime's [GitHub CI run 40427358640](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/40427358640)
is complete: native, conversion and portable jobs all pass. Its separate
Clang 18.1.3 Debug / `-O1` ASan/UBSan run passes **138/138 in 253.87 s**.
The historical stage 11 fixture failure and its stage 12 correction remain
recorded in the manifest; no sanitizer coverage or strict assertion was removed.

The final ordinary-OS Continue follows the saved character's textured wipe,
black transition and room entry. The genuine earlier natural run resumes at
56/80 HP, 114 gold, one potion, 11 cards and floor 2; map, Save and Quit, title,
normal OS exit and USB state readback all succeed. Its 7,377-byte canonical save
remains byte-identical to the completed natural stage 7 run. The UI progress
codec records the expected seven additional active seconds and new save
timestamps; all other encoded progress is unchanged after normalizing only
those time fields. The earlier natural loop includes Neow, legal map travel,
a seven-turn Nibbit victory, 15 gold, Colorless Potion and Headbutt. This is
**not a natural three-act victory**.

## Measured performance boundaries

Final Random's matched 30 s warm window is **611 frames / 29.963654 s = 20.391372 FPS**,
with 48.7053 ms mean render, zero reads, zero resource failures and no frames
above 100 ms. The first 10 s window is 19.034958 FPS. The retained water, original
light and all 40 particles are pixel-exact, and warm throughput is about 2.07×
the earlier 9.8567 FPS record. It is still below 30 FPS; the original low-FPS
feedback is improved, not declared fully eliminated.

Ordinary and Random Embark both reach Neow and save/exit normally. Ordinary
Embark retains an 800 ms visible departure, but its initial input/new-run
update is about 1.795 s. Random selects Silent and retains the 1000 ms reveal
followed by 800 ms departure. The 13/38 recorded Character frames include cold
preparation and are not animation-only FPS. The frontend ledger preserves
all timestamps and the distinction between the agent's incomplete install
trace and root's complete independent installation evidence.

The real stage 13 Haunted Ship turn shows five generated Dazed at 29.5419 FPS
in the 2-4 s preview window, and flies them into Discard while drawing the next
hand at 28.9576 FPS in the 4-6 s window. The complete first 0-6 s action is 25.3955 FPS
and still contains one frame above 250 ms. A 12 s record includes settled idle
and is not used as the action's FPS. Its checkpoint remains explicitly 13;
it is not relabeled as a final 16 or physical-device measurement.

The stage 12 closed merchant reaches 30.0073 FPS in the matched 60 s window after
120 s, with zero reads, frames above 50 ms or resource failures. The first rug opening
still peaks at 3.7716 s render / 3.8987 s frame interval. It has no invented loading
animation, and the complete goods and dialogue layout are verified, but this
cold spike is not called resolved. Exact merchant factoring preserves all 1,817
source frames; the 120-frame rug comparison has 9,216,000 identical pixels.

## Delivery size and remaining source inputs

The bilingual install's four program/resource files total **80,021,945 bytes**
(80.021945 decimal MB), which is 1,452,499 bytes larger than UI13 after adding
missing original art. The 6,215,294-byte merchant moving-payload saving is
against the intermediate independent-frame UI14 encoding, not a net reduction
of the complete UI13 install. The earlier rough 65–70 MB goal remains unmet.
Both player ZIPs contain no controlled save, ROM, snapshot, browser player or
separate debug variant. The calculator ZIP contains an empty `state/`;
the optional-language ZIP contains only files under `data/`.

Two reference-fidelity items remain blocked on the removed pinned originals:
complete surrounding awaited commands for every monster move, and Vantom's
750 ms `root_scale_adjust` bone transition with its independent tail control.
The 196 AttackCommand profiles and sourced move waits remain; Vantom's full/
half endpoints are correct for both shrink sources. Do not substitute guessed
waits or whole-sprite shrinking, and do not turn 123 move nodes outside the
AttackCommand profile table into an invented missing-rule count. Local files,
preserved evidence, Library and
the connected Drive's scoped filename searches did not recover the DLL/PCK.
The precise required paths/hash are in the retained-differences ledger and
validation manifest.

The accepted full-Ancient-actor-animation storage exception remains. Special
card materials use the disclosed bounded CPU phase sampling. Audio,
multiplayer and Timeline remain outside the agreed offline single-player
scope. The actual final package hashes and verification scopes, rather than
historical headings below, govern this delivery.

---

# UI14 integration checkpoint — 2026-10-26

Stage4 source `97a63d78979daba7d12fa8475b305e836fc9503c` builds both
native and ARM. ARM is 1,677,286 bytes with 22,738,604 bytes allocated under
24 MiB. The 277 discovery and 56 supplemental Python tests pass (two tests
are present in both lists). All sixteen actual locale packs pass 38,686
checks and 53,072 descriptions, with no unexpanded templates.

The first complete native run passed 129/138. Two executables had local
non-executable permissions; the remaining failures exposed stale geometry /
old memory-budget expectations, an invalid new test fixture, and a real
small-card cache layer-order difference. This checkpoint fixes the identified
source/test issues without weakening pixel-equivalence assertions. It also
adds the original Big Mushroom, Phantasmal Gardener and Test Subject scale
changes, and skips image I/O for fully hidden entrance/exit layers. Fresh
native and ARM validation remains required for this checkpoint.

The ordinary OS has executed the new installation paths, created state/,
started a natural Ironclad run, taken Neow's reward, saved and returned to
the title and OS. Save, progress and language are under state/. This is not
yet final-resource gameplay or a full-run completion claim.

The first exact merchant split reduces visible moving pixels by 42.12%, but
increases the common resource from 70,659,306 to 78,354,908 bytes. Do not
publish that as a storage optimization. Bounded grouped encoding is being
measured and implemented before final packaging. Resource pixels, frame
counts, timings and all language tails are preserved. The final package,
Firebird cold/warm performance, final feature captures and main sync remain
open; private source artwork and generated resources remain unpublished.

---

# UI14 continuation — 2026-10-26

The user's current report supersedes UI13 acceptance for the reported layouts,
animations and performance. The unpublished game needs no legacy filename or
obsolete-build compatibility. The active implementation now keeps only
`nSpire2.tns` at installation root, data in `data/nSpire2.dat.tns` and
`data/nSpire2.<locale>.dat.tns`, and all mutable files under `state/`.
The developer console is compiled unconditionally, with no separate variant.

Frontend, combat and room work is running in parallel. The first stage includes
fixed HUD tooltip/ascension placement, smooth hand text, hand-format caching,
pile hover motion, compact compendium and defeat layouts, full-size score art
conversion, correct credits section, shared scrollbar hover motion, and deck
controls outside the return-button area. Shared bounded labels now scroll
horizontally/vertically at pixel speed rather than ellipsizing. Room selection,
merchant warmup and the remaining effects are being completed next.

Source-backed behavior: the pinned credits screen does not auto-close at its
end; the pinned reward screen marks completion and still waits for Proceed to
open the map. These are retained intentionally. No UI14 validation is claimed
at this checkpoint. Resource regeneration, fresh ARM build, ordinary-OS
Firebird gameplay/performance and final packaging remain required.

---

# Work state — 2026-10-26

## UI13 playable release — final runtime, resources and device acceptance complete

The user authorized all operations on `maxmixalex/slay-the-nspire-2`. Frontend,
combat/input and room work ran in parallel, with source checkpoints published
and independently fetched before long builds, conversions and device jobs.
The complete 28-item mapping is [UI13 LCD follow-up](ui13-lcd-followup-0.17.md);
precise artifact identities and closed gates are in
[the final acceptance manifest](validation-ui13-0.17.json).

The accepted production runtime is **`792904cb5e7f5fcbbab7a5320ee0d54d247591c7`**;
the complete converter/resources are **`e606108221fd6f8202f61b3e5ae1372970de0e27`**.
Later test/document stages preserve all `src/include/platform/CMake` bytes.
The 1,653,686-byte ARM has SHA-256
`77a41a60cdd99c2a4d2011e567a69afdb6b61851947e9692624f99612e28be4b`
and allocates **20,703,800 bytes**, below the unchanged 24 MiB ceiling.
The final ordinary-OS USB restore/readback journal
`cp17-release-restore-f7c4994628da` is closed PASS with unchanged source;
all four installed files match the delivered inputs exactly. The natural run
and progress also match their preserved pre-fixture readbacks byte for byte.

### Implemented scope

All 28 reported areas are corrected: menu/logo/Continue geometry, compact
confirmation controls, sixteen-language port credits, compendium arrows,
source language-list/scrollbar assets, shared HUD spacing/clock/ascension text,
physical-arrow navigation and all ten numeric hand shortcuts, padded map
legend, banner glyphs, adjacent Stars/Energy, health/block strokes, aspect-fit
hand aura, orb object lifecycle and capacity layout, original pile art and
positions, thought bubbles, reward alignment/Proceed hover, bounded event
options/previews, enchant markers/sidebar, merchant hand/cache performance,
modal hover and overlapping-card input, distinct transformation VFX, rest
outlines, SmartFormat history and higher-resolution boss canvases.

The earlier explicit presentation omissions are also implemented: Doom's
particle/body-disappearance family, complete Architect reading/trails/attacks,
Random's source water/noise/particles, Fake Merchant's original actors and
post-fight/no-rug layers, main-menu blur and label-local rainbow clocks.
The eight boss models retain **286 clips / 5,340 samples**. The complete new
auxiliary endpoint is 5,156; all **2,955 appended images** are decoded through
actual memory and range readers, and prior verified payloads remain exact.

### Closed verification

- **137/137 registered native tests**, zero failures, 42.68 seconds.
- **325 unique Python tests**: converter checkpoint discovery 271 plus
  supplemental 56, with two duplicates counted once.
- All **16 locales**, 38,686 checks / 53,072 rendered descriptions, zero
  unexpanded templates; complete glyph and kerning metadata verification.
- Final English/Chinese frontend, combat/HUD, label-clock and room gates;
  Fake Merchant's ten source animation sequences match their independently
  reconstructed distinct-image counts. Final event gates are 5,850 / 5,827.
- All eight Fake Merchant sustained native scenarios have zero reads/bytes/
  seeks in their second and third windows, within the same 11 MiB arena.
- A normal final-program Ironclad run starts from blank progress, takes Neow
  rewards, wins the first battle in four turns, collects gold/card, returns
  to the map, saves, Continues the same cleared room, saves again and exits
  to the ordinary OS. Journal `cp17-natural-final-4881221c7040` is PASS.
  Seed **FD81C60B**, Act 1 floor 2, HP **80/80**, gold **119**, deck **13**,
  revision **38**; production storage decoding and repeated canonical save
  round trips pass, with matching run/progress identity.
- Separate, explicitly controlled device cases show Vantom through its next
  turn, Architect dialogue/attacks/death/summary/title, Regent Fake Merchant
  animation/inventory/finished-room controls, and the post-fight no-rug room.
  All four display journals close PASS. They are not natural boss wins or a
  natural three-act completion; coarse device stills do not replace the
  complete native VFX pixel gates.

### Measured performance and storage

The same closed merchant room is warmed for 120 simulated seconds, then
measured over a matched additional 60-second cumulative-counter window.
The ordinary-OS A/B journals close PASS independently of the earlier failed
USB-link reuse attempt. **UI12: 9.8280 FPS / 101.7497 ms; final UI13:
29.9982 FPS / 33.3353 ms**, with 1,800 final frames. Requested resource bytes
fall from **20,364,684 to 12,700** (99.9376%); calls/seeks fall from
1,205/1,051 to 1/1. Final mean render is 14.9121 ms. This is Firebird emulated
calculator timing, not a physical-calculator claim. Device I/O is one read,
not zero; native warm-cache zero-I/O results are a separate scope.

The default four-file installation is **78,569,446 bytes (78.57 MB /
74.93 MiB)**, excluding saves, and remains above the rough 65–70 MB target.
It is 2,271,661 bytes larger than the earlier d5 UI13 candidate after retaining
higher-resolution bosses and completing the scenes. Lossless grouping and
1,013 exact aliases save **9,262,910 bytes** in appended image payload;
the inherited safe prune saves another 307,420 bytes in the default install.
No additional frames, source timing or source pixel quality were dropped to
make the storage result appear smaller.

### Release boundaries

The user-approved full Ancient-animation storage exception remains. Audio,
multiplayer and Timeline remain outside the agreed offline single-player
scope. RGB565, LCD rounding and bounded CPU shader/particle integration are
explicit platform adaptations, not blanket desktop-GPU equivalence. There is
no manual WASM/browser acceptance or physical hardware FPS claim.

Source-only checkpoints and evidence metadata are public; original PCK,
fonts, assemblies, derived resource packs and ROM/NAND stay out of GitHub.
The calculator ZIPs contain the final program/resources and installation,
upgrade/control instructions; controlled test saves are not included.
Failed, interrupted and superseded attempts retain their original identities
and are excluded from final passed gates. Historical entries below describe
only their named intermediate checkpoints; their old pending statements do
not override this completed release summary.


### Retained-scene completion checkpoint

Checkpoint `2b58ee31eb5ef6674ce95f02ad2eb9694a98a041` closes the
Random background/source particles, source main-menu blur and label-local
rainbow clocks, with initial complete Fake Merchant scene support. Its full
registered native run passes **137/137**, zero failures, 43.63 seconds, with
an unchanged source tree (`retained-registered-045d3e9eec57`). This native result
is separate from the next resource and device acceptance.

The follow-up stage adds the source Fake Merchant post-combat scene and
rug-removal clip, using existing canonical event inventory facts without a new
save field. It also gives that actual room the same bounded 10/1 MiB merchant
cache split, preserving 5/6 MiB for combat and combat-choice underlays. The
host-only Spine sampler accepts up to 512 ordered samples so the longest
320-frame relaxed loop remains one continuous physics solve. The earlier
256-sample conversion failed closed and is not counted as a completed pack.
The new resource endpoint is 5,156; the complete 5,011-image base remains an
explicit compatible endpoint. Final source-matched conversion, actual-resource
gates, ARM startup, sustained merchant comparison and packaging remain pending.

The intermediate d5 device run has now completed a natural first battle,
collected gold, a potion and a card reward, returned to the map and saved through
the ordinary pause menu. Its Continue title visibly clears the logo. Continued
play and USB readback are being checked under that same recorded d5 identity;
these captures are not final-runtime evidence.


### Final input and ordered-blend corrections

The `e5cdd7f28098a12bfd01d257584d157d12035498` full native run closes with
136/137 passing and one real input failure: a finished Fake Merchant accepted
an ordinary-event ReadEvent overlay. Both the keyboard dispatch and direct
UI-action entry are being corrected; the retained failing assertion is extended
with canonical Proceed and unfinished-shop interaction checks. This result is
not reported as 137 passing. The same checkpoint's Python discovery passes 265
tests and the supplemental converter suite passes 56 (two overlap, **319 unique**).
A first supplemental invocation omitted the tests module search path and failed;
the explicit-PYTHONPATH rerun is the accepted result.

Complete source sampling also rejects Regent frame 47 because its additive eye
crosses a partially transparent body edge. The new representation preserves the
original Normal/Add/Normal slot order in one bounded sparse frame atlas, rather
than treating emission as ordinary alpha or doubling cache entries. The seven
already completed source sequences retain independent receipts and exact
payload hashes; an explicit validated resume prefix will preserve those frames.
The complete new scene pack and its actual-resource gates remain pending.

The same integration review catches the original 256×256 Light/Dot textures
against Content's unchanged 320×240 decoder bound. Their 65,536 texels are now
stored in original flat order inside 320×205 images, with 64 transparent padding
texels and a logical 256×256 sampler view. There is no cropping/resampling and
no decoder-bound relaxation. A short original-image decoder check passes all
texels through both memory and range-reader LZ4 paths (786,725 checks); that
pre-check is not substituted for complete resource-pack acceptance. The final
pack gate will actually decode all 2,955 appended auxiliary images.

The `e5cdd7f` ARM build succeeds with 1,653,701 bytes and a 20,704,060-byte
allocation. It remains an intermediate artifact pending the input/blend fix.
The d5 natural playthrough's run/progress USB downloads validate with the
production codec: Ironclad, seed 27EE432E, floor 2, Reward phase, HP 68/80,
111 gold, 12 cards and matching revision 30. Its controller journal is failed
because a later optional settings download timed out; the two completed save
transfers and observed gameplay are retained with that explicit limitation.
A separate mismatched snapshot/backing-flash attempt is excluded completely.
A clean paired OS preparation journal passes, but supplies no final gameplay or
performance evidence. The final runtime/resources will be installed and verified
in a separate, matched device run before publishing main.


### Runtime accepted; final converter visibility correction

Runtime checkpoint `792904cb5e7f5fcbbab7a5320ee0d54d247591c7` now passes the
complete **137/137** registered native suite, zero failures, 42.68 seconds,
with `source_changed=false` (`cp17-registered-3b0638b5ead5`). Its Python runs
pass 269 discovered plus 56 supplemental tests (two repeated, **323 unique**).
The final runtime ARM is 1,653,686 bytes, SHA-256
`77a41a60cdd99c2a4d2011e567a69afdb6b61851947e9692624f99612e28be4b`,
with a 20,703,800-byte allocation. Its ordinary-OS USB upload/readback matches;
full new-pack gameplay and timing still await the resource build.

The complete Regent converter run exposes one overly rigid new build-time
assertion: 258 source frames contain 32 visible draws; 62 contain an additional
Normal `minion sweat` attachment, beginning at frame 234 (9.75 seconds). Every
one of the 320 samples still contains exactly the reviewed `eye add` Add layer;
the eye union remains [92,77,100,83]. The converter now validates source slot
identity and blend materials while retaining each frame's actual visible order,
instead of asserting a constant total length. Five source-free tests, full
320-frame metadata/crop audit and eight key-frame atlas checks pass before this
checkpoint. The failed earlier conversion is retained separately. This follow-up
changes only the converter and its tests/documentation: runtime `src`, `include`
and `platform` remain byte-identical, so the accepted runtime/ARM is preserved.
Final original-resource conversion, complete actual-image decoding and matched
device acceptance remain pending and are not counted as passed here.


### Complete private resources; final test oracles

Converter checkpoint `e606108221fd6f8202f61b3e5ae1372970de0e27` now closes the
complete 5,156-entry auxiliary catalog and all sixteen locale packs with
unchanged source (`61ffb1e8372c`). All 2,955 appended source images, including
1,800 Fake Merchant dynamic frames and the no-rug variant, are retained. The
original 1,138-image UI13 prefix and seven already verified Fake Merchant
sequences (1,199 images) match their earlier payload hashes exactly.
Lossless grouping and 1,013 exact aliases reduce 12,254,308 padded source
bytes to 2,991,398 stored bytes, saving 9,262,910 bytes. The inherited unreachable
record pruning additionally saves 307,420 bytes in the default split install.

The actual four-file install is **78,569,446 bytes (78.57 MB / 74.93 MiB)**:
launcher 1,653,686; common 70,659,306; English 1,140,675; Simplified Chinese
5,115,779. This is 2,271,661 bytes larger than the d5 UI13 candidate and remains
above the rough 65–70 MB target. No frame, source timing or source pixel quality
was dropped to hide that difference. The complete no-rug extension adds only
26,246 bytes to the common file after exact compression.

Actual-resource validation exposes two outdated test assumptions, both now
replaced by stronger source-backed checks without a production change. The
foreground animation has exactly 11 distinct LCD images across 144 original
samples; all ten sequences now require exact independently reconstructed
counts [133,11,80,80,259,89,288,290,144,133], not an arbitrary density threshold.
The whole glyph-directory test must distinguish the 7,124 kerning metadata
records from drawable glyphs; it now checks every pair through the public
kerning API at every present font size against independent directory arithmetic,
while retaining full raster-byte comparison for all ordinary glyphs.

The actual Fake Merchant warm-cache gate passes 14,544 App.render frames over
eight scenarios (five characters, focus, and both post-fight rug outcomes).
Every second and third 606-frame window performs zero source reads/bytes/seeks;
peak usage is 553 entries / 1,730 pages, within the unchanged 1,023-entry and
10 MiB room tier of the existing 11 MiB arena. This is host correctness/cache
evidence, not calculator FPS. Final actual-resource gates and ordinary-OS
device timing/gameplay are still being closed before main and delivery.


## UI12 playable follow-up — acceptance completed

Delivered runtime: **`9398633d8fd715c008560cc038f61d8f85c98466`**. Validation source:
**`1848338c68d4f10e9fa46d414729087fbd0b1224`**; the only difference is the
independent Architect regression source. All 504 production `src`/`include`
files match. Verified source is synchronized to `main`; no original assets,
fonts, ROM/NAND, private assemblies or generated game packs enter GitHub.

The prior recovered UI12 baseline below is retained with its actual receipts.
This follow-up additionally stops invisible glyph-span raster work on empty
merchant preparation canvases and restores the original Architect/player actor
layer. Its VisualOnly scene uses existing idle/attack/hurt clips, source first
line waits, 400ms speech reveal, authored dialogue/next labels and the original
left-side death animation at the terminal handoff. It does not add Osty from
old combat state. Long English/Chinese dialogue uses the existing reader.

Actual original-resource validation found two real omissions: the dialogue was
queried from `events` instead of `ancients`, and a partial-history fallback
covered the actors before the terminal quote stage. Both are fixed; the original
pixel and nonempty-text assertions now pass. Test fixture corrections preserve
completed-run exclusion from ordinary Continue and the independent Silent
actor/UI clocks. These test corrections are not misreported as game features.

### Final acceptance and deliverables

- **135/135** registered native tests pass, **0 failures**, 64.05s; **310 Python
  tests** and tool compilation pass. Final actual-resource checks pass for UI12
  combat feedback (12,594), merchant preparation (223), and the original
  English/Japanese camera/sleep/lifecycle gate (2,122,238).
- The independent actor gate passes all **20** five-character × Normal/Fast ×
  English/Chinese combinations, 7,279 assertions per language. It checks original
  sprite ink/anchors, repeated hits, actual text and scrolling, the two-actor
  terminal composition, no residual Osty, death and canonical-state invariance.
- Final ARM is **1,600,738 bytes**, SHA-256
  `5b62c0797bb10d49e9d7d36bc478dff808dc1cdf83cc55633433d2bc3d6a7275`.
  It requests **20,363,472 bytes**, below the unchanged 24 MiB ceiling. Common,
  English, Chinese and launcher install at **69,514,501 bytes**. No new resource
  payload, App/State field or persistent cache was added by the actor follow-up.
- Final Firebird journal **`ui12-architect-device-final-55becdaab6a4`** passes
  with unchanged source. Ordinary revision69 Continue/SaveQuit/normal OS exit
  preserves the canonical run. A separate native-generated original-entry
  Ironclad fixture shows dialogue, Proceed, actor actions, original-position
  death, summary, Main Menu and normal OS exit. It is not a natural three-act
  playthrough or a new performance benchmark.
- The ordinary save and progress were then restored through USB and read back
  **byte-for-byte equal** to the original 7,708-byte/1,191-byte files. Production
  load/validate/roundtrip passes, resource failures are zero, and the controller
  has stopped normally.
- The paired calculator ZIP and separate fourteen-language archive are verified.
  Evidence contains **82 files / 65 lossless 320×240 PNGs / 10 source groups**;
  earlier screenshots remain labelled with their actual runtime.

Exact closed journals, file sizes, SHA-256 identities and test scopes are in
`validation-ui12-0.17.json`; the user-feedback matrix is
`ui12-lcd-followup-0.17.md`. The first complete native attempt could not launch
two unchanged host ELF files missing execute bits. Its failure remains recorded;
a complete same-session permission-corrected rerun passed all 135. Intermediate
actor failures and superseded candidate builds remain separate from final passes.

The existing matched Shop result still belongs to `1826969`: mean room interval
improved 23.23%, while the largest cold interval rose to 2703.094 ms. The new empty
clip fast path has correctness evidence, not a newly measured speed percentage.
Camera/noise/LCD mappings and the specific remaining Doom/Architect visual
families are explicitly separated in `screen-effects-ui12-0.17.md`; no blanket
GPU equality or 95% score is claimed. The user's full Ancient animation storage
exception and audio/multiplayer/Timeline exclusions remain.

This acceptance cycle has no open build, packaging, source-persistence or
emulator task. Install the four matched default files; keep ordinary saves,
progress and settings. Any later implementation work starts from this verified
runtime and preserves its paired UI40 resources.


## UI12 recovered 169 baseline — historical completed acceptance

Runtime checkpoint: **`169cea998135a47e5f8580b15df798244e0e7e6a`**. The final documentation/source
checkpoint is synchronized from `work/0.17-world-parity` to **`main`** after
verification. Every coherent source stage was pushed and independently fetched
before its long validation; public source excludes private game data, fonts,
calculator images and generated packs. The earlier approval block described in
historical notes below was resolved by the user's explicit resumed instruction.

The interrupted source at `af95e293` was recovered intact, preserved remotely at
`fe11b6d756012b889eb48d5911df408caa126da1`, and advanced through the integrated
feedback/console/input/renderer fixes. `ui12-lcd-followup-0.17.md` is the full
feedback matrix; `validation-ui12-0.17.json` binds the deliverable, executed gates,
ordinary flows, measured limits and excluded attempts to exact checkpoints.

The calculator deliverable has a **1,595,942-byte launcher** and requests
**20,356,556 bytes** under the 24 MiB ceiling. Common + ENG + ZHS +
launcher install at **69,509,705 bytes**. All sixteen language resources
were independently verified; only ENG/ZHS are in the default installation.
The repaired Necrobinder adds 98,098 shared bytes, the sampled studio-logo
sequence 165,511, and the swarm/sleep sprites 2,284. The accepted UI11 animation
precision remains; this stage does not remove animation frames to fit storage.

### Verified behavior and complete suites

- The complete registered native suite passes **134/134**
  against the frozen release runtime. Python discovery plus the six additional
  converter/packaging modules passes **310 tests**. Compilation of tools passes.
- Actual final-resource gates cover all sixteen locale descriptions/fonts,
  startup/SkipIntroLogo, merchant preparation and occlusion, English/Chinese
  combat, and the dedicated UI12 card/banner/cache/early-Loot contracts.
- Real source-driven screen tests cover rumble/trauma/punch/drunk schedules,
  normal Smith, sleep/wake tails and sprite bounds, Doom's actual execution
  boundaries, and Architect Normal/Fast/Off/Continue/terminal lifecycles. These
  are presentation contracts and do not consume canonical RNG or alter saves.
- Ordinary Firebird `f4a7cd59` completed an Ancient-to-battle/reward/map/save/
  Continue flow. `44ebf0e` naturally continued to floor 6, confirmed a single
  Battle Start then Player Turn, and drew Rage normally. Its selected card
  revealed the complete `gain 3 Block.` tail. The ordinary saved state was
  floor 6, turn 5, HP 59/85, gold 280, deck 12, revision 68; it validated and
  roundtripped exactly through the production decoder.
- Final `169cea99` ordinary startup/Continue, actual digit-1 / Enter Rage play,
  SaveQuit and return to the OS pass in `78793058034c`; USB production readback
  validates and roundtrips revision 69 exactly. Separate Flag-console original
  `fight`/Tab fixtures verify moving phobia swarm and sleeping Z, then restore
  the ordinary save/progress with identical USB readback (`ea63bb4cdfce`).
  Earlier captures are never relabelled as the final binary.

### Performance and real limits

The closed matched Shop fixture compared UI11 N with `1826969c` under the same
canonical input and protocol. Mean room interval fell 106.880 → 82.053 ms
(**-23.23%**), while the maximum increased 2,387.054 → 2,703.094 ms
(**+13.24%**). Canonical readback stayed identical and resource failures were
zero. Inventory preparation provides visible feedback and bounded image/text
steps; three separate observed cold 180 ms Esc taps cancelled. Cold I/O latency
has not been eliminated. These are Firebird SP804 timings, not physical-device
FPS. `44ebf0e`'s ordinary three-enemy combat sample averaged 38.522 ms over
1,438 frames, with a 487.305 ms maximum; it is not a matched speed comparison.

Failed/unfinished earlier attempts remain excluded: the old short-tap merchant
smoke, the ambiguous black-screen/blocked-USB exit attempt, superseded native
builds, and an optional exhaustive memory/range resource traversal. The latter
thrashed the shared tiled cache and was interrupted; its passing components have
separate exit receipts. A new Doom gate exposed a real death-screen target reset;
the release preserves the original combat scene for the source 300 ms punch and
verifies visible frames before the real terminal screen. No diagnostic-only
probe is counted as completion of a feature.

Audio, multiplayer and Timeline retain the agreed exclusions. Full Ancient
actors retain the user-authorized storage exception. Dense GPU effects and
exact desktop rasterization remain CPU/LCD mappings. Architect camera timing is
implemented; this does not claim every authored player-attack/FireBurst particle.
No manual WASM/browser validation or physical-device speed claim is made.

## UI12 active feedback correction — historical pre-recovery notes

The user explicitly authorized pushing UI11, then requested the next complete
calculator usability pass. Remote main/work were read at UI10 `9dc0c112`.
The atomic fast-forward of UI11 `40def2b0` was rejected by automatic approval
review despite that authorization: it requested the exact source payload and
destination be bound in user-authored text. No alternate transport was used.
All UI12 checkpoints remain local until that block is resolved.

Three isolated worktrees implement frontend/credits/collections, combat/cards/
rewards, and rooms/map/character selection. Root owns shared fractional glyph
layout, private resource integration, calculator physical inputs, the top bar
and final Firebird acceptance. The console is now assigned to the frontend
agent after its coherent UI stage. This section records work in progress;
no UI12 emulator or completion claim is made yet.

The root integration is now frozen at J `1074b720cf281629b52003794739e5c71d1daa27`.
Its ARM launcher builds successfully at 1,570,350 bytes and requests 20,126,948
bytes under the 24 MiB build ceiling. H's unchanged final UI40 resources plus J
install at 69,318,101 bytes with English and Simplified Chinese. All sixteen
locale conversions completed; 15 H locale loading/formatting routes passed,
then the Turkish route exposed an invisible U+00AD mistaken for a visible glyph.
J handles that format control and fixes two final cross-family kerning cases;
matching native gates are running. Full NSL2 English and Chinese reconstruction
already passes with I's test-only include fix and unchanged H loader library.

Three independent ordinary Firebird sessions have executed H: typed Flag-key
console, original settings controls, ordinary new-run/character transition,
Ancient choices, save/title/language change/continue, and a normal first battle.
Their acceptance remains scoped to their actual checkpoints. Real interaction
found an aimed-card clipping defect and an Ancient first-click focus defect;
J implements both. A further ordinary OfferReward alignment omission was found
in the combat session and is being fixed before final delivery. Native tests
are correctness evidence, never calculator speed measurements.

Root implements fractional advances, true source pair kerning/negative bearings,
official TI key glyphs, real pad-edge navigation, 1–0 hand shortcuts, exact-old-
default-only migration, expanded potion pages and top-bar tooltip/arrival anchors.
The frontend implements original material buttons/tabs/scrollbars, credits and
collections, and the actual typed command console. World implements source random
selection/transitions, reconstructed Necrobinder head/fire, in-place reading and
map/room layout. Combat implements target-only values, actor/HUD/card layout,
thought bubbles, early loot, actual Smith animation and merchant card caching.

The private character repair adds only 98,098 bytes by replacing its old tile
references and reclaiming dead tiles, instead of appending 8.42 MB of sparse
frames. Full images/clip clocks remain preserved at the accepted UI11 precision.
The final source-only report and package are still being assembled. No WASM or
browser validation is run. Runtime source stages have local archives/journals;
none is described as pushed while automatic approval blocks the remote update.

Host ENOSPC interrupted two native journal closures and truncated a frontend
emulator snapshot. The failed/unfinished receipts are retained, not counted as
passes. Removing only old reproducible UI9/UI10 CMake objects/libraries/executables
restored space; original inputs, source, reports and emulator evidence were kept.
The frontend restarted from a clean independent NAND and again installed Ndless
normally before H. The earlier truncated snapshot/installer errors did not execute
H and are not attributed to the game.

### UI12 world final acceptance and complete registered suites

World production stages are merged through the L source-order correction.
Ordinary Firebird H exercised Random reveal/original-mask departure, Neow,
compact map legend/drawing tips, UI language change plus saved-run Continue,
Chinese inline event keyboard/thumb scrolling and the rest room. J executed
the actual Smith selection/confirmation, full enlargement/flash/deck flight,
and the repaired first pointer Ancient choice after reading focus. L verified
the source portrait order and a real mouse click on Random. Its session
`ui12-world-firebird-l-preview-95adc07760f1` passed, returned perf through USB,
flushed its independent NAND and quit normally; `source_changed=false`.

The M-matched native core library now passes all eight complete world UI /
lifecycle suites plus the registered focused reader gate (403,476 checks).
The final event, room, map, act-intro counts are 5,161 / 8,809 / 98,037 /
47,282; no complete suite is replaced by a reader-only invocation. Test-only
checkpoints `c083b79`, `fdab3db`, `e16933b`, `5981dd3` update changed LCD
geometry, explicit legend controls, the actual 1,600ms departure prelude,
settled post-heal actor samples and deliberate pointer hover ownership.
Canonical saves, RNG assertions, four exact Defect composite colors and
uint32 map-clock wrap remain checked. All final journals passed with unchanged
source fingerprints; the earlier failures remain separately recorded.

`docs/world-ui12-0.17.md` records checkpoints, complete suite receipts, ordinary
emulator observations, measured limits and the optional unadded Ancient
animation. No WASM/browser test or physical-device performance claim is made.
The remote approval rejection remains unchanged; these are local persisted
checkpoints, not pushed source.

## UI11 final N — readable LCD, storage and ordinary ARM execution

UI11 continues from accepted UI10 `9dc0c11291b828e4e17a347e7ef7a27c7b371e60`.
The user-authorized 320 × 240 adaptation uses roughly 1.5× main text/control
sizes, reflow, scrollable descriptions, thinner outlines and corrected Latin
spacing. Exact UI10 screenshot equality is no longer the acceptance criterion.
The runtime removes patch notes and feedback, preserves original credits and
appends the zyf722/Codex Nspire port credit. Focused cards, inspection, events,
Ancients, shops, collections and settings now use the readable LCD layouts.
The terminal summary also has a readable scrolling article and keyboard focus
reveal. Ordinary K/L gameplay and final N title/terminal execution now pass in
Firebird; their distinct scopes are recorded below and in
`ui11-lcd-adaptation-0.17.md`.

### Implemented retained differences

- Combat now has actual hand gold/red/cyan highlight conditions, selection
  flashes, exhaust dissolve/embers, seven-node power-card travel, recorded
  trails, aggregate shuffle and generated-card entry effects. All five orb
  kinds report real passive/evoke events, including repeated evokes and extra
  Gold-Plated Cables activations, without resampling targets or changing RNG.
- Heal smoke/desaturation, rest embers, merchant candles/light nodes, chest
  coins/rarity glows/empty smoke, card acquisition trails and separate potion/
  relic arrival flashes are linked to real actions; restored rooms do not
  replay completed effects.
- Bestiary previews retain detached phase and liveness facts for the tested
  regeneration, charge, burrow, stun, hatch, growth, summon and multi-actor
  down/revive contexts. Character-select particle contexts and duplicate-actor
  cosmetic scale/hue are implemented with bounded independent display state.

`combat-secondary-ui11-0.17.md`, `room-vfx-ui11-0.17.md` and the current
contract rows in `ui8-source-ui-audit-0.17.json` identify the implementation and
limits. Dense GPU turbulence, continuous particle paths, HDR bloom, smoke/noise
materials and exact desktop glyph rasterization remain explicit CPU/LCD
approximations. Closing their real triggers does not establish pixel identity
or a measured 95% restoration score.

### Resource, language and memory results

The final private common art file is **61,186,168 bytes**. English is
**1,049,523 bytes** and Simplified Chinese **5,034,485 bytes**: the default
resource files total **67,270,176 bytes** before the launcher. Final N's
**1,506,285-byte** launcher gives a four-file installation of **68,776,461
bytes** (68.78 MB decimal), **24.9206% smaller** than UI10's 91,604,965 bytes.
This is storage reduction, not a speedup percentage. The common file alone
is **23,648,624 bytes** smaller than UI10. Lossless retiling is followed by authorized bounded mixed
RGB555/454/444 and alpha6 animation precision; all frames, timings, dimensions
and geometry remain intact. Static UI/cards/icons/text remain exact, and the
five character-select static planes use the same map as their animated layers
to avoid seams. Color precision is explicitly lossy, not a lossless-compression
claim. See `storage-memory-ui11-0.17.md`.

All **16 original locale editions** have actual source text, original font
chains and verified glyph coverage. Each optional edition shares the same
common file; installing all editions is not the default 65–70 MB target.
Original missing translations use English fallback (Thai: 1,966 bound keys;
other non-English editions: 91–129); calculator-only instructions outside ZHS
and early boot text retain the documented fallback. See
`localization-ui11-0.17.md` for the installation table and formatter limits.

The build allocation ceiling is **24 MiB**. The D candidate's **23,874,780-byte**
contiguous Zehn request failed in ordinary Ndless with `Memory allocation
failed`; being under that ceiling was not sufficient. F uses a real **5 MiB
image cache / 480 keys** and **6 MiB tile-group cache / 48 pages**, producing a
**19,755,048-byte** linked allocation that starts successfully in ordinary
Firebird. Subsequent K/L ordinary gameplay and N's final launch/terminal/exit
also pass. Final N's measured linked allocation is **19,806,488 bytes**; the
5 MiB image and 6 MiB tile caches are real runtime buffers, while 24 MiB is
the build ceiling rather than a claim that the OS accepted a 24 MiB block.

### Executed focused correctness

- I (`4e4ce0692a01132de613c01e3cb6a8a56f0d001c`): all 16 final locale editions
  pass **38,644 checks / 53,072 model-event descriptions**, with zero failures
  or unexpanded templates. Actual room effects pass **105 checks**. Independent
  card-highlight scalar/clipping checks pass **12,592,419 comparisons**.
- J (`f16573e1b9c6da390cd9944bd447acb4dc0482d3`): actual final common/English
  combat secondary checks pass **325,107**. J's test and renderer were compiled
  against the unchanged I core library; this is explicit incremental evidence.
- Bestiary phase fixture checkpoint
  `49340f020bfbe96c49cc6de71aa122e1556c269c` passes **139 checks** with unchanged
  H core and canonical-save preservation. F tile/cache checks pass
  **16,396,600** and **5,966** respectively. Exact receipts are in the module
  reports and source audit; these are correctness, not speed measurements.
- Final Game Over fixture checkpoint
  `02f0f4b266dc1ac9578a43e02521a07d95d647de` passes **176 checks**, compiling
  the actual L UI against unchanged I core. Original score/badge/history and
  release timing, long-summary scroll bounds, focus wrap/visible auto-reveal
  and canonical SaveData/RNG preservation pass. Build `e4bc30c76261` and run
  `d246a073ed6e` both record `source_changed=false`. This asset-independent
  gate is not a font screenshot or calculator speed measurement; the two
  earlier fixture-only failures remain explained in the source audit.

### Ordinary ARM/Firebird acceptance and measured limits

- **K ordinary continuation** (`90d981c4e42e5f67f7a72b52f37fa1ac45410a71`):
  continued the F floor-4 combat, inspected/played cards, completed four turns,
  won, claimed gold and Setup Strike, opened the retained-room map, saved and
  exited normally. Actual NSZ1 readback passes production decoding, L native
  load/validate and exact canonical resave. HP 73→64, gold 109→125, deck 11→12
  and revision 30→48; Setup Strike is the only added card. The NSH9 readback
  matches floor 4 and four battle turns. Final canonical phase is Reward; the
  open map does not claim travel to another floor. Journal `8a5389fd74a2` and
  readback `dfa5befd59c0` pass with `source_changed=false`.
- **L ordinary continuation** (`7722cad8adb6626081b0100e5017d3de095904c5`):
  traveled from that saved run to the floor-5 Tablet of Truth, used the full
  reader and healed 64→80, then entered floor-6 three-slime combat. Actual
  middle-enemy targeting changed its HP 27→9; ending the turn left player HP 73.
  Voluntary Give Up opened the readable summary, scrolling, Main Menu and
  normal Quit. Journal `93bd401af87c` passes with `source_changed=false`.
- **L Chinese room/Bestiary**: ordinary cold OS/Ndless launch used constructed
  canonical Neow and compendium-progress fixtures. Actual option selection
  changed HP/max HP 80→91, finished the event and advanced revision 4→6; native
  readback confirms it. Five-action paging exposes and plays the sixth Death
  action. Journals `d6f0eebb90d7` and `7012def96add` pass. These fixtures verify
  Chinese interaction and presentation, not ordinary-playthrough reachability.
- **Final N** (`e95893c9effaa9b5f7fd09158cca546d7b65ce3b`) changes only two
  initial-defeat-quote rendering lines from L: natural paragraph height and
  source `quote_y + 8` placement. Ordinary boot, corrected quote, summary,
  Main Menu, normal Quit and exported performance pass journal `383291056740`
  with `source_changed=false`. The release owner directly reviewed the quote
  below the banner without corpse overlap. N's 206 title / 338 room / 67
  overlay samples are a focused terminal smoke, **not an N combat benchmark**.

Measured mean frame intervals and cold outliers remain visible:

| Actual session / scene | Frames | Mean interval ms | Mean render ms | Maximum interval ms | Over 100 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| K ordinary combat | 1,324 | 35.800 | 27.880 | 382.874 | 14 |
| L ordinary three-enemy combat | 430 | 41.751 | 34.526 | 752.655 | 16 |
| K map | 46 | 80.050 | 79.165 | 452.087 | 3 |
| L map | 81 | 72.307 | 71.338 | 393.829 | 2 |
| L Chinese collection fixture | 784 | 42.713 | 20.161 | 1,807.068 | 18 |
| L Chinese room fixture | 102 | 54.170 | 46.605 | 1,393.280 | 3 |

All four reports have **zero resource-read failures**. Preparation is separate:
K's 17 preparation frames average 139.442 ms, maximum 1,205.963 ms; L's 45 average
91.601 ms, maximum 987.152 ms. L also records a 1,214.386 ms mixed-room interval.
Map/overlay work and cold-entry stalls remain measurable limits; these values
are not removed from the report or described as uniformly smooth performance.

`performance-ui11-0.17.json` binds each result to its source checkpoint,
closed journal, raw report hash and exact scope. It records SP804 emulated
calculator time, not host wall time or a physical-device measurement. K/L
use different encounters and cache histories; there is no matched UI10/F
baseline, speedup percentage, median or percentile claim. Startup counts only
selected resource processing/App initialization, with first render separate.
No UI11 WASM/browser build or test was run. Final private package/readback and
delivery receipts are maintained by the release owner.

All UI11 checkpoints cited above are local frozen stages with logged results.
Automatic approval review rejected remote push; this work state does **not**
claim UI11 was synchronized remotely. Private game assets stay outside public
Git. The UI10 K delivery and browser/performance receipts below are historical
and must not be reused as acceptance for changed UI11 code.

## Final UI10 K runtime and delivery acceptance

The delivered runtime and regenerated resources are frozen at remotely verified
`61c5a93acbd9730ab345a4add8293bda744820f0`, tree
`3e155a32748e3917ba8f369de4da7cd8678389fe`. The final closeout changes only
documentation. Main-menu/Continue/profile/collection/statistics/history and
General/Input/run-setup work from the earlier UI10 stages remains implemented;
the final production change skips inactive long-press work while preserving
input ownership, residual decay, the source threshold and the source bar.

Matching K acceptance is complete:

- Native `c6dc1b85d710`: **121/121 pass**, 104.98 seconds of CTest execution.
- Python `2c681130c3ef`: **252 + 54 pass**; compileall also passes.
- ARM `4413aa9add78`: launcher and linked division/LZMA2/tile/RGR instruction
  and ABI suites pass. WASM `cf00a6665385` also passes.
- Actual resources: **96/96 bilingual targets pass** (root 17, front 15 and
  combat 16 per language). Both complete split streams pass separately.
- Complete image comparison covers **1,228 PPM captures**: root 536, front
  214 and combat 478. Root uses I except the 38 corrected J FTUE captures;
  front is exactly I; combat keeps only J's three corrected Settings hover
  captures and one added Long Press capture per language. All 116 card/HUD
  captures match J, and both hold-bar states per language match I. Two combat
  captures per locale are emitted beside their target directory and are
  included in this total, although the generic per-target receipt lists omit
  those four files. No unexplained runtime image difference remains.
- Ordinary offline browser `e70b5849ae91`: **21 flows / 47 captures**, no
  page errors or external requests. The final HTML SHA matches the closed
  artifact receipt both before and after execution.

Every completed K source-guarded journal reports `source_changed=false`.
These tests cover port behavior and source-derived contracts; neither their
count nor equality between port versions establishes a 95% original-game
restoration score. See `ui10-front-source-review-0.17.md`,
`settings-run-setup-ui10-0.17.md`, and `ui8-source-ui-audit-0.17.json`.

Resource conversion `64c1f36fbb05` reproduces both logical NSP2/39 packs and
all three physical files exactly as H and I. The installed resources total
**90,114,227 bytes**. K's launcher is **1,490,738 bytes** and the complete
four-file installation is **91,604,965 bytes**. Actual prefix/image/BSS
allocation is **16,256,400 bytes**, leaving **520,816 bytes** below the
unchanged 16 MiB ceiling. Relative to I, the launcher is 102 bytes smaller
while allocation is 88 bytes larger; these are separate measurements.

K ordinary installation `1f6bbbeca0eb` fully reads back the new launcher and
associates the identical resources with H's complete USB readbacks. Cold
execution `9808e0b2a6b0` opens the persisted Long Press control, switches it
off and on, continues the actual floor-2 reward room, saves, and exits to the
OS normally. All seven LCD captures are reviewed and all five profile files
are read back. Matched K validation `a2b145331ee7` confirms the original
230,540-byte canonical run and backup remain exact, NSH9 records the actual
save/revision 24, and the 108-byte NSPF5 retains flags 644 and both key banks.
The original NAND is unchanged; the emulator is stopped. H's four capacity
cases and I's 9.8-second physical hold remain labeled with their actual
execution checkpoints rather than being counted again as K tests.

Final performance association `9a986cce7aaf` compares all 25 official core
objects, their archive order, and the complete 7,756,600-byte benchmark ELF
exactly with the measured private candidate. The four balanced same-battle
samples therefore apply to the final native implementation: **4.22% lower
median CPU time**, with all 14,400 paired frames and canonical bytes equal
and no warm I/O. The 9.53% mean is influenced by one high I sample and is
not the preferred general description. Earlier F/I regressions, cold-start
costs and all raw samples remain in `performance-ui10-0.17.md`. No physical
calculator FPS or universal speedup is claimed.

Packaging `b42a15d72cae` and independent closed-artifact readback
`868e0847e8d0` pass all 13 ZIP members. The final private deliverables are:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `Slay-the-Nspire-2-0.17-ui10-bilingual-calculator.zip` | 90,276,254 | `6ffc75158b29df152502e9b85cac4973f88b9349f70d24d9ea6ecbc3d090c460` |
| `Slay-the-Nspire-2-0.17-ui10-bilingual-offline.html` | 129,282,581 | `d03be509015e5bb8ad515fd0933ca07236804e57a61e0a265be85bbdf2a576a1` |

Both manifests identify K for runtime and resources; formats remain resource
39, canonical save 17, history 9, and settings 5. Original assets and embedded
deliverables remain outside public Git. The K GitHub Actions run
`40178587658` was still queued at the pre-closeout readback; the I and J runs
were cancelled and H `40174150511` passed. Local matching acceptance above
must not be presented as a completed remote K Actions run.

### Historical UI10 remaining scope, superseded by UI11 above

This records the limits at UI10 delivery. UI11's current implementation,
authorized LCD/color adaptations and actual remaining gates are recorded at
the top of this file and in the current source-audit contracts.

The source inventory covers 3,538 C# paths, 714 nonempty node-namespace files
(plus two excluded empty files), 1,022 scenes and 72 flow contracts. Inventory
and targeted body review do not mean every original method has been audited
or reproduced. The 95% goal remains unproven. Prioritize the recorded
phase-specific/multi-creature Bestiary and secondary VFX contexts, original
glyph ink/kerning and the measured Continue-date advance difference, then
the remaining original startup/settings platform differences. Keep the
explicit LCD, bounded-history, UTC and offline-service adaptations visible.
Audio, multiplayer and Timeline remain excluded by the established scope.

The 30–40 MB storage goal is also unmet. Preserve exact pixels and animation
data while investigating further bounded compression; do not trade original
UI fidelity for an unmeasured size target. The measured bitmap/cache/table
and foreground-workspace improvements are retained, with counterexamples.

## UI10k adopts the measured idle-off optimization

The final bounded experiment changes only two production branches, keeping
I's App fields, layout, global address/size and render address/size unchanged.
The long-press timer advances when the preference is enabled or an owner,
residual timer or unfinished fade still needs service. The progress-bar
visual is computed only when that setting is enabled. The elapsed-run clock,
500ms threshold, cancellation, source colors,250ms fade, canonical formats,
resources and resident pools remain unchanged.

Four balanced I/off-guard pairs each render3,600 identical frames; all28,800
frames, canonical bytes and zero warm I/O checks pass. Every pair improves,
with CPU median1446.0925->1385.028ms (-4.22%). Mean1529.02075->1383.31975ms
(-9.53%) includes a visibly high I sample, which remains in the receipt.
This is a measured improvement in one native battle workload, not an ARM
FPS or universal speed claim. See `performance-ui10-0.17.md` for every sample
and the earlier contradictory/negative windows.

The two production files are byte-identical to the measured private variant.
The existing hold regression adds App-level disabled-but-residual owner,
timer and fade assertions after its original captures. Its short1731 checks
pass with the unchanged I ABI. K requires its own verified remote checkpoint,
matching full native/ARM/WASM/Python and actual-resource/browser gates, final
closed artifacts and ordinary-OS launcher/cold-persistence check. Source
association with the measured binary must be verified before describing K's
performance as accepted. J's fixture corrections and all earlier evidence
remain preserved; source/test counts still do not establish95% fidelity.

## UI10j verification follow-up after the accepted I runtime

I runtime/resource source is remotely verified at
`76c022a1e2db17c49bf3ab8edb335dbb98618582`, tree
`546128015f0087d2327b28f3514019ccdc53d44d`. Matching native121/121 passes in
101.74 seconds, Python252+54 passes, ARM linked division/LZMA/tile/RGR and
WASM pass, and all96 bilingual actual-resource targets plus both complete
NSL2 stream comparisons pass. The ordinary offline browser passes21 flows
and47 captures without errors or external requests. Its real long-press
setting, reload, short taps, pointer cancellation, physical E repeat and HTML
button lifetimes produce canonical turns1,2,3 while preserving both other
profile saves. These layers remain distinct from an original-fidelity score.

The launcher is1,490,840 bytes; actual Zehn-prefix+image+BSS allocation is
16,256,312 bytes, leaving520,904 bytes below the unchanged16MiB ceiling.
All I-required glyphs already exist in H, so all three final resource files
are byte-identical to H; their total remains90,114,227 bytes. The full four-
file installation is91,605,067 bytes. Actual USB readback verifies the I
launcher and reuses the proven-identical H resource readbacks. The largest
specified clean history-write case again writes and reads all16 files,
2,701,448 bytes, successfully with I's actual108-byte NSPF5 record.

The ordinary cloned OS enables Long Press through the real General row and
persists bit7 with the other settings and both key tables unchanged. From the
preserved floor2 reward room it enters the next real battle. A250ms short
release and an Esc-cancelled hold leave the230,556-byte battle save exact;
holding physical E for9,800ms across enemy action ends exactly one turn.
The resulting canonical bytes equal one matching-core end_turn operation.
The original floor2 primary/backup, NSH9 metadata and language are restored;
an independent cold reopen retains the new preference and the original
230,540-byte canonical run. H's four capacity cases and two-language cold
proofs remain separate historical evidence rather than being renamed I.

I's independent closed-artifact readback passes all13 ZIP members. The
calculator ZIP is90,275,858 bytes (SHA256
`cb95c73324efb7cc1bc98e87b78400eb72fec87275818bab5e7b71b92a70c976`);
the offline HTML is129,282,437 bytes (SHA256
`8ecb49ca4108410fbd634e250a40328ee25ef4886d44cf7ff10090bebda18c06`).
The browser verifies that exact HTML before and after execution.

Screenshot review found a verification-only gap: three old Settings hover
captures used obsolete positions. One showed Phobia rather than Screenshake;
Text Effects and Keyboard-Only did not show the named tooltip. J corrects
their fixed source-coordinate points, adds Long Press hover, and compares
the real Settings result with the specified source title/body and anchor.
It also moves the new long-press FTUE assertions after all older captures, so
their added elapsed time does not change the old animated-background samples.
All new assertions remain. J is remotely verified at
`8a2ebda2cfab09d1ce90925599255cd7e11cf14e`, tree
`12dacbcf996f2cfd8024347508e1ee39e3542ea4`. Its focused gate
`ui10j-fixture-acceptance-2a725e41f14d` passes:683 production inputs compare
exactly to I, the accepted I core remains unchanged, both synthetic targets
and all four bilingual actual runs pass. Each locale's19 original FTUE
captures are again byte-identical to H. Each locale's54 other card/HUD
captures remain exact; the three corrected hover captures and the new Long
Press capture pass their full-screen binding/position oracle with unchanged
canonical bytes. No I launcher, WASM, resource or deliverable changes.

Quiet F/I tests retain the same physical F input. Requested-row batching
reduces callbacks46->31/62->39 and seeks43->12/59->12 for English/Chinese,
while total CPU means11.2674->11.32905ms and19.9334->19.6404ms do not show a
universal CPU gain. One identical actual battle exposes a concrete regression:
180-frame CPU mean74.73375->85.2925ms (+14.13%), median74.736->82.8135ms
(+10.81%), despite all720 paired frame fingerprints/canonical bytes matching
and zero warm I/O. The full raw samples remain. A bounded H/I comparison is
now separating pre-hold changes from the new input feature before any proposed
production optimization. Host CPU is not physical-calculator FPS.

The H/I isolation window does not reproduce the first difference: H's180-
frame CPU mean/median is77.8655/77.141ms and the same I binary gives
76.41175/76.3015ms. Idle hold adds only fixed branches; no extra pixel loop or
font traversal was found. Source inspection alone does not attribute timing
to the changed transform-function or App layout. A final balanced F/H/I
window lengthens this same battle sample to distinguish cross-window noise;
it introduces no new scene. That balanced F/H/I run passes43,200 rendered
frames,14,400 fingerprints per version, canonical equality and zero warm I/O.
Its3,600-frame CPU means are1445.43225/1467.781/1616.211ms, so I is11.82%
above F and10.11% above H in this same-window test. A private default-off
branch experiment is therefore isolating code-path cost while retaining I's
App layout. No production optimization is accepted from speculation or from
the earlier short-window result.

Final performance interpretation, documentation, main ref synchronization and
private delivery remain to be completed. No95% restoration claim is inferred
from source inventories or passing tests.

## UI10i closes two further source-review findings

H is remotely preserved at `0eb82c19b8794f5235e6c5283d339f0c93f3a21f`,
tree `77e404db6c133adacb7c02bbd7c275dc319c4ef0`. Its matching native120/120,
Python251+54, ARM linked ABI/codec, WASM,94 bilingual actual-resource targets
and both complete split-stream comparisons pass. The ordinary browser
passes18 flows/41 captures without page errors or external requests. These
are execution layers, not an original-game fidelity percentage.
H GitHub Actions40174150511 also passes; G Actions40172979898 retains its
failed result alongside the already documented G local findings.

H's ARM allocation is16,251,288 bytes,525,928 below the unchanged16MiB ceiling.
The final physical resources total90,114,227 bytes; the installed four files
total91,602,752. All four ordinary cloned-OS USB capacity cases pass with
exact readbacks:1,027,498;1,421,773;2,085,823;2,701,448 bytes. This last case
models the specified history-write peak, not every combination of stale
failure residues. Normal English/Chinese Save and Quit preserves the actual
230,540-byte canonical run while migrating its17,731-byte NSH8 metadata to
18,267-byte NSH9 (469 bytes in the device container). Its observed saved
revision matches canonical revision24; unknown old build IDs remain unknown.
The independent Chinese and English cold reopens also pass, with both primary
and backup canonical bytes unchanged and the final English preference exact.

Continued source review found an in-scope functional omission: the original
optional Long Press Confirmations setting. I implements its default-off,
per-profile End Turn gate, strictly greater than500ms, release/unfocus decay,
one action per owned press and the source additive204x6 progress bar. Mouse,
mapped physical keys and HTML toolbar controls share the native timer. The
toolbar now forwards real down/up/cancel lifetimes, preserving ordinary
non-combat confirmations. NSPF5 uses reserved bit7 in the unchanged108-byte
preference record; older records retain their original validation and masks.
Canonical NSV17 and NSH9 are unchanged. Short JS/module/transaction checks
pass; matching native, ARM, actual UI, ordinary browser and OS checks remain
required. See `settings-run-setup-ui10-0.17.md`.

The known-date Continue capture also exposed an incorrect H oracle premise:
the original DateLabel has automatic wrapping disabled. Direct pinned Godot
measurement of its400x90 rectangle and font-size estimator retains24px for
English/Chinese unknown and representative longest date strings. I keeps the
corresponding5px glyphs, renders only explicit newlines and uses the measured
English6px/Chinese7px line spacing. It does not invent a smaller original font
to hide the port's integer glyph-advance differences. The previous H pass and
this subsequent visual discovery are both retained. All I production work
must start after the combined source has been pushed and read back.

## UI10h closes the G acceptance findings

G is remotely preserved at `7d159cb4534c7923c10f1e5570a1befea1c9d687`,
tree `20460e9f30fc5cc256fa7f3d12653e0e2329deda`; main remains the accepted F
baseline. The matching G ARM and WASM gates pass. The launcher is1,485,615
bytes and its actual Zehn-prefix+image+BSS allocation is16,247,324 bytes,
leaving529,892 bytes below the unchanged16MiB ceiling. Linked ARM division,
LZMA2, tile and retained-filter regressions pass; WASM is6,761,143 bytes.

G native passes118/120 suites in84.92 seconds. Instrumented production hits
confirm that the two failed UI tests request the former SettingsBack action
after Settings adopted the original Back control. H clicks the actual Back
and adds the strict clipped(0,173,30,21) bounds; both short corrected tests
pass. The original missing-hit failures remain in their receipts. G Python runs250
tests with one error: thirteen new Chinese Settings/date fallback literals
need reviewed inline locale bindings. The subsequent54-test gate was not
executed. The missing bindings are now reviewed without moving old pack
string indexes.

The first G resource conversion fails on the new9px source heading band,
which the offline glyph geometry grammar had not admitted. H keeps old
formats strict: append the86 original images to create NSP2/39 first, then
append the exact fonts and compact only their new payload. This retains the
same old artwork, catalog identities and bounded runtime directory. The
failed G conversion produced no accepted resource set.

H also closes the explicitly identified History room-icon tilt/hover
rotation and Input-settings focus-release motion where supported by the
pinned source. All H code, resources, actual bilingual UI, ordinary browser,
updated NSH9 capacity and normal cloned-OS flows remain pending until a
matching frozen H checkpoint is built and executed. The completed F proofs
are not substituted for these new gates.

The web storage regression retains the maximum295,349-byte NSH8 case and
adds the295,885-byte NSH9 case, including denied writes, interrupted profile
deletion and recovery in another host instance. Its opaque-byte short check
passes; the frozen H execution must also use both valid C++ exported records.

## UI10g follows the fully accepted F baseline

F is now verified on both `main` and `work/0.17-world-parity` at
`361c36c27f06c4905e1899b8bdc2ab281ba9b9cc`, exact tree
`a031c1bc29513b80fbf3fd629bb07ef20fd53453`. Its GitHub Actions run40170450855
passes; the preceding E run40168822819 also completed successfully. F's
matching local native119 suites, Python249+54, linked ARM
ABI/LZMA/tile/RGR, WASM,94 bilingual actual UI targets and both complete
NSL2 virtual-stream comparisons pass. The normal offline browser passes18
flows/41 captures with no errors or external requests. F/E compare identical
468 combat and20 key front captures. These counts are distinct execution
layers and are never added into a source-fidelity percentage.

The ordinary cloned OS passes all four actual USB capacity cases:
1,025,362;1,419,637;2,083,681;2,699,304 bytes. Thus both E failures are closed
by actual writes and readbacks. Original files were restored, Chinese and
English Continue/Save and Quit and two cold reopens pass, and the canonical
73/80HP,109gold,floor2,11-card run remains byte-identical. The original NAND
guard is unchanged. F installs91,480,069 bytes; its physical resources total
90,014,771 bytes,2,394,329 fewer than E, with unchanged logical NSP2/38 data.
Its loaded ARM allocation is16,213,224 bytes,563,992 below the unchanged
16MiB limit. The completed private F ZIP is90,183,913 bytes; HTML129,077,681.
An initial long write into the shared workspace lost the ZIP central
directory after the packaging process's own successful readback. A separate
closed `/tmp` package passed independent full archive reads. Its HTML is
byte-identical to the successfully tested browser artifact. The damaged
workspace copy was discarded; its failed independent readback is retained.

F quiet measurements now cover glyph metadata caching, RGR2 and NSL2 startup
with complete raw samples and retained cold outliers. Warm frame CPU improves
in the aggregate, while NSL2 trades extra decode CPU for much less startup
I/O. `performance-ui10-0.17.md` records both sides. G batches only the requested
NSL2 table rows into256 stack bytes to remove redundant callbacks/seeks;
same-input F/G timing is still required. There is no new resident pool or
integrity-only startup scan.

The next source batch restores whole-control Compendium press modulation,
History body fonts/rows/death quotes/badges and64 original room icons,
Settings and Standard/Custom/Daily geometry/fonts/input and22 source-sized
controls. It adds a small exact font suffix and one combined append-only
NSP2/39 artwork catalog. Sparse9px headings are opt-in so ordinary card/text
fitting cannot select their incomplete alphabet. The full old payload remains
an immutable input; no earlier artwork is recompressed.

Optional NSH9 UI metadata records real successful run-save timestamps and new
runs' observed port build identifiers, retaining unknown old values. Canonical
NSV1/version17 remains unchanged. The fixed536-byte tail grows the maximal
history fixture from295,349 to295,885 bytes. The same-revision optional-write
failure limit is documented in `run-metadata-ui10-0.17.md` rather than hidden
behind extra invalidation writes. Short source/codec/font tests and mixed
diagnostics do not replace matching G native/Python/ARM/WASM, font/artwork
conversion, actual bilingual UI/browser and updated ordinary-OS acceptance.
Old E38 packs correctly fail the new History oracle on missing4px ellipsis
and Chinese quote glyphs; the exact added glyph requirements retain those
failures until the matching G resources are executed.

## UI10f directory storage closes the remaining capacity failure

UI10e source is verified at `04038b2cda0240bfa2be1af79bf62401ec26e7a9`, tree
`0013609a7d365c985f62f8c2798a3af790c12589`. Its native119/119, Python244+54,
ARM linked ABI/codec, WASM and all94 bilingual actual-resource gates pass.
The ordinary offline browser passes18 flows with41 captures, no page errors
and no external requests. Its first Chromium launch was blocked by sandbox
socket permissions before executing the page; the permitted retry is a
separate successful receipt. E runtime allocation is16,211,240 bytes, leaving
565,976 bytes below16MiB. E actual scene/font resources preserve old payloads.

Ordinary USB installation and complete readback of all four E files pass.
Packed capacity1,025,362 and raw-backup migration1,419,637 bytes pass; the
2,083,681-byte raw-history and2,699,304-byte all-raw cases fail actual writes.
The four original run/backup/history/language files were fully restored and
independently decoded, with no fixture files left behind and original NAND
unchanged. The failed E capacity gates are not accepted as completed storage.

F therefore adds NSL2 compression of only the existing cached metadata prefix,
keeping all frame descriptors, common artwork and logical NSP2/38 bytes exact.
The short actual candidate saves2,394,329 bytes without a new resident pool.
Its4KiB-aligned boundary and cached-prefix constraint prevent directory decode
reentry during image reads. See `split-directory-ui10-0.17.md` for format,
ownership and negative cases. The matching frozen F source, physical-container
readback, ARM/WASM/browser, real capacity retry, quiet performance comparison,
private delivery and verified main synchronization remain required.

## UI10e source follow-up and final matched acceptance

UI10d is remotely preserved at `78ac86e35651d3832e1b9638001aa0aa6be3f0a0`,
tree `9a271d5013c8397932bd4c6414aba56a1281ebd9`. Its full Python gate passes
240+54 tests and compileall; ARM and WASM builds pass. The linked ARM image
uses16,196,964 bytes including the Zehn prefix and BSS, leaving580,252 bytes
under the unchanged16MiB allocation limit. ARM division/LZMA/tile/RGR2
regressions pass, including malformed retained-color groups.

The d native gate passes117/118 targets. Its single failure is the frontend
test incorrectly applying new title-menu bounds to the separately laid-out
Pause menu; independent bounds for both menus are restored. The complete d
actual-resource gate passes88/94 targets: the same frontend premise fails
both locales, the Profiles oracle composited reticles before rather than
after text in both locales, Chinese collection headings reveal three real
missing5px bold glyphs, and English world_ui has a separately investigated
fixture failure. All failures remain in their original receipts.

The e implementation restores the source joined-title/description Card
Library search and exact query whitespace handling; Statistics composes its
scrolling subtree once through the source alpha mask; History restores the
source header fonts, positions, information grouping and reminder icon.
Ordinary and Fake Merchant inventory navigation now skips sold entries;
price/icon geometry follows original Godot font/container measurements.
Turn banners project the source24px/16px outline widths through their current
zoom, using one bounded scanline to combine half-alpha coverage before drawing.
The full e runtime and actual resources still require their own acceptance.

The completed d resource split is92,408,754 bytes. Its exact RGR2 transform
saves949,484 bytes and new-glyph compaction saves21,498 bytes against their
matching inputs. The first attempted reused compression receipt was stopped
after exposing an offline reuse bug; it is not counted as a pass. The later
conversion with the legacy receipt passes. The e converter recognizes both
receipt forms, validates cached decoded content, and recompresses missing
cache entries rather than silently skipping profitable groups.

Font-only iteration now preserves versions36–38 and the complete existing
payload. The next resource build appends only missing e glyphs to d's37,
compacts only that new literal tail, and appends the original6×6 History lock
as NSP2/38. No old image recompression, new runtime pool or full startup
validation pass is introduced. The final common/locale set, ordinary browser
and cloned-OS USB capacity/Continue results, quiet benchmark, private delivery
and verified main synchronization remain required. Source inventory and test
counts do not establish a95% original-game restoration measurement.

## UI10d title, confirmations and source speech

The verified UI10c checkpoint is `3d6d79907bb76269474acd8bd5c27baf575837f2`.
All118 native CTest suites pass. Its ARM build and linked ABI/LZMA/tile
regressions pass; the actual Zehn-prefix+loaded-image+BSS allocation is
16,180,804 bytes, leaving596,412 bytes under the unchanged16MiB ceiling.
WASM compiles. Both complete scene-font appends preserve their input payloads:
3534 English and36123 Chinese glyphs, with zero missing required glyphs.
The formal Python run failed on missing private lz4 imports, an obsolete
unknown-version fixture and new Chinese calendar bindings. The environment
and both source issues are corrected in the next checkpoint; a full rerun
remains required rather than relabeling that failure.

The next coherent batch restores source main-menu typography/spacing and its
label/reticle motion, the Continue information panel, and generic popup
geometry/material/backdrop timing. Source SaveTime is unavailable in the
canonical run state, so Continue displays an explicit unknown timestamp.
An independent actual-resource oracle exposed blank Card Library sorting
captions caused by looking in the wrong string index; the renderer now uses
the world index. Locale calendar bindings and an extreme statistics clipping
premise are corrected without weakening their ordinary label assertions.

NSP2/37 appends six source-derived images: the independent blue Fake Merchant
welcome bubble and five precisely sized popup skins. The welcome has its own
delay/reveal/scale/rotation/fade state and does not restart when inventory is
toggled or purchased. Existing images, strings, glyphs and clip clocks are
preserved. An offline appended-glyph compactor reuses existing RGR1 groups
without adding a reader or runtime pool; its c-input proof saves21,517 bytes,
including all descriptor/header/alignment costs. Final d savings must be
measured with final fonts.

Source equations, short functional tests and targeted c bilingual captures
are recorded in the module reviews. The d batch still requires matched
native/Python/ARM/WASM builds, all bilingual actual-resource gates, ordinary
browser and cloned-OS capacity/Continue checks, final private deliverables
and verified main synchronization. The source review continues to identify
real remaining differences; no95% claim is inferred from coverage counts.

## UI10 source-sized scenes, locale fonts and shared workspace

The next coherent integration includes the source Statistics layout and real
row focus/hover tips, Profiles/Compendium independent pixel oracles, localized
profile/history dates, and normal-font locale substitution across frontend,
rooms, reward/tutorial, Ancient and run-end labels. Ordinary events now use
the original text column and option height with dynamic full-page prose;
rest uses the full UI projection; merchant text and turn banners use their
source bands and timing. The rest tutorial shares the ordinary option-picture
renderer. Chomper/Byrdonis multi-hit moves now enqueue each original Attack
animation; OnlyPlayAnimOnce remains a single cue. Short7655 world checks pass;
the full matching runtime gate is still required.

The foreground map and resource-transfer workspaces now share mutually
exclusive storage without reducing their capacities or image caches. The
static object drops from1,412,116 to794,792 bytes (617,324 saved), with explicit
Map-vs-Save/Codec/Bytes leases. Dirty workspace/map/save/LZMA cycling passes
788,050 short checks, and retained-resource callback/reentrancy checks pass
3,403,377. Actual linked ARM allocation remains a separate gate.

The prior cb52b94 RGR2 conversion has completed:689 of840 retained groups are
smaller, all51,123 images per locale decode exactly, and glyphs,8704 strings,
1783 clip clocks and1184 tile/index streams are preserved. That intermediate
split totals92,183,452 bytes,949,484 fewer than its matching input. The final
font append changes the locale payload sizes and must be measured separately;
new output, complete acceptance and ordinary USB migration capacity are pending.

Five stage-A native gates passed (content, content_tiles, split_resource,
boot_locale and library). Its world_ui/world_text failures were traced to two
test-fixture premises: missing original hover artwork and Artifact absorbing
the setup Weak. Both tests retain explicit fallback/source-art distinctions
and now apply the intended canonical power setup. Short reruns pass; the
failed formal receipt remains evidence and is not relabeled a pass.

All3538 pinned C# paths are available for review except five generated or
excluded-platform/multiplayer empty files. Nine in-scope empty bodies were
recovered. Source availability, a directory scan and test counts are not a
95% original-game restoration measurement. Matched native/Python/ARM/WASM,
bilingual rendered scenes, ordinary browser and cloned-OS acceptance, final
private packages and main synchronization remain the delivery sequence.

## UI10 integration resumed from the preserved UI9 source — 2026-10-25

The interrupted Card Library stage is now preserved remotely at
`7332b2f23d0ac41b342326df4eba0e8e6cda13c7`, exact tree
`f3293d412be66221fbd341c98d440de94bb8fb0a`. It is byte-identical to the
previous local `f2e4e83` tree. The new frozen conversion in this turn passes:
English3292 and Chinese32764 glyphs, no missing library labels, unchanged
old image/animation/text payloads. This does not complete the new runtime gate.

The next coherent source stage restores source-sized Profiles labels, original
panel/text transforms, delete-label timing and navigation past empty profiles;
centers the Card Library count; and replaces the legacy combined enemy-intent
panel with the original independent intent/creature hover chain. Intent motion,
number fonts and the localized multiplication sign follow the pinned source.
A bounded glyph-descriptor cache removes repeated binary searches during card
layout and outlined text. Its final performance and ARM budget are still pending.

NSP2/35 appends exact profile and intent fonts without discarding old glyphs.
Its versioned glyph count bound grows to65535, while the resident directory
stays at2MiB and every range/geometry check remains enforced. Old versions
retain their old limits. The required extra entries are only25 English and28
Chinese at this source stage; no font bitmap is silently scaled or substituted.
The final paired resources and exact scene oracles remain required.

Recovered ordinary UI9 save receipts prove the73HP/109gold/floor2 run and
full canonical run/history decoding; the dense three-profile capacity case
also passed. The two raw-migration capacity cases failed actual USB writes,
so neither is counted as a pass. File restoration passed; its last Continue
journal was left running/unknown. A complete live-payload audit found only
about1.4KiB of alignment gaps. A new exact retained-color format is being
implemented separately to address the real migration headroom problem.
It is not included or accepted by this font/UI source stage.

This turn recovered all pinned C# source again for direct review. Source
availability and native/backend agreement are not a numerical original-game
fidelity measurement. Complete source-stage testing, rendered scene review,
normal browser and cloned-OS acceptance, final packages and verified main
synchronization remain the delivery gates.

## Complete matched UI gates and Card Library source follow-up — 2026-10-25

The frozen `bbb47e23e0939bbcf2534ad2ba7d4221e6adce5f` runtime passes all116
native CTest suites. All45 actual private-resource UI targets pass in both
languages:17 world/root,13 front and15 combat,90 results in total. There are
1178 resulting captures, with module-specific composed reviews. The ordinary
browser run now passes18 flows with41 captures, zero page errors and zero
external requests. Its corrected test source is
`66b433cb574f3f0348b34bbde41e7ee4ef7ba51d`; all486 runtime/build files are
byte-identical to the accepted8l runtime. The first browser cursor failure and
native wrapper's wrong-build-directory failure are retained separately from
the successful runs. GitHub's8l run was cancelled by the newer8m checkpoint,
not reported as a CI pass.

Composed browser review exposed another actual implementation gap: Card
Library still used8px labels in its narrow original sidebar. The next source
stage restores original4px bold sorting, rarity and checkbox labels,5px search
and cost text,4px card counts and6px empty results, together with the source
control positions, label-sized sort arrows and selection colors. Card stats
use their original5/6px glyphs at the actual card scales and clip only at the
original label boundary. The old8px temporary bitmap/rescaling path is
removed, including its 5808-byte stack image/alpha buffers. Matching
new-font composed and runtime acceptance is required after this checkpoint.

The required glyph budget was proved against both actual v34 packs. The
selector generator incorrectly counted the literal characters of[red]/[gold]
as4px italic flavor text. All306 Chinese relic flavors were independently
fed through the real C++ RichReader: nine keys `/ [ ] d e g l o r` never appear
as visible output. They were added only by that selector stage, and no other
runtime screen uses the same face/size. The new converter retires only these
nine directory keys, retaining even their old payload bytes. It adds all
required Card Library characters: English3282+10=3292 and Chinese32749−9+24
=32764, within the unchanged32768 limit. The converter verifies all other
old glyph keys/metrics/alpha, every string, image, clip clock and the complete
old payload. The public correction changes the collector, not original text.
The immutable21/22 inputs are preserved; new outputs will be23/24 after the
new source is remotely verified.

The additional retained-color codec is deliberately not installed. A short
native comparison using the actual LZMA2 decoder and once-per-group inverse
preserves all output bytes but slows the measured192 interleaved cold pairs
from3875.739 to4073.144µs on average (+5.093%). This is not an ARM timing claim;
there is no demonstrated performance benefit to justify that storage tradeoff.
Adding the necessary per-group slice table would reduce its full candidate
saving to870,076 bytes. A separate byte-delta sample improves only one of16
groups, by14,891 bytes total, and is not promoted to a new resource format.
The current reader, decoded pools and image payloads remain unchanged. The
offline oracle is retained as future engineering evidence, not a shipped
storage feature.

Ordinary cloned-OS acceptance has cold-booted the real8l launcher, switched
language in Settings, started Ironclad, accepted tutorials with physical5,
walked Neow and the map, completed the three-page combat tutorial, won the
first battle through real inputs, received rewards and completed Save/Quit
and Continue. The continued state shows73/80HP,109gold and Floor2; the reward
scene has no residual hand, energy, piles or End Turn control. Game exit and
OS flush/quit complete normally. Final save-file decoding, capacity cases,
new-library launcher acceptance and private delivery remain separate gates.
Long-lived emulator outputs now reside in `/tmp` until closed: earlier
scratch output paths retained truncated prefixes despite success logs, and
their independent cold-boot reads are retained as distinct evidence.

## Ordinary browser cursor isolation and storage candidate — 2026-10-25

Runtime `bbb47e23e0939bbcf2534ad2ba7d4221e6adce5f` passes the matching ARM
build and12,880 /42 /14 linked checks. The launcher is1,421,585 bytes;
actual allocation is16,755,332 bytes, leaving21,884 under16MiB. The real
history callback passes9469 checks and exports74,001 bytes that independently
decode to the exact295,349-byte NSH8 fixture. Python discovery217 plus the
additional54 tests pass. Matching native and actual-scene groups are running.

The ordinary v34 browser run passes the first15 complete flows, including
Sealed's real four-column selection. Its next assertion incorrectly counted
the visible source mouse cursor inside the relic-icon comparison rectangle.
A normal DOM replay proves that moving the cursor within the still-open paper
restores the rectangle exactly, without any other input. The corrected test
keeps the real icon click, moves the cursor out of that rectangle, and now
requires exact equality. Separate normal-DOM diagnostics also pass right/left
relic navigation, backstop/Escape, Potion focus purity and the visible card
library timer. These are diagnostics; the full formal browser rerun remains
required. This test-only change does not require a different runtime binary.

A private full retained-color experiment preserves every source byte while
reducing candidate LZMA2 payloads by913,893 bytes across474 of840 groups.
The pure offline transform and exhaustive scalar-oracle tests are preserved
here. There is not yet a new resource reader, delivered compressed pack,
device storage saving or performance result for this experiment. A field-plane
shuffle alternative was rejected after its sample saved nothing. Adoption
requires the actual bounded reader, full reversible pack proof and a cold-load
comparison; the current v34 three-file resources remain unchanged.

## Matched v34 gates and final visible repairs — 2026-10-25

The remotely verified source `c3a44adfce84ef6355c89b0038210f58ab63afa2`
(tree `8b48c0cdd67c11476c5ae7754ed4d0f048826442`) passes all115 native
CTest suites and GitHub Actions run40155811647. Python discovery passes217
tests. The first combined local journal then failed because the wrapper named
three nonexistent modules; that failed receipt is retained. A separate correct
invocation passes all54 additional animation, packing and logging tests plus
compileall. These are build/converter results, not an original-game fidelity
percentage.

The complete v34 resources have51,123 images,1783 clips,558 card records,
2061 auxiliary records and8704 strings. English has3282 glyphs and Chinese
32749, within the unchanged32768-entry limit. The bilingual three-file split
totals93,128,740 bytes. The18 Kaiser supplements contain344 original samples;
old image payloads, clip clocks, glyphs and strings are unchanged. Both locale
full packs and their split reconstruction compare exactly. Both complete
Kaiser poses were inspected with the missing source pixels now restored.

Matching ARM is1,419,971 bytes, with actual Zehn prefix plus loaded memory/BSS
of16,753,056 bytes:24,160 bytes below the unchanged16MiB limit. Linked ARM
division/ABI, LZMA2 and TGR checks pass12,880 /42 /14 cases. Matching WASM is
6,632,992 bytes. This is not physical-device performance evidence.

Against the final v34 packs, all17 root scene targets pass in each language,
producing265 captures per locale. Composed review covered direct title,
character selection, the centered map/legend, events, rest, merchant and
owned hovers, treasure, rewards, pause, potions, tutorials and run end. The
first front group passes11/13 per locale; the first combat group passes10/15.
The failures are retained. They expose old test premises: rendering before
bounded combat preparation, advancing through an inspector before its source
exit animation, and comparing event entrance animation as though only rainbow
text could change. The repaired short diagnostics preserve canonical-save
bytes, animation clocks, input barriers and actual text bounds. They require
the next frozen full rerun before being counted as accepted.

Actual visual review also found two implementation defects. Purity's prompt
left `{Cards:diff()}` unresolved; its canonical3/5 source variable is now bound,
with all32 selection prompts checked at both upgrade states. Collection
category headings used8px fitted text instead of fixed source24/inline28
projected to5px. Their complete regular/bold text, source colors and shadow
are restored without truncating the two longest English headings. All20
actual heading variants in each language use existing exact glyph keys; no
resource or glyph-capacity increase is needed. The original fixed4px View
Upgrades labels in deck/world selectors also bypass the general card-text
fitter's5px minimum. Their full labels retain the original control state and
hit regions. Matching full/browser acceptance of these changes is pending.
The calculator's default physical5 Confirm alias is also accepted by the
blocking tutorial dispatcher, consistently with ordinary controls; the
release transition retains tutorial input ownership and save purity.

The first ordinary browser run passed14 entry, settings, profile, tutorial,
save/continue and language-persistence checks before its Sealed navigation
fixture repeatedly visited four cards: Right wraps within the source's
four-column row. The corrected fixture uses actual Right/Down navigation,
checks that nine cards cannot confirm and that ten commit only on release.
This is a test repair, not an alternate release navigation behavior. A new
ordinary-browser run must use the next matching WASM and final v34 resources.

The exact highlight optimization has now passed748 old/new whole-frame pairs
(57,446,400 RGB565 pixels), with identical canonical State, hits and presentation.
In an isolated native warm-render measurement,1920 interleaved old/new pairs
per locale show mean1477.545→1446.472µs English and1468.639→1445.731µs Chinese
(2.103% and1.560% reductions). Warm resource read calls and bytes are zero.
This is the measured scene on the native memory provider, not ARM FPS.
A16-of732 tile-group context sample saved only592 of936,431 compressed bytes;
it is not a full-pack saving and does not justify another resource rewrite.

Calculator history now shares the existing512-byte streaming NSZ1 envelope
with runs, preserving canonical NSH1–8 and raw fallback. Its valid full NSH8
fixture compresses295,349→74,001 bytes and decodes exactly. One process byte
tracks existing full-load recovery facts; a known malformed primary cannot
overwrite the valid recovery backup. Fault injection covers normal, unknown
I/O, invalid-primary, missing-primary, rollback and retry paths. The short
actual-callback test passes9463 cases; matching native/ARM and ordinary device
execution remain required. No resident buffer or startup integrity scan is
added. See `run-storage-ui9-0.17.md` for exact limits.

Capacity cases must remain distinct: with these measured packed histories and
dense runs, a three-profile history-update peak is1,025,362 bytes; retaining
three conservative old raw run backups raises it to1,419,637. The first update
with three old raw full histories instead peaks at2,083,681, and the all-raw
conservative bound remains2,699,304. Filesystem costs are additional. Rounded
OS free-space labels are not exact capacity evidence. The ordinary cloned OS
is cold-booted and its game directory is empty; final four-file installation,
actual play/save/continue, capacity write/readback and restoration are still
open. Final calculator ZIP/offline HTML delivery and verified main sync follow
those gates. Original assets and captures remain private.

## Kaiser crop completion and bounded selector fonts — 2026-10-25

The matching `f3585dc5f8be8c12ca76187d130b4e556fd9b946` native build now
passes115/115 CTest suites. Its ARM launcher is1,419,446 bytes; the actual
serialized Zehn prefix plus loaded memory/BSS is16,752,616 bytes, leaving
24,600 bytes under the unchanged16MiB limit. Linked ARM division/ABI,
LZMA2 and TGR checks pass12,880 /42 /14 cases. The matching WASM is6,632,532
bytes. These are build/runtime gates, not original-game visual parity or
physical-device timing evidence. The Python suite passed211/212 before
rejecting two unregistered Bestiary fallback labels; their locale bindings
are repaired here, with the focused six localization tests passing.

Source-based composed inspection exposed a real Kaiser Bestiary crop: the
shared world animation ends at a320×218 rectangle, while the original
Bestiary translation shows pixels outside it. The next resource profile is
NSP2/34 with18 supplemental clips /344 frames, bringing the exact animation
directory to1783 entries. Only visible original pixels outside the retained
world crop are appended at native sampling coordinates. Original world
frames, frame clocks and earlier directory IDs remain unchanged. The runtime
draws sparse supplements using the existing image cache and no new pixel
buffer. Both full/split readers and the boot language hint support the new
profile; old v33/1765 profiles remain accepted. Focused format checks pass,
including the unchanged32768 glyph limit. Full generation, composed capture,
new ARM allocation and ordinary-host gates are still required.

The private recovery chain passed stages6–18, including lossless retained,
glyph, string and tile compression. Stage19 correctly stopped when selector
font insertion would produce32777 Chinese glyphs, nine over the existing
bound. The cause was adding0–9 to every static label/title/flavor even when
none occurs in that text. The converter now adds dynamic numeric characters
only for template-bearing text and preserves every literal character and
every old glyph. Exact budgets are English3189+93=3282 and Chinese32021+728
=32749, leaving19 entries. Seven focused converter tests pass. The full
selector/split rerun remains pending this checkpoint.

The ordinary merchant/room regression passed14,606 checks in each language
against the recovered stage18 resources, and the actual stock relic/potion
and owned-relic hover captures were inspected. This is an early room-specific
check; final selection resources and the complete bilingual run still need
their matching acceptance. Ordinary calculator installation, maximum-save
capacity, whole-frame highlight timing, offline browser flows and final
private delivery remain open. No restoration percentage is inferred from
these counts.

## Integrated follow-up candidate — 2026-10-25

The next runtime stage corrects additional visible differences found while
reviewing the original scenes and methods. Bestiary now follows the original
list/header geometry, fixed font bands, controls, per-move animation preview,
and physical key release semantics. Mode preference is persisted on submenu
close. Its presentation structure increases392 bytes; canonical State and
save formats do not change. The shared Kaiser frame registration still needs
actual composed inspection after its corrected0.75 source transform.

Card descriptions now assemble the original keyword prefix/suffix order,
including the missing Purity / Purity+ / Scrawl+ Retain paragraphs, mutable
Retain/Sly/Ethereal and Hexed, detached enchantment facts and localized Replay.
Only complete keyword paragraphs are replaced; prose mentioning the same
words remains. Cookie relic icons now distinguish canonical/no-owner process
variation from actual player/history ownership; collection outlines use that
same process variant. This adds one byte of process UI state, no gameplay RNG
draw or save field.

Ordinary merchant relic/potion stock now uses the complete original model
hover set and source-sized columns, replacing the remaining174px legacy box
and five-line cutoff. Associated cards and keywords, detached stock values,
modal/input suppression and save purity have focused regression coverage.

The highlight renderer skips source samples proven transparent by an exact
32-bit threshold. The old scalar shader remains unchanged;12,592,419 oracle
checks pass. An isolated actual-SDF kernel improved23.41% /34.25% in two
scenarios; whole-frame and ARM timing remain unmeasured. Zehn's allocation
gate now includes its serialized prefix as well as loaded memory/BSS and
rejects overflow before replacing an existing output. The unchanged16MiB
boundary passes two synthetic ELF cases. Source syntax and pure checks pass;
all changed runtime code still requires the next matching full/native,
actual bilingual, browser and ordinary calculator gates.

The exact-font source is already preserved at
`4e8a6e5377eed0b869de24a829555a5259377e7e`, tree
`e5ef51a959f85475e3807579fa2504ff133e34a9`, with remote ref readback. The private
conversion chain has passed fonts, death append, history/journal/transition
and tooltip glyph stages. The final compression/selection/split chain remains
running, and is not yet a final resource acceptance result.

## Recovered full gates and Bestiary font stage — 2026-10-25

The matched `3e61c041ffdc02e82ea8f8e14dc27601e2751aac` native build passed
113/113 CTest suites. The wrapper's redirected console log is incomplete;
the independent complete CTest LastTest.log contains all113 passes and no
failed test. Python passed209 tests plus54 animation/packing/logging checks.
These are native/converter gates, not original-game visual parity evidence.

The original input chain has been reconstructed through v28 history art,
and all1006 character-death frames /12 clips have been regenerated. A private
recovery archive preserves the split stage5 pack, death cache and exact font
and converter inputs. It is a development checkpoint, not the final release.

Bestiary source review found the MonsterName uses Spectral Bold48 (native10),
Actions/Mode uses bold28 (native5), act/event dividers use bold32 (native6),
and dialogue uses italic28 (native5). The font converter now adds only the
actual reachable titles and labels at those exact face/size bands. A focused
font-band regression passes. The full private font build must establish the
final glyph count against the unchanged32768-entry limit. Bestiary geometry
and input, Retain description, Cookie ownership, highlight and allocation
repairs are still being integrated separately from this font-only stage.

## Environment recovery and texture-bound repair — 2026-10-25

The complete selector/inspection source is preserved at
`1e50404901ab398574fcf8fa44d863e8961248ef`. The transient executor was recycled
while its native, ARM, WASM and resource jobs were running. Their missing
completion records are **unknown**, not passes. The repository was restored
at that exact commit. The original owner PCK/DLL and previously delivered UI6
split resources were recovered from the saved original archives; their source
identities were verified. Newer private packs and conversion caches must be
rebuilt, then persisted privately before further long acceptance work.

The relic resource append failure is now reproduced against the exact original
PCK: the source frame texture is408×408, whereas the scene's containing Control
is304×304. The converter confused those bounds. The repair accepts the actual
408px texture, records both bounds and retains the58×58 projected frame.
Runtime inspector geometry remains based on the304px scene Control.
The corrected conversion and final composed acceptance remain required.

The recovered remote CI run
[40118639939](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/40118639939)
built native, ARM and WASM. Native passed102/113 suites; converter tests
passed208/209 before stopping. The eleven native failures were traced to
test premises retained from the replaced selector/inspection UI: source
entrance/exit clocks, focus-driven hand layout, the original Back action,
filtered nonremovable cards, and source single-candidate shortcuts. The
repaired tests retain gameplay, invalid-input, save and pixel assertions.
Their individual Release reruns against the frozen1e504 core all passed;
the next full matching suite remains required. The Python failure required
the obsolete text-checkmark glyph although original checkbox images had
replaced that literal; the corrected symbol coverage check passed.

Recovery also rebuilt the exact1e504 WASM (6,612,714 bytes), the public
Firebird controller, original fonts and fixed official Spine dependency.
The original CXII NAND/boot identities were recovered; an isolated NAND
clone boots the ordinary OS6.4.0.74 and has installed Ndless. Original NAND
bytes remain unchanged. This is environment preparation, not final game
execution. Actual regenerated NSH8/dense-NSZ capacity fixtures match their
historical hashes and sizes:295,349 /121,503 bytes; final OS write/readback
and three-profile capacity acceptance remain required.

## Active objective and continuity

Continue 0.17 on `work/0.17-world-parity`: pinned original UI fidelity first,
calculator performance second, lossless storage third. Reference remains
**v0.111.0 / 41cef1ea**. Audio, multiplayer and Timeline are the only agreed
exclusions. The user's five reported defects were startup language selection,
legacy expedition settings, potion destination selection, a small merchant
inventory and a left-shifted map. Their runtime paths were corrected earlier
in this work; final composed acceptance is tracked below.

The user authorized source checkpoints, GitHub Actions and synchronizing
verified progress to `main`, plus delivery of a calculator ZIP and bilingual
self-contained offline HTML. Original game assets, PCK/DLL, fonts, emulator
images, generated packs, captures and embedded HTML stay private. Do not claim
an original restoration percentage from path inventories or passing tests.
Do not treat probes as implemented functionality. Preserve each coherent source
checkpoint remotely, read the ref back, freeze with `git archive`, then run
long jobs with `tools/run_logged.py`. Clang 18 is the verified native compiler;
a GCC 13 internal compiler error was recorded earlier.

The previous 300 KB chronological work log remains exactly available in Git:
[complete earlier stage record at d938a661](https://github.com/maxmixalex/slay-the-nspire-2/blob/d938a661c7a69e46b829a4a879ce6cd411be0f2b/docs/WORK_STATE.md).
This shorter current record preserves active decisions and verified boundaries;
it does not discard that history or replace module-specific evidence.

## Integration status before the complete selector/inspection checkpoint

The latest complete runtime/resource build is stage8e, `492915341106b129f730e46ed84d152637a24b7d`.
Its native/ARM/WASM and resource builds passed; the detailed acceptance below
retains the initial FTUE and map-window fixture failures and subsequent short
repairs. The earlier a0df measurements in this document are historical, not
the current source. No final artifact from the new selector/inspection work
has been delivered yet.

The coherent core checkpoints `1606be12622f1bffe8b557ed6ee01b8781b32429`
and `5afb7a9aa8b719b31dcd282c203f43b4341ce7c1` are preserved remotely with
exact tree/ref receipts. The complete integration adds original combat,
standard-deck and world selectors; automatic all-eligible source shortcuts;
pause and potion entrance/exit behavior; original relic inspection; and the
FTUE arrow orientation correction. Their independent short checks have
passed, but require the next matched full source/resource/ordinary-host gate.

The 5afb resource receipt is an intermediate v33/2060 result. The candidate
v33/2061 converter uses12×12 upgrade arrows, the source card-frame shader
input and58×58 relic frame,32 original prompts and append-only native glyphs.
Its five public synthetic data/format checks passed. The selected-card shader
optimization passed5,317,924 exact old/new scalar and coordinate comparisons;
ordinary ARM timing and final linked-code review remain separate evidence.
No renderer mutates canonical gameplay to satisfy these comparisons.

## Historical verified runtime and resources before stage8e

Runtime `a0df79d8347b76dc86804b0ab8d17f948c600204` passed **103/103 native**
suites and **201/201 Python** tests. Matching ARM and WASM built. Its launcher
is 1,350,761 bytes, SHA-256
`4e49fd48fad18fac98676fe0a0876e96a4d94d6e2f6b48f7994b7fbcea922ea3`;
allocation is 16,637,436 bytes under the unchanged 16MiB gate. Linked ARM checks
passed 12,880 division/ABI, 42 LZMA2 and 14 tile-filter cases. Linked checks are
not physical-device frame-rate evidence.

Actual final-v33 card/HUD runs passed 359,342 English / 383,522 Chinese checks.
Power/keyword/associated-card runs passed 635,832 per locale. The final root
scene pass executed nineteen targets per locale: eighteen passed; the only
failure was the map-window fixture opening combat input before explicit asset
preparation completed. Its source repair waits only for that bounded phase,
asserting unchanged canonical State and animation, and keeps all map/layout/
route/alpha checks. Formal matched re-execution remains required.

A subsequent multiple-image preview suggested missing GameOver banners and
buttons for some characters. Direct complete-region comparisons proved this
was a preview interpretation error: original PPM pixels match exactly across
Ironclad/Silent/Defect/Necrobinder in both locales. The source texture is present,
alpha is 255 and Content is healthy. Runtime drawing was correctly left alone.
An added opaque-source-pixel oracle passed short frozen-core checks (33,531
English / 32,814 Chinese); it needs the next formal matching suite run.

Resource conversion through `424c06c6d29f8230fcbac8969627fe20cea0a593` produced:

| Component | Bytes |
| --- | ---: |
| Common | 84,719,566 |
| English | 2,153,050 |
| Chinese | 5,149,970 |
| Three resource files | 92,022,586 |
| Including a0df launcher | 93,373,347 |

NSP2/33 has 2051 auxiliaries, 8704 strings and 1765 clips. Old original pixels,
alpha, frames, timing, glyphs and locale values remain exact. Grouped glyphs and
strings reuse existing cache/transfer memory. TGR2 keeps 728 smaller color
groups, preserves four non-saving TGR1 groups, and saves 3,733,886 common bytes;
443,044,043 decoded bytes per locale were compared. Twenty-four missing original
Chinese bold 5px title glyphs added 1,608 bytes. All 2,783 hover fields per
locale have zero missing required glyphs. An extra literal-context sample saved
only 632 bytes over 24 groups and was rejected; no further image degradation or
production context expansion was used.

Keep original v31, grouped-font v32 and final-v33 resources and exact receipts.
Superseded old packs/HTML and obsolete native builds were retired only with
file-hash manifests; captures and acceptance records remain. Main has not yet
been synchronized with this active branch.

## Ordinary calculator execution and save capacity

Final a0df files passed ordinary USB transfer and exact readback in a disposable
Firebird OS clone, preserving the original NAND. OS display: 92.3 MB total,
2.4 MB remaining after installation. The ordinary flow passed Chinese title →
in-game Settings/English → Standard/Ironclad → Neow Golden Pearl → centered
map → five-turn real combat victory → eleven gold and Twin Slam → Save & Quit
→ Continue with **69 HP, 260 gold, eleven cards**, then normal OS return.
Raw NS17 primary/backup were both 230,540 bytes; history 17,729 bytes. There
were zero reported resource failures. No developer commands or RAM injection
were used. Journal `ui9-firebird-final-flow-fdd5d6335ab8` is passed.

Installation alone did not guarantee three full profiles plus temporary writes.
A full NSH7 history is 295,347 bytes; a conservative canonical v17 run bound is
252,928. Existing ordinary backup semantics require a history-write peak of
`4H + 6R + 340 <= 2,699,296` bytes before filesystem allocation.

Checkpoint `d938a661c7a69e46b829a4a879ce6cd411be0f2b` adds the **NSZ1 host
storage envelope**. Canonical NSV1/v17, RNG, history and backup identities remain
unchanged. A two-pass zero/literal stream uses a 512-byte foreground stack
buffer. It writes compressed bytes only when strictly smaller; old raw saves
still load, and incompressible files never grow. No new pool or integrity-only
hash is introduced. Exact-host tests passed **1,410 checks**, covering old v3–13
fixtures/raw v17, canonical equality, short/truncated/oversized/trailing input,
close/write/rename failures, existing leases and primary-only loading policy.

A dense valid State with all live card/relic/reward/hook capacities and nonzero
metadata compresses **250,964 → 121,503 bytes**. Three such runs and full NSH7
histories peak at 1,910,746 bytes; retaining three conservatively bounded old raw
backups during migration peaks at 2,305,021 bytes. These are explicit fixture
bounds, not a claim that every arbitrary accepted raw payload compresses so
well. The unconditional raw fallback bound remains 2,699,296 bytes. See
`run-storage-ui9-0.17.md`. Ordinary8d readback now passed: the old230540-byte
raw run continued, and two explicit saves produced7456-byte NSZ1 primary and
backup. Both independently decode to the exact original canonical bytes. A
USB-session timeout remains recorded as failed; a restarted normal OS session
completed readback without changing any save bytes.

The d938 ARM build and all three linked gates pass. Launcher: 1,352,081 bytes,
SHA-256 `b7e453c9646b117dca2832872a019909b772a43a54b949614575d2008de7bb5b`;
allocation 16,639,308, remaining 137,908 bytes. Production ARM uses `-O2`.
Tile filter mode dispatch is now outside its pixel loops: the actual linked
function selects mode once per group. This is an executed code improvement;
static instruction differences are not physical-device timing measurements.

## Implemented UI and performance work

The source-led changes include full standard/custom/daily setup and twenty
modifiers; three profiles; original native fonts; original large merchant;
centered retained-room map, drawing and per-floor history; actual terminal
combat reward ownership; original card/relic/potion/keyword hover columns;
all five normal/merchant Die tracks and separate Defect/Regent particles;
source GameOver intro/summary/score/badges/discoveries; original reward/rest/
treasure/Ancient/Crystal flows; real preferences, timer, rebinding, feedback,
notes and credits. Details remain in the corresponding module audit documents.

Latest source corrections include the original card-rule resource cross,
preview-only changed upgrade-cost green/outline (permanent upgrades remain
ordinary), ninety-nine Power ExtraHoverTips with independent-instance values,
source Creature anchors/debuff skins, and adjacent associated-card placement.
Energy/stars/orbs/settings/map legend/drawing/GameOver tips use native 69px/5px
source projection. Proceed uses source YIQ HSV and independent hover/disable
motion with no per-button allocation increase.

Executed performance improvements include 200,368 bytes less State/codec BSS;
map workspace 1,114,120 → 794,788 bytes with 1024 exact graph/RNG comparisons;
merchant occlusion with 2376 exact full frames per locale and quiet native
steady-render reductions of 28.74% / 19.94%; and history glyph/string cache
indexing with 7200 fewer ARM bytes, 2160 exact frames per locale and zero warm
provider I/O (prior Chinese repeated-pass requests were 28.7 / 14.3 MB).

Same a0df runtime with v32 versus final-v33 packs produced **3246 exact frames
per locale**, unchanged State in six scenes, and zero reads in every final warm
window. Three quiet cold repeats show first/preparation increases of roughly
0.68–5.52 ms native (2.3–20.1%); whole-v33 grouping changes add 3–11 read requests
and 19.8–70.5 KB in those cold cases. Do not attribute this whole-pack result to
TGR alone. The subsequent actual ARM mode-loop improvement needs final timing.
The ordinary Firebird report's map mean includes cold/overlay frames; isolate
that cost before changing source geometry, frames or cache pools.

## Newly found required gap: singleplayer tutorials

The broader source audit found thirteen singleplayer FTUE screens previously
suppressed as already seen. They were not an agreed exclusion and are now being
implemented. The tutorials are accept/decline, three-page combat basics, map
selection, cannot-play, still-can-play, power card, shuffle, combat reward,
potion, relic, rest, merchant and singleplayer Ascension. Ascension uses
`SeenPopup` independently of the general tutorial enabled flag.

Resource tools append seven original images at auxiliaries 2051–2057 and thirty-
one original texts at reserved slots 8347–8377, preserving all old payloads and
keeping NSP2/33, 8704 strings and 1765 clips. Three focused converter tests pass;
the first production conversion passed with zero missing required glyphs.
Runtime layout review identified the source4px standard body requirement.
The small-font production attempt `ui8-ftue-smallfont-production-afb7039c49d3`
failed the older tool glyph-size whitelist before writing a final receipt.
The corrected v33-only validator and independent glyph-preservation test are
ready; rerun from the new frozen source before using the pack. See
`ftue-resources-ui9-0.17.md`.

The profile candidate adds only two serialized bytes in **NSH8**: thirteen seen
bits, enabled, initialized and a rejected reserved bit. Earlier NSH1–7 decode
to a compatibility state without fabricating seen history. Actual new empty
profiles enable tutorials after the existing save lookup; no new startup scan
of all profiles is added. No-persistence native fixtures keep legacy behavior.
Source release `NGame.IsReleaseGame()` is true; Reset Tutorials and its divider
are hidden in the pinned scene and remain hidden in the ordinary port.
The complete trigger/input/render candidate and its native/browser regression
sources are ready for the next combined freeze. The source4px production append
passed at f8701249 with zero missing glyphs: English3181 and Chinese31675 glyphs.
The final append also adds the original display face for the post-tutorial
Battle Start banner. No tutorial completion claim is made before integrated
execution.

Ordinary first-victory review found an additional concrete defect: the reward
list retained hand, energy and End Turn beneath it. `finish_combat_feedback`
mistook terminal WorldRelicChoice(parent Reward) for a suspended combat choice.
The candidate completes that trace, allowing the existing source-shaped
`combat_reward_room` to render. A new real select/aim/Strike regression against
the frozen a0df core reproduces the exact stuck-trace assertion after its full
canonical-save oracle has already passed. Matching execution of the repair is
required; the earlier all-scene declaration is not treated as proof of this
previously missing transition coverage.

## Remaining gates and boundaries

Finish concrete FTUE display/trigger/input integration and review every new
bilingual scene. Preserve source before production conversion/builds. Re-run
matching native/Python, final ARM/WASM, actual-resource map and GameOver oracles,
new-profile tutorials, save/Continue and the complete offline-browser flows.
Complete any measured map-render optimization without changing pixels/State.
Then build matching calculator ZIP/offline HTML, verify them, save the files for
the user, update evidence/audit statuses and fast-forward verified source to main.

The source inventory enumerates 3538 C# paths, 724 node paths and 1022 scene
paths; empty decompilation outputs have relevant recovered method sources.
Path enumeration is not review of every method. GPU noise/turbulence, some
secondary particles/trails/material variants, randomized Cookie/creature visual
variants and every monster-specific timing path still lack full differential
proof. Original-game paired captures and physical-hardware measurements are not
available. Do not convert adapted 320×240 rendering or emulator results into an
unsupported “95% original fidelity” assertion.

## Stage8e integrated acceptance — before fixture-only checkpoint

Runtime/resource checkpoint `492915341106b129f730e46ed84d152637a24b7d`
built native, ordinary ARM and ordinary WASM successfully. Python passed
204/204. Initial CTest passed105/106: only the new FTUE fixture failed.
Initial bilingual actual-resource acceptance passed20/21 targets per locale:
the remaining map-window failure was the last reset/wrap fixture starting
before actor preparation. All runtime sources remain unchanged by the fixture
repairs. Complete tests are retained, including canonical State and animation
purity during bounded preparation, source reward-card entrance timing, and
strict synthetic tile-directory bounds. Short repaired FTUE checks passed
59564 synthetic /5987 English /5284 Chinese; repaired map passed82678 per locale.
Formal execution must use the next remotely frozen test checkpoint.

Actual terminal select/aim/Strike now passes full canonical-save equivalence
and closes the combat display queue. Both actual locales pass4076 reward
checks, with inspected captures showing no hand, energy, piles or End Turn
beneath the terminal reward list or card choices. The GameOver source-pixel
oracle and all other actual scene targets pass.

Final FTUE resource conversion and byte-identical split roundtrip passed.
The seven original images,31 texts and native4px/18px font additions retain all
old payloads. Common84772852 + English2160488 + Chinese5184978 =92118318 bytes.
The ARM launcher is1370912 bytes (SHA256
`17634b518ef85b3a46ac4d1d42da7205e32e281ab3e71b2f44a518cd63968918`);
allocation16672244 leaves104972 bytes below16MiB. Linked12880 division/ABI,
42 LZMA2,14 tile-filter and9110 additional signed32-bit division/remainder
checks pass. These do not establish physical-device FPS.

Per-profile formal storage checks pass6413 core/export plus259 Ndless cases
and the actual exported NSH7/8 browser transaction tests. Full NSH8 is295349
bytes. The final ordinary offline HTML passed16 browser flows and27 captures,
including actual Yes/No, three tutorial pages, final-page marking, real Neow
and map travel, v17 first-turn save/reload/Continue without repeat prompts,
three-profile isolation and deletion, Settings language/fast/timer persistence.
No page errors or external requests occurred. The first Chrome launch was
blocked by sandbox setsockopt; its failed receipt remains distinct from the
subsequent approved local-only pass.

USB upgrade exposed a real temporary-storage constraint: replacing the old
Chinese sidecar while it still existed failed with-255. Removing only that old
resource and sending the new matching sidecar succeeded. The install session
remains recorded as failed; subsequent readback/startup uses a new gate. The
release instructions now explain removing all three old resource sidecars
before replacement while retaining every save/profile file. Final ordinary
map timing, new-profile first victory, three-profile capacity and final
source/main/artifact delivery are still in progress.

## Stage8f core/resource checkpoint — UI integration still in progress

Independent source review identified active legacy standard-deck, combat
selector, world-choice, pause and potion-popup presentation. These are now
being implemented in parallel, not deferred as a diagnostic-only result.
The core/resource checkpoint preserves the selector command shortcuts,
32 private original prompt bindings and a two-image append-only converter.
See `docs/selection-ui9-0.17.md` and the exact caller audit in
`docs/combat-selection-source-0.17.md`. The converter's four synthetic data
contracts and short Clang syntax checks pass; formal core execution and
private conversion start only after its branch ref is verified remotely.
The complete UI worktree remains uncommitted until those agents finish and
must receive its own remote checkpoint before the final full builds.

An additional actual FTUE pixel oracle found the relic arrow missing its
source vertical flip. The narrow correction changes870 pixels per locale;
the old parameter fails the independent orientation check and the corrected
private-overlay diagnostic passes6113 English/5410 Chinese checks. This is
a runtime correction, unlike the earlier FTUE/map fixture repairs. Its
formal final gate remains part of the complete UI checkpoint.
