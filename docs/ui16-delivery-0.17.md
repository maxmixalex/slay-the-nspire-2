# UI16 calculator delivery and acceptance — 2026-10-29

Reference: the owner's pinned **v0.111.0 / 41cef1ea**. Development starts from
the recovered, selected UI15 source and resources, including the UI15 history
bundle. It does not restart from the older public checkout. Original PCK,
assembly, fonts, NAND and derived resource packs remain private.

This ledger separates implementation, converter checks, native execution and
ordinary-OS Firebird acceptance. A probe, a compiled target, an asset count or
a historical test result is not a newly completed gameplay feature.

## Selected implementation and resource identity

The integrated candidate is runtime
`def1db3c68d3730edfddb4985feb403cc86b256b`. Its launcher is 1,759,300 bytes,
SHA-256 `6698dc8e4fb02589ce810a9b1c5bf565601bcdc88b71982457648a07be0f5a3a`.
The converter checkpoint is `ee1074d79b7eba3217b4ff7e71511b47df79b019`, with
the exact optional Portuguese/Turkish source-glyph repairs recorded in
`ui16-language-fonts-0.17.md`. Resource semantics remain NSP2/40. The new
physical wrappers are NSM1/NSS1 artwork and NSL3 language pages, distinct from
the existing NSZ1 save/history envelope.

The final native build at this runtime passes **146/146 suites**, with no
source change during its logged run. The final ARM build passes the total
and executable/BSS memory budgets. A further isolated hand-reader allocation
candidate is being evaluated against the observed warm-OS allocation boundary;
it is not yet part of this identity or the savings below.

## Implemented feedback

| Requested concern | Delivered behavior and reference decision |
| --- | --- |
| One oversized common file | A 1,616-byte index and eight required files for interface, cards, characters, enemies, bosses, rooms, effects and shared groups. The old logical artwork bytes reconstruct exactly. |
| Default languages and failures | Saved explicit preference takes priority. Failed candidates are removed for the current session; system language, English and the first remaining usable edition are tried in order. Unsupported calculator locales map to English when installed. Startup and technical failure text is built-in English. |
| Upgrade checkbox | Original pointer-only View Upgrades control retained. Directional navigation hides it and resets the preview. |
| Tiny deck sort labels | 6 px text becomes 8 px; the strip and hit rectangles grow, while original ordering, arrows and long-label scrolling remain. |
| Tucked card and eye | The card center moves from (31,88) to (18,78), sharing layout constants with the moved eye. The original 250 ms cubic transition and half scale remain. |
| Mixed font families | Original normal-label substitution and explicit rich bold/italic roles now apply consistently to the audited controls in all 16 editions. Chinese normal Sans and rich Serif emphasis are an intentional original distinction. |
| Tablet versus Smith upgrade effect | The original uses EventLayout/MessyLayout for Tablet and a separate Smith VFX. Their deliberate difference is retained. |
| Map icons looking smaller | The original transparent atlas canvas is preserved. Normal/hover controls remain 20/29 px, matching the source projection; cropped ink is not stretched to fill the control. |
| Treasure heading below ribbon | The title now uses the authored banner's projected inner label rectangle, a single 10 px line and bounded horizontal scrolling. |

See `ui16-ui-feedback.md` and `ui16-language-fonts-0.17.md` for original
class/scene evidence, exact geometry and executed tests.

## Actual installed storage

All values below exclude mutable saves and explanatory/license text. They count
the files installed on the calculator, not the ZIP download.

| Configuration | UI15 bytes | Integrated UI16 bytes | Net saving |
| --- | ---: | ---: | ---: |
| Default English/Chinese resources | 80,155,768 | 79,262,359 | 893,409 |
| Default resources and launcher | 81,910,800 | 81,021,659 | 889,141 (1.0855%) |
| All 16 language resources | 104,025,419 | 97,814,267 | 6,211,152 |
| All 16 languages and launcher | 105,780,451 | 99,573,567 | 6,206,884 (5.8677%) |

The default installation is **81.021659 MB / 77.268275 MiB**. Splitting artwork
adds 1,808 bytes; it does not itself provide compression. Independent 4 KB
language pages supply the storage reduction. The optional glyph repairs add
468 bytes against the original UI15 language inputs, already accounted for
above. The launcher grows by 4,268 bytes. No existing texture pixel, animation
frame, string or valid glyph is removed by this pass.

The prior 30–40 MB and later roughly 65–70 MB installation targets are **not
achieved**. The approximately 74 MB artwork representation remains the dominant
cost. Sampled predictors, byte planes, palette variants and further codec
profiles did not justify a faster/smaller production replacement. A smaller
ZIP alone would not improve installed storage. The old GPU-to-LCD conversion
and its disclosed temporal sampling are separate from this byte-exact
container repacking.

`ui16-storage-0.17.md` lists every installed concern file. All eight parts and
the index must be installed together from the same edition; they are not
optional content packs. Additional language files are optional.

## Runtime and memory changes

Opaque coverage pixels use the exact RGB565 source-over result without three
variable divisions. Unshaded affine cards use a direct loop while preserving
the original coordinates, clipping, alpha and layered effects. ARM926 bulk
memory operations use aligned words with safe byte heads/tails; short and
differently aligned calls retain their documented small dispatch overhead.

The saved title preview and synchronous console rollback share one owned State
workspace. Random's gradient/light tables and the merchant's rug share a
mutually exclusive raster workspace. Their measured complete-frame, canonical
save and I/O comparisons preserve behavior. No additional image-pool eviction
is used: the **13,369,344-byte hot picture pool stays intact**.

The integrated Zehn allocation is **22,568,868 bytes**, 311,028 bytes below
UI15 after accounting for new code and storage adapters. Its 157,048 bytes of
metadata leave a **22,411,820-byte executable/BSS request**. The build checks
both the existing 24 MiB total ceiling and a 22,450,000-byte executable/BSS
ceiling. Neither ceiling guarantees an arbitrarily fragmented OS heap.

The eight logical artwork providers use at most four open artwork streams.
Startup probes lengths sequentially and then closes the resident index. The
LRU evicts before opening, owns independent seek cursors and invalidates them
on failures. Locale and state streams remain separate. Cold open/close costs
are included in resource timing, and their input edges are collected through
the ordinary foreground hooks. This is an actual device-open failure fix,
not an extra diagnostic-only path.

Detailed contracts and measurements are in `performance-ui16-0.17.md`,
`ui16-scene-raster-memory-0.17.md` and `ui16-artwork-file-handles-0.17.md`.

## Executed correctness gates

| Gate | Executed result and scope |
| --- | --- |
| Final complete native build/test | 146/146, runtime def1db3; `ui16-final-native-7a70d2f91d93` |
| Final ARM build | Passed; `ui16-final-arm-78c95353b257` |
| Default-language model | 137,139 checks, 43 system tags and every one of 65,536 installed masks |
| Final NSM1/NSL3 editions | 89,802 checks and 53,072 formatted descriptions across 16 locales; 90 injected bad-selected-language recoveries render English |
| Font inventory | 3,166 exact font/size/codepoint requirements; 48/48 original family sample rasters |
| Storage adapters | 13,270 synthetic checks; actual English/Chinese 15,680/15,814 checks and full logical-byte comparison; changed transfer workspace remains untouched |
| Storage sanitizer checks | Actual English and Chinese pass ASan/UBSan; LeakSanitizer alone disabled for executor ptrace |
| Four-slot artwork cache | 982 native and 982 ASan/UBSan checks, eight logical parts, retry/failure/reused-address cases and a seven-stream host ceiling |
| Shared State | Six relevant suites, actual English/Chinese 246 checks each; 738 matching complete frames, canonical states and resource counters |
| Random/merchant raster sharing | 18 byte-identical complete frames, 2,764,800 compared bytes; actual Random 4,899,904 checks per language and foreground input 66,021 |
| ARM memory operations | 4,799 vectors per linked launcher version; separate actual object 9,908 including the unused-in-launcher memcmp |
| UI feedback | Actual English/Chinese deck, map, card/HUD, room and world paths; original Tablet/Smith and monster command traversals remain implemented |
| Package readback | Correct-identity receipt `ui16-package-verified-identity-63512909f467`; every ZIP member is compared with its input and both logical default packs reconstruct |

The full native suite's synthetic cache/locale tests do not imply that every
suite individually loads proprietary assets. The separately named actual-pack
gates above cover those paths. Sanitizer results refer to their named targets,
not an unexecuted whole-program sanitizer sweep.

Earlier test-only compile errors, source-changed provisional journals and
generated-ELF execute-bit launch failures remain in the private history. The
final complete run normalizes only generated ELF owner-execute permissions
before CTest and passes all 146 tests. A provisional package receipt with a
mistyped checkpoint is superseded by the correct-identity receipt above; its
result is not used as final source-identity evidence.

## Ordinary-OS Firebird acceptance

The restored calculator configuration is **CX II CAS, 396 MHz, OS 6.4.0.74,
Ndless r2022**, using the user-supplied NAND and matching Boot1 in an unchanged
upstream Firebird core. These results are emulator execution with ordinary OS
file/input paths; they are not physical-handheld timing measurements.

At def1db3 with the final NSM1/NSL3 package:

- Chinese OS with no language preference reaches the Chinese title and saved
  preview. Normal Quit returns to the OS. Immediate reopening without USB
  reaches the Chinese title again.
- A 196-command route executes actual deck, map, merchant opening, treasure,
  Smith pointer/directional control, Smith animation, physical eye-button
  click and Tablet upgrade paths, then exits normally. The console sets up
  selected rooms; the subsequent controls are ordinary UI input. This is not
  a natural three-act victory.
- French OS cold startup after Ndless installation, with no game-language
  preference, reaches the French title automatically.
- Dutch OS cold startup without a game-language preference reaches the
  English title and resumes the English combat fixture; the persisted language
  bytes are `NSL1eng\n`. French similarly persists `NSL1fra\n`.
- Removing the required shared artwork part produces the built-in English
  `RESOURCES NOT FOUND` screen; Esc returns normally to the OS.

The def1db3 multipart six-second pair is a **performance regression requiring
correction before final delivery**: 159 frames, 197,375 interval ticks, 140,266
render ticks, 20,919 resource ticks, 31 reads / 133,382 bytes / 21 seeks. One
interval exceeds 250 ms. Its mean interval/render are 37.88/26.92 ms; UI15 is
36.28/26.51 ms. The action-run maximum interval is 288.42 ms (not a subtracted
maximum). Source/resource traces are being used to identify the extra physical
reads and opens. This failed comparison is retained and is not replaced by the
earlier old-container result below.

The earlier matched six-second UI15 versus rendering-candidate comparison is
27.560 to 27.874 FPS and mean render 26.511 to 25.395 ms, with identical
19 reads / 116,259 bytes / 19 seeks. That is a small measured improvement in
one five-Dazed End Turn fixture. It uses old container files and predates the
integrated RAM/file-handle changes, so it is not mislabeled as final multipart
performance. Native CPU reductions likewise do not establish calculator FPS.

Two actual failures drove production fixes: an ordinary UI15 exit/reopen could
fail Zehn's contiguous allocation, and keeping all multipart files open failed
the seventh part's open. The shared workspaces and bounded file cache fix the
executed Chinese cold-start/normal-reopen path. A warm OS after changing its
language and USB transfer can still fail allocation before main; French cold
startup succeeds. The frozen prelaunch heap's largest free block is 22,145,408
bytes, already 266,412 bytes below def1db3's executable request before metadata
allocation. The approximately 75 KB table/reader changes cannot fix this exact
heap. That boundary is retained, not attributed to a French font
load that never ran. Arbitrary heap-fragmentation immunity is not claimed.

## Reconciliation and remaining boundaries

UI15 already completed surrounding monster command awaits, conditional
summon/dialogue/death paths, Vantom's 750 ms independent bone transition,
compound clips, masks and four source emitters. They remain connected to
production and are reverified; this turn does not relabel their probes as new
features. Earlier Doom, Architect, Random, Fake Merchant, rainbow/blur clocks,
card trails, generated-card arrivals, orb and creature effects also remain.

The following are still explicit:

- Full Ancient actor/background animation uses the previously accepted
  static storage exception. Ancient gameplay, dialogue, options, healing and
  card materials are implemented.
- Seed entry remains a 1–64 ASCII subset. Complete Unicode normalization and
  the obsolete lowercase `old` seed path are not implemented. This is unrelated
  to localized game text or the 16 supported language packs.
- RGB565/LDR, integer LCD rasterization, bounded CPU particles, independent
  cosmetic randomness and sampled material phases are calculator mappings.
  They are not continuous Godot GPU or global cosmetic-history equivalence.
- UTC calendar interpretation, 96-floor/32-run history and bounded drawing
  capacities remain. Audio, multiplayer, Timeline and desktop/network services
  remain outside the agreed offline single-player scope.

No 95% restoration score, universal 30 FPS, zero cold I/O stalls, natural
three-act victory or exhaustive original-desktop visual acceptance is inferred
from test counts. No manual browser/WASM acceptance was performed this turn.

## Installation and source persistence

Replace the launcher and the matching `data` resources together. Preserve the
existing `state` directory and its contents. Keep English installed for the
normal English fallback; copy only desired additional locales. The archive
contains no test saves or profiling marker, no NAND/ROM, and no original DLL/PCK.

Source is committed locally and preserved with the complete source archive and
Git bundle. The attempted GitHub source push was rejected by automatic approval
review for insufficient authorization evidence for that outbound payload and
destination. No connector bypass or claimed remote publication is used. A
remote ref receipt is still required before claiming synchronization to main.
