# Source17: original conditional text and treasure clocks

Reference remains v0.111.0 / 41cef1ea. This stage follows source16 `9cf375893b9ddce526b6a0fdb85a4f3c11cc6b83`, tree `afeb0c2c179a1a88b6a8a724d3e5a0e3ed9388fa`, remotely read back with all **463 files** and all **45 changed blob contents** equal to the local source. Its source-only archive SHA-256 is `23609076b9036c00caa80f1f080a37506e62b7777f957d76e31a9b49368e36bd`.

## Source16 executed results

| Gate | Actual result |
| --- | --- |
| Complete native build | All 132 ASan/UBSan build steps passed |
| ARM launcher | 855,071 bytes; static allocation 10,442,840 bytes; 28,560 relocations |
| Linked ARM division/ABI | 5,249 vectors passed after correcting the optional Python dependency path |
| WASM | 4,825,157 bytes; fixed 16 MiB memory |
| Enemy suite | 47,428 checks / 174 encounter-ascension variants; 82.48 seconds |
| Core original map | 1,070 checks passed |
| Retained-room lifecycle | 287 checks, including an actual source15 v15 save loaded and re-saved byte-for-byte |
| Actual resource memory/range | 317,981 checks; all resource entries; 40 identical frames |
| Actual Ancient/event UI | 3,656 checks passed; all eight Ancient layouts inspected |
| Synthetic Ancient color oracle | 4,921 checks passed |
| Actual retained map window | 82,682 checks passed; Normal/Fast entry and retained merchant captures inspected |

The complete source16 native suite was **not** run. Its seven-suite focused map/room batch initially encountered missing `ctest`, then unsupported LeakSanitizer process enumeration. Those launch/error/interrupted journals remain retained. The corrected environment disables only LeakSanitizer, retaining AddressSanitizer and UndefinedBehaviorSanitizer. The map, travel, window, window-formula and reward suites passed, while room UI exposed its old immediate-empty-chest Proceed assertion. The remaining frontend result will be recorded separately after completion.

The actual 18-case driver failed in `world-map-fast` frame 37: the 300 ms keyboard fade-in lock had ended, but an independently opened/closed map still had 50 ms of its original 350 ms close tail. Source17 tests these two contracts independently, retaining the real keyboard/pointer events and complete save oracle. It does not shorten the runtime's close protection to satisfy the old script.

## Complete private pack

The source16 converter completed normally with unchanged frozen inputs. The independently re-decoded/re-encoded file is **231,312,495 bytes**, SHA-256 `d9adddc61c4f4f61b2ce50ee90960b112d752a694595ffa5f0feb2a6d9b204fb`. Its actual directory is **1,011,812 bytes**, and it contains **45,724 images / 1,695 clips / 11,915 glyphs / 6,100 strings / 558 cards / 1,175 auxiliary entries**.

Every retained 0.16 prefix and complete clip/glyph definition passed independent checks. Comparing every decoded image with the earlier source14 pack found precisely two changed entries, the existing Ancient button and outline. The remaining 45,722 images, all cards, strings, auxiliary IDs, clips and glyph payloads are identical. Actual screenshots confirm the formerly white Ancient buttons now use their original colors, alpha, outline and border-preserving scale.

These private files and screenshots are not in the public source repository. Resource completeness and native/WASM equality are not original-game fidelity measurements.

## New source

The bounded locale interpreter now implements the original string/boolean `cond` alternatives and successive numeric comparisons. All **19** original Chinese entries containing `cond` were inventoried. A private controlled-binding probe found **52 failures out of 76 cases** in the old interpreter and **76/76 passes** in the repair. Public tests use neutral labels while retaining the original syntax structures. Production text integration covers all five characters, prepared/owned relics, exact upgraded card names, generic empty StringVars and unknown-variable visibility. Pael's `thinky_dots` markup now produces bounded original-period motion instead of visible tags. See [localized-conditional-0.17.md](localized-conditional-0.17.md).

Treasure's fresh empty collection waits **1,000 raw ms** before Proceed or map travel; a fresh offered relic exposes Skip after **2,500 raw ms**. Original `Cmd.Wait` uses the same ordinary timer in Normal and Fast, so neither gate follows the accelerated legacy visual tick. Presentation timers are App-only and leave every canonical state/RNG field unchanged. Loading a mid-room save settles transient presentation, as already documented for this port; it is not presented as an original-game mid-chest save capability. Tests cover exact boundaries, Fast, map overlays, invalid input, save/load and clock wrap. See [world-map-window-0.17.md](world-map-window-0.17.md).

## Next gates and remaining scope

Push/read back and freeze this stage before its full native/ARM/WASM rebuilds. Run the complete native suite, actual text/Ancient/window/room regressions, final actual-pack memory/range checks as needed, and all 18 native/WASM scenarios. Reconfirm converter inputs and text glyph coverage before reusing the complete source16 pack with this runtime.

Original act-entry camera/banner presentation and documented dynamic scene particles/shaders remain open visual differences. No 95% acceptance, original-PC behavior oracle, Firebird execution or physical-calculator performance result is claimed. Main, disabled CI and the verified 0.16 delivery pair remain unchanged.
