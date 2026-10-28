# Original relic inspection (0.17)

## Source contract

The implementation follows the pinned v0.111.0 / `41cef1ea`
`NInspectRelicScreen`, `inspect_relic_screen.tscn`, `NGoldArrowButton` and
`NHoverTipSet.SetAlignment`. The original scene's 1680 × 1260 viewport is
projected to 320 × 240 uniformly at 4/21. It replaces the old 234 × 206
generic item sheet and removes the reachable 18-item relic grid.

The popup is 165 × 165 at (78, 44), with the original reward-paper background,
58 × 58 inspection frame, 37 × 37 icon control and independent title, rarity,
description and flavor labels. Title uses native Kreon Bold at 7 pixels
(source 36, minimum 24); the 147-pixel slot covers the measured source title
set without dropping to another band. Rarity and description use 5-pixel
Kreon; flavor uses 4-pixel Bitter Medium Italic (the source Chinese italic
fallback is Source Han Serif Medium). Rich-text style and actual model
description facts are retained. Flavor's `[center]` controls its local label
alignment and is not shown as literal text. The VBox keeps complete prose;
there is no invented progress line or whole-popup scroll panel.

Rarity frame colors apply the actual six authored HSV material triples
through the source YIQ matrix. The native implementation uses bounded Q16
matrices and existing image-cache pixels, with no frame-sized persistent
buffer. Its RGB565 quantization and integer tween samples are explicit
calculator adaptations, not a claim of desktop GPU pixel equivalence. The
one-pixel separator preserves the authored fade gradient; the desktop
procedural noise mask's subpixel pattern is not reproduced.

The original source distinguishes locked models from undiscovered models.
This project's agreed scope excludes Timeline: its canonical pools and
collection include all supported relics, and `UiProgress` stores discoveries,
not Timeline unlock gates. Therefore there is no reachable locked-relic
state to substitute with “undiscovered.” The real undiscovered branch shows
the source unknown name/body, a dark silhouette and no rarity/flavor/tips;
viewing it does not set its discovery bit. This work does **not** claim to
implement Timeline's locked-model presentation.

## Interaction and timing

Collection clicks/Select open the source inspector with the current ordered
collection. HUD relic clicks and keyboard relic focus open it with the owned
instance order, including the actual instance index for dynamic text. Left
and right clamp at the list boundaries; pointer arrows disappear at the
corresponding boundary. Navigation never edits or regenerates the list.

The popup enters from 200 source pixels below its normal position over
250 ms using Cubic Out, while transparent black becomes white linearly.
Arrows wait 100 ms, then move inward by 100 source pixels over 250 ms using
Back Out. Next/previous moves the paper horizontally by 100 source pixels
over 250 ms using Expo Out. Arrow focus/press/unfocus retains the original
1.1 scale and 1.2/0.7/0.9 material value behavior.

The backdrop is pure black. Its scene-initial alpha is 1; the first Open
tweens to 0.9, while subsequent opens begin from the existing closing alpha.
As in NGame's retained inspector node, this first-open state survives run
and presentation resets; only a fresh App initialization clears it.
Backstop release or Cancel closes the modal. Clicking the paper does not
close it. Confirm and Detail do not replace the source Cancel binding.
Closing clears keyboard/backstop ownership immediately, fades paper/arrows
over 100 ms and the black backdrop over 250 ms. The paper keeps its mouse
Stop filter until that 250 ms visibility boundary; outside it, underlying
controls are already available. Captured pointer actions and already-held
underlying hotkey releases cannot activate gameplay through the inspector.
Canonical State and RNG remain untouched by rendering or inspection.

Related `HoverTipsExcludingRelic` are shown automatically at the stationary
source `HoverTipRect`: text at the right, related cards at the left. The
description is not duplicated in another relic tooltip. Collection views
use canonical model facts, rather than facts from an unrelated current run.

PotionLab and potion holders have no original inspection scene. Their
ordinary click/focus/hover behavior remains, and the old release potion
detail-modal routes have been removed. Developer potion injection remains
behind `SPIRE_DEBUG_COMMANDS`; it is not presented as an original game UI.
The normal R shortcut focuses the visible HUD relic strip; the game-over
summary does not open an invented inventory panel.

## Verification status

Source is implemented and release/debug translation units pass the short
Clang 18 syntax check. The new `relic_inspect_ui9_regression` covers actual
HUD/collection entry, ordered navigation, opaque paper hit ownership,
pointer backstop close, press/release isolation, pure State comparison,
all source tweens, source HSV equations, undiscovered behavior and related
tip placement. With an actual pack it additionally checks all displayed
model title/description/flavor glyphs against exact pack directory keys
(codepoint, actual RichReader face, size), title fit and original frame pixels,
and captures every displayed rarity plus owned/unknown/related cases.

Formal matching-source execution and bilingual screenshot review are
**pending the parent checkpoint/build**. The existing library/UI-flow
fixtures now expect source relic inspection, retain potion hover-only
expectations, and explicitly close the modal before resuming collection
navigation. No prior failed gate is relabeled as passed.

The recovered remote `1e504049` CI failure was a fixture requesting a
physical keypress for the intentionally unbound mouse/keyboard TopPanel
action. Its correction verifies both default input banks: unbound key zero
is rejected, and the real keyboard-only TopPanel binding is consumed by
the inspector. With the unchanged frozen runtime, the focused diagnostic
passed 107,949 checks. The exhaustive UI-flow diagnostic passed 6,508 after
waiting for the original 250 ms Popup mouse filter to disappear between
collection clicks. All canonical State checks remain intact. These short
diagnostics do not replace the pending matching production-pack glyph,
pixel and bilingual capture acceptance above.

Actual invocation:

```text
relic_inspect_ui9_regression <matching-ui9-eng-or-zhs.nsp> <private-capture-directory>
```

All generated screenshots and original assets remain private.

## Cookie ownership and process cache — recovered source correction

The original `YummyCookie.IconBaseName` uses its actual owner's character for
a mutable owned model. Canonical models and mutable models without an owner
share one static, randomly chosen character for the process. The prior port
used the current game's character for every context, causing collection
previews to change with the last run and history icons to use the wrong owner.

The renderer now distinguishes these contexts explicitly. HUD relics and
pickup animations use the current player; `EventModel.RelicOption` assigns
that same owner before displaying an Ancient option. `NRelicHistory` assigns
the historical player, so its icon and inspector use the saved run character.
Ordinary merchant stock, unclaimed treasure/rewards, canonical collection
models and unrelated hover-tip models have no owner. They use one process
variant, with the same variant's outline. Collection prewarming now requests
that actual outline rather than an unrelated fixed outline.

The port keeps only one additional static byte. Its first choice mixes the
optional host wall clock and the UI clock; it never calls `seed_entropy`,
uses a canonical RNG stream, or writes State/save metadata. It remains fixed
across character, profile, presentation and App resets in the same process.
This follows the source's chaotic presentation lifetime without pretending
to reproduce an unseeded original process's exact random choice.

The expanded inspector regression checks all five image identities, exactly
one clock read, zero seed-entropy calls, byte-identical canonical State,
cross-reset/process-instance stability, owned/collection/Ancient/stock owner
routes, and an historical Silent icon while the active run is Regent.
Production captures add owned Regent, canonical, and historical Silent Cookie
inspection. These new cases have passed syntax checks; matching execution
and pixel review are pending the next runtime checkpoint, not inferred from
the earlier inspector diagnostics.
