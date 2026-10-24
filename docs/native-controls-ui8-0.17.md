# Original Proceed and treasure composition

Reference: v0.111.0 / 41cef1ea, `NProceedButton`, `proceed_button.tscn`,
`NCommonBanner`, `common_banner.tscn`, `treasure_room.tscn`,
`NTreasureRoomRelicHolder`, `treasure_relic_holder.tscn` and `relic.tscn`.

The original Proceed position is a viewport ratio `(1583,764)/(1920,1080)`.
On the 320×240 host its parent hit rectangle is `(264,170,51,21)`. The child
image has a separate 296×155 source box and preserves the 393×192 atlas aspect
ratio, producing a 56×27 arrow. Its label rectangle is measured against that
image box, not against the raw atlas. The previous implementation placed a
regular 12px label near the bottom of the display and used a legacy oversized
hit rectangle. The new label uses the original bold face at a 6px target with
a red outline. Fitting is performed once for all outline passes.

Merchant, fake merchant, treasure, rest, combat rewards, event/relic rewards,
potion rewards and Crystal terminal pages share this component and hit box.
Disabled controls are absent and cannot capture a pointer. Focus uses the
source 50ms scale transition, press/unfocus use 500ms, and the original outline
pulse runs between 0.25 and 0.75 in half-second legs. The independent original
enable/disable translation now slides by 400/1920 of the viewport width with
500/800ms Expo.Out curves. Image brightness and outline visibility follow
their separate linear tweens. Pointer hit rectangles follow the moving
Control, disabled exits receive no input, and reversing an interrupted entry
or exit preserves its current position. The presentation survives overlays
that retain the same room and is reset for a new room or restored session.
The original HSV shader remains an explicit material difference.

The treasure scene also contained visible legacy UI. Its banner is now the
original 654×162 component projected to 125×31, with the source label family
and size. The centered 204px single-player holder and the 60px relic icon at
2× scale have distinct 39px and 23px projected rectangles. Hover/press scales
follow the original relic control, and the actual acquired relic's complete
hover-tip model is available through pointer or keyboard focus. The permanently
displayed large title, two description lines and extra Details button are
removed. The existing detail hotkey remains available for reading. The empty
label is centered with its source sky-blue color. Original chest, collection,
holder and banner timing and acquisition guards remain in force; rare/uncommon
particle glow and the full Godot material renderer remain separate gaps.

`button_presentation_regression` checks independent source ratio projection,
focus/press timing, pulse boundaries and both complete translation curves
against the independent floating-point source equation, including reversal
and clock wrap. The card/HUD regression exercises the actually clipped moving
hit rectangles and disabled exit, with unchanged serialized game bytes.
`world_room_ui_regression` retains
its actual command, alpha, RNG and saved-byte checks at the corrected source
rectangles, and checks the real hover target and absence of the legacy Details
button. Actual ENG/ZHS resource captures are required after the frozen runtime
build; a source change or test declaration is not an executed visual pass.

## Card resource labels and title outlines

The recovered complete `NCard.UpdateEnergyCostColor`,
`UpdateStarCostColor`, `GetTitleLabelOutlineColor`, `CardCostHelper` and
`NHandCardHolder.UpdateCard` expose another legacy difference. The port used
cream-like text and an Ink outline for every resource state, then dimmed the
entire card when it could not be played. The source changes resource labels and
playable highlights; it does not gray every portrait and description.

Hand cards now use the source cream, increased-cost cyan, decreased-cost green
and insufficient-resource red, with their corresponding source outline colors.
Energy outlines use the card pool; star outlines retain the source 220/255
alpha when unmodified. Resource styles participate in both hand and small-card
cache keys. A card that is still unplayable for its star cost must refresh its
energy label when energy becomes sufficient. Unknown cards retain the source
question-mark energy icon even when the underlying model is unplayable. The
existing canonical-star-cost guard keeps a star icon visible when its price
becomes zero; that behavior was verified rather than changed.

Card titles use the source upgraded green and rarity-specific outline colors.
The whole-card inability dim is removed. No gameplay price, resource count,
command ordering or canonical State field changes. The card/HUD regression
checks real hand cache invalidation, zero star costs, exact source colors,
unknown-cost artwork and serialized State purity. Original disabled-resource
cross artwork and every secondary highlight shader remain separate gaps; these
changes are not a claim that all card materials are now identical.
