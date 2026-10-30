# UI18 calculator delivery — 0.17

Reference remains the owner's **v0.111.0 / 41cef1ea** installation. This stage
continues the complete UI17 source and resources, rather than the older public
UI14 checkout. Original UI and readable 320×240 presentation remain the
priority; the selected changes preserve the existing complete image and frame
catalog and add the missing locale calendar behavior.

## Frozen identities

| Item | Identity |
|---|---|
| Tested production runtime | `44d7fb79f7d87d522038a074fa80d25e2bdeff31` |
| Resource conversion | `3acb5b310d2d2cb42dbf98f21a79f0b45d8b42ed` |
| Final integrated native/ARM gate | `44d7fb79f7d87d522038a074fa80d25e2bdeff31` |
| Full Python gate | `299ee0085097cc281e1745a67dedcf36f395482c` |
| Final source/documentation/history | Recorded separately in `SOURCE-IDENTITY.json` |
| Launcher SHA-256 | `7d34c4163a61c5f4984915d08272c42a3b4b69e003ddb1c388826d21d9098be7` |

The later source changes are tools, tests, exact locale-binding audit data and
documentation. The final source archive verifies that `src/`, `include/` and
`platform/` are byte-identical to the tested production runtime. Component
tests and the final integrated gates retain their own checkpoints.

## Actual installed storage and allocation

| Program and installed resources, before saves | UI17 bytes | UI18 bytes | Saved bytes |
|---|---:|---:|---:|
| Launcher | 1,766,579 | **1,719,223** | **47,356** |
| Common index and eight artwork parts | 72,957,060 | **72,727,684** | **229,376** |
| Default bilingual data, including shared artwork | 78,190,434 | **77,962,451** | **227,983** |
| Default bilingual program and data | 79,957,013 | **79,681,674** | **275,339** |
| All sixteen languages, program and data | 98,499,241 | **98,226,131** | **273,110** |

The default is **75.990366 MiB**, a **0.34436%** installed reduction against
UI17. In the default installation, compressed tile streams, retained images
and frame-index streams account for 90.07% of bytes. The common part/index
wrappers total only 1,776 bytes. The detailed component accounting and rejected
compression candidates are in the storage record; their results do not prove
a mathematical lower bound for every possible lossless codec.

Installed sizes count the calculator program and resource files, before saves.
The complete archive also includes installation instructions and licenses;
those expanded document bytes are accounted for separately in its receipt.
ZIP compression is not an installed-size measurement. The earlier **65–70 MB**
default target remains unmet, and no additional image, frame, glyph or string
has been removed to meet it.

The launcher requests **22,289,380 bytes** through Zehn, including **157,612
metadata bytes**. Execution/static memory is **22,131,768 bytes**, below the
unchanged 22,450,000-byte execution limit. The complete **13,369,344-byte image
pool** remains. Total allocation is 19,688 bytes above UI17 after the new
preparation and calendar features; this is a build-time allocation request,
not physical peak RAM or an arbitrary fragmented-heap guarantee.

### Implemented storage work

- NSP42 removes unused local tile slots inside existing groups and redirects
  exact duplicate tiles only when doing so introduces no additional group for
  the consuming frame. Global group IDs and the 680-slot logical stride stay
  stable. Short groups are checked against their actual item count.
- The converter preserves complete tiles, including padded edge values and
  hidden RGB. It selects physically smaller results after counting rewritten
  TGI2 indices, locale wrappers, alignment and multipart spans.
- A first tightly packed result changed the retained suffix's 4 KiB page
  phase and increased English reads. The selected converter restores that
  phase algorithmically. Its measured 2,624-byte padding is included in the
  delivered size; the original failed layout remains recorded.
- Release-only Zopfli packaging saves **61,696 bytes** against level-9 zlib
  for the same final executable image. The calculator decoder, loaded image,
  relocations and allocation request remain identical. Safe ICF is enabled;
  the broader `--icf=all` mode is not used.

The 87,440,430-byte reduction in expanded tile records is **not** the installed
saving. Most catalog-page reductions are in groups that were already unused.
Only four nonempty groups lose a total of five decoded pages. No whole-game
cache or FPS improvement is inferred from that raw count. See the
[storage record](ui18-storage-0.17.md) and
[launcher measurements](ui18-launcher-compression-0.17.md).

The proposed static-App-to-BSS initialization change was rejected because its
real launcher grew by 12,039 bytes and allocation by 35,844 bytes. The exact
comparison and unexecuted-test boundary remain in the
[rejected-candidate record](ui18-app-bss-rejected-0.17.md).

## Implemented runtime and original-behavior changes

### Incremental cold preparation

Actual tile index, tile stream and image reconstruction work now yields in
bounded steps. The implementation uses a 4 KiB decode-output step, bounded
input and the existing cache arena. Pending work, publication and pins are
kept separate; cancellation and failures release unfinished work. It retains
the four artwork-file streams and the full picture pool.

The App prepares the actual current actors, HUD, hand images and fitted text
before presenting the combat view. The font visitor uses the same measured
size and face as visible drawing. It does not discover cards, advance label
clocks, consume gameplay RNG or run a hidden full combat render. Visible play
keeps its existing 30 Hz pacing; the black preparation phase resumes sooner.
The time budget is not a hard upper bound on an individual OS operation.

The final App also queues the common card-flight glow when the combat card
registry is nonempty,
and the Colorless Skill frame when an actually displayed enemy intent announces
a status action. These were two real first-use image reads in the ordinary
target action window. They use the existing deduplicated preparation queue,
budgets and cancellation/failure paths, without enlarging a pool or opening
another resource stream. They neither predict a future gameplay state nor
consume RNG. Queue saturation, a combat without registered cards and later newly revealed
intents can still require demand loading. The final two-image change passes
824,868 focused native checks and the same scoped ASan/UBSan checks. English
and Chinese first-visible combat plus 182-frame action replays match the
previous integrated runtime's complete RGB565 and canonical State bytes.

### Exact coverage arithmetic

Coverage blending replaces three bounded integer divisions with one small
reciprocal table and exact correction. It passes **273,597,054** arithmetic
comparisons and actual Canvas comparisons. The scoped native operation
benchmark changes from 158.602872 to 69.635448 ms; this is neither complete
frame time nor calculator FPS. The slower opaque-blend factoring experiment
was withdrawn. See [renderer evidence](ui18-rendering-0.17.md).

### All sixteen original locale calendars

History dates and times now use the original locale's format/provider rules;
daily challenges retain the original invariant-provider behavior. Examples
include Thai Buddhist history years, Polish/Russian month forms, Japanese
periods and the Spanish and Indonesian punctuation rules. Thai daily dates
still use the original Gregorian year and invariant month names.

Fourteen exact 8 px glyphs were added from the original fonts, including the
shaped Thai clusters. Older language packs lacking these glyphs get readable
English date output. The independent original-method oracle covers 7,136
rows, while the final actual-resource UI gate covers **16 × 445** history and
daily cases with exact source glyphs. The RTC/UTC adapter remains separate
from a desktop timezone database. See [calendar implementation](calendar-locales-ui18-0.17.md).

## Resolution of the nine reported items

These are retained implementations and source-confirmed decisions, not nine
new UI18 features. The complete current native suite reruns their regressions;
the new resource comparisons preserve their rendered content.

| Feedback | Current behavior |
|---|---|
| Oversized common file | Small `nSpire2.dat.tns` index plus eight concern files: interface, cards, characters, enemies, bosses, rooms, effects and shared. Locale files remain separately optional. |
| Default languages and English recovery | Valid saved selection, then supported installed system language, then installed English, then another usable edition. Unsupported calculator languages choose English when available. Built-in technical errors use English independently of game fonts. |
| View Upgrades checkbox | Original pointer-only preview control. Directional input hides it and resets the preview. It is not a newly introduced radio choice. |
| Tiny deck filters | Four sort labels use 8 px in an 18 px strip, with matched measurement and drawing. |
| Tucked card behind the eye | Eye `(5,108)`; card final center `(18,78)`, with the existing half scale and 250 ms transition. |
| Mixed font families | Seven substituted locales use locale Regular for normal labels; the other nine retain authored scene faces. Explicit rich-text bold/italic keep their original roles. Chinese Sans normal text and Serif emphasis intentionally coexist. |
| Tablet of Truth vs Smith | Tablet uses the original Event/Messy upgrade layout and staggering; Smith uses its distinct hammer, impacts and sparks. |
| Map icon size | Ordinary/focused nodes remain 20/29 px, using the full authored atlas canvas and its transparent margins. Boss/start nodes retain separate geometry. |
| Treasure text below ribbon | The title remains a single 10 px line at `(117,62)` in an 87 px box, following the ribbon's animation; long text scrolls inside it. |

The exact original methods/scenes and concrete source paths are in the
[feedback ledger](ui17-feedback-acceptance-0.17.md). Earlier missing monster
command programs, Vantom's root transition and independent tail, Smith
effects, healing/upgrade notifications and secondary card trails were already
implemented in UI15 and remain present. UI17's Unicode/raw legacy seed
implementation is retained. Historical omission lists must be read with
these superseding records.

The seven font-substitution locales are Simplified/Traditional Chinese,
Japanese, Korean, Polish, Russian and Thai. `source_label_face()` follows the
locale substitution flag for ordinary labels; other locales retain the scene's
authored Kreon Regular/Bold, Spectral Bold or Bitter Medium Italic face. In
Chinese the normal face is Noto Sans Mono CJK, while the rich-text emphasis
roles use Source Han Serif. That family difference is an original role
distinction, not an instruction to make every title Serif. Japanese uses its
Noto Sans CJK roles; Korean uses Gyeonggi Cheonnyeon Batang Bold, Polish and
Russian use Fira Sans Extra Condensed roles, and Thai uses CS ChatThai UI.
The exact source methods, scene mapping and original-font acceptance are in
[the font record](ui16-language-fonts-0.17.md). UI18 preserves the existing
glyph masks and metrics and adds only the fourteen calendar glyphs described above.

## Executed verification

| Gate | Result |
|---|---|
| Final native suites | **157/157**, `ui18-final-warmup-native-suite-e8bea92722e6` |
| Final converter/tool tests | **345/345**, `ui18-final-python-suite-451ac753ab9c` |
| Ordinary ARM | `ui18-final-ordinary-arm-86e313858b62`, selected 44d7 launcher |
| Final full image/metadata comparison | **58,060 images / 1,167,930,140 decoded bytes**, `ui18-final-fullresource-preload-a80f9b814cf5` |
| Actual final locales and calendar UI | **89,814 locale checks / 53,072 descriptions** and **831,111 calendar checks**, `ui18-final-all16-calendar-4456ddbbb943` |
| Complete provider replay | English/Chinese, two providers, **728 full frames**, exact RGB565 and canonical State |
| New tile/App ASan + UBSan | **1,580,418** Content checks and **796,473** App checks; instrumentation scope in the component report |
| Final transition-image preparation | **824,868** native and scoped ASan/UBSan checks; complete bilingual frame/State comparisons |
| Calculator archives | Complete member readback and matching frozen-file hashes |

The full image gate checks memory, ranged and incremental decoding against the
independent UI17 reference. It also checks 13,299 glyph records, 7,124 kerning
pairs, 56,019 exact-size samples, 8,704 strings, 1,783 clips and 40 complete UI
frames. Incremental loading takes 18,622,022 steps over the entire catalog,
with a maximum of 11,093 steps for one image. After `Ready`, retrieving the
image causes **no further provider read**. The 244,829,205 comparisons are
checks within this gate, not that many distinct gameplay scenarios.

All listed final gates completed with unchanged source inputs. The first
native build's 600-second timeout and the first full Python run's Japanese
literal-binding failure are preserved. The build resumed without discarding
completed objects; the exact missing bindings were implemented, then all
tests passed. Scoped sanitizer results do not claim whole-core instrumentation
or LeakSanitizer support in this environment.

The image/incremental and actual sixteen-language gates ran at `e84f3ef`,
using the final frozen resource files. Later production changes only add the
two App image-preparation requests above; Content, Canvas, locale/calendar
formatting and the resource files are unchanged. The 728-frame storage replay
uses the storage component core at `305c1b7`; the later final-preparation replay
at `feb344b` separately covers complete bilingual combat output. Its production
`src/`, `include/` and `platform/` trees are identical to final integration
`44d7fb7`. Gates keep these separate source identities rather than being
relabeled as a single unexecuted final run.

## Ordinary-OS target acceptance

The selected final launcher is the ordinary release build, with no developer
I/O trace flag. The accepted final preparation change was measured against
the previous integrated launcher after loading the **same frozen NAND and
installation snapshot**, explicitly transferring the corresponding ordinary
launcher on both sides and replaying identical inputs.

| Matched final-change comparison | Before the two-image preparation | Final UI18 |
|---|---:|---:|
| Continue to complete HP / End Turn controls | 5,460 ms | **5,480 ms** |
| Maximum preparation frame interval | 6,308 ticks / 192.505 ms | **6,309 ticks / 192.535 ms** |
| Preparation frame intervals over 250 ms | 0 | **0** |
| Cold resource elapsed time | 73,159 ticks | **73,292 ticks** |
| Fixed warm window frames / elapsed ticks | 169 / 197,605 | **170 / 196,892** |
| Fixed warm window FPS | 28.02455 | **28.29246** |
| Warm resource elapsed time | 9,883 ticks / 301.605 ms | **7,754 ticks / 236.633 ms** |
| Warm resource read requests / bytes / seeks | 18 / 105,199 / 18 | **15 / 91,338 / 15** |

The final change removes three warm resource read requests and about **64.972 ms** of resource
elapsed time, while total cold waiting grows by **20 ms**. Its warm-window
FPS rises **0.956%** within this matched pair. These figures are not presented
as a complete UI17-to-UI18 speedup: the initial ordinary UI17 control measured
5,530 ms to the complete controls, a 37,005-tick / **1,129.303 ms** maximum
preparation frame, and **28.45571 FPS** over 171 frames / 196,914 ticks. Final
UI18's longest preparation interval is about **82.95% shorter** in this
fixture, and its total wait is 50 ms shorter, but the warm-window FPS remains
**0.5737% lower**. There is no claim of universal 30 FPS or that the entire
warm-window difference has disappeared.

The intermediate new-data warm regression remains recorded. A diagnostic-only
I/O trace localized approximately 90.6 ms to changed latency of a few physical
OS reads, despite identical interface-file bytes and identical interface
request shapes. The precise OS cache/file-placement mechanism is unproven.
That diagnostic launcher is excluded from release and performance acceptance;
the final change and timings above come from actual ordinary builds.

Ordinary target lifecycle acceptance performs End Turn, Save & Quit, OS exit,
immediate reopen without another USB transfer, and Continue. The resulting
7,320-byte compressed run save is byte-identical to the prior integrated
launcher's same-action save. First complete combat, Weak, the five Dazed cards
and the resumed state are also inspected in the actual LCD output.

System-language checks include Simplified Chinese, French with a no-USB reopen,
and unsupported Dutch falling back to English. Final language/font directories
are the same frozen sixteen-language files used by the final full locale gate.
Technical errors remain built-in English even when French was requested.
The native boot suite also checks all 65,536 installed-language subsets,
saved/system/English/remaining-edition ordering, failure of successive language
candidates, supported locale aliases and unsupported tags including Dutch,
Swedish, Danish, Finnish, Norwegian, Arabic and Czech. It compares all built-in
technical-error text and rendered error frames against the English version.
These system-language and injected-resource-failure observations used the
ordinary `a7e3e94` runtime with final aligned data. The final `feb344b` / `44d7fb7`
change only adds the two App artwork-preparation requests; it does not change
locale selection or the failure guard. The final cold/warm pair and save/reopen
lifecycle were executed separately on the final ordinary launcher.

The early missing-shared-part case preserves the complete run and progress
files byte-for-byte through failure and a no-USB reopen. In the late corrupted
enemy-header case, the run is unchanged, and decoded progress differs only in
its four-byte `last_saved_at` timestamp: the legitimate Continue/room-entry
persistence occurred **before** the deferred bad header was first read. Its
progress envelope also changes from raw to compressed. This is not a claim
that the whole late-failure launch performs no writes; the failure latch blocks
subsequent callbacks once the error is detected. The original overbroad
whole-file assertion failure is preserved alongside the canonical review.

An Escape observation during the original character-departure transition did
not open the pause menu, because that transition intentionally gates input.
It is not accepted as evidence of preparation-menu responsiveness. Focused
native coverage exercises cancellation and pause outside that gate. Further
device details, original failed attempts and screenshots are in the separate
[ordinary-OS report](ui18-cold-preparation-0.17.md) and evidence archive.

The target uses unchanged upstream Firebird core
`b10f3b51a9ab8e27d2703444b5e6bd278d4c5e6e`, CX II CAS / 396 MHz /
OS 6.4.0.74 / Ndless r2022. Warm action windows and cold preparation are
reported separately. Per-scene maximum frame interval is not a direct
measurement of the longest gap between input-poll callbacks. A 10 ms LCD
observation of complete HP/End Turn controls is not an instruction-level input
enable timestamp. Full-image/frame equality and target elapsed time have
different evidence.

## Install and preserve existing saves

Install `nSpire2.tns`, the small `data/nSpire2.dat.tns` index, **all eight**
matching artwork parts, English and the desired language files. The default
ZIP contains English and Simplified Chinese; the optional ZIP provides the
other fourteen original editions. English should remain for fallback. All
artwork concerns are required even when only one language is installed.

Replace the program and matching data together and preserve the complete
existing `state` directory. The delivered archive contains no test saves,
language preferences, profile markers or performance output. When replacing
an old monolithic common file, replacing it with the small index first releases
its space before transferring the other parts. Finish the matching transfer
before launching.

The developer console remains included in the normal launcher. Touchpad,
keyboard controls and two remappable input banks retain their existing
behavior. The original intro remains skippable; its duration is included only
where a complete-launch observation explicitly says so.

## Scope and persistence

The agreed static full Ancient scene exception and excluded audio,
multiplayer and Timeline remain. RGB565/LDR CPU rendering, sampled GPU
materials, bounded cosmetic history, offline service behavior, RTC/UTC and
finite history capacity remain explicitly documented boundaries. These are
retained platform differences, not blanket user-approved parity exceptions.
In particular, the current seed field is bounded to 64 UTF-8 bytes with no new
calculator IME, and the RTC adapter does not provide a desktop timezone/DST
database. Progress keeps the existing 96-floor/32-run bounds and finite journal
and drawing capacities; offline services do not reproduce hosted leaderboards
or cloud behavior. This stage
does not establish physical-handheld FPS, universal 30 FPS, zero cold stalls,
natural three-act victory, exhaustive Godot equivalence or measured 95% UI
restoration.

The calculator delivery consists of the **78,866,855-byte** bilingual ZIP and
**17,075,088-byte** optional-fourteen-language ZIP. Complete ZIP member
readback, frozen file hashes, notices and empty mutable state are recorded in
`calculator-final-receipt.json`. Its accepted status binds the final 157-suite
native gate and ordinary ARM build. Including all installation documents and
licenses, expanded archive bytes are 79,751,780 for the default package and
98,297,072 with all optional languages; these are separate from program/data
installed totals above.

The separate source ZIP contains the final tracked tree, complete local Git
history/ref bundle, exact source/runtime/resource identity, original passed
and failed verification journals, and recovery/audit scripts. Its own
`SOURCE-IDENTITY.json` and recovery receipt record the final documentation tip
and complete mirror restoration check. It excludes original game inputs,
fonts, device images, saves and compiled player data. The source identity also
binds the two player ZIPs and the separate device evidence ZIP by exact hash.

The **9,146,827-byte** device evidence ZIP contains 621 verified members:
ordinary LCD screenshots, profiles, source-identified journals and reproduction
scripts. Private NAND/RAM/ROM, original game packs/fonts, save/preferences
payloads, launchers and complete native RGB565/State payloads are excluded;
necessary identities remain as metadata. The failed and slower measurements
are retained, not removed from the record.

The final local development branch and local `main` are advanced to the
complete source/documentation tip after acceptance. The earlier automatic
approval review rejected outbound source/history publication; this stage does
not bypass that rejection or claim a new public GitHub update or remote CI
pass. Calculator-file saving and outbound publication receipts are reported
separately from these local build, installation and recovery results.
