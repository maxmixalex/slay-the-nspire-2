# UI22 menu, reward and lifecycle acceptance

Reference: owner-supplied Slay the Spire 2 v0.111.0 / `41cef1ea`.
Private original assets, decompiled reference, locale packs and captures stay
outside the repository. These are executed native production-renderer checks;
they do not establish Firebird execution or physical-calculator frame rate.

## Continue metadata placement

The previous panel followed only the Continue label. That could intersect the
larger Abandon Run label and its hit region. The shared title layout now measures
all localized menu labels, including the focused scale and reticle padding, and
uses the resulting bounds for both hit testing and popup placement. A compact
right gutter is preferred. A long-menu fallback occupies the clear upper-right
area. Both placements account for the complete logo and profile-control bounds,
not just text baselines. The original rise/fade behavior is retained.

`world_frontend_ui_regression` checks 20 combinations per actual locale: five
menu focuses, ascension zero/three, and mouse/keyboard. At the same clock, showing
the metadata must leave every menu-hit pixel, complete logo pixel and profile
control pixel unchanged. The panel must remain onscreen and paint visible pixels.
The independent text-placement check in `profiles_ui7_regression` uses the same
measured placement; its font raster reference remains independent.

English, Simplified Chinese, German, French and Russian all passed. The French
long-label fallback was also inspected in the actual 320x240 capture. No menu
item or profile control is covered, and the popup remains close to the menu.

## Loot potion and relic hover tips

The root cause was an absent central hover route: reward rows use `UiAction` or
`WorldChoice`, so the existing shop/inventory routes never selected them. The
new route identifies the actual visible reward and supports mouse hover and
keyboard focus for ordinary, world and event rewards, including claimed-index
gaps and standalone potion offers. Gold and ordinary card rows retain their
existing behavior. Detached relic previews do not borrow an owned-inventory
index.

The placement follows the source `NRewardButton` left-side hover behavior. At
the normal reward row's x=87 position, a readable 80-pixel panel uses the left
gutter with 10-pixel text. Derived cards retain their separate right-side space.
Long headers use the existing paused horizontal scroll; the reward row also
retains the full item label. Body text is paginated without discarding words.

The **V** details command opens the reward reader; potion Details can enter it
as well. Up/Down or the footer arrows change pages. Back/Details returns to the
same reward focus. A modal input layer prevents an overlay click, Confirm,
EndTurn or navigation shortcuts from claiming/skipping a covered reward. A click
outside the reader exits only. Normal card Details and the top-bar potion
controls retain their original routes. Save/Load/Quit remain global commands;
an actual Load check restored a different run, cleared reader state and restored
normal scene input.

### Executed actual-language coverage

The catalog check visits every formatted tip's actual rich-text glyph layout,
collects the non-whitespace Unicode sequence across all pages, and compares it
to the complete formatted description. It also verifies contiguous page spans,
onscreen geometry, one copy of each derived card, and no overlap with reward
rows or sibling tips. Character counts below include both passive and full
reader layouts; they are verification work, not a count of new features.

| Locale | Potion/relic models | Characters retained across all pages | Passive entries requiring multiple pages |
| --- | ---: | ---: | ---: |
| English | 361 | 48,276 | 24 |
| Simplified Chinese | 361 | 21,610 | 15 |
| German | 361 | 57,784 | 37 |
| French | 361 | 64,052 | 38 |
| Russian | 361 | 48,544 | 28 |

All seven reward-route scenarios passed mouse and keyboard ownership checks,
actual painted-pixel checks, modal controls and unchanged canonical-save checks.
The five actual-language full frontend and reward runs passed, plus both English
and Chinese profile runs: twelve successful target invocations. The five-locale
reader contact sheet and passive potion/relic captures were visually inspected.

## Exact affliction phase clock

The original Entangled material stores `distortion_st=(1,1,0,0.175)` in
`materials/vfx/ui/card/afflictions/entangled/vfx_ui_card_affliction_entangled_main.tres`.
Its shader offsets the texture by `TIME * distortion_st.w`; the original
Tainted scene references that same material. A repeated texture therefore has
an authored period of exactly 40/7 seconds. Rounding it to 5714 milliseconds
made each port cycle end 2/7 milliseconds early and accumulated phase drift.

`card_affliction_frame` now selects the existing eight samples using the exact
rational phase `((now % 40000) * 7 / 5000) % 8`. Other affliction clocks use
bounded 32-bit constant arithmetic as well. There are no additional resource
images, tables or allocations. The original complete ten-frame Ancient fire
sequence and thirty-frame Infection sequence remain unchanged.

`combat_ui14_regression` compares all seven clocks to an independent 64-bit
rational reference at transitions, long elapsed times and the uint32 boundary.
For Entangled and Tainted it also compares the actual rendered card overlay to
the source image selected by that reference. Eight sampled affliction phases
remain a finite representation; this change resolves the accumulated clock
error without claiming continuous shader rendering.

## Reused App address and shared scene memory

An independent review of the selection raster cache found that a newly
constructed App at a previously used address can reopen Content at generation
one. Address plus generation alone could then match a stale scene raster.
Both App initialization paths now invalidate the shared scene owner before
opening data. The large arrays are not cleared or duplicated.

`scene_raster_cache_regression` constructs two successive App objects at exactly
the same address using both memory and ranged input. Each starts at Content
generation one, invalidates the owner and reconstructs all 76,800 pixels from
its new source. Existing Random/merchant ownership checks remain in place.

## Source and asset validation boundary

The UI/lifecycle runtime and public tests above were executed from published
checkpoint `701c85a1d270bed2beca20a5481388d22ca79a9d`. The actual-language inputs
were the complete UI21 resource controls, whose text and UI catalogs are retained
by UI22. Earlier failed journals record the obsolete two-language test fixture,
a small synthetic fixture's missing hover text, and the overlay Back shortcut
that previously entered Pause; the corrected checkpoint passes all five scoped
synthetic UI/lifecycle targets and all actual-language runs above.

The first complete 427-action / 11,051-frame actor resource input also passed
`character_death_ui9_regression`, `world_ui_regression` and
`five_character_ui_regression` through the production renderer. These checks
cover all five characters' combat/event/rest/shop death routing, terminal holds,
particle decay, and actual Osty growth, kill, dead-loop and revival gameplay.
The native source-pixel oracle and final-resource receipts are separate release
gates; intermediate artwork acceptance is not a claim that the final installer
has already been built or tested on a calculator.

The accepted audio/multiplayer/Timeline exclusions and static Ancient-scene
exception remain explicit in the product scope and retained-differences ledger.
Finite sampled affliction shaders remain a distinct material representation;
this UI acceptance does not claim complete Godot GPU equivalence.
