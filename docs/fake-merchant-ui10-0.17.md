# Fake Merchant room speech — UI10 / NSP2/37

Reference remains v0.111.0 /41cef1ea. This stage follows the preserved room,
font and repeated-attack checkpoint `3d6d79907bb76269474acd8bd5c27baf575837f2`.
It restores the missing visible welcome speech when entering Fake Merchant.
It does not replace the separate inventory rug dialogue or claim an original
desktop screenshot match.

## Source behavior and implementation

The inspected private `NFakeMerchant.AfterRoomIsLoaded` starts
`ShowWelcomeDialogue` unless the event has already started its fight. The
method chooses among four welcome lines through cosmetic `Rng.Chaotic`, waits
750ms, then calls `NMerchantButton.PlayDialogue` with4 seconds. Normal merchant
has no welcome lines in the pinned localization; it does not get this bubble.

`NMerchantButton` creates `NSpeechBubbleVfx` with Right side and Blue color.
Its original global anchor is the merchant button's left position minus its
330px width. At the established1680→320 UI projection, the speech root is
`(684,435)*4/21`, and the settled bubble center is `(591,387)*4/21`. The text
control projects from `(451,303,280,130)` to approximately `(86,58,53,25)`.
The original24px text uses the existing5→4px compact fitter and Regular locale
substitution. Existing fake-merchant font requirements cover every line.

The presentation stores only an8-byte start/line pair in the merchant view.
It chooses a line once, using the separate cosmetic stream, and starts the
visible effect after750ms. Entry alpha and visible-character ratio last400ms;
the bubble scales from one third of its settled size over500ms with EXPO_OUT;
the root rotates from7 degrees to zero over300ms with SINE_OUT. After the
500ms parallel entry and3 second hold, the root fades to transparent black
over400ms with SINE_OUT. The scene's small sinusoidal motion remains separate.

`RichTextFlyIn` uses its inspected per-character clock, alpha, QuadOut
translation and20-degree local rotation. The glyphs and bubble are projected
directly to the existing canvas; there is no temporary raster, extra pixel
pool, glyph atlas or per-frame allocation. Both ordinary and RLE glyph records
are supported. The welcome belongs to the room, so inventory open/close and
purchase speech do not restart or overwrite it. It is drawn before the
inventory's dim/backdrop, following the scene child order. Room presentation
reset clears it, and the completed/fighting event path does not paint it.

## Original artwork and compatibility

Neither existing speech image is interchangeable with this effect. Crystal
dialogue uses a different material and180-degree rotation; the merchant rug
also uses another material and180-degree rotation. The appended image uses
the original420×285 `speech_bubble3` with the inspected Blue parameters
`h=.05,s=1.3,v=.55`, the source shader's YIQ hue transform, Right-side horizontal
flip, and `.75*4/21` projection to60×41px. Its initial private build encodes to
2712 bytes. Live root rotation and scaling are not baked into another sequence.

`tools/fake_merchant_ui10_assets.py` accepts the complete36 pack and emits37.
It adds six images: auxiliary2061 (`I10_FakeMerchantWelcome`) and the generic
popup's five source-sized images at2062–2066, bringing the auxiliary count
to2067; card, clip, glyph and string counts are unchanged. The popup panel is
109×126 and its cancel/confirm controls and outlines are37×18. Each is sampled
directly from the original atlas with the existing authored material, avoiding
a second resampling of the old174×200 panel/60×24 buttons. The parent owns the
corresponding popup layout and interaction changes. All six image payloads
together encode to17426 bytes in the initial private extraction. The
shared append routine preserves every old payload, relative retained/grouped
descriptor, tile group, frame, index, glyph metric and text byte, with explicit
relocation/readback checks. RGR2 remains a version36 capability and is valid
inside37. The conversion order is35 scene fonts,36 retained-color compaction,
then37 room-art append. All older supported packs keep their existing reader
paths and catalog boundaries. Old packs without the appended image remain
usable and do not substitute an incorrect speech texture.

## Checks already executed

Short checks compiled the changed App/input/content translation units against
the preserved stage-C core. The synthetic event regression passes5135 checks,
including welcome delay, actual room interaction, inventory independence,
scalar sine fade samples, expiry, reset and unchanged canonical saves. The
changed retained-content regression passes3655707 checks, including both RGR2
color modes under36 and37, memory/stream reads and the complete37 auxiliary
catalog. Tile compatibility passes14260750 checks, boot probing619 and the
split adapter6642. These are runtime correctness checks, not original-game
visual similarity percentages.

The synthetic Python append test passes with a real RGR2 group and grouped
glyph/text tails: every old image, glyph alpha and string is independently
decoded before/after the six-image append. The split test covers17 through37
and exact reconstruction. The pinned private source image extraction succeeds
at60×41, without publishing any original bytes. The new app/content/input and
changed native tests pass Clang syntax/short compilation.

The final37 checkpoint, actual bilingual append, resource-specific captures,
full native suite and target builds remain the parent's next verification
step. Fixed-point/subpixel rounding, software glyph rasterization and the
existing cropped Fake Merchant background remain limitations; this stage does
not assert GPU pixel identity, add missing scene particles or restore unrelated
character/background animation. The concrete delivered feature is the room's
previously absent timed blue welcome, with its own source lifecycle.
