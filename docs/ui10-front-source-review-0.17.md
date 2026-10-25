# UI10 front-end source review

Pinned reference: v0.111.0 / 41cef1ea. This is a source implementation record,
not a release receipt or a measured restoration percentage. Original scenes,
assemblies, font files, converted packs and captures remain private.

## Locale font behavior

The scene's authored font is not the final Chinese font. `MegaLabel._Ready`
calls `RefreshFont`, which selects `FontType.Regular` through
`FontControlUtils.ApplyLocaleFontSubstitution`. `FontManager` substitutes the
listed non-English languages, including `zhs`, but does not substitute `eng`.
`ZhsFontPathSet` maps Regular to Noto Sans Mono CJK, Bold to Source Han Serif
Bold, and Italic to Source Han Serif Medium. `MegaRichTextLabel.RefreshFont`
substitutes its normal, bold and italic roles independently.

`source_label_face` implements only that normal-role decision. Explicit rich
bold/italic spans and business-method font overrides must be checked
separately. A blanket conversion of every bold face would be wrong. In
particular, the English relic category scene assigns its bold span the same
Kreon Regular font as its normal text; the Chinese runtime replaces that bold
role with Source Han Serif Bold. Potion category bold spans use Bold in both
languages. Ordinary profile, library, Bestiary, character-selection and relic
inspection labels now apply the normal-role substitution at their real call
sites. Character-name source tracking is retained only for English, because
substituting the font also replaces the authored font variation.

`config/profile-ui-fonts.json` now describes these front-end roles, despite
retaining its original filename. It includes existing localization key
references, size/face requirements, locale-specific dynamic calendar alphabets
and the full relic title/flavor key set. It contains no original prose.
Exact glyph coverage is a conversion requirement; a fallback glyph is not a
passing result.

## Implemented source contracts

| Surface | Concrete change | Verification target |
| --- | --- | --- |
| Profiles | 7px regular header; 6px slot title; 5px description/delete/menu title; source rectangles, panel/icon/text parent scaling, delete label cubic-out motion, menu-specific 50/300ms easing; keyboard navigation skips hidden delete buttons. | `profiles_ui7_regression` persistence/input checks and exact-font scalar pixel oracle. |
| Profile helper | Authored 18px helper projects to3px; the port deliberately keeps4px for calculator readability. Mouse-only visibility is restored. | Actual locale pack and pointer/keyboard composed review. |
| Profile/history calendar | Chinese month names and twelve-hour period labels follow the pinned formats. Profiles put the period before the clock; history puts it after. The existing UTC host boundary is unchanged. | Profile pixel oracle and exact dynamic glyph requirements at5px/8px. |
| Compendium | Source5px titles/descriptions, true multiline descriptions, title outline/shadow, icon rectangles and source pivot-based parent scaling; short-button press scale is0.98 without changing long-menu defaults. | Profile regression additionally checks six buttons at rest and full hover against an independent text/transform oracle. |
| Statistics | Source container minimums produce a1096px content column at sourceX227; two256x90 tabs, source section backgrounds,5px entries,7px headers,6px tabs and5px character names replace the enlarged generic layout. | `statistics_ui10_regression` source geometry, dynamic row layout and actual packed-font pixel checks. |
| Statistics conditions | Missing fastest victories and zero ascensions leave their original rows hidden; single visible rows remain vertically centered. Original character colors and red character-ascension prefix are restored. | Empty/full metadata cases, single/pair label pixel cases and saturated counter fitting. |
| Statistics interaction | Twenty-four bounded entry motions, initial first-entry focus, directional navigation, focus-driven scrolling, original Playtime/Ascension hover text, corrected scrollbar drag geometry. UI state is292bytes and stays outside canonical saves/RNG. | Input/scroll, endpoint checks, captures and canonical-save identity. |
| Card Library | The count follows the original centered rich-text markup. Sort/filter/cost/check labels select each locale's actual normal face. | `library_ui7_regression` independent sidebar/count oracle. |
| Relic/potion collections | Rich role substitution fixes Chinese relic headings while keeping English's authored regular-bold mapping. | All category variants in the collection scalar pixel oracle. |
| Bestiary | Normal-role locale substitution covers the large name, category dividers, Actions/Stats and mode labels; actual rich italic dialogue retains its separate role. | `bestiary_ui7_regression` exact glyph coverage, actions, layouts and captures. |
| Character selection | Ordinary name/stats/relic-title roles use the actual locale font; name measurement and glyph drawing agree, including tracking. | `five_character_ui_regression` and full actual-language review. |
| Relic inspection | Chinese title and normal flavor text follow runtime font substitution; rich body bold/italic stays explicit. | `relic_inspect_ui9_regression` checks every actual relic text field and exact glyph key. |
| Pause | Header and ordinary button labels follow the same source locale rule. | `world_frontend_ui_regression` plus final actual-language captures. |

The statistics layout follows `stats_screen.tscn`, `stats_screen_section.tscn`,
`character_stats.tscn`, `NStatsScreen`, `NStatEntry`, `NGeneralStatsGrid` and
`NCharacterStats`. Minimum entry heights are88/92 source pixels; two-column
character rows use the taller panel. The source scroll container overrides Y
but preserves the centered oversized margin-container X position. No prior
metadata is invented for excluded Timeline/platform achievements.

## Remaining concrete differences and next implementation steps

| Surface | Remaining difference or unverified scope | Next action |
| --- | --- | --- |
| Statistics mask | The source scroll mask has alpha ramps at12.5–15% and97.5–100% of viewport height. This implementation clips at the top boundary but does not reproduce those per-row alpha ramps. | Add exact compositing using bounded existing scratch storage; verify scrolled labels at both edges. |
| Compendium press | Original press modulates the whole control toward light gray. The port's panel state changes, but icon/text parent press modulation remains separate work. | Apply the original parent modulation through the same bounded transition and add press/release oracle states. |
| Card Library search | Source searches the normalized concatenation of title, a space and description. The port still searches the two fields separately and normalizes/trims the query more aggressively. | Restore the source matching boundary and retain the existing debounce/input ownership. |
| Profile/history time zone | The source converts timestamps to the host's local time zone; the port uses UTC. | Supply a bounded host calendar conversion interface if local-zone parity is required on every host. |
| History typography | History summary still uses its existing8px band for date/time. This batch fixes its calendar contents, not the entire history header's authored size/position. | Audit the full `NRunHistory` scene typography and apply source sizes with exact dynamic glyph coverage. |
| Title/Continue | Existing Continue preview still uses the general text renderer. This batch does not establish its precise source label sizes/padding. | Recheck `NContinueRunInfo` and its scene against composed bilingual frames. |
| Settings and run setup | Prior preference/input behavior remains implemented, but this batch is not a renewed per-node typography audit of all settings/custom/daily controls. | Enumerate visible normal/bold roles and check exact widths, dropdowns and focus states in the matched build. |
| Bestiary previews | Every phase-specific multi-creature preview and secondary visual effect has not been independently replayed. | Drive the remaining actual move/state contexts, keeping preview state detached from the run. |
| General acceptance | New glyphs, the changed App layout and render/input code require one matching frozen build and paired private resources. | Run native, actual bilingual, ordinary browser and calculator gates; retain failures separately from passes. |

The source-only syntax pass covers profile/statistics/library/relic-inspect/
Bestiary regression translation units plus the real input and pointer units.
It does not establish runtime acceptance. The new statistics test is registered
with CTest and additionally accepts a full private pack and capture directory.
