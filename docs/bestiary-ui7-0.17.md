# Bestiary source layout, input and previews

Reference: **v0.111.0 / 41cef1ea**. Source-backed implementation and the
matching acceptance gates are distinguished below. Original source, prose,
textures and captures remain private.

## UI11 current status

UI11 replaces the historical dense typography below with 10px two-line list
entries and five-action paging. Detached preview facts now preserve the named
regeneration, charge, burrow, stun, hatch, coral-growth, liquefaction and summon
phases, plus the three Decimillipede actors' staggered down/revive state.
The phase fixture passes **139 checks** at local checkpoint
`49340f020bfbe96c49cc6de71aa122e1556c269c`, with unchanged H core and exact
canonical-save preservation; receipt
`ui11h-bestiary-phase-correct-fixture-ab6202c82be3.json` uses the historical
format39 English pack. It is not a final-art render or Firebird speed test.

The sections below retain the UI7–UI10 implementation/acceptance history,
including their then-current sizes and typography. Their old phase-context
omissions are superseded by UI11's source-audit contract. Exact GPU materials,
every possible preview combination and final K emulator acceptance are not
established by the targeted phase test.

## Source contract

The screen follows `NBestiary`, `NBestiaryEntry`, `NBestiaryMoveButton`,
`NBestiaryModeButton`, `NBestiaryLayoutDefault`, `NBestiaryLayoutKaiserCrab`
and their authored scenes. `NInputManager`, `MegaInput` and `MegaLabel`
establish the input banks and actual font sizing. Geometry uses the source
FourByThree viewport, 1680×1260 projected to 320×240.

The catalogue contains 101 entries grouped into 29 Overgrowth, 22 Underdocks,
22 Hive, 23 Glory and five event entries. Five headers produce 106 rows when
every act is discovered. Model identifiers are `EnemyId`; encounter entries
use `EncounterId`, not current battle slots. This is a catalogue description,
not a restoration percentage.

Ordinary entries require a discovered act and an observed enemy victory.
Unknown entries remain disabled. Clicking selects an entry; hovering does
not. Keyboard focus and confirmation remain distinct. Character filters
require observed wins or losses. A Decimillipede entry uses the encounter
result, not an individual segment reaching zero HP.

## Layout and typography

The previous uniform 14px list, large action text and generic close button
have been replaced with the authored screen hierarchy:

| Element | Source contract | Native rendering |
| --- | --- | --- |
| Right list | 400px sidebar; 280px labels; 120px top/bottom inset | x244–297, y23–217 |
| Entry / divider | 36px / 64px heights, 4px separation | projected cumulative row offsets |
| Monster title | centered Spectral Bold48, y24→88 | face4 at10px, 500ms ExpoOut entrance |
| Action / mode labels | Kreon Bold28 | face5 at5px |
| Act / event dividers | Kreon Bold32 | face5 at6px |
| Move labels | Kreon Regular32, 56px row | regular6px, independent focus/press scaling |
| Statistics | original STATS.layout; authored literal Stats header | regular5px; bold5px header |
| Dialogue | original character quote and fallback | regular5px, original italic fallback at face6/5px |
| Mode control | x94, bottom−124, 260×58 | x18, y216, 50×11 |
| Back | shared source Back control and entrance | existing native Back action |

The original scene's `Stats` heading is a literal in both locales:
`NBestiary._Ready` localizes the actions heading, but does not replace it.
Hidden epithets remain hidden. Dialogue does not fabricate missing character
quotes. Known TestSubject titles use the separately persisted death counter
implemented in the progress codec; enemy victories are not substituted.

Entry focus uses the source 50ms enlargement and 300ms return. Move buttons
use the 50ms focus, 200ms press, 300ms bounce release and green release tint.
The mode control uses its separate 50ms focus, 250ms press and 500ms ExpoOut
unfocus; its source class does not add a release scale tween. Focused labels
resample a bounded foreground glyph tile instead of selecting an unrelated
larger font band. The tile is temporary stack storage, not a new resident
cache. Scroll and selection pointer calculations use real variable row
heights.

The glyph converter adds only reachable title and label characters at the
four newly required face/size pairs: 4/10, 5/5, 5/6 and 6/5. It does not add
all monster text at every size. The font-only checkpoint is
`4e8a6e5377eed0b869de24a829555a5259377e7e`; final pack generation and the
runtime directory-key checks establish actual coverage within the unchanged
32768-glyph limit.

## Input and actual animation playback

`CreateBestiaryMoveButton` binds actions on release. The implementation
captures the resolved action and current selection identity on key-down.
Key-repeat, rebinding while held, selection changes, closing and host focus
loss cannot release into a different preview. Mode and Back also execute on
release. Ordinary directional navigation still repeats while held.

| Original input bank | Implemented mapping |
| --- | --- |
| Mouse and keyboard | Card1–Card9 commands, normally numeric1–9 |
| Keyboard only, actions1–5 | Deck, Exhaust, Discard, Draw, TopPanel; default Q/F/D/A/W |
| Mode | Confirm command; default E or Enter according to bank |
| Statistics filters | Deck / Exhaust commands |
| Entry / action list | existing directional focus plus Select |

The source's remaining `ui_alt_*` commands are alternate controller
directions; neither pinned keyboard map binds them. This port does not
invent an Alt+arrow keyboard mapping or consume the calculator's directional
keypad as extra numeric shortcuts. All seven actions in the largest current
move list remain reachable through list focus and Select. Existing desktop
Enter selection and calculator directional adapters remain available.

Actions call the detached preview animation state machine and its real
registered clips. Nibbit hides `SLICE_MOVE` in the source, so its four buttons
play Butt, Hiss, Hurt and Die. Crusher's seven actions exercise five actual
move animations plus Hurt and Die. Selecting another entry resets its
detached actors, and a Die preview holds its final frame. None of these
operations changes canonical `State`, gameplay RNG or run saves.

Default actors use original sprite registration independently of hitbox
size; the hitbox supplies the source vertical placement. Decimillipede uses
its translated encounter markers. Kaiser uses the original shared layout's
0.75 scale and per-arm translation, including the source division by0.75.
The existing shared world frame is cropped. Its missing pixels are supplied
by the bounded NSP34 extension below. Full detached phase-dependent gameplay
contexts and all move-specific VFX are still distinct from playing the
registered clips.

Mode preference is written when the submenu closes, matching
`OnSubmenuClosed`, rather than on each toggle. Preview motion, pressed keys
and list geometry remain presentation-only. `BestiaryPresentation` is
1680 bytes on the native compiler, an increase of392 bytes from the preceding
1288-byte structure. No canonical save field or resource cache was added.

## Stable source ordering

`NBestiary.AddEntries` compares room type, boss encounter title and then
entry title with CurrentCulture. `NGame._EnterTree` sets the default thread
culture to **InvariantCulture**, unlike the locale-specific card-library
ordering contract.

The two generated 101-byte order tables replace runtime insertion sorting.
An independent .NET comparer and the generator's ICU root collation agree.
The historical checkpoint-6 source-order receipt is
`ui7-bestiary-invariant-reference-formatted-0f3c6dbdb4c1`; its preceding
dynamic-title rejection remains recorded as
`ui7-bestiary-invariant-reference-22b7799b0904`. Six formatted TestSubject
death-count boundaries per locale did not change the order. The current
runtime performs one bounded catalogue pass and uses202 read-only order
bytes. This describes removed work; it is not a measured latency claim.

## Acceptance status

The recovered `3e61c041ffdc02e82ea8f8e14dc27601e2751aac` full native suite
passed113/113 before the layout/input work above. Its result is a baseline,
not acceptance of these changes.

Current source passes Clang18 syntax checks for App, UI input, pointer,
map-preview input and the expanded Bestiary regression. An independent short
header check passed18 layout/motion assertions, including clock wrap and
the separate mode-release behavior. These do not replace the next matching
native/resource run.

`bestiary_ui7_regression` now checks all106 ordered rows; native source
geometry; old and new Back semantics; preference-write timing; true clip
selection for all four Nibbit and seven Crusher actions; keyboard-only
TopPanel mapping; repeat/down/up behavior; focus-loss and selection-change
release isolation; canonical byte purity; and actual glyph keys for every
reachable title and required source font band. It captures ordinary action,
statistics, Decimillipede, both Kaiser arms and the seven-action layout.
The full matching run and both regenerated-language captures remain pending
at this source checkpoint.

## Kaiser crop completion, NSP34

The shared world frame's alpha touches its left, top and right edges. Merely
translating that320×218 frame into the Bestiary exposes an artificial edge.
The dedicated converter samples the original skeleton outside that crop,
keeps only pixels visible in the chosen Bestiary view, and uses exactly the
same nearest-neighbour coordinates as the retained world picture. Samples
already owned by the world rectangle have zero supplementary alpha. This
does not move or paint over the old edge to hide it.

NSP34 appends18 clips at1765–1782, containing344 frames. They cover the two
actors' five actual move choices, Hurt, Die and required idle/charged/rest
loops. Each new directory entry copies its source clip's count and frame
interval; there is no frame removal, altered endpoint convention or reduced
display resolution. The extra `sprite_units` word is used only in these
supplementary clips as source-directory-plus-one, never as a sprite scale.
Its pivot places the tightly bounded native patch directly on screen.

The original1765 clips,2061 auxiliary IDs,8704 strings, glyphs and all old
art payloads remain intact. Grouped glyphs, grouped strings and TGR2 retain
their complete relocation semantics. The renderer adds one native sparse
image through the existing image and retained-group caches; it allocates no
new resident buffer or gameplay state. Older resources remain readable.

The first private source measurement sampled both idle poses and confirmed
the crop deficiency. Its initial script rejected an unsupported Pack context
manager before sampling; the corrected run passed under journal
`ui9-bestiary-kaiser-patch-measure-retry-c736372e97a0`. Those two older-grid
LZ4 measurements are not a final installation-size estimate. Full patch
baking and retained-group compression must establish the final byte budget.

Three focused synthetic converter tests pass: exact344-frame source clocks,
partitioning every native pixel across all crop boundaries, and append
relocation preserving old TGR2, grouped glyphs, grouped text, frames and alpha.
The native Bestiary regression checks every actual patch frame excludes the
old world rectangle and verifies opaque supplementary pixels through real
App renders for both arms. The matched full native/resource execution is
pending. Bilingual installation accounting adds the compressed artwork once
and the6136-byte new prefix to each locale; it does not count those two
directories as shared artwork.
