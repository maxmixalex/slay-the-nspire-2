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
The private rebake receipt records each original and new canvas, all 232 clip
registrations, frame totals, final sizes and preservation comparisons. A source
checkpoint alone does not establish the final image or calculator frame rate;
the full private conversion and Firebird review remain required integration
gates.
