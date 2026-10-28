# 0.12 combat composition revision

The 0.12 recovery was published first at `6a9f27764a7a0f054126eaba9a9895a3222e0ffd`. This subsequent revision changes presentation, not the card/relic rules or save format. Remote CI remains disabled.

## Original reference and adaptations

Read the owner's hash-verified `combat_ui`, `player_hand`, `combat_piles_container`, `end_turn_button` and `card` scenes. Their hashes and observed anchoring are recorded in `config/combat-ui-audit-0.12.json`. The hand is bottom-centered; energy stays left, end turn right, and draw/discard piles occupy opposite corners. Card titles belong above the portrait; type and description follow it.

The new integer-only fan and 80x112 focused card are authored 320x240 adaptations, not a transcription of NPlayerHand's animation curves. Focused cards are clamped away from the side controls. A stable lower selection rail preserves access to all ten instances and prevents hover oscillation. Card faces mark long descriptions with an ellipsis; I retains full detail text. Hero HP is also visible below the actor. The duplicate starting relic in the main top bar is removed; its ordinary owned-relic icon remains.

Card rasters have a fixed ten-slot cache with RGB565 plus coverage, invalidated when art context or displayed properties change. Warm frames reuse them; rotation uses bounded integer inverse sampling. Input routing and non-combat room rendering are separated into include units without changing their behavior.

## Executed validation

All fifteen CTest targets passed under ASan/UBSan. The new combat UI target passed 800 asset-free checks and 815 checks with the actual pack. It covers 1-10 hand sizes, centering, stable hover, corner piles, right-side end turn, attack/skill dragging, unaffordable cards, modal capture cancellation and cache invalidation. Five asset-aware targets passed (535/106/353/223/815 checks).

The actual final WASM was instantiated with explicit storage callbacks. For both Ironclad and Silent, six snapshots (combat, card detail, post-play, statuses, piles and load) each matched all 76,800 RGB565 pixels from the native renderer. The 2,276-byte and 2,288-byte post-play saves also matched exactly. This is project-backend agreement, not original-PC or Firebird parity, browser DOM automation or real persistent localStorage certification.

ARM compiled successfully: 118,154-byte launcher, 8,337,248-byte reported static loader allocation and 1,503 relocations. WASM is 406,056 bytes; its fixed initial/maximum memory is nine MiB, including the pack buffer, raster caches and stack. The prior eight-MiB WASM link bound was too small after adding the card cache; it was increased explicitly, not left to unbounded growth. These figures are not measured peak runtime RAM or sustained handheld frame rate.

The NSP2/9 pack remains byte-identical to the completed recovery pack: 5,687,197 bytes, SHA-256 `844ca67d477bb9117cc1b9c87c4600e64ba6b6aa20f66885184806c119d894ce`. No animation image was removed, resized or retimed. Save v9 and historical v3-v8 reading remain unchanged.

Firebird, physical-device performance and a full original-game differential run were not rerun for this UI revision. The custom expedition, remaining characters/encounters, other screens and exact PC motion remain fidelity gaps. Audio, multiplayer and Timeline remain outside scope.
