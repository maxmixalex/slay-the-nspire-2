# UI6 shared controls, persistence and developer entry points

Reference remains the owner-provided **v0.111.0 / 41cef1ea** source and scenes.
This describes implementation; acceptance belongs to the final runtime and
resource receipts, not to a syntax check or an earlier UI5 test run.

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
`SPIRE_DEBUG_COMMANDS` flag enables a real command console on all three hosts;
see [developer commands](debug-commands-0.17.md). Production binaries omit its
transaction storage, menu, handlers and mutation exports.

## Resource extension and compatibility

Content version 23 appends the original close/target/cursor/sort/scrollbar/
banner/popup textures and bounded card-removal samples. Existing image, clip,
card and locale indices remain stable. The extension preserves old payloads
and the accepted Chinese glyph locality. Old packs retain fallback controls.
Corrected merchant clips are a separate original-atlas conversion repair; see
[rooms and merchant](rooms-ui6-0.17.md). No private art, font, assembly, PCK,
generated pack or embedded-content HTML belongs in public Git.

Remaining broader work includes event typewriter and option entry/press timing,
special ancient/fake-merchant layouts, secondary GPU particles, original pickup
trails and full input remapping. None is counted as completed by this document.
