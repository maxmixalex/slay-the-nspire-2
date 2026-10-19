# Slay the Nspire 2

Native TI-Nspire Ndless adaptation of Slay the Spire 2. Reference: the owner's hash-pinned **v0.111.0 / 41cef1ea** installation. Active development is **0.16: all five single-player character pools**, on `work/0.16-five-characters`. The first source integration checkpoint is [`2eabce11`](https://github.com/zyf722/slay-the-nspire-2/commit/2eabce11a5ad5e5f2460e67eb5f9713c8ea424eb); its runtime validation is pending. The last distributed verified runtime remains 0.15 at [`4df76b04`](https://github.com/zyf722/slay-the-nspire-2/commit/4df76b04e2a07e6be9a898793f03fd0ab8df4aae) until this stage completes.

**Audio, multiplayer and Timeline are excluded.** Original UI fidelity, animation and single-player content breadth remain in scope. Original game data, fonts and calculator images are not included in public source.

## Current content

497 card models: **85 single-player Ironclad**, **86 each for Silent, Defect, Regent and Necrobinder**, **53 single-player colorless** and 15 generated/status models. There are **64 active potion models**, 33 relics and five adapted events. The five character pools include their base and upgraded rules. New 0.16 execution is still pending verification. Ancient-rarity cards are available in the compendium/practice pool and excluded from ordinary rewards and generation. The twelve-floor playable route and Nibbit encounters remain adapted; the reference regional map is still separate from a complete original run.

Practice exposes every character/colorless card and potion through the compendium. Open it with **C** or the practice HUD button, inspect an entry and use its acquisition button. Card variants, potion targeting, multi-card selections, generated offers, nested automatic plays and persistent statuses share the same engine and UI on native and browser builds. A full hand sends newly created cards to discard; a full potion inventory must be freed before acquiring another bottle.

Ordinary potion pools follow all five current characters and the pinned rarity weights. Other-character and special-acquisition bottles remain available for explicit practice use; their original special acquisition routes are separate work. The 0.13 event, relic, treasure and fanned-hand UI work is retained. These layouts are adaptations, not complete original-PC fidelity.

Read [current work state](docs/WORK_STATE.md) before resuming development and the [Ironclad](docs/ironclad-completion-0.15.md), [Silent](docs/silent-completion-0.15.md), [Defect](docs/defect-completion-0.16.md), [Regent](docs/regent-completion-0.16.md), [Necrobinder](docs/necro-completion-0.16.md), [colorless](docs/colorless-completion-0.14.md) and [potion](docs/potions-completion-0.14.md) audits for implementation scope and original-rule evidence. Registry counts alone are not a passing test or a claim that the entire original game is complete.

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
python3 tests/all_character_pack_regression.py
```

ARM requires LLVM clang++ and ld.lld; the browser build also requires wasm-ld. The X11 host is optional. Install converter dependencies from requirements.txt. Convert an existing private 0.15 NSP2/12 pack without resampling its retained animation frames:

```sh
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label content -- python3 tools/build_all_characters.py /path/to/game /path/to/old/nSpire2.nsp.tns --output dist/nSpire2.nsp.tns --font /path/to/local-font
python3 tools/run_logged.py --checkpoint "$NSPIRE_CHECKPOINT" --label web -- python3 tools/build_web.py
```

For a full conversion from the pinned installation, `build_content.py` now chains through the same final character conversion. The conversion checks the original release metadata, assembly and PCK hashes. New launchers require **NSP2/13** (497 cards, 339 auxiliary images and 1,556 strings). Put `nSpire2.tns` and `nSpire2.nsp.tns` in the same calculator directory; `nSpire2-desktop.html` is the self-contained offline browser build. Replace the launcher and pack together.

New saves use **v14** and retain readers for project v3–v13. The appended v14 payload records all three new character modules, wide card/relic identifiers, independent star prices, persistent deck growth, expanded acquisition order and cross-character Nightmare modifiers. Earlier writers did not record the new histories; migration preserves encoded facts and seeds only observable pending play/death baselines. The genuine frozen v13 continuation fixture and earlier historical fixtures exercise those boundaries. Older Entropy/Boulder and Panache/Calamity acquisition orders retain their documented historical migration policies. The browser writes to a separate v14 storage key and can import earlier keys. Old launchers cannot read the new save.

The native suite includes all character, colorless, potion, historical migration and UI regressions. `expansion_ui_regression`, `character_ui_regression` and `five_character_ui_regression` optionally accept a real NSP2/13 file and an existing private screenshot directory. They cover all 860 character/Giant Rock variants, retained colorless/potion flows, generated/status cards and five-character mechanics. The optional native/WASM oracle adds five controlled combat sequences to the four retained room sequences.

## Persistence and validation

The following results are historical. At the verified 0.15 runtime source, **25/25 C++ suites and 17/17 Python tests passed**, with ASan/UBSan on native regressions. The native suites report 748,629 checks/assertions, including Ironclad's 4,390 checks across 170 base/upgraded variants plus Giant Rock and interactions, and Silent's 17,753 checks across 174 variants. Checks include repeated assertions and are not counts of unique scenarios.

Real-content UI passed 22,225 checks across all **344 character/token variants**, plus 7,374 checks covering all **106 colorless variants and 64 potions**. The real NSP2/12 parser passed 1,667 checks. Native/WASM room comparison matched 84 frames and eight saves. Fresh X11 processes loaded v13 saves including a 384-instance case and re-saved identical bytes. The deliberately full bounded v13 fixture measured 61,956 bytes; its conservative 62,008-byte ceiling fits the 65,536-byte save buffer. ARM, WASM and X11 builds succeeded. [Current validation receipts and artifact hashes](docs/validation-0.15.json) include the exact build source and evidence limits; [0.14 results](docs/validation-0.14.json) are historical.

Commit source to a named work branch and read the remote ref back **before** long compilation or conversion. `tools/run_logged.py` records the supplied checkpoint, input hashes, command, running/completed state, exit code and log hash. It does not itself verify or push a remote commit. A running journal after interruption is not a passed test.

Current execution results belong in WORK_STATE, not inherited historical reports. Compilation, cross-backend agreement and actual target execution are different evidence levels. Physical performance, complete original encounters/runs and exact UI parity remain unverified or unfinished.

## Controls and license

Directions navigate; Enter confirms; Space ends a turn or confirms a pending selection; Tab piles; I details; C compendium; V statuses; M map; P potions; R relics; S save; O load; Esc return/pause. Touchpad clicks and dragging share the native dispatcher.

Source: GPL-3.0-or-later (COPYING). Preserve NOTICE.md and licenses/. Original assets and trademarks remain with their owners; source licensing grants no additional asset redistribution rights.
