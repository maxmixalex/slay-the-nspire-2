# Slay the Nspire 2

Native TI-Nspire Ndless adaptation of Slay the Spire 2. Reference: the owner's **v0.111.0 / 41cef1ea** installation. Current development is **0.17**; verified progress is synchronized to [`main`](https://github.com/maxmixalex/slay-the-nspire-2/tree/main) from the integration branch. Current work integrates the original title and bilingual UI, on-demand loading, shared-resource installation and measured card-rendering improvements. Exact executed checkpoints and delivery status are in [WORK_STATE.md](docs/WORK_STATE.md); earlier results below retain their historical identities.

**Audio, multiplayer and Timeline are excluded.** Original UI fidelity, animation and single-player content breadth remain in scope. Original game data, fonts and calculator images are not included in public source.

## Active 0.17 development

The original three-act route now drives the playable world: Overgrowth or Underdocks, then Hive and Glory, with original regional room queues, encounters, events, ancients and saved map history. The five character/card/potion foundation is retained. There are **558 card models**, including the 61 appended event/curse dependencies, **64 potions**, a **297-model world relic registry** and **109 enemy IDs** including compatibility/display entries. Those are model counts, not a claim that every interaction has original-game differential coverage.

### UI12 calculator release — 2026-10-25

The interrupted workspace has been recovered, the user's UI/input feedback is
integrated, and verified source is synchronized to `main`. The deliverable is
the ordinary Ndless calculator launcher with shared artwork and independently
installed languages. Runtime: `169cea998135a47e5f8580b15df798244e0e7e6a`.

| Release property | Verified value |
| --- | --- |
| Default installed files | 69,509,705 bytes: launcher + common + English + Chinese |
| Launcher allocation | 20,356,556 bytes; unchanged 24 MiB ceiling |
| Locales | All 16 original editions; 14 are an optional separate archive |
| Regression suites | 134 complete registered native suites; 310 Python conversion/packaging tests |
| Device acceptance | Ordinary Firebird/Ndless startup, actual battles and rewards, save/Continue and return to OS |

The [UI12 feedback matrix](docs/ui12-lcd-followup-0.17.md) covers typography,
original controls/scrollbars, credits, title, collections, Back-button clearance,
character selection, rooms, map, combat, rewards, merchant and the Flag-key typed
console. Startup/SkipIntroLogo, sleep/swarm effects and source-timed camera
triggers close additional retained feature differences. Selected long hand
cards now scroll their body without moving their title, cost or neighbours.
Calculator defaults use the physical touchpad and surrounding keys; digits
1–9/0 select hand slots directly.

The closed matched merchant fixture measured a 23.23% lower mean frame interval
than UI11 N, but its maximum cold frame increased from 2,387.054 to 2,703.094 ms.
Those are emulator protocol measurements, not physical-device FPS or an end to
cold stalls. The ordinary run and the final launcher receipt have separate
checkpoints. Exact sizes, hashes, test identities and exclusions are in the
[release validation record](docs/validation-ui12-0.17.json).

Install `nSpire2.tns`, `nSpire2.common.tns` and the desired language files together.
The default pair is `nSpire2.eng.tns` / `nSpire2.zhs.tns`. Update these as a matched
set while preserving all saves/progress/settings/profile files. The first launch
plays the sampled original studio logo; Select/Cancel/click skips it, and
Settings can remember Skip Intro Logo. Flag opens the normal typed console.

Full Ancient actors retain the user's storage exception. Dense GPU materials,
HDR and exact desktop glyph rasterization remain bounded CPU/LCD adaptations;
no blanket source-pixel equality or 95% restoration score is asserted. Original
assets stay out of this public repository.

### Historical UI8 development


UI8 replaces the retired startup language page, expedition setup and potion
destination chooser with the original navigation and acquisition flow. The
current source includes original-scale merchant/map composition, Standard /
Custom / Daily setup, real two-bank key rebinding, source hand/orb/creature
geometry, continuous loot/card selection, Crystal Sphere and Ancient dialogue,
run history, profiles, credits, patch notes and local feedback export. The
integrated candidate also repairs actual card typography, unavailable-size
blank text and leaked rich-text markup, and implements solo score/badges and
discoveries from observed run facts.

The current source-to-UI ledger is
[the UI8 source audit](docs/ui8-source-ui-audit-0.17.json). It distinguishes
implemented behavior, unavailable historical facts, original hidden controls,
scope exclusions and remaining visual differences. The path scan covers the
supplied C# and scene inventory; it does not turn file or test counts into a
restoration percentage. Module evidence is recorded in
[rewards](docs/rewards-ui8-0.17.md),
[modes and input](docs/ui8-input-and-modes-0.17.md),
[powers](docs/power-order-ui8-0.17.md),
[auxiliary screens](docs/auxiliary-ui8-0.17.md), and
[performance/storage](docs/performance-ui8-0.17.md).

The UI8 resource extension is **NSP2/28**. One launcher can use one shared
art/animation file and either or both language files:

| File | Purpose |
| --- | --- |
| `nSpire2.tns` | Ndless launcher |
| `nSpire2.common.tns` | Shared artwork and animations |
| `nSpire2.eng.tns` | English text, glyphs and directories |
| `nSpire2.zhs.tns` | Chinese text, glyphs and directories |

Install these files together in one calculator directory. Startup goes directly
to the game menu. With both languages installed, choose the language in the
main menu's **Settings**; subsequent launches remember it. There is no separate
startup language screen. Both languages share **v17** gameplay saves, with
readers for historical v3–v16 saves. All startup stages and
resource errors have Chinese and English text. See
[platform-languages.md](docs/platform-languages.md) for browser and native hosts,
legacy full-pack loading and exact filename rules.

Exit the game normally and keep a backup of `nSpire2.sav.tns` when upgrading.
TI OS can require temporary space for both copies while replacing the large
common file. If it rejects the transfer for insufficient space, remove only
the old `nSpire2.common.tns` before transferring the new common file, then
finish updating the launcher and both installed locale files. Keep the save
and language preference files. When migrating from a complete pack with too
little free space, first back up the old `nSpire2.nsp.tns` to a computer and
move it off the calculator, then transfer all required new files before launch.
Otherwise, after confirming the split installation works,
the old complete `nSpire2.nsp.tns` resource file can be moved out to reclaim its
space; it is not needed alongside the common and locale files.
Startup reads the format header, resident directories and language marker.
Image, glyph and string payloads are decoded when used. Whole-pack integrity
hashing, integrity-only CRC and eager resource-wide validation are removed;
the actual ranges and format rules required for decoding remain.

### Historical UI3 measurements and delivery

The historical UI3 title-glow hybrid resources total **81,100,703 bytes**, compared with
93,783,363 bytes for the previous bilingual NSP2/19 split. Common card/UI images
keep their fast individual encoding; other retained animation images use bounded
compressed groups. This preserves image pixels and timing while saving
12,682,660 bytes. Including the 983,682-byte launcher, a bilingual installation
uses **82,084,385 bytes before saves**. English-only and Chinese-only installs
use 77,520,799 and 80,303,498 bytes respectively. The paired host performance
measurements used the earlier b4f7ba0 candidate; the final title glow adds only
15,602 shared bytes. The separate 30–40 MB target remains unmet. See
[the retained storage measurements](docs/retained-storage-0.17.md) for the
all-group comparison, cold-page tradeoffs and host-only timing limits.

The UI3 fixes included explicit event-option tooltips, contextual rich-text and
template substitution, scaled card focus geometry, original energy/intent/banner
composition, merchant body interaction and shop animations, and the original
Proceed artwork. Combat selections now wait for their effect cue, and intent
tooltips follow the visible actor, including slot replacement. Merchant focus
uses the original outline skin and sampled candle layers in both v19 and v20.
The title uses the original lettering, ten fire frames and a single additive
light mask with its authored scale and brightness curve.
Enemy HP, block, energy and intent display use a small read-only presentation
trace instead of jumping immediately to the simulation's final values.
Status/power strips and the displayed hand share that clock. Discarded cards
leave at their cue, Retain cards remain inspectable during enemy actions, and
the next player's draw appears at the player boundary. Full pile/card-effect
animation and unaudited enemy command delays remain separate fidelity work.
Exact coverage and remaining gaps are recorded in
[feedback-ui-0.17.md](docs/feedback-ui-0.17.md),
[event-option-tooltips.md](docs/event-option-tooltips.md),
[ui-template-audit.md](docs/ui-template-audit.md) and
[rendering-performance-0.17.md](docs/rendering-performance-0.17.md).
The merchant's finite six-second flame samples and remaining GPU particles are
described in [merchant-effects-0.17.md](docs/merchant-effects-0.17.md). The title's
separate background stars and fog were added later as bounded CPU effects;
full original GPU turbulence is still a visual difference.
Original UI fidelity is the first priority; neither overall 95% acceptance nor
physical calculator FPS has been established.

The UI3 delivery contains a **76,697,282-byte calculator ZIP** and a
**115,479,696-byte self-contained bilingual HTML**. Their exact contents and
installation rules are in [the delivery record](docs/delivery-ui3-0.17.json).
[GitHub Actions run 39977430306](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/39977430306)
passed all 73 ASan/UBSan suites, 185 conversion/packaging tests and both portable
builds. Ordinary Firebird battle/save/exit and real browser language/save flows
also passed. First entry into the Chinese card collection still produced a
1,646.8 ms render in the emulator; that cold-page cost is not hidden by the
faster desktop-browser warm-cache measurements. See [Firebird acceptance](docs/validation-ui3-firebird-0.17.json)
and [browser acceptance](docs/validation-ui3-browser-0.17.json).

### Earlier 0.17 execution evidence

The source17 checkpoint [`26a90975`](https://github.com/maxmixalex/slay-the-nspire-2/commit/26a9097521a971b69a4b58494985c6062a24017e) passed **all 52 registered native suites** from one frozen source. This includes all **860 character/Giant Rock acquisition variants** and **174 encounter/ascension variants** through the actual turn driver. ARM and WASM builds passed; the actual resource backend comparison matched **357 frames and 25 saves across 18 scenarios**. The exact evidence is in [validation-source17-0.17.json](docs/validation-source17-0.17.json). Later stages and their separate gates are recorded in [WORK_STATE.md](docs/WORK_STATE.md).

Original scene work includes the retained room below the map window, map travel and fade layers, regional rest/treasure scenes, native enemy animation states, event/ancient text and option styling. The requested **95%+ original UI fidelity has not been established**. See the [fidelity contract](docs/parity.md) for the distinction between screen adaptations, remaining implementation gaps and original-application visual acceptance.

Source19 [`00c6b9dd`](https://github.com/maxmixalex/slay-the-nspire-2/commit/00c6b9dda80333c3aa260d82e650b079f9fe76a3) subsequently passed **31 affected suites**, native/ARM/WASM builds, **48,111 enemy checks / 174 encounter-ascension variants**, and the expanded actual-resource backend comparison: **480 exact frames / 30 saves across 23 scenarios**. Its first-open map camera/banner and escape/cache repairs are covered in [validation-source19-0.17.json](docs/validation-source19-0.17.json). This focused run does not replace the earlier full 52-suite baseline with a claimed full 54-suite run.

**The historical complete lossless NSP2/15 resource pack was built and verified:** **80,846,058 bytes (77.10 MiB)**, preserving every original sparse-frame byte and all timing aliases. Source20 [`3f66da83`](https://github.com/maxmixalex/slay-the-nspire-2/commit/3f66da839e056fe291c27ca5e93260b3833bd7de) passed native/ARM/WASM builds, ten affected sanitizer suites, 24 encoder tests and linked ARM decoder/ABI checks; its ARM allocation is **14,710,272 bytes**, below the project's 16 MiB bound. See [validation-source20-0.17.json](docs/validation-source20-0.17.json).

Source21 [`d5738010`](https://github.com/maxmixalex/slay-the-nspire-2/commit/d5738010f2d7ebc6139a2fa181ff87820d344bbb) removes eager decoding of every tiled image from startup. Opening still checks the complete file, directories and index geometry; each image receives complete pixel validation before its first successful use. Its frozen native/ARM/WASM builds, ten affected sanitizer suites and fresh actual-resource backend comparison (**23 scenarios / 480 frames / 30 saves**) passed. The complete C++ oracle passed both new-memory and new-range traversals against the original v14 bytes: **45,724 images / 994,105,847 decoded bytes per backend**, all metadata and 40 three-owner UI frames. It completed in 1,551.05 seconds; the two failed build attempts remain recorded separately. See [validation-source21-0.17.json](docs/validation-source21-0.17.json).

Source22 [`5d177be7`](https://github.com/maxmixalex/slay-the-nspire-2/commit/5d177be78c507c9cd4b3c9d38a4600e84a44c02e) replaces rigid image-size tiers with a **748-page / 90-entry shared LRU cache** within the previous total cache budget. Its 147-step native build, ARM/WASM builds, ten affected ASan/UBSan suites, linked ARM vectors and fresh **23-scenario / 480-frame / 30-save** comparison passed. Selected actual-resource checks compare all 3,535 retained images and a 23-frame animation through both new backends against v14; these selected checks are distinct from source21's complete catalog proof. The animation's second cycle now has **zero resource callbacks for all 23 frames**, versus 216 calls / 11,671,220 bytes on source21. Cold first-cycle reads are unchanged; device speed is unmeasured. See [validation-source22-0.17.json](docs/validation-source22-0.17.json).

The historical source22 launcher is **885,386 bytes**; the two calculator files total **81,731,444 bytes before saves**. The **30–40 MB target is not achieved**. None of the seven alternative reversible tile representations improved the existing encoding in the declared eight-group sample. Grouped retained images/glyphs show about **8.43 MB** of potential component savings, but require an unimplemented format/reader and still model roughly **69 MiB** before new global metadata. See [resource-budget-0.17.md](docs/resource-budget-0.17.md) and [the exact diagnostic evidence](docs/validation-resource-research-source22.json). The private ZIP and offline HTML are development artifacts, not hardware or 95% visual acceptance. That historical pair requires its matching NSP2/15 resource. Use the current split installation above for this feedback stage.

Those source20–22 resource and launcher sizes describe earlier NSP2/14–15 stages. Current UI8 source accepts NSP2/28 and retains old complete-pack readers, including the verified bilingual NSP2/17 and NSP2/18 packs. Gameplay writes **v17**, retaining historical v3–v16 readers; resource-storage and save versions are independent. The browser selects a save key from the actual encoded version and preserves compatible historical keys. Build the current native/ARM sources with the logged commands below. World resource conversion is described in [enemy-assets-0.17.md](docs/enemy-assets-0.17.md), with the latest append-only extensions in [feedback-ui-0.17.md](docs/feedback-ui-0.17.md) and [merchant-effects-0.17.md](docs/merchant-effects-0.17.md). Generated packs and embedded-content HTML stay outside this public repository. The older conversion commands and artifact statistics below describe **0.16 only**.

## Verified 0.16 baseline — historical content

**0.16 completed all five single-player character card pools** on `work/0.16-five-characters`. Its verified runtime is [`8c6977d3`](https://github.com/maxmixalex/slay-the-nspire-2/commit/8c6977d3cc1dc9f17926a480127bc1067abd0286), with **31/31 native suites, 30/30 Python tests, actual-resource UI and ARM/WASM/X11 builds passing**. The subsequent commits on that older branch updated documentation only. Exact scope, artifact hashes and journal receipts are in [validation-0.16.json](docs/validation-0.16.json).

497 card models: **85 single-player Ironclad**, **86 each for Silent, Defect, Regent and Necrobinder**, **53 single-player colorless** and 15 generated/status models. There are **64 active potion models**, 33 relics and five adapted events. The five character pools include executable base and upgraded rules and numerical regression coverage. Defect has five orb types and Focus; Regent has independent star payments and Forge; Necrobinder has Osty, Soul and Doom. Ancient-rarity cards are available in the compendium/practice pool and excluded from ordinary rewards and generation. The twelve-floor playable route and Nibbit encounters remain adapted; the reference regional map is still separate from a complete original run.

Practice exposes every character/colorless card and potion through the compendium. Open it with **C** or the practice HUD button, inspect an entry and use its acquisition button. Card variants, potion targeting, multi-card selections, generated offers, nested automatic plays and persistent statuses share the same engine and UI on native and browser builds. A full hand sends newly created cards to discard; a full potion inventory must be freed before acquiring another bottle.

Ordinary potion pools follow all five current characters and the pinned rarity weights. Other-character and special-acquisition bottles remain available for explicit practice use; their original special acquisition routes are separate work. The 0.13 event, relic, treasure and fanned-hand UI work is retained. These layouts are adaptations, not complete original-PC fidelity.

Read [current work state](docs/WORK_STATE.md) before resuming development and the [Ironclad](docs/ironclad-completion-0.15.md), [Silent](docs/silent-completion-0.15.md), [Defect](docs/defect-completion-0.16.md), [Regent](docs/regent-completion-0.16.md), [Necrobinder](docs/necro-completion-0.16.md), [colorless](docs/colorless-completion-0.14.md) and [potion](docs/potions-completion-0.14.md) audits for implementation scope and original-rule evidence. Registry counts alone are not a passing test or a claim that the entire original game is complete.

## Source builds and historical 0.16 conversion

GitHub Actions is enabled in [`.github/workflows/ci.yml`](.github/workflows/ci.yml).
It runs native ASan/UBSan regressions, converter tests and ARM/WASM builds without
private original assets. See the [workflow runs](https://github.com/maxmixalex/slay-the-nspire-2/actions)
for each exact source checkpoint's status.

```sh
# Set this to the full commit whose remote branch and source tree were verified.
NSPIRE_CHECKPOINT='<verified-full-commit-sha>'
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label arm -- python3 tools/build_ndless.py
cmake -S . -B build/host -DSPIRE_X11=OFF
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label native-build -- cmake --build build/host --parallel
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label native-tests -- ctest --test-dir build/host --output-on-failure
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label python-tests -- python3 -m unittest discover -s tests -p '*_regression.py'
```

ARM requires LLVM clang++ and ld.lld; the browser build also requires wasm-ld. The X11 host is optional. Install converter dependencies from requirements.txt. Convert an existing private 0.15 NSP2/12 pack without resampling its retained animation frames:

```sh
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label content -- python3 tools/build_all_characters.py /path/to/game /path/to/old/nSpire2.nsp.tns --output dist/nSpire2.nsp.tns --font /path/to/local-font
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label web -- python3 tools/build_web.py
```

For a full conversion from the pinned installation, `build_content.py` chains through the character conversion. The converter checks the release version and commit metadata; content identities serve conversion cache reuse rather than an integrity allow-list. The historical 0.16 launcher requires **NSP2/13** (497 cards, 339 auxiliary images and 1,556 strings). Put `nSpire2.tns` and `nSpire2.nsp.tns` in the same calculator directory for that historical format; `nSpire2-desktop.html` is the self-contained offline browser build. Replace the launcher and pack together.

Historical 0.16 saves use **v14** and retain readers for project v3–v13. The appended v14 payload records all three new character modules, wide card/relic identifiers, independent star prices, persistent deck growth, expanded acquisition order and cross-character Nightmare modifiers. Earlier writers did not record the new histories; migration preserves encoded facts and seeds only observable pending play/death baselines. The genuine frozen v13 continuation fixture and earlier historical fixtures exercise those boundaries. Older Entropy/Boulder and Panache/Calamity acquisition orders retain their documented historical migration policies. The browser writes to a separate v14 storage key and can import earlier keys. Old launchers cannot read the new save.

The native suite includes all character, colorless, potion, historical migration and UI regressions. `expansion_ui_regression`, `character_ui_regression` and `five_character_ui_regression` optionally accept a real NSP2/13 file and an existing private screenshot directory. They cover all 860 character/Giant Rock variants, retained colorless/potion flows, generated/status cards and five-character mechanics. The optional native/WASM oracle adds five controlled combat sequences to the four retained room sequences.

## Historical 0.16 persistence and validation

At the verified **0.16** runtime source, **31/31 C++ suites and 30/30 Python tests passed**, with ASan/UBSan on native regressions. The native suites report **929,474 checks/assertions**. The new completion suites pass Defect **16,190 checks / 172 variants**, Regent **9,892 / 181**, and Necrobinder **19,494 / 174**; the latter variant counts include their required generated cards where stated in the audits. Checks include repeated assertions and are not counts of unique scenarios. Earlier [0.15](docs/validation-0.15.json) and [0.14](docs/validation-0.14.json) reports remain historical.

Actual-resource UI passed **81,026 checks**: **66,478** across all **860 character/Giant Rock acquisitions**, **7,173** for five-character mechanisms and **7,375** for all **106 colorless variants / 64 potions**. The real NSP2/13 parser passed **3,736 checks**. Forty final UI scenes and two Osty animation-end scenes were visually reviewed. Native/WASM comparison matched **384 RGB565 frames and 23 saves** across four room and five character-combat sequences. It uses the project storage adapter; browser DOM/localStorage and original-PC differential parity are separate claims.

Fresh X11 host processes loaded and identically re-saved v14 ordinary **88,764-byte** and **512-instance / 109,012-byte** files using production file callbacks in headless action mode. The fully bounded save also contains 192 permanent cards, 12 automatic frames, 512 effects and 16 Nightmare snapshots; its conservative **109,064-byte** ceiling fits **131,072 bytes**. The genuine old v13 fixture migrates **41,820 to 88,876 bytes** while retaining and resuming its suspended payload.

The final **12,910,778-byte NSP2/13** pack contains 2,638 images, 7,215 glyph records and 38 clips. All 1,622 old image payloads, 6,265 old glyph payloads and earlier card/string/auxiliary/clip records are preserved. It fits the **13 MiB** resource buffer with 720,710 bytes remaining. ARM produces a **284,458-byte** launcher with **16,174,284-byte** static allocation, below the unchanged 16 MiB packer ceiling. WASM is **817,288 bytes**, links with fixed **16 MiB** memory and is embedded with the identical pack in the **18,319,349-byte** offline HTML. No new Firebird or physical-device execution is established. All **19 final journals** passed with unchanged frozen source; the two canonical release files are `Slay-the-Nspire-2-calculator.zip` and `Slay-the-Nspire-2-offline.html`.

Commit source to a named work branch and read the remote ref back **before** long compilation or conversion. `tools/run_logged.py` records the supplied checkpoint, input hashes, command, running/completed state, exit code and log hash. It does not itself verify or push a remote commit. A running journal after interruption is not a passed test.

Current execution results belong in WORK_STATE, not inherited historical reports. Compilation, cross-backend agreement and actual target execution are different evidence levels. Physical performance, complete original encounters/runs and exact UI parity remain unverified or unfinished.

## Controls and license

Directions navigate; Enter confirms; Space ends a turn or confirms a pending selection; Tab piles; I details; C compendium; V statuses; M map; P potions; R relics; S save; O load; Esc return/pause. Touchpad clicks and dragging share the native dispatcher.

Source: GPL-3.0-or-later (COPYING). Preserve NOTICE.md and licenses/. Original assets and trademarks remain with their owners; source licensing grants no additional asset redistribution rights.
