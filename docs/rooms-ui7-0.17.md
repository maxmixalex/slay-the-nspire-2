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

Merchant inventory now uses one **1440×1080 → 320×240** camera projection,
including rug, cards, prices, relics, potions and removal service. The original
1,747×978 rug projects to x=-27, y=18, width=388 and height=217; the viewport
clips the authored edges. The seven card centers come from the source slots,
with 43×61 idle and 53×75 focused cards. These new dimensions address the UI6
inventory becoming too small; final visual bounds still require actual pack
captures. They are not an arbitrary enlargement of selected items alone.

Character selection uses the source bottom-centered five-portrait container,
left information block, progression strip and independent back/confirm child
geometry. The portrait centers are 108, 134, 160, 186 and 212; their top is
y=194. The previous right-side vertical column and 155×209 stretched black
panel are removed. Readable 8px information text can grow the left block
upward; this is an explicit small-screen adaptation, not exact desktop text
geometry. The selected source release always hides the act dropdown.

## Source contracts and implementation boundaries

Class names below are within `MegaCrit.Sts2.Core.Nodes` unless otherwise
qualified. Scene paths are relative to the original PCK. The broad global
inventory remains the denominator discussion; this table supplies concrete
room contracts rather than claiming every generated Godot wrapper reviewed.

| Surface / original sources | Implemented behavior | Remaining or unverified |
| --- | --- | --- |
| `Screens.Map/NMapScreen.cs`, `NMapBg.cs`; `screens/map/map_screen.tscn` | Centered graph/paper, original point geometry, map open/close and travel timing; bounded preparation holds black until ready | New center needs bilingual actual capture; elastic drag/inertia and drawing controls are not all reproduced |
| `Screens.Map/NMapPoint.cs`, `NNormalMapPoint.cs`, `NBossMapPoint.cs`, `NAncientMapPoint.cs`, `NMapMarker.cs` | Reachable pulse independent of unused keyboard cursor; original marker and reticle assets; visited room disclosure and boss cycles | Deterministic cosmetic jitter substitutes for process-global Chaotic values; no original GPU pixel oracle |
| `Screens.Map/NMapLegendItem.cs` | Persistent original sheet, type highlighting and tooltip; 1.25 focus scale with 500ms exponential recovery | Readable 8px labels are a viewport adaptation; final capture pending |
| `HoverTips/NMapPointHistoryHoverTip.cs`, `Screens.Map/NMapPoint.cs` | Visited non-current node tooltip uses actual recorded floor, HP, max HP, gold, room/model and turns | New source was recovered by targeted decompilation; old saves without records show no invented history; full actions/cards/rewards history is not yet represented |
| `Screens.Map/NMapDrawings.cs`, `NMapDrawingInput.cs`, mouse/controller drawing implementations and draw/erase/clear buttons | Source and scene inventory recovered | Player freehand drawing/erasing/clearing remain unimplemented; these are not excluded with multiplayer sharing |
| `Rooms/NMerchantRoom.cs`, `NMerchantButton.cs`; `rooms/merchant_room.tscn`, `rooms/merchant_button.tscn` | Source actor orientation and original corrected atlas textures; open/close input ownership | Existing UI6 atlas correction is targeted, not a rebake of every historic asset |
| `Screens.Shops/NMerchantInventory.cs`, `NMerchantSlot.cs`; `merchant/merchant_inventory.tscn` and item scenes | Rug opens over 700ms, closes over 500ms; original dim recovery; one camera scale; real prices, availability, nearest remaining focus and failure motion | Final interactive bounds/readability and first-open costs need new resource/runtime acceptance |
| `Screens.Shops/NMerchantCard.cs`, `NMerchantRelic.cs`, `NMerchantPotion.cs`, `NMerchantCardRemoval.cs` | Source item focus/unfocus, failure wiggle, card-removal used progression, actual purchase and full-belt error integration | Additional original tooltip minutiae and source price font metrics require final visual inspection |
| `Screens.Shops/NMerchantHand.cs` | Both real and fake merchant hands sampled with official Spine runtime and constraints; item tracking, lerp and linger/return behavior | 47 poses per hand quantize angle; source Perlin micro-motion is not yet reproduced |
| `Screens.Shops/NMerchantDialogue.cs`; `merchant/merchant_rug_dialogue.tscn` | Source message families, bubble, original positions and fade/typewriter clocks | `[fly_in]` glyph transform is not yet represented |
| `Events/NEventLayout.cs`, `NCombatEventLayout.cs`, `NEventOptionButton.cs`; default/combat layouts and option scenes | Title/body reveal and original normal/fast option entry; focused/pressed motion; disabled hit ownership during entry | More than three options retain a small-screen paging adaptation; some page-to-page transitions and original particles remain |
| `Events/NAncientEventLayout.cs`, `NAncientDialogueLine.cs`, `NAncientDialogueHitbox.cs`; ancient layout/dialogue/option scenes | Original dialogue bubble/tail/character icon, entry easing and directional focus delay | `NAncientNameBanner` intro and the complete historical multi-line dialogue stack remain |
| `Events.Custom/NFakeMerchant.cs`, `Screens.Shops/NFakeMerchantInventory.cs`; three fake-merchant scenes | Distinct six-slot arrangement, correct original hand, open/close/speech and purchase failure behavior | Character/body relaxed-loop animation, focus outline and initial idle dialogue remain incomplete |
| `Events.Custom.CrystalSphere/NCrystalSphereScreen.cs`, cell/item/mask/dialogue classes; five Crystal Sphere scenes | Existing actual 11×11 game logic and hidden-cell masking remain playable | The old simplified grid/control presentation, reveal mask animation, source dialogue and exact divination controls are still different; not accepted as restored |
| `Rooms/NRestSiteRoom.cs`, `RestSite/NRestSiteButton.cs`; `rooms/rest_site_room.tscn`, `rest_site/rest_site_button.tscn` | Original icon/label/description hierarchy, entry/focus/press/unfocus clocks; original saturation/value treatment and true black shadow | Completion hide/show, result description and 800ms description vertical move remain; options above four still page |
| `RestSite/NRestSiteCharacter.cs`, `NParticleSystemUpscaler.cs`; four act scenes and five character scenes | Existing registered source room camera, per-character looping actor planes and lighting states retained | Selected-action character VFX and all independent particles are not complete |
| `Rooms/NTreasureRoom.cs`, `TreasureRooms/NTreasureButton.cs`; treasure room scene | Existing source chest/relic flow and original room artwork retained | Full hand-image choreography and multiplayer-related presentation are not jointly claimed |
| `Screens/NRewardsScreen.cs`, `Rewards/NRewardButton.cs`, `NLinkedRewardSet.cs` | Original reward panel/banner/rows, entry clocks, real procurement and post-combat wait retained | Linked reward choreography and source secondary effects need broader coverage |
| `Vfx.Cards/NCardRemoveVfx.cs`, `Commands/CardPileCmd.cs` | Direct removals now observe full card facts before deck compaction, sharing real removal preview/effect | Multiple simultaneous removals use a bounded last preview; GPU embers/clouds remain absent |
| `Vfx/NCardFlyVfx.cs` and procurement call sites | Direct shop/event pickups now create flight presentation while canonical acquisition stays immediate | Character-specific trails and exact frame-delta rotation interpolation remain different |
| `Screens.CharacterSelect/NCharacterSelectScreen.cs`, `NCharacterSelectButton.cs`, `NAscensionPanel.cs`; character-select/button/ascension scenes | Bottom-centered click selection; source info motion, focus saturation/value/scale, selected outline pulse and independently entering buttons | Readable text layout differs from desktop; random/locked progress states need separate frontend policy |
| `Screens.CharacterSelect/NCharacterSelectScreenBg.cs`, `NRegentCharacterSelectBg.cs`, `Animation/NSpineAutoPlayer.cs`; five character background scenes | Full five skeletal loops are implemented at original camera scale and duration, with authored slot followers/material overrides | Sampling and captures pending; 12fps is bounded sampling, particles/water distortion and Regent constellation hover skins remain absent |

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
64 / 140 / 160 / 80 / 96 sampled frames planned. Runtime frame selection uses
those real periods instead of multiplying a rounded per-frame millisecond
value. Initial frames also supply the five static auxiliary entries, avoiding
duplicate payloads. Independent fire shader time is explicitly frozen to
zero rather than falsely assigned the skeletal loop's period.

The new grouping helper reuses existing retained format 7: independent
128KiB-target hand groups and up to the existing 512KiB bound for character
loops. It introduces no new runtime buffer and inserts only one reachable
payload copy. Exact new decoded pixels, all historic payload bytes and locale
indices must be checked after actual conversion. A smaller implementation is
not a storage or speed result until the resulting byte counts are recorded.

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
character capture is claimed. The official fixed runtime's
[constructor and geometry binding](https://github.com/EsotericSoftware/spine-runtimes/blob/e7dc1435fa4a0083ab431f1b28e083c14a1f5c68/spine-godot/spine_godot/SpineSprite.h)
and [internal mesh generation](https://github.com/EsotericSoftware/spine-runtimes/blob/e7dc1435fa4a0083ab431f1b28e083c14a1f5c68/spine-godot/spine_godot/SpineSprite.cpp)
confirm that these extra scene nodes have no geometry. The pending correction
retains their child transforms/order without drawing a duplicate attachment.
It also applies the source Godot default Screen material's subtract mode.

The new grouping suite's first real run, receipt
`ui7-hand-groups-synthetic-979facac2502`, failed all three fixtures before
grouping because their synthetic bytearrays were passed into a byte-keyed
LZ4 helper. The fixture now produces immutable bytes. That correction and the
new serialized-node regression need the next verified checkpoint rerun.

The next gate is actual ENG/ZHS map, shop, event, rest, reward and character
captures using the new assets, followed by exact native/WASM replay. Only then
will first-open and warm merchant intervals and ranged resource reads be
measured in a quiet serial window. UI6 timings remain historical and cannot
stand in for this UI7 camera, animation and resource change.
