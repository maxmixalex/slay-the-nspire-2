# UI7 original UI inventory and behavioral audit

Reference: **v0.111.0 /41cef1ea**. Port baseline: `9cecd0f743ce6cd100df91b103babe67babf480f`. This is an inventory and selected source-contract review. UI7 implementation and validation are ongoing.

## What was actually indexed

The private pinned assembly decompilation contains **3,528 C# files**, including **724 files in Nodes namespaces**. A broad UI path filter adds **13 AutoSlay screen handlers** and **3 Entities.UI support files**, producing **740**. There are **682 N-prefixed files** across the entire source tree, including seven non-UI/debug helpers; that prefix alone is not a visible-interface inventory. The pinned PCK contains **1,022 scene files**. These are indexing counts, not implementation or restoration percentages.

The initial full decompile contains **14 empty files**, including **five in the broad UI set**. `NCombatRoom` and `GalvanicPower` have separate repaired decompilations. `NCard`, `PotionModel`, `NDropdownContainer` and `NMapPointHistoryHoverTip` are assigned targeted recovery. An empty or incomplete class is unknown until that recovery is inspected. Generated Godot method/property wrappers are not functional source coverage.

## Visibility is a code contract

A scene node is not automatically a player-facing requirement. The pinned `NCharacterSelectScreen.ShouldShowActDropdown` returns false; its act selector stays hidden. `NCompendiumSubmenu.OnSubmenuOpened` hides the leaderboard button, and its `OpenLeaderboards` handler is empty. Bestiary requires a discovered act plus at least one enemy victory; run history requires a saved historical run. Custom and Daily use their epoch-unlock gates. `NSettingsScreen` shows Modding only when installed-mod configuration exists and hides Fullscreen on fullscreen-only platforms. The in-run language dropdown is disabled.

Only **audio, multiplayer and Timeline** are explicitly outside the active project scope. Daily singleplayer, profiles, statistics, history and Bestiary do not become excluded merely because they are missing. Original hidden or capability-gated controls are recorded separately.

## Behavioral contracts

The JSON companion contains the exact source type paths, namespace/scene counts, empty-source inventory and **58 behavioral contracts**. These contracts have unequal size and complexity and must not be divided into a claimed fidelity percentage. `implemented_unvalidated` means source code and meaningful regression source exist, not a completed feature acceptance. `partial` includes real port behavior plus named remaining original differences.

| Domain | Contracts | Current boundaries |
| --- | ---: | --- |
| entry | 6 | 1 original_hidden, 3 partial, 2 unimplemented |
| collections | 8 | 1 original_hidden, 3 partial, 4 unimplemented |
| run | 12 | 11 partial, 1 unverified |
| combat | 18 | 2 implemented_unvalidated, 15 partial, 1 unimplemented |
| global | 11 | 2 implemented_unvalidated, 3 partial, 4 unimplemented, 2 unverified |
| excluded | 3 | 3 excluded |

## Immediate implementation assignments

- Combat: exact DamageResult feedback, authored block break, actual power Flash triggers, source screen punch and hit/block particles. These are implemented through real commands and rendering, then checked with original assets; probes alone do not close them.
- Cards: real grid scrolling/thumb, target reticles, potion error/selection behavior, and original library/collection filters.
- Rooms: merchant movement/dialogue/hand tracking, event typewriter/option timing and special layouts, map legend/history, reward alternatives and direct removal.
- Shared interface: title/compendium routes, original-supported settings and language switching, persistent statistics/history contracts.
- Bestiary: a coherent enemy-list, pose and move-description flow can reuse the pinned enemy catalogue. Its original progress gate needs one shared metadata policy rather than an invented local counter.

## What this audit does not establish

It does not establish 95% original-UI restoration, every original method reviewed, every scene rendered, exact Godot shader/particle replication, or full original-PC equivalence. Native/WASM frame and save agreement tests determinism of the port. ARM compilation does not measure physical-device performance. The explicit missing singleplayer flows and per-feature remaining limits stay in the denominator discussion.

Original source, artwork, screenshots and save data remain private. Public files contain independent implementation, source paths/identifiers and numeric facts.
