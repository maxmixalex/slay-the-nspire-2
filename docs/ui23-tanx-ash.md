# Tanx ash and scrolling smoke

This source-derived extension implements the two CPU ash emitters and two
scrolling-smoke quads in the original `tanx.tscn`, after the painted room and
before dialogue and HUD. It does not implement the separate water material on
the room's TextureRect. The original release is v0.111.0 (`41cef1ea`).

## Original data and catalog

The producer pins the complete original scene and scrolling shader by SHA-256,
then validates every decoded RGBA byte of these textures before conversion:

| Texture | Original size | Runtime representation |
| --- | --- | --- |
| short_grain_rice_particle | 32 × 32 | Exact white RGB565 and alpha8 |
| short_rice_no_glow_particle | 8 × 16 | Exact white RGB565 and alpha8 |
| tanx_smoke | 200 × 800 | Four exact 200 × 200 pages |
| lost_smoke_gradient | 10 × 200 | Exact 200 R8 rows; identical columns are verified |

The smoke shader samples `tanx_smoke`, not the pixels of `fog_sheet_1`.
The fog sheet's 2472 × 3316 dimensions still determine the authored geometry.
No texture is resized, color-quantized, or substituted.

The extension requires the complete 7214-auxiliary / 59667-image Ancient
catalog and appends seven images. Auxiliary 7214 refers to parameter image
59673. Its six explicit mask references are images 59667–59672. The resulting
catalog has exactly 7215 auxiliaries and 59674 images. Strict producer and
release validators check cardinality, references, dimensions, formats, white
mask channels, lookup values, padding and retirement liveness. The runtime
recognizes this endpoint while retaining the earlier catalog endpoints.

The parameter image contains the typed header, 200 gradient values, and a
65,536-byte lookup. Every lookup entry is the exact nearest integer of
`255 * smoothstep(0, 1, (a/255) * (b/255))` for integer alpha8 values. This is
an exact integer lookup; it is not a claim that the entire floating-point GPU
shader and fixed-point runtime are bit-identical. Source bilinear texture,
gradient and lookup interpolation is separately checked at page boundaries,
texture wraps and clock wraps, with a measured error of at most 1/255 alpha.

## Time, particles and reduced profile

The two CPU emitters keep their authored count of 20 particles each, 15-second
lifetime and 15-second entry preprocessing. The runtime reuses 40 slots in the
existing 60-slot cosmetic particle state. It preserves the source rectangle,
gravity, radial/orbit acceleration ranges, scale ranges, angle range and color
curves. Its 30 Hz fixed-point integration is compared with the source equations;
the reconstruction's bounded test measured at most 0.597613 LCD pixels of
position error over a lifetime. This is a numerical/cadence boundary, not an
exact Godot CPU simulation claim.

The original process's random particle seed is unavailable. A deterministic
cosmetic seed uses run, floor and room-entry identity, without advancing the
saved gameplay RNG. The original shader uses Godot `TIME`, independently of
particle age. Runtime smoke therefore uses the continuous existing visual clock
and the source scroll rate's exact 10-second texture period. It is not restarted
for each particle or dialogue refresh. The original process-wide time epoch is
unknown and is not presented as recovered.

The reduced-animation profile holds the preprocessed source-entry particle pose
and a fixed smoke phase. Room redraws and dialogue/HUD updates do not advance
that cosmetic pose. The profile marker is preserved through append, pruning,
glyph packing and publication.

## Measured resource cost and remaining acceptance

On both restored reference profiles, this source-bound append added **30,796
bytes**, including new directory/auxiliary entries. Five existing retained
groups contain 30,674 stored bytes; the largest group decodes to 123,456 bytes.
All seven views decode to 549,504 bytes and occupy **139 existing 4 KiB image
cache pages**. The parameter image alone decodes to **66,048 bytes / 17 pages**.
These are measured occupancy and cold-decode costs, not zero-cost claims.
Drawing acquires the four smoke pages and parameter view before using their
borrowed pointers and issues no additional image requests during smoke drawing.
No framebuffer, image-cache capacity or particle pool is enlarged.

The source tests cover all 65,536 lookup pairs, malformed schema, six explicit
mask roots, interpolation/page/wrap behavior, preprocessing, rewind and reduced
pose stability. Actual source-bound full and reduced reference appends pass
strict final content guards. These bounded development checks do not by
themselves establish the final calculator allocation, guest frame cost, final
fresh-converter output budget, or physical-device behavior. Those gates must be
measured on the integrated frozen source and actual final conversion outputs.
