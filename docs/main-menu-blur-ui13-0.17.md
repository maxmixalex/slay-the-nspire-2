# Main-menu submenu blur — UI13

Reference remains the owner's pinned v0.111.0 / 41cef1ea. The earlier
frontend report explicitly retained a dark rectangle in place of the original
main-menu blur. This stage implements that missing screen filter and its
entry/exit lifecycle. Original scene/shader/assembly data stays private.

## Source behavior

`scenes/screens/main_menu.tscn` places `BlurBackstop` below `Submenus` and binds
`shaders/dark_blur.gdshader`. The scene's initial material parameters are zero.
`NMainMenu.EnableBackstop` linearly animates LOD to **3** and the black mix to
**0.7** over **250 ms**. `DisableBackstop` animates both to zero over **150 ms**;
an interrupted tween starts from its current values. The corresponding
instant methods set those same endpoints. The shader samples the screen
texture with linear mipmap filtering, then mixes its color with black.

The defaults in the shader file are not the runtime settings: blindly using
its default LOD 5 / mix 0.3 would reproduce the wrong effect.

## Calculator implementation

`MainMenuBackdrop` stores only its presentation tween. It neither enters a
canonical save nor consumes any gameplay random stream. Main-menu settings,
profiles, collections, run setup, statistics, history and bestiary use the
shared backdrop. In-run overlays retain their separate run-scene treatment.
Settings hides the covered title controls and logo; closing it restores the
title while its backdrop fades. Leaving the whole presentation resets it.

The CPU screen filter creates 160×120, 80×60 and 40×30 RGB565 mip levels from
the native 320×240 framebuffer. It uses pixel-centre UV coordinates,
clamp-to-edge bilinear sampling, interpolation between adjacent mip levels,
and the source black-mix factor. Work is bounded to **50,400 stack bytes**;
there is no additional full-frame copy, heap allocation or persistent image
cache. No new artwork, font payload or installation file is required.

RGB565 mip precision and a 320×240 framebuffer are explicit native display
adaptations. This implements the source filter at the native resolution;
it is not a claim of bit identity with a high-resolution floating GPU buffer.

## Verification

The standalone regression independently evaluates the screen-texture
equation using floating coordinates and separate mip arrays. It covers eight
LOD values, four black-mix values, all 320×240 output pixels, constant-color
preservation, edge clamping, row-stride guards, entry/exit, interrupted fades
and clock wraparound. The actual frontend gate verifies the settings-only hit
regions, both fade durations, real filtered background pixels and unchanged
canonical saves. Final executed journals and screenshots are recorded in the
UI13 acceptance manifest after the immutable checkpoint runs close.
