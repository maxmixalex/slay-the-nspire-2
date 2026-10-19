# Five-character presentation — 0.16

## Source scope

The UI now exposes Ironclad, Silent, Defect, Regent and Necrobinder through character selection, normal runs, practice, compendium filters, map markers, combat art and camp animations. It uses the existing append-only model directory through model 496. `CardKind` remains wide throughout the compendium and permanent-deck previews; `card_token` and `card_from_token` carry upgraded state without truncating models above 255. The five character-color filters contain 85 Ironclad cards, 87 Silent cards including generated Shiv, and 86 cards for each additional character. Separate colorless and all-model filters remain available.

Regent card previews display energy and independent star costs, including star X costs. Combat affordability uses both resources, and descriptions use the selected live instance. Choice metadata for an upgraded transformed minion no longer incorrectly zeroes the original card's energy cost. Stars have a persistent Regent HUD entry, including when the count is zero.

Defect combat renders the five actual orb types in their ordered slots, with the oldest orb on the right. Orb details show passive and evoke values with current Focus and stored Dark or Glass amounts; up to ten slots are readable through a second page. Necrobinder combat renders Osty's current and maximum HP, attack, hurt, death and revival feedback, and the player's own Doom. The status directory covers all 24 Defect, 22 Regent and 22 Necrobinder player powers, plus Conqueror, Debilitate, Hang, Oblivion and Sic 'Em on enemies. Long status descriptions and the orb list have explicit paging.

Permanent-deck details retain the selected deck index. Genetic Algorithm's stored Block and The Scythe's stored Damage therefore use the correct physical deck card, including in upgrade previews, without applying combat Strength, Dexterity or enemy Vulnerable. Compendium previews retain ordinary model values. Live instances retain their existing modifiers, including the shared Sovereign Blade and its enchantments.

Forbidden Grimoire rewards open an optional permanent-deck removal flow. Eternal cards are disabled. The confirmation captures the run revision and the selected deck index, and each successful removal or explicit skip consumes one opportunity. Back closes the list without spending an opportunity. Starter relic presentation uses the full 64-bit relic mask, with the new index-based log records understood by the UI.

## Resource contract

The validated resource profile is NSP2/13: 497 models, 1,556 strings, 339 auxiliary images and 38 animation clips, with a 13 MiB input limit. The complete current-profile check includes animation count. All earlier auxiliary and animation IDs retain their meanings; the three new character sets, orb art, Osty and character powers append to them. Original resource conversion and its input hashes are documented separately by the resource module. No original asset payload or generated pack is stored in the source repository.

## Verification status

The source contains `tests/five_character_ui_regression.cpp`, registered as a native CMake test. With no arguments it uses a synthetic current-profile resource fixture. An optional first argument loads an actual resource pack, and an optional second argument writes PPM review scenes into an existing directory. The suite drives actual keyboard and pointer actions, checks read-only rendering against serialized state, exercises all five character filters and appended generated models, and covers resource costs, transformations, five orb types, Osty, permanent growth, optional Grimoire removal and relic-mask width. Existing character UI acquisition coverage is extended separately to all 860 normal and upgraded pool/Giant Rock variants.

The first formal repair gate at pushed/read-back checkpoint **`79b17017674cbbc02e8255e9bdc714c87472dfe6` passed all eight suites**, with ASan and UBSan enabled. The five-character UI suite passed **1,687 checks using its synthetic profile**. Its repairs include settling generated-card callbacks before the lab API returns, rejecting unaffordable star costs before modifying the relic feedback mask, and recognizing the third row of the expanded relic directory before its caption. Fixed-resource selection previews also use the shared cost pipeline: Borrowed Time can leave one energy after a local zero Set while the selected fixed star cost becomes zero.

| Formal repair-gate suite | Printed checks | Scope reported by the suite |
| --- | ---: | --- |
| `shared_ui_regression` | 90 | Shared directory and confirmation flows; no-asset mode |
| `relic_combo_regression` | 80,442 | 2,776 transitions |
| `colorless_potions_regression` | 7,903 | 106 card variants and 64 potions |
| `colorless_completion_regression` | 17,795 | 106 explicit card-effect variants and continuation/token contracts |
| `defect_completion_regression` | 16,190 | 172 canonical variants |
| `regent_completion_regression` | 9,892 | 181 base/upgraded/token variants |
| `necro_completion_regression` | 19,494 | 174 canonical variants |
| `five_character_ui_regression` | 1,687 | Synthetic current-profile fixture |

The 153,493 total is the sum of the printed checks, not a count of unique scenarios. Journal `native016-targeted-78c63eab0b28` finished with `source_changed: false`. Its log SHA-256 is `80b5c30551d9e33a0bd455856d7abf976927f3b133176fec953eb62254a22018`; the common source fingerprint is `42524961055aec1040238ee5266de9c57a3c8ffc1a5243eaabe71c4cbaea4853`.

## Final actual-resource verification

The final pushed/read-back runtime checkpoint is **`8c6977d3cc1dc9f17926a480127bc1067abd0286`**, tree `db96da21a4f39ae8612857fab5c2ea1b4e1295a3`. Its full native ASan/UBSan gate passed **31/31 suites**, including the 860-acquisition synthetic run. The three resource-aware UI suites were then executed separately against those same frozen binaries and the actual **12,910,778-byte NSP2/13 pack**, SHA-256 `4b83320460798a5dd57d4faeee3cdf9613bef7413259ac57b61fb74d109a5917`.

| Actual-resource suite | Printed checks | Exercised scope | Captured scenes | Journal |
| --- | ---: | --- | ---: | --- |
| `five_character_ui_regression` | 7,173 | Five characters, appended generated/status cards, stars, transformations, orb paging, Osty, permanent previews, Grimoire and all role statuses | 22 | `actual-five_character_ui_regression-954ab779863c` |
| `expansion_ui_regression` | 7,375 | All 106 colorless variants and 64 potion acquisitions, choices and live Sovereign Blade instances | 9 | `actual-expansion_ui_regression-83001031cb13` |
| `character_ui_regression` | 66,478 | All 860 normal/upgraded character-pool and Giant Rock acquisitions; retained instance and keyword previews | 9 | `actual-character_ui_regression-cfdc7891ecf9` |

All three completed successfully with **81,026 printed checks** and `source_changed: false`; this sum is not a unique-scenario count and is separate from the full native gate. Their common source fingerprint is `9fadd7740d7abde1d3144e293f22a6846476d84b6cefe74927f3065182a5ea2e`. Their log SHA-256 values, in the table's order, are `a77df148cc270f2bdce5acf741ca4a9ef1f8681f99909365fa1e69516295cfde`, `3c5e140913778945ef66cbf8522dfd1a15574c286c66bed1b264e77652d1dc38` and `a8f3313a21a027307adbfefba5377f9cccabe98262ef1edc83e9f908a52c03fe`. The retained expansion test's success sentence still says “NSP2/11”; its actual input is the v13 pack identified above, accepted by `Content::current_profile()`.

All **40 captured 320×240 scenes were visually inspected**. The five-character layouts, Chinese text, original character art, color frames, independent energy/star markers, star X, ordered orb icons and detail paging showed no blocking overlap or missing art. The review also checked Sovereign Blade's special frame and 25/47 live damage values, Genetic Algorithm and The Scythe's selected permanent-card values, optional Grimoire confirmation, the role-status panel, and retained Ironclad/Silent/colorless/potion screens. This verifies these rendered scenes; it is not an original-game pixel-comparison claim.

The captures named `necro-osty-down` and `necro-osty-revived` deliberately show the **first frame** immediately after the HP transition. A separate small diagnostic, linked to the same frozen App/core and using the same pack, advanced the complete death and revival clips. Death is 17 frames × 78 ms = 1,326 ms; after 41 ticks, the zero-HP sprite is absent and only the subdued status icon remains. Revival is 12 frames × 81 ms = 972 ms; after 30 ticks, the five-HP sprite returns to Idle. Both end-state frames exactly match rendering at the same time with no active one-shot animation, and rendering/ticking preserve the serialized game state. Both supplemental end-state captures were visually inspected. Journal `osty-clip-end-diagnostic-89d393fb2fb5` passed 540 diagnostic checks with unchanged source; its log SHA-256 is `cc63c40cbc993484c09baa02db495f11d892c65d4baa08c340a60ad7e943adbe`. These diagnostic checks are not added to the three formal UI suite counts.

The full native journal is `native016-ctest-54a1d9ac8e6f`, with log SHA-256 `edd6c06add6696ba140bc3c6af64849db990deaeba8d58d4a4bd3f6a0edfd38a`. Resource parser, backend comparison, fresh-process storage, ARM compilation and remaining device-execution boundaries retain their own receipts in the final project validation record and `WORK_STATE.md`.
