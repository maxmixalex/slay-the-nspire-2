# Source10 checkpoint evidence — 2026-10-21

Human-readable times use Asia/Shanghai (UTC+08:00). Machine journals retain their original UTC timestamps. The reference remains v0.111.0 / 41cef1ea.

## Verified source9 persistence

The GitHub connector resumed writing. At **2026-10-21 10:50:57 Asia/Shanghai**, `work/0.17-world-parity` read back as [`7efafe07e956bba6a49b2c9f0cca85100c7e9598`](https://github.com/zyf722/slay-the-nspire-2/commit/7efafe07e956bba6a49b2c9f0cca85100c7e9598), tree `05a8e3db7ae31e5ca897e309ffc404d749599db0`. All 438 file paths, modes and blob hashes matched the committed local tree; all 49 uploaded file contents were separately read back and matched exactly. Main remained `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0`.

The API checkpoint has the exact tree of local source9 `aa3badfd794f971f7fd1f6a093c739bff8679f03`, whose original three-stage history remains on `checkpoint/0.17-room-presentation-source9`. The remote API checkpoint uses the existing remote resource commit `6b2e83c651055480a3150e545241769be99459e2` as parent. The different commit identities are recorded explicitly; they are not presented as the same commit. A detached worktree at `/tmp/nspire-stage-7efafe07` was reconstructed and all 438 committed blobs were verified before validation.

Remote receipt: `source9-remote-readback-20261021.json` outside the checkout; SHA-256 `4036a85cd204df39288c33947525daca09355098fbcb6324ff1e8998f99bf467`. Frozen source fingerprint: `4a73e410b5552913ab20c6ee3e823308d01c2990e1b68ce3b22b61e905813aad`.

## Formal validation of source9

All stages below used the frozen source and `tools/run_logged.py --checkpoint 7efafe07e956bba6a49b2c9f0cca85100c7e9598`. All completed journals report `source_changed: false`.

| Journal | Result | Elapsed seconds | Log SHA-256 |
| --- | --- | ---: | --- |
| `source9-arm-0ea2f2a847ef.json` | failed / exit 1 | 46.221 | `e8ef2c29b2ca3800e21e7f426b3a9e980465efed6affead8b82a017ba499a0de` |
| `source9-native-build-b5d2795d6000.json` | passed / exit 0 | 125.196 | `1ff40d1d550d94ba9a688dff8c63790e0e921dd7187c8f6a9533eab0338adcb1` |
| `source9-native-configure-821a80828c57.json` | passed / exit 0 | 4.336 | `0a83691675f448df97485244325efcbe6e0097c03b47c610bbec3ccb1e10cf4d` |
| `source9-native-risk9-708c78e1dfc9.json` | failed / exit 8 | 81.609 | `2b9367c8c04c1b1e73f61998a4b5897505a95ca63d4cce77a230797d90247b33` |
| `source9-wasm-2bf4cbf5422f.json` | failed / exit 1 | 43.516 | `5c8fee9a1552d14516095f3bfa545b11b310198d5920dde236cb53ebaecdbc98` |

Native Debug ASan/UBSan construction completed **122/122 build steps**. LeakSanitizer was explicitly disabled because this execution environment uses ptrace; ASan and UBSan remained enabled. ARM and WASM both stopped while compiling `src/app.cpp` because `include/world_room_assets.hpp` included the unavailable C++ standard-library header `<cstdint>`. Neither failed cross-target attempt produced a validated new runtime.

The targeted native group completed **7 passed / 2 failed**, with no timeout. Its original failing journal and JUnit are retained. The other 37 suites were not started on this checkpoint; they will run as part of the next complete, corrected checkpoint.

| Risk test | Result | Seconds |
| --- | --- | ---: |
| `ironclad_completion_regression` | passed | 4.382 |
| `five_character_integration_regression` | passed | 0.396 |
| `five_character_ui_regression` | passed | 28.565 |
| `enemies_regression` | failed | 77.130 |
| `world_game_over_ui_regression` | passed | 8.802 |
| `world_room_ui_regression` | failed | 0.707 |
| `presentation_blend_regression` | passed | 0.018 |
| `card_result_regression` | passed | 0.577 |
| `world_frontend_ui_regression` | passed | 20.171 |

The two failures were diagnosed separately:

- **Rest presentation lookup:** the original runtime region key uses names such as `Overgrowth`, while rest image keys use `OVERGROWTH`. The previous Camp lookup missed the registered image, used the historical fallback and skipped the new native actors. The same case mismatch affected ordinary combat region backgrounds; encounter-specific uppercase keys already followed the correct priority. This requires a presentation-code repair, not a weakened pixel assertion.
- **Hopper fixture:** the synthetic deck directly set `energy_offset = -1` while leaving enchantment as `None`, so validation failed before any card was stolen. The strict runtime validator correctly requires that persistent discount to belong to Tezcataras Ember. The repair creates the real enchantment and preserves its captured/returned card metadata. Original Hopper stealing uses forced deck removal and does not apply the ordinary Eternal-removal restriction.

The room failure occurred before the later Defect plane and treasure-transition assertions; those assertions are not counted as executed. The seven completed suites and their detailed check counts are available in the separate risk receipt.

Risk receipt: `source9-native-risk9-receipt.json`, SHA-256 `49252857e6b064977399d18595d2891bbc40d1e32e75a96c5403f74c1a579a34`; risk journal SHA-256 `26ad01fa969004f8d4e68ef9b2298b3bbbe89b32ea0c4226c24fc704215d3930`; JUnit SHA-256 `704c9b6d62ddec8fb99a8b1ad02c65567a92a0cd12ba1961363fc809741d61a6`.

## Source10 corrections and bounded diagnostics

The freestanding repair changes the three presentation headers and both header generators to `<stdint.h>`. It changes neither integer types nor algorithms. Generated-header validation and ARM/WASM `src/app.cpp` syntax checks passed before the subsequent room repair; those checks are not cross-target linking or execution.

The header-only short receipt is `source10-short-diagnostics-20261021/receipt.json` outside the checkout, SHA-256 `f08481ed2015e0865a55a73a06549d3588288560ed092bcbe879e666de47dbb1`; log SHA-256 `12892af96f76d8184bcc7365489a07f85aef8331cd695911b47d928812237bae`. Its source fingerprint is `40d494f3060e2b4e717d949b85ce7fb5602951edc48f512ec15aab65ed840fe2`, and it reports no source changes during execution.

The Hopper fixture repair uses `event_card_enchant(..., TezcatarasEmber, 1)` and explicitly checks the captured/returned enchantment, amount, discount, upgrade and Genetic growth. An untagged nonzero discount remains a rejected-state negative case. The new enemies test TU, linked to the frozen source9 core, passed **289 assertions** in the `stolen_assets_and_timeout` subset in **0.380876 seconds**. No runtime validator or theft/reward code changed. Its receipt `source10-hopper-fixture-repair-receipt.json` has SHA-256 `571744b1e8898f9fd371e388390ecbfde2fcb43acf6258538a291bb260b72df7`; the run log SHA-256 is `4e09c575f9db182b8bbe394c375aeb992c2aa50ac9ee853ec66d1523469b0443`; the test source SHA-256 is `3a31fe673b58a0fa3bc8dbc84e3efde57603b06f042e3a7b973d0503a19ca43e`.

The presentation repair introduces one uppercase image-key helper used by Camp and ordinary combat. It preserves the original model region names and encounter-specific background priority. Tests assert the exact registered lit/dark background pixel for all four regions, all four ordinary combat backgrounds and The Kin's dedicated boss background, with state/RNG purity checks.

An isolated new frontend and room test linked to the unchanged source9 archive passed the earlier room sections but **failed** at the later Dolly pickup state validation (`Phase::WorldRelicChoice`). Its run lasted 15.274555 seconds; it is not a full room-suite pass. Compilation took 42.310762 seconds for the new frontend and 5.238814 seconds for the room test, and linking took 0.502369 seconds. The link map confirmed the new App object was selected and the old archive App member was not. The attempt retained `source_changed: false` and `frozen_archive_unchanged: true`; log SHA-256 `d7007ecee5aab5cf423746905b55c8588c0489dbcdf48b6dcaee3c363698eba9`. Separate trace/tail diagnostics do not turn this failing attempt into a completed suite.

### Claimed native chest awaiting a relic choice

The old generic predicate rejected any non-Treasure phase with nonzero `treasure_state`. The native acquisition sequence first marks the chest claimed and grants its relic, then may wait for that relic's card selection. The real Dolly continuation was `WorldRelicChoice / Duplicate / return_phase=Treasure`, with `treasure_state=2`, an open native Treasure room and no legacy treasure relic mask. Dolly was already owned and removed from both relic bags. Both `world_valid_run` and `valid_world_relic_state` passed. In a diagnostic copy, only clearing `treasure_state` made the old overall validator pass, identifying the exact predicate.

The source9 frozen-core probe passed **53 assertions in 1.480804 seconds**. Its log SHA-256 is `e46026a2faef633bab8d807e212db578268a17ef3b366fcace24caff458eab50`; journal SHA-256 `5d3719cbc144329cb6b15c83adbf92a58c010e608d386d0232ba4ef4a04a7945`. This demonstrates the old defect; it is not execution of the new validator. Dolly normally belongs to the Shop pool. The diagnostic explicitly injects its acquisition model into a real map Treasure room to exercise the awaited callback, and makes no claim about ordinary treasure rarity selection.

Source10 permits the exception only for active native run/relic state, an open Treasure room, claimed state `2`, no legacy relic mask, an actual nonempty `WorldRelicChoice` and a Treasure return phase. The predicate reads direct fields without indexing arrays or invoking a helper on unvalidated map indices. All existing room, relic and remaining state validators still run. There are no State-layout, codec, RNG or acquisition-operation changes.

The added map test covers canonical pending save/load, removal from both bags, no repeated claim or premature exit, selecting once and obtaining exactly one duplicate/one relic, normal room cleanup, preservation of all named RNG words/counters, and wrong-phase/room/inactive/unclaimed negative cases. These added cases have **not yet executed** at this checkpoint. Short syntax checks of the updated core and map test passed in **1.092021 seconds**. The combined repair receipt `source10-dolly-repair-receipt.json` has SHA-256 `7ce34023d96b370d3a0b6108d1eb96615b316a6c51830008f99760c50b7573a3`.

All isolated diagnostics remain distinct from the next complete frozen native/cross-target build.

## Conversion and release boundary

The enemy prefetch resumed from the verified source9 checkpoint with two workers, the reviewed private inputs and strict existing cache validation. Its scan identified **122 reusable clips and 1,478 missing clips**, out of 1,600 planned enemy clips / 33,210 frames. All 327 per-model bounds/reference/batched cache closures matched between frozen source9 and the header-repaired source at that comparison. This explains cache reuse; it does not declare any future frame complete. The running prefetch journal remains unfinished until it reports a terminal result.

No complete new NSP2/14 pack, matching desktop HTML/calculator ZIP, original-game GPU comparison, physical-calculator performance result or 95% UI similarity result is established here. Keep the completed 0.16 delivered pair until a matching corrected runtime and complete pack pass their own checks. The next source checkpoint must be pushed and its ref/tree/changed files read back before formal source10 rebuilding.
