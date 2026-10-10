# Original-game fidelity contract

## Explicit product scope

The requested product is single-player gameplay and its original-informed visual interface. **Audio, multiplayer and the Timeline meta-progression system are excluded**, not postponed. They do not belong in the completion denominator. Implemented content is available without Timeline unlock replay. Animation remains in scope. The unused multiplayer row-reduction option was removed from the map API in 0.9; solo map generation is unchanged. See `config/product-scope.json`.

## Reference and acceptance

From version 0.4 onward, the objective is the closest practical reproduction of the original Slay the Spire 2, not an expanding custom card-game demo. The initial reference is **v0.111.0 / 41cef1ea**, with the exact assembly and PCK hashes in `config/reference-release.json`. A newer reference must be an explicit migration with a reviewed difference inventory.

Allowed adaptations include text reflow, larger selectable targets, scrolling, detail overlays, keyboard shortcuts and touchpad navigation for the 320×240 display. Adapted layout should preserve the original visual identity, information, choices and game flow. Lower-cost effects and offline animation sampling are presentation tradeoffs, not permission to alter rules.

Do not change card effects, pool contents, enemy behavior or random probabilities just to make a run work. Unsupported content must be tracked as missing or a named substitution. A replacement enemy must never be reported as the original encounter. A character portrait attached to another character's logic is not a supported character.

Use distinct evidence states: **discovered**, **semantics inspected**, **implemented**, **reference-trace matched**, and **target tested**. Definitions or extracted images alone do not count as implemented behavior. Testing this project's own invariants is not original-game parity testing.

## Audited inventory, not completion percentages

The read-only inventory command verifies the installation's hashes and extracts the CLI TypeDef table. The pinned assembly contains these namespace-level definitions:

| Category | Type definitions | Interpretation |
|---|---:|---|
| Cards | 596 | Includes special, token and deprecated models. |
| Relics | 300 | Availability still requires pool and unlock inspection. |
| Potions | 65 | Definitions are not proof of normal acquisition. |
| Characters | 8 | Five belong to the standard playable-character registry. |
| Monsters | 121 | Includes an abstract model. |
| Encounters | 91 | Includes an abstract model. |
| Events | 68 | Per-act availability is a separate question. |

Direct inspection of `ModelDb.get_AllCharacters` confirms the standard registry, in order: **Ironclad, Silent, Regent, Necrobinder, Defect**. Inspection of the five `GenerateAllCards` literal arrays found 90, 91, 91, 91 and 91 entries respectively, before profile unlock filtering. These registry/pool observations are recorded in the reference lock; the TypeDef inventory tool does not independently execute those methods.

Localization has additional obsolete or placeholder entries. Counting `.title` keys therefore overstates the intended content denominator. Later coverage reports must use verified active pools and explicitly separate generated cards, tokens, curses, status cards and inaccessible/deprecated definitions.

The executable models **58 card kinds (24 Ironclad pool models + 24 Silent pool models + 9 colorless + 1 generated Shiv), 25 relics, 16 potions and two playable character subsets**. Their selected semantics have been inspected and tested internally/on the emulator; no complete original-combat differential suite exists. The content denominator must distinguish generated cards from ordinarily obtainable pool cards. See `silent-0.9.md` and the historical expansion records.

## Work plan and current gaps

| Area | Required acceptance | Current state |
|---|---|---|
| Seed and RNG | Same canonical seed, named streams, bounded sampling and call order; original reference vectors. | Reference primitives implemented and replay-checked; Unicode/legacy seeds and full-run integration remain open. |
| Map | Exact topology, room types and stream state for fixed original seeds; original generation/pruning order. | Regional quota pipeline and isolated seed editor implemented; quota CIL replay and target frame checks pass; complete original-run/encounter integration remains open. |
| Cards and combat | Base/upgraded behavior, choice suspension, event ordering, rounding and combinations match reference traces. | 58 modeled cards; optional exhaust, Retain and card-source No Block added to existing combat support; full original combat differential coverage remains open. |
| Relics and potions | Active pools, prices/acquisition, hooks, counters, removals and persistence. | 25 relics and 16 potions, including character-specific starting relics; remaining content and reference acquisition behavior unfinished. |
| Characters | Each supported character's actual starting state, pool and unique mechanics. | Ironclad and Silent subsets with distinct starts, pools and original presentation; other three characters remain in scope but unimplemented. |
| Enemies, encounters, events | Original AI/intent/HP, room pools, event choices and results. | Nibbit and three adapted events; several named substitutions. |
| Run flow and progression | Single-player acts, room choices, rewards, difficulty, save/continue and win/loss behavior. | Custom finite expedition and practice; Timeline excluded, not a future unlock milestone. |
| UI | Compare title, character selection, pause, combat, piles, card choices, rewards, camp, shop, events, map and settings with original structures. | Two-character selection, colored frames/energy, mandatory/optional choice grids, deck-operation confirmation and upgrade comparison, and paged compendium details; not complete original screen-flow parity. |
| Assets and visual effects | Original visual assets and timing where practical; document sampling/scale tradeoffs. | 17 actor/room clips, sampled campfire layer and original scene stills; per-card effects incomplete. Audio explicitly excluded. |
| Target reliability | Ndless startup, inputs, files, exit and sustained resource budgets, followed by physical-handheld tests. | Firebird CX II tests; physical device and memory peak outstanding. |

Next priorities: expand the two real character subsets and shared combat/content coverage; connect original room/encounter pools to the reference map; extend the remaining characters and visual screens. Original single-player difficulty and run configuration remain in scope; audio, multiplayer and Timeline do not. UI work must follow original captures/assets rather than indefinitely decorating a different game flow.

## Small-screen input boundary

Touchpad and mouse hit regions come from the rendered controls. Hover, scrolling, detail inspection, cursor motion and visual timing cannot consume gameplay RNG or mutate game state. Press/release crossing a modal or changed game revision must cancel rather than activate a new underlying target. Card targeting captures a card instance, not a hand index that can shift after a draw or discard.

Version 0.4 implements relative pad motion, repositioning after finger lift, physical clicks, short taps and drag targeting. Its current sensitivity/tap thresholds remain fixed defaults; physical-device calibration and an in-game configuration page remain open.
