# UI25 player-feedback reconciliation

Reference: original **v0.111.0 / 41cef1ea**. Current runtime:
**ed5dd2564625779313aa2c71046079dee7506e76**. Corrected resource producer:
**e98a6cb4ed29cd4e3bcaa597cff9ad089c6854f7**. This is a bounded source and
evidence reconciliation with the separately identified touchpad check below,
not a new gameplay feature or a repeat of the complete release gate.
The recovered final [release record](ui25-release-validation.md) and
[numeric receipt](ui25-release-validation.json) own the executed UI25 claims.
Earlier ledgers retain their original source/data identities.

## Result and release boundary

The reviewed ordinary UI feedback has production implementations in ed5.
The old UI20/early-UI23 omissions do not describe the current program:
Timeline, independent Ancient effect families, continuous Affliction materials,
long-ASCII seed import and paged history were implemented later. No identified
ordinary UI callback is repaired by simply copying those old pending lists.

No runtime, converter, resource, cache, allocation or action clock is changed by
this review. The three accepted ZIPs remain fixed. A future concrete runtime
fix must have a new source checkpoint, native checks at the affected boundary,
ordinary ARM build under **22,291,016 bytes**, and the relevant ordinary 64 MiB
Firebird interaction before replacing a launcher or package. Old guest captures
cannot be relabelled as that new runtime's acceptance.

## Reported UI and action behavior

| Feedback | Current production behavior and source | Evidence and player-visible check |
| --- | --- | --- |
| Top bar spacing and two-digit floor | `include/hud_ui7_presentation.hpp::hud_room_geometry` places room/floor/boss/timer at uniformly spaced centres after the actual potion tray. `src/hud_ui6.inc::hud_room_nodes` centres the floor string within its bounded field; the timer remains an icon with elapsed time in its tooltip. The readable bar is 22 pixels high. | `tests/lcd_input_ui12_regression.cpp` checks the location gaps for different inventories; `tests/deck_selection_ui9_regression.cpp::two_digit_floor` reaches a real floor at least 10 and checks separation from the boss hit region. UI14/UI19 record actual LCD review. Player check: enter floor 10+, then inspect the clock and a crowded potion tray. This review does not claim a new UI25 capture of each inventory. |
| Upgrade picker and View Upgrades | `include/deck_selection_layout.hpp` reserves `UpgradeGridBottom=190` and `ToggleY=195`, with a 14-pixel control. `src/deck_selection_ui.inc::card_selection_upgrades_control`, `src/world_selection_ui.inc` and the shared card-grid clipping keep enlarged cards and their hits above that row. Pointer mode exposes the original preview checkbox; directional input hides it and resets the preview. | `deck_selection_ui9_regression` toggles by pointer, changes to directional input, selects a real Smith card and confirms its upgrade. `world_selection_ui9_regression` covers world selection. UI19 records the ordinary LCD checkbox separation. Its presence is original behavior, not an extra unrelated choice. Tablet's Event/Messy previews and Smith's hammer impacts intentionally differ. |
| Merchant potion/relic descriptions | `src/event_hover_ui.inc::merchant_stock_hover_target` uses the actual unsold detached stock model; `merchant_hover_target` routes pointer and keyboard focus. `src/keyword_tooltip_ui.inc::prepare_merchant_hover` prepares the same text/derived-card set without changing inventory or save state. | `tests/world_room_ui_regression.cpp::merchant_model_hover_sets` checks real model ownership, rendered tips and stock interaction. Player check: open the rug, focus each potion/relic, leave it without buying, then inspect a sold slot. Source presence is not a claim that every optional locale was newly visually reviewed on UI25. |
| Loot potion/relic descriptions | `src/event_hover_ui.inc::reward_hover_target` routes normal, world and event rewards, including claimed-index gaps and standalone potion offers. The common tooltip renderer preserves the item description and associated cards. The V/Details reader pages long text and returns to the same reward focus. | `world_reward_ui_regression::reward_tooltip_routes`, `check_reward_hover` and `reward_tooltip_catalog_pages` cover pointer/keyboard ownership, seven route scenarios, modal input, page retention and Load reset. UI22's actual-resource gate retained every formatted character for 361 potion/relic models in each of five tested locales. Player check: hover a reward, open V, page it, close it and confirm the reward remains unclaimed. These historical visual results are not relabelled as new UI25 guest checks. |
| Continue popover overlaps Abandon or drifts right | `src/ui_frontend.inc::title_menu_bounds` measures all current localized labels, including the Timeline label. `include/title_scene.hpp::continue_bounds` places the panel in the available gutter and clears the logo/profile controls; `menu_height` handles the current six-row saved-run menu. | `world_frontend_ui_regression` exercises the current five/six-row menus and verifies menu, logo and profile pixels remain unchanged under the popover. UI22 visually inspected English/Chinese and a long French fallback. The final UI25 guest ran ordinary Continue/save/exit, but its receipt does not assert a fresh all-locale popover review. |
| Necrobinder slash and Osty death | `tools/actor_slot_materials.py` supplies the authored slash material; `tools/actor_storage_policy.py` protects its main/registered-piece images from repeat generic dicing/alpha reduction. The owner-death cleanup kills living Osty only after prevention fails; combat and game-over rendering share the actual pet die/dead-loop route. | UI25 full and reduced normal card actions passed after ordinary USB readback. Full retains the blue slash and 12 × 83 ms schedule; the exact corrected pixel group is 83,445 decoded bytes plus its separate 140-byte parameter table. Both corpses remain through 10.1 s. Reduced uses its documented essential poses; a full continuous slash is not promised from those three poses. |
| First attack or hit reaction stalls | `src/combat_preparation.inc` appends player attack, first-card targeting/impact/text and living enemies' current Hit tracks after existing preparation priorities, within 288 images / 640 pages. The actual gameplay and animation clocks are unchanged. | Final full/reduced Necrobinder/Nibbits first-Strike pairs add zero combat source reads/bytes/time and keep cumulative extrema equal to their idle controls. This does not cover every character/encounter. Saturated Regent/Nibbits can still leave optional attack preparation cold; this is disclosed, not fixed by increasing pools. |

The final ed5 CI record covers all **183 native suites** with ASan/UBSan,
Python discovery and portable builds. The review has not run that suite again:
the runtime source is unchanged. Targeted UI25 native and ordinary guest
results remain attributed to their recorded source and resources.

## Touchpad Custom Run check and historical exit issue

The original final numeric receipt explicitly states:

> No physical-calculator measurement, seven-Affliction stress rerun, or resolution of the historical first touch-start/exit trigger is claimed. Normal keyboard save/Continue/exit is covered.

The recorded failure was the initial touchpad Custom Run route's first exit
relaunching the application. Later keyboard Start/save/exit, a plain title exit
and an old UI23 touchpad Continue control passed; these are different routes.
The final receipt's `remaining_scenarios=[]` closes its defined release gate,
not this separately disclosed historical trigger. The supplementary current-
runtime check below adds a successful touchpad route without identifying or
repairing the original cause.

Production entry/exit handling exists in `platform/ndless/main.cpp`: ordinary
entry waits for a stable release; exit observes the keyboard and touchpad,
closes resource/report I/O, then repeats the bounded release gate before display
restoration. `include/touchpad.hpp` and `include/calculator_pad_input.hpp` handle
held-contact priming, pointer motion and releases. The native input regressions
exercise these components; they cannot establish OS handoff after this exact
physical gesture. No root cause or repair is inferred from the source review.

On 3 November 2026, the accepted ed5 launcher and e98 full resources ran through
the ordinary supplied 64 MiB Firebird/OS/Ndless path. All twelve program/data
files were transferred through normal USB and read back with matching hashes.
The launcher SHA-256 is
`9c578a2cb0ca079a3aea523e8fcfadf0aecef3e4aee5ca557423c9c238932b1f`.
Profile 1 retained the separate gameplay demonstration. The control below used
isolated Profile 2 after normal USB installation and byte-identical readback of
the legacy input described next; ordinary keyboard preparation selected that
profile. The tested menu/start/save/exit sequence itself used actual relative
touchpad movement and central mechanical press/release, not keyboard activation.

A separate, explicitly synthetic legacy-compatibility control was prepared on
the host using the unmodified production codec from historical public
`d556d767c8072f19a8243504a85f3e8a907a1cb9`. That version's run-menu source has no
Timeline mode gate. Its `clear_ui_progress` and `encode_ui_progress` produce a
16,810-byte empty NSH2 file, which round-trips exactly through its historical
decoder. The ed5 encoder produces identical bytes from the same empty input;
the ed5 decoder automatically migrates it to legacy access. All 57 Epoch states
remain None, with zero obtain dates, score, observed characters, wins/losses and
run history. Its normal unlock policy makes Custom available without a direct
flag edit. Its SHA-256 is
`ab617b58ca61345f21cf71fc7e3bd6b604676bfe24b6521b9fadd495a8312d7f`.

This reconstructs old-format Custom eligibility, not the lost original failing
profile or its exact gestures. It contains no claimed natural progression.

The observed touchpad route was **Singleplayer → Custom → Start → Ironclad at
Neow → Pause → Save & Quit → title with Continue → Quit → Yes → TI OS**. The
title's Continue entry was observed, not activated in this route. Each click
used contact, central press, central release and finger lift. The initial fast
Pause movement landed on the potion area and did not open Pause; its capture is
retained as a miss. Repositioning with 400/500 ms gesture intervals reached the
actual Pause control before continuing. No runtime, RAM or snapshot header was
modified.

The final Yes gesture is recorded at these simulated times:

| Event | Simulated milliseconds |
| --- | ---: |
| Finger contact | 540,790 |
| Central press | 541,190 |
| Central release | 541,590 |
| Finger lift | 541,990 |
| Initial TI OS capture, after a further 2,000 ms | 543,990 |
| Last TI OS capture, after the complete 30-second observation | 573,990 |

The initial OS frame plus one frame every 1,000 ms gives **31 captures at
320×240**, all pixel-identical. The OS remained visible at both 20 and 30 seconds,
with no automatic relaunch observed in that window. This current-runtime
compatibility-profile route passed. Its raw event/command log and captures are
separate from the earlier release receipt; they do not overwrite the historical
failure. The original failing profile and exact gesture sequence were not
recovered, and the historical trigger and root cause remain unknown. This is
not a claimed root-cause fix or a physical-handheld result.

The private `touch-control-result.json` receipt has SHA-256
`7808e3dcbef1a6c451f7781fa7ff7f747a87ef64939a374aec7b48982b64c359`;
its event log has SHA-256
`4340b25679483478e0822ba315839ddae1352eaeafccfe3853bf02c1d3c849a8`.
The recording session's journal remains failed because an earlier enemy-file
USB upload failed. The subsequent transfer retry, all twelve matching readbacks
and this scoped input result do not relabel that whole journal as passed.

## Later implementations supersede old retained differences

| Historical omission | Current status and remaining boundary |
| --- | --- |
| Eight static Ancient rooms / missing independent emitter families | The current `src/world_events_ui.inc` connects skeleton loops, light, fire, turbulence, water/post and particle modules. The later [UI23 ledger](ui23-retained-differences.md) identifies no missing active family in its bounded eight-room inventory. UI24 deliberately simplifies expensive effects under the user's later permission; the old missing-node counts are obsolete. |
| Full-density Tez fire/glow | The accepted 9×9 live material grid retains all 237 fire geometries and 506 body samples. Both foreground emitters keep all 24 particles; staggered background indices yield 192 retained cinders. Ninety glow planes use the consolidated soft field. These are explicit appearance/performance adaptations, not accidental missing callbacks. [Ancient acceptance](ui24-ancient-performance.md) reports same-clock review and ordinary guest measurement. Reintroducing full density would undo the accepted performance decision. |
| Eight discrete Affliction phases | ACU1 implements continuous clocks for nine materials across seven Affliction families, with all 55 authored visible particles. The eight-phase path is compatibility for historical packs. Infection's 30 and Ancient fire's 10 images are complete authored flipbooks, not missing interpolation. See [continuous materials](ui23-continuous-card-materials.md). The seven-family stress case remains a heavy workload without a claimed new UI25 optimization. |
| Timeline absent | All 57 Epoch states, actual obtain dates, score, reveal edges, frozen run unlocks and persistent profiles are implemented through normal UI. Initial fresh progression, legacy access, Silent unlock and Daily boundaries have separately identified UI24 ordinary firmware evidence. The LCD list adapts the source spatial board; Act2B/Act3B remain original Early Access placeholders. See [Timeline acceptance](ui24-timeline.md). |
| Only 32 runs / 96 floors exist | Those arrays are bounded working windows backed by the append archive; older successfully archived records remain browsable. Earlier versions' already-erased facts cannot be recovered. See [paged history](ui23-history-archive.md). |
| Only 64-byte ASCII seeds work | Direct editing remains 64 UTF-8 bytes. Explicit normal Custom Run import accepts the converter's identity code for the complete longer ASCII string; the source text is not silently truncated. Original-DLL ASCII/map/save comparisons and normal import UI evidence are in [seed acceptance](seed-ascii-ui24.md). The 2,048 points are optional map-pen samples, not generated rooms. |

## Genuine remaining scope and evidence limits

Audio, multiplayer and hosted/cloud services remain unimplemented. The retained
reports do not establish a user waiver for them. They are separate systems,
not narrow top-bar, tooltip or exit fixes, and this review does not count local
Daily/Custom gameplay as hosted-service equivalence.

The calculator has no added desktop timezone/DST database or IME. RGB565/LDR,
integer projection, documented material/noise approximation, deterministic
cosmetic seed banks and bounded resources remain real representation limits.
Unrecorded desktop cosmetic RNG/process history and arbitrary cross-track
composition are not reconstructed. Port-authored Timeline/history diagnostics
use Chinese/English fallback rather than claiming all sixteen translations.

Current ordinary Firebird results are tied to the supplied 64 MiB CX II CAS
configuration. They do not establish physical-handheld performance or every
possible cold action. The full default installation has **160,505 bytes** of
headroom under 70,000,000 decimal bytes; the launcher has **2,096 bytes** under
the formal allocation ceiling. These are measured release constraints, not a
reason to classify an unverified interaction as implemented or fixed.

## Build-script readability follow-up

`tools/build_ndless.py` and `tools/build_converter_validation.py` now expand
semicolon-separated statements, compiler/source arrays and long argument lists.
For each file, Python's parsed AST including type comments, with location
attributes omitted, is exactly equal to the ed5 baseline. Ordered comment-token
contents are also identical. Both actual `--help` commands exit zero, emit no
stderr and reproduce their baseline stdout byte for byte. `git diff --check`
passes. Import order, docstrings, compiler flags, source/argument order and
execution statements are unchanged. The [portable CI run](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/41637348636)
published launcher artifact `13231387011`. Its downloaded `nSpire2.tns` is
1,737,025 bytes, with SHA-256 `9c578a2cb0ca079a3aea523e8fcfadf0aecef3e4aee5ca557423c9c238932b1f`:
the exact launcher from both verified UI25 installations.
