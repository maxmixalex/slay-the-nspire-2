# Development status — 0.4

## Fidelity baseline

The project objective is now original-game fidelity across rules, content and presentation. `docs/parity.md` is the acceptance contract. `config/reference-release.json` pins the supplied assembly/PCK hashes and inspected registry observations. `tools/reference_inventory.py` verifies these inputs and reports bounded CLI TypeDef metadata without executing the game. The content converter now also rejects mismatched reference inputs.

No card/relic/potion/character-count expansion is claimed in this iteration. The original map and seed implementation remains unfinished.

## Pointer implementation

`include/touchpad.hpp` converts absolute touchpad reports to a relative screen cursor with fractional accumulation, finger-lift reanchoring, edge clamping, physical-button transitions, short-tap detection and invalid-report cancellation. Initial held input is suppressed. The Ndless launcher also waits for 100 ms of released launch inputs so ENTER used to open the document cannot accidentally activate the title menu.

`src/pointer.cpp` dispatches pointer events through controls registered by the renderer. Press/release captures include UI context and game revision. Card targeting keeps the unique combat card ID. Modal layers clear underlying hit regions. The same dispatcher receives browser PointerEvents and X11 mouse input, including cancellation and scrolling. Numeric/directional keys remain available independently.

Existing menus and rooms gained pointer interaction. Pile inspection uses a paged 5×2 grid and enlarged card details; unordered draw inspection sorts only a presentation copy. Top-right menu and panel close controls remove keyboard-only navigation dead ends. These are small-screen interaction improvements, not completed original UI parity.

The save codec and NSP2 content format remain version 3. New interface glyphs require the new pack, but 0.3 saves remain compatible.

## Executed checks

The C++ pointer/touchpad suite passed **8,630 checks under AddressSanitizer and UndefinedBehaviorSanitizer**, including raw report transitions, repositioning, short taps, mechanical press edges, read failure/recovery, card targeting and drag cancellation, modal isolation, non-mutating hover/render, pile/details, room actions and suspended-choice saves. These are internal correctness tests, not an original-game differential suite.

ARM, WASM and CMake/X11 builds succeeded. The actual offline HTML was instantiated as an inline browser document and exercised through its mouse handlers and WASM with no page errors. Direct `file:` navigation was blocked by the environment's browser administration policy, so file-page startup and cross-session browser storage are not claimed as tested.

The final ARM build was opened from the calculator document browser in Firebird, using the owner's private CX II CAS working-image copy, OS 6.4.0.74 and Ndless r2022. Relative touchpad motion, finger-lift repositioning, physical clicks, card-then-enemy targeting, drag-to-play, pile navigation and a 120 ms short tap opening card details were exercised. The final build also saved via its pointer-operated pause menu and returned to the calculator OS. The save was then retrieved through emulated USB and loaded by the desktop codec: HP 80, energy 2, four hand cards, one discarded card, valid state.

The test frontend must not start file transfers while the foreground Ndless application is active or cancel an in-flight upstream USB queue. An earlier local harness attempt did so and hit Firebird's queue assertion after returning to the OS; this was corrected in the private harness. Final save retrieval was performed only after normal application exit.

The generated pack is **6,179,174 bytes**, with 772 images and 3,148 glyphs. It still uses the fixed six-MiB resident file buffer; this iteration does not claim streaming or a measured handheld memory peak. Physical touchpad hardware, other model revisions, actual X11 interactive mouse use and the original game have not been certified by these checks.

## Earlier release record

## Version 0.3

## Architecture changes

`src/relics.cpp` owns definitions, acquisition, ordered hooks, persistent counters and card-specific modifiers. Combat entry and turn transitions call that layer; shops and treasure no longer mutate three special bits independently. Pen Nib's attack scope covers every hit of its triggering card, not potion damage. Happy Flower's counter survives encounter boundaries and saves. Flat relic/potion block does not receive card dexterity.

`include/rich_text.hpp` parses bounded, nested presentation markup without allocation or gameplay RNG. Event text remains markup-bearing in the pack. The renderer implements color, bold, sine and jitter. The italic tag is recognized but has no slant rendering in this release.

`tools/sparse_image.py` losslessly encodes RGB565 plus alpha as scanline runs. Zero-alpha areas are omitted. A minimum-cost opaque/alpha segmentation avoids wasting run headers on tiny opaque islands. The native-size renderer skips omitted pixels and fast-paths opaque runs. Scaled sparse images are deliberately unsupported; the animation profile bakes the required size offline.

Original animation sampling increased from 216 to 706 frames across the same eleven clips, approximately 24 fps. The pack contains 772 images in total and is 6,176,194 bytes with the tested font. The six-MiB file buffer remains fixed: this release does not yet stream animation resources from flash. On-device memory peak has not been measured.

WASM uses elapsed time from requestAnimationFrame; input no longer adds animation time. X11 does not sleep a full frame interval after its rendering work. Ndless no longer intentionally renders alternate frames. Its foreground clock uses SP804 at 0x900D0000, restoring load/control on exit rather than resetting the OS's 0x900C clock.

## Executed locally

Built the provided Firebird sources with the matching gif-h submodule. The headless emulator core was used with a local frontend for key injection, USB transfer, framebuffer capture and snapshots. Its benchmark-only unlimited-turbo build flag was disabled for real-time interaction. Only private working copies of the owner's flash and snapshots were written.

Inside CX II CAS OS 6.4.0.74, Ndless r2022 displayed successful installation. The actual 0.3 Zehn launcher was then opened from the document browser. Exercises included title, native animation, combat input, relic inventory, save/load while Burning Pact awaited an exhaust selection, continuation of the two exhaust-trigger powers, return to the OS, and USB retrieval of the actual save. The retrieved save passed the native codec's structural and checksum validation.

ASan/UBSan: 1,000 generated runs, 123,071 transitions and 615,509 assertions passed, including relic acquisition/order, counter persistence, Pen Nib multi-hit/vulnerability/potion boundaries, Happy Flower encounter boundaries, flat block versus dexterity, and codec round trips. The stress driver restores HP/gold to reach more states; these are not 1,000 normal unassisted victories and not an original-game differential suite.

The native and browser WebAssembly cores matched at 13 framebuffer and stored-save checkpoints with identical actions and time steps. This exercised the module and storage callbacks; full HTML navigation was blocked by the browser administration policy in the build environment, so file-page startup and cross-session localStorage persistence were not verified.

The final converter was run again from the checked-in helpers/profile and reproduced the delivered content pack byte for byte. ARM, WASM and CMake/X11 builds succeeded locally.

The physical handheld, earlier CX/CM models, arbitrary OS builds, complete source-game behavior and original map/seed compatibility are not certified by this work. Calculator images, game assets, test saves and screenshots remain outside the public source tree.

## Next implementation boundary

Original map/seed compatibility is still open; see `map-seed.md`. Additional high-value work is true animation streaming with measured memory budgets, original-model differential tests, encounter/content expansion, and physical CX II testing. Do not infer complete relic or game coverage from the ten implemented relics.
