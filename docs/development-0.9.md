# Development status — 0.9

## Product scope and implemented work

Audio, multiplayer and Timeline meta progression are explicitly excluded, not deferred. Animation remains in scope. The unused multiplayer map-height option was removed; existing single-player regional parameters are unchanged. Implemented content does not require a Timeline unlock sequence.

This release adds a real Silent subset, not an Ironclad reskin: initial HP/deck/relic, character-specific card/reward/shop pools, Poison, discard, Shivs and powers, plus original character/energy/card-frame/animation assets. There are 51 card models: 24 Ironclad, 24 Silent, two colorless and one generated Shiv. Relics increase to 17; potions remain 16. Both characters are incomplete subsets. The existing modeled enemy and three adapted events remain; the reference map is still separate from the playable custom expedition.

Mandatory discard and exhaust now use a modal grid with explicit confirmation. The compendium has character/color filters and cost/type sorting. Canonical catalog descriptions are isolated from live combat buffs. NSP2/6 appends the Silent records/assets/clips; save v6 reads prior v3/v4/v5 formats and preserves pending card choices. See `silent-0.9.md` for exact card coverage and evidence boundaries.

## Source and original-reference checks

The private game assembly and PCK were verified against `config/reference-release.json`. Card constructors, canonical variables, upgrades, keywords, relevant OnPlay methods, the 91-entry Silent registry, character starting values, Ring of the Snake and relevant Poison/status/card-pile methods were inspected. The public token inventory is `config/card-audit-0.9.json`.

This is **metadata/CIL inspection**, not a new original-CIL execution suite and not an unmodified original-game combat oracle. Invariant tests based on those observations do not prove all combinations match the original game. Historical reference-map replay evidence is separately labeled in `map-seed.md`.

## Executed regression checks

All eight CTest targets passed with AddressSanitizer and UndefinedBehaviorSanitizer enabled. They run without original game assets using synthetic content and project-generated historical saves.

| Target | Executed result |
|---|---|
| core_regression | 103,307 checks; 9,885 generated transitions across 600 controlled Ironclad scenarios. |
| input_regression | 59 checks. |
| content_regression | 152 asset-free checks; 413 with the delivered private pack. |
| expansion_regression | 867 checks, including the historical v4 shop fixture. |
| ui_flow_regression | 505 checks. |
| compression_regression | 320,058 boundary checks. |
| silent_regression | 64,833 checks; 8,119 generated transitions across 400 controlled Silent scenarios, including v5 migration. |
| silent_ui_regression | 694 asset-free checks; 696 with the delivered pack. |

Controlled stress scenarios increase HP to exercise more transitions; these are not ordinary complete runs or original-game differential traces. Per-card tests cover both upgrade states, first-turn relic behavior, independent character pools, generation versus drawing, hand overflow, lethal Poison, attack modifiers, invalid/duplicate discard selections, pending-choice serialization and old-save migration. UI tests verify browsing is non-mutating and mandatory-choice overlays do not expose underlying combat controls.

The full-conversion private pack also passed the 413-check content suite. Native/X11, ARM926 Ndless and WebAssembly builds completed successfully. Interactive X11 input was not separately retested; its headless mode was used for native captures and save decoding.

## Final-build Firebird execution

The uploaded Firebird source and matching gif-h submodule were compiled with a private stepping/key/touchpad/USB/framebuffer frontend. The experiment used a private working copy of the owner's CX II CAS flash and supplied injected boot image. Uploaded originals were not modified. The supplied Ndless r2022 installer was opened through the calculator OS and reported successful installation.

The final 0.9 launcher was transferred while the calculator OS was active, then opened from its document browser. The test used normal Silent practice, whose fixed opening allows the following sequence without editing emulator RAM or injecting a combat fixture:

1. Select Silent and start practice with 70 HP, Ring of the Snake and seven opening cards.
2. Apply Deadly Poison and play Acrobatics, entering the new one-card discard overlay.
3. Save while the mandatory discard is pending, select/confirm, then load back to the same pending choice with empty presentation-only selection.
4. Confirm again and play Blade Dance; three Shivs are created and Blade Dance exhausts.
5. End the turn; the enemy loses five HP to Poison before acting, its Poison decreases, and the player takes the modeled enemy attack.
6. Exit normally to the OS and only then retrieve the actual save through emulated USB.

The retrieved **1,876-byte save v6** was accepted by the native decoder: Silent (character 1), HP 70, energy 1, phase DiscardChoice, eight hand cards, required discard 1, enemy Poison 5, Ring of the Snake bit 65,536 and one queued remaining effect. This is the earlier saved choice state, not the post-turn state. A separate browser save was decoded with the same fields.

Final-build title and Silent character-selection captures matched the native core in **all 76,800 RGB565 pixels** after correcting each host's 8-bit channel expansion. This compares two backends of this project, not screenshots against the original game. The final build also exercised the filtered Silent compendium. The short Silent animation recording consists of 60 actual framebuffer captures sampled every four 100-Hz emulation ticks; it is not a prerendered game-scene demonstration.

Physical calculators, other OS/hardware revisions, sustained handheld performance and measured peak memory are still unverified. The reference map remains a diagnostic and was not certified as an original playable run by these tests.

## Browser evidence

Direct `file:` and loopback HTTP navigation both returned `ERR_BLOCKED_BY_ADMINISTRATOR`. The actual generated HTML/WASM was instead loaded into an inline Chromium document. The test explicitly provided an in-memory storage adapter; it did not bypass navigation policy.

Actual DOM mouse/keyboard handlers exercised Silent practice, Poison, the discard overlay, save/confirm/load, v5 fallback, separate v6 writes and re-instantiation with identical saved bytes. No page errors were reported. Old v5 data was unchanged. These checks do **not** establish real cross-session localStorage persistence or direct offline-file startup in an ordinary user browser.

## Content conversion and resource figures

The delivered incremental pack is **6,118,099 bytes**, containing 1,286 image records and 17 clip records. All 999 decoded baseline image records match 0.8 byte for byte; the old 11 clips/706 frames and 160 campfire frames are retained. Six new Silent clips contribute 244 frames. Combat clips are sampled at approximately 24 fps; only the new Silent camp loop uses approximately 16 fps to fit the existing six-MiB resident-pack bound. Sampling rates are not achieved device frame rates.

Repeating the incremental conversion produced exactly the same file. Full conversion from the pinned installation also completed, producing a **6,107,967-byte pack** whose decoded images and clip records match the incremental result. The file-size difference comes from retained historical glyph coverage in the incremental path; complete full/incremental pack byte identity is not claimed.

The final ARM launcher is **102,945 bytes**; the Zehn packer reports **8,036,368 bytes of static allocation**. Those are file/loader allocation figures, not measured peak RAM including runtime stack/heap. Existing resident-pack and bounded workspace limits remain.

## Delivery state

The current connector exposed read/search operations only. At the remote check, GitHub `main` remained at the 0.7 commit `5f7715014a269c871d64816f28a5af91ff7d7138`. Neither the retained 0.8 nor this 0.9 work was published by that check. Deliver source plus a cumulative patch against that remote baseline, rather than presenting a local commit as a pushed one. Source archives exclude original data, calculator images, private test drivers, generated content and font files.
