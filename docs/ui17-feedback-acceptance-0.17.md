# UI17 feedback and retained-difference acceptance ledger

Reference: **v0.111.0 / 41cef1ea**. This source-only reconciliation was checked
against combined source `483f8d4db6c21ed351d03e5207afa60d9814256c`, recovered
UI16 `1526ede2a84e168cd46d430338e5d5a540ecaf18`, and the named original-reference
records below. It contains no original scenes, assembly, disassembly, fonts or
extracted textures.

The nine reported UI/storage items have concrete implementations or an
explicit original-behavior decision. The previously missing Unicode/raw legacy
seed behavior is newly implemented in UI17. The named monster-command and
Vantom omissions were already implemented in UI15 and must remain implemented.
This ledger does not convert those historical results into new UI17 tests,
claim 100% original-game equivalence, or substitute an audit for a feature.
Final launcher/data identities and target performance belong to the integrated
release evidence referenced by [WORK_STATE](WORK_STATE.md).

## The nine reported items

| Feedback | Implemented behavior / acceptance decision | Concrete source and tests |
|---|---|---|
| **1. Split the oversized `nSpire2.dat.tns` by concern** | **Implemented in UI16; further compression implemented in UI17.** The old common file becomes a small NSM1 index plus eight required NSS1 files: interface, cards, characters, enemies, bosses, rooms, effects and shared. Locales remain independently optional NSL3 files. All required parts must be from a matching edition. This is a functional split, not several copies of the old file. | [`storage_source.hpp`](../include/storage_source.hpp) names the concerns; [`storage_source.cpp`](../src/storage_source.cpp) maps their unchanged logical address space; [`split_content.py`](../tools/split_content.py) reconstructs complete editions. `storage_source_regression`, `resource_file_cache_regression`, `split_resource_regression` and actual-edition tests cover bounds, reads, failures and reconstruction. [UI16 storage](ui16-storage-0.17.md), [UI17 index encoding](ui17-index-prediction-0.17.md), [UI17 retained encoding](ui17-retained-completion-0.17.md). |
| **2. Test more default languages; unsupported calculator languages and data errors should fall back appropriately** | **Implemented and tested in UI16; preserved.** A healthy saved selection wins, followed by the system language, installed English and then another usable installed edition. Danish/Dutch/Finnish/Norwegian/Swedish and unknown system tags choose English when it is installed. Failed candidates are removed for the session. Built-in recovery screens and technical load/save/language errors are English and do not require a working game font pack. A missing English pack cannot provide an English game edition; keep it installed for the guaranteed normal fallback path. | [`locale_boot.hpp`](../include/locale_boot.hpp), [`platform/ndless/main.cpp`](../platform/ndless/main.cpp), `boot_locale_regression`, `locale_editions_regression`. The historical boot matrix covers 43 system-tag cases and all 65,536 installed-language masks; the complete UI16 edition gate includes 90 actual failed-pack English recoveries. Historical Firebird selected cold French and cold Dutch, with Dutch falling back to English, and showed English recovery after a required part was removed. These are named tests, not sixteen individually tested calculator OS menus. [Language/font report](ui16-language-fonts-0.17.md), [Firebird receipt](ui16-firebird-0.17.md). |
| **3. Why does the upgrade picker have a View Upgrades control?** | **Verified original behavior; preserve it.** It is a pointer-only checkbox which previews upgrades across the selection grid. Directional input hides it and resets its preview. It is not an unrelated new radio choice and should not be deleted to satisfy the visual suspicion. | The pinned `NDeckUpgradeSelectScreen._Ready` and controller behavior are recorded in [UI16 feedback](ui16-ui-feedback.md). [`deck_selection_ui.inc`](../src/deck_selection_ui.inc), [`ui_input.cpp`](../src/ui_input.cpp) and [`world_selection_input.inc`](../src/world_selection_input.inc) implement the visibility/reset rules. `deck_selection_ui9_regression` toggles it by pointer, switches to directional input, verifies reset/hiding and performs the actual upgrade. |
| **4. Deck-page filter/sort text is too small** | **Fixed in UI16; preserved.** The four sort labels use 8 px, with an 18 px strip, the original order/alignment/arrows, and bounded horizontal overflow. Measuring/preparing and drawing use the same size and locale face. The first card row is not displaced into the controls. | [`pile_ui6.inc`](../src/pile_ui6.inc), [`ui_frontend.inc`](../src/ui_frontend.inc). `card_hud_ui_regression` and real English/Chinese captures cover the deck; `audit_locale_fonts_ui16.py` requires exact 8 px records for all four labels in all sixteen editions, rejecting hidden `?` or neighboring-size substitution. [UI16 feedback](ui16-ui-feedback.md), [font evidence](ui16-language-fonts-0.17.md). |
| **5. The tucked card did not follow the eye button up and left** | **Fixed in UI16; preserved.** Eye and tucked-card geometry share constants. The eye is at `(5,108)`, 24 px square; the played card ends at center `(18,78)`, previously `(31,88)`. The 250 ms cubic transition and half-scale remain. The retained one-pixel horizontal/vertical-clearance choices are declared LCD readability adaptations. | [`combat_layout.hpp`](../include/combat_layout.hpp); `combat_layout_regression` checks endpoints/interpolation/bounds. `world_ui_regression` plays real Prepared+, waits for its two actual draws, peeks the pending discard selection and checks the original card's final position and unchanged canonical continuation. The original marker under `NPeekButton` is documented in [UI16 feedback](ui16-ui-feedback.md). |
| **6. Check the apparently mixed Sans/Serif fonts, including other languages** | **Original role mapping audited and corrected in UI16; preserved.** Normal MegaLabel/MegaRichTextLabel text uses the locale's Regular substitution where required; explicit rich bold/italic keep their own roles. Chinese Sans normal text and Serif emphasis intentionally coexist in the original. A blanket single-family substitution would be wrong. | [`source_locale_fonts.hpp`](../include/source_locale_fonts.hpp), [`native_controls.inc`](../src/native_controls.inc), [`deck_selection_ui.inc`](../src/deck_selection_ui.inc), [`card_choice_ui.inc`](../src/card_choice_ui.inc), [`world_selection_ui.inc`](../src/world_selection_ui.inc). The report names the original `FontManager`, locale font sets, label classes and audited scene families. Actual evidence covers 3,166 required glyph/size/face records, 48 original-family sample rasters and all sixteen locale normal-label oracles. It establishes those audited controls and roles, not an exhaustive comparison of every Godot control. [Font report](ui16-language-fonts-0.17.md). |
| **7. Tablet of Truth upgrade effects differ from rest-site Smith** | **Verified intentional original distinction; preserve it.** Tablet's random upgrades use Event layout; its final whole-deck upgrade uses Messy layout with 200 ms normal / 100 ms fast staggering. Smith has its own hammer, impact and spark sequence rather than that event preview. | [`reward_presentation.inc`](../src/reward_presentation.inc), [`world_camp_ui.inc`](../src/world_camp_ui.inc), [`reward_presentation.hpp`](../include/reward_presentation.hpp). `combat_ui15_regression::tablet_upgrade` exercises all five real Tablet stages in normal/fast modes, detached per-card facts and staggering. `deck_selection_ui9_regression` and `world_room_ui_regression` cover actual Smith selection/confirmation. Original `TabletOfTruth` and `SmithRestSiteOption` choices are recorded in [UI16 feedback](ui16-ui-feedback.md). |
| **8. Map icons appear smaller** | **Verified original control size and restored atlas margins; preserve them.** Ordinary nodes are 20 px, focused nodes 29 px, with the original selectable pulse. The 92 px source control projects to 20.24 px at the established 0.22 scale. The complete 128 px logical atlas canvas includes transparent margins; earlier cropped-as-full drawing enlarged the visible ink. Enlarging it again would reintroduce the mismatch. | [`world_map_ui.inc`](../src/world_map_ui.inc), `world_map_ui_regression` and [UI16 source-scene/atlas evidence](ui16-ui-feedback.md). Icon and outline use the same logical canvas and scale; history hover preserves the source outline rule. Boss/start nodes retain their separate geometry, so 20/29 px is not a claim about every map object. |
| **9. Treasure's “What's Inside?” / “里面有什么？” is below the ribbon** | **Fixed in UI16; preserved.** The banner's authored child-label rectangle projects to a single 10 px line at `(117,62)` in an 87 px box, including the banner's animated vertical offset. Long titles scroll inside the same line rather than wrapping below the ribbon. English fallback is “Treasure”. | [`world_treasure_ui.inc`](../src/world_treasure_ui.inc), `world_room_ui_regression` and the English/Chinese room captures. The original `common_banner.tscn` inset `(100,21)..(554,106)` inside 654×162 is documented in [UI16 feedback](ui16-ui-feedback.md). |

The UI16 component run covered eight affected suites and actual English/
Chinese scenes. The final UI16 release separately passed 147/147 native
suites. Its broad 196-input Firebird route, French/Dutch defaults and missing-
part recovery belong to their recorded intermediate runtime; the final UI16
runtime has separately identified cold/reopen and warm-combat/reopen evidence.
Those captures must not be relabeled as new UI17 execution. UI17's exact
resource-byte comparisons and code regressions preserve the implementations;
its final ordinary-OS receipt supplies the new runtime-specific acceptance.

## Newly closed original-behavior difference in UI17

The earlier “ASCII seed only; Unicode normalization and lowercase `old`
unsupported” statement is superseded by the implemented
[Unicode seed report](seed-unicode-ui17-0.17.md).

The original canonicalizer performs invariant simple uppercase, ASCII O/I
replacement and Unicode trimming. It does **not** perform NFC/NFKC or full-case
expansion. Normal setup input `oldA` becomes `0LDA` and uses modern xxHash64.
The separate raw/debug path performs the original linguistic lowercase-prefix
test, fixed three-UTF-16-unit substring and old 32-bit hash. The original game
sets invariant culture; the executed oracle's Linux/.NET/ICU environment is
explicitly recorded rather than generalized to all globalization engines.

Production source is [`reference_rng.cpp`](../src/reference_rng.cpp),
[`world_rng.cpp`](../src/world_rng.cpp), setup/departure input, map preview and
the `raw:` debug shortcut. The source-only original-method runner is in
`tools/seed_reference_oracle`. It preserves original seed/hash/RNG methods,
isolates only the telemetry module initializer in a private copy, and ships
no assembly or IL. Native and ASan/UBSan each pass **3,354,588 checks** over
31,209 strings and all 1,112,063 non-NUL valid Unicode scalars. Nine native
suites cover integration; the new world/UI/save test has 1,161 checks. Full
ARM compilation passes with zero additional `.data` or `.bss` for this seed
component.

This is a completed implementation with working playable-run and save/
continue paths. Its retained boundary is **64 UTF-8 bytes**, not 64 arbitrary
characters. Malformed UTF-8 is rejected; C strings cannot contain an embedded
NUL. It does not add a calculator Unicode input method or unlimited font
coverage. Existing Unicode-capable host input can feed complete scalars.

## Older missing features which are already implemented

Historical omission lists must be read with their superseding records. In
particular, [UI14 retained differences](ui14-retained-differences-0.17.md)
describes missing work that [UI15 reconciliation](ui15-retained-differences-0.17.md)
subsequently implements:

| Earlier item | Current implementation and evidence identity |
|---|---|
| Monster awaited commands beyond the AttackCommand profile table | `enemy_command_program.hpp` and `enemy_command_program_ui.inc` execute source-derived command programs through real move/selection/death observers. UI15's `combat_ui15_regression` executes all 108 original actor kinds / 1,312 legal moves with normal/fast and A0/A9 coverage. Animation duration is not substituted for an unknown source await. This is retained UI15 implementation. |
| Vantom's 750 ms root-bone transition and independent tail | `vantom_ui15.inc` keeps compound layers, source bone intervals and independent tail coordinates; `vantom_vfx_ui15.inc` implements the associated masks, emitters and delayed death callbacks. UI15 verifies 116 compound clips, 2,040 full-scale frames and its sampled source vertices, with dedicated VFX tests. Neither correct endpoints alone nor a new probe is being counted as the transition. |
| Ancient healing, actual upgrade notifications and rest Smith impacts | The real gameplay observer schedules positive Ancient healing, retains before/after card facts, and drives the nine Smith impact frames with the hammer/sparks. `combat_ui15_regression`, the reward observer and rest/deck tests identify the actual commit paths. Full-HP zero healing does not invent a heal effect. |
| Doom, Architect, Random and Fake Merchant visual families | `combat_doom_ui.inc`, `architect_effects_ui13.inc`, `architect_attack_ui13.inc`, `random_character_ui13.inc` and `fake_merchant_ui13.inc` retain their implemented source families. Their UI13/UI14 ledgers and named regressions own the historical resource/performance evidence. No newly counted UI17 implementation follows merely from rereading them. |
| Exhaust/discard/power trails, generated-card arrival and orb activation | `combat_secondary_ui11.inc`, command observers and `combat_secondary_ui11_regression` already implement these paths. Generated cards retain growth and flight; a resource preparation/performance fix is distinct from adding an absent animation. |
| Creature scale/hue/sleep effects, preview contexts, startup logos, blur/rainbow and natural font metrics | These have the implementations and superseding per-family records listed in UI14/UI15 reconciliation. Named context and regression coverage does not imply every possible Spine track combination or desktop GPU history has been exhaustively reproduced. |

The initial [world-parity plan](world-parity-0.17.md) and the older sections of
[map-seed scope](map-seed.md) describe their historical baselines. Their old
12×3 expedition and “preview not integrated” statements do not mean today's
seven-column original three-act world remains unimplemented. Conversely,
registry counts, portrait availability and a controlled fight do not prove
all original interactions or a natural three-act completion.

## UI17 performance/storage work is implementation, with separate target gates

The new TGI2 index predictor, additional complete retained-group filtering,
compact resident image records, retained directory address table and exact
constant-color drawing have production code and executed gates. They do not
discard another texture, frame, glyph or string to create a smaller result.

The completed resource converter chooses 496 smaller index groups and 608
additional smaller retained groups. Actual reference comparison covers all
58,060 images / 1,167,930,140 decoded image bytes through both memory and
ranged backends, glyphs, strings, clips and 40 complete UI frames. The changed
bytes encode the same already-converted LCD content; they do not certify the
original desktop GPU's entire rendering process. See the [index](ui17-index-prediction-0.17.md)
and [retained](ui17-retained-completion-0.17.md) reports for checkpoint-specific
counts and conversion receipts.

The current complete default resource set is **78,190,434 bytes before its
launcher**. The earlier 65–70 MB installation target is therefore still unmet;
the final launcher must be included in the delivered installed total. A
resource saving, native microbenchmark or lower mean render-stage elapsed
time cannot independently establish higher whole-window calculator FPS.

The [directory review](directory-address-table-ui17-0.17.md) identified and
implemented the startup address-table repair: repeated 64 KiB requests now
retain only 256 bytes of functional addressing. Actual English split-source
open drops from 44 to 36 physical reads compared with compact records before
that repair; this is an I/O count, not a Firebird timing claim. The original
image-pool capacity is preserved. [Exact dim rendering](ui17-rendering-0.17.md)
has its own exhaustive RGB565 and instruction/layout evidence.

## Explicit agreements and retained limits

Only the first three entries below are described as already agreed scope or
readability allowances. The other entries are disclosed implementation or
evidence limits, not a blanket claim that the user approved every difference.

| Category | Retained boundary |
|---|---|
| Agreed storage exception | Complete Ancient scene actor/background animation stays static. Ancient gameplay, dialogue, options, healing and card materials remain implemented. No other missing animation is automatically covered by this exception. |
| Agreed excluded systems | Audio, multiplayer and Timeline are outside the offline single-player scope. These absences receive no fidelity credit. |
| Agreed readable LCD/input adaptations | Reflow, readable type, hit targets, scrolling, detail views and keyboard/touchpad mappings at 320×240. This does not authorize changes to card effects, probabilities, canonical RNG, command timing or removal of animation frames. |
| Material/raster representation | RGB565/LDR output, integer LCD rasterization, bounded CPU particles and sampled shader phases. Affliction materials retain their disclosed eight phases; Infection retains 30 frames and Ancient card fire 10. Continuous GPU equality is not established. |
| Cosmetic randomness | Local presentation noise preserves gameplay RNG but does not reconstruct an unrecorded process-global cosmetic RNG history. It is separate from the newly corrected seeded-run hash semantics. |
| Offline service boundary | Desktop/network integrations are not emulated. The local daily/custom modes and their unranked statistics are tested by `run_modes_ui8_regression`; those tests do not establish original online-service or leaderboard behavior. |
| Calendar and history | `ui_calendar.hpp` interprets RTC timestamps in UTC without a desktop timezone database. `ui_progress.hpp` retains 96 floors / 32 runs; finite transition, drawing and note capacities remain. Evicted facts are not represented as an unlimited complete history. |
| Seed input | The original transformations and raw hash path are implemented within the existing 64-byte UTF-8 field; malformed input and unavailable display glyphs remain explicit boundaries. |
| Directory capacity | Logical metadata is bounded by 2 MiB, with a 1,835,012-byte resident budget after image-row compaction. Actual editions and the existing 65,535-image full-directory fixture fit; some arbitrary older extreme combinations with fewer images and excessive unused group rows do not. |
| Loader/OS heap | Native allocation budgets are not a physical peak-RAM measurement or a guarantee under every fragmented OS heap. UI16's warm-French/USB allocation failure is historical evidence before main, not a French font failure. Any UI17 claim to close that exact state requires its own matched target run. |
| Verification coverage | Full suite passes, exact converter bytes, native/ARM builds and named Firebird scenarios have different meanings. No physical-handheld FPS, universal 30 FPS, zero cold stalls, exhaustive original-Godot acceptance, natural three-act victory or measured 95% restoration follows automatically. |

## What remains to close in this round

This reconciliation found **no additional source-confirmed missing UI feature**
among the nine reported items after the implemented seed and address-table
work. The three intentionally retained original behaviors are not open bugs.
Older Vantom/monster-command pending paragraphs are superseded by actual
UI15 code and tests, rather than a new unimplemented backlog.

The remaining release work is concrete integration acceptance: freeze the
combined launcher and matching complete data, pass its full gates, record
ordinary-OS cold/reopen and matched action-window results, recount the final
installed bytes including the launcher, and retain any slower scope or failed
heap configuration. The final evidence must identify its actual source/data
and distinguish historical captures. A source-publication approval block is
also separate from gameplay completion; a local bundle is not a new remote
ref or a CI result.

The seed's field capacity, complete Ancient animation exception, GPU/cosmetic
representation, timezone/history bounds and unexecuted broad acceptance are
not silently marked complete. Expanding one would require a separately scoped
production change and corresponding evidence; this audit does not invent a
replacement feature or run unrelated probes to make the checklist look full.
