# Development status — 0.6

## Changes

`reference_act.cpp` ports the region-dependent map quotas and composes them with the existing reference constructor. The integer Gaussian sampler uses the original sine branch, nearest-even integer rounding and rejection/redraw. `reference_math.cpp` supplies portable software binary64 and restricted-domain elementary functions. Near-half-integer ambiguity fails explicitly rather than changing random call order. See `map-seed.md` for the supported inputs and reference limitations.

The preview supports custom ASCII seed entry, an on-screen keyboard, four region configurations, the increased-elite quota and visible generated quotas. Browser/X11 text input uses the same editor, while Ndless uses the touchpad or directional/confirm keys. `reference_map_ui.inc` shares the production Canvas/hit registry; `map_preview_input.cpp` handles the editor without changing gameplay.

No card, relic, potion, character or encounter expansion is claimed. The legacy expedition, its random streams and the version-3 save codec are unchanged. The preview is still not the playable original run.

## Executed local validation

- **2,000,978** ASan/UBSan math checks against host binary64, including arithmetic, square root, subnormal boundaries, rounding and domain guards. Tested maximum absolute differences versus host libm were 8.88e-16 for log and 2.22e-16 for sine, not proven global error bounds.
- **20,016** original-CIL quota/sampler replay cases, matching returned quantities, RNG consumption and final RNG words. Includes 4,663 rejection cases. The reference math adapter uses Linux libm, not the original Windows library.
- **1,000** ASan/UBSan generated maps across four regions and varying flags: graph structure and composed-versus-split pipeline results matched. This is internal integration coverage, not a new full original-map differential suite.
- **279** UI regression checks under ASan/UBSan: editing, invalid and unsupported seed input, canonicalization, regional settings, pointer actions, paging, cancellation, repeat generation, and unchanged 1,352-byte gameplay saves after preview browsing.
- ARM, WebAssembly and CMake/X11 builds succeeded. The private source-game files and test programs are outside the production tree.

## Firebird validation

Built the supplied Firebird source and matching gif-h submodule with a private key/touchpad/USB/framebuffer frontend. Only private copies of the owner's CX II CAS flash and boot image were used. Installed the supplied Ndless r2022 and launched the actual 0.6 Zehn file through the document browser.

The touchpad operated the seed editor, entered `7`, generated maps, switched through Overgrowth/Underdocks/Hive/Glory and toggled elite quota 5/8. Six stable preview captures, including the initial `NSP1RE-1` case, matched the desktop core in all 76,800 RGB565 pixels. This tests target arithmetic and rendering together, not only an x86 implementation.

An early private driver stroke exceeded the emulated pad's bounded coordinates and clicked the wrong virtual key. The driver was corrected to use central anchors and separate finger-lift strokes. A further transient comparison contained only the visible cursor because a key release occurred while synchronous generation was busy. Stable captures after generation matched; these harness issues were not reported as game parity failures. Generation remains synchronous and can delay input; no map-generation performance target is claimed.

The legacy combat regression played Feel No Pain, Dark Embrace and Burning Pact, saved while the exhaust choice was suspended, resolved a choice and loaded back. After normal exit to the calculator OS, emulated USB retrieved the **1,432-byte** save. The desktop decoder accepted HP 80, energy 0, pain 3, embrace 1, two hand cards and two pending effects. No transfer was started while the foreground game was running.

Physical handheld input and calibration, other models/OS versions, sustained frame rates and actual peak memory remain untested in this release.

## Browser and resources

The actual generated HTML/WASM was exercised as an inline Chromium document through DOM mouse/keyboard handlers, including custom seed typing, region and elite selection, legacy save and re-instantiation/continue. There were no page errors. An in-memory storage adapter was used because direct `file:` navigation is blocked by the environment's administrator policy. Do not infer file-page startup or real cross-session localStorage persistence from this check. X11 was built but its interactive text input was not exercised.

The final resident pack is **6,206,245 bytes**. Only interface glyphs were regenerated from the previous private pack; image bytes, strings and animation records were preserved. No full skeleton conversion was rerun. The final ARM launcher is **78,132 bytes**, and the Zehn packer reports **7,726,192 bytes** of static allocation. These are file/static-allocation figures, not measured runtime peak memory. The map workspace and six-MiB resident pack design remain unchanged.

Earlier verification records remain in Git history. Delivery must use actual files and the verified remote tree, not unavailable past working directories.
