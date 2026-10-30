# UI20 frontend and run-lifecycle corrections

Reference remains StS II v0.111.0 / 41cef1ea. This branch continues UI19
`d3063d8`. The source and regenerated art below passed actual-resource native
acceptance in English and Simplified Chinese. Integrated ARM, ordinary
Firebird, and final storage-format/package acceptance remain recorded by the
release owner rather than inferred from these native results.

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
- The game-over ribbon label moves three pixels up. The summary measures its
  localized score labels into a centered 112..152-pixel column and uses a
  184-pixel quote column. Amounts share the label baseline and explicitly align
  to the column's right edge; the former single-line flag was not a right-align
  operation and left unused space after every short amount. Banner/score text
  now uses the original common ivory color. Summary/terminal return uses the
  existing 800 ms fade out, enters the menu at black, and completes its normal
  800 ms fade in.
- Successful console commands no longer detach an already resumed/saved run
  from its checkpoint. A separately remembered association identifies the
  actual saved run even after an act command changes its region parameters.
  A genuinely new run and a different externally replaced checkpoint remain
  protected. Terminal sav/bak/tmp ownership is no longer confused with whether
  the file can be continued, and Save on an ended run cannot recreate it.
- Abandoning the standalone act map before selecting its first node now
  remains a valid ended State. That screen legitimately has no current map
  point; the old validator accepted it only while its phase was Map and
  rejected the forced Dead state. It now accepts that terminal phase under
  the existing finished/HP/empty-room constraints without inventing a visit.

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

## Recorded actual-resource results

The complete logical-v44 resource packs include all eight regenerated Ancient
images and the companion animation changes. The following runs use the final
UI source corresponding to integrated runtime `175ca7a`; English binaries
link its changed app/run objects with the otherwise unchanged native core,
and Chinese binaries link that complete rebuilt core directly. All eight
`run_logged.py` journals report `passed` and `source_changed=false`.

| Actual-resource regression | English checks | Chinese checks |
| --- | ---: | ---: |
| Frontend and save lifecycle | 70,562 | 70,589 |
| Game-over actors, summary, transitions, save invariance | 627,422 | 628,672 |
| Merchant, eight Ancients, dialogue and ordinary events | 6,637 | 6,614 |
| Map entry, pause, interruption, load and next act | 40,881 | 40,903 |

Journal IDs are `ui20-final-eng-frontend-7c9586f1e13c`,
`ui20-final-eng-gameover-94259b11a104`,
`ui20-final-eng-event-ffed1b596e2c`,
`ui20-final-eng-map-354b0fc893ee`,
`ui20-final-zhs-frontend-79d7a9c5e52d`,
`ui20-final-zhs-gameover-5b5834c7e9ce`,
`ui20-final-zhs-event-797d26e454b2`, and
`ui20-final-zhs-map-352dbe57e0ab`.

Visual inspection covered both languages' settled Ironclad summaries and Neow
rooms, the English Vakuu room, title/Continue placement, ordinary room-backed
map overlay, and both halves of the title fade. The production fixture adds
the standalone act-two/three map cases for ordinary-device acceptance; the
room-backed map intentionally retains its underlying room art.

The release owner independently confirmed ordinary Firebird Continue / Pause /
Give Up / Yes reaches defeat and summary. After leaving to the operating
system, the state directory retained performance, progress, profile and
language files, with no run save, backup save or temporary save. This is a
filesystem observation through the normal menu flow, in addition to the
native ownership regressions. The original partial merchant fixture omits
scores by design; the new complete Neow pair is the score-layout device case.

## Storage and disclosed differences

The independent review of the companion TGR3 decoder found no remaining
boundary or cancellation defect. Its maximum 1,360-tile group retains the
existing 770-byte expanded tile stride; the 224-pixel-per-tile sparse limit
bounds the compact representation and scratch requirements below the existing
allocations. Forward/reverse restore order and one 512-byte color buffer avoid
overwriting unread compact pixels. Existing regression coverage includes the
maximum group, mixed occupancy, canaries, malformed lengths/populations,
incremental restore and cancellation/reopen. No duplicate codec or extra
runtime workspace was introduced by this UI work.

The Chinese locale audit found 479 unique compressed font groups (3,457,236
bytes including headers), no duplicate compressed groups, and no small
10-pixel-or-less glyph in a group larger than 4 KiB. The remaining 669 literal
glyph records total only 18,265 bytes. Existing small-font cohorts and shared
descriptors are already active; undoing them to seek larger compression groups
would reintroduce cold-glyph work. The storage branch owns final fixed-group
compression tuning and final package byte counts.

This round closes the Ancient viewport discrepancy and the lifecycle/layout
issues above. It does not remove the previously disclosed static Ancient
scene adaptation, 320x240/RGB565 rendering limits, bounded history, or platform
text-input limits. Audio, multiplayer and Timeline remain outside the agreed
product scope. Previously completed UI14/UI15 follow-ups are not counted again
as new implementations.
