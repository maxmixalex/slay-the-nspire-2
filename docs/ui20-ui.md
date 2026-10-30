# UI20 frontend and run-lifecycle corrections

Reference remains StS II v0.111.0 / 41cef1ea. This branch continues UI19
`d3063d8`; implementation below awaits the recorded frozen build and actual
resource acceptance before it is treated as delivered.

## Implemented changes

- The complete version label is right-aligned against the bottom-right margin.
  Continue information follows the current localized label's right edge with
  an eight-pixel gap, rather than using the old fixed 171-pixel anchor.
- Ancient backgrounds fill the complete 320x240 viewport before the normal
  top bar is composited. The former 17-pixel offset squeezed and lowered every
  Ancient scene, including its retained selection/terminal underlays. All eight
  source rows now use the original 1680x1260 viewport and 320x240 output;
  `rebake_ancient_ui20.py` produces replacements for their existing image IDs.
  The regular world builder also honors that viewport, preventing a later
  full rebuild from restoring the incorrectly cropped room-sized pictures.
- Standalone map rooms have a true-black backing. The original map layers keep
  their authored transparency instead of receiving a synthetic paper-colored
  rectangle during second/third-act entry and camera movement.
- The game-over ribbon label moves three pixels up. The summary uses a
  152-pixel centered score column and 184-pixel quote column; amounts share the
  label's baseline. Summary/terminal return uses the existing 800 ms fade out,
  enters the menu at black, and completes its normal 800 ms fade in.
- Successful console commands no longer detach an already resumed/saved run
  from its checkpoint. A separately remembered association identifies the
  actual saved run even after an act command changes its region parameters.
  A genuinely new run and a different externally replaced checkpoint remain
  protected. Terminal sav/bak/tmp ownership is no longer confused with whether
  the file can be continued, and Save on an ended run cannot recreate it.

The checkpoint identity adds one bounded `SavedRunIdentity` to App. It is
presentation/host lifecycle state, not a save-format revision or a second State.
No original assets, private disassembly, or device images are in this source
change.

## Original scene checks and regenerated art

The owner's pinned PCK confirms `scenes/rooms/map_room.tscn` has a full-anchor
`Background` ColorRect with `(0,0,0,1)`, and
`scenes/events/ancient_event_layout.tscn` has a full-anchor `AncientBgContainer`
without a top-bar inset. `scenes/ui/common_banner.tscn` places its label at
top 21 and bottom -56 inside a 162-high banner: the label's vertical center is
39.2% down the banner, above the image's midpoint. The score child scene uses
a 520x48 HBox, a 36x42 icon, equal 26-point fonts and centered vertical
alignment for both label and amount. The calculator's retained readable
10-pixel labels now share that baseline in the narrower column; its font size
is an explicit 320x240 readability adaptation, not a literal source scaling.

All eight Ancient replacements completed through the original scene/Spine
renderer at 320x240 with a 1680x1260 logical viewport. The intermediate lossless
image payloads total 690,277 bytes; their final shared-package cost is measured
by the integrated resource build. Auxiliary/image IDs remain unchanged.
The `ui20-ancient-full-viewport-v44-49227f6ad4f0` journal passed; Neow, Darv and
Vakuu full-frame previews were inspected. This fixes the complete viewport,
while the previously disclosed static Ancient scene limitation remains.

## Acceptance scope

The lifecycle regression now covers visible Continue followed by gold/card,
room and act console edits, then Pause / Give Up / Confirm; a different disk
run; a genuinely new same-parameter console run; already-terminal owned files;
and Save after terminal retirement. Transition checks inspect the 800 ms black
boundary, both halves of the menu fade, and repeated-input suppression while
preserving canonical run bytes. Existing profile, map, event and game-over
regressions remain required. Actual-resource captures and ordinary Firebird
acceptance are recorded by the integrated release, not inferred from compilation.

`tests/ui20_device_fixtures.cpp` exports ten matching production-codec pairs
and passed 205 encode/decode/re-encode checks against the integrated native
core. The Neow pair is the untouched opening room of a real new A3 run and
has complete progress. Ordinary Continue / Pause / Give Up / Yes produces its
three score lines without fabricated history. The other seven Ancient rooms
and act-two/three maps are controlled constructor fixtures and deliberately
mark their unobserved prefix partial. The exporter does not open or patch a
calculator image; its README describes ordinary menu input for device capture.
