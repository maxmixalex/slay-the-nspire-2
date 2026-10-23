# Original world and UI — 0.17

Reference remains the owner's pinned v0.111.0 / 41cef1ea installation. User order: all single-player events and enemies; playable original maps; original UI targeting 95%+ fidelity after justified adaptation to 320 × 240. Audio, multiplayer and Timeline remain excluded. On 2026-10-23 the user transferred the repository to `maxmixalex/slay-the-nspire-2`, authorized Actions and requested verified progress on main. Original assembly/PCK data, disassembly, fonts and derived assets stay private.

Current priorities are original UI and calculator usability, runtime speed, and
storage size. Remove integrity-only runtime hashing and validation that is not
required by original gameplay or functional parsing/decoding. This supersedes
the earlier retention policy below; necessary range and format handling remain.

The owner subsequently requested investigation of Sprite Dicing, streaming and a
**30–40 MB calculator installation**. The UI4 split resources total
81,238,925 bytes (77.48 MiB), with 82,236,673 bytes installed before saves when
both languages and the launcher are included; this newer target is not yet
reached. Storage and access evidence,
the shared decoded-image cache and bounded compression experiments are tracked
in [resource-budget-0.17.md](resource-budget-0.17.md). Pixel/frame changes must
not be disguised as the allowed screen-readability adaptation.

## Work boundaries

| Area | Owner | Required result |
| --- | --- | --- |
| Monsters and encounters | Defect agent | Reachable models, move state machines, targeting, intents, powers, summons, phase changes, rewards and event combat handoff |
| Events | Regent agent | Reachable event and ancient pools, option eligibility, costs/rewards, pages, deck selection, special acquisition and combat continuations |
| Original resources | Reference agent | Original asset/scene mapping and bounded random-access pack access; retain the previous asset payloads |
| Shared state and run | Root | Wider enemy storage with frozen historical save widths, new saved world state, three-act routing, room pools/cursors and seven-column map integration |
| Original UI | Root and available agents after content integration | Original scene hierarchy, composition, assets, controls and transitions, with explicit screen adaptations and visual comparisons |

All new modules must expose behavior through the normal dispatcher and save every suspended gameplay decision. Declaring a model, generating its illustration or adding an option label does not complete an implementation. Failed or cancelled choices must not partially change state or consume random draws. Existing 0.16 card, potion and save behavior needs regression protection at the changed boundaries.

## Initial baseline and resource constraint

The completed 0.16 baseline has 497 card models, 64 potions and 33 relics. Its playable expedition is a separate 12 × 3 graph, five adapted events and Nibbit encounters with at most two enemies. The reference map generator already supplies seven-column maps and four act templates, but does not drive the expedition.

The pinned assembly's raw TypeDef counts are 121 monsters, 91 encounters and 68 events. These counts include definitions whose solo availability still needs inspection. The final coverage ledger must distinguish reachable content, explicitly excluded content and unfinished dependencies.

The previous private pack is 12,910,778 bytes, with a 13 MiB resident cap. ARM static allocation is 16,174,284 bytes against a 16 MiB ceiling. Complete enemy/event art and animation cannot be accommodated by simply raising the resident array. The resource path must support bounded range reads and stable short-lived decoded views on calculator, native and WASM backends. File integrity and payload decoding remain checked, and the actual largest working set must be measured.

## Run and visual acceptance

The original run has three act positions: Overgrowth or Underdocks, then Hive, then Glory. The existing generator's grid row count must not be confused with the original total floor count. Encounter/event pools, shuffle order, weak-opening encounters, elite queues, bosses, ancients and saved cursors form part of the playable map integration.

UI comparisons must cover title, character/run setup, map, combat, intents and creature statuses, card inspection and selection, events/ancients, merchant, rest, treasure, rewards, pause and run end. Compare original assets and scene composition at a declared normalized scale. Explain necessary font, spacing, viewport and input changes caused by calculator constraints. A weighted checklist or image similarity alone is not proof of 95% original fidelity; distinguish rendered-scene comparisons from actual unmodified game captures and user acceptance.

## Checkpoints and evidence

Start: remote branch and commit/tree were read back at 4abacfa4. This document is an initial development checkpoint, with no new runtime validation yet. Each coherent implementation stage must be pushed and read back before long compilation, resource conversion or emulator execution. Use immutable archives and `tools/run_logged.py`; retain failures and report them together with the next corrective action. Final artifact replacement follows successful integration and relevant verification.
