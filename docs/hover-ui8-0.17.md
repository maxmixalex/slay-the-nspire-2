# Hover tips and related collection models — UI8 / 0.17

## Merchant follow-up — UI9

The recovered `NMerchantRelic.CreateHoverTip` and
`NMerchantPotion.CreateHoverTip` expose the complete model hover set. Their
shared slot chooses left/right alignment at the viewport midpoint, using its
122px Control at the focused0.8 scale. It does not use the larger purchase
Hitbox as the tooltip anchor. The port still had a separate174px-wide box,
12/10px text and a five-line cutoff in the ordinary shop after other hover
surfaces had been corrected.

The shop now uses the existing69px original text columns, actual5px faces,
associated card/keyword/power tips, and bounded overflow paging. Its anchor
uses the projected22px slot Control and the original half-size offset. The
existing7/6 readability zoom of the rug and goods remains. Unpurchased stock
uses detached relic facts; it cannot borrow the same relic's mutable values
from the player's inventory. The separate local box and its now-unused
fallback text helpers are removed. This adds no persistent buffer or State
field.

The top-bar owned-relic holder had the same association omission despite
already using source-sized text. It now shares the complete hover chain,
retains the actual owned index and dynamic progress paragraph, and uses the
owner's Cookie variant. The old single-box pass is removed, so there is no
duplicate primary description. Mouse/keyboard holder ownership, associated
models, overlay suppression and unchanged saves are covered in the expanded
room regression alongside shop stock.

The expanded room regression covers all six real stock positions, mouse and
keyboard ownership, associated Fencing Manual/Cunning Potion models, every
tooltip page, viewport bounds, press/modal/closed/sold suppression and
byte-identical run saves. Its syntax check passes; the next matching native
and actual bilingual gates must execute it. No percentage or device-speed
claim follows from these implementation checks.

Reference: original v0.111.0 / `41cef1ea`, `NHoverTipSet`,
`NHoverTipCardContainer`, `hover_tip.tscn`, and `card_hover_tip.tscn`.

## Corrected presentation

The prior 136–166-pixel boxes and 10/12-pixel text were approximately twice
source scale beside the now-correct 46×64 card grid. Ordinary keyword,
static HUD, owned relic, potion, collection and library-control tips now share
the source layout: a 360-source-pixel column projects to 69 pixels; source
22-pixel Kreon regular/bold is rasterized at 5 pixels. The source margins
22/16/45/28 project to 4/3/9/5. The zero-separation VBox uses a six-pixel line
step. BBCode emphasis selects the actual bold face. Older packs select their
available font size; missing-size metrics never qualify as a fitting title.

The existing original background is sampled with its nine-patch margins in
its projected 65×20 texture domain. It is not stretched with the old
half-resolution source margins as if those were display pixels. Original
normal/debuff skins, gold title, cream body and quarter-alpha shadow remain.
There is no new image resource, permanent buffer or gameplay state.

`NHoverTipSet` text flows vertically and then into available adjacent columns.
The constrained display retains paging only after available columns are full.
Title/body line ranges survive page changes. Related cards occupy their own
46×62 container and render the original 0.75 card at 43×60, without duplicating
its title and description in an extra text box. Card inspect retains opposing
card/text columns. Relic tips begin below the owner, move above low owners and
correct viewport overflow. All hover surfaces remain passive input layers;
existing drag, captured-release, modal and paging behavior is retained.

The relic collection and potion lab now request the actual model-associated
`HoverTips`, including upgrades, related cards, powers and keywords. Unknown
models retain their original unknown text without revealing related content.
The shared observer marks actually displayed related models seen without
rebuilding the open collection or moving its selected/captured holder.

## Bounded fonts and verification

The append-only v31 font tool adds only absent 5-pixel bold title glyphs and
the separately required character-select display titles at 10/12/14 pixels.
The final Chinese pack has 31,162 glyphs within the 32,768 directory bound.
Existing art, glyph bitmaps, strings, model indices and clip registrations are
required byte-exact. The original Chinese font is Source Han Serif SC Bold;
English keeps the original Kreon/Spectral faces. Native FreeType rasterization
is not claimed pixel-identical to Godot glyph rasterization.

All changed translation units pass Clang 18 syntax checking. The expanded
keyword regression visits every associated tip of all 558 cards and both
upgrade states, validates each native text/card geometry, complete line access,
viewport containment and nonoverlap at 28 edge positions. Collection tests
exercise actual Fencing Manual and Cunning Potion hover, related-card discovery,
upgrade state, stable scroll/collection identity and captured pointer release.
Matching immutable stage eight-a-one (`16bdbda2`) and the complete private
v31 font packs pass **405,309 keyword checks per language** and **771,075
library/collection checks per language**. Captures are retained under
`private-inputs/ui9-stage8a1-captures/{eng,zhs}`; bilingual hand, related relic
and related potion montages were visually reviewed. The actual explicit-font
oracle also passes 152,614 ENG and 151,852 ZHS checks, including all five
character information blocks; 64 identical warm frames issue zero additional
provider reads in both languages.

The first actual library run exposed its old practice-combat fixture clicking
before compressed actors finished preparing. Its real-resource setup now waits
for that actual bounded preparation; the no-resource synthetic fixture retains
its original fallback path and separately passes 3,049 checks. The production
loading phase was not shortened or bypassed. Visual review also exposed event
tips appearing before their invisible options completed entry. The source
`AnimateIn.Finished -> EnableButton` and `OnFocus` lock guards now control hover
target eligibility. Tests explicitly verify absent entry/locked tips and then
wait only that event's original entry/reveal before interacting. Their real
captures include the settled story/options alongside associated cards.

The remaining last cleanup hides a redundant 1/1 footer, including the optional
expanded tip view, while preserving actual multipage navigation and its hits.
It is included in the parent's final eight-b checkpoint; the earlier captures
and logs remain evidence for their exact executed revisions.
# Battle power hover follow-up

The final integration candidate routes `BattleStatus` and `PowerInstance`
through the same source-sized hover set as cards and relics. The old 170px
panel, extra large icon/count header and nine-line cutoff are removed. The
source chain is `NPower.OnHovered` → `NCreature.ShowHoverTips` →
`PowerModel.HoverTips`: the whole creature Hitbox owns the set, with left/right
alignment chosen at the viewport midpoint. The current smart description and
independent instance progress remain in the first panel; debuff status keeps
its original skin. Text continues through the existing bounded columns/pages
instead of silently losing its final lines.

`generate_power_hovers.py` reads all 99 authored power `ExtraHoverTips` getters
and emits only model keys and association flags. The runtime consumes the
table, including independent related-card previews and their requested card
hover chains. Stateful Swipe uses the captured card; Withering Presence uses
the retained Wither instance at the current original upgrade level. The
existing separate temporary-Focus/Setup Strike status channels retain their
origin model; when mixed upgraded copies make the particular origin unknown,
the extra preview is withheld rather than claiming a fabricated upgrade.
Primary status amounts/descriptions remain available in that case.

The empty reference stubs for Bound, Hexed and Tainted were checked against
the original assembly. Bound has no extra tip; Hexed adds Ethereal; Tainted
adds the original Tainted Power chain. The recovered source remains private.

The new regression exercises real status hits, the creature Hitbox anchor,
both alignments, source widths, all-page access, debuff/related-Shiv captures,
drag/input suppression, save purity and separate Panache instances. At this
point the candidate has passed short syntax checks; matching frozen execution
and bilingual captures are still required before calling this follow-up
accepted.

The final Power visual review also checked the original
`NHoverTipSet.CorrectHorizontalOverflow` branches. A card preview that initially
lands beyond the creature's opposite screen edge now moves adjacent to the
actual text container; unused available columns no longer create a large gap.
This correction is limited to creature Power tips and covers both screen edges.
The two-Panache regression uses the actual Power command to register aggregate
amount and post-play listeners. Newly required Power icons explicitly finish
the bounded combat preparation phase before pointer hit checks, with the
canonical save and animation clock unchanged. A diagnostic run against the
frozen UI runtime passed 635,828 checks in each language before the final
Power-only horizontal-position correction. The final immutable source
`a0df79d8347b76dc86804b0ab8d17f948c600204` then passed all 103 native tests;
matching final-v33 keyword runs passed 635,832 checks in each language,
including both edge-adjacency assertions and reviewed live battle captures.
The complete final GameOver UI also passed its source-hover pixel-bound cases
in both languages (44,051 English and 43,334 Chinese checks).
