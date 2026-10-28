# UI6 shared controls, persistence and developer entry points

Reference remains the owner-provided **v0.111.0 / 41cef1ea** source and scenes.
This describes implementation; acceptance belongs to the final runtime and
resource receipts, not to a syntax check or an earlier UI5 test run.
The final receipts and their distinct scopes are summarized below and in the
[runtime acceptance record](validation-ui6-runtime-0.17.json).

## Original controls and readable dimensions

The close glyph comes from the original compressed atlas `back_button_x`,
separate from the navigation arrow. Shared controls preserve both width and
height constraints, including authored transparent margins. Character confirm
retains its original icon-only cream tick. Proceed labels are fitted inside the
arrow's label area instead of stretching the entire sprite to fill a hit box.
Pointer targets remain readable at 320 by 240.

The quit/abandon dialog uses the original vertical popup and its own cancel and
confirm textures. It no longer reuses a navigation Back button for cancellation.
`NBackButton` has its own motion family: 50 ms linear focus, 250 ms press with
scale returning to 1 (Expo.Out) and color/outline using Cubic.Out. Its source
unfocus scale remains 1.05. This is separate from generic action controls.

The topmost settings/confirmation/help overlay owns pointer focus. Rendering
and stale-hit pointer motion cannot change covered pause/title/settings hover
selection. Explicit Details/Read controls remain usable while repeated I/V
suffixes and decorative instruction strings have been removed from card,
collection, relic, selection and reward surfaces. The dedicated controls page
retains actual keyboard instructions, including aiming and independent piles.

## Settings survive restart

The implemented Fast Mode preference uses a separate, bounded eight-byte
`NSPF` record, version 1. Invalid, truncated, overlong or failed reads restore
the default. Failed writes are reported; changing the preference never mutates
the run State or its historical save representation.

- Browser: separate `nSpire2-preferences-v1` localStorage key, shared across
  languages. The host acknowledges successful writes explicitly.
- Calculator: `nSpire2.settings.tns`, using temporary/backup replacement.
- Linux: the same record, with optional `--settings PATH`; headless execution
  writes preferences only when explicitly given a path.

This persists the setting currently implemented. It is not a claim that every
original audio, video or input option exists on the calculator.

## Startup and optional debugging

The custom loading illustration and percentage have been removed. Required
resource reads and combat preparation remain bounded; room fade-in starts
after its first required scene is ready. Actual load errors and the bilingual
language chooser remain available. There are no new full-file integrity scans.

Old practice and reference-map entry points are absent from production menus
and hotkeys. Their historical core/save support is retained. A default-off
`SPIRE_DEBUG_COMMANDS` flag enables developer commands on all three hosts.
The calculator offers fourteen presets and left/right model enumeration;
browser/Linux additionally support free-text commands. The calculator's
`SPIRE_DEBUG_TEXT_INPUT=0` removes the unavailable I editor and its clickable
region. See [developer commands](debug-commands-0.17.md). Production binaries omit its
transaction storage, menu, handlers and mutation exports.

## Resource extension and compatibility

Content version 23 appends the original close/target/cursor/sort/scrollbar/
banner/popup textures and bounded card-removal samples. Existing image, clip,
card and locale indices remain stable. The extension preserves old payloads
and the accepted Chinese glyph locality. Old packs retain fallback controls.
Corrected merchant clips are a separate original-atlas conversion repair; see
[rooms and merchant](rooms-ui6-0.17.md). No private art, font, assembly, PCK,
generated pack or embedded-content HTML belongs in public Git.

New port-authored labels have a small compiled bilingual fallback catalog.
This keeps every one of the historical 935 sorted UI-string slots stable;
inserting new labels into that old table would silently mistranslate existing
packs. Both generation and runtime fallback are checked. The final Chinese
pack already contains every glyph needed by the new labels at 8/10/12/16 px.

## Final runtime receipts

The accepted runtime is `96f7b5c6914762fad998f03ab5da1b4c283ecaa0`, tree
`579718ea9f6c5c6aaf21dcffbedf802686760da1`. The final v23 resource conversion and
lossless compaction remain tied to `20262f09eb1edab0035e632ca9ee4f065fb51fc4`;
later runtime fixes do not require reconverting those packs. Final ENG/ZHS
sizes, SHA-256 values, split-resource reconstruction, portable receipts and the
three delivery files are recorded in the [runtime JSON](validation-ui6-runtime-0.17.json).
The delivered files are the bilingual calculator ZIP, ordinary offline HTML and
separate debug offline HTML. Their metadata comes from the final three-entry
delivery manifest; private storage identifiers are not published.

| Gate | Executed result and boundary |
| --- | --- |
| Final local Release | Clang 18, ordinary runtime: all **77/77** registered suites passed; journal `ui6-native8-ctest-f36d8c3c05a5`. |
| Final GitHub Actions | [Run 40073427331](https://github.com/maxmixalex/slay-the-nspire-2/actions/runs/40073427331), associated with the accepted runtime, has three successful jobs: native, conversion and portable. Native includes the complete **77-suite ASan/UBSan** gate and separate Release debug command/menu checks with text capability both on and off. CI uses public fixtures without private game assets. |
| Local sanitizer follow-up | The earlier full stage-6 run remains **76/77**, failed on signed gear-angle overflow. Stage 8 passed the repaired HUD test with synthetic, actual ENG and actual ZHS resources. These three local profiles are not described as a complete local stage-8 sanitizer rerun; the complete final gate is the separate CI run above. |
| Debug capabilities | Independent stage-7 Clang builds passed three text-capability CTest cases and the menu-only interaction case, plus actual ENG/ZHS UI flows in each mode. The final HUD arithmetic change leaves that core/menu code unchanged; final CI also reruns both capability configurations. |
| Portable runtimes | Ordinary and debug ARM and WASM builds all passed at the final runtime. Independently checked ARM allocation headers are 16,445,872 bytes ordinary and 16,662,860 bytes debug, both below 16 MiB. This is build/allocation evidence. |
| Real offline browser | Final isolated Chromium acceptance passed both languages, real settings persistence, Save/Load/Continue, language confirmation and cancellation, production debug-API exclusion and the separate debug F1 text/room/act flows. It reported no page errors or external requests; journal `ui6-browser8-16ff711d7d64`. |
| Native/WASM agreement | Final runtime and packs passed **64 language scenarios, 2,376 exact RGB565 frames and 106 exact serialized saves**, documented in [the backend record](validation-ui6-backends-0.17.json). This is agreement between this project's implementations, not original-PC pixel equivalence or browser-storage validation. |
| Firebird execution | The final ordinary runtime passed OS/Documents launch, Continue, restored Fast Mode, Save/Exit, return to the OS and exact save/preference readback. Final debug keyboard targeting, kill-to-loot, shop and reload were observed in a separate wrapper that later timed out; its complete exit is not claimed. [The dedicated receipt](validation-ui6-firebird-0.17.json) retains those scopes and the separate installer failure. |

The final HUD correction retains the original gear period and angle while using
bounded unsigned multiplication, a completed-hold branch before multiplication,
and whole-turn reduction before the Q12 sine conversion. The earlier failed
sanitizer journal is retained. The previous backend discrepancy is also retained:
ten deck-button frames differed when an App reused an old address/generation.
Both initialization paths now clear static HUD motion; the final backend suite
reran every frame and save without a tolerance. Earlier geometry/reward-oracle
failures and the debug test artifact's missing execution permission remain
identified in the [combat report](combat-ui6-0.17.md), rather than being relabeled
as passes.

## Measured performance and next work

The [quiet host comparison](ui6-card-performance-0.17.md) uses the same driver
from `e9d4017629219a28f072fb54bcb0327fb538f121` against accepted UI5 and UI6
runtimes with their corresponding packs. It repeats a complete serial 12-run
matrix after excluding the earlier tooltip-contaminated idle fixture. The
driver correction does not change production runtime or resources.

Measured warm merchant inventory time is **18.8–19.6% lower**, and the hovered
window is **23.8–27.8% lower**. The room-open/setup/first-render cost rises by
about 12% in the idle fixture, while warm ranged resource reads rise from about
4.0 KB to 5.1 KB per frame. Warmed details remain zero-I/O in both versions and
show modest timing differences in either direction. These are whole-version,
unpaced host measurements, without a claim of physical-calculator FPS or an
isolated cache gain. Per-layer merchant resource grouping and retention are
concrete follow-ups; the full values and limitations are in the linked report.

Remaining UI work includes continuous grid scrolling and its draggable thumb,
all-target/ally reticles, the deck-count gain pulse, event typewriter and option
entry/press timing, special Ancient/fake-merchant layouts, original overkill and
blocked-hit result presentation, secondary GPU particles and pickup trails,
rare viewport-overflow formations, and full input remapping. Merchant opening
tweens, dialogue and hand tracking remain unimplemented; the card-removal
presentation bridge also does not cover every direct/forced removal path.
The map legend item also still returns from its enlarged focus size immediately
instead of following the original 500 ms unfocus tween. Settings persist
the currently implemented Fast Mode preference; this is not completion of the
original settings catalog. The [card](cards-ui6-0.17.md),
[combat](combat-ui6-0.17.md) and [room](rooms-ui6-0.17.md) reports retain the
specific source references and bounded 320×240 adaptations. These remaining
differences are not counted as completed by the successful gates above.
