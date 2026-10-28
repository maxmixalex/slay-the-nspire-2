# UI7 original UI inventory and behavioral audit

Reference: **v0.111.0 /41cef1ea**. Port baseline: `9cecd0f743ce6cd100df91b103babe67babf480f`. This is an inventory and selected source-contract review. UI7 implementation and validation are ongoing.

## What was actually indexed

The private pinned assembly decompilation contains **3,528 C# files**, including **724 files in Nodes namespaces**. A broad UI path filter adds **13 AutoSlay screen handlers** and **3 Entities.UI support files**, producing **740**. There are **682 N-prefixed files** across the entire source tree, including seven non-UI/debug helpers; that prefix alone is not a visible-interface inventory. The pinned PCK contains **1,022 scene files**. These are indexing counts, not implementation or restoration percentages.

The initial full decompile contains **14 empty files**, including **five in the broad UI set**. `NCombatRoom`, `GalvanicPower`, `NCard`, `PotionModel`, `NDropdownContainer`, `NMapPointHistoryHoverTip` and `ActModel` now have separate recovered decompilations used by their corresponding audits. The correct dropdown type is in `Nodes.CommonUi`. Other empty or incomplete classes remain unknown until independently recovered. Generated Godot method/property wrappers are not functional source coverage.

## Visibility is a code contract

A scene node is not automatically a player-facing requirement. The pinned `NCharacterSelectScreen.ShouldShowActDropdown` returns false; its act selector stays hidden. `NCompendiumSubmenu.OnSubmenuOpened` hides the leaderboard button, and its `OpenLeaderboards` handler is empty. Bestiary requires a discovered act plus at least one enemy victory; run history requires a saved historical run. Custom and Daily use their epoch-unlock gates. `NSettingsScreen` shows Modding only when installed-mod configuration exists and hides Fullscreen on fullscreen-only platforms. The in-run language dropdown is disabled.

Only **audio, multiplayer and Timeline** are explicitly outside the active project scope. Daily singleplayer, profiles, statistics, history and Bestiary do not become excluded merely because they are missing. Original hidden or capability-gated controls are recorded separately.

## Behavioral contracts

The JSON companion contains the exact source type paths, namespace/scene counts, empty-source inventory and **59 behavioral contracts**, including the font-family contract added after actual font recovery. These contracts have unequal size and complexity and must not be divided into a claimed fidelity percentage. `implemented_unvalidated` means source code and meaningful regression source exist, not a completed feature acceptance. `partial` includes real port behavior plus named remaining original differences; `in_progress` is an active source change that has not reached an acceptance gate.

| Domain | Contracts | Current boundaries |
| --- | ---: | --- |
| entry | 6 | 1 original_hidden, 3 partial, 2 unimplemented |
| collections | 8 | 1 original_hidden, 6 partial, 1 in_progress |
| run | 12 | 12 partial |
| combat | 18 | 2 implemented_unvalidated, 16 partial |
| global | 12 | 2 implemented_unvalidated, 4 partial, 4 unimplemented, 2 unverified |
| excluded | 3 | 3 excluded |

## Immediate implementation assignments

- Combat: exact DamageResult feedback, authored block break, actual power Flash triggers, source screen punch and hit/block particles. These are implemented through real commands and rendering, then checked with original assets; probes alone do not close them.
- Cards: real grid scrolling/thumb, target reticles, potion error/selection behavior, and original library/collection filters.
- Rooms: merchant movement/dialogue/hand tracking, event typewriter/option timing and special layouts, map legend/history, reward alternatives and direct removal.
- Shared interface: title/compendium routes, original-supported settings and language switching, persistent statistics/history contracts.
- Bestiary: the real enemy list, preview actions and character statistics use independent observed progress and the original gate. Its precomputed bilingual order now matches the original invariant culture and removes runtime insertion sorting. The corrected four-action Nibbit fixture and complete order oracle await checkpoint-8 execution; special move VFX, phase-dependent contexts and actual bilingual screen acceptance remain partial. See [bestiary-ui7-0.17.md](bestiary-ui7-0.17.md).
- Fonts and phobia: intermediate actual bilingual original-font conversion passed. Phobia's original43-clip/four-background conversion and both actual-pack13-model App flows passed, with ordinary registrations and811 decoded new images verified. Final integrated font/resources/platform acceptance and Entomancer's independent swarm remain open. See [phobia-ui7-0.17.md](phobia-ui7-0.17.md).

Checkpoint 4's Release build succeeded, but its complete native gate passed
**68 of 84 suites**. Corrected fixtures do not retroactively change that result.
Checkpoint 5 separately passed five font-tool tests and both actual font
conversions. The library suite and 860-character-variant suite passed their
stated checkpoint-4 scope; failed later modal/room suites and still-unexecuted
viewport corrections remain separate. The actual character conversion also
exposed an incorrect 1440×1080 assumption; the source's 1680×1260 contract is
now the basis for the pending resample. Sampling original artwork is not
whole-screen acceptance. Checkpoint6 then built all216 native steps and passed
75/87 CTest suites; combat-damage, font-style, phobia and Ndless-profile suites
passed within that partial result. Its corrected1680 character conversion
and both actual phobia flows passed separately. Checkpoint8's split17..26
matrix passed, but the suite stopped at a stale negative assertion that
still rejected the now-supported24 format. The unknown-version fix awaits
the next checkpoint. No complete latest native or platform pass is implied.

## What this audit does not establish

It does not establish 95% original-UI restoration, every original method reviewed, every scene rendered, exact Godot shader/particle replication, or full original-PC equivalence. Native/WASM frame and save agreement tests determinism of the port. ARM compilation does not measure physical-device performance. The explicit missing singleplayer flows and per-feature remaining limits stay in the denominator discussion.

Original source, artwork, screenshots and save data remain private. Public files contain independent implementation, source paths/identifiers and numeric facts.
