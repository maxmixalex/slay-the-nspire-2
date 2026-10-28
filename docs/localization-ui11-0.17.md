# UI11 original-language editions

Reference: **Slay the Spire 2 v0.111.0 / 41cef1ea**. The converter reads the
owner's pinned PCK and the font files extracted from that PCK. Original
translations, fonts and generated packs remain private.

## Installation

The normal calculator installation contains the executable, one common art
file, English and Simplified Chinese. Each other language is an optional file.
Copy the desired language file into the same folder as `nSpire2.tns` and
`nSpire2.common.tns`; no second copy of the art is needed. English and Chinese
can also be installed individually when free storage is limited.

Open **Settings → General → Language** from the main menu. The dropdown lists
only installed editions, scrolls when necessary, and remembers the selection
across launches. Language changes are disabled during a run, following the
original settings behavior. Gameplay saves and progress do not contain locale
text or glyph identifiers.

All language files must come from the same release as the common file. A
language filename alone does not constitute support: an edition includes the
actual original localized tables and all required visible glyphs.

| ID | Original code | Language | Optional filename | Bytes |
|---:|---|---|---|---:|
| 1 | eng | English | `nSpire2.eng.tns` | 1,049,523 |
| 2 | zhs | 简体中文 | `nSpire2.zhs.tns` | 5,034,485 |
| 3 | zht | 繁體中文 | `nSpire2.zht.tns` | 5,196,469 |
| 4 | deu | Deutsch | `nSpire2.deu.tns` | 1,077,603 |
| 5 | esp | Español (Latinoamérica) | `nSpire2.esp.tns` | 1,080,782 |
| 6 | fra | Français | `nSpire2.fra.tns` | 1,094,674 |
| 7 | ind | Bahasa Indonesia | `nSpire2.ind.tns` | 1,047,959 |
| 8 | ita | Italiano | `nSpire2.ita.tns` | 1,071,665 |
| 9 | jpn | 日本語 | `nSpire2.jpn.tns` | 3,900,535 |
| 10 | kor | 한국어 | `nSpire2.kor.tns` | 1,927,911 |
| 11 | pol | Polski | `nSpire2.pol.tns` | 1,027,493 |
| 12 | ptb | Português (Brasil) | `nSpire2.ptb.tns` | 1,089,906 |
| 13 | rus | Русский | `nSpire2.rus.tns` | 1,083,601 |
| 14 | spa | Español (España) | `nSpire2.spa.tns` | 1,081,108 |
| 15 | tha | ภาษาไทย | `nSpire2.tha.tns` | 1,236,853 |
| 16 | tur | Türkçe | `nSpire2.tur.tns` | 1,079,570 |

## What is localized

The converter binds the fixed 8,704 string slots to their original table keys,
including all 558 card names and base/upgraded descriptions, model descriptions,
events, Ancient dialogue, settings, run setup, compendia, history and credits.
The original English fallback is used when the pinned language table omits a
key. Thai has 1,966 missing bound keys in this release; the other non-English
editions have 91–129. These include the character table and other source
content; this port does not claim that the pinned Thai translation is complete.
The receipts identify every missing original key; the converter does not create
translations for them.

Calculator-specific instructions and historical labels without an unambiguous
original key retain the reviewed English fallback outside Simplified Chinese.
Early startup/error messages use the small built-in English/Chinese boot font,
before any optional font pack can be read. These platform messages are separate
from the original game tables. Patch-note and feedback text slots are emptied
while their historical indices remain stable.

The runtime passes the active language into every template renderer. Russian
and Polish plural forms, French/Brazilian Portuguese singular rules, localized
numeric separators, character grammatical gender and the original object and
pronoun forms are handled without changing gameplay values or consuming RNG.
The Italian Refract template also chains a numeric preview annotation into
plural selection. Energy and star formatters use bounded inline image cells. Energy amounts one
through three repeat the icon; other amounts show the number plus one icon.
The existing character-colored art is shared by all editions.

## Fonts and LCD adaptation

The font chain follows the pinned `FontManager`: Kreon/Bitter/Spectral for Latin
languages, Fira Sans Extra Condensed for Russian and Polish, the original Noto
and Source Han families for Chinese/Japanese, Gyeonggi Cheonnyeon Batang for
Korean, and CS ChatThai UI for Thai. Cross-script fallback fonts cover the
language dropdown and multinational credits.

Full body glyph bands exist at 5, 6, 8, 10 and 12 pixels; the smaller historical
bands remain for bounded secondary labels while the main LCD layouts use 8–12
pixels. Titles have additional larger bands. Glyph advances round to the
nearest pixel instead of rounding every character upward. This removes the
systematic excessive tracking of long English labels, while retaining integer
LCD positions. It does not claim full subpixel kerning equivalence with Godot.

Combining graphemes are shaped with the original font during conversion and
stored as pack-local display glyphs. This includes Thai tone/vowel stacks and
SARA AM, so the calculator needs no resident shaping engine. These glyphs never
enter gameplay saves. A narrow nonbreaking space missing from the original font
chain is represented by a blank cell derived from that face's ordinary space
advance; it is not replaced with a visible question-mark glyph.

## Reproducible private build

Run these tools from a frozen source checkpoint, with the dependencies in
`requirements.txt` and private inputs/outputs outside the public checkout:

```sh
python tools/build_locale_editions.py \
  --input /private/ui11-eng.nsp \
  --pck /private/SlayTheSpire2.pck \
  --fonts /private/original-fonts \
  --output /private/locale-editions \
  --languages all
```

If the common art is subsequently optimized, preserve the already rendered
glyphs and translated text:

```sh
python tools/rebase_locale_editions.py \
  --artwork /private/final-ui11-eng.nsp \
  --editions /private/locale-editions \
  --output /private/final-editions
```

Both tools preserve model/image IDs and animation timing. The rebase compares
every glyph's key, dimensions, alpha payload and advance and every text byte.
Each NSL2 edition is reconstructed against the common file and compared with
its complete logical pack. Per-language receipts record byte sizes, hashes,
missing original keys, platform fallback slots and glyph coverage.

`locale_editions_regression` accepts the final editions directory and exercises
the production split-resource loader, model/event text routes, font loading and
inline sprite rendering. Its native results establish data and formatting
correctness; calculator speed and interaction acceptance require Firebird.

## Final resource size and verification

The shared art file is **61,186,168 bytes**. English and Simplified Chinese
together add **6,084,008 bytes**, for **67,270,176 bytes** before the launcher.
Optional editions share that exact art file; installing all sixteen languages
is not the default calculator installation.

The final art rebase completed under checkpoint
`be92f62d2a8f9a386ad5895fd84812217590c173` with journal
`ui11-rebase-final-art-sixteen-locales-g-69c96a2ed77b.json`. Every locale has
zero missing visible font characters, and every font/text byte and split
stream reconstructed exactly. Glyph counts range from 5,664 to 45,231, and
the largest resource directory stays below the 2 MiB loader limit.

The actual runtime pass at checkpoint
`4e4ce0692a01132de613c01e3cb6a8a56f0d001c` completes **38,644 checks over
53,072 model/event descriptions with zero failures**. All sixteen editions
report zero unexpanded templates. It checks every visible codepoint at
8/10/12 px and each character's energy sprite through normal, rich, scaled
and clipped profile text. The final resource directory is used directly;
this result is not inferred from the successful resource conversion.

The closed native receipt is
`ui11-sixteen-final-locale-runtime-i-4c191c83846c.json`; its matching Clang
build receipt is `ui11-clang-locale-room-build-i-943d0506436d.json`. Both
record `source_changed=false`. This establishes text/font data correctness;
it is separate from Firebird interaction and performance acceptance.
