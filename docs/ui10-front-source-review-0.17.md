# UI10 front-end source review

Pinned reference: v0.111.0 / 41cef1ea. This records source implementation and
the separately identified matching acceptance stages, not a measured
restoration percentage. Original scenes,
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

The Chinese Regular resource declares a0.95 transform and−0.05 baseline
offset. A direct Godot4.5.1 experiment found the same advance, ascent and
descent as the base font at twelve sampled sizes. Those declared properties
therefore do not by themselves prove a packed-metric defect. The experiment
does not establish identical glyph ink, kerning or complete raster output.
Integer LCD advances remain a measured adaptation; the Continue date case
below records their visible effect separately from the source variation.

## Implemented source contracts

| Surface | Concrete change | Verification target |
| --- | --- | --- |
| Main menu | The original269×450 centered VBox and200×50 controls project to38px-wide hits,6px regular labels and9/10px rows. English retains fractional source tracking; Chinese substitution removes it. Source3px half-alpha outline,50ms hover, sequential200ms press/release scale then color, and animated8px reticles are restored. | Independent six-label pixel oracle in both visible stacks, source input bounds and browser coordinates. |
| Continue preview | Inherited half-anchors plus the source145/5px offsets place the shown panel at153,154 with85×38/44 dimensions.5px rich saved/progress/ascension/HP/gold labels,7px icons and the conditional Ascension row replace the enlarged character-name panel.200ms show/hide motion is restored. I keeps the source DateLabel's two explicit lines and measured6px English/7px Chinese line spacing. | Matching I independent label/color/position oracle for zero/nonzero Ascension, unknown/Sunday/long-Wednesday timestamps and read-only run checks. |
| Profiles | 7px regular header; 6px slot title; 5px description/delete/menu title; source rectangles, panel/icon/text parent scaling, delete label cubic-out motion, menu-specific 50/300ms easing; keyboard navigation skips hidden delete buttons. | `profiles_ui7_regression` persistence/input checks and exact-font scalar pixel oracle. |
| Profile helper | Authored 18px helper projects to3px; the port deliberately keeps4px for calculator readability. Mouse-only visibility is restored. | Actual locale pack and pointer/keyboard composed review. |
| Profile/history calendar | Chinese month names and twelve-hour period labels follow the pinned formats. Profiles put the period before the clock; history puts it after. The existing UTC host boundary is unchanged. | Profile/history pixel oracles and exact dynamic glyph requirements at5px. |
| Compendium | Source5px labels, outlines/shadows, icon rectangles and pivots remain. G restores whole-parent light-gray press modulation, preserves the current HSV value during press, and resumes interrupted shade/scale transitions; short/bottom press scales remain0.98/0.95. | Six-button rest/hover/mid-press/pressed/mid-release/released scalar pixel states and interrupted-refocus values pass both matching I packs. Earlier E-pack short diagnostics remain separately identified. |
| Statistics | Source container minimums produce a1096px content column at sourceX227; two256x90 tabs, source section backgrounds,5px entries,7px headers,6px tabs and5px character names replace the enlarged generic layout. | `statistics_ui10_regression` source geometry, dynamic row layout and actual packed-font pixel checks. |
| Statistics conditions | Missing fastest victories and zero ascensions leave their original rows hidden; single visible rows remain vertically centered. Original character colors and red character-ascension prefix are restored. | Empty/full metadata cases, single/pair label pixel cases and saturated counter minimum-band/clipping. |
| Statistics interaction | Twenty-four bounded entry motions, initial first-entry focus, directional navigation, focus-driven scrolling, original Playtime/Ascension hover text, corrected scrollbar drag geometry. UI state is292bytes and stays outside canonical saves/RNG. | Input/scroll, endpoint checks, captures and canonical-save identity. |
| Statistics mask | The original128px gradient texture is sampled at the source offsets and composed once over the complete scrolling subtree. At240px the two filtered bands occupy rows29–37 and232–239. A10,880byte temporary stack snapshot preserves only those17rows; no persistent buffer or ordinary-canvas branch is added. | Independent floating-point texture-sampling/group-compositing oracle, overlapping translucent primitives, unchanged central/outside rows and actual scrolled captures pass both matching I packs. |
| Card Library | The count follows the original centered rich-text markup. Sort/filter/cost/check labels select each locale's actual normal face. The actual-pack oracle exposed four empty sort captions: they now resolve the original world-text keys instead of searching only the supplemental locale index. | `library_ui7_regression` independent sidebar/count oracle. |
| Card Library search | Source normalization now streams the joined title, one separating space and description. Queries keep their source lowercase-only behavior and meaningful leading/trailing spaces; blank queries remain unfiltered. A128byte prefix table replaces the old2KiB normalized-copy buffer. | Cross-field phrase/space cases, debounce/input ownership and both matching I actual-language gates pass. |
| Relic/potion collections | Rich role substitution fixes Chinese relic headings while keeping English's authored regular-bold mapping. D actual acceptance exposed three missing Chinese5px Bold glyphs; E adds exact rich-category/Ancient title requirements. | All20 category variants pass the collection scalar pixel oracle in both matching I packs, with no missing exact glyph keys. |
| History header | Source24px rich information and32px counters project to5px and6px. Right-aligned date/seed/full game-mode/build rows keep fixed sizes and source overflow behavior. The single HBox retains source minimum widths, counter colors, potion backdrop/slots, selected1.1 portrait, conditional Ascension and original custom/daily lock. The extra character-name heading is removed. | Matching I packs pass the independent full-frame header oracle for Standard/Custom/Daily at scroll0/5, partial time, known/unknown recorded BuildId, source HBox minimum/expansion and exact locale glyphs. The scrolled header remains visible to screen row0. |
| History body and motion | Source fixed labels, rarity summaries,60px act rows,19-column relics, five-column card entries, death quote/badges and64 original room/outline images are implemented. H restores UI-only random tilt and interrupted focus/unfocus rotation of the icon, outline and quest child. | Matching I History/journal gates pass both languages, including independent body/header pixels, recorded facts, source motion timelines and canonical-state identity. The quantified LCD and bounded-history limits remain below. |
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
| Profile/history time zone | The source converts timestamps to the host's local time zone; the port uses UTC. | Supply a bounded host calendar conversion interface if local-zone parity is required on every host. |
| History bounds and raster | Room angles use1/16-degree quantization and Q10 inverse sampling at LCD resolution. Presentation retains at most eight simultaneous transitions; history retains96 floors and32 runs. Source16px quote fitting keeps the documented4px minimum. | Preserve these explicit limits when adding later scenarios; do not report the matched source timelines as continuous-angle or unlimited-history parity. |
| History partial metadata | Older or evicted floor journal facts remain unavailable. New BuildId is the actual port identifier recorded at new-run creation; pre-metadata records retain an unknown dash. | Keep the verified old/new optional metadata codecs and unknown handling; do not synthesize missing historical values. |
| Continue date advances | I restores the source nonwrapping two-line date at5px. Integer glyph advances make the tested long English line extend6 LCD pixels beyond the panel and the Chinese line5, while staying on screen. The original source-width measurements are narrower. | Compare future glyph-advance/ink work against the measured source case; do not invent a smaller source font to conceal the difference. |
| Chinese regular FontVariation | The source resource declares a0.95 transform and−0.05 baseline offset. The targeted Godot experiment found the same advance/ascent/descent as its base at12 sampled sizes; this does not establish identical glyph ink. | Retain the exact face/size coverage and independently compare glyph ink before asserting full font raster equivalence. |
| Settings and run setup | The implemented General/Input and Standard/Custom/Daily surfaces pass matching I. Source startup-logo skip, desktop background-FPS control and remote gameplay upload service have no corresponding port feature. The Daily4px minimum and offline leaderboard state remain. | Keep these feature/platform differences explicit; see `settings-run-setup-ui10-0.17.md`. |
| Settings hover verification | I's three old named hover captures used stale row coordinates. J corrects those source coordinates and adds Long Press hover ownership/binding checks without changing production code. | Run the frozen J test translation unit with the proven unchanged I core and both I packs; the old capture names alone do not prove the intended hover was exercised. |
| Bestiary previews | Every phase-specific multi-creature preview and secondary visual effect has not been independently replayed. | Drive the remaining actual move/state contexts, keeping preview state detached from the run. |
| Original-frame and device scope | I native, actual bilingual and ordinary browser gates pass at the checkpoint below. These source-derived tests do not establish unmodified Godot-frame equality or physical-calculator timing. | Keep original-frame comparison and ordinary calculator capacity/save execution as separate evidence; do not turn passing-test counts into a restoration percentage. |

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

The following batch restored the main menu/Continue source geometry and
added their independent text/presentation checks. Its initial short syntax
checks did not substitute for a matched binary and font append; the subsequent
D failures and E passes are recorded below. The locale binding gate passed after registering all16
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
were explicitly not E release acceptance. The matching E gates below now cover
the source changes, new format38 lock and exact font requirements. Failed D
receipts and the mixed-stage diagnostic records remain unchanged.

## UI10e matched acceptance and composed review

Frozen runtime `04038b2cda0240bfa2be1af79bf62401ec26e7a9`, tree
`0013609a7d365c985f62f8c2798a3af790c12589`, was built with the same checkpoint's
format38 English and Chinese resources. All15 front targets pass in each
language: Profiles, Statistics, deck/world selection, Card Library and
collections, run modes, auxiliary screens, History and its journal, input
bindings, Bestiary, phobia, device feedback, power ordering and world text.
The private acceptance records confirm that neither tested binaries nor packs
changed during execution. This closes the matching-resource gaps identified
above; it does not close the concrete remaining source differences.

The two front journals are `ui10e-front-eng-aac2c29a203c.json` and
`ui10e-front-zhs-852ca306d067.json`. The same checkpoint's six actual-resource
receipts pass94/94 target-language pairs, and its native journal
`ui10e-native-034f41360da8.json` passes119/119 suites. The ordinary offline HTML
browser retry passes18 checks with41 captures, no page errors and no external
requests. Its initial Chromium launch failed before any flow ran with a
sandbox `setsockopt`/`SIGTRAP` error; that failed launch receipt remains separate
from the successful retry. These are different execution layers, not additive
units of source coverage.

Composed visual review covered20 current E captures across both languages:
Statistics initial/scrolled views, History top, Library sidebar, relic and
potion collection tops, new/continued title states and quit/abandon popups.
The Statistics subtree fades at both edges; the compact History header keeps
its date/game-mode alignment and counter separation; all collection category
headings remain visible with the exact Bold additions; the title/Continue and
popup text has no newly observed clipping or unintended overlap. Captures and
contact sheets remain private. This review supplements the independent
source-derived pixel/input oracles, and is not a side-by-side comparison with
an unmodified running Godot build.


## F matched front acceptance and G source follow-up

Frozen F `361c36c27f06c4905e1899b8bdc2ab281ba9b9cc` passes the15 front
targets in each language. Its ordinary offline HTML passes18 checks and41
captures with no page errors or external requests, including English→Chinese→
English and saved-language reload. The tested HTML and the closed final
package's HTML were compared byte for byte. Twenty key F front/menu renders
also match the previously reviewed E captures exactly. The receipts are
`ui10f-front-eng-6adea22e0605.json`, `ui10f-front-zhs-335b830e492f.json` and
`ui10f-ordinary-browser-94cec6d351c1.json`; these validate F, not later G edits.

G restores the remaining History body's source24/28px labels to5px, the
original rarity-category summaries, source card colors and20–24px fitted
card labels with the authored U+22EF ellipsis. Acts use one60px entry row
without the former18-column artificial wrap. Relics use68px holders in19
columns; cards use240×32px holders in five columns, with their original
32px tiny card, enchantment child and8px focus label shift. Card focus
interruptions fast-forward the prior tween as the source requires. The
scrolling viewport is the full1260px screen, with the original25px bottom
padding. Source extrema and the maximum192-card fixture replace the old
oversized rows as the reason the scrolling test needs a scrollbar.

The death-quote/badge HBox now uses saved run character/cause/seed values,
original conditional badge visibility, source equal expansion/minimums and
ordinary source hover tips. It does not borrow the current live character or
consume canonical run RNG. Source22px italic quotes project to4px; the shared
4px readability floor remains when the source would shrink to16px. Chapter
labels and captions use their authored cream; upgraded/enchantment labels use
source chartreuse/violet. Historical relics also receive their existing
original black half-alpha outlines. Presentation focus ID ranges are separated
so the maximum320 relics cannot overlap card focus IDs.

Format39 appends32 original History room image/outline pairs from the pinned
`ImageHelper` mapping, including unknown rooms, bosses and ancients. They are
kept at their maximum13px hover resolution, preserve alpha and aspect, and
retain the source outline's47→128→64 opacity plus black/white focus color.
Existing image/glyph payloads remain unchanged. Old packs retain an explicit
HUD-icon fallback. The append metadata is source-only; original image data
remains private. The shared G converter also appends independently reviewed
settings artwork after the64 History entries.

G syntax and bounded synthetic artwork checks pass. Mixed short diagnostics
recompile every App-dependent translation unit against current headers before
linking unchanged F simulation objects; they are not matching G release gates.
Both Profiles locales pass the new press/release and known/unknown Continue
pixel cases. History's no-pack navigation/state checks pass62 assertions.
Its E-pack exact oracle exposes the expected newly required English4px regular
U+22EF and Chinese4px regular U+7D2F in the new quote coverage. Both failures
are retained and both glyphs are required by the new font metadata; neither
oracle is relaxed. Earlier Chinese body/header checks passed before the final
quote-band case was added. Final acceptance must use G's complete matching
font and artwork append.

## H strict font band and room rotation

G's first formal font conversion failed because the modern offline parser
omitted the new9px source heading band. H admits9 only in NSP2/39, including
the actual Spectral encoded band41. Formats33–38 still reject9; zero or
oversized glyph geometry, unsupported11px bands, key ordering and payload
bounds remain checked. Legacy20–23 glyph-tail relocation does not accept
modern packs and needs no expanded grammar. Runtime font selection already
discovers bounded present bands; generic fitting still excludes sparse9px.
The corrected resource order appends39 artwork before fonts and compacts
only the new glyph payload against that39 artwork baseline.

`NMapPointHistoryEntry` uses a standard normal conditioned on[0,1], then
multiplies by5; its random initial sign is independent of the positive base
angle restored on unfocus. H follows that distribution with16-bit CDF
thresholds and1/16-degree angle steps. Focus kills the previous tween and
rotates to zero in50ms ExpoOut; unfocus resumes the sampled angle and rotates
to the positive base over1000ms. A private cosmetic seed and96 visited bits
fit in the124-byte existing bounded presentation. They never enter canonical
state or consume saved RNG. Retiring a transition keeps the post-hover
positive angle and outline alpha64 instead of restoring the initial sign or
alpha47. At most eight concurrent transitions are retained, as before.

The room icon, its outline and quest child rotate about the parent's authored
32px pivot. The quest position rotates with its parent, rather than only
rotating the child's texture. Small LCD boxes use an explicit half-pixel
alignment and exact rational stepping, avoiding a shifted zero-angle image
or an extra edge pixel at odd scale ratios. Q10 sine/cosine values and
nearest sampling are explicit raster adaptations. Other UI transforms retain
their original hot loop and pixel convention; no image pool was added.

H short checks pass: three font-schema tests,66,076 independent distribution/
timeline checks,4,147,254 synthetic texture comparisons and syntax for the
actual App and History regression units. The synthetic comparisons use a
direct inverse matrix, positive/negative/zero angles,9/12/13px boxes, screen
edges and nested opacity; they also protect the legacy transform pixels.
These standalone checks link no prior App core. Subsequent matching H native
passes120/120 suites; its actual resources pass94 target-language pairs and
its ordinary browser passes18 flows/41 captures. The front journals are
`ui10h-front-eng-c298c25cce58.json` and
`ui10h-front-zhs-f62b1f420d19.json`; native and ordinary-browser journals are
`ui10h-native-45833e325d2e.json` and
`ui10h-ordinary-browser-8e1687f0e40a.json`. Failed G conversion and fixture
receipts remain failed, separate from this matching H acceptance.

Twenty H captures were visually reviewed across History, run setup, Input and
Continue. The F-to-H comparison found170 captures byte-identical and14
auxiliary captures changed only by the source Settings underlay or identical
cursor artwork at the moved Credits button. Source-changed History/setup/date
captures were reviewed separately. The review found the date defect below
after H's prior oracle had passed; that pass did not prove its premise correct.

## I matched acceptance and corrected Continue date

Frozen I `76c022a1e2db17c49bf3ab8edb335dbb98618582`, tree
`546128015f0087d2327b28f3514019ccdc53d44d`, passes121/121 native suites in
101.74seconds. Its matching format39 resources pass all15 front targets per
language,17 root targets per language and16 combat targets per language:
96 target-language executions. Both full NSL2 virtual-stream comparisons also
pass. These are separate execution layers, not source-fidelity units.

The front journals are `ui10i-front-eng-484de2c1322c.json` and
`ui10i-front-zhs-082c1fd5df37.json`; the native journal is
`ui10i-native-e108c5719b58.json`. Tested binaries and packs are unchanged
through each execution. The actual History body/art/quote/badge, Compendium
press, source Settings/run setup and observed metadata checks are now matched
to their production fonts and artwork. Earlier short diagnostics are not
substituted for these results.

H's Chinese known-date capture had split the clock between two unintended
lines. The pinned DateLabel disables automatic wrapping, allows overflow and
has a400×90 text box. A direct Godot4.5.1 experiment with the original fonts
and the translated `MegaRichTextLabel` size estimator tested unknown, Sunday
and long-Wednesday dates in both languages. All six retain24px type, two
explicit lines, and source line offsets30.5px English/36px Chinese. I projects
these to5px type with6/7px line steps, keeps the gold first line top aligned
and gives the pixel oracle independent hard-line layout. No extra glyph or
artwork payload is required. The previous H pass and the subsequent visual
failure remain recorded rather than rewritten as an unbroken pass.

The LCD's integer advances are a remaining measured difference. The long
English date is87px wide and ends6px beyond the85px panel; the Chinese date
is86px wide and ends5px beyond it. Original Godot widths are369 and372 source
pixels respectively. The source explicitly allows overflow, and the port's
text stays within the screen without covering another control. The port
does not invent a4px source size to hide the advance mismatch.

The H-to-I front comparison contains214 captures:188 unchanged captures are
byte-identical, eight Continue captures change under the corrected oracle,
four long-date cases are new, and14 auxiliary differences are fully explained.
Each of the four Credits differences is exactly the same66-pixel cursor moved
down one added Long Press row; every other pixel is identical. All ten
Feedback opaque panels are identical; their live Settings underlays contain
the new source checkbox. No unexplained difference remains. Eight current
date/Ascension captures and four current ordinary long-press captures were
visually inspected; the explicit clock line remains intact.

The final ordinary HTML passes21 flows and47 captures with no page errors or
external requests. Real key/button down, repeat, up and pointer-exit input
verifies the per-profile Long Press setting: short attempts preserve turn one,
a held mapped key ends one turn, and a held toolbar control ends one further
turn. Reload preserves NSPF5 and NSV17 while the other two profile saves stay
exact. The original StillCanPlay and Shuffle tutorials are handled through
their actual modal confirmations. This adds ordinary coverage for those two
tutorial contexts, not all thirteen tutorials.

The browser journal is `ui10i-ordinary-browser-3bdf52198bec.json`. The tested
HTML is129,282,437bytes, SHA256
`8ecb49ca4108410fbd634e250a40328ee25ef4886d44cf7ff10090bebda18c06`;
the independent closed-package manifest and hashes before/after browser use
agree. A later test-only J correction is still required for the three stale
Settings hover coordinates and added Long Press hover assertion above.
