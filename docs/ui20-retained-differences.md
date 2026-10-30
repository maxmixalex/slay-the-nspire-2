# UI20 reconciliation of retained differences

Reference: StS II v0.111.0 / 41cef1ea. This record distinguishes newly completed
behavior, older completed work, and differences still present in UI20. It does
not turn documentation, a registry entry, an image oracle, or a performance
probe into a gameplay implementation.

## Concrete differences closed by UI20

| Earlier behavior or newly exposed omission | UI20 production implementation |
| --- | --- |
| Ancient art was squeezed below the top bar, including terminal/selection underlays | Full 320x240 runtime placement and all eight scenes regenerated with their original 1680x1260 viewport; the regular source builder preserves that viewport. This does not animate the whole Ancient scene. |
| Osty had no lasting source death presentation | The animation work supplies the source `dead_loop` and its runtime selection. The historical Ancient-animation exception does not cover Osty. |
| Monster action frames could lose substantial detail despite readable idle poses | The animation work regenerates the affected action clips at their declared source-to-LCD projection, preserving their original image identity, clocks and gameplay routing. Source pixel comparisons verify those replacements; the implementation is the new frames and rendering path. |
| Die for You hover text lacked its icon; derived cards reserved space for absent keyword panels; rare-card rewards used the ordinary icon | The actual status/tooltip, preview positioning and reward icon paths are corrected by the companion animation/UI changes. They are visible renderer changes, not new diagnostic labels. |
| UI19 intentionally detached a saved run after any successful console command, so a later real menu abandonment could leave that checkpoint | Only starting a genuinely new debug run detaches the association. A separate saved identity survives current-run gold/card/room/act edits. Terminal retirement checks that identity and removes the owned save, temporary save and backup without deleting a different external run. An ended Save cannot recreate it. |
| Abandonment from the start-of-act map produced an invalid Dead state before any node had been selected | The validator now accepts the ended no-point map state under the existing finished/HP/empty-room constraints. It does not invent a node visit or history fact. |
| Standalone next-act maps received a synthetic paper-colored background | The full map room uses the original black backing; room-overlay maps preserve the underlying room. |
| Defeat labels and score columns did not match their original controls, and summary return lacked the menu transition | Original banner label placement/color and score baselines are applied; localized rows are measured into a compact centered column with explicit right-aligned amounts. Return uses the existing 800 ms out / black switch / 800 ms in sequence and ignores repeated activation during departure. |
| Title version and Continue information retained the wrong anchors | Version uses the actual text width at the bottom-right margin. Continue information follows the localized menu label's right edge. |

Implementation and bilingual actual-resource acceptance for this branch are in
[UI20 UI/lifecycle](ui20-ui.md). The companion animation and integrated release
records own their exact changed-clip set, source-pixel proof and ordinary-device
acceptance. Earlier accepted frames and timing are not removed for the UI20
storage changes.

## Older omission statements already superseded

| Historical retained item | Existing implementation carried forward |
| --- | --- |
| Awaited monster commands beyond AttackCommand hit profiles | UI15 connected source-derived `enemy_command_program.hpp` / `enemy_command_program_ui.inc` programs to real move, selection and death observers. Its 108 actor kinds / 1,312 legal moves are an earlier executed scope, not a new UI20 claim. |
| Vantom's 750 ms independent root/bone transition and tail | UI15 compound clips, independent tail coordinates, triangle/UV scale intervals and VFX/death callbacks are present in `vantom_ui15.inc` and `vantom_vfx_ui15.inc`. Correct endpoints alone were not considered completion. |
| Ancient positive healing, Tablet upgrade notification and Smith impacts | UI15 schedules the real positive heal, retains before/after card facts and executes all nine Smith impact frames. A full-HP visit intentionally has no invented positive heal. |
| Doom, Architect, Random and Fake Merchant visual families | Their UI13/UI14 production observers and scene/effect modules already exist. UI20 compression does not count those historical additions again. |
| Exhaust/discard/power trails, generated-card arrival and orb activation | The UI11 command observers and secondary effects already implement these paths; UI19 additionally corrected actual Draw versus Discard destinations. |
| Creature scale/hue/sleep, detached Bestiary contexts, startup logos, title blur/rainbow, natural glyph advances and kerning | Their UI11–UI15 source modules supersede the earlier missing-feature paragraphs. Those implementations do not prove arbitrary cross-track Spine composition or continuous desktop GPU identity. |
| Original seeded hash transformations and raw-seed semantics | UI17 implements the source transformations and legacy raw path. The 64-byte input capacity below still applies. |

Sources: [UI14 reconciliation](ui14-retained-differences-0.17.md),
[UI15 reconciliation](ui15-retained-differences-0.17.md),
[UI17 acceptance](ui17-feedback-acceptance-0.17.md), and
[UI19 feedback](ui19-ui-feedback.md). Their historical test counts and timings
retain their original checkpoint identity.

## Specific differences still present

| Difference | Current behavior and UI20 status |
| --- | --- |
| Complete Ancient actor/background animation | The full Ancient room is still a static composite under the recorded storage exception. UI20 corrects its viewport and source pixels; it does not add the complete room animation. Dialogue, options, gameplay, healing and animated card materials remain implemented. No other absent actor animation inherits this exception. |
| Audio, multiplayer and Timeline | These remain explicitly excluded systems in `config/product-scope.json`; they receive no implementation or fidelity credit. |
| LCD and material representation | Output remains 320x240 RGB565/LDR with integer rasterization, bounded CPU particles and sampled material phases. Affliction materials have eight phases, Infection 30 source frames and Ancient card fire 10. These are implemented representations, not continuous Godot GPU equivalence. UI20 does not silently reduce them further. |
| Cosmetic randomness | Presentation noise uses its own deterministic/local state and does not recreate an unrecorded process-global cosmetic RNG history. Canonical gameplay RNG is separate and preserved. |
| Hosted services | Local daily/custom modes exist; hosted leaderboards, cloud behavior and desktop service integrations are not emulated. Local unranked statistics are not online-service parity. |
| Calendar/timezone | `ui_calendar.hpp` formats the selected calendar fields, while the underlying RTC timestamp is interpreted without a desktop timezone/DST database. Locale-specific date labels are implemented; timezone reconstruction is not. |
| History and map notes | `ui_progress.hpp` retains 32 runs and 96 floors per run. Map drawing has 2,048 points (`map_drawing.hpp`); transition/journal capacities are also finite. Lost or evicted facts remain explicitly partial. UI20 does not claim unbounded desktop history. |
| Seed/input field | World and progress records store 64 UTF-8 bytes plus termination (`seed_text[65]`), with corresponding codec checks. There is no new calculator IME. Expanding a field is not the same as implementing the missing input platform or extending the save contract safely. |
| Metadata/cache/OS bounds | Resource directories, decoded cache groups and process allocation remain bounded. UI20's lossless codecs preserve those existing group bounds. An allocation gate does not guarantee operation under every fragmented OS heap, and emulator timings are not physical-handheld measurements. |

This reconciliation found no additional source-confirmed, bounded missing UI
callback in the historical omission ledger beyond the concrete corrections
above. It does **not** establish that all retained differences are gone. In
particular, the full static Ancient scene, sampled materials, cosmetic process
history, timezone data and finite capacities must remain visible in the final
release's limits; a broad claim of complete original-game parity would be false.
The actual installed-storage goal and cold-frame behavior must be assessed from
the final package and measured device windows, not from internal codec savings.
