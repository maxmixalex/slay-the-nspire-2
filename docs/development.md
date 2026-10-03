# Development status — 0.3

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
