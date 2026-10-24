# Frontend and event UI5 reference work

Reference: the owner's original v0.111.0 / `41cef1ea` PCK and assembly, matching
`config/reference-release.json`. Original assets, decompiled C#, captures and
generated resource packs remain outside the public source repository.

## Implemented source contracts

| Surface | Direct reference | Runtime change |
| --- | --- | --- |
| Settings | `settings_screen.tscn`, `settings_tab.tscn`, `settings_tickbox.tscn`, `tickbox.tscn`, `NSettingsTab` | The reward panel has been removed. Functional General/Input tabs sit above left-label/right-control rows and faint dividers. Tab selection uses the authored cyan additive stroke; focus uses gold lettering, 1.05 scale and a 0.5 s EXPO_OUT return. The original tickbox retains its 0.8 inner scale. |
| Character selection | `char_select_button.tscn`, `character_select_screen.tscn`, `confirm_button.tscn`, `NCharacterSelectButton`, `NCharacterSelectScreen` | Pointer hover is presentation only; release on the pressed portrait selects. The authored gold selection outline uses additive compositing. The invented full-width footer is removed. The formal-run confirmation uses the original cream tick without a text label; the separate practice entry retains its existing label. Hover and drag-cancel regression coverage protects selection and save state. |
| Map | `map_screen.tscn`, existing pinned map audit | Paper occupies the entire area under the HUD. The duplicated full-width act heading and bottom instruction bar are gone. The existing original transient act banner supplies the act heading; compact legend/scroll controls remain directly over the paper. |
| Ordinary event | `default_event_layout.tscn`, `event_option_button.tscn`, `NEventLayout`, `NEventOptionButton` | The right-column story is centered above one options stack. The unrelated dark column and repeated selected-option description over the illustration are gone. A single option label contains its gold/red bold title, newline and description; the focused outline is blue/additive. The story rectangle still opens the complete reader. |
| Resource storage | `tools/patch_frontend_assets.py` | NSP2/22 appends seven original settings/potion-popup images and three original star-counter layers. All old image, clip and auxiliary indices, image/group payloads, relative retained-group links and localized payloads remain intact. Directory offsets and the resident decoded-cost table grow without a runtime scan. |

General contains the implemented fast-mode control and the pre-existing practice
and reference-map entries. Input documents the calculator bindings. Audio and
graphics controls have not been fabricated for unavailable calculator features.
Original settings tab labels occupy the previously empty slots 8103 and 8104.
The converter reads both languages from the pinned PCK and refuses an input
whose reserved slots are occupied. It appends localized bytes without changing
any of the 935 earlier UI bindings or pre-existing string payloads. All five
existing font sizes already contain the required glyphs. The runtime accepts
those new keys only for v22; older resources retain English tab fallbacks.
Other controls use existing indexed labels, including separate original map
and potion-slot titles.

The original character screen instances `ConfirmButton` without a text override.
Its scene contains image, tick, outline and hotkey children, with no label or
localization key; `NCharacterSelectScreen` connects its release to
`OnEmbarkPressed`. The tick is 80 px on the 267 px button artwork and uses
`(1, 0.964706, 0.886275)` modulation. The calculator centers this original icon
and bounds it to the available control height. The old `启程` UI binding pointed
to the unrelated Standard-mode title; it remains at its old index for pack
compatibility, but this entry point no longer asks for that string. Existing
nonempty-label native controls retain their drawing path.

## Concrete remaining differences

- Original settings use a blurred parent backstop. The calculator currently
  dims the existing scene; it does not reproduce the GPU blur. Original text
  sizes and 1012 px rows are reflowed for the 320×240 display.
- Original settings persist preferences on close. Fast mode is still the
  existing runtime preference; this pass does not add a new preference file.
- `NEventLayout.AnimateIn` delays/fades title and description and reveals the
  narrative. `NEventOptionButton.AnimateIn` staggers rows with an authored
  horizontal BackOut arrival. This pass changes ordinary-event composition,
  typography and focus blending; those entrance sequences are still separate
  work. The source event body is unscrollable; the calculator's complete reader
  remains necessary for long text at readable sizes.
- Original event buttons tween focus to 1.01 in 50 ms, press toward 0.99 and
  fade their focus outline over 300 ms. The ordinary-event rows currently have
  static selected/locked shading; the full timing curve is not yet represented.
- Ancient events and the fake merchant retain their earlier dedicated layouts.
  The ordinary-event change must not be described as a replacement for those
  layouts or as evidence that all event scenes now match the original.
- Source ratios cannot be copied uniformly to 4:3 without making labels and
  controls too small. Portrait cropping, the event reader and enlarged minimum
  input targets are explicit calculator adaptations.

## Validation boundary

The frontend regression exercises hover without selection, pressed/released
tabs, keyboard tab changes, native tickbox targeting, modal isolation and
unchanged serialized run state. The event regression covers ordinary option
pagination/hit bounds, Up/Down wrapping, complete text access, untouched illustration pixels and
unchanged run/RNG state. The synthetic retained-pack fixture includes ordinary,
LZ4, relative-group and tiled images and checks the v22 extension and bilingual
split/reconstruction path. It compares all earlier localized strings and
rejects attempted overwrites of the two new label slots.

At stage 2 checkpoint `eb6f9564c6f3c88bc47b73829036af21d0489072`, actual
English and Chinese event, map and reward UI regressions passed. Frontend
testing stopped on its obsolete v21-only version assertion; that failure is
retained, and the assertion now permits later compatible resource profiles.
Stage 2 screenshots exposed a card-reward heading overlapping the relic band;
the heading now starts below that band. The restored map viewport also exposed
a folded-legend/back-button overlap. They now have separate hit regions, and
the expanded legend uses its own close control.

At stage 3 checkpoint `55fdd6e703cd921220740f41e1537f864a02b300`, the four
actual-pack frontend/event/map/reward suites pass in both languages: eight
runs and 168,744 checks. The corrected character-selection capture observes
the first frame before advancing its entrance, and both final language packs
pass the stage 4 frontend suite at `1ad378862fae54fc8c8b346c2680b5f448e26883`.
Manual inspection of those settled captures exposed the old Standard-mode
label on the character confirmation. The source correction above and its
actual-art centered-tick / click-to-start regressions are recorded separately
from those historical stage 4 passes.
At final checkpoint `1c774c0847ef8e47b2bd23fd3bc6a11635d8e219`, both accepted
language packs pass frontend, card/HUD and potion UI regressions: six runs and
786,962 checks under ASan/UBSan. Actual atlas pixels establish the centered
cream tick; pointer actions establish that hovering preserves the selection,
clicking selects Regent and a separate confirmation click starts that run.
Both settled character captures were inspected, along with the Fire Potion
and disabled Fairy in a Bottle menus. The formerly premature potion fixture
now waits for the real bounded preparation screen and passes its original
805 checks in each language.
The accepted Chinese resource subsequently changes only glyph payload order;
the performance report records exact glyph geometry/pixels, unchanged other
bytes and 18 exact frame pairs. See
[executed frontend evidence](validation-ui5-frontend-0.17.json) for source,
resource hashes, failed-fixture provenance and explicit remaining scope.

These tests and desktop captures do not measure touchpad latency, frame pacing
or cold resource I/O on physical hardware. Calculator/Firebird timings must be
reported separately, with their exact resource version and cache conditions.

Private conversion command (paths are owner-provided):

```sh
python tools/patch_frontend_assets.py --pck /private/SlayTheSpire2.pck \
  --input-eng /private/ui4/complete-eng.nsp --input-zhs /private/ui4/complete-zhs.nsp \
  --cache-dir /private/frontend-assets --output-dir /private/frontend-v22
```

Capture-capable targets accept a resource pack and an existing private output
directory as arguments: `world_frontend_ui_regression` emits `settings-general`,
`settings-input`, `settings-checked`; `world_event_ui_regression` emits
`event-wellspring`, `event-colorful-philosophers`, `event-relic-trader` in addition
to its existing ancient/merchant scenes. Both language packs must be inspected.
