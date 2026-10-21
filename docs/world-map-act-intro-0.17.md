# Original map act entry — source19

Reference: the owner's pinned v0.111.0 / `41cef1ea` installation and its Godot
4.5.1 scene/runtime behavior. Original source, scene text, fonts and screenshots
remain private. This implementation adds presentation to the existing original
map and retained-room window. It adds no canonical `State` fields, save-format
bytes, gameplay random draws, resource entries or original payloads.

## Source contract

The private `NMapScreen.cs` evidence is concentrated in `SetMap` (around lines
494–521), `_Process` / scrolling (914 onward), controller input (1100 onward),
`StartOfActAnim`, `CanScroll` and cancellation (1164–1240), `Close` (1280 onward),
`Open` (1328 onward), and `IsNodeOnScreen` (1677 onward). `NMapRoom._Ready`
(around line 71) and `NActBanner.AnimateVfx` supply the independent room/banner
lifecycles. These method names and ranges identify the private audit inputs;
the repository does not reproduce their source.

| Behavior | Pinned source contract | Runtime implementation |
| --- | --- | --- |
| First ordinary act open | First act after Neow at ActFloor 1; later acts at ActFloor 0; once per installed map | Visited-root / unvisited later-map predicates; separate `played` flag |
| Normal camera | Hold 1,000 ms at Y 1,800; EXPO_IN_OUT to Y −600 over 3,000 ms | Q12 coordinates relative to −600; full 2,400-unit sweep |
| Fast-constructed camera | Hold 500 ms, sweep 1,500 ms | Durations latched when this run's map presentation is constructed |
| First-open branch | Ordinary open plays camera in Normal; Fast or top-bar open skips it if map FTUE is already seen | Current preference and `from_top_bar` select the branch; both branches add the banner |
| Interruption | Available after hold plus one quarter of the sweep: 1,750 / 875 ms | Up/down, wheel, or blank-map press cancels at the same boundary |
| Cancellation | Kill camera, clear interruptible latch, target first floor, stop dragging; 200 ms scroll guard | Separate unaccelerated running-clock guard; banner survives; camera lerps toward the target |
| Other controller input | Left/right/Select changes the target without CanScroll and without canceling the camera | Same target-only behavior; directional route selection also requires a currently visible node |
| Ordinary close | Camera and banner survive; map processes throughout its 350 ms closing tail | Close-Y tween takes precedence for 250 ms, then the earlier camera/scroll writer resumes |
| Fully closed map | MapScreen uses disabled process mode | Camera and child banners pause; reopening snaps to the current floor before the old tween can write again |
| Covering capstone | Hide Backstop; MapScreen still processes | Deck/potion/relic/compendium overlays do not pause the camera or banner |
| SetMap | Reset only the played flag | New act/variant preserves old banners, camera and interruptible latch |
| Later MapRoom | Open map, disable TopBar.Map, add another act banner | Later standalone map opens automatically, has no close button, and composites both banner instances |

The preserved interruptible latch matters: a naturally completed first camera
leaves it set. A later map installation does not clear it, so a later sweep may
be interrupted immediately. Likewise, ordinary Close and Open do not kill an
unfinished camera. These source behaviors are covered explicitly rather than
silently normalized into a simpler animation state machine.

Event Proceed now calls the same App window helper used by merchant and rest
Proceed when the retained room is already complete. This repairs repeated
Event Proceed after closing the map: the existing canonical `room_open=2`
does not need another 1→2 transition, another completion count or a new banner.

### Banner timeline

The original number, name and black band have separate tracks. They are map
screen children and do not inherit the map's ordinary open/close modulation.
Each active instance is drawn separately; the later MapRoom's two bands
therefore composite twice rather than being collapsed into one equivalent
banner record.

| Track | Start | Duration / end |
| --- | --- | --- |
| Name alpha 0→1 | 250 ms | 1,000 ms; full at 1,250 |
| Black band alpha 0→0.25 | 500 ms | 500 ms; full at 1,000 |
| Number alpha 0→1 | 500 ms | 1,000 ms; full at 1,500 |
| Number Y 450→440, QUAD_OUT | 500 ms | 1,250 ms; finished at 1,750 |
| Normal hold after longest initial track | 1,750 ms | 2,000 ms |
| Fast hold after longest initial track | 1,750 ms | 500 ms |
| Root alpha 1→0, QUAD_OUT | 3,750 / 2,250 ms | 1,000 ms; remove at 4,750 / 3,250 |

The number's explicit `From(450)` is written when its delayed property starts;
before 500 ms its authored Y remains 440. Each banner captures its own current
Fast preference. Later settings changes do not rewrite an existing banner.

## Integer implementation and source evidence

`include/world_map_act_intro.hpp` contains bounded presentation state and pure
sampling; `src/world_map_act_intro.inc` connects it to App clocks, map identity,
viewport coordinates and input. The existing map/window/render files call those
interfaces. Camera and banner elapsed time is accumulated only while the map
node is processable. Rendering multiple times at one clock does not advance it.

Godot's EXPO_IN_OUT includes unequal `.0005` and `1.0005` corrections. The
implementation preserves its tiny interior endpoint overshoots and explicit
endpoints. A 257-entry, 1,028-byte Q24 decay table with linear interpolation
replaces floating-point exponentiation. Production code uses integer arithmetic
only; unsigned 64-bit division uses the already available runtime support.
The Godot / Penner MIT attribution accompanies the equations.

Public engine evidence is the tagged [Godot 4.5.1 easing equations](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/animation/easing_equations.h),
[Tween implementation](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/animation/tween.cpp),
[Tween declarations](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/animation/tween.h),
[Node processing](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/main/node.cpp)
and [SceneTree tween processing](https://github.com/godotengine/godot/blob/4.5.1-stable/scene/main/scene_tree.cpp).
`Node::create_tween` binds the tween to the node; the default bound pause mode
checks that node's processing ability. Property tween initial values are taken
when the step starts, including after a preceding interval. Consequently, a
close/reopen during the initial hold affects the captured camera start value.

## Explicit platform boundaries

- **FTUE:** this frontend has no original profile-wide `map_select_ftue`
  persistence or original modal tutorial. Its established non-tutorial flow is
  explicitly treated as already seen. Fast therefore skips the camera; the
  original unseen-FTUE Fast exception is not claimed as implemented. No fake
  profile/save flag is introduced.
- **Loading:** canonical loads restore a settled map presentation, without
  replaying the camera or banner. For a retained room the map remains closed;
  for a saved later MapRoom it opens settled. A failed load preserves the
  current presentation. This is a stated platform lifecycle convention, not
  evidence that the original game's complete load path is identical.
- **Clock:** these times use the existing unaccelerated App running-ms clock,
  whose host delta is capped at 250 ms per `advance`. Fast does not double the
  clock. The 200 ms asynchronous source delay is represented on that same clock
  even while the map is hidden; this does not claim background wall-clock
  behavior when the application is suspended.
- **Viewport and input:** map coordinates use the existing 0.22 scale and the
  calculator's 44–205 map viewport. The sweep is applied after ordinary scroll
  clamping so its full travel is preserved. Directional node visibility uses
  the node center inside that reserved viewport. Keyboard/wheel browsing keeps
  bounded native rows; blank-map pointer dragging uses a smooth bounded target.
  Source elastic overscroll and exact desktop mouse hit propagation are not
  claimed. Left/right/Select recentering uses the source delta×15 lerp.
- **Typography:** the banner uses existing private-pack glyph sizes 12 and 20,
  localized `gameplay_ui.ACT_NUMBER` and act titles, sky-blue number and gold
  title, with 0.05 black shadows. It occupies a full-width 44-pixel band at
  Y=96; number/title anchors are Y=94/109. The original Kreon/Spectral family
  distinction and desktop font dimensions are not reproduced by the shared
  baked calculator glyph bank. No new glyphs, fonts or textures are required.
- **Capacity:** at most 16 simultaneous child banners are stored, with no
  allocation or implicit replacement of an older banner. The normal three-act
  flow needs fewer. An extra insertion beyond that explicit capacity is refused
  and is not described as unbounded original child-node behavior.

## Verification status

Before the source19 checkpoint, the pure regression passed **61,072 independent
checks**, comparing every millisecond of the 1,500 / 3,000 ms sweeps against
continuous `std::pow` equations over positive, zero and negative starting
positions. Maximum observed error was **444 / 4,096 original source pixel**
(approximately 0.1084), before projection. Separate tests cover banner equations,
hidden pause, wraparound, interruption, the retained latch, a changed initial
position during hold, duplicate banners, capacity and large elapsed input.

The changed App, pointer and UI translation units and new App test passed a
short C++17 syntax check. The first standalone sanitizer link attempt could not
find this minimal clang installation's compiler-rt runtime libraries; the
successful pre-checkpoint pure run was ordinary O1, not a claimed sanitizer run.

`world_map_act_intro_ui_regression` is registered for the frozen App build. It
uses a real title → character → default Neow acquisition → Proceed input flow,
and covers ordinary/Fast/top-bar opening, setting changes after construction,
no-render advancement, repeated redraw, pointer and controller interruption,
closing tail/pause/reopen, immediate close, capstones, failed/successful saves
and loads, clock wrap, repeat Event Proceed, later-map double compositing and
real Golden Compass map replacement. Only the route to the next act supplies
explicit room-completion outcomes; that fixture does not claim to play combat.
It optionally accepts the actual private pack and a screenshot directory.

Existing map/reticle and travel tests explicitly start their isolated route
fixtures settled and allow the original recentering lerp before the next route
confirmation. Parent integration adds actual new-run scenarios to the native /
WASM recording driver. Complete App execution, frozen sanitizer suites, actual
pack rendering and cross-target results must be recorded against source19 after
its push/readback/freeze. These changes alone do not establish 95% UI fidelity,
physical-calculator performance, or a resource package that fits its storage.
