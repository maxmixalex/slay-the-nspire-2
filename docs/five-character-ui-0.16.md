# Five-character presentation — 0.16

## Source scope

The UI now exposes Ironclad, Silent, Defect, Regent and Necrobinder through character selection, normal runs, practice, compendium filters, map markers, combat art and camp animations. It uses the existing append-only model directory through model 496. `CardKind` remains wide throughout the compendium and permanent-deck previews; `card_token` and `card_from_token` carry upgraded state without truncating models above 255. The five character-color filters contain 85 Ironclad cards, 87 Silent cards including generated Shiv, and 86 cards for each additional character. Separate colorless and all-model filters remain available.

Regent card previews display energy and independent star costs, including star X costs. Combat affordability uses both resources, and descriptions use the selected live instance. Choice metadata for an upgraded transformed minion no longer incorrectly zeroes the original card's energy cost. Stars have a persistent Regent HUD entry, including when the count is zero.

Defect combat renders the five actual orb types in their ordered slots, with the oldest orb on the right. Orb details show passive and evoke values with current Focus and stored Dark or Glass amounts; up to ten slots are readable through a second page. Necrobinder combat renders Osty's current and maximum HP, attack, hurt, death and revival feedback, and the player's own Doom. The status directory covers all 24 Defect, 22 Regent and 22 Necrobinder player powers, plus Conqueror, Debilitate, Hang, Oblivion and Sic 'Em on enemies. Long status descriptions and the orb list have explicit paging.

Permanent-deck details retain the selected deck index. Genetic Algorithm's stored Block and The Scythe's stored Damage therefore use the correct physical deck card, including in upgrade previews, without applying combat Strength, Dexterity or enemy Vulnerable. Compendium previews retain ordinary model values. Live instances retain their existing modifiers, including the shared Sovereign Blade and its enchantments.

Forbidden Grimoire rewards open an optional permanent-deck removal flow. Eternal cards are disabled. The confirmation captures the run revision and the selected deck index, and each successful removal or explicit skip consumes one opportunity. Back closes the list without spending an opportunity. Starter relic presentation uses the full 64-bit relic mask, with the new index-based log records understood by the UI.

## Resource contract

The intended resource profile is NSP2/13: 497 models, 1,556 strings, 339 auxiliary images and 38 animation clips, with a 13 MiB input limit. The complete current-profile check includes animation count. All earlier auxiliary and animation IDs retain their meanings; the three new character sets, orb art, Osty and character powers append to them. Original resource conversion and its input hashes are documented separately by the resource module. No original asset payload or generated pack is stored in the source repository.

## Verification status

The source contains `tests/five_character_ui_regression.cpp`, registered as a native CMake test. With no arguments it uses a synthetic current-profile resource fixture. An optional first argument loads an actual resource pack, and an optional second argument writes PPM review scenes into an existing directory. The suite drives actual keyboard and pointer actions, checks read-only rendering against serialized state, exercises all five character filters and appended generated models, and covers resource costs, transformations, five orb types, Osty, permanent growth, optional Grimoire removal and relic-mask width. Existing character UI acquisition coverage is extended separately to all 860 normal and upgraded pool/Giant Rock variants.

These are authored coverage statements. No 0.16 compilation, execution, screenshot inspection or resource conversion is claimed by this document. Validation must use a pushed, read-back, immutable source checkpoint and the required `tools/run_logged.py` journal. Failures and the exact passing checkpoint, if established, belong in the project validation receipt and `WORK_STATE.md`.
