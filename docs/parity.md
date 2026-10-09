# Original-game fidelity contract

## Reference and acceptance

From version 0.4 onward, the objective is the closest practical reproduction of the original Slay the Spire 2, not an expanding custom card-game demo. The initial reference is **v0.111.0 / 41cef1ea**, with the exact assembly and PCK hashes in `config/reference-release.json`. A newer reference must be an explicit migration with a reviewed difference inventory.

Allowed adaptations include text reflow, larger selectable targets, scrolling, detail overlays, keyboard shortcuts and touchpad navigation for the 320×240 display. Adapted layout should preserve the original visual identity, information, choices and game flow. Lower-cost effects and offline animation sampling are presentation tradeoffs, not permission to alter rules.

Do not change card effects, pool contents, enemy behavior, random probabilities or unlock conditions just to make a run work. Unsupported content must be tracked as missing or a named substitution. A replacement enemy must never be reported as the original encounter. A character portrait attached to another character's logic is not a supported character.

Use distinct evidence states: **discovered**, **semantics inspected**, **implemented**, **reference-trace matched**, and **target tested**. Definitions or extracted images alone do not count as implemented behavior. Testing this project's own invariants is not original-game parity testing.

## Audited inventory, not completion percentages

The read-only inventory command verifies the installation's hashes and extracts the CLI TypeDef table. The pinned assembly contains these namespace-level definitions:

| Category | Type definitions | Interpretation |
|---|---:|---|
| Cards | 596 | Includes special, token and deprecated models. |
| Relics | 300 | Availability still requires pool and unlock inspection. |
| Potions | 65 | Definitions are not proof of normal acquisition. |
| Characters | 8 | Only five belong to the standard character registry. |
| Monsters | 121 | Includes an abstract model. |
| Encounters | 91 | Includes an abstract model. |
| Events | 68 | Per-act availability is a separate question. |

Direct inspection of `ModelDb.get_AllCharacters` confirms the standard registry, in order: **Ironclad, Silent, Regent, Necrobinder, Defect**. Inspection of the five `GenerateAllCards` literal arrays found 90, 91, 91, 91 and 91 entries respectively, before profile unlock filtering. These registry/pool observations are recorded in the reference lock; the TypeDef inventory tool does not independently execute those methods.

Localization has additional obsolete or placeholder entries. Counting `.title` keys therefore overstates the intended content denominator. Later coverage reports must use verified active pools and explicitly separate generated cards, tokens, curses, status cards and inaccessible/deprecated definitions.

The executable currently models **24 card kinds, 10 relics, 3 potions and one character**. Their selected semantics have been inspected and exercised internally, but no full original-model differential suite exists yet. Version 0.7 adds ten audited card models; see `cards-0.7.md`.

## Work plan and current gaps

| Area | Required acceptance | Current state |
|---|---|---|
| Seed and RNG | Same canonical seed, named streams, bounded sampling and call order; original reference vectors. | Reference primitives implemented and replay-checked; Unicode/legacy seeds and full-run integration remain open. |
| Map | Exact topology, room types and stream state for fixed original seeds; original generation/pruning order. | Regional quota pipeline and isolated seed editor implemented; quota CIL replay and target frame checks pass; complete original-run/unlock/encounter integration remains open. |
| Cards and combat | Per-card/upgraded behavior, choice suspension, event ordering, damage rounding and combinations match original traces. | 24 modeled cards, including HP loss, energy, self/hand exhaustion, Weak and No Draw; no complete original-combat differential suite. |
| Relics and potions | Complete active pools, prices/acquisition, hooks, counters, removals, interactions and persistence. | 10 relics and 3 potions; limited shop pools. |
| Characters | Each standard character's real starting state, pools and unique mechanics. | Ironclad subset only; no duplicate-logic character placeholders. |
| Enemies, encounters, events | Original AI/intent/HP, room pools, event choices and results. | Nibbit and three adapted events; several named substitutions. |
| Run flow and progression | Original act transitions, rewards, difficulty, unlocks, profiles and failure/victory behavior. | Custom finite expedition and practice entry. |
| UI | Reference captures for title, character selection, pause, combat, deck/piles, rewards, camp, shop, events, map and settings; preserve original flow. | Original art in adapted screens; 24-card grid/detail support and ten pointer-accessible hand slots; original screen-flow parity remains open. |
| Assets, effects and audio | Original assets and timed presentation where practical; document performance-driven differences. | 11 offline-sampled clips; no audio; no complete effect parity. |
| Multiplayer and other modes | Explicit feasibility and behavioral scope; no silent equivalence to offline play. | Not implemented; tracked separately, not counted as reproduced. |
| Target reliability | Ndless startup, inputs, files, exit and sustained resource budgets, followed by physical-handheld tests. | Firebird CX II tests; physical device and memory peak outstanding. |

Implementation order: establish original RNG reference vectors and map comparison first; then complete an original Ironclad run path, including its cards and shared room/content dependencies; then migrate the other four characters and expand full-run/meta coverage. UI reproduction proceeds against original captures alongside those systems, rather than polishing a knowingly different flow indefinitely.

## Small-screen input boundary

Touchpad and mouse hit regions come from the rendered controls. Hover, scrolling, detail inspection, cursor motion and visual timing cannot consume gameplay RNG or mutate game state. Press/release crossing a modal or changed game revision must cancel rather than activate a new underlying target. Card targeting captures a card instance, not a hand index that can shift after a draw or discard.

Version 0.4 implements relative pad motion, repositioning after finger lift, physical clicks, short taps and drag targeting. Its current sensitivity/tap thresholds remain fixed defaults; physical-device calibration and an in-game configuration page remain open.
