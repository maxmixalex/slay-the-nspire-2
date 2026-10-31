# UI21 UI and saved-run acceptance

## Scope and source checkpoint

The UI implementation under review is UI20 (`f4875c2`). The production decoder
used by the final native build is `7b0cd8b4f2fd824eea781e0e0a853011c3474b1c`.
The final actual-resource acceptance below ran at
`9d5f9966bb061cec48f7f6efeef131aa243540c6`, whose intervening changes cover
offline encoding policy, the new filesystem regression, and documentation.
The runtime used by these tests remains the published `7b0cd8b` runtime.
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
| Associated cards reserve nonexistent keyword-tooltip space | The common hover layout counts actual text tips separately from associated cards. `keyword_tooltip_regression` covers card-only and combined hover stacks across their shared callers; both final actual-pack runs passed. |
| Ritual Beast animation loses resolution | The artwork lane supplies new source-derived frames and independently checks all affected Canvas frames. This document records the UI lane; its source-pixel evidence belongs to the asset report. |
| Gold card-reward icon | `world_reward_ui.inc` selects the new rare/uncommon resource IDs. The matching resource bytes are present and both actual-pack reward tests passed. |
| Ancient background starts below the top bar | The reference scene fills its parent. The actual-art Neow frame was inspected with its backdrop extending behind the top bar; event-room regression also exercises the other Ancient compositions. |
| Act 2/3 map has yellow background | `world_map_ui.inc` clears the standalone map room to black, matching the reference `map_room.tscn`. The map regression routes to the next act; the filesystem gate also opens and captures saved act-2 and act-3 standalone maps. Their paper surrounds are black, while the source map paper retains its chapter-specific color. |
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

The new target was first built and executed after publication of
`c8dbcd16739762b389858e912742dd384f301f96`. All four scenarios and twelve
independent processes passed in `ui21-lifecycle-target-4c9216a0a175.json`, with
unchanged source. Its successful build journal is
`ui21-lifecycle-target-build-05a7932a0bea.json`. Together with the existing 158
targets, all 159 native targets have executed and passed against the same
production runtime.

## Final actual-pack release gate

All sixteen runs passed: these eight existing production-Canvas targets were
each run against the final English and Simplified Chinese logical pack:

- `world_frontend_ui_regression`
- `world_game_over_ui_regression`
- `world_event_ui_regression`
- `world_map_act_intro_ui_regression`
- `world_reward_ui_regression`
- `keyword_tooltip_regression`
- `world_ui_regression`
- `world_room_ui_regression`

The gate records each binary and pack SHA-256, result, and capture paths, with
664 captured frames in total. The successful journals are
`ui21-release-ui-eng-8d4fdc1aae18.json` and
`ui21-release-ui-zhs-7e8ab753cc6c.json`; both report unchanged source. The combined
private acceptance receipt is `validation/ui-release/receipt.json`.

| Final logical resource | Bytes | SHA-256 |
| --- | ---: | --- |
| English | 68,726,154 | `36e7c560558617053081bc4c731d838a3916801f6f9ce3cc8747e1c77dc94e40` |
| Simplified Chinese | 73,150,868 | `6094d63ef015d691252e5bb788aed356f7b252d1cd77b5fc6d322646f59ce2ee` |

These are the reconstructed logical resources used by the native harness;
installed bilingual size shares artwork and is measured separately by the
packaging report. These final resources preserve original font/text streams
and use the density-limited sparse-tile policy chosen after the ARM instruction
comparison. Earlier candidate receipts are superseded for release identity.

The final filesystem/restart and power-icon scan also passed, in
`ui21-release-filesystem-ff3aef9c7ebb.json`, with unchanged source and the same
two resource hashes. Its private receipt is
`validation/filesystem-release/receipt.json`. Six saved-run scenarios each use
three independent processes, for eighteen lifecycle processes:

- English title-menu abandonment.
- English Neow pause-menu abandonment.
- English forced engine death followed by ordinary terminal-run retirement.
- English act-2 standalone-map abandonment.
- English act-3 standalone-map abandonment.
- Chinese Neow pause-menu abandonment.

Every scenario checked removal of the active primary, backup, and temporary
save; preservation of unrelated profile/settings/progress/language markers;
and absence of Continue after an independent process reopened the installation.
Terminal Save could not resurrect the run. The five in-run terminal flows also
passed through the defeat summary and return-to-menu transition.

Two additional processes scanned both languages' power-icon bindings. Each
language passed all 272 canonical bindings and all 237 concrete instanced
status bindings, excluding the single abstract internal slot described above.
All twenty final subprocesses passed.

Visual review covered both languages' title/Continue panel, defeat ribbon,
compact score rows, Ancient backdrop, and Die for You tooltip, plus the opened
act-2 and act-3 standalone maps. Some existing game-over captures intentionally
show a debug-forced death in Neow and a summary scrolled to its discoveries;
their source debug death quote and clipped scroll position are test context,
not the default presentation of an ordinary abandoned run.

## Evidence boundaries

These checks execute native App/Canvas code and actual Ndless storage callbacks
on a host filesystem. They are not a Firebird or physical-calculator run and do
not establish a physical-device frame rate. The source-art oracle and the
decoder byte/pixel oracle establish resource fidelity separately from visual
inspection. Delivery acceptance also requires that the final ZIPs contain the
tested launcher and corresponding complete resource packs.
