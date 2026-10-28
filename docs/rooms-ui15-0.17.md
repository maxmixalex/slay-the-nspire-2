# UI15 room presentation and retained-resource work

Reference: the owner's complete v0.111.0 / `41cef1ea` assembly and PCK.
Original assembly, texture pixels, fonts, generated packs and calculator
evidence remain private. The public converter records paths, geometry and
timing parameters.

## Final selection and measurement scope

The selected runtime is local `foreground-pointer-v2`,
`bd158d205d877f767f59fe2a1eb33874cb925899`, with the complete `strings15`
resources. The later scheduling, independent-idle and saved-card-template
experiments are **not included in the delivered runtime or resource pack**.
Their correctness results, screenshots, normal-exit files and unsuccessful
performance measurements are retained. No remote publication is implied by
these local checkpoints.

The selected ARM launcher is 1,755,032 bytes, SHA-256
`8596f9c1ed5778d229944adf0fbde3d9e2f9c22d770b7ce8fef11c6465666555`,
with 22,879,896 bytes of linked allocation. The final live rollback restored
exactly the three files that differ between the rejected v22 and this runtime:
`src/merchant_ui.inc`, `tests/merchant_preparation_regression.cpp`, and
`tools/isolate_card_art_ui15.py`. Input, Canvas and the other implemented
room changes were preserved.

All recent windows below run the production ARM executable through the
ordinary calculator OS in Firebird, with Ndless r2022, 132 MHz CPU / 66 MHz
AHB / 33 MHz APB, the unmodified 32768 Hz SP804 counter, and complete common
plus Chinese resources. They are emulator measurements, not physical-device
measurements. Normal-exit USB performance files cross-check the read-only
counter snapshots. No emulated RAM, timing, game state or clock is modified.

The fixed first-opening sequence waits 30 seconds on the title, enters the
saved Shop or travels from the completed event into the legal Shop, and waits
7 seconds after entry. It then supplies the same 100 ms touch contact,
100 ms movement, 180 ms press and 100 ms release, with an 11-second window.
These are first-opening measurements after a fixed preparation period;
they are not measurements of an immediate click on a completely cold room.
The map test uses a controlled production-route fixture: the completed Aroma
of Chaos on floor 7 has exactly one legal next Shop. It is not described as
natural multi-floor play, and its title cannot prewarm a saved Shop's stock.

| Runtime / complete resource edition | Actual path | Largest first-open frame | Assessment |
| --- | --- | --- | --- |
| v14 / inventory13 | Continue to saved Shop | 776.093 ms | Cold-frame defect remained |
| v15 / strings15 | Continue to saved Shop | 388.062 ms | Cold-frame defect reduced |
| v15 / strings15 | Controlled legal map travel into Shop | 450.928 ms | Separate map-entry baseline |
| v17 / strings15 | Recorded touch sequence | Not an opening measurement | Rejected: rug never opened |
| v19 pointer-v2 / strings15 | Continue to saved Shop, original touch gesture | 256.012 ms | Selected runtime; input passed, cold-frame limitation remains |
| v19 pointer-v2 / strings15 | Controlled legal map travel into Shop | 549.438 ms | Selected playable combination with a quantified remaining cold stall |
| v20 / strings15 | Continue to saved Shop | 278.534 ms | Rejected for performance |
| v20 ARM / idle21 resources | Continue to saved Shop | 374.329 ms | Rejected for performance |
| v22 / idle21 resources | Continue to saved Shop | 256.104 ms | Improved some measures, insufficient to accept the combination |
| v22 / idle21 resources | Controlled legal map travel into Shop | 808.289 ms | Rejected: substantial map-entry regression |

The selected-runtime map-entry record in `os-map-final19` is complete. Its
launcher, common resource, Chinese resource and matching fixture files all
passed byte-exact USB readback before launch. Actual screenshots show the
completed event, legal map entry, opened rug and normal OS exit. Read-only
SP804 counters agree with the normal-exit USB performance file.

### Selected runtime: first opening, warm windows and controls

The selected v19 first-opening window contains 129 frames over 10.887451 s:
11.848503 FPS, 132 reads / 919,686 bytes, 13 frames above 100 ms and one above
250 ms. Its 256.012 ms peak contains 8 reads / 39,909 bytes. Peak-frame and
whole-window I/O are different measures: v15's 24 reads / 113,637 bytes
belong to its peak frame, whereas its whole window reads 167 times for
1,082,490 bytes. The selected runtime reduces the larger cold stall but does
not eliminate every visible first-opening hitch.

The final map-entry window contains 108 frames over 10.932098 s: 9.879165 FPS,
255 reads / 1,409,390 bytes, 35 frames above 100 ms and three above 250 ms.
Its 549.438 ms peak consists of 540.344 ms render, 8.270 ms update and
294.037 ms resource time, with 57 reads / 254,550 bytes in that frame.
This is below rejected v22's 808.289 ms peak, but above the earlier v15 map
baseline of 450.928 ms. The final runtime keeps the complete short-input
repair and functional changes; it is accepted as the verified playable
combination with these measured limits, not as a fully smooth or stall-free
build. At arrival, 32 of 40 critical image entries were ready and card text
preparation had not yet begun. Actual opening still proceeded immediately;
no input delay or loading cover conceals the remaining work.

The final map route's normal-exit save is 7,693 bytes and is byte-identical to
the v22 map route's save, SHA-256
`f687fb52dc340d26a29a21c7a1c5121a4aa23c57e7958af42da43f31cd868f37`.
The progress files are also retained; they are not claimed to be byte-identical.

The independent settled closed-room window measures 30.007234 FPS /
29.068759 ms mean render. The warm open rug measures 14.031902 FPS /
70.267378 ms mean render. Both windows have zero resource reads and no frame
above 100 ms. A late opening after the same extended idle period peaks at
204.865 ms, with two reads / 9,780 bytes across the complete opening window.
These warm results are not substituted for first-opening results.

The original 100/100/180 ms gesture reaches pointer (234,150) and opens the
rug. The retained input FIFO fixes the earlier lost contact/movement/press
sequence without requiring a longer hold. Immediate Open, purchase and Back
remain available during preparation. The original rug movement and fade
run without an extra loading graphic, input gate or hidden delay.

The settled OS screenshot places the cursor away from the goods. Both lower
colorless cards retain their respective prices below their card slots, clear
of Back. That screenshot contains no speech bubble and therefore is not used
to claim a separate bubble-centering check. Source/layout tests cover the
speech text's centered alignment.

### Bounded preparation and complete resources

Live-room preparation prioritizes critical images, all seven card faces and
prices, then the six stock relic/potion hover sets, followed by the alternate
145-frame merchant idle. A title which already has a validated Shop save
prepares its canonical images and hover resources during existing idle
frames. Continue validates the save again and visits the actual live stock.
Neither path installs the preview into the active game or alters discoveries,
serialized game state or gameplay RNG.

Hover preparation uses the same detached model and rich-text font styles as
visible hover. Its cursors reuse existing padding in the 404-byte merchant
queue. The two original hover skins and inline star/player-energy images
are included. There is no new pixel pool. Source reads and decoder outputs
are bounded individually, but a cold string formatter can touch more than
one 4 KiB cohort: the 8 ms scheduling allowance is cooperative, not a hard
upper bound on a frame's elapsed time.

The selected runtime's native and ASan/UBSan gates pass with the actual
English and Chinese resources: merchant preparation has 25,515 / 27,506
checks; the related keyword and card-occlusion gates also pass. The latter
compare 2,376 complete rendered frames exactly for each tested locale.
Preparation checks cover queued images and stock/hover glyphs, unchanged
serialized game and RNG bytes, discoveries, pixels, hover pages and both
label-clock tables. These are correctness proofs, not an FPS estimate.

The complete `strings15` edition uses 74,024,394 bytes for common artwork,
1,130,938 bytes for English, and 5,000,436 bytes for Chinese. Chinese's
reconstructed logical NSP is 80,793,488 bytes. All 16 locales retain their
UTF-8 text, IDs, aliases, glyph payloads and metrics, image pixels and
animation streams byte-exactly. String regrouping adds 725,820 bytes across
all 16 locale files in exchange for at-most-4-KiB raw string cohorts;
unusually long individual texts remain literal. Independent card streams,
inventory-material isolation and small-font grouping remain in this edition.
The format uses the existing codec and workspaces.

| Selected resource | SHA-256 |
| --- | --- |
| Common | `24f81a1f67e89ccb6da39c4dd98031a16e951662c10083dda6a088bc6afe5d6d` |
| English | `06d1496be83af9c3b261abaab74054fafa384417715d8efd7adcee4371b320ca` |
| Chinese | `ba66ef22e4f9eed078d8dbc2bfcbfd5d49f3b9ef50955b0167e72ee2aa59d563` |

### Preserved rejected experiments

The v17 trace does not measure opening: its pointer remained at (160,120)
and its apparent 153 ms peak came from a closed room. Later scripted
navigation did not reach the OS, so its interrupted journal is not relabeled
as a normal-exit success. The failed record remains available.

The v20 scheduling experiment removed artificial yields for cached strings
and fonts, then repeatedly selected idle images near the current animation
phase. This scattered work among shared eight-frame decode groups. Only
7 Top frames were ready at arrival, versus 48 in v19; the complete opening
window used 143 reads / 976,626 bytes, with two frames above 250 ms. Native,
ASan/UBSan and ordinary OS exit all passed, but performance was rejected.

The separate `idle21` resource experiment isolated only the original
145 unfocused Top frames. It preserved all 16 editions, every pixel, clip
order and timing. Common artwork grew by 187,034 bytes; locale files grew
by 2,102 bytes in aggregate. Summed independent cold decode work for those
frames fell from 5,001,122 to 628,985 bytes. The other 57,915 image entries
and 1,655 animation tile/index streams were unchanged. Despite those byte
proofs, the unchanged v20 runtime with idle21 used 178 reads / 1,141,271 bytes
and peaked at 374.329 ms. It was rejected; less decoded data alone did not
prove lower OS latency.

The v22 source removed that repeated phase reordering and prepared immutable
saved-Shop card templates and their glyphs during title idle. No active-game
formatter or gameplay state was changed. This made all seven cards, prices
and hover sets ready at saved-Shop arrival. Its saved-Shop window used
110 reads / 762,561 bytes, 12.094533 FPS, and a 256.104 ms peak, but it had
20 frames above 100 ms. Its settled closed room reached 30.006958 FPS and
its warm rug 14.010475 FPS, both with zero reads and no frame above 100 ms.
Late opening peaked at 160.370 ms. The v19, v20, v21 and v22 saved-Shop
normal-exit saves were identical: 7,424 bytes, SHA-256
`c69c2769e8bac8618b39b41b3ff9e10219e58a19a5c8e4cab900d360e6766b69`.

However, the controlled map-entry window did not benefit from saved-Shop
title preparation. v22 arrived with critical images complete but zero of
seven card text passes complete. Its actual opening used 279 reads /
1,490,184 bytes and only 9.868134 FPS, with 31 frames above 100 ms and four
above 250 ms. The 808.289 ms peak contained 77 reads / 343,391 bytes. Actual
screenshots, source/resource identities and normal-exit USB counters all
confirm this result. The entire v22/idle21 combination was therefore rejected,
and its three-file source delta was restored to the selected v19 baseline.

The exact production checkpoints, failed journals and frame traces are kept
in the private validation records (`os-perf19`, `os-perf20`, `os-perf21`,
`os-perf22`, `os-map-shop22` and `os-map-final19`). No failed result is deleted
or replaced by the subsequent successful correctness gate. The rejected
v22 source is `99ea002ac163a3288f65d7da0d4b32eda73e6d79`; the independent-idle
tool source is `176eed5b40ddb9d47d87e7747373d9ed8eab78e4`.

The remaining sections describe the implemented room presentation work and
older retained experiments. Older intermediate pack sizes or offline step
counts must not be substituted for the selected production measurements.

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

## Historical validation and allocation evidence

The following records describe earlier checkpoints and resource experiments.
Their intermediate counts, pack identities and unfinished-work notes are
superseded by the final selection at the beginning of this document.

### Bounded work and ordinary-OS evidence

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
reject that empty viewport before requesting image payloads. At that intermediate checkpoint, the actual full
English preparation gate retained its16KiB-per-four-step assertion and passed
31,373checks, including a purchase while background preparation remained active.
That gate used the deterministic four-step clock; its3,093updates are not a
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
state. These are correctness results. The earlier perf4 record reported
30FPS/27.874ms in the closed room with no resource reads, but opening after
7seconds contained a1,982.544ms frame and744,992bytes of reads. This was an
intermediate resource/runtime combination, not the selected final pack.
Its failure motivated the subsequent ordinary-OS measurements reported above;
the old settled figure does not establish the final first-opening behavior.

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

The initial room-work source was frozen as local commit
`0b00d2a45932b3b17fdd20317df13553b4a1a526` after the connector's remote writes
were interrupted. That commit is explicitly **not published or remotely
verified**. Its immutable source archive and local validation receipts retain
the distinction; no GitHub CI result is implied.

Both32KiB and64KiB band/frequency glyph grouping were executed on the original
Chinese pack. All53,631 glyph records retain their exact payloads and metrics,
all strings remain byte-identical and shared artwork is unchanged. The32KiB
result saves83,744bytes;64KiB saves127,752bytes. Against the same frozen UI14
renderer, both produce identical full-frame transcripts in all seven room
scenes. At that stage,32KiB grouping was selected for its lower
cold-read/decode volume; later small-font grouping is included in strings15:

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

The complete resources, actual-art room regressions and ordinary-Ndless
Firebird opening measurements were subsequently completed and are reported
under the final selection above. The historical UI14 first-rug stall of
3771.576ms is retained as an earlier observation, not substituted for a
matched final-runtime comparison. Reduced reads alone never establish
first-opening performance acceptance.
