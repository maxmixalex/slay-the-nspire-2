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
The matching immutable eight-a native gate and real ENG/ZHS capture review are
still required; syntax and catalog inspection alone are not visual acceptance.
