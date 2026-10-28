# Merchant inventory consistency — UI10

Reference remains v0.111.0 / 41cef1ea. This stage follows checkpoint
`78ac86e35651d3832e1b9638001aa0aa6be3f0a0` and fixes visible inventory artwork,
price labels and controller focus in ordinary shops and Fake Merchant. It uses
the existing inventory projection of 2/9, including its documented 7/6
readability zoom. It does not change the canonical inventory, prices, purchase
effects, RNG or save format.

## Stock navigation and purchase focus

The inspected `NMerchantInventory.UpdateHorizontalNavigation` assigns the top
row, then the bottom row, then the middle row. The two colorless controls and
removal control occur in both lower rows, so the final middle pass determines
their links. Sold controls are removed from horizontal rows. Once all relics
are sold, the source explicitly reconnects the last colorless card and removal
control to the potion row. Row edges point to themselves.

`UpdateVerticalNavigation` retains the original row indices and searches for a
stocked control at that index, checking the right neighbor before the left at
each distance. It falls through to the other row when an entire candidate row
is empty. These assignments include the source's downward self-links on the
shared colorless/removal controls. The previous fixed tables and horizontal
wraparound did not preserve these behaviors and could focus sold items.

`NFakeMerchantInventory.UpdateNavigation` instead filters its 2/3/1 rows,
removes empty rows, and clamps the selected column into the preceding or next
remaining row. Its inventory previously used a flat seven-entry loop, which
included both sold items and the calculator Back control. The new bounded
focus graph follows the actual source row assignments. Back remains available
through the dedicated key and button; when no item remains it becomes the
calculator's focus fallback.

On successful purchase, `NMerchantInventory.OnPurchaseCompleted` chooses the
nearest visible other slot by its `GlobalPosition`. The implementation now
uses the original slot positions rather than the center of an enlarged input
rectangle. Fake Merchant receives the same purchase successor behavior. The
purchased slot is excluded even when The Courier replenishes it. Opening the
inventory begins on its first stocked item. Unaffordable items remain
focusable so their original failed-purchase speech and wiggle still occur.

## Price label measurement

All four source merchant scenes use a centered Cost HBox with 6px separation,
a 54px coin and a 39px ordinary MegaLabel. English uses Kreon Bold with 2px
extra glyph spacing. MegaLabel's locale substitution makes Chinese Regular;
it does not retain the authored English bold face. This was checked in
`MegaLabel._Ready`, `RefreshFont`, `AdjustFontSize` and `MegaLabelHelper`.

The label's stored minimum font setting alone is insufficient to infer its
actual font size. A private native layout oracle therefore loaded the original
exported font resources and Cost scene controls into **Godot 4.5.1**, matching
the original PCK's engine version. It reproduced the inspected MegaLabel
fitter with the engine's TextParagraph measurements. The 56 combinations
cover both locales, all four Cost scenes, and prices 0, 9, 20, 99, 100, 999 and
1000. Measurements were recorded at creation and after 1, 2, 5 and 16 frames.
The settled label remained 39px in every combination.

| Measured source value | English | Chinese |
| --- | ---: | ---: |
| Font size | 39 | 39 |
| Label height | 49.5625 | 58.5 |
| Font ascent | 38.1875 | 45.5 |
| Relic Cost label Y in its HBox | 2 | -1 |
| Relic Cost baseline | 40.1875 | 44.5 |

The label height at creation already has these values. This matters because
Large relic placement reads the height before setting the price text.
The private oracle is an original-resource native **layout** measurement, not
a capture of the unmodified original game. The exported fonts and source
project remain private.

The renderer now projects the measured baseline, source spacing, coin size
and label font through each slot's own 0.65/0.8 scale. Prices use existing
6px/7px glyph bands, English Bold and Chinese Regular. The ordinary inventory
and Fake Merchant share the same label painter. Affordable prices use the
original cream `#FFF6E2`; affordable sale cards use `#7FFF00`; unaffordable
prices use `#FF5555`. The coin does not dim when gold is insufficient. The
source's 15px dark outline is projected with the label instead of leaving the
old unrelated 8px/10px text styles.

## Relic artwork and input bounds

`NMerchantRelic.UpdateVisual` sizes and positions its hitbox from the actual
relic icon. The source `RelicHolder` has position (-64, -80) and scale 2.
An ordinary 68px NRelic control has 4px margins, leaving a 60px icon. The
resulting icon rectangle relative to the slot is (-56, -72, 120, 120).

Fake Merchant uses Large icons. It resizes NRelic to 128px, leaving a 120px
icon, and subtracts `CostLabel.Size.Y` from the icon's local Y. Its resulting
rectangle is (-56, -72 - 2H, 240, 240), where H is the measured label height.
The previous centered 19px icon missed both the large size and this offset.

| Projected icon relative to the slot | Resting 0.65 | Focused 0.8 |
| --- | --- | --- |
| Ordinary | (-8, -10), 17×17 | (-10, -13), 21×21 |
| Fake Merchant, English | (-8, -25), 35×35 | (-10, -30), 43×43 |
| Fake Merchant, Chinese | (-8, -27), 35×35 | (-10, -34), 43×43 |

Both inventories now use these icon rectangles for pointer focus and input;
the price is not part of the relic click area. The immediate focus enlargement
and existing 500ms ExpoOut return share the same geometry. Fake Merchant's
hover information also uses the source 122px merchant-slot anchor and its
left/right alignment, independent of the larger icon. It retains the complete
canonical relic and associated-model hover set.

## Storage and runtime cost

No new image, glyph, pack version, persistent state or pixel pool is required.
The existing 48px original relic images already cover the 43px focused Large
icon. The stage-D font packs were checked for every decimal digit at 6px and
7px in both required locale faces. Each price resolves its bounded glyph list
once and reuses those glyph references for outline and foreground passes.
Navigation scans at most 14 slots and performs no allocation; purchase focus
uses squared integer distance rather than a square root.

## Verification and remaining limits

The standalone `merchant_navigation_regression` passes 129 assertions against
independently transcribed source neighbor tables, sold-row cases, purchase
successors and explicit English/Chinese icon coordinates. Short isolated App
checks use the preserved stage-D core with only this merchant source overlaid,
so unrelated in-progress frontend changes cannot affect the result. The
synthetic event suite passes 5,149 assertions; it checks large pointer bounds,
the actual changed price pixels, source hover anchors, failed purchase,
purchase successor, sold navigation and unchanged canonical saves. The room
suite passes 8,883 assertions and additionally checks ordinary source navigation through actual key
input, normal relic bounds, pickup lifecycle and real room exit.

The parent's next coherent checkpoint owns the final bilingual resource runs,
target builds and combined integration suite. These assertions establish the
specific repaired behaviors; their count is not a fidelity percentage.
Software glyph rasterization, integer pixel projection and the previously
documented inventory zoom remain adaptations. The layout measurements do not
establish GPU pixel identity, restore unrelated room animation, or count
unexamined scenes as verified.
