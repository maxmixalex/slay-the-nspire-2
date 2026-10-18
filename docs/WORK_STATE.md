# Work state — 2026-10-18

## Active branch and recovery receipt

Continue on `work/0.14-colorless-potions`. The user prioritizes GitHub persistence before long validation, then completion of potions and single-player colorless cards, with parallel agents. Main remains `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0` (historically tested 0.13). CI remains disabled. Reference remains **v0.111.0 / 41cef1ea**. Audio, multiplayer and Timeline are excluded.

The accessible scratch workspaces did not contain the earlier checkout. All 163 source files were restored from work-branch commit `50a58c9b876c3038e549b97e7e7d245224d717d8`. Every Git blob and the complete tree `2fb04a1bb26b50d96d6741cbe2cba17f4a9366cc` were verified, and the exact shallow Git commit was reconstructed. The current working directory is `/workspace/scratch/5af9d4bf40b9/slay-the-nspire-2`.

Recovery checkpoint `0215379bff2dbb5478def539dd8aa670d35e4e93` and completion checkpoint `b0945483e4f50ca065c0b7c3a0d9f917a2b375e6` were pushed and the work-branch ref read back. The latter's remote tree `554cde3772af4b602c11b9d65abd8d758687211f` exactly matches the staged local source. Source-only archives and isolated test copies are retained locally. Always repeat this verified checkpoint cycle before testing changed source.

## Current implementation scope

The engine registers 53 single-player colorless card models, all 64 active potion models, and generated Soul/SovereignBlade tokens: 114 card IDs total, 30 relics and two partially implemented playable characters. These are implementation counts, not a claim that the entire original game is complete. Nine other-character potions and four special-acquisition potions can be exercised explicitly in practice; the other three playable character systems and original special event routes remain outside this stage.

The source writes save v11 and requires NSP2/11: 114 cards, 407 strings and 185 auxiliary images. Historical v3-v9 fixture bytes have not been changed. Extended selections use model tokens for offers and instance IDs for pile choices, and support suspended save/resume. Practice now has a direct HUD compendium entry, card/potion acquisition buttons and accurate full-hand feedback. Acquisition is guarded against use from non-practice runs, the title screen or a paused game.

At `b0945483`, temporary/permanent free-cost precedence and queue-capacity checks were corrected. Old test expectations were migrated to the expanded practice/content/save profiles, and separate colorless, potion and UI acquisition regressions were added.

The next coherent source stage has been audited against the actual pinned assembly. It corrects random card/potion generation restrictions, distinct generated batches, original potion rarity weights, BeatDown's captured discard selection, Mayhem's turn-start ordering, Anointed's hand capacity, temporary costs expiring on the first completed play or turn end, X-cost precedence, Plating's turn-end-only block, and damage-copy effects retaining overkill. Fairy now participates in lethal event HP loss as well as combat. These repairs and their new assertions await execution at this source checkpoint.

Browser saves now use an isolated v11 key with v3-v10 import fallback, and the X11 reader accepts the full expanded save size. The full content-build entry point now includes the final NSP2/11 conversion. No new save fields or format increment were required. Potion and colorless agents own their model logic; the integration owner owns shared state, save compatibility and checkpoints.

## Fresh validation results

All native commands run through `tools/run_logged.py` against a verified frozen source snapshot. GCC AddressSanitizer and UndefinedBehaviorSanitizer remain enabled. LeakSanitizer alone is disabled because it cannot inspect the sandbox's process/thread information.

| Source checkpoint | Result | Meaning |
| --- | --- | --- |
| Recovered `50a58c9` | All 18 C++ regressions compiled; 8 passed, 10 failed at runtime. | Failures exposed old save/profile assertions and stale fixture/driver setup; this was not a clean baseline. |
| `b0945483` | All 21 C++ regressions compiled; 16 passed, 5 failed at runtime. | Four older random-play drivers did not resolve ExtendedChoice; the new colorless fixture also used enemy HP outside the engine's legal encounter bounds. Repairs are awaiting the next checkpoint run. |
| `b0945483`, potion completion | 7,573 checks passed across 64 model policies. | Numeric effects, availability, selection, acquisition, continuation and save/resume within this engine. |
| `b0945483`, expansion UI | 4,852 checks passed. | All 106 colorless variants and 64 potion entries can be acquired through the actual UI handlers; shop/reward transactions and guards are exercised. |
| `b0945483`, combined content behavior | 7,832 checks passed, 106 card variants and 64 potions. | Initial model-level assertions; not an original-game replay comparison. |

The baseline and `b0945483` journals report `source_changed: false`. The latest full journal is `/tmp/nspire-stage-b0945483/build/journal/native-all-005f0097da17.json`. Test source alone does not establish a passing result, and agreeing native/WASM outputs would not establish original-game parity.

## Recovered original inputs and build work

The old missing-volume blocker is resolved. Four complete split ZIP volumes were recovered and streamed through ZIP extraction without constructing another combined archive. The actual release metadata identifies `v0.111.0`, commit `41cef1ea`, and the following extracted inputs match the project's locked SHA-256 values:

| Private input | Bytes | SHA-256 |
| --- | ---: | --- |
| `sts2.dll` | 9,757,184 | `0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9` |
| `SlayTheSpire2.pck` | 1,990,705,700 | `c60f672ee7804e6aefa1e19a582fa1c80b126a7b0eef4d084d3abf110df2eab7` |
| Existing NSP2/10 base pack | 6,057,527 | `e157ee346a4eb1fc2849bc2d18e5dc1f4ccf364b85386d92dc7306a1430908a5` |

These private inputs are outside the Git checkout under `/workspace/scratch/5af9d4bf40b9/reference-inputs`. Do not upload original archives, PCK data, assemblies, fonts, game-derived packs or generated HTML with embedded original art to the source repository. The old NSP2/10 pack is only a conversion input, not a compatible resource pack for the new launcher.

The real NSP2/11 conversion passed at frozen `b0945483` without changing conversion source. The new pack is **6,584,524 bytes**, SHA-256 `7add6827c5a8a271f0f411f1797a2a75cb6b9db36f450a7402f667ff789a12b4`: 114 cards, 407 strings, 185 auxiliary images, 1,460 images, 5,770 glyphs and 17 clips. All 1,334 earlier image payloads are preserved; 1,804,084 bytes remain below the resident 8 MiB pack limit. The conversion journal `real-content-pack-19a1867ba47b` and actual-pack native Content/App rendering journal `real-content-runtime-a2781fd4f4c7` passed with unchanged source. Synthetic packs remain parser/UI test fixtures only.

LLVM 21.1.8 was obtained from official Noble packages, with signed-index and per-package SHA verification, and unpacked privately under `/tmp/nspire-toolchain`. Final ARM/WASM builds await the next verified semantic source checkpoint. No new Firebird execution or physical-device performance result has been established.

## Next actions

1. Complete the current original-rule audit, persist the coherent repairs and verify the branch ref and source tree.
2. Run all 21 native regressions on that frozen snapshot; resolve actual failures and record the results.
3. Compile ARM and WASM from a verified final source checkpoint and exercise the resulting real content/UI path.
4. Persist reviewed result summaries and remaining scope boundaries. Keep CI disabled and leave main unchanged until the work branch has a sufficiently verified deliverable.
