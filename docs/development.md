# Development status — 0.7

## Delivered scope

This iteration moves from the standalone map preview to playable combat content: 14 to 24 modeled Ironclad cards, including upgrades and original extracted art/text. It adds HP-loss and energy effects, self-exhaust, captured-instance hand exhaustion, Weak and No Draw, and fixes X-cost effect queue growth. All ten hand slots remain reachable by pointer without hover moving a hidden five-card viewport. The reference map subsystem is unchanged and is still not connected to original room/encounter pools.

Card-art/string references now live in an NSP2/4 directory. Save version 4 persists the additional status fields and reads old version-3 saves. The old browser storage key remains available for fallback. The implementation inventory remains one character, ten relics, three potions and three adapted events; no new original encounter is claimed.

## Executed checks

The checked-in regression targets run without the original installation. All three CTest targets passed under AddressSanitizer and UndefinedBehaviorSanitizer. Individual runs reported:

- Core: **103,307 assertions**, including **9,885 generated transitions across 600 controlled combat scenarios**. Scenarios use increased HP to reach more states; these are not ordinary complete runs or original-game differential traces.
- Input: **59 assertions**, including 24-card paging, cancellation, stable menu capture, ten accessible hand slots and unchanged serialized state during inspection.
- Content: **224 assertions** when also given the actual delivered private pack, covering its card records/text/images and corruption rejection. The asset-free CTest variant uses a synthetic pack.

The card constructors, canonical values/keywords, upgrade methods and relevant play/status CIL were inspected directly from the hash-verified supplied assembly. The runtime regression expectations reflect that inspection. No new original-CIL execution oracle or full original combat differential suite was run in this iteration.

ARM, WebAssembly and CMake/X11 builds succeeded. The same native reader accepted the actual save retrieved from Firebird. Source packaging retains tests and the old-runtime save fixture; original installation files, disassembly, emulator images and private drivers are not included.

## Firebird

Built the supplied Firebird source with its matching gif-h submodule and a private stepping/key/touchpad/USB/framebuffer frontend. Ran the final 0.7 Zehn launcher with Ndless r2022 on a copy of the supplied CX II CAS image, OS 6.4.0.74. Original uploaded calculator images were not modified.

Ordinary practice launch, expanded deck paging and Offering's enlarged detail view were exercised with the emulated touchpad. A separately generated **controlled regression save** activated the two implemented exhaust powers and provided the new cards for focused testing. It is a test fixture, not an unassisted game run.

The controlled sequence used Offering to reach ten hand cards, then Battle Trance, Uppercut and Fiend Fire. No Draw prevented the exhaust hooks from refilling the hand; Fiend Fire exhausted the captured hand and itself. Save/load returned to the state before Fiend Fire. After normal application exit, USB retrieved the actual **1,760-byte version-4 save**: HP 74, energy 3, block 3, nine hand cards, one exhausted card, No Draw 1 and Weak 1 on the first enemy. The native decoder accepted its structure and checksum. USB was only used after returning to the calculator OS.

Physical hardware, sustained handheld frame rate and measured peak memory remain untested. Existing animation clips are reused; the new cards do not have a complete set of original per-card effects/animations.

## Browser and content

Direct `file:` navigation returned `ERR_BLOCKED_BY_ADMINISTRATOR`. The actual generated HTML/WASM was instead exercised as an inline Chromium document using its DOM keyboard handlers and an explicitly supplied in-memory storage adapter. Version-3 fallback, writing the separate version-4 key and re-instantiating with the same adapter data produced identical saved bytes, with no page errors. This does not verify actual cross-session localStorage persistence or final file-page startup. X11 was built and used for headless captures/codec checks; its interactive input was not retested.

The incremental content pack is **6,252,215 bytes**, with 782 images, 3,296 glyphs and 24 card records. It fits the existing six-MiB resident buffer. Existing image bytes and clip records are retained; ten additional 41×30 card images and refreshed localized text/glyphs come from the pinned installation. Full skeleton resampling was not rerun. Repeating the incremental conversion is checked for byte identity before delivery.

Next work remains original room/encounter binding and further card/shared-combat coverage, rather than declaring the expanded prototype an original full run. Earlier map and software-math evidence remains in Git history and `map-seed.md`.
