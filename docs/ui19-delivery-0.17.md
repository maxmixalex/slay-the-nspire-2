# UI19 delivery — nSpire 0.1.0 (StS II v0.111.0)

Date: 2026-10-30 UTC. Reference game: **v0.111.0 / 41cef1ea**.

This delivery continues the complete local UI18 baseline `fc50e39`, rather than
assuming the older published repository contains the recovered implementation.
The tested production runtime is
`090023d45ec6211da3b2e5a887cfcc2d1dd68004`; the final resource converter is
`171f30b97e03b0a18e2977bdd0459f31689d1ec6`. Later source changes are offline
packaging/verification tools, test assertions, scope metadata and documentation.
The source archive records the final integrated tip and verifies its production
sources against the tested runtime.

## Delivered storage result

These figures count the launcher and actual game-resource files, including the
multipart index, all required artwork parts, selected locales, relocation and
padding. ZIP sizes are separate. Instructions/licenses, save growth and
filesystem allocation overhead are not included in program/data sizes.

| Installation | UI18 program + data | Accepted UI19 program + data | Reduction |
| --- | ---: | ---: | ---: |
| English + Simplified Chinese | 79,681,674 B | **74,538,517 B** | **5,143,157 B (6.454630%)** |
| All sixteen languages | 98,226,131 B | **93,036,373 B** | **5,189,758 B (5.283480%)** |

The bilingual result is **71.085469 MiB**, compared with **75.990366 MiB** in
UI18. One MiB is 1,048,576 bytes. The final calculator archive has another **70,161 B** of instructions and
licenses: its uncompressed members total **74,608,678 B**. A compressed download
length is not an installed resource length.

Only the needed language files must be installed. Simplified Chinese alone uses
**73,761,595 B (70.344539 MiB)** of program/data; English alone uses
**70,085,971 B (66.839190 MiB)**. These single-language figures are not the default
bilingual result. The earlier 65–70 MB default goal is not claimed as achieved;
units and selected languages matter.
These are supported optional file combinations: the production launcher chooses
an installed locale even when English is absent, covered by the existing
single-language boot regressions. This round's final ordinary measurements used
English alone; the single-Chinese combination was not separately installed and
visually accepted on the ordinary emulator. Its contents have host and embedded
resource validation.

The final launcher occupies **1,724,027 B**, SHA-256
`fd40e3badd58366a1b48f9525daa4ef92709510b0c7d89756b0b87349dfd1d52`.
It is 4,804 B larger than UI18; the installation savings above already include
that increase. The final all-language resource set contains 25 files and uses
**91,312,346 B**. The bilingual subset uses **72,814,490 B**: **67,585,022 B** of
shared resources, **776,922 B** of English and **4,452,546 B** of Simplified
Chinese. The complete final manifest has SHA-256
`463582860bf9a354771cbfd2f9ac5044433b943bcdcc019f55d8a7c1efd569ab`.

### How the reduction was obtained

RGR3 reversibly rearranges retained geometry, colour and alpha streams before
compression. Two measured passes merge compatible retained groups within the
same physical artwork concern and the existing 512 KiB raw-group bound. The
final tile policy reduces 1,155 groups to **637**. It keeps **59 source pairs**
as their original small groups for the measured Bowlbug, hero/Osty and Construct
working sets, while other groups can use the 1 MiB layout. The maximum decoded
tile group is 1,047,200 B; every frame and every decoded tile record is retained.

The mixed-tile stage saves **3,471,759 B** across all sixteen languages after the
retained passes. Bounding Merchant's active groups then adds **309,239 installed
bytes**, leaving a net **3,162,520 B** reduction after those passes. Its 97
medium body groups use existing RGR2 at unchanged one-page boundaries, at an
encoded cost of 92,820 B versus RGR3. Top/Focused retain their smaller original
RGR2 groups. The converter derives protection from semantic models/animations,
validates complete raw bytes and rewrites all directories; it does not bake in
physical group numbers from this one release.

The smaller all-paired candidate was **rejected by the ordinary performance
gate**, despite passing pixel equality. The accepted final data cost 694,403 B
more across all languages than that candidate and remove its severe measured
cache thrashing. That rejected candidate's 90,617,943-B all-language or
72,120,507-B bilingual data figures are not the delivered sizes.

The fixed picture arena remains **13,369,344 B**. Reconstruction reuses existing
transfer/workspace storage and temporarily borrows the room-raster cache. The
layout adds no large static pool. A reproduced background/foreground shared
index-pin ownership bug was fixed before the final runtime was built. Existing
formats remain supported; unknown future resource version 45 is rejected.
The cache-policy repairs themselves change offline data, not the ordinary
runtime or its allocation.

The final ARM allocation gate reports **22,298,288 B (21.265305 MiB)**, below the
existing allocation guards and 24 MiB ceiling. This is a build allocation value,
not a measured whole-process or heap peak. ARM sections are text 2,520,080 B,
rodata 665,492 B, data 4,092,000 B and BSS 14,863,032 B.

Detailed stage receipts and algorithms are in [retained storage](ui19-retained-groups.md),
[retained planes](ui19-retained-planes.md), [paired tiles](ui19-tile-layout-0.17.md)
and [cache recovery](ui19-cache-recovery.md).

## All thirteen current feedback items

| Feedback | Implemented behavior |
| --- | --- |
| Upgrade-preview control overlaps cards | The control has a separate reserved region; hovered card bounds respect it. |
| Treasure ribbon title too low | The visible title baseline is moved into the original ribbon bounds. |
| Draw-pile insertions fly to discard | The presentation follows the actual destination, including generated cards, selected discard-to-draw transfers and restored pending selections. |
| Two-digit floor number crowds the boss icon | The floor label is centred within its fixed field. |
| Sovereign Blade has no tooltip | The live card/upgrade/enchantment facts appear on hover. Original hover behavior pauses that sword's orbit and resumes continuously; attack/forge/remove clocks remain functional. |
| Defeat text jumps and panel is too wide | The quote retains its position through Continue/fade; the summary uses a compact layout. |
| Defeat leaves a resumable save | Terminal entry retires the owned current-run checkpoint after progress/history handling, matching original RunManager.OnEnded. Real enemy-caused death was exercised; current sav/bak/tmp disappear while progress/history and other profiles remain. Debug-console death is not used as this evidence. |
| Continue popover is too far right | Its anchor is moved left toward the Continue label. |
| Character tooltip padding; Random stats absent | Padding increases; Random displays the original unknown HP and gold values, ??/?? and ???. |
| Draw/discard badge counts off-centre | Visible glyph-ink bounds are centred, including multi-digit values; per-pixel division is removed from the drawing path. |
| Save and Quit lacks transition | Saving/exiting includes fade-out and title fade-in. |
| Version reset | The active label is nSpire 0.1.0 (StS II v0.111.0); VERSION and active release packaging are updated. |
| Osty position and Die for You | Osty uses the original final position farther left, and exposes the trait icon/tooltip. Existing interception, death and revival behavior was checked and covered; it is not misreported as a newly invented mechanic. |

Source anchors, actual ENG/ZHS captures and executed behavior checks are in
[UI feedback](ui19-ui-feedback.md) and [combat/save feedback](combat-ui19-0.17.md).
Earlier UI14 omission lists are superseded by implemented UI15/UI17 behavior;
those older completed features are not counted again as new UI19 work.

## Executed acceptance

| Gate | Result |
| --- | --- |
| Final ordinary ARM build, runtime 090023d | Passed; Zopfli launcher and allocation recorded above; source unchanged during build. |
| Final native suite, runtime 090023d | **157/157 passed**, 64.58 s; source unchanged. |
| Final integrated Python suite, ec76d3b | **369/369 passed**, 48.898 s; source unchanged. Includes final semantic tile policy, merchant boundaries/RGR2 cache validation and offline auditing. |
| Mixed 512 KiB / 1 MiB resource decoder | **2,607,962 checks passed**: internal slot holes, index remapping, memory/ranged/incremental reads, bounded room workspace, late cancellation and malformed-hole rejection. |
| Final WebAssembly build, runtime 090023d | Passed; **7,516,049 B**. |
| Final all-language tile/merchant conversion | Passed at 171f30b; 159.226 s and 100.743 s, source unchanged, complete raw selected-group comparisons and all locale/catalog invariants. |
| Final versus measured device identity | Every shared file and English companion, and the 69,923,632-B English logical pack, match the actually measured device candidate exactly. |
| Complete accepted-final 44 versus complete UI18/42 image oracle | **58,060/58,060 images, 1,167,930,140/1,167,930,140 decoded bytes**, all memory/ranged/incremental paths byte-identical; 20 contiguous intervals, 547.541 s, source unchanged. Maximum spool 67,108,736 B, below 64 MiB. |
| Final all-sixteen-locale gate | **89,790 checks / 53,072 descriptions / zero failures**, source unchanged. |
| Final actual offline HTML | **107,211,281-B HTML** contains byte-identical WASM, joined common resources and ENG/ZHS streams; both logical packs validate and English equals the exact full-image/device input. Local source notices verified; no published GitHub URL is claimed. |
| Final release ZIP readback | Every member of calculator, optional-language and offline ZIPs read back with matching sizes/hashes. Calculator has an empty state/ and no save files; other archives contain no state. Distribution byte counts appear below. |

The image comparison preserves the existing baked resource catalogue and its
RGB565 outputs; it is not a claim that CPU rendering duplicates every original
GPU material. The catalogue retains 1,783 clips, 13,299 English glyphs and 8,704 strings.
The final complete-image receipt has SHA-256
`e8f056ac65f3e655902b6249e75b3b2ebd93a0d4cb9ffe5e86fb63c453893ed9`.

Important final journals:

- ui19-final-release-arm-633f454a09d8
- ui19-final-native-suite-a482d929945f
- ui19-cache-aware-final-python-suite-e61e79f7635d
- ui19-final-web-build-b9030f8e3390
- ui19-refined59-final-all16-1bf452858700
- ui19-refined-rgr2-final-all16-d51cf6d5e64e
- ui19-refined-final-manifest-c43c8bacfb13
- ui19-corrected44-final-all-images-a3d6e468f4d9
- ui19-corrected44-final-all16-locales-1194d77dee25
- ui19-corrected44-offline-html-readback-b59b99a08d0e
- ui19-refined-final-offline-embedded-60b837e6d686
- ui19-corrected44-player-zip-readback-5b9d1b655566

The final embedded-readback receipt has SHA-256
`f9352f8da3d94d9bb8458c21cdee6dab28725fd2f6432f5e4607142950dfc842`.
Two independently started HTML audits produced that same receipt; this is
recorded as duplicate byte verification, not additional image or gameplay
coverage. The final ZIP/member-readback receipt has SHA-256
`1b85a702fc4fceaafdf2682783a8e7e1698cf4d8caf7b747d47afc9f32375944`.

| Actual distribution artifact | File bytes | SHA-256 |
| --- | ---: | --- |
| Bilingual calculator ZIP | 73,726,639 | `26b0130169791eaf0d4a955a0c7f0864ac9c494ae502d2f6edceb6c99ce6954c` |
| Bilingual offline ZIP | 74,584,713 | `16c6cd910a97ff76494480dd2793135a89304d605529acfdb5333f1970c2116f` |
| Optional fourteen-language ZIP | 17,022,494 | `aff1cdd7d3bdefd8750db12cd55544dd74293c2c8ad2f084603624a6ae356597` |
| Standalone bilingual offline HTML | 107,211,281 | `5100395d029e98ce66315c12b79be9f43a34a51bf4a0333c8c52850d5d64dd13` |

These distribution containers are separate from the installed program/data
figures above. Historical all-paired byte proofs and packages remain labelled
as superseded, not reused as the accepted final artifact's validation.
Failed and superseded attempts remain in the evidence, including converter/build
timeouts, temporary disk exhaustion, an outdated popover assertion, absent lz4,
diagnostic fixture/protocol failures and ordinary setup attempts that never
entered the game. They are not relabelled as successful acceptance runs.

## Ordinary Firebird acceptance and measured performance

All resource-format timing comparisons use the same ordinary runtime 090023d on
an unchanged Firebird CX II CAS / OS 6.4.0.74 / 396 MHz / Ndless r2022 environment.
The final common files and English companion read back byte-exactly over USB;
adding the other fifteen locale companions did not change those tested bytes.
These are emulator observations, not physical-handheld FPS measurements.

| Scenario | Original 42 FPS | Rejected all-paired 44 FPS | Accepted final 44 FPS | Original / accepted maximum frame |
| --- | ---: | ---: | ---: | ---: |
| Bowlbugs and one enemy turn | 27.4491 | 17.3680 | 27.2301 | 439.026 / 491.577 ms |
| Decimillipede and one enemy turn | 29.4261 | 29.3967 | 29.4265 | 406.433 / 405.212 ms |
| Merchant, first final sample | 28.0458 | 25.7294 | 27.3321 | 1,213.257 / 1,322.144 ms |
| Merchant, same-protocol repeat | Same single baseline | Same rejected sample | 27.4347 | 1,213.257 / 1,345.398 ms |
| Construct first charge/burrow | No device baseline | No device sample | 28.8794 | No baseline / 395.691 ms |

The severe Bowlbug regression is removed: requested scene bytes fall from the
rejected candidate's 12,343,303 B to the final 1,615,991 B, compared with
1,455,451 B originally. Its final FPS is still 0.80% below the single original
sample, with a 52.55-ms higher maximum. Construct's ordinary states were checked,
and its host first-turn full-group decode work returns from 32,503,205 to exactly
the original 7,776,501 raw bytes; a missing 42 device FPS sample is not invented.

Merchant's two final samples total 2,232 frames over 81.509 seconds:
**27.3834 pooled FPS, 2.36% below** the one 28.0458-FPS baseline sample. The final
cold maxima remain 1.322/1.345 seconds versus 1.213 seconds originally. Requested
bytes decrease, but measured provider/seek/read time is higher. Both samples
and their residual are retained; they are not dismissed as noise or described
as zero regression. Resource-read timing can be nested in update/render timing,
so those fields must not be summed as independent elapsed categories.

The matched warm flow immediately reopens after a real Save and Quit without
USB operations between processes. Warm combat is **29.9819 FPS** versus 29.9821,
with zero resource reads in both. Character selection averages **21.8398 FPS**
versus 21.8565; its maximum rises from 736.603 to 797.241 ms. Warm preparation
total/max changes from 3,039.246/250.610 to 3,029.968/189.789 ms. Startup's measured
resource-validation/App-init scope changes from 753.967 to 680.176 ms; it excludes
OS boot and loader work before game main. No universal 30-FPS or fresh-cold-boot
speedup is claimed.

Every final selected ordinary scene had zero resource failures and was visually
reviewed. Functional checks include a resumed Cosmic Indifference choice flying
to Draw, fade-out/fade-in around Save and Quit, persistent pile counts after
ordinary relaunch, Osty's position/trait, actual enemy-caused terminal save
retirement, and a forged/enhanced Sovereign Blade tooltip with paused/resumed
orbit. Controlled production-codec fixtures are not represented as a naturally
completed three-act playthrough.

[Ordinary device acceptance](ui19-device-storage.md) records all controls,
raw-performance hashes, selected LCD captures and exclusions. Its final
`final-pair-summary.json` SHA-256 is
`c5ecec5dc1353457c17e08b06e7b47378365dec3268df444ad2ca863bf866961`.
Normal Ndless installation recovery and OS focus/USB failures are documented;
no patched emulator core, RAM, special launcher or test runner was substituted.


## Future original-game updates

The existing extraction, baking and regeneration architecture is reusable.
The executed order is documented in [content regeneration](content-regeneration.md):
matched local original install/PCK/assemblies/release metadata/fonts, extraction
and baking, world/UI additions, locale/font/metadata construction, compact and
aligned resource layout, retained-plane transform and accepted merges, tile
mixed grouping, semantic merchant bounds, multipart relocation, full validation
and release packaging.

`tools/build_content.py` still ends at its historical 0.16 stage. It is not a
single command that independently reconstructs every current UI19 addition.
The repository now records the actual sequence and supports the new format
through converters, readers, splitters, alignment and packaging tools. The final
storage chain is complete 42 → RGR3/43 → two retained passes → mixed tiles/44 →
bounded merchant groups. Preserve the same-generation 42 English logical pack
and first-merged 43 English pack until the final merchant transform completes;
its full raw/catalog comparisons reject a reference from the wrong build.
The semantic tile policy rederives group protection from model/animation names
and checks profile/header agreement rather than trusting old physical group IDs.

A future update can change enums, IDs, asset paths, schemas, timing, balance or
mechanics. Update the version pin and adapters/audits, port changed behavior,
then regenerate and rerun catalogue/locale and ordinary Firebird validation.
Replacing a PCK alone does not implement changed game rules. Private original
inputs remain local; public CI has no complete licensed input set and cannot
rebuild them independently. Encoded compression streams may be reused only when
their raw-content identity matches; old physical offsets are rebuilt.

## Installation and remaining boundaries

Exit the game before updating. Keep the existing **state/** directory. Remove
old game-resource files from **data/** before transferring the complete matching
new resource set and launcher, so the calculator does not temporarily hold old
and new large parts simultaneously. Install every required artwork part/index
and only the selected languages. Start the game after all files have arrived.
The package contains no test profiles, preferences, saves or performance markers.

The accepted static full Ancient scene and excluded audio, multiplayer and
Timeline remain. CPU/RGB565 material approximation, cosmetic RNG differences,
finite-history/input capacities and RTC/timezone boundaries remain disclosed in
[scope metadata](../config/product-scope.json); they are not described as full
original-game parity or as blanket user-approved exceptions.

No manual browser playthrough or physical-calculator FPS claim is made in this
release. Web validation covers compilation, automated host behavior, resource
reconstruction, embedded bytes and archive integrity.

## Source delivery and publication block

The artifacts include a complete tracked source snapshot, full local Git-history
bundle, identity/restore instructions and the acceptance evidence. The source
snapshot and history are checked for original resource/ROM/NAND binaries; private
assets and generated game packs are not included in the source archive.

The attempted public GitHub push was rejected by automatic approval review as
potentially sensitive game-derived source/history disclosure; the then-proposed
push contained 178 accumulated commits. This was not bypassed through another
transport. The remote refs actually read in this turn were b84801b3. Subsequent
local work is preserved in the source archive, and the final source manifest
records the current tip and accumulated commit count separately.

Publishing the concrete source/history payload to the public development branch
requires the specifically requested public-disclosure approval. All local
implementation, verification and private deliverables are completed independently
of that blocked action. No original assets, assemblies, fonts, resource packs,
ROM/NAND or test saves are proposed for public upload.
