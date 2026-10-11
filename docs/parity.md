# Original-game fidelity contract

## Scope and reference

Reproduce original Slay the Spire 2 single-player rules and visual identity as closely as practical on a 320x240 TI-Nspire. **Audio, multiplayer and Timeline progression are excluded, not postponed.** Animation remains in scope. Implemented content does not require replaying Timeline unlocks.

Reference: the owner's **v0.111.0 / 41cef1ea**, with exact hashes in `config/reference-release.json`. Updating the reference requires an explicit reviewed migration. Do not silently mix later balance changes or Slay the Spire 1 rules into this build.

Text reflow, larger click targets, scrolling, detail overlays, keyboard/touchpad navigation and offline sampled effects are allowed adaptations. Card effects, pool membership, enemy behavior or random probabilities must not be changed merely to make a demo work. Named substitutions and unsupported content remain visible development gaps. A portrait attached to another character's logic is not an implemented character.

## Evidence levels

Distinguish discovered definitions, inspected semantics, implemented behavior, reference-trace matches and target tests. Extracted text/images alone are not implemented content. Self-consistency tests and native/ARM/WASM agreement are not comparisons against the complete original runtime.

Historical inventory reports 596 card, 300 relic, 65 potion, 121 monster, 91 encounter and 68 event namespace-level definitions, including nonstandard/deprecated/abstract entries. They are not valid completion denominators. The standard character registry is Ironclad, Silent, Regent, Necrobinder and Defect. Previously inspected literal character pools contain 90/91/91/91/91 entries before unlock filtering. This iteration independently rechecked the Ironclad and Silent registry calls for the ten additions.

## Current coverage - 0.12

The runtime has **68 card models: 29 Ironclad, 29 Silent, nine colorless and one generated Shiv**, plus 30 relics and 16 potions. Both playable character subsets are incomplete. The other three standard characters remain in scope. Current additions and evidence are documented in `development.md` and `config/relic-audit-0.12.json`.

The playable route remains a custom twelve-floor expedition with the bounded Nibbit encounter and three adapted events. The separately implemented reference RNG/regional map does not yet drive a complete original run. Original encounter/event pools, acquisition probabilities, acts, difficulty and all supported-model interactions still need integration and reference validation.

Current UI includes real two-character selection, original-derived frames/energy/icons, mandatory/optional card choices, paged discard-to-draw selection, permanent-deck confirmation, upgrade comparison, compendium and actor-specific status details, owned-relic grid/counter/hover inspection and staged treasure reveal/claim/proceed. These are adapted layouts, not complete original screen-flow parity. Seventeen actor/room clips and the sampled campfire remain; per-card visual effects are incomplete.

## Input and reliability

Hovering, paging, details, status inspection and visual clocks cannot consume gameplay RNG or mutate gameplay state. Press/release across a modal or changed revision must cancel. Choices capture card instances rather than mutable hand indexes. Touchpad motion supports clutching, physical clicks, taps and drag targeting; physical-device calibration/settings remain open.

The 0.12 final ARM build was exercised in Firebird. Native/ARM/WASM controlled save sequences matched. Physical-device testing, sustained frame rate, peak RAM and a complete original combat differential suite remain unverified. The full validation record distinguishes actual target inputs, controlled fixtures and asset-free tests.
