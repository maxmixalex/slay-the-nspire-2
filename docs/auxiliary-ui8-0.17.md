# Original auxiliary screens and bounded text

Pinned reference: **v0.111.0 / 41cef1ea**. This document distinguishes
implemented source behavior from executed acceptance. Stage 5 compiled all
native targets and passed 91 of 96 suites; its five failures are retained in
the journal. The auxiliary input correction was frozen in stage 5c
(`44b4999659121ef6418e58c7711e2eb110f5b16a`), whose focused native gate and
actual English/Chinese auxiliary gates both passed. Notes, credits and
feedback captures were reviewed individually. The later Latin-wrap and
BBCode corrections still require stage 6 execution.

## Working entry points

The General settings list contains the original Send Feedback and Credits
rows. Dismissing either reader returns to the same settings position. The
patch-note icon sits in the main menu's upper-right corner; the original
Confirm hotkey opens/closes it on release. Left/Right and Back in that reader
also follow release, and covered menus receive no click or gameplay command.

`NPatchNotesScreen`, `NPatchNotesButton`, `NSettingsScreen`,
`NSendFeedbackScreen`, `NCreditsScreen`, `NScrollableContainer` and the
matching original scenes establish these controls. The source's startup
language selector is not restored by these additions.

## Complete original text in private resources

The converter enumerates all **126** pinned English patch notes, newest first.
Chinese uses its original localized note when present and the original English
fallback otherwise. The complete articles occupy **130** bounded string
slices; UTF-8 splits preserve every original byte. Only filenames, dates,
slice counts and indices enter the public source repository. Each slice stays
below the existing 16 KiB runtime string limit. The reader measures one
article on entry and renders only its visible slices, with wheel, content drag
and scrollbar-thumb controls. It does not allocate a whole-article buffer.

Credits consume all 64 localized source keys. The display follows the actual
scene order, including Italian before Indonesian, the trailer animation
heading before its team, and each final tool logo before its attribution.
Authored header heights project to 46/31/23/15 pixels. Only Localization and
Trailer headers select the scene's explicit KreonBold face and green color;
other headings use KreonRegular. Roles and names retain two columns, multiple
voice roles retain the empty name cells, modders use one shuffled column, and
playtesters use three. Shuffling uses only a local presentation generator.
The source malformed Chinese single-separator credit row is skipped by the
same two-column rule. No missing-name repair is invented.

Credits retain 80 source pixels/second autoscroll, 50-pixel wheel steps,
20-per-second position smoothing, a two-second fade/exit gate and one-second
exit fade. All translated credits and attributions can wrap at the calculator's
readable minimum instead of being silently cut off. Content remains private.

## Feedback form and actual local export

The panel, five original emoji images, dropdown, Sun, Cupcake, Flower and
hover/success variants come from the original feedback scene. Category count
is three for English and four with Translation for Chinese. The field accepts
literal UTF-8, newlines, insertion/deletion at the caret, Home/End and vertical
navigation. Its original **8,000 UTF-16-unit** limit distinguishes a BMP
character from a supplementary emoji. The draft survives reopening within
60 seconds; later reopening clears it. The draft buffer is 24,001 bytes and
the entire auxiliary state stays below 25 KiB. No second framebuffer is added.

This port exports a **local report**. The UI explicitly says Export and local
report; it never sends a request to the original feedback service or reports
an external submission as successful. The report includes category, selected
emoji, port/reference version, language/profile, basic run location and the
literal draft. Failure retains the draft and shows a retryable failure state.

X11 writes `nSpire2-feedback.txt` beside the save, scoped to the selected
profile; Ndless writes `nSpire2-feedback.txt.tns` with the same profile suffix.
Both use temporary files and atomic replacement; Ndless preserves/restores
the old report if its rename fails. No run, preferences or history file is
repurposed. Web downloads `nSpire2-feedback.txt` using Blob and consumes the
existing foreground SaveData transfer buffer immediately after input. It
adds optional exports while retaining the established three-import WASM ABI.

X11's XIM path and Web's composition/paste path enter the actual field.
Calculator letters/numbers use the existing matrix adapter; Shift+4/6/8/2
moves the caret without consuming the ordinary digit keys. Tab leaves the
field for category navigation. Keyboard rebinding capture remains higher
priority than text entry on every host.

## Font coverage, storage and limitations

Body text is 8 px, section/date text 10 px, and short feedback labels/counters
6 px. These readable minimums are explicit small-screen adaptations. New
codepoints introduced only by the historical notes/credits are baked at
8/10 px rather than all UI sizes. Patch-note bold text explicitly maps to
the original KreonBold family; ordinary font styling is unchanged elsewhere.

The converter accepts an ordered chain of real fallback fonts and records
which font supplies each codepoint. Original Kreon covers the Latin credit
letters missing from the Chinese font. The official Google Fonts Noto Emoji
and Noto Sans Symbols2 fonts supply symbols absent from the original PCK
fonts; their complete OFL 1.1 notices are included in both distribution
formats. Emoji are monochrome font glyphs, not recreated pictures. Original
Godot color-emoji output is not claimed to be pixel-identical.

Unicode variation selectors and non-rendering format controls remain in the
stored text but consume no independent glyph or advance. The runtime does
not turn VS16 into a question mark, perform a glyph read for it, or add a
visible-character reveal slot. The offline converter reports these controls
separately and refuses a replacement with uncovered newly localized glyphs.

The stage 5c resource tail is **2013 auxiliary entries**. The 17 auxiliary-screen
images occupy 1996–2012, including the shared original keyboard keycap at
2012. The 147 new localized entries use existing reserved slots 8507–8653;
the total 8704-string contract does not grow. Image pixels, original fonts,
credits and articles stay in private owner resources.

## Verification boundaries

`auxiliary_ui8_regression` exercises all note indices and multiple slices,
released shortcuts, content/thumb scrolling, covered-menu isolation, credits
timing, both settings entries/return positions, UTF-8 caret editing, the
UTF-16 limit, retention/expiry, export success/failure and byte-identical
game saves. It can use a synthetic public pack or the actual private pack
and emit screenshots. The Ndless profile suite now exercises the exact
report writer, short writes, rename rollback and profile isolation.

The font generator's nine synthetic tests passed, including
fallback priority, auxiliary-only sizes and format-control exclusion. C++
format-control tests additionally assert zero reads/advance and no visible
replacement glyph. The stage 5c native and actual-resource gates exercised
these paths successfully. Their receipts are `ui8-stage5c-aux-native-8f1713345d71`,
`ui8-stage5c-aux-eng-0396553cd68d` and `ui8-stage5c-aux-zhs-d5fd7bfee33b`.

The stage 5 native run exposed a real input-order defect: after switching an
article, a wheel event before the next rendered frame used an unconfigured
scroll range and was lost. Layout is now measured once when an article is
opened or changed, before accepting scroll input. The existing no-render
between next-article and wheel regression is retained.

Actual Chinese captures also exposed English fallback articles breaking
Latin words at individual letters. Patch-note measurement and painting now
use word-aware wrapping irrespective of the selected UI language. This is
the original rich-text wrapping behavior, and leaves the stored article
bytes unchanged. A separate excluded benchmark target measures seven actual
auxiliary scenes, stationary render time, provider reads, whole-frame hashes
and canonical-save preservation. Its first measured run awaits the next
immutable runtime checkpoint; no rendering speedup is asserted from its
existence.
