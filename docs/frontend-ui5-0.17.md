# Frontend and event UI5 reference work

Reference: the owner's original v0.111.0 / `41cef1ea` PCK and assembly, matching
`config/reference-release.json`. Original assets, decompiled C#, captures and
generated resource packs remain outside the public source repository.

## Implemented source contracts

| Surface | Direct reference | Runtime change |
| --- | --- | --- |
| Settings | `settings_screen.tscn`, `settings_tab.tscn`, `settings_tickbox.tscn`, `tickbox.tscn`, `NSettingsTab` | The reward panel has been removed. Functional General/Input tabs sit above left-label/right-control rows and faint dividers. Tab selection uses the authored cyan additive stroke; focus uses gold lettering, 1.05 scale and a 0.5 s EXPO_OUT return. The original tickbox retains its 0.8 inner scale. |
| Character selection | `char_select_button.tscn`, `NCharacterSelectButton` | Pointer hover is presentation only; release on the pressed portrait selects. The existing authored gold selection outline now uses additive compositing. Hover and drag-cancel regression coverage protects selection and save state. |
| Map | `map_screen.tscn`, existing pinned map audit | Paper occupies the entire area under the HUD. The duplicated full-width act heading and bottom instruction bar are gone. The existing original transient act banner supplies the act heading; compact legend/scroll controls remain directly over the paper. |
| Ordinary event | `default_event_layout.tscn`, `event_option_button.tscn`, `NEventLayout`, `NEventOptionButton` | The right-column story is centered above one options stack. The unrelated dark column and repeated selected-option description over the illustration are gone. A single option label contains its gold/red bold title, newline and description; the focused outline is blue/additive. The story rectangle still opens the complete reader. |
| Resource storage | `tools/patch_frontend_assets.py` | NSP2/22 appends seven original settings/potion-popup images. All old image, clip and auxiliary indices, image/group payloads, relative retained-group links and localized payloads remain intact. Directory offsets and the resident decoded-cost table grow without a runtime scan. |

General contains the implemented fast-mode control and the pre-existing practice
and reference-map entries. Input documents the calculator bindings. Audio and
graphics controls have not been fabricated for unavailable calculator features.
Original settings tab labels use explicit English/Chinese fallbacks where v21
has no indexed string. Other existing labels retain the established UI locale
mapping.

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
pagination/hit bounds, complete text access, untouched illustration pixels and
unchanged run/RNG state. The synthetic retained-pack fixture includes ordinary,
LZ4, relative-group and tiled images and checks the v22 extension and bilingual
split/reconstruction path.

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
