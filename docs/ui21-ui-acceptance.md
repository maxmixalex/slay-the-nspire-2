# UI21 UI and saved-run acceptance

## Scope and source checkpoint

The UI implementation under review is UI20 (`f4875c2`). The production decoder
used by the final native build is `7b0cd8b4f2fd824eea781e0e0a853011c3474b1c`.
UI21 restores the matching UI20 source-derived artwork into complete language
packs and checks the actual resulting UI. It does not count diagnosis, source
presence, or a launcher-only build as delivery of the matching assets.

Original references are pinned to Slay the Spire 2 `v0.111.0` / `41cef1ea`.
The scene and C# references inspected for this review include
`map_room.tscn`, `ancient_event_layout.tscn`, `common_banner.tscn`, the score-line
scene, and `RunManager.Abandon` / `RunManager.OnEnded`.

## Reported issues and corresponding acceptance

| User feedback | Implementation and acceptance exercised |
| --- | --- |
| nSpire version at the bottom right | `ui_frontend.inc` right-aligns the measured label at `ScreenW - 2`; English and Chinese actual-art title captures were reviewed. |
| Die for You and other status tooltip icons | `keyword_tooltip_ui.inc` resolves canonical and instanced power icons. Actual-art English and Chinese checks decoded 272 canonical power icons and 237 concrete status icons per language. The one excluded internal debug slot is the abstract original `TemporaryStrengthPower`, which has no concrete source icon. Die for You was visually inspected in the production Canvas. |
| Associated cards reserve nonexistent keyword-tooltip space | The common hover layout counts actual text tips separately from associated cards. `keyword_tooltip_regression` covers card-only and combined hover stacks across their shared callers; final actual-pack runs are part of the release gate below. |
| Ritual Beast animation loses resolution | The artwork lane supplies new source-derived frames and independently checks all affected Canvas frames. This document records the UI lane; its source-pixel evidence belongs to the asset report. |
| Gold card-reward icon | `world_reward_ui.inc` selects the new rare/uncommon resource IDs. Both the matching resource bytes and the actual-pack reward test are required. |
| Ancient background starts below the top bar | The reference scene fills its parent. The actual-art Neow frame was inspected with its backdrop extending behind the top bar; event-room regression also exercises the other Ancient compositions. |
| Act 2/3 map has yellow background | `world_map_ui.inc` clears the standalone map room to black, matching the reference `map_room.tscn`. The map regression routes to the next act; the filesystem gate also opens saved act-2 and act-3 standalone maps. |
| Defeat label too low; second page too wide | The banner title follows the source label center (approximately 39.2% of banner height), and score rows use the source icon/label/value arrangement in a compact centered group. Actual-art defeat and summary captures were reviewed. |
| Direct abandonment leaves a continue save | The App retains the identity of the run linked by Continue/save, retires the run on abandonment or death, and calls the production Ndless deletion of primary, backup, and temporary files. The filesystem gate checks a fresh process after each terminal flow. |
| Osty lacks a death animation | The complete pack retains the 17-frame `die` action, adds the source `dead_loop` pose, and uses the corrected terminal-impact trigger. Death plays through before the corpse remains, and revival returns through its existing action. Actual-pack UI tests exercise ordinary and terminal-damage presentation; the asset report owns the frame/source oracle. |
| Continue metadata popover too far right | `title_scene::continue_x` uses the localized rendered label width. The English and Chinese actual-art title captures were inspected. |
| Defeat-to-menu fade missing | `world_game_over_ui_regression` checks the 800 ms fade out, switch at black, 800 ms fade in, and release/input gates for all five characters and victory. |

The reference score row is 520 x 48 with a 36 x 42 icon and right-aligned points.
The reference terminal-run flow writes history and then deletes the single-player
current run. These references informed the review of the scaled calculator UI
and its persistence boundaries.

## Executed native evidence

At production checkpoint `7b0cd8b`, all 158 existing CTest targets have actually
executed and passed. The initial final CTest journal contains 156 passes and two
`Not Run` entries caused by lost executable permission on built files
(`input_regression` and `foreground_pointer_regression`). Restoring those file
permissions and rerunning exactly those two targets passed; there was no failed
test assertion. The paired journals are:

- `ui21-final-native-all-36675e2499e1.json`
- `ui21-final-native-permission-retry-0f9f542ac6aa.json`

Both report unchanged source. The final native build and extra actual-resource
targets also passed in `ui21-final-native-build-d955dc9c13ad.json` and
`ui21-final-extra-targets-a3b4a7253ecc.json`.

An earlier full-artwork filesystem gate passed with the actual Ndless callbacks:
Neow abandonment, engine-forced death, act-2 abandonment, act-3 abandonment, and
Chinese Neow abandonment. It used separate prepare, terminal-flow, and reopen
processes and checked all three saved-run files plus unrelated settings,
progress, language, and other-profile markers. It also checked that terminal
Save cannot recreate a Continue entry. The retained journal is
`ui21-native-filesystem-308c130099d7.json`.

## Permanent filesystem regression

`ndless_run_lifecycle_regression` is a UNIX native integration test built from
the existing public synthetic resource fixture. It includes the actual
`platform/ndless/run_storage.inc` callbacks, ordinary host files, and the App
menu lifecycle. Four scenarios run through twelve independent processes:
prepare, abandon/death, then reopen. The fixture creates only its own temporary
directory. It checks deletion of the active primary/backup/temporary files,
preservation of unrelated profile/state markers, inability to save a terminal
run, the summary-to-menu transition, and absence of Continue after restart.

At this source checkpoint the new target is staged for its first runtime check
after publication, as required by `AGENTS.md`. It has passed syntax checking.

## Final actual-pack release gate

The remaining UI gate runs these eight existing production-Canvas targets
against each final English and Simplified Chinese logical pack:

- `world_frontend_ui_regression`
- `world_game_over_ui_regression`
- `world_event_ui_regression`
- `world_map_act_intro_ui_regression`
- `world_reward_ui_regression`
- `keyword_tooltip_regression`
- `world_ui_regression`
- `world_room_ui_regression`

The gate records each binary and pack SHA-256, result, and capture paths.
The filesystem/restart and power-icon scan is then repeated with the final
compressed packs and the final decoder. Those results will be appended once
the compression lane supplies its final resources.

## Evidence boundaries

These checks execute native App/Canvas code and actual Ndless storage callbacks
on a host filesystem. They are not a Firebird or physical-calculator run and do
not establish a physical-device frame rate. The source-art oracle and the
decoder byte/pixel oracle establish resource fidelity separately from visual
inspection. Delivery acceptance also requires that the final ZIPs contain the
tested launcher and corresponding complete resource packs.
