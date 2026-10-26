# UI13 independent enemy animation canvases

The earlier world conversion measures the union of every clip, skin, phase and
move before choosing one raster scale for the entire model. Vantom has 232
conditional clip entries (content directories 1383–1614). Its long tail/action
extents reduce the model-wide sampling density to approximately 0.0453 pixels
per source unit (`combat_sprite_units.inc`: 22598 Q10 source units per pixel).
The normal LCD projection is 4/21 pixels per source unit. The renderer therefore
enlarges the already small idle body, which loses the original source detail.

`tools/rebake_enemy_ui13.py` samples every requested original clip directly from
the private PCK and reviewed Spine runtime. It retains all source frames, skins,
conditional tracks, physics steps and timing. Each clip receives its own canvas,
sampled up to the LCD density and bounded by the existing 320×218 frame limit.
Integer pivots and inverse raster scales are carried in the existing clip
registration fields. Ordinary combat enemies and encyclopedia previews now use
those fields; old clips with zero units retain their previous registration.
World positions, hitboxes, intent and health anchors keep the source transforms.

The tool uses the existing sparse-tile codec and 454a6 animation precision. Its
replacement pass identifies every tile still used by a retained image, reuses
dead tile slots before extending the catalog, and recompresses only affected
groups. Each output compares all protected tiles, existing index groups,
unselected pictures, fonts and localized strings against its input. Frame IDs,
counts and clocks remain identical; only replacement image geometry/pixels and
explicit clip pivot/units fields may change. No runtime cache, full-pack scan or
new codec is introduced.

Source tests cover the independently bounded layout, a distant tail's effect on
sampling density, complete frame replacement, rejected clock changes, and two
locales passing the same retile plan without changes to their text and fonts.
The full private conversion at checkpoint `7bfc391` completed in 1,132.19s:
eight models (Vantom, Ceremonial Beast, Waterfall Giant, Mecha Knight, Frog
Knight, Thieving Hopper, Bygone Effigy and Architect), 286 clips and 5,340 frames.
Its English/Chinese outputs retain all clocks, 701,988 protected tiles, 546
pre-existing index groups, all original glyphs and all 8,704 strings exactly.
Their logical pack sizes are 70,174,824/74,662,627 bytes, adding approximately
6.258 MB of shared original-source pixels before subsequent tile alignment.

Actual output decoding gives Vantom's normal idle frame91×102 ink pixels,
previously21×25. Ceremonial Beast changes26×29→96×108, Waterfall Giant
28×28→96×97, and Mecha Knight37×28→126×95. Their source-world poses and pivots
are preserved. These observations come from the completed private packs,
not the earlier short sampling probe. The comparison contains private original
assets and remains outside the source repository.

`tools/align_enemy_tiles_ui13.py` is the lossless storage follow-up. It keeps
one tile-grid phase per model/density and only adds transparent margins, moving
each pivot by the same integer amount. It parses and moves sparse run headers
without recoloring, resampling, removing frames or changing timing. Every output
frame is independently compared in source-world coordinates; inverse density,
span pixels, original fonts and other resources stay exact. It retains the
existing codec and includes no additional runtime cache or resource scan.
The receipt records actual encoded sizes, per-clip image-cache pages and group
working sets. Its group-only LRU simulation is a catalog-cost estimate, not
elapsed-time or emulator evidence. Expanded transparent margins can change a
nearest-neighbor boundary pixel under non-integer screen scales, requiring real
screen review before selecting the optimized candidate. Full Firebird review
and hot-frame measurement remain required integration gates.

The full alignment trial at `333b5ec` closed successfully after250.311s.
Both languages prove all5,340 world-space frames identical, but each logical
pack grows725,442 bytes. Fewer unique tiles did not improve the compressed
groups' physical size; some working sets also grew. This candidate is rejected
and is not part of the selected artwork. The completed source rebake keeps its
unpadded canvases, preserving the original nearest-neighbor screen behavior.

The separate `tools/prune_tile_directory_ui13.py` pass removes only unreachable
frame descriptors and entire index groups with no remaining image reference.
It remaps live directory entries, copies every live compressed tile/index stream
verbatim, and preserves all dimensions, pivots, pixel samples and frame clocks.
It also compares every retained image, glyph and string. This avoids both a new
codec and the rejected padding change. Tests cover real replacement-created
dead records, decoded image equality, idempotence and rejection of a malformed
live reference. Its private receipt records the actual byte saving after the
source checkpoint is frozen and the complete pack is processed.

A native file-reader check of nine representative idle clips, including normal
and maximum-tail Vantom, observes zero source callbacks/bytes on their second
and third loops. Vantom's27-frame loops occupy81 image-cache pages (324 KiB),
Ceremonial Beast and Waterfall Giant128 pages (512 KiB), Mecha Knight131 pages
(524 KiB), Frog Knight128 pages, and Architect86 pages (344 KiB). Every rebaked
clip independently fits the ordinary5 MiB image pool; the largest sparse frame
is38,108 bytes. These are cache/working-set measurements, not an emulator or
physical-calculator frame-rate claim. Full combat with other active resources
still receives the final device acceptance gate.
