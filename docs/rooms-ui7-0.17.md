# UI7 room, map and character-selection review

Reference: **Slay the Spire 2 v0.111.0 / 41cef1ea**. This is a work-in-progress
implementation inventory. A source contract, an implemented renderer, a passing
synthetic fixture and an actual bilingual capture are separate evidence.
None of the counts below establishes a restoration percentage.

The original PCK and assembly identities are unchanged from the UI6 room
report. Original files, decompilations, sampled artwork and captures stay
private. Public code contains independently implemented behavior and numeric
scene facts. Audio, multiplayer and Timeline are outside project scope;
single-player map drawing and custom event interfaces remain in scope.

## Corrected geometry

The map graph and paper now share display center **x=160**. The previous UI6
graph-only shift to x=126 moved the routes off their authored paper. Original
spacing, named jitter stream and scroll bounds remain intact. The legend
retains the original viewport-width × 0.8 anchor, with a readable small-screen
text column. This is a correction to the earlier documented adaptation.

Inspection of the actual sampled backgrounds exposed an incorrect camera
assumption. `NGame.ApplyDisplaySettings` selects **1680×1260** for FourByThree;
the prior 1440×1080 projection cropped character scenes too closely. The live
sampler and shared `reference_viewport.hpp` now use the source 1680×1260
canvas. All 540 corrected frames were sampled and checked under checkpoint 6.
Their background inspection restores the previously cropped Defect face/chest
and Ironclad sword grip. Complete-screen acceptance remains pending; background
samples do not establish readable UI or correct interaction. The old samples
remain available for comparison.

Merchant inventory retains an explicit **7/6 readability zoom** around display
(160,0), applied after that original projection to the rug and all its children.
The combined scale is 2/9; it is a small-screen adaptation, not the original
camera scale. The 1,747×978 rug is x=-27, y=18, width=388 and height=217. The
seven source card centers, 43×61 idle cards and 53×75 focused cards share this
same transform, preserving their relative size to the rug. The viewport clips
the rug edges. Final merchandise bounds, text readability and interaction
still require actual complete-screen captures.

Character selection uses the source bottom-centered five-portrait container,
left information block, progression strip and independent back/confirm child
geometry. At the corrected viewport the portrait centers are 116, 138, 160,
182 and 204; their top is y=200. The previous right-side vertical column and 155×209 stretched black
panel are removed. Readable 8px information text can grow the left block
upward; this is an explicit small-screen adaptation, not exact desktop text
geometry. The selected source release always hides the act dropdown. Original
back/confirm child controls preserve the full 300×202 atlas margins and the
source keep-aspect behavior instead of stretching the visible 267×160 region.

## Source contracts and implementation boundaries

Class names below are within `MegaCrit.Sts2.Core.Nodes` unless otherwise
qualified. Scene paths are relative to the original PCK. The broad global
inventory remains the denominator discussion; this table supplies concrete
room contracts rather than claiming every generated Godot wrapper reviewed.

| Surface / original sources | Implemented behavior | Remaining or unverified |
| --- | --- | --- |
| `Screens.Map/NMapScreen.cs`, `NMapBg.cs`; `screens/map/map_screen.tscn` | Centered graph/paper, original point geometry, map open/close and travel timing; bounded preparation holds black until ready | CP8 actual bilingual center and interaction accepted; elastic drag/inertia and drawing controls are not all reproduced |
| `Screens.Map/NMapPoint.cs`, `NNormalMapPoint.cs`, `NBossMapPoint.cs`, `NAncientMapPoint.cs`, `NMapMarker.cs` | Reachable pulse independent of unused keyboard cursor; original marker and reticle assets; visited room disclosure and boss cycles | Deterministic cosmetic jitter substitutes for process-global Chaotic values; no original GPU pixel oracle |
| `Screens.Map/NMapLegendItem.cs` | Persistent original sheet, type highlighting and tooltip; 1.25 focus scale with 500ms exponential recovery | CP8 actual bilingual sheet/tooltip inspected; readable 8px labels are a viewport adaptation |
| `HoverTips/NMapPointHistoryHoverTip.cs`, `Screens.Map/NMapPoint.cs` | Visited non-current node tooltip uses actual recorded floor, HP, max HP, gold, room/model and turns | New source was recovered by targeted decompilation; old saves without records show no invented history; full actions/cards/rewards history is not yet represented |
| `Screens.Map/NMapDrawings.cs`, `NMapDrawingInput.cs`, mouse/controller drawing implementations and draw/erase/clear buttons | Source and scene inventory recovered | Player freehand drawing/erasing/clearing remain unimplemented; these are not excluded with multiplayer sharing |
| `Rooms/NMerchantRoom.cs`, `NMerchantButton.cs`; `rooms/merchant_room.tscn`, `rooms/merchant_button.tscn` | Source actor orientation and original corrected atlas textures; open/close input ownership | Existing UI6 atlas correction is targeted, not a rebake of every historic asset |
| `Screens.Shops/NMerchantInventory.cs`, `NMerchantSlot.cs`; `merchant/merchant_inventory.tscn` and item scenes | Rug opens over 700ms, closes over 500ms; original dim recovery; one camera scale; real prices, availability, nearest remaining focus and failure motion | CP8 actual ENG/ZHS seven-card/relic/potion bounds accepted; small card text is truncated with tooltip access. Final original fonts and first-open/warm costs still need acceptance |
| `Screens.Shops/NMerchantCard.cs`, `NMerchantRelic.cs`, `NMerchantPotion.cs`, `NMerchantCardRemoval.cs` | Source item focus/unfocus, failure wiggle, card-removal used progression, actual purchase and full-belt error integration | Additional original tooltip minutiae and source price font metrics require final visual inspection |
| `Screens.Shops/NMerchantHand.cs` | Both real and fake merchant hands sampled with official Spine runtime and constraints; item tracking, lerp and linger/return behavior | 47 poses per hand quantize angle; the original Perlin drift and its fixed +40/-50 source-pixel bias remain absent |
| `Screens.Shops/NMerchantDialogue.cs`; `merchant/merchant_rug_dialogue.tscn` | Source message families, bubble, original positions and fade/typewriter clocks | `[fly_in]` glyph transform is not yet represented |
| `Events/NEventLayout.cs`, `NCombatEventLayout.cs`, `NEventOptionButton.cs`; default/combat layouts and option scenes | Title/body reveal and original normal/fast option entry; focused/pressed motion; disabled hit ownership during entry | More than three options retain a small-screen paging adaptation; some page-to-page transitions and original particles remain |
| `Events/NAncientEventLayout.cs`, `NAncientDialogueLine.cs`, `NAncientDialogueHitbox.cs`; ancient layout/dialogue/option scenes | Original dialogue bubble/tail/character icon, entry easing and directional focus delay | `NAncientNameBanner` intro and the complete historical multi-line dialogue stack remain |
| `Events.Custom/NFakeMerchant.cs`, `Screens.Shops/NFakeMerchantInventory.cs`; three fake-merchant scenes | Distinct six-slot arrangement, correct original hand, open/close/speech and purchase failure behavior | Character/body relaxed-loop animation, focus outline and initial idle dialogue remain incomplete |
| `Events.Custom.CrystalSphere/NCrystalSphereScreen.cs`, cell/item/mask/dialogue classes; five Crystal Sphere scenes | Existing actual 11×11 game logic and hidden-cell masking remain playable | The old simplified grid/control presentation, reveal mask animation, source dialogue and exact divination controls are still different; not accepted as restored |
| `Rooms/NRestSiteRoom.cs`, `RestSite/NRestSiteButton.cs`; `rooms/rest_site_room.tscn`, `rest_site/rest_site_button.tscn` | Original icon/label/description hierarchy, entry/focus/press/unfocus clocks; saturation/value and true black shadow. CP9 source adds whole ChoicesScreen 500ms hide/reentry, local Heal/Smith waits, canceled-picker restoration and first-Proceed gating | CP9 completion path awaits immutable execution. Source retains the selected Option.Description; there is no invented result message. Options above four still page; heal smoke/desaturation and Smith secondary VFX remain absent. The 800ms vertical description move belongs only to excluded multiplayer Mend |
| `RestSite/NRestSiteCharacter.cs`, `NParticleSystemUpscaler.cs`; four act scenes and five character scenes | Existing registered source room camera, per-character looping actor planes and lighting states retained | Selected-action character VFX and all independent particles are not complete |
| `Rooms/NTreasureRoom.cs`, `TreasureRooms/NTreasureButton.cs`; treasure room scene | Existing source chest/relic flow and original room artwork retained | Full hand-image choreography and multiplayer-related presentation are not jointly claimed |
| `Screens/NRewardsScreen.cs`, `Rewards/NRewardButton.cs`, `NLinkedRewardSet.cs` | Original reward panel/banner/rows, entry clocks, real procurement and post-combat wait retained | Linked reward choreography and source secondary effects need broader coverage |
| `Vfx.Cards/NCardRemoveVfx.cs`, `Commands/CardPileCmd.cs` | Direct removals now observe full card facts before deck compaction, sharing real removal preview/effect | Multiple simultaneous removals use a bounded last preview; GPU embers/clouds remain absent |
| `Vfx/NCardFlyVfx.cs` and procurement call sites | Direct shop/event pickups now create flight presentation while canonical acquisition stays immediate | Character-specific trails and exact frame-delta rotation interpolation remain different |
| `Screens.CharacterSelect/NCharacterSelectScreen.cs`, `NCharacterSelectButton.cs`, `NAscensionPanel.cs`; character-select/button/ascension scenes | Bottom-centered click selection; source info motion, focus saturation/value/scale, selected outline pulse and independently entering buttons | Readable text layout differs from desktop; random/locked progress states need separate frontend policy |
| `Screens.CharacterSelect/NCharacterSelectScreenBg.cs`, `NRegentCharacterSelectBg.cs`, `Animation/NSpineAutoPlayer.cs`; five character background scenes | Five skeletal loops use actual durations, authored slot followers and material overrides; all 540 corrected 1680×1260 frames were sampled and their static/dynamic recombination verified | Complete-screen acceptance remains pending; 12fps is bounded sampling, particles/water distortion and Regent constellation hover skins remain absent |

## Actual resource evidence already available

The official Spine 4.2 runtime was built from fixed commit
`e7dc1435fa4a0083ab431f1b28e083c14a1f5c68`. Its private assembly SHA-256 is
`ba71b52d54a9cd72b2c0c364f8defeafd8ec93ad5e80b3a71c465d3dea44b8d1`.
The logged stage-2 hand sampling passed: 94 actual hand poses used 1,036,637
independently encoded payload bytes before new grouping. Sample bounds were
inside the 320×240 output. This proves those sampled poses and their geometry;
it does not accept the complete merchant screen or measure its frame rate.

Character metadata was independently read from the same runtime. Original
loop durations are 5,333 / 11,667 / 13,333 / 6,667 / 8,000 ms, with
64 / 140 / 160 / 80 / 96 frames actually sampled at checkpoint 5. Runtime frame selection uses
those real periods instead of multiplying a rounded per-frame millisecond
value. Independent fire shader time is explicitly frozen to
zero rather than falsely assigned the skeletal loop's period.

The complete checkpoint-5 conversion passed with `source_changed=false`:
`ui7-full-assets-with-explicit-font-b88ba62f78bc`. Its ENG pack is 92,994,374
bytes, SHA-256 `9c9bd8cb09fee9baa0074d74eb0cf7b97159e2a90a6a9ea644041acc5af1dc26`;
ZHS is 95,787,006 bytes, SHA-256
`763d17d4076674345b77272cd05c96f1a9bc621f75f51c64114b5c81ff06043b`.
This validates 1,092 appended images, five clips, preserved historic payloads
and locale indices, but does **not** accept the old camera framing as correct.
The temporary glyph supplement is superseded by the separate original-font
replacement stage before delivery.

The executed grouping used existing retained format 7: 128KiB-target hand
groups and character groups within the existing 512KiB bound. For the old
full-frame artwork, 634 images used 202 groups; the largest decoded group was
465,840 bytes. Hand payloads fell from 1,036,682 padded independent bytes to
536,668 bytes. Across hands and characters, grouping saved 29,883,044 bytes
relative to independent image encoding. New common artwork still occupied
15,980,182 bytes. These are storage measurements of the prior-viewport pack,
not timing or final UI acceptance results.

### Executed corrected conversion and lossless storage follow-up

The corrected character converter now separates each loop into one static
plane and its original ordered dynamic frames. A complete 16×16 region moves
to the static plane only if its RGB565 color **and alpha** are identical in
every frame. Static and dynamic masks are disjoint, so partially transparent
pixels are applied once, in the existing rendering order. The converter
decodes both resulting planes and checks their recombination against every
original sampled frame. Source timestamps, frame counts and durations are
unchanged; no runtime pixel buffer or new image format is introduced.

The new catalog contains five bases plus 540 dynamic frames, changing the
appended image count from 1,092 to 1,097 while leaving auxiliary and clip counts
unchanged. Older v24 packs alias each character auxiliary to the first complete
frame; the renderer detects that alias and skips the extra base draw. The new
three-case synthetic suite covers opaque/partial-alpha pixels, edge regions,
identical loops and corrupt conversion input. It passed under checkpoint 6,
receipt `ui7-character-static-layers-2df7d2922357`.

The actual corrected bilingual conversion also passed with
`source_changed=false`, receipt
`ui7-corrected-character-room-assets-37189e69528b`. All 1,097 appended images,
historic payload/index/cost preservation, locale indices and offset relocation
were checked. Its intermediate v24 outputs are:

| Locale | Bytes | SHA-256 |
| --- | ---: | --- |
| ENG | 90,960,006 | `8def5d1bd11d711bebf2d4a09a0da4d3a72a8d937a2dab46bae4ab8e8b4302dd` |
| ZHS | 93,752,638 | `f58c68fa2976a49beab40f151b2a78d5fbf9228d8cff4492f9868c86184e6fd7` |

These are converter inputs for the subsequent Phobia, profile/HUD and original
font stages, not final delivery hashes. The temporary Source Han Serif Medium
supplement adds 10 ENG and 45 ZHS glyphs and is superseded by actual font-face
replacement. Both packs share 13,945,728 bytes of appended common artwork,
SHA-256 `918bcaf926f009315713c4d7c8c4385050b7748acfd1f7dda70e47ccb6419e01`.

The matched storage comparison, receipt
`ui7-character-storage-comparison-510dd6db22b8`, holds the corrected 1680
viewport, original 540 samples, other artwork and retained codec constant.
Full-frame retained artwork uses 15,231,994 bytes; static-plus-dynamic retained
artwork uses 13,945,728 bytes. After 86 bytes for the five additional directory
and cost entries, the **net reduction is 1,286,180 bytes per pack**. This is not
a comparison against the incorrectly cropped checkpoint-5 samples. Group count
falls from 202 to 177; the largest decoded group is 496,256 bytes, within the
existing 512KiB bound. There is no new runtime pixel buffer, unreachable old
payload or runtime integrity scan. Each of the 540 reconstructed RGB565 and
alpha frames matches its original sample exactly. This establishes storage
and converter equivalence; runtime I/O, rendering speed and original Godot GPU
pixel equivalence are not established by this comparison.

## Acceptance still required

Stage-1 failures include an obsolete map fixture that pressed Back after a
mouse release had already dismissed legend focus, and shop fixtures trying to
click before the original rug entry finished. Source fixtures now retain the
full drag-to-boundary and real click coverage with correct timing; no pass is
claimed until the immutable-stage rerun.

The complete stage-4 converter was actually executed from verified checkpoint
`68350fe532d34ee7c16f55027324b4f54be86816`. It reused the accepted 94 hand
poses and produced the static prefix, then failed on the first character's
extra serialized `SpineMesh2D`. Receipt `ui7-full-room-character-art-a2c2184bfcdd`
records failure with `source_changed=false`; no completed resource pack or
character capture was claimed for that failed stage. The official fixed runtime's
[constructor and geometry binding](https://github.com/EsotericSoftware/spine-runtimes/blob/e7dc1435fa4a0083ab431f1b28e083c14a1f5c68/spine-godot/spine_godot/SpineSprite.h)
and [internal mesh generation](https://github.com/EsotericSoftware/spine-runtimes/blob/e7dc1435fa4a0083ab431f1b28e083c14a1f5c68/spine-godot/spine_godot/SpineSprite.cpp)
confirm that these extra scene nodes have no geometry. The checkpoint-5
correction retains their child transforms/order without drawing a duplicate
attachment. It also applies the source Godot default Screen material's subtract
mode. The subsequent 19-case scene suite passed under receipt
`ui7-scene-geometry-synthetic-2f3e641ade2f`.

The new grouping suite's first real run, receipt
`ui7-hand-groups-synthetic-979facac2502`, failed all three fixtures before
grouping because their synthetic bytearrays were passed into a byte-keyed
LZ4 helper. The fixture now produces immutable bytes. The corrected three-case
suite passed at checkpoint 5 under receipt
`ui7-hand-groups-synthetic-7c6b744ec39b`; the first failure remains recorded.

The complete checkpoint-4 native build passed, followed by 68/84 passing
regression suites. Six room/map suites exposed stale interaction assumptions:
the Ancient pixel oracle sampled outside its shifted third option; the shop
test's fixed leftmost route contained no shop; camp input was attempted before
the source 500ms entry block; the library skipped its restored Compendium
route; and character/Ancient entry fixtures clicked before the original
controls arrived. The live fixtures now use actual generated routes, explicit
source timing boundaries and the restored intermediate controls. The selected
character scale/value expectation also now matches the source's separate
initial, hover and selected states. These fixture migrations preserve the
real click, save and pixel assertions and await the next immutable rerun.

Checkpoint 6 built successfully and passed 75/87 native suites. Five assigned
failures were traced to precise obsolete expectations: Q12 interpolation gives
an exact portrait value of 810; leaving a native fake-merchant room preserves
its finished event beneath the map; normal shop Back is a `Choice` control;
the wraparound map-intro fixture must account for the actual character/Ancient
entry prelude; and Rest's 500ms entry block cannot exercise the fast 300ms fade
pointer gate. The pending fixture changes preserve exact values and full saved
state. The last case uses an actual generated Shop route, whose original
closed-room Proceed is immediately available, retaining the same pointer,
keyboard, 599/600ms travel, saved-state and RNG assertions. No rerun of these
changes is claimed before the next verified checkpoint.

CP8 actual v26 artwork passed the room, map and reward suites in both languages,
with unchanged canonical-save assertions and `source_changed=false`:
`ui7-room-actual-eng-ad9e6be6631a`, `ui7-room-actual-zhs-e0160a6ce848`,
`ui7-map-actual-eng-71f9b6f182f6`, `ui7-map-actual-zhs-d10049a85ebf`,
`ui7-reward-actual-eng-7386f769a831`, `ui7-reward-actual-zhs-3147eda7b992`.
Actual whole-screen inspection confirms the shared x=160 map/paper center,
right-side original legend, seven shop cards plus relics/potions inside the
visible rug, sampled pointing hands, and rest label/description hierarchy.
These captures use the intermediate v26 fonts, not the final original-style
font replacement. They are port output, not an original-game GPU pixel oracle.

The remaining native event failure is a fixture replacing an Ancient option
key after its layout had already entered. That replacement starts a fresh
1000ms Ancient entrance, so the third button is initially below the clipped
viewport. The CP9 fixture waits the full source interval and retains unique,
enabled and in-bounds hit assertions. No CP9 execution is claimed yet.

The next gate is CP9 completion-boundary tests, actual event and character
screens with final fonts, followed by exact native/WASM replay. Only then
will first-open and warm merchant intervals and ranged resource reads be
measured in a quiet serial window. UI6 timings remain historical and cannot
stand in for this UI7 camera, animation and resource change.
