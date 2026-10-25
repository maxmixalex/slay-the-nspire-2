# Bestiary source layout, input and previews

Reference: **v0.111.0 / 41cef1ea**. Source-backed implementation and the
matching acceptance gates are distinguished below. Original source, prose,
textures and captures remain private.

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
The existing shared world frame is cropped; final actual-pack captures must
check that this reused registration covers the displayed arm after the
Bestiary translation. Full detached phase-dependent gameplay contexts and
all move-specific VFX are still distinct from playing the registered clips.

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
