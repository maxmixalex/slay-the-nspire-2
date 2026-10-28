# Generic confirmations — UI10

Reference: the owner's pinned v0.111.0 / 41cef1ea `generic_popup`,
`vertical_popup`, `NGenericPopup`, `NVerticalPopup`, `NPopupYesNoButton`
and `NModalContainer`.

The generic instance overrides the inherited popup offsets. Its actual source
rectangle is573×659 at(528,271) in the1680×1260 UI canvas. The prior174×200
calculator panel came from an incorrect enlarged522×600 assumption. The new
renderer projects the actual rectangle uniformly to109×126 at(101,52), with
the original body rectangle, header position,180×72 button controls and
196×96 button pictures. New NSP2/37 images are sampled directly from the
original assets at those target sizes; old resource versions retain their
compatible image fallback.

Normal labels use the scene's Kreon Bold in English and the locale's Regular
in Chinese. The projected header is6px with the existing4px readability
floor; body and button labels are5px with4px fitting. The font append includes
the exact required labels, digits and both source languages. Rich colors and
line breaks remain active. Source subpixel sine movement smaller than a
calculator pixel is not represented as an invented multi-pixel oscillation.

Buttons preserve the original independent visual pivot and hit rectangle.
Focus scales to1.025 in50ms and changes saturation/value with Expo.Out;
press scales to.975 over500ms while retaining the focused outline. Unfocus
returns to rest over500ms, and release restores neutral scale/material in50ms.
The outline changes between additive gold and half-alpha black as in the
source. The image shader's relative saturation/value is applied to the
original baked base material through the existing bounded icon renderer;
this is not a claim of identical GPU rasterization.

The modal backdrop independently enters/exits over300ms toward.85 opacity.
Closing releases modal input immediately while the backdrop fades. Keyboard
badges display the actual configured Confirm/Cancel bindings. Historical
navigation and profile callbacks continue to handle quit, abandon, reset
and deletion without a second state implementation.

Short validation: app syntax passes;100,710 existing/new button, projection,
backdrop and nine-patch assertions pass. Source-equation tests cover complete
focus/press/release curves, interruption, focus preservation and clock wrap.
Matched resource, full runtime and ordinary browser checks are pending for
this source checkpoint. Neither these equations nor source availability
establish a numerical whole-game restoration percentage.
