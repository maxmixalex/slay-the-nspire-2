# Hover tips and related collection models — UI8 / 0.17

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
