# Original-game fidelity contract

Reference remains the owner's hash-pinned v0.111.0 / 41cef1ea. Do not silently mix other releases or Slay the Spire 1 rules into this build. Audio, multiplayer and Timeline progression are excluded, not deferred; animation remains in scope.

Reflow, larger targets, scrolling, detail overlays, offline visual sampling and keyboard/touchpad adaptation are allowed for 320x240. Card effects, pools, enemy behavior and random probabilities must not be changed merely to make a demo work. Unsupported content and substitutions remain explicit gaps. Portraits and extracted text alone do not constitute supported gameplay.

## Current coverage

68 executable card models (29 Ironclad, 29 Silent, nine colorless, one generated Shiv), 30 relics and 16 potions. Both playable characters are incomplete subsets; the other three standard characters remain in scope. The original registry audits, rather than localization key totals, define the eventual coverage denominator.

The playable expedition still uses a custom twelve-floor route, bounded Nibbit encounters and three adapted events. The reference RNG and regional-map subsystem remains separate from the playable route. Original encounter/event pools, acquisition probabilities, acts, difficulty and full-run integration are unfinished.

The combat UI revision follows inspected original scene anchoring: bottom-centered hand, energy left, end turn right, opposite-corner piles and card title/portrait/type/description hierarchy. The integer fan, focus scale, lower hit rail and text truncation/detail policy are explicit small-screen adaptations, not exact original motion or pixel parity. Other room and menu screens remain fidelity work. Existing seventeen clips and campfire payloads are retained.

## Evidence boundary

Distinguish discovery, inspected semantics, implementation, original-reference trace agreement and target testing. Native/WASM pixel/save agreement and invariant tests do not establish complete original-game parity. The current fifteen-target local suite, ARM compilation and native/WASM comparisons are recorded in combat-ui-0.12.md. No new Firebird, physical-handheld, browser DOM/persistent-storage, sustained frame-rate or peak-RAM certification is claimed for this revision. Historical target reports remain separate.

Hover, paging, details and visual clocks must not consume gameplay RNG or mutate gameplay state. Choices retain actual instance IDs. A changed revision, new modal or scroll cancels captured pointer actions. Remote CI stays disabled until explicitly re-enabled by the owner.
