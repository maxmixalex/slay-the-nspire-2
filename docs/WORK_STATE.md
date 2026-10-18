# Work state — 2026-10-18

## Active 0.15 development

The user requested continued development followed by the **complete Ironclad and Silent card pools**, with parallel agents. Continue on **`work/0.15-full-characters`**, created from and read back at `9c4d47e543a2ee9af58fe075700ff50cf205740b` (tree `949fc6b2b5e748ac1d7b4668252d87322c9299b4`). The completed 0.14 work branch is retained at that commit; its tested runtime remains `b4b67e714ca76e7afb72f3a170789ad90047e6f4`. No 0.15 behavior or build has passed yet.

The locked assembly declares 90 Ironclad and 91 Silent models. Applying its multiplayer constraints leaves **85 Ironclad and 86 Silent single-player models**, including Silent's Ancient-rarity WraithForm in the compendium/practice pool. Ordinary rewards and generated cards must exclude Ancient rarity. Existing character IDs remain unchanged; append the 56 missing Ironclad models at IDs 114–169, the 57 missing Silent models at 170–226, and generated GiantRock at 227. Inky is an enchantment, not a new card token. Do not import unrelated STS1 status cards from memory.

The root owns shared CardID/pool dispatch, combat hooks, status UI, save v13 integration, tests/build registration and verified source persistence. The `colorless` agent owns the isolated Ironclad state/operations/definitions/core/codec/validation and its audited config/regression; `potions` owns the corresponding Silent modules. `recover_workspace` owns original resource conversion, fixed historical resource profiles, descriptions, card-detail type/cost rendering and real-content UI coverage. Source state stays explicit and bounded; save v13 appends character modules after the frozen v3–v12 encoding. The intended content profile is NSP2/12. The practice deck capacity will grow to 192 to hold each full character pool plus all 53 colorless cards.

The two existing downloadable artifacts are the completed 0.14 calculator ZIP and offline HTML. Keep them available until a fully verified matching 0.15 pair replaces them. Preserve every prior image payload while appending original card and power art; Sidestep may use the actual original beta portrait with that source recorded explicitly. No image generation is authorized.

## Completed 0.14 delivery

Runtime source **`b4b67e714ca76e7afb72f3a170789ad90047e6f4`**, tree `5c97f84b8357176ddae5deec5b1911c7b0ff7aa8`, was pushed and read back before final validation. All **21 C++ suites and 10 Python tests passed**. All 12 final journals completed with unchanged frozen source. The build/artifact receipts and scope limits are recorded in [validation-0.14.json](validation-0.14.json). Later changes only update audit/result documentation; distributed binaries are identified by this tested source commit.

## Active branch and recovery receipt

The completed 0.14 branch is `work/0.14-colorless-potions`; new development uses the 0.15 branch above. The user prioritizes GitHub persistence before long validation and parallel agents. Main remains `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0` (historically tested 0.13). CI remains disabled. Reference remains **v0.111.0 / 41cef1ea**. Audio, multiplayer and Timeline are excluded.

The accessible scratch workspaces did not contain the earlier checkout. All 163 source files were restored from work-branch commit `50a58c9b876c3038e549b97e7e7d245224d717d8`. Every Git blob and the complete tree `2fb04a1bb26b50d96d6741cbe2cba17f4a9366cc` were verified, and the exact shallow Git commit was reconstructed. The current working directory is `/workspace/scratch/5af9d4bf40b9/slay-the-nspire-2`.

Recovery checkpoint `0215379bff2dbb5478def539dd8aa670d35e4e93` and completion checkpoint `b0945483e4f50ca065c0b7c3a0d9f917a2b375e6` were pushed and the work-branch ref read back. The latter's remote tree `554cde3772af4b602c11b9d65abd8d758687211f` exactly matches the staged local source. Source-only archives and isolated test copies are retained locally. Always repeat this verified checkpoint cycle before testing changed source.

Original-rule checkpoint `8adeaeebe4a0380b92ceceff154e30ab5069696c` was subsequently pushed and read back; tree `9c5d5f0f3c5e96c6f96457dc604518b882c7bf28` matched the complete staged source. Its frozen test/build root is `/tmp/nspire-stage-8adeaeeb`. Follow-up checkpoint `afe33815cfc9f9ab24bbf457cdaeed0cd0e3f2df` was pushed and read back; its complete remote tree `67a0b819cee92d14fec31bd5a8045ec8993cb38d` matched local source. It adds batch auto-play reservations, full-hand pile selection, Touch eligibility and Doom/Demise ordering. Frozen validation root: `/tmp/nspire-stage-afe33815`. The final tested `b4b67e71` stage records turn-start acquisition order explicitly in save v12 and fixes temporary costs before enemy-side choices.

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

The completed baseline, `b0945483`, `8adeaeeb`, `afe33815` and `b4b67e71` validation/build journals report `source_changed: false`. The latest native journal is `/tmp/nspire-stage-b4b67e71/build/journal/native-final-6173a9899340.json`; the reviewed local summary is `/workspace/scratch/5af9d4bf40b9/nspire-validation-b4b67e71.json`. The final v12 stage passed at `b4b67e71`. A genuine v11 fixture was produced by the unchanged `afe33815` writer under journal `v11-frozen-fixture-3c145f292253`: 12,580 binary bytes, SHA-256 `39469ff1e754da978689eab2e849f85f32fbab61138b2bdfc10bee5664c68a4f`. The passing migration regression reads it instead of regenerating historical data with the new codec. Test source alone does not establish a passing result, and agreeing native/WASM outputs would not establish original-game parity.

## Recovered original inputs and build work

The old missing-volume blocker is resolved. Four complete split ZIP volumes were recovered and streamed through ZIP extraction without constructing another combined archive. The actual release metadata identifies `v0.111.0`, commit `41cef1ea`, and the following extracted inputs match the project's locked SHA-256 values:

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `sts2.dll` | 9,757,184 | `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9` |
| `SlayTheSpire2.pck` | 1,990,705,700 | `c60f672ee7804e6aefa1e19a582fa1c80b126a7b0eef4d084d3abf110df2eab7` |
| Existing NSP2/10 base pack | 6,057,527 | `e157ee346a4eb1fc2849bc2d18e5dc1f4ccf364b85386d92dc7306a1430908a5` |

These private inputs are outside the Git checkout under `/workspace/scratch/5af9d4bf40b9/reference-inputs`. Do not upload original archives, PCK data, assemblies, fonts, game-derived packs or generated HTML with embedded original art to the source repository. The old NSP2/10 pack is only a conversion input, not a compatible resource pack for the new launcher.

The real NSP2/11 conversion passed at frozen `b0945483` without changing conversion source. The new pack is **6,584,524 bytes**, SHA-256 `7add6827c5a8a271f0f411f1797a2a75cb6b9db36f450a7402f667ff789a12b4`: 114 cards, 407 strings, 185 auxiliary images, 1,460 images, 5,770 glyphs and 17 clips. All 1,334 earlier image payloads are preserved; 1,804,084 bytes remain below the resident 8 MiB pack limit. The conversion journal `real-content-pack-19a1867ba47b` and actual-pack native Content/App rendering journal `real-content-runtime-a2781fd4f4c7` passed with unchanged source. Synthetic packs remain parser/UI test fixtures only.

LLVM 21.1.8 was obtained from official Noble packages, with signed-index and per-package SHA verification, and unpacked privately under `/tmp/nspire-toolchain`. At final `b4b67e71`, ARM compiled to a 152,721-byte launcher with 10,564,912-byte allocation and 2,124 absolute relocations. WASM compiled to 493,402 bytes and the X11 host to 498,800 bytes. The self-contained offline HTML is 9,452,257 bytes. The content conversion repeated with the same NSP2/11 hash.

Four existing room scenarios produced 84 exact native/WASM RGB565 frame matches and eight exact save matches. This checks the shared engine/storage adapter, not browser DOM/localStorage or the original game. At `afe33815`, X11 fresh-process save/reopen passed with both a 14,340-byte ordinary save and a 19,424-byte / 384-instance expanded v11 save. Both checks passed again at `b4b67e71` with the actual v12 writer: ordinary 14,344 bytes and expanded 19,428 bytes, including exact re-saved bytes in a fresh process. No new Firebird execution or physical-device performance result has been established.

## Handoff and remaining development

The requested 0.14 colorless/potion stage has passed its defined validation and has matching calculator and offline-browser builds. The work branch contains source and audit evidence; private game-derived outputs remain outside Git. Main and disabled CI have not changed.

Further development should begin from this verified work branch: expand the three missing character systems and full character pools, implement original special-potion acquisition events, replace adapted encounters/run structure, and validate a future build in Firebird and on physical hardware. Continue the checkpoint-before-build cycle for any new source changes.
