# UI10 Settings and run setup source review

Pinned reference: v0.111.0 / 41cef1ea. This document records implemented source
contracts, matched execution evidence and their limits. It is not a measured
restoration percentage. Original scenes, fonts, artwork and captures stay
outside the public repository.

## Source layout and rendering

The projection is the same1680×1260 to320×240 viewport used by the other
original front-end screens. `settings_layout.hpp` and `run_setup_layout.hpp`
name the resulting coordinates. The review includes the relevant scenes,
their inherited controls, container minimum sizes, locale font substitution
and the business methods that alter the authored initial appearance.

| Surface | Implemented source contract |
| --- | --- |
| Settings tabs | Source256×90 controls project to49×17 with6px labels. The two supported host tabs are centered without enlarging their controls. English keeps authored tracking; Chinese ordinary labels use the runtime Regular substitution. |
| General | Source64px rows and82px pitch produce12px controls and eleven supported rows, including Long Press Confirmations and Reset. The separate original platform/feature controls listed below remain unavailable. Labels use5px regular type. Checkbox artwork is10px after the source child0.8 scale; only directional focus shows the selection corners. Source61×12 action buttons and9px paginator arrows replace enlarged older thumbnails. |
| Input | The source1100px panel,308px command column and64px rows show ten commands. Long command names shrink from5px to4px inside their original column. Binding labels retain their own5px band. The inactive binding bank uses the source disabled red; unavailable bindings remain gray. The Listening modal uses its source267×38 backstop and12px animated label. |
| Language | The original320px dropdown control projects to61px. Its44px item rows retain4px normal labels, including the original Chinese fallback in the English pack. The dropdown opens on its first item; Up/Down stop at the ends; Left/Right remain on the item. Selecting the current locale leaves the menu open. |
| Singleplayer | Source330×705 roots project to63×134 atX52/129/205. Their wider362px child backgrounds are69×134. Icons retain the source284×216 aspect at54×41; titles are6px and descriptions5px. Focus immediately applies scale1.025 and material value1. Unfocus uses500ms cubic scale and1000ms exponential material transitions. There is no invented press shrink. |
| Custom | The9px Spectral heading,7px Modifiers heading,5px rules/seed fields,7px modifier checkboxes and original portrait positions replace generic oversized controls. The Randomize button follows the measured heading width: X213 in English and195 in Chinese. The actual Ascension HBox expands to844 source pixels from its children and spacing; the visible controls use that expanded layout. |
| Daily | The9px heading,7px Leaderboards heading,6px Modifiers heading and5px body/date labels retain their source hierarchy. The date uses the blue assigned by `NDailyRunScreen.InitializeDisplay`, rather than the scene's initial loading color. The extra Ascension text line is removed: its original label is hidden and `NDailyRunCharacterContainer.Fill` never shows it. |

`MegaLabel._Ready` refreshes ordinary fonts through the active locale's Regular
role. Therefore Chinese authored Kreon Bold or Spectral labels do not imply
Chinese bold text. Explicit rich bold spans keep their separate face. The new
`config/run-setup-ui-fonts.json` describes the exact source sizes and roles,
including dynamic dates, seeds, input keys, Ascension numbers and native
language labels. Sparse9px headings opt into scene font selection; ordinary
card and tooltip fitting cannot choose this incomplete alphabet.

`tools/settings_ui10_assets.py` extracts22 original controls directly from
the pinned pack at their source-projected sizes. It applies the scene's aspect
mode, atlas region, tint and material before encoding, including distinct
rest/focus menu backgrounds. These append at auxiliary2132–2153 after the64
History room icons in the combined NSP2/39 conversion. No older image is
resized in place. The full converter owns old-payload preservation checks.

## Input and ownership

`NSettingsPanel.UpdateNavigation` clamps vertical navigation and keeps lateral
navigation on the selected control. The host's separate Inspect shortcut
changes the two supported tabs; arrow keys no longer change tabs accidentally.
`NDropdown.OpenDropdown` supplies the first-item and boundary rules above;
`NLanguageDropdown.OnDropdownItemSelected` closes only after a real locale
change. General rows no longer expose a fake scrolling offset.

`NCustomRunScreen.InitCharacterButtons` gives the first and last portraits
self-neighbors at their outside edges and sends Up to the seed field.
`NCharacterSelectButton` selects on `FocusEntered`, so keyboard focus now
updates the selected character immediately. These presentation changes do not
alter canonical run seeds, modifier selection rules or simulation save v17.

Small control motion reuses the existing bounded button slots. Modifier text
layout uses bounded32-line stack bookkeeping and draws each glyph before
requesting the next glyph; it does not hold cache-owned pixel pointers across
reads. No resident pixel or text buffer is added by this batch.

The H follow-up closes the Input-row focus-release gap: the row appears at
alpha0.2 immediately and fades linearly to zero over100ms. Refocus kills the
interrupted fade. Command identity remains stable when scrolling, and closing
Settings or leaving Input releases its owned slots without touching other
controls. The independent floating-point timeline and composed pixel checks
cover pointer-to-keyboard transitions, interruption, scrolling and reopening.

## Evidence and remaining limits

A private Godot4.5.1 layout experiment with the original fonts measured the
English/Chinese Modifiers headings at164/72 source pixels, seed labels at62/84,
and the expanded Ascension HBox at844. This is a translated native layout and
font experiment, not an unmodified original-game full-frame screenshot.

The22-image private extraction completed successfully. G's first matching
resource conversion failed because the offline parser omitted the new9px
heading band; two native fixtures also still clicked the old Settings Back
location. H fixes the bounded format39 grammar and uses the original Back hit
rectangle. The failed G records remain separate from later successful gates.

Matching I native passes121/121 suites, including Settings/input/profile/run
setup and long-press regressions. Both format39 actual-language runs pass the
front15, root17 and combat16 targets. This covers the source fonts/artwork,
General/Input panels, source Singleplayer/Custom/Daily controls and input
ownership. I ARM, WASM and Python gates also pass; ARM compilation is not
physical-calculator timing evidence. The ordinary browser coordinates are
derived from the pinned source control positions and use only real DOM/canvas
input, with no application debug API.

The supported Settings panels remain General and Input; unsupported audio,
graphics and controller panels are not fabricated. Offline Daily leaderboards
remain explicitly unavailable. The very small Daily Ascension number keeps
the established4px calculator minimum instead of its approximately3px source
projection. Those adaptations and limits are not counted as exact source
restoration.


## I follow-up: real long-press confirmations

The pinned `settings_screen.tscn` exposes LongPressConfirmations in the release
General panel without a platform or controller visibility condition.
`NLongPressConfirmationTickbox` writes the per-profile `PrefsSave.long_press`,
which defaults to false. I adds that real option after Hand Card Count. The
remaining rows move down one source82px pitch; their fonts and controls retain
the previous source projection. Both original localized hover keys are used.

`NEndTurnLongPressBar._Process` supplies the strict greater-than500ms trigger,
linear release decay, and continued progress after a short release/repress.
The visual uses its authored204×6 ColorRect and outline, with
`CubicOut(0.75 * elapsed / 500)` alpha, the source unclamped color ramp, and a
250ms linear completion fade. The bar is a sibling of the pressed Visuals node,
so its projected(269,186) position does not move with the button artwork.
The largest fill is39×1 pixels. Rendering has no new image, full-screen pass,
resident pixel pool or glyph cache.

Mouse capture, both physical key banks, calculator matrix edges and semantic
HTML button edges share one20-byte timer. Owners survive an unfocus until their
real release; repeat cannot start the next turn. Pointer exit, another focused
control, pause/modal/context changes and host cancellation stop accumulation.
A completed hold fires once, including when synchronous game logic immediately
returns to Combat. Default-off input retains the prior immediate behavior.
The source short-release playable-cards tutorial remains available; completed
holds call the release logic directly, and the CannotPlayCard tutorial's
existing special end-turn control retains its source bypass.

NSPF version5 assigns the formerly reserved bit7. The record remains108 bytes,
including both original key maps; versions1–4 remain readable with long press
off and still reject that formerly reserved bit. The future version6 negative
case remains invalid. General Reset clears the preference, profile switching
loads each profile's value, and neither canonical NSV1/version17 nor NSH9 grows.

`command_key(Key,bool)` captures one semantic press. A matching ordinary
release calls the former `input` plus map-drawing release pair. When Long Press
is enabled for Combat EndTurn, down starts the shared owner and up cancels it;
no release is dispatched into a new phase. `command_keys_cancel` cancels only
the semantic owner, retaining any concurrently held physical key and its repeat
latch. Window blur uses `physical_keys_cancel`, which also cancels semantic
input. A stray release after cancellation is ignored.
The legacy one-shot EndTurn API cannot bypass the enabled Combat hold gate;
its noncombat uses, such as confirming a selection, remain available.

The independent timeline oracle covers threshold500/501, release decay,
repress, completion fade and owner conflicts. `end_turn_hold_regression` also
covers ordinary mouse, mapped physical and calculator input, both keyboard
modes, semantic release/cancel, stale repeats across turns, canonical-state
immutability before completion and source bar composition. Preference and
profile regressions retain explicit legacy-format cases. Matching frozen I
now passes these gates: `ui10i-native-e108c5719b58.json`, the two front journals
`ui10i-front-eng-484de2c1322c.json` and
`ui10i-front-zhs-082c1fd5df37.json`, and the two combat journals
`ui10i-combat-eng-593b644de463.json` and
`ui10i-combat-zhs-9dae868f268e.json`. Each actual long-press target passes155,303
checks with its matched pack. Short diagnostic results remain identified as
short diagnostics, rather than substituted for those executions.

The ordinary final I HTML passes21 flows and47 captures in
`ui10i-ordinary-browser-3bdf52198bec.json`. The added flows toggle the real
Long Press checkbox, preserve the actual first-turn save after short toolbar
and keyboard taps or pointer exit, then advance exactly one turn per held
mapped key and held HTML toolbar control. A real OS-repeat event cannot
advance an extra turn, and releasing after completion cannot dispatch into
the next phase. Save/reload retains the NSPF5 preference and NSV17 run, while
the other two profiles' canonical saves stay byte-identical. StillCanPlay and
Shuffle confirmations execute through the normal tutorial UI. The page has
no errors or external requests, and its before/after hash matches the closed
delivery manifest.

### J follow-up: precise Settings hover coverage

Final capture review found that three old `card_hud_ui_regression` capture
names still used pre-restoration Settings coordinates. Their I pass did not
prove that Screenshake, Text Effects and Keyboard-Only hover boxes were shown.
J changes only that fixture: independently projected source row coordinates,
explicit localized title/body bindings and unchanged canonical bytes are
checked for those three rows and the new Long Press row. The expected hover
composition reuses the shared tooltip painter, so it verifies Settings
ownership/binding rather than independently proving that painter's raster.
Matching J test execution against the production-identical I core and both
actual packs remains pending; no production or resource change is implied.

### General rows resolved against release visibility and consumers

CommonTooltips is hidden in the authored scene and its callback is unimplemented;
ResetTutorials is shown only outside release builds, while the pinned
`NGame.IsReleaseGame()` returns true. They are not missing release controls.
ShowMpDrawings affects other players' lines and belongs to the user-excluded
multiplayer scope. LongPressConfirmations was a real single-player gap and is
implemented above.

SkipIntroLogo is visible in the original and controls a separate startup logo
sequence; the port currently starts on the title scene, so no equivalent
startup animation/skip setting is implemented. LimitFpsInBackground controls
the original desktop focus-out30fps cap; the browser currently suspends hidden
updates instead and the calculator has no equivalent window-focus lifecycle.
UploadGameplayData gates original remote metric/Sentry uploads; this offline
port has no such upload service. These are explicit platform/feature
differences, not hidden controls or evidence of complete Settings parity.
