# Original map drawing controls — UI8, 2026-10-24

The map now has operable Pencil, Eraser and Clear controls. Notes are attached
to paper coordinates, follow map scrolling, and survive closing/reopening the
map and continuing the same run. This implements the previously missing map
drawing interaction rather than adding a comparison-only probe.

The reference is the owner's locked v0.111.0 / `41cef1ea` source:
`NMapDrawings`, `NMapDrawingInput`, `NMouseHeldMapDrawingInput`,
`NMouseModeMapDrawingInput`, `NControllerMapDrawingInput`, `NMapDrawButton`,
`NMapEraseButton`, `NMapClearButton`, `NMapPoint`, `NMapBg`, `NMapScreen`,
`map_screen.tscn`, and the original drawing/erasing shaders. Original source
and derived artwork stay outside the repository.

## Interaction and original artwork

- Selecting Pencil or Eraser changes the persistent left-button mode; selecting
  it again restores None. Clear removes the current map's notes.
- Right and middle buttons cancel a selected mode. With no selected mode,
  holding right draws and holding middle erases, matching the original
  temporary mouse controls.
- The map's existing view-exhaust/tab-right action focuses the three tools.
  Left/right selects a tool. Confirm selects its mode; holding Confirm while
  moving the drawing cursor with direction keys makes a stroke. Key release,
  Escape, screen changes and mode changes terminate the active stroke.
- Drawing mode prevents map-node travel and hover inspection while preserving
  the graph's original travelability colors. Map scrolling still changes the
  camera, not the stored note coordinates.
- Each character uses the original line color. Erasing subtracts coverage over
  the visited region; it does not delete an entire stroke merely because one
  segment intersects the eraser. Later pencil strokes can draw over erased
  regions.

The toolbar uses the original quill/eraser/clear images and glows, the four
original cursor images, and the two original trail textures. Aux IDs
1972–1983 are reserved for those 12 images. Tool and cursor images retain their
square aspect ratio. Trail textures retain their native 64×64 alpha field.
The cursor assets are imported Godot `.image` resources; the converter uses
the existing GDIM reader instead of assuming `.ctex`.

The source toolbar's bottom-left position, individual button widths and
selected/hover tint are projected through the current map layout. Tooltips use
existing localized original keys. The source draw/erase widths are 4/12 in a
half-size viewport, or 8/24 paper pixels before screen projection. Both the
paper origin and projection scale are explicit inputs to drawing transforms;
the existing map's scrolling changes only the origin. This does not add an
independent user zoom control.

## Bounded storage and rendering

`MapDrawingDocument` holds at most 2,048 four-byte points. Its coordinate
fields encode source x/y plus stroke-start and erase flags. Coordinates cover
the original 1920×3240 paper. The document is bound to run seed, progress-run
serial and act; entering a different map clears it. At capacity the port
preserves existing notes and shows a localized notice instead of overwriting
earlier strokes. This finite capacity is an explicit calculator adaptation.

The rasterizer uses a shared visible-map alpha cache with row bounds, one
scratch row and path distances; the whole structure is statically bounded
below 80 KiB. Segments in one stroke contribute their maximum coverage before
the stroke blends into the note layer. That avoids darker joints from blending
every segment separately. Strokes blend in chronological source-over or
subtractive order, with round caps and sampled original trail alpha.

The cache key includes the App owner, note revision, camera origin and point
count. Unchanged frames composite only nonempty row spans. They do not replay
the entire point list or reread the original trail textures. Rebuilding after
drawing or camera movement has bounded work; target-device costs still require
actual runtime measurement. Integer software rasterization is not claimed to
be pixel-identical to Godot's GPU line renderer.

## Save compatibility

Map notes are UI progress, outside the gameplay `State`. The existing State
save format and RNG are unchanged. `UiProgress` writes an optional `NSH4`
tail only when notes exist. Ordinary records retain `NSH2`; records with mode
metadata and no notes retain `NSH3`. `NSH1`–`NSH3` remain readable and start
with empty notes. The new reader validates point count, coordinate bounds,
stroke heads and drawing modes before accepting the tail.

Changes mark UI progress dirty; normal saves and map closing persist them.
Pointer motion does not cause a filesystem write for every sampled point.
Starting a new run or entering a different act cannot attach old notes to the
new paper. The three input hosts export key-release and auxiliary-button edges
needed to stop strokes correctly.

## Executed validation and remaining acceptance

At frozen checkpoint `dc4c07c0da37e7ade17ee042868b4e6e42883303`, the native
`map_drawing_ui_regression` completed **2,093 checks** with no failures. It
tests independent coordinate transforms, camera movement, a second projection
scale, finite alpha, clipping, partial erasure and redrawing; malformed/truncated
NSH4 and legacy progress; actual App pointer/keyboard/release/Escape handlers;
map close/reopen, clear, capacity and run-identity changes. State save bytes are
compared before and after drawing to verify that the UI action does not alter
gameplay state.

The same binary accepts a real matching pack and capture directory for Pencil,
erased and cleared frames. Final English/Chinese original-art captures and
ordinary Firebird interaction with the integrated UI8 launcher remain separate
acceptance gates. A synthetic handler/raster pass is not presented as completed
original-art or calculator validation.
