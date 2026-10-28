# UI15 room presentation and retained-resource work

Reference: the owner's complete v0.111.0 / `41cef1ea` assembly and PCK.
Original assembly, texture pixels, fonts, generated packs and calculator
evidence remain private. The public converter records paths, geometry and
timing parameters.

## Implemented source changes

### Ancient options and room focus

Ancient events use a separate dialogue/option renderer. Their label-only
options now measure one label and reserve a24px control, with additional
height only when that label itself wraps. Options with a secondary description
retain their35px height. The complete stack is measured before its bottom
anchor is computed, so the finished Proceed remains at the same234px bottom
without an empty description line. The icon follows the measured row center.
The compact control keeps a14px inner text viewport. Its previous12px viewport
created one pixel of false overflow and made even a single Continue label move
after the initial pause. The actual-resource regression compares its settled
hover before and after that pause.

The closed treasure chest now uses the actual pointer rectangle when the
pointer is visible. A retained keyboard cursor cannot activate its outline
from elsewhere in the room. Modal input ownership also applies to the chest.

History-tip bounds now describe the visible frame. Leaving a map floor,
closing/reopening its map, changing to another history element, or suppressing
the reader clears its previous hit rectangle. A visible reader retains its
floor while the pointer moves into it to scroll. Underlying map points cannot
replace that owner. The source eligibility remains a traveled, non-current
point with an actual history record and pointer navigation.

### Map icon registration

The six base normal-node icons previously used only `AtlasTexture.region`.
Their separately loaded outlines included `AtlasTexture.margin`, so scaling
both to the same output control did not align their silhouettes. The rest and
elite shapes exposed this especially clearly.

Both layers now retain the original128×128 logical canvas. The base icons use
32×32 stored canvases, matching the existing outline resolution. Ordinary
hover and historical inspection use the source's scale behavior. The white
outline applies to travelable points; a historical node keeps its map-paper
outline color. The original act colors and map recoloring shader remain in
effect.

### Merchant layout

The five upper cards keep their60px column pitch and now draw at54×76.
Focused cards draw at66×93 and retain their source focus tween and final draw
order. The lower colorless centers use the original rug's384/658 horizontal
and676 vertical source coordinates: LCD centers58/119 at y168. Prices remain
children of the same scaled slot. The separate fixed Back control therefore
has room beneath the cards and beside their prices. Card hit rectangles
include the cost row without overlapping the next row.

The original rug opening/fade clocks remain active during resource preparation.
No additional loading graphic is introduced.

Both new settled card sizes now use the existing84×108 face-cache tiles.
Their earlier unlisted dimensions bypassed that cache and remeasured every
card on each frame. The same pixel oracle covers normal/focused cards at all
seven actual slot positions, including the two lifted lower-row cards.

Price labels first resolve their resident glyph metrics. Glyph payloads are
loaded only when their ink or1px shadow can reach the viewport. Prices above
the screen during the rug tween therefore do not force unrelated cold font
groups into memory. The resource regression compares independently cropped
visible prices at every screen edge and checks a cold range reader for zero
callbacks from fully hidden prices.

### Rest button proportions

`rest_site_button.tscn` fits the256×169 icon into250×153 and the256×173 outline
into248×173. The packed icon and outline additionally have transparent
padding. The renderer now carries those complete fitted extents in Q8 until
the final parent/outline scale has been applied.

The former early integer rounding changed the resting outline from the
source-projected43px width to42px. At full focus, the source-projected icon is
49×35 and its outline52×37; the old projection gave48×34 and52×36. Both layers
now share the original125px horizontal pivot, projected to24px in the holder.
The separate outline0.9→1 and parent1→1.1 focus tweens are preserved.

### Loot ribbon

The original1499×270 ribbon is fitted into a652×162 control. Its244px resource
canvas is now61px high, retaining the same244×44 useful texture pixels and
the correct transparent vertical padding. Drawing that canvas at180×45
preserves the authored visible ribbon proportions.

`HeaderLabel` occupies y=-9..130 within that control. Its original Kreon
baseline and substituted-font baseline are projected separately. This places
the title on the front band of the ribbon and retains the source's elevated
label position relative to its long lower tails.

## Complete Smith presentation

`SmithRestSiteOption.OnSelect` commits upgrades without ordinary upgrade
preview, then `DoLocalPostSelectVfx` creates `NCardSmithVfx` for the selection.
The room preserves the original local post-selection wait of2s, or1s in fast
mode, beside the500ms choice fade.

The Smith card preview now follows its separate source sequence:

| Time | Presentation |
| --- | --- |
| 0–250ms | Cubic-out scale from zero to the readable card face |
| 250ms | First impact, rotation toward+20°, weak short directional shake |
| 500ms | Second impact, rotation toward−10°, weak short directional shake |
| 750ms | Third impact, rotation toward+5°, weak short directional shake |
| 1250ms | Card flight to its real deck destination |
| Through1350ms | Remaining third-impact particle lifetime |

Each impact uses the source hammer texture, all nine cells of the3×3 impact
flipbook, and20 sparks. The former UI14 impact asset contained all nine cells
in one40px image. UI15 adds nine120px cells, sufficient for the approximately
118px maximum normal preview projection, and reuses the existing hammer and
spark images. No source cell is omitted.

Particle shape, alpha and scale curves come from `vfx_card_smith.tscn`.
The emitted spark lifetime is600ms with44% variation, velocity422–1000 source
pixels per second, downward gravity1500 source pixels per second squared and
a50ms trail. Presentation-only hashes supply repeatable particle variation.
The renderer retains the source's single particle-root set for multi-card
Smith while rotating each selected card. Generic upgrades keep their distinct
`NCardUpgradeVfx` presentation.

## Retained group allocation

Quarter allocation adds no picture pool or tile-group metadata. The current
12.75MiB shared arena uses6.75MiB image/6MiB tile tiers in ordinary scenes and
11.75MiB image/1MiB tile tiers in merchant scenes; the256KiB reduction funds
the exact Random-water lookup. The image-key capacity change below adds
2,048bytes of ARM metadata. Retained groups of up to96KiB now use
one, two or three contiguous32KiB quarters within a128KiB tile page. This
allows32KiB glyph groups and64KiB merchant animation groups to share parent
pages. Larger tile/index groups keep their original page allocation.

Quarter location/count use the previously unused retained-group `items` word
and bit6 of its first-page byte. The occupied parent bitmap is still shared
with full-page allocations. Parent occupancy is derived from the existing
bounded64 records, so this does not add another bitmap or memory pool.

The allocator preserves oldest-unpinned LRU eviction, pinned frame indexes,
owner isolation, failed-decode release and counter-wrap ordering. A shared
parent page becomes free only after its final quarter allocation is released.
Noncontiguous quarter holes cannot satisfy a larger retained allocation.

## Verification status

### Follow-up: bounded work and ordinary-OS evidence

Inventory preparation is background work and no longer controls whether a
stock item can be selected or purchased. This matches `NMerchantInventory.Open`,
which enables its controls when the original rug tween starts. The renderer
continues to show the original moving rug, its items and the live room.

The same384-byte queue now holds192 16-bit image IDs instead of96 32-bit IDs.
It includes the unfocused merchant skin that appears as soon as the merchant
loses focus, its fixed plane, Back artwork and the first hand pose. Visible
stock and its exact fitted fonts precede the extra145-frame idle cycle.
The decoder scheduler checks SP804 before each bounded step, with a262tick
(approximately8ms) start budget for critical inventory and65ticks
(approximately2ms) for the extra idle cycle. A provider operation already in
progress may finish beyond that budget; whole-frame measurements retain it.
The offline deterministic limit remains four decoder steps. Each decoder step
has its independent512-byte output and4096-byte input bounds.

The integrated font visitor exposed an additional synchronous read: an energy
icon with negative coordinates could intersect both axes even though the hidden
Canvas had a zero-area viewport. `picture`, nine-slice and material paths now
reject that empty viewport before requesting image payloads. The actual full
English preparation gate retains its16KiB-per-four-step assertion and passes
31,373checks, including a purchase while background preparation remains active.
That gate uses the deterministic four-step clock; its3,093updates are not a
calculator timing claim.

The complete closed-room loops, the alternate skin and inventory exceed the
previous1,023image metadata slots. The cache now has1,151slots in the same
picture arena. The extra chain bit shares an unused bit of the encoded-format
byte, preserving64hash buckets and adding2,048bytes of ARM metadata. No pixel
pool grows. The separate sanitizer gate covers1,151colliding entries, all five
decoded formats, chain removal across the old1,023boundary, owner clearing,
recency wrap and the real English/Chinese image and glyph preload inventory.

Material icons now sample dense and indexed source textures directly instead
of rasterizing a temporary RGBA tile and painting it again. Sparse textures
retain their bounded16-row path. Both paths preserve the original color
arithmetic, alpha, clipping and modulation; the previous raster is retained as
a test-only whole-surface oracle. Main-menu mip generation similarly borrows
the existing foreground transfer bytes instead of placing50,400bytes on the
calculator stack; its independent pixel oracle passes2,919,376checks.

The fixed local perf5 source passes656,627Random original-texture checks and
19,395,167merchant checks across2,376complete frames, all five characters,
opening/closing/interrupted tweens, boundary clocks and unchanged gameplay
state. These are correctness results. Later ordinary-OS performance work is
still active: the perf4 closed room measured30FPS/27.874ms with no resource
reads, but its immediate open after7seconds contained a1,982.544ms frame and
744,992bytes of reads. The1151-slot cache and separate critical-work budget
must close that observed defect in a subsequent ordinary-OS measurement.

The source regressions now cover mixed32/64/96KiB retained allocations,
fragmented quarter holes, full512KiB groups, all pinned-index sizes, owner
clear, failed publication, image-arena boundary canaries and recency wrap.
Room regressions cover the true Smith picker/commit path, its scheduled
shakes, launched card flight, the label-only ancient Proceed, stale history
bounds and source texture hover behavior.

The complete private room-art converter has executed successfully: nine
Smith impact frames, six margin-preserving map icons and the corrected reward
banner were generated from the pinned PCK. The nine-cell contact sheet has
been inspected as a continuous ring-breakup sequence.

The source was frozen as local commit
`0b00d2a45932b3b17fdd20317df13553b4a1a526` after the connector's remote writes
were interrupted. That commit is explicitly **not published or remotely
verified**. Its immutable source archive and local validation receipts retain
the distinction; no GitHub CI result is implied.

Both32KiB and64KiB band/frequency glyph grouping were executed on the original
Chinese pack. All53,631 glyph records retain their exact payloads and metrics,
all strings remain byte-identical and shared artwork is unchanged. The32KiB
result saves83,744bytes;64KiB saves127,752bytes. Against the same frozen UI14
renderer, both produce identical full-frame transcripts in all seven room
scenes. The32KiB grouping is selected for its lower cold-read/decode volume:

| UI14 renderer, Chinese merchant window | Original font groups | 32KiB grouping | 64KiB grouping |
| --- | --- | --- | --- |
| Initial room and first opening frame | 12reads /206,415bytes | 3reads /52,652bytes | 2reads /69,766bytes |
| First120animation frames | 73reads /1,248,145bytes | 21reads /386,179bytes | 13reads /475,715bytes |

The new local runtime was also linked to the unchanged room measurement
driver and exercised with the original and32KiB-grouped packs. Those two
resource variants again produce identical full-frame transcripts across all
seven scenes, with canonical gameplay state unchanged. The first merchant
window uses51reads /348,627bytes with the grouped pack; its closed-room frame
accounts for45reads /320,472bytes, leaving6reads /28,155bytes for the immediate
opening frame. Later opening-animation frames still have their own cold
resource work. These are native resource-access observations, not calculator
timing measurements.

The final complete resource package, dedicated actual-art room regressions
and ordinary-Ndless Firebird cold-opening measurements remain required. The
historical UI14 first-rug stall of3771.576ms is the comparison baseline, not a
UI15 result. Reduced reads alone do not complete the cold-opening task.
