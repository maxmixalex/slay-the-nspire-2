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
| Main menu | The original269×450 centered VBox and200×50 controls project to38px-wide hits,6px regular labels and9/10px rows. English retains fractional source tracking; Chinese substitution removes it. Source3px half-alpha outline,50ms hover, sequential200ms press/release scale then color, and animated8px reticles are restored. | Independent six-label pixel oracle in both visible stacks, source input bounds and browser coordinates. |
| Continue preview | Inherited half-anchors plus the source145/5px offsets place the shown panel at153,154 with85×38/44 dimensions.5px rich saved/progress/ascension/HP/gold labels,7px icons and the conditional Ascension row replace the enlarged character-name panel.200ms show/hide motion is restored. | Independent label/color/position oracle for zero/nonzero Ascension plus read-only run checks. |
| Profiles | 7px regular header; 6px slot title; 5px description/delete/menu title; source rectangles, panel/icon/text parent scaling, delete label cubic-out motion, menu-specific 50/300ms easing; keyboard navigation skips hidden delete buttons. | `profiles_ui7_regression` persistence/input checks and exact-font scalar pixel oracle. |
| Profile helper | Authored 18px helper projects to3px; the port deliberately keeps4px for calculator readability. Mouse-only visibility is restored. | Actual locale pack and pointer/keyboard composed review. |
| Profile/history calendar | Chinese month names and twelve-hour period labels follow the pinned formats. Profiles put the period before the clock; history puts it after. The existing UTC host boundary is unchanged. | Profile/history pixel oracles and exact dynamic glyph requirements at5px. |
| Compendium | Source5px titles/descriptions, true multiline descriptions, title outline/shadow, icon rectangles and source pivot-based parent scaling; short-button press scale is0.98 without changing long-menu defaults. | Profile regression additionally checks six buttons at rest and full hover against an independent text/transform oracle. |
| Statistics | Source container minimums produce a1096px content column at sourceX227; two256x90 tabs, source section backgrounds,5px entries,7px headers,6px tabs and5px character names replace the enlarged generic layout. | `statistics_ui10_regression` source geometry, dynamic row layout and actual packed-font pixel checks. |
| Statistics conditions | Missing fastest victories and zero ascensions leave their original rows hidden; single visible rows remain vertically centered. Original character colors and red character-ascension prefix are restored. | Empty/full metadata cases, single/pair label pixel cases and saturated counter minimum-band/clipping. |
| Statistics interaction | Twenty-four bounded entry motions, initial first-entry focus, directional navigation, focus-driven scrolling, original Playtime/Ascension hover text, corrected scrollbar drag geometry. UI state is292bytes and stays outside canonical saves/RNG. | Input/scroll, endpoint checks, captures and canonical-save identity. |
| Statistics mask | The original128px gradient texture is sampled at the source offsets and composed once over the complete scrolling subtree. At240px the two filtered bands occupy rows29–37 and232–239. A10,880byte temporary stack snapshot preserves only those17rows; no persistent buffer or ordinary-canvas branch is added. | Independent floating-point texture-sampling/group-compositing oracle, overlapping translucent primitives, unchanged central/outside rows and actual scrolled captures. E matching acceptance pending. |
| Card Library | The count follows the original centered rich-text markup. Sort/filter/cost/check labels select each locale's actual normal face. The actual-pack oracle exposed four empty sort captions: they now resolve the original world-text keys instead of searching only the supplemental locale index. | `library_ui7_regression` independent sidebar/count oracle. |
| Card Library search | Source normalization now streams the joined title, one separating space and description. Queries keep their source lowercase-only behavior and meaningful leading/trailing spaces; blank queries remain unfiltered. A128byte prefix table replaces the old2KiB normalized-copy buffer. | Cross-field phrase/space cases, debounce/input ownership and actual matching gates; E matching acceptance pending. |
| Relic/potion collections | Rich role substitution fixes Chinese relic headings while keeping English's authored regular-bold mapping. D actual acceptance exposed three missing Chinese5px Bold glyphs; E adds exact rich-category/Ancient title requirements. | All20 category variants in the collection scalar pixel oracle; E matching acceptance pending. |
| History header | Source24px rich information and32px counters project to5px and6px. Right-aligned date/seed/full game-mode/build rows keep fixed sizes and source overflow behavior. The single HBox retains source minimum widths, counter colors, potion backdrop/slots, selected1.1 portrait, conditional Ascension and original custom/daily lock. The extra character-name heading is removed. | Independent full-frame header text oracle for Standard/Custom/Daily and partial time, source HBox minimum/expansion checks and exact locale glyph requirements; E matching acceptance pending. |
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
| Compendium press | Original press modulates the whole control toward light gray. The port's panel state changes, but icon/text parent press modulation remains separate work. | Apply the original parent modulation through the same bounded transition and add press/release oracle states. |
| Profile/history time zone | The source converts timestamps to the host's local time zone; the port uses UTC. | Supply a bounded host calendar conversion interface if local-zone parity is required on every host. |
| History body | This batch restores the summary header. The existing map act labels, relic/deck section captions and grouped-card list still use their earlier8px bands/layout. The source death-quote/badge strip needs a separate history-specific comparison. | Apply the remaining body nodes' fixed source sizes and geometry, then inspect a long full history and its tooltips. |
| History build identifier | Original `BuildLabel` displays the recorded run's `BuildId`. Current UI history never recorded that field, so the new source-positioned label displays an unknown dash. | Persist a separate observed build identifier for newly recorded runs while leaving old history unknown. |
| Continue save time | The canonical run/host interface has no original `SaveTime`. The new source saved-time label therefore displays an explicit unknown dash; profile-progress modification time is not substituted. | Add a separately persisted run-save timestamp with backward-compatible unknown handling if exact historical save time is required. |
| Chinese regular FontVariation | The source Chinese Regular resource also contains a0.95 transform and−0.05 baseline offset. The current packed raster/oracles establish exact selected font keys and LCD composition, without reproducing this Godot variation transform. | Bake and independently compare the source variation metrics before asserting full font raster equivalence. |
| Settings and run setup | Prior preference/input behavior remains implemented, but this batch is not a renewed per-node typography audit of all settings/custom/daily controls. | Enumerate visible normal/bold roles and check exact widths, dropdowns and focus states in the matched build. |
| Bestiary previews | Every phase-specific multi-creature preview and secondary visual effect has not been independently replayed. | Drive the remaining actual move/state contexts, keeping preview state detached from the run. |
| General acceptance | New glyphs, the changed App layout and render/input code require one matching frozen build and paired private resources. | Run native, actual bilingual, ordinary browser and calculator gates; retain failures separately from passes. |

The source-only syntax pass covers profile/statistics/library/relic-inspect/
Bestiary regression translation units plus the real input and pointer units.
It does not establish runtime acceptance. The new statistics test is registered
with CTest and additionally accepts a full private pack and capture directory.

## UI10c actual front gates and the following correction batch

Checkpoint `3d6d79907bb76269474acd8bd5c27baf575837f2` with its matched
English/Chinese fonts passes the Profiles/Compendium, Relic Inspector and
Bestiary actual-pack tests in both languages. Statistics passes Chinese;
its English saturated-counter test incorrectly assumed two32-bit maxima
must fit the minimum4px band, despite the source clip rectangle. The next
batch retains a complete independent clipped-pixel comparison and allows
that explicitly constructed extreme case. The Card Library actual tests
failed on the empty-sort-caption lookup defect described above; those failed
receipts remain separate from the correction's eventual runtime acceptance.

The next batch also restores the main menu/Continue source geometry and
adds their independent text/presentation checks. Short syntax checks pass,
but their new runtime pixel oracles require the matching next frozen binary
and font append. The locale binding gate passes after registering all16
Chinese calendar symbols and regenerating only the append-only inline table;
legacy pack string slots are unchanged. Native/ordinary-host final acceptance
and actual unmodified-game frame comparison are not implied by these results.

## UI10d actual front gates and E corrections

Checkpoint `78ac86e35651d3832e1b9638001aa0aa6be3f0a0`, its frozen native
build and its matching format37 resources executed all15 front targets per
language. English passed14/15 and Chinese13/15. Statistics, History, selections,
run modes, auxiliary screens, input bindings, Bestiary, phobia, feedback, power
ordering and world text passed both locales. Card Library passed English; the
Chinese collection oracle correctly rejected three absent exact Bold glyphs.
Both Profiles tests rejected the newly added title text oracle because its
baseline composited the highlight before the label; the source highlights are
later siblings. The corrected oracle preserves that drawing order and its
complete independent label-pixel comparisons.

The separate world-front fixture also incorrectly applied title-row geometry
to paused menus and hid the pointer while recording a background-animation
comparison. E keeps independent geometry assertions for both menus and
preserves pointer/focus state during those animation captures. No production
background bounds or pixel assertions were weakened.

E short diagnostics compile the current Profiles/Statistics/History test
translation units against the frozen D core and run both D packs: all six
pass, including the new group-mask and header oracles. Both corrected
world-front diagnostics also pass with captures. These mixed-stage diagnostics
are explicitly not E release acceptance. The source changes, new format38 lock,
and exact font requirements still require the single matching E build/resource
gates and ordinary hosts. Failed D receipts remain failures.
