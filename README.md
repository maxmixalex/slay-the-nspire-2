# Slay the Nspire 2

Native TI-Nspire Ndless adaptation of Slay the Spire 2. Reference: the owner's hash-pinned **v0.111.0 / 41cef1ea** installation. Current source: **0.15 complete Ironclad and Silent pools**, on `work/0.15-full-characters`. This first implementation checkpoint is awaiting validation; the last verified distributed build is 0.14.

**Audio, multiplayer and Timeline are excluded.** Original UI fidelity, animation and single-player content breadth remain in scope. Original game data, fonts and calculator images are not included in public source.

## Current content

228 card models: **85 single-player Ironclad**, **86 single-player Silent**, **53 single-player colorless** and four generated tokens (Shiv, Soul, Sovereign Blade and Giant Rock). There are **64 active potion models**, 30 relics and five adapted events. The two character pools include their base and upgraded rules. Ancient-rarity Suppress and Wraith Form are available in the compendium/practice pool and excluded from ordinary rewards and generation. The twelve-floor playable route and Nibbit encounters remain adapted; the reference regional map is still separate from a complete original run.

Practice exposes every character/colorless card and potion through the compendium. Open it with **C** or the practice HUD button, inspect an entry and use its acquisition button. Card variants, potion targeting, multi-card selections, generated offers, nested automatic plays and persistent statuses share the same engine and UI on native and browser builds. A full hand sends newly created cards to discard; a full potion inventory must be freed before acquiring another bottle.

Ordinary potion pools follow the current character and the pinned rarity weights. Other-character and special-acquisition bottles remain available for explicit practice use; this does not add the missing three characters or their original event routes. The 0.13 event, relic, treasure and fanned-hand UI work is retained. These layouts are adaptations, not complete original-PC fidelity.

Read [current work state](docs/WORK_STATE.md) before resuming development and the [colorless](docs/colorless-completion-0.14.md) and [potion](docs/potions-completion-0.14.md) audits for tested behavior and original-rule evidence. Registry counts alone are not a passing test or a claim that the entire original game is complete.

## Build

Remote CI is disabled by request. Its definition stays in `.github/disabled-workflows/ci.yml`.

```sh
# Set this to the full commit whose remote branch and source tree were verified.
NSPIRE_CHECKPOINT='<verified-full-commit-sha>'
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label arm -- python3 tools/build_ndless.py
cmake -S . -B build/host -DSPIRE_X11=OFF
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label native-build -- cmake --build build/host --parallel
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label native-tests -- ctest --test-dir build/host --output-on-failure
python3 tests/event_pack_regression.py
python3 tests/logged_command_regression.py
python3 tests/character_pack_regression.py
```

ARM requires LLVM clang++ and ld.lld; the browser build also requires wasm-ld. The X11 host is optional. Install converter dependencies from requirements.txt. Convert an existing private 0.14 NSP2/11 pack without resampling its retained animation frames:

```sh
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label content -- python3 tools/build_characters.py /path/to/game /path/to/old/nSpire2.nsp.tns --output dist/nSpire2.nsp.tns --font /path/to/local-font
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label web -- python3 tools/build_web.py
```

For a full conversion from the pinned installation, `build_content.py` now chains through the same final character conversion. The conversion checks the original release metadata, assembly and PCK hashes. New launchers require **NSP2/12** (228 cards, 233 auxiliary images and 749 strings). Put `nSpire2.tns` and `nSpire2.nsp.tns` in the same calculator directory; `nSpire2-desktop.html` is the self-contained offline browser build. Replace the launcher and pack together.

New saves use **v13**; project v3-v12 saves remain readable. v13 appends explicit character powers, histories, per-instance modifiers and Nightmare snapshots. Historical writers did not record the new character histories, which start at zero on migration. v12 records the acquisition order between Entropy and Rolling Boulder. Earlier v11 saves did not record that order and retain the previous engine's all-Boulders-before-Entropy ordering when migrated. The browser writes to a separate v13 storage key and can import earlier keys. Old launchers cannot read the new save. Keep a copy of an older save when testing a new launcher.

The native suite includes complete character, colorless, potion, historical migration and UI regressions. `expansion_ui_regression` optionally accepts a real NSP2/12 file and an existing private screenshot directory, so the same acquisition/choice tests can exercise converted art in addition to synthetic parser fixtures. `character_ui_regression` accepts the same two optional arguments for all 344 character/token variants.

## Persistence and validation

**0.15 validation is pending.** Historical verified 0.14 build source: [`b4b67e71`](https://github.com/zyf722/slay-the-nspire-2/commit/b4b67e714ca76e7afb72f3a170789ad90047e6f4). All **21 C++ suites and 10 Python tests passed**, with ASan/UBSan on native regressions. Real-content UI exercised 106 colorless variants and all 64 potions; native/WASM room comparison matched 84 frames and eight saves. Fresh X11 processes loaded v12 saves including a 384-instance / 19,428-byte boundary case. ARM, WASM and X11 builds succeeded. [Full validation receipts and artifact hashes](docs/validation-0.14.json).

Commit source to a named work branch and read the remote ref back **before** long compilation or conversion. `tools/run_logged.py` records the supplied checkpoint, input hashes, command, running/completed state, exit code and log hash. It does not itself verify or push a remote commit. A running journal after interruption is not a passed test.

Current execution results belong in WORK_STATE, not inherited historical reports. Compilation, cross-backend agreement and actual target execution are different evidence levels. Physical performance, remaining characters, complete original encounters/runs and exact UI parity remain unverified or unfinished.

## Controls and license

Directions navigate; Enter confirms; Space ends a turn or confirms a pending selection; Tab piles; I details; C compendium; V statuses; M map; P potions; R relics; S save; O load; Esc return/pause. Touchpad clicks and dragging share the native dispatcher.

Source: GPL-3.0-or-later (COPYING). Preserve NOTICE.md and licenses/. Original assets and trademarks remain with their owners; source licensing grants no additional asset redistribution rights.
