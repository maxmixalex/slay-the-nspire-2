# UI18: original locale dates in run history and daily challenges

Reference: **v0.111.0 / 41cef1ea**. Runtime checkpoint:
`b240f41eca742429fa18f1695d0ac6b4950d1a47`, based on the UI17 source tip
`433445cf8218e7ac5d085cb4e4248d1c5393cb83`.

This closes a concrete remaining locale behavior gap. The previous history
formatter used English dates for fourteen languages and a handwritten
Simplified-Chinese exception. It ignored the date/time patterns already in
the original language packs. The normal history screen now uses those patterns
and the original culture's month forms, day periods, separators and calendar
year. The daily screen uses its own original pattern and **InvariantCulture**,
as the original call specifies. Daily timestamps, UTC boundaries, countdown,
character/modifier selection and gameplay seeds are unchanged.

## Original evidence and the distinct providers

The privately supplied `sts2.dll` has SHA-256
`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`.
No original assembly, IL, localization payload or font is committed here.

| Original method | Token | Relevant rule |
|---|---|---|
| `NRunHistory.LoadTimeDetails` | `0x0600703f` | Uses `run_history.INFO.DATE_FORMAT` / `INFO.TIME_FORMAT` with `LocManager.CultureInfo.DateTimeFormat` |
| `LocManager.CultureInfoFromThreeLetterCode` | `0x06004aad` | Maps the sixteen game language codes to explicit .NET cultures |
| `LocManager.GetCultureInfoSafe` | `0x06004aaa` | Uses `CultureInfo.GetCultureInfo`, with invariant fallback |
| `LocManager.SetLanguageInternal` | `0x06004ab5` | Assigns the selected culture when changing the game language |
| `NDailyRunScreen.InitializeDisplay` | `0x060077a2` | Calls `DateTimeOffset.ToString(pattern)` without a locale provider |
| `NDailyRunScreen..cctor` | `0x060077c0` | Reads the localized daily date/countdown patterns |

`NGame._EnterTree` sets the default thread culture and UI culture to invariant.
Consequently, a translated daily date pattern does not imply translated month
names or a Thai Buddhist year. This distinction is explicitly tested in the
normal UI paths, rather than inferred from the helper's output alone.

The optional `tools/calendar_reference_oracle` executes the unchanged original
language-to-culture method. Only the existing Sentry module initializer is
disabled in an isolated private copy, at offsets 584/585; the isolated assembly
SHA is `ad22c60604a9da57c0dfe46acb0b44f34b10679f70f79ba13e7bed5e17cba961`.
No game scene, SaveManager constructor or native Godot interface is invoked.

The recorded provider is **.NET 9.0.22 / ICU 74.2 / Ubuntu 24.04.3 LTS**, with
globalization-invariant and NLS modes both disabled. The process culture is
invariant and the oracle time zone is UTC. It uses identical UTC calendar
fields to isolate formatting from the separately retained calculator time-zone
boundary. These results do not assert byte-identical culture data on every
Windows/NLS or future ICU version.

### Example: the same calendar fields, 2026-10-24 at 14:07

| Language | Original culture | History date | History time |
|---|---|---|---|
| English | `en` | October 24, 2026 | 2:07 PM |
| Simplified Chinese | `zh-Hans` | 2026 十月 24 | 2:07 下午 |
| Traditional Chinese | `zh-Hant` | 2026 10月 24 | 2:07 下午 |
| German | `de` | 24. Oktober 2026 | 14:07 |
| Latin American Spanish | `es-419` | octubre 24, 2026 | 2:07 p.m. |
| French | `fr` | 24 octobre 2026 | 14:07 |
| Indonesian | `id` | 24 Oktober 2026 | 2.07 PM |
| Italian | `it` | 24 ottobre 2026 | 14:07 |
| Japanese | `ja` | 2026年10月24日 | 2:07 午後 |
| Korean | `ko` | 2026 10월 24 | 오후 2:07 |
| Polish | `pl` | 24 października 2026 | 14:07 |
| Brazilian Portuguese | `pt-BR` | 24 outubro, 2026 | 2:07 PM |
| Russian | `ru` | 24.10.2026 | 14:07 |
| European Spanish | `es-ES` | 24 octubre, 2026 | 2:07 p. m. |
| Thai | `th` | 24 ตุลาคม 2569 | 14:07 |
| Turkish | `tr` | 24 Ekim 2026 | 14:07 |

The space inside European-Spanish `p. m.` is U+00A0, not an ordinary breaking
space. Indonesian `:` in the custom pattern expands to the culture's `.` time
separator. Polish month names use the genitive form when a numeric day is
present; a quoted or escaped `d` does not trigger it. The supported cultures
use Gregorian calendars except Thai's `ThaiBuddhistCalendar`, whose year is
Gregorian +543 throughout the existing unsigned-32-bit Unix timestamp range.
All supplied history/daily patterns use numeric or full month forms; none
requests an abbreviated month name.

The same Thai daily challenge displays **24 October 2026**. This is the
original invariant-provider behavior, including its English month name.

## Production implementation and font completeness

`ui_calendar.hpp` retains the existing timestamp-to-calendar conversion and
adds a bounded custom-pattern formatter. `ui_calendar_locale_data.inc` contains
the small, read-only culture facts. Month sets are NUL-separated, avoiding a
dozen pointers per language. No ICU runtime, heap allocation, new image cache
or writable static table is introduced. An undersized output is empty rather
than a partial UTF-8 scalar. The supported custom-pattern subset covers every
actual history and daily pattern, plus quoted/escaped literals; it does not
claim to implement the entire .NET `DateTime` format API.

`ui_progress_ui.inc::progress_date_text` formats the real history header.
`daily_date_text` is called by `run_modes_ui.inc::daily_run_screen` and explicitly
selects the invariant provider. Each resource pattern is consumed before the
next packed-string lookup, respecting the existing bounded string cache.
The zero timestamp retains the prior unknown-value dash. Profile buttons and
Continue summaries keep their separately documented compact numeric layouts.

Dynamic dates require glyphs which are absent from otherwise complete static
localization text. `calendar_fonts_ui18.py` checks all twelve months and day
periods in all sixteen editions, at the actual **8 px Regular history role**
and **10 px daily label role**. The existing U+00A0 raster/advance passes the
European-Spanish check. Exactly fourteen glyphs are added:

- Simplified Chinese: U+5348 (`午`) at 8 px Regular.
- Japanese: U+5348 (`午`) at 8 px Regular.
- Thai: U+0E0E (`ฎ`) plus eleven complete month graphemes at 8 px Regular.

Thai already uses offline shaped graphemes throughout the resource pack. The
new dynamic words preserve that design: the logical formatter produces the
original Unicode string, then the display helper compacts only the eleven
known calendar clusters into U+10F000–U+10F00A. These fixed keys are outside both
the existing U+F0000 pack-local cluster range and U+100000–U+102340 ASCII-kerning
metadata range. Runtime conversion is in place and strictly shrinks each
replacement. No general-purpose runtime shaping engine is added.

Original fonts are extracted through the pinned PCK's `.import` remap and
binary `FontFile.data` property, with the existing strict extraction tool:

| Original font | SHA-256 |
|---|---|
| `NotoSansMonoCJKsc-Regular.otf` | `2b1dbdc50df666a00c50ecd77377e2394c6babd35a71e37505ab411ff55c7906` |
| `NotoSansCJKjp-Regular.otf` | `f18253affd968c8ea592d8df6d84b1d4c5d3a46d267e53a1f6749e6c4f76a23d` |
| `CSChatThaiUI.ttf` | `9212bbd455d4688618b5349c7adf72f904d790f89d356944d27da5d290e5a89a` |

The new glyph metrics and decoded alpha are compared with an independent
render of the same original font/grapheme. Existing **108,066 glyphs** in the
three changed editions retain their keys, metrics and alpha exactly. All old
strings, complete payload suffixes, artwork, cards, clips and IDs are also
unchanged. Thirteen locale files are byte-identical before final UI18 storage
conversion. The independent patch adds **1,175 logical bytes** and **2,665
physical NSL3 bytes**: ZHS +1,098, JPN +835, Thai +732. The common index/eight
parts remain byte-identical. The final combined storage converter measures its
own lengths again because changed shared addresses can affect compression.

`Content::has_exact_glyph` checks resident descriptor codepoint and band. It
does not allocate, decode pixels, read files or accept `?`/another-face fallback.
If an older edition cannot draw a particular dynamic date/time, the history
helper selects a readable English date/time. Old Chinese, Japanese and Thai
packs are exercised through the same actual UI and independent raster oracle.

## Bounds fix exposed by the actual streamed sanitizer gate

The initial instrumented ZHS load found an existing UI17 pointer-formation UB
in `Content::open`: `directory_ + base - directory_reduction_` first formed a
pointer beyond the 1,835,012-byte array, then subtracted back into range. The
observed virtual base was 1,915,836. The final dereferenced resident address was
valid; this was not an observed out-of-bounds write.

Both the mutable and const expressions now subtract the already validated
integer offsets before forming the pointer: `directory_ + (base - reduction)`
and `p + (base - reduction)`. Directory capacity, data layout, validation and
decoded bytes are unchanged. The original failing sanitizer journal is kept.

## Runtime and storage cost in the isolated ARM comparison

The comparison uses the exact UI17 d430 launcher as its baseline: 1,766,579
bytes, SHA `1a9a0fb8de2ba7f99a43ccbef3a21d915e72536c9eb62782634a3abdb6f83459`.
The changed App and Content translation units are compiled with the ordinary
ARM926 freestanding flags and linked with the other 27 frozen baseline objects.
This is a controlled component build; the root's final combined clean ARM
build is a separate acceptance gate.

| Isolated calendar change | Bytes |
|---|---:|
| `.text` | +3,664 |
| `.rodata` | +2,356 |
| Mutable `.data` / `.bss` | 0 / 0 |
| Contiguous execution/static allocation | +6,016 |
| Zehn relocation metadata | +572 |
| Total Zehn allocation | +6,588 |
| Compressed launcher | +4,796 |
| Default language data, independent NSL3 patch | +1,098 |
| All language data, independent NSL3 patch | +2,665 |

The component launcher is 1,771,375 bytes with total allocation 22,276,280
bytes, SHA `9ade67a9423b68bc3f476d2a2a2e443ea1144e1003f67f87414e5b96c30f396c`;
execution/static memory is 22,118,488 bytes and metadata is 157,792 bytes.
It remains within both existing build guards. Section alignment accounts for
the four-byte difference between the section-size sum and execution delta.
These figures are not
physical peak RAM, a universal fragmented-heap guarantee or a calculator FPS
measurement. Native screenshots are explicitly labeled native, not Firebird.

## Executed validation

- Original mapper/provider oracle: **16 cultures × 446 timestamps = 7,136
  rows**, covering all months, midnight/noon, leap days, year boundaries,
  1970–2106 and `UINT32_MAX`. Maximum date/time/daily output is 38/12/22 UTF-8
  bytes, below the unchanged 64/32/64-byte UI buffers.
- Formatter/shaper regression: **1,164,007 checks**, including every bounded
  output capacity around multibyte names, literal-versus-numeric day tokens,
  genitive months, separators, Thai/invariant years and in-place grapheme
  contraction. Sixty-four synthetic public vectors enable asset-free CI.
- Five focused native tests pass: calendar formatter, actual-UI test's public
  fallback mode, existing history, existing daily/run-mode and timestamp
  metadata regressions.
- Actual sixteen-language integration: **3,288,775 checks including capture
  write checks** (831,111 without exporting pictures), 445 nonzero
  timestamp rows per locale, exact glyph bands and zero resource reads from
  capability checks. Normal history/back/compendium/title/single-player/daily
  navigation passes. Date regions match the independent LCD mask/metrics
  compositor pixel for pixel; daily configuration and seed remain unchanged.
- Original unpatched ZHS/JPN/Thai editions pass **168,967 additional checks**
  for exact-metadata detection, readable fallback and actual UI raster output.
- Font extraction and the complete old/new resource preservation checks pass.
  `generate_calendar_locale_data.py` reproduces the committed read-only table
  byte for byte from the executed original-culture metadata.

- Final ASan/UBSan: the formatter/shaper's 1,164,007 checks and all sixteen
  actual streamed locale/UI/raster runs pass after the pointer-expression fix.
  Instrumentation covers the complete App and Content translation units and
  the calendar helpers; the unchanged core library remains a native build.
  LeakSanitizer is disabled for this executor. This is not presented as a
  fully instrumented build of every unchanged engine translation unit.
- Final ARM component build and both existing allocation guards pass. After
  the pointer-expression fix, Content is rebuilt and linked with the unchanged
  App object previously rebuilt at e0b27679 and the 27 frozen UI17 objects.

The following source-only journals/logs are under `build/journal` in the
calendar worktree. Final runtime gates name b240f41 and all report
`source_changed: false`.

| Journal suffix | Scope | Result |
|---|---|---|
| `ui18-calendar-original-a2d54f90227b` | Original DLL mapper and .NET providers, 16 cultures | Passed |
| `ui18-calendar-original-fonts-32a3dd24a828` | PCK FontFile extraction and font identities | Passed |
| `ui18-calendar-fonts-nsl3-1fec206256c4` | 957 required glyph keys/roles, new original rasters, preservation, NSL3 | Passed |
| `ui18-calendar-final-native-805e45bcc531` | Five focused tests, 7,136 original vectors, actual sixteen-language UI | Passed |
| `ui18-calendar-legacy-bc701cc7911a` | Old unpatched ZHS/JPN/Thai fallback at e0b27679; same calendar code | Passed |
| `ui18-calendar-final-asan-2fb05c7bdf3e` | Instrumented formatter/App/Content, actual streamed sixteen-language UI | Passed |
| `ui18-calendar-final-arm-de08e20db41f` | ARM component, section deltas and allocation guards | Passed |
| `ui18-calendar-original-66dfaea50b47` | Initial oracle C# variable-name collision | Failed, fixed |
| `ui18-calendar-asan-eeef2cc77273` | Earlier streamed-array intermediate-pointer UB | Failed, fixed |

The initial actual UI dry run also caught a fixture navigation mistake: Daily
was requested while the fixture was still in the compendium. The regression
now exercises both Back operations and the real single-player entry. This
changed the test setup, not production navigation.

### Reproduction

Asset-free checks require the existing compiler/CMake dependencies only:

```sh
cmake -S . -B build/calendar -DSPIRE_X11=OFF
cmake --build build/calendar -j2 --target calendar_locale_regression calendar_history_regression
ctest --test-dir build/calendar --output-on-failure -R '^calendar_(locale|history)_regression$'
```

For private-reference checks, copy `tools/calendar_reference_oracle` outside
the source worktree, then run its project with the original assembly directory,
the extracted sixteen-locale pattern JSON and a separate output directory.
The expected assembly hash is enforced by the tool. Its full
`calendar-original.tsv` can replace the compact fixture argument of
`calendar_locale_regression`.

```sh
build/calendar/calendar_history_regression PRIVATE/eng.nsp PRIVATE/calendar-original.tsv
build/calendar/calendar_history_regression PRIVATE/old-zhs.nsp PRIVATE/calendar-original.tsv --legacy
python3 tools/calendar_fonts_ui18.py --input PRIVATE/ui17-data --output PRIVATE/calendar-data \
  --fonts PRIVATE/original-fonts --vectors PRIVATE/calendar-original.tsv
```

All original inputs and generated resource files stay outside the source
tree. Existing old image/glyph/string bytes are verified before an output
edition is published. The root's final NSP42 conversion takes the frozen
patched editions as inputs and verifies its own final resources separately.

## Retained boundaries

The original history converts its UTC saved timestamp through the desktop
local time zone. The calculator still has its existing RTC/UTC adapter and no
desktop time-zone database. This work closes culture formatting, not that
separate time-zone capability. Offline daily challenges continue to use the
existing UTC day definition; no server-time synchronization is introduced.
Finite history capacity, compact profile/Continue timestamps, the accepted
Ancient static scenes and excluded audio/multiplayer/Timeline are unchanged.
