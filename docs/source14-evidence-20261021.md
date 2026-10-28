# Source14 — original map travel and presentation

This stage continues from remotely verified source13 `a21fd47e38af598ab14cfc8aeac398dfb9abbfb2`, tree `36dbdcce1d3e21c49016584fc3dcf54db607fc0b`. The source13 branch ref, all 445 paths/modes/blob IDs and all 20 changed contents were read back at **12:21:27 Asia/Shanghai on 2026-10-21**. Main remains `efc50bc0b0cc6ca8e83461a2ff394cc7a56895e0`, and remote CI remains disabled. The source14 changes described below require their own commit/ref readback and frozen runtime gates. They are not a release or a 95% fidelity result.

## Map selection now owns a delayed controller operation

The App captures the departure's complete canonical-save fingerprint, full native seed, act, point, revision, destination and selected animation speed. During the original travel presentation, the canonical State stays at the departure point. Only `advance` can submit the destination; rendering cannot enter a room. At the deadline the App rechecks identity, reachability and the canonical fingerprint, clears the pending flag before callbacks, closes the old overlays, and invokes the ordinary `world_choose_point` controller exactly once. The saved world, encounter/event choices, charges and gameplay RNG continue to belong to that controller.

The fingerprint is the existing size plus 64-bit FNV checkpoint evidence, not a collision-proof identity or a host storage transaction. Tests independently compare complete canonical save bytes against a direct controller oracle. Successful loading, reset, run exit and a changed departure cancel the pending presentation. A failed storage probe cannot mutate the visible run or its pending route. Saving during the wait saves the actual departure; no destination or UI clock is added to State or the codec.

| Original timing | Normal | Fast |
| --- | ---: | ---: |
| Connected path traversal | 800 ms | 300 ms |
| Room fade-out, including its initial delay | 1,100 ms | 600 ms |
| Room fade-in | 800 ms | 300 ms |
| Keyboard/controller release after entry | 200 ms | 300 ms |
| Pointer release after entry | 200 ms | Immediate |

Free travel has no invented dotted edge or connected-path delay. Both modes still wait for their room fade-out. The implementation exposes the two speeds supported by this frontend; it does not invent an Instant branch.

The input distinction follows the original complete handler chain. `NInputManager` checks `InTransition` for keyboard/controller dispatch. During fade-out the transition's mouse filter remains Ignore until its tween finishes, while the selected map points have their own traveling gate. Top-bar deck, pause and map controls therefore remain available to the pointer. Opening a pure overlay cannot cancel travel. `RunManager.EnterMapPointInternal` subsequently calls `ClearScreens`, closing overlay/capstone/map screens before creating the destination room. The adapter closes its corresponding old panels, details, pause, settings and compendium at the same entry boundary. A capstone merely covering a map does not manufacture a close/open marker lifecycle.

The map UI still uses the project's existing Phase::Map presentation rather than retaining a complete hidden previous-room scene. In particular, ordinary map-window closing/opening and act-start camera presentation are not completed by this stage. The pending pointer tests verify the available frontend controls and do not imply that this remaining scene-stack difference has disappeared.

Clearing old enemy presentation happens **before** observing room entry. Clearing it afterwards would erase newly issued wake-up or other animation commands. Room-entry damage feedback also now establishes the new enemy's current visual identity before dispatching its hurt cue; an observer can have been bound before those enemies existed. The new tests cover an ordinary Mercury Hourglass entry and a real sleeping-boss wake-up boundary.

## Original circle, path, marker and transition layers

Four early circle AtlasTextures append at auxiliary IDs **1171–1174**; the existing terminal frame **1134** and selection reticle **1170** retain their IDs. All five actual resources were decoded through the production atlas path, preserving their complete **192 × 192** logical margins before fitting to **48 × 48**. No circle bitmap was redrawn or cropped to its visible ink. The diagnostic receipt is `9750ed58863560bc067139de08e73a81048e65228c877f09cc4b19818f0cb2d8`; generated images remain private.

The destination is visually traveled immediately. Its nominal 24 Hz circle frames, 500 ms alpha tween and 1,000 ms scale tween remain independent of room-entry completion. The original local seed/row-based rotation and scale consume no gameplay RNG. Dotted paths activate in order, use their original fresh-travel ink and retain the 400 ms cubic size tail. The character marker follows the original hide, horizontal appearance, upward sine and elastic return phases. Its fast-forward and already-visible behavior is preserved, including the original reopen-during-travel edge case. Restored visited circles use their terminal state instead of replaying a new selection.

An unscaled wrapping host millisecond clock drives these effects. Source13's reticle had used the legacy animation clock, which the frontend doubles in Fast mode; this stage corrects that unintended extra acceleration. The reticle retains 33 ms sampling, while the new map presentation samples its own real deadlines. Fixed-point tween interpolation, RGB565/byte-alpha quantization, the 0.22 map projection and the existing 16-pixel readable character marker are explicit calculator adaptations.

Room fade-out reproduces the authored moving black gradient plus the simple black layer. Normal and Fast retain their distinct delay/easing behavior. The gradient's 128-entry texture and source layout are evaluated as bounded integer samples; there is no new full-screen asset or floating-point runtime dependency. Independent comparison with the Godot binary32 equations found a maximum final coverage difference of one byte-alpha unit at five rounding boundaries. That is a declared pixel quantization limit, not original GPU equality.

The planned directory is **45,724 images, 1,175 auxiliary entries, 1,695 clips, 6,100 strings and 558 card models**, with a **1,345,460-byte** conservative directory bound. These are directory capacities, not a measured complete payload or verified playable-model count.

## Validation and retained failures

### Frozen source13

All **122 native build steps**, ARM packaging and WASM linking passed with the source fingerprint unchanged. The ARM launcher is **841,732 bytes**, allocation **10,421,136 bytes**, with 28,478 relocations; its SHA-256 is `edcfe6b1911044524cac59e8a80ea38fa4421b5eb784b95c692367e03eaf6835`. WASM is **4,799,333 bytes**, SHA-256 `5ad289e10fb53bd700477b14867f7cf1e76345a647b922d07314f2707a7e442c`. These are build results, not calculator execution.

The affected UI batch finished **10 passed / one failed / no timeouts**, in 212.16 seconds. The failure was the map suite's complete-frame reticle comparison. Its preceding timing fixture had moved `animation` to approximately `0x70000000`; the pixel fixture reset presentation and then rewound `animation` to 4,000, leaving `combat_linger_until` in the apparent future. The App correctly rendered its lingering combat branch, so the test was comparing the wrong screen.

A bounded diagnostic linked the same frozen source13 library, reproduced all 178 differing pixels, then moved the clock assignment before the presentation reset. All **76,800 pixels** matched the independent oracle, with 77,203 checks in that diagnostic. Source14 fixes that fixture lifecycle without weakening pixel assertions or changing the reticle renderer to satisfy it. The original failed batch remains failed; the diagnostic is not a retroactive full-suite pass. The build/UI receipt is `f7a438177df8a3e7cd46dac703a462889052e06acbe14a6de73ad4166cb99204`.

All **95 distinct Python asset/cache tests** passed on frozen source13, along with generated-table validation and the complete directory plan. One initial plan command incorrectly used an unsupported `--check` argument. Its failed journal is retained; the documented no-argument validator then passed. Formal Python receipt: `107dac8918c62cac1b12c0015ed0ff58ac7d57b0b1ebff710e4883c0bc67698d`.

### Earlier long UI checks

The source12 affected batch finished **19 passed / one character-UI timeout**, with a 2,400-second per-suite limit. The timed-out process was still consuming CPU and emitted no completed assertion result; it is not counted as passed. The source12 room suite passed 7,637 checks, blend and frontend also passed. Its final receipt is `18092f6717d40d3165d04430bff2d73cb99e84509e62fd04caebdf0f949200ce`.

The initial source10 full batch remains **44 passed / two timeouts**. Its sequential same-source recheck passed the events suite in 1,071.96 seconds and again timed out in character UI at 2,400.10 seconds. Source10 therefore has **45 distinct suites passed / one unresolved**; the final coverage receipt is `c359e4368a65dd6ec727ea0825a17a65e91982add679a12ab5f2d6107be170b4`. No results across source10, source12, source13 or source14 are combined into a fictional full-suite pass.

### Source14 short diagnostics and pending formal gates

Pure motion/raster checks passed 28,170,973 assertions including 360 complete synthetic frames; the separate transition reference passed 3,452,835 assertions. These counts include repeated formula/pixel checks, not that many independent gameplay cases. Their receipts are `4df8aeea0483174f97762cc120923759236b3ee3068f5bf66ab68e53fc2a9bb4` and `e11b5144857118d831e1b9c15128e89fc46faa6d4d9a99c8c4d5b781637c2c97`. Together with selected translation-unit syntax and the actual five-resource atlas diagnostic, these remain short source diagnostics. They do not replace linking and executing the changed App. The source14 full-App travel suite adds real keyboard/pointer routes, complete save/RNG oracles, duplicate-input rejection, speed changes during a pending route, millisecond wraparound, persistence/cancellation, Winged Boots charging, entry feedback and separate fade-in input release.

The actual-pack native/WASM driver expands from 16 to **18** scenarios. Map cases now capture intermediate circles, path/marker and fade layers, keyboard and pointer selection, a deck overlay during the pending route, Fast input-channel differences, and exact saved departure/entry states. These scenarios still need a matching complete pack and frozen source14 native/WASM runtimes. Backend equality is an internal implementation check, not an unmodified original-PC comparison.

## Rest conversion throughput and remaining work

A bounded actual Osty ACT3 dark frame compared the retained per-triangle rasterizer with the existing bounded batch rasterizer. All **279,040 RGBA bytes** and complete metadata matched. For the ten draws/992 triangles in this frame, raster CPU time changed from **0.868710 to 0.275807 seconds**; complete Python sampling CPU changed from **2.154967 to 1.759609 seconds**. Adapter compilation was measured separately and is not counted as rendering speedup. The receipt is `7018f2a04bf9cc35e48b6f532c763c9bfd88360bd1e597a1bd0e1ab377f2c49b`. This one frame does not establish a whole-period or all-actor equivalence result.

The rest-only batch wrapper keeps the original sampler and all scene/actor code, restores the temporary raster binding even on failure, and gives new batch output a distinct complete cache closure. Only a strictly checked, exact old reference closure can supply migrated pixels; an unrelated batching version cannot. All 31 short batch/cache/raster tests and independent review passed. Of 335 existing render scopes, 334 remain identical, including all 327 enemy bounds/reference/batched scopes; the new rest reference scope exactly equals the former normal rest scope. Source-ready receipt: `d7603ed9f3faa8c4b92d94fd5693b97f50df59aacf7534843148fdcaa2c17074`; closure receipt: `223169c8b9958753d65fcfbc62cdd743cca00cb1a67d7ce165d96a4f526ee43e`. Existing source13 reference conversion continues unchanged. A complete 200-frame original/reference versus batched comparison must pass after remote preservation before any new bulk job uses that path.

Complete enemy/rest conversion, latest-profile pack assembly, actual-pack parsing, the 18 native/WASM capture cases, screenshot inspection and a matching calculator launcher/content package remain required. Ordinary map-open/act-start camera effects, remaining rest/treasure effects, dynamic original GPU shader behavior and physical-device performance remain separate open work. Keep the verified 0.16 pair available until a matching new pair is actually verified.
