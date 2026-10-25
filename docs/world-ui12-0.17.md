# UI12 character, map and world-room adaptation

This stage follows the user's second calculator feedback pass. The reference
remains v0.111.0 / 41cef1ea. Changes retain original art and room state while
adapting placement, clipping, pointer interaction and text to the 320 × 240 LCD.
Actual calculator acceptance is recorded separately from native contracts.

## Character selection and departure

Six portrait positions now include the original Random option. Random selection
uses the source character order after the three act-selection draws, derived
from the selected run seed. It creates one run, reveals the selected character
for one second, then uses the original character transition mask for an 800 ms
departure and an 800 ms incoming room fade. Repeated input cannot create a
second run during this sequence. Existing direct native start APIs remain
immediate; player-facing start actions use the animation.

Portraits and horizontal navigation follow the original model order:
Ironclad, Silent, Regent, Necrobinder, Defect, Random. A display mapping preserves
the existing canonical character enum and all saved character IDs. Pointer
actions, selected-frame animation and keyboard wrap use the same displayed
positions; seeded Random resolution retains its original independent stream.

The character description has a larger inset. Long description and relic text
remain clipped to that interior and scroll instead of hiding the final lines.
The ascension numeral has a thin outline and is centered in its flame.

The Necrobinder's missing skull flame and rear Osty fire were a conversion
coordinate defect. A Control below a SpineSlotNode has an empty immediate
parent anchor rectangle in the original engine. The converter incorrectly
inherited the grandparent's full canvas rectangle. The repair uses the empty
slot rectangle and retains all 96 samples over the original 8,000 ms clock.
The corrected source frame and its static plane are disjoint and reconstruct
the fully rendered frame before the existing bounded UI11 color conversion.

The private targeted retile scans every retained tile descriptor, protects all
live tile IDs, recycles only dead slots, and copies untouched tile/index groups
verbatim. Replacing the static image and 96 animated images adds **98,098 bytes**
to each UI11 logical pack. It does not retain the 8.42 MB independent-frame
intermediate as an installed payload. The existing RGB454/alpha6 precision is
unchanged; no animation frame or duration is removed. Optional background water
distortion and decorative particles remain a static LCD approximation.

## Map and rooms

- The map legend opens from a compact lower-right button. Its icon/label rows
  use explicit aligned columns and 17-pixel spacing. The drawing toolbar's
  tooltip opens to its right and above the controls. Map close uses the native
  Back presentation when the map is an overlay; the initial travel map does
  not receive a false Close action.
- Ordinary event prose uses an in-place clipped scroll container and source
  scrollbar. Pointer wheel, body dragging, thumb dragging and keyboard reading
  focus use the same scroll state. Detail no longer opens a duplicate reader.
  Event option text is clipped inside its actual texture, and focused overflow
  scrolls within that button.
- Ancient backgrounds meet the bottom of the HUD without a seam. The entrance
  title is higher, then settles in the upper-right. Ancient reading focus stays
  in the original conversation container; long option text stays in the
  texture's inset and scrolls. Full new Ancient background animation is not
  included in this storage budget, as allowed by the user.
- Merchant stock is registered against the rug with compact separate rows.
  The hand retains its fingertip target with a smaller scaled image. Removal
  service uses its source tooltip. Immutable rug alpha coverage is memoized in
  64 bounded entries, eliminating repeated per-frame coverage scans without
  approximating the result. Ordinary merchant card-face caching is integrated
  by the combat stage and must be measured in the final emulator build.
- Rest-option outlines now enclose the whole icon and labels use readable
  10-pixel text. The actual smith upgrade animation belongs to the integrated
  combat stage and is included in the room acceptance sequence.

## Verification and persistence

Source stages: `5315dcd` (rooms and map), `3414b93` (slot anchors), `05aefa9`
(character selection/departure), `9bc21e0` (obsolete run-menu test geometry),
`311c1b0` (bounded retile), and `8d6416f` (Ancient inline reading/text clipping).
Actual Firebird findings were corrected in `91781b7` (whole Ancient room names,
pointer reading focus and the finished fake-merchant reader), `0730159` (current
map-tool binding label), and `0dd5a28` (source portrait/navigation order).
The authorized remote push was blocked by automatic approval review; local
commits and frozen source archives are preserved, and are not described as
remote receipts.

Native character-departure, FTUE, five-character, map-window and corrected
run-menu contracts passed against their frozen source stages. Converter and
retile receipts passed with unchanged source fingerprints. Native checks
establish functional contracts, not calculator frame rate or ordinary startup.

The final CI maintenance is based on M
`c316608a59845832de0044d2e942177e55108fd1` and the matching Clang 18 core library
with SHA-256
`eb414307d04a7c9c498f5f27ba5e162caa889076da4e256bed6ffda6b136bc06`.
Only tests change in `c083b79`, `fdab3db`, `e16933b` and `5981dd3`.
Every registered world suite below ran completely; the focused reader test
does not substitute for the complete event suite.

| Full suite or registered focused gate | Checks | Passing receipt |
| --- | ---: | --- |
| Map drawing | 2,093 | `ui12-world-ci-baseline-run-map_drawing_ui-08659e940c0e` |
| Map travel | 154,901 | `ui12-world-ci-baseline-run-world_map_travel_ui-56e1b7acd7fd` |
| Map window | 82,520 | `ui12-world-ci-baseline-run-world_map_window_ui-d1b4445f4809` |
| Room lifecycle | 282 | `ui12-world-ci-baseline-run-world_room_lifecycle-80060fe6a018` |
| Map UI | 98,037 | `ui12-world-ci-run-world_map_ui-8a19a63a4823` |
| Map act intro UI | 47,282 | `ui12-world-ci2-run-world_map_act_intro_ui-d692b9ddd4fe` |
| Room UI | 8,809 | `ui12-world-ci2-run-world_room_ui-e28e5bc5b32d` |
| Event UI | 5,161 | `ui12-world-ci4-run-world_event_ui-66b38d109bf3` |
| Inline reading focus | 4,391 | `ui12-world-ci4-run-reading-3204d5d4e3a9` |

All these receipts passed with `source_changed=false`. Canonical-save and RNG
assertions remain intact. The changes update the independent expected LCD
geometry, explicit legend open/close behavior and real character-departure
prelude while retaining exact map-intro uint32-wrap checks. The synthetic
Defect plane colors match all five lit samples and the first dark sample;
the later difference is the intended heal tone. That fixture now advances the
real four-second effect lifetime before comparing the same four exact settled
colors, without disabling the effect. Event wheel/background checks move the
actual pointer off options so enlarged paged keyword tips do not intentionally
consume the wheel or cover the sampled artwork. Earlier failing receipts are
preserved as fixture diagnostics and are not counted as passes.

The J focused native reader gate passed **33 checks** with actual Simplified
Chinese resources. It compares the first pointer choice after entering reading
focus against the complete canonical save produced by `event_choice`, and
checks that a finished fake-merchant page regains inline reading. The receipt
is `ui12-j-world-reading-zhs-c81cf439c1ad`, checkpoint
`1074b720cf281629b52003794739e5c71d1daa27`, with unchanged source fingerprints.

### Ordinary Firebird execution

An independent copy of the accepted UI11 N working NAND was started normally,
Ndless was launched through the OS, and matching resources/programs were sent
through ordinary USB. Only superseded resource files were removed before the
large common-file transfer; all save and profile files were retained. The
common resource was 61,501,372 bytes; final H English and Simplified Chinese
sidecars were 1,133,049 and 5,113,330 bytes. No emulator RAM or game-state patch
was used. The original developer console was operated with the physical Flag
key and ordinary character/Tab/Enter inputs when selecting focused room cases.

| Checkpoint and scope | Completed observation |
| --- | --- |
| G `607d14e`, preview only | The sixth Random portrait and original background render. Necrobinder skull/top flame and rear Osty flame are present; two captures 500 ms apart differ by 20,613 LCD pixels. No run was started with G. |
| H `71f8f33`, ordinary new run | Random reveal, the selected Ironclad's original mask departure, room arrival, Neow choice and Proceed all execute. Neow background meets the HUD; the entrance banner remains above the options. |
| H, map controls | The lower-right legend opens with aligned compact rows. The drawing tooltip is above and to the right of its buttons. Physical Var focuses that toolbar. |
| H, bilingual persistence | Normal Save and Quit, title Settings language change and Continue restore the same run in Simplified Chinese. The locale change is performed through the player UI. |
| H, event reader | Physical console typing and Tab completion enter `COLORFUL_PHILOSOPHERS`. I/Down scroll the actual Chinese prose; dragging the source scrollbar thumb returns it to the top. All three options remain inside their textures. |
| J `1074b72`, actual Smith | Continue restores the saved rest room. Selecting Smith, choosing Strike, reviewing 6/9 damage and confirming commits its upgrade. The enlarged green-name card flashes, holds, shrinks and flies to the upper-right deck. The room then exposes Proceed. |
| J, repaired Ancient interaction | The settled upper-right title and epithet are whole. After entering reading focus, the first valid pointer click on Neow's Torment obtains it and its curse: deck count changes from 11 to 12 and the options become Proceed. |
| L `ab0ce99`, source portrait order | The ordinary character screen shows Ironclad, Silent, Regent, Necrobinder, Defect, Random from left to right. Moving the real pointer onto the sixth portrait and clicking once selects Random, changes the description to its Chinese source text and displays the original random background. |

The H event and rest room were entered through real console commands, not an
ordinary graph path. These cases establish actual room interaction, rendering
and execution; they are not a claim of naturally traversing an entire act.
The Smith action itself was performed through the ordinary player UI after
save/Continue, rather than invoking a direct upgrade/debug command.

Both world sessions ended through normal Save and Quit and the title's Quit
confirmation, then returned their performance/save files through USB and
flushed the NAND. Their logged commands completed with `source_changed=false`:

- `ui12-world-firebird-g-preview-e86288c955eb`: passed, preview scope only.
- `ui12-world-firebird-h-fea8df64c13f`: passed, with the two observed Ancient
  defects explicitly retained as findings until the J repair.
- `ui12-world-firebird-j-1f01014af398`: passed, real Smith and repaired Ancient
  interaction.
- `ui12-world-firebird-l-preview-95adc07760f1`: passed, source portrait order and
  pointer Random selection; normal title Quit, USB perf retrieval and NAND flush.

The matching L native character suite passed **241 checks**, including an
independent hard-coded source-order oracle, all six pointer actions, complete
forward/reverse keyboard rings and unchanged seed/run-number checks while
selecting. The earlier attempt linked to a stale J renderer was retained as an
invalid-source-linkage failure and was not counted. The passing receipt is
`ui12-l-matching-character-run-39d2d3f430a8`.

H captured 171 map frames averaging 66.31 ms rendering and 1,082 room frames
averaging 41.52 ms. J captured 758 room frames averaging 35.98 ms rendering,
including rest/Smith and Ancient cases. Both recorded zero resource-read
failures. H's English startup was 331.12 ms plus 495.33 ms first rendering;
J's Chinese startup was 620.91 ms plus 1,154.85 ms first rendering. These are
foreground Firebird timer measurements of mixed cold/warm interactions,
including console and transitions. They are not physical-device performance
claims or a controlled H-versus-J speedup comparison.

The earlier F program was held before launch when actual UI40 loading exposed
two remaining version-39 boundaries. G corrected those boundaries; H restored
the departure update hook lost in an integration merge. Neither a successful
resource converter nor a native build was counted as ordinary calculator
startup. The final source-order change received the separate L preview check;
it does not invalidate the unchanged room/Smith execution above. The L preview
included one incidental ordinary Silent start and its normal Save and Quit/
Abandon flow after a cold framebuffer capture lagged the selected page. That
capture timing was not reported as a program or transition failure.
