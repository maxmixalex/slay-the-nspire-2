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
slices inside groups still needed by another image. A separate
`compact_ui6_merchant.py` subsequently removes only proved unreachable old
blocks, repairs every affected absolute and relative pointer, and verifies
all current decoded images and encoded group bytes. It never recompresses
images or changes glyph order. No runtime pixel/cache allocation is added by
the atlas correction or its compaction.

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

### Recorded resource validation and storage improvement

The art sampler and initial correction ran from immutable source checkpoint
`6b46a74eddf37381cff9a89cdbcf30572849cf73`. The compactor, synthetic checks,
final split, and the following actual-pack UI checks ran from checkpoint
`20262f09eb1edab0035e632ca9ee4f065fb51fc4`.

- 1,518 merchant candidates produced **1,517 changed images**; the unchanged
  backdrop was skipped. No clip/image/auxiliary directory entry was added by
  this repair, and the two direct-material candle clips were untouched.
- The repair added 2,757,238 bytes of new common art. Compaction recovered
  **2,555,380 bytes** in two disjoint ranges, including 58 alignment bytes.
  The repaired merchant artwork therefore adds **201,858 bytes net**.
- Both languages independently checked all **47,686 current images**, with
  exact decoded pixel/alpha equality before and after compaction. All retained
  compressed payloads, tile/index-group bytes, glyph order/metrics and UTF-8
  strings were preserved. 3,524 retained relative pointers were relocated.
- The 9,454 decoded bytes of obsolete slices inside still-shared groups were
  deliberately retained; their encoded bytes cannot be removed independently.
- Final splitting verified both reconstructions byte-for-byte and verified
  identical shared artwork. Common plus the two locales total **81,543,574
  bytes**, compared with the recovered UI5 pair's 81,249,303 bytes: this entire
  UI6 resource extension, including the merchant repair and new UI/VFX art,
  adds **294,271 bytes**. These totals exclude the launcher and saves.

| Final resource | Bytes | SHA-256 |
| --- | ---: | --- |
| `nSpire2.common.tns` | 75,196,602 | `a2d3c270a16a9d0f2670547b1569c50c11453b5c21c8e498b61d2a9377975063` |
| `nSpire2.eng.tns` | 1,782,129 | `579b4b87d76a0071811986ea693a25c40375f29c69f9fe3db6e6c89394277174` |
| `nSpire2.zhs.tns` | 4,564,843 | `c79247964c630cf374a2732c85e73d5aa601e80de545bada17ef7de4dba64ba8` |

The stage-3 actual-pack map, rest/treasure room, and reward suites passed in
both languages: respectively **12,994 / 8,617 / 2,915 checks per language**.
These include repeated/pixel assertions and must not be presented as distinct
features. The original atlas fixture passed six tests, and all three new
compaction fixtures passed, covering shared descriptors, fully orphaned groups
and positive/negative relative links across removed spans. The stage-3 full
Python discovery still reported two unrelated localization/version-fixture
failures; those are not relabeled as passes here. Subsequent runtime/fixture
validation belongs to the integration report for that later checkpoint.

Private `run_logged` receipt suffixes: art `e2ff874eab05`, corrected packs
`42a17c8e76a6`, compaction synthetic `ffb5938b884b`, final English compaction
`d351c25d0e7b`, Chinese `045ad80f56c4`, and split/reconstruction `dc98cd52aee8`.
Map/room/reward English receipts are `058f7c73aecd`, `343962b256a1`,
`10ba4dcd5b54`; Chinese receipts are `2fecf79edf7f`, `0599f5cd243e`,
`550b845e3505`. These journal identifiers refer to preserved private build
evidence and are not public artwork downloads.

`card_grid_benchmark` now also has real shop-inventory idle and hover scenes.
It includes actual pointer opening through the first complete inventory frame,
and includes both the opening frame and the first real hit-targeted hover
frame in the hover cold sample. Warm samples continue the underlying room
animations, record ranged-resource callbacks/bytes/seeks, and compare full
canonical saves and hashes. This provides the needed measurement workload;
adding it alone is not a measured runtime speed improvement.

### Final functional replay at stage 4

Checkpoint `8c9270300aae0f479dadb216cfb37744e9c2b220` re-ran the complete
`test_*.py` discovery: **143/143 passed**, including the reviewed compiled
localization fallbacks and the corrected unsupported-version fixture. The six
additional Python unittest modules passed **54/54**, including the six atlas
tests. The optional ARM emulator command-line scripts were not counted as
unittest coverage; they require a linked ELF and a separate Unicorn runtime.

The final compacted English and Chinese packs each passed the actual map
suite (**12,994 checks**), rest/treasure room suite (**8,617 checks**) and
reward suite (**3,410 checks**). Synthetic map travel passed **154,920 checks**,
including original HUD settings opening the pause overlay directly over an
in-flight map. This corrects the obsolete test expectation that the settings
button first closed the map. Production behavior did not need a second patch.

The reward captures now explicitly advance the entrance from 0 to 250 to
500 ms and verify alpha progression plus byte-identical canonical saves.
Both settled loot screens were visually reviewed: banner, paper, rows and
labels are visible and remain within their authored proportions. Chinese
rest copy is directly below the option labels, with no doubled gold shadow.
The full world UI integration passed **27,039 checks in each language**;
its actual captures were reviewed for the pre-removal facts, growing card,
original slash/erosion samples, corrected merchant orientation, and all five
characters' inventory geometry. No runtime mirror was added to the merchant.

Private stage-4 receipt suffixes: Python discovery `be87b996b77a`, additional
unittest suites `32dd14f4e04b`, map travel `8e170ef9d977`; English map/room/reward
`37da4417175c` / `deaed3c001c6` / `f2cf1cc0f364`; Chinese map/room/reward
`b12b28e99197` / `feabfefdfd47` / `385f915ea3e1`. The combat agent's full world
UI receipts are `969731c071a6` / `7c6579bf6437`. Earlier failed journals remain
preserved: an overly broad Python import pattern included two optional ARM
CLI modules without Unicorn, and the first reward capture run lacked its
output directory. Neither is represented as a completed validation run.
