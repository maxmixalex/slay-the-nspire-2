# Original frontend and ending integration

Reference: owner-supplied `v0.111.0 / 41cef1ea`, single-player. The relevant original behaviors are `NMainMenu.RefreshButtons`, `NMainMenu.AbandonRun`, `NPauseMenu`, `NAbandonRunConfirmPopup`, `RunManager.AbandonInternal` and `NFastModeTickbox`. Only resource keys and numeric layout metadata are committed; the source game and converted images remain private inputs.

## Menus and storage

`frontend_input.inc` separates menu presentation from the saved simulation. A validated unfinished save exposes Continue and Abandon; otherwise the first entry is Single Player. Compendium, Settings and Quit follow. Save metadata is decoded into a single static scratch State, so inspection cannot overwrite the visible run or consume its RNG. Continue validates the complete candidate and its unfinished phase before assigning it. Rejected or corrupt loads leave the visible state unchanged.

The pause menu uses the five original single-player actions. Save and Quit writes successfully before returning to the main menu; a failed save keeps the pause menu open. Give Up asks for confirmation and uses the core's forced player death with its actual continuations. The title's Abandon removes the saved expedition after confirmation. These are gameplay dialogs, not development approval requests.

`Callbacks::clear_save` is optional for test/embedded hosts. The production X11 host deletes its save; Ndless removes backup and temporary files before its main save; the paired browser host interprets the explicit `save(nullptr, 0)` call as deletion of all known v3–v15 local-storage keys. Ordinary serialized saves are nonempty. This prevents an old browser fallback key from reviving an abandoned run. Hosts without deletion can write a completed terminal state when finishing, but cannot silently claim to abandon a still-active stored run.

End-of-run cleanup compares saved seed, character, native-world mode and practice mode before removing a stored run. Finishing practice must not erase a separate expedition. These identity fields do not constitute a persistent global run UUID; imported runs with the same identity fields are an existing storage limitation.

Settings currently provides presentation speed and the calculator's control reference. The title additionally exposes practice and the reference-map tool there. The speed switch changes only UI animation advancement, with no game-state, save or RNG change. Settings are retained for this application session. Audio, desktop video options, remappable controls and persistent cross-run preferences are not implemented by this screen.

## Drawing and interaction

`native_controls.inc` draws the original back, confirm and proceed components and their original icon/outline layers. Authored transparent margins and bitmap aspect ratios are retained; callers provide readable hit rectangles for the 320×240 display. Character selection uses its original fuzzy information panel and gold highlight behind a 27×40 portrait; the corresponding highlight is 33×47. Continue's original nine-patch panel shows only facts available in the save. It does not fabricate a last-saved timestamp.

`world_game_over_ui.inc` gives completed and defeated native expeditions their original Continue → summary → main-menu sequence. It uses the pinned build's completion banner and stable initial defeat banner without consuming a game RNG draw. The summary resolves original floor/ascension text and shows current HP, gold, deck, relics and seed. Gold balance is never substituted for cumulative gold gained. Score, run duration, badges, unlocks and exact kill attribution require history that the present save does not contain.

Frontend confirmation, settings and summary selection are App-only fields. Modal input guards prevent underlying game-over actions from firing, and pointer context changes invalidate captured gestures. The existing deck and relic inspectors remain available from the summary. The original full character death animation, cross-run progress and repeat-history presentation remain separate unfinished work.

## Verification boundaries

New regression sources cover menu visibility, atomic rejected loads, real save/clear failures, forced abandonment, different-run save protection, current-run terminal cleanup, modal exclusivity, original ending branches and complete serialized-state equality around presentation. Both new UI suites accept an actual private pack and optional screenshot directory. Synthetic profiles test interaction and bounds; they do not establish visual fidelity. Native/WASM frame agreement establishes this project's backend consistency and is not an original-game screenshot oracle. No 95% similarity score is claimed.
