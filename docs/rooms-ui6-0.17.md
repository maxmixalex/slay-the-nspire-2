# UI6 rooms, map, transitions and card removal

Reference: **Slay the Spire 2 v0.111.0 / 41cef1ea**. This document records
implementation and reference boundaries. Verification results must name the
checkpoint and generated pack actually exercised; prior UI5 results are not
acceptance of this revision. Original files, decompiled source, sampled artwork
and comparison captures stay in the private input/output directories.

## Original source and important findings

The reference inputs are `SlayTheSpire2.pck` SHA-256
`c60f672ee7804e6aefa1e19a582fa1c80b126a7b0eef4d084d3abf110df2eab7`
and `sts2.dll` SHA-256
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.
Relevant scenes/classes include `NMapScreen`, `NMapLegendItem`,
`NMerchantRoom`, `NMerchantButton`, merchant inventory/slot scenes,
`NRestSiteButton`, reward-screen/common-banner scenes, `CardPileCmd.RemoveFromDeck`
and `NCardRemoveVfx` with its original materials.

### Merchant orientation: atlas conversion, not actor reflection

The original merchant scene and `NMerchantButton` focus/ready code retain their
positive scene scales and switch default/outline skins. They do not flip the
merchant toward the player on hover. A private comparison using the exact same
idle pose, bone matrices, original scene origin and scale identified the actual
error: rotated Spine atlas regions were unpacked with Pillow's positive angle.

Spine's packed-region UV mapping requires the inverse unpacking rotation. The
primary public reference is
[Spine 4.2 MeshAttachment.updateRegion](https://github.com/EsotericSoftware/spine-runtimes/blob/4.2/spine-ts/spine-core/src/attachments/MeshAttachment.ts),
fetched blob `7ad9104836e154f46a17f9fc6b6a8c6a03fb671c`. A 90-degree region maps
canonical `(u,v)` into atlas `(v,1-u)`. `animation.py` now unpacks with `-rot`.
The merchant's head atlas region is rotated 90 degrees; the independent private
atlas reconstruction restores his face toward the left and his hands resting
correctly on the rug without changing his world matrix. Curtains and the
player's affected parts also stop appearing turned around inside their meshes.

`patch_ui6_merchant.py` rebakes the backdrop and eight affected Spine clips
(1695–1702, 1,518 candidate images). The two candle clips (1703–1704) use PNG
material sampling directly and are excluded. Candidates are compared against
the previous decoded RGB565/alpha payload; identical candidates add no bytes.
The correction retains all image, clip and auxiliary ids, frame timing, pivots,
crop sizes, unrelated image entries and localized payloads. Changed samples
use the existing bounded 128 KiB retained-group encoder, with the backdrop
remaining independently decodable. It never overwrites a shared old group.
Receipts distinguish fully unreachable stored blocks from obsolete decoded
slices inside groups still needed by another image. Reclaiming those old bytes
requires a later lossless repack; no runtime pixel/cache allocation is added.

The general atlas loader correction applies to future conversions, but this
targeted pack repair does **not** rebake the entire game's old animation
catalog. Other already-baked rotated regions require a separately bounded
conversion pass and visual review before claiming the whole catalog corrected.

## Map and room transitions

The first reachable node now pulses in mouse mode like the other reachable
nodes. A default numeric cursor is not visual focus; only pointer hover or
explicit directional navigation suppresses the pulse. Leaving hover restores
the pulse. None of these paths consumes gameplay RNG or mutates map state.

The original legend is a persistent sheet at the right of the map, rather than
a custom folded button/dialog. Six original localized labels, original legend
icons and localized hover descriptions are visible, with the normal map close
button remaining independent. Pointer movement selects legend hover content;
leaving the sheet returns to map hits. Calculator Detail still offers a
keyboard focus entry, with Up/Down navigation and Back/Left/Confirm returning
to the graph. The graph's horizontal camera anchor moves from 166 to 126 on the
320-pixel display so its route icons are not hidden by the persistent sheet;
the graph scale, authored spacing, noise, scroll and canonical routes are
unchanged. This camera placement is an explicit small-screen adaptation.

Map travel preserves the original normal/fast fade durations already in the
transition model. Bounded combat resource preparation holds a fully black
transition until its first frame can be prepared; its elapsed time no longer
consumes the incoming room's fade. There is no custom combat loading artwork,
progress text or artificial minimum loading animation. Preparation itself is
still incremental. Separately, combat presentation discards old room enemies
when leaving the actual combat room, preventing a stale enemy room from being
drawn during the transition into an event.

## Merchant inventory and first-open work

The original 1,747×978 inventory surface is projected to the rug at
`(0,41) 320×180`. Its seven card positions, three relic positions, three potion
positions and removal service derive from the original slots. Cards are
36×50 normally and 44×62 when hovered or directionally focused; relic and
potion sprites use the matching smaller projection. Sold cards disappear.
Original gold price icons, affordable/unaffordable colors and hover tooltips
replace custom selection frames, selected-name strips and repeated Details
instructions. Actual hit rectangles supply neighboring card-hover behavior.

Visible shop portraits are collected and prepared in one bounded batch before
the card loop, using the existing image pool. The card renderer's separately
implemented size-keyed cache supports both shop card sizes without allocating
a second full-card cache. The purpose is to reduce the real first inventory
present cost while preserving card pixels; measurements belong to the final
matching resource/runtime checkpoint, not this implementation description.

## Rest and rewards

Rest option icons, labels and centered description now follow the original
scene's proportions. Enabled option labels share the original gold color.
The description's shadow is rendered through a black-tinted, half-alpha
canvas, then the rich text is rendered once in its authored colors. Previously
the gold rich-text spans ignored the nominal shadow color and produced a
second displaced gold copy, especially visible on Upgrade/Deck wording.

Loot uses the original panel, banner and row aspect ratios. The shared helper
keeps reward rows consistent across world and ordinary paths: cream text,
original focused gold, actual hover state, original keyboard reticle corners,
and no synthetic rectangular selection border. Card rewards use the original
common banner. Parent-owned return/proceed controls supply their original
sprites; local Unicode next-arrow glyphs and repeated Details/Enter hints
have been removed from these reward/shop surfaces.

## Actual card-removal presentation

Successful selection removal and Grimoire removal capture the card instance,
cost, stars, facts and formatted description **before** canonical deck
compaction. Core removal remains immediate; a separate transient presentation
grows the captured card with the original 250 ms cubic-out curve, holds for
250 ms, starts the original removal effect, and removes the card preview
400 ms into that effect. Failed actions produce no removal effect.

The effect uses 24 fixed 50 ms samples from the original flare, polar ring,
slash flipbook and red erosion textures/material equations. Original 250 ms
anticipation and 100 ms slash delay govern the sample sequence. The renderer
uses one fixed 96×120 snapshot tile, retaining the cost/star margins and the
pre-removal description, so growing the preview does not rasterize a long
card body every frame. Its key includes content generation and the captured
card details. It adds no gameplay RNG, save data, replay events or per-frame
heap allocation, and fast mode does not accelerate this UI-only effect.

Secondary GPU ember/cloud particles are not represented by these samples.
The generic direct event paths that remove multiple cards without this
selection confirmation are not all observed by the new presentation bridge;
they remain separate follow-up work. This implementation must not be called
complete parity for every original removal effect.

## Regression coverage and acceptance boundaries

New synthetic checks cover asymmetric 0/90/180/270-degree atlas regions with
trim offsets; targeted pack replacement with shared old groups, unchanged
frame skipping, idempotence and stable catalog/locale data; map pulse under
mouse/keyboard/legend focus; permanent legend and canceled pointer gestures;
normal/fast fade held during actual preparation; old card facts surviving deck
compaction; removal timing; and all five characters' shop hit bounds. These
are implementation checks, not substitutes for original-PC visual comparison.

Actual private resource captures and native/WASM/ARM results must be recorded
after the source checkpoint is remotely preserved. Timing measurements must
identify cold versus warm windows and the precise resource pair. Emulator or
host timing is not a physical calculator measurement. Remaining broader room
work includes event typewriter/entry/press timing, original pickup trails,
special ancient/fake-merchant pages, secondary removal particles, and the
unrepaired historical animation regions described above.
