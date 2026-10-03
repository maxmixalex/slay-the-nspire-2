# Development status — 0.5

## Delivered implementation

The verified recovery checkpoint restored source from 0.4 and introduced the reference RNG primitives and pointer fixes. The next checkpoint adds the explicit-quota reference map constructor, an isolated interactive preview and incremental interface-glyph conversion. The complete playable expedition remains the existing bounded content subset. Original Gaussian room counts and full-run seed behavior are not implemented.

The preview uses its own RNG and scratch workspace. Opening, inspecting, regenerating and closing it does not mutate gameplay state or legacy saves. Pointer capture is cancelled by scrolling, and ten-card grids now page by ten cards rather than losing five-card wheel movements to page alignment.

## Checks executed for this release

- 8,192 generator outputs matched original initialization/next-output CIL replay. An additional 1,352,673 comparisons checked libxxhash results and host binary64 rounding, including generated boundary neighborhoods, under ASan/UBSan.
- Ten explicit-quota map cases matched restricted original-CIL replay in topology, types, ordered edges, stage call counts and final RNG state. Two cases include treasure replacement and a second boss.
- 1,000 configuration-varying maps passed ASan/UBSan and graph validity checks. This is structural stress coverage, not 1,000 original-game comparisons or completed runs.
- Pointer and map UI regression scenarios passed under ASan/UBSan: page navigation, captured-release cancellation, held-menu stability, repeat generation and unchanged serialized gameplay state after inspection.
- ARM, WebAssembly and CMake/X11 builds succeeded. The final ARM rebuild exactly matched the executable exercised in Firebird.

The map reference runner replays the original CIL through restricted runtime adapters. It is stronger than comparing two handwritten map implementations, but is not equivalent to running the unmodified original game; the adapters themselves need independent reference-run validation.

## Firebird execution

Built the uploaded upstream source with its matching gif-h submodule and a private input/USB/framebuffer frontend. The test used a copy of the owner's CX II CAS flash and the owner's supplied injected boot image, with Ndless r2022 installed. Original uploaded images were not modified or committed.

The final 0.5 launcher was opened through the OS document browser. Title/reference preview, seed change, scrolling, G toggling, normal return to the legacy game and application exit were exercised. The initial reference preview's framebuffer matched the desktop output in every RGB565 channel after undoing the hosts' different RGB-to-eight-bit expansion.

The combat regression played Feel No Pain, Dark Embrace and Burning Pact, saved while exhaust selection was suspended, completed a selection, then loaded back into the suspended state. After normal exit, emulated USB retrieved a 1,432-byte save. The desktop decoder accepted it: HP 80, energy 0, pain 3, embrace 1, two hand cards and two pending effects. USB transfers were only initiated after the foreground application exited.

## Browser and resources

The actual generated HTML/WASM ran as an inline Chromium document, with keyboard and mouse map navigation, save and reinstantiation using an in-memory storage adapter; no page errors occurred. Direct file navigation is blocked by the environment's administrator policy. File-page startup and real cross-session localStorage persistence remain unverified.

The pack is 6,190,460 bytes. This release refreshed glyphs while preserving the previous pack's images, strings and animation directory; it did not rerun full skeleton conversion. The fixed six-MiB resident pack remains. The map scratch buffer is 1,114,120 bytes, and the Zehn loader reports 7,710,156 bytes of static allocation. These are not measured peak heap/stack usage or a handheld frame-rate result.

No physical handheld, other hardware revision, full original-game behavior or default PC seed compatibility is certified. Original data, CIL dumps, test programs, saves and screenshots remain outside public production source. Earlier release details remain available in Git history.
