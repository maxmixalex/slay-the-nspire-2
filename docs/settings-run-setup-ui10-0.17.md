# UI10 Settings and run setup source review

Pinned reference: v0.111.0 / 41cef1ea. This document records implemented source
contracts and their limits. It is not a measured restoration percentage or a
release acceptance receipt. Original scenes, fonts, artwork and captures stay
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
| General | Source64px rows and82px pitch produce12px controls and the complete ten-row panel, including Reset. Labels use5px regular type. Checkbox artwork is10px after the source child0.8 scale; only directional focus shows the selection corners. Source61×12 action buttons and9px paginator arrows replace enlarged older thumbnails. |
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

## Evidence and remaining limits

A private Godot4.5.1 layout experiment with the original fonts measured the
English/Chinese Modifiers headings at164/72 source pixels, seed labels at62/84,
and the expanded Ascension HBox at844. This is a translated native layout and
font experiment, not an unmodified original-game full-frame screenshot.

The22-image private extraction completed successfully. C++ syntax checks cover
the changed render/input units and Settings/run-mode regression fixtures;
Python configuration/tool syntax and the ordinary browser script parse check
also pass. Matching G native/ARM/WASM builds, actual bilingual font/artwork
rendering and browser execution are still release gates, to be run only after
the coherent source checkpoint. The new ordinary browser coordinates are
derived from the pinned source control positions and use only real DOM/canvas
input, with no application debug API.

The supported Settings panels remain General and Input; unsupported audio,
graphics and controller panels are not fabricated. Offline Daily leaderboards
remain explicitly unavailable. The very small Daily Ascension number keeps
the established4px calculator minimum instead of its approximately3px source
projection. Input-row focus release currently clears immediately rather than
the original100ms fade. Those adaptations and limits are not counted as exact
source restoration.
