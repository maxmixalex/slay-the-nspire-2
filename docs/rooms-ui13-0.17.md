# UI13 rooms, card modification, and final encounter

Reference: the owner's v0.111.0 / `41cef1ea` PCK and independently inspected
decompiled source. Original textures, generated source frames, the runtime,
and complete resource packs remain private inputs/outputs.

## Room layout and input ownership

The reward ribbon places its title in the visible ribbon band. Reward row text
and the marquee use the same vertically centered baseline. Proceed stops using
a residual keyboard selection when the pointer is visible, so its existing
source focus/unfocus animation follows actual pointer entry and exit.

The default event keeps a 104 px left hover lane and a 186 px option column.
`event_button.png` and `event_button_outline.png` now retain their corner and
edge proportions through a nine-patch; the interior can grow without stretching
the bevel horizontally. Fifteen pixels of horizontal padding and six pixels of
vertical padding belong to each button. Long text is clipped and scrolls inside
that region, and its complete description remains available in hover help.
Related cards use a 69 × 96 face at the right edge of the hover lane. Their
keyword descriptions occupy a separate region below the card and paginate
without overlapping either the card or another tooltip. The Wood Carvings
event is included in the existing event-hover regression.

Enchant selection uses two columns, a separate scrollbar at x = 187, and a
fixed 104 px explanation pane at x = 212. The original rich title and body remain
available; long localized bodies scroll in that pane. Single before/after and
multiple selection previews remain to its left.

Grid painters explicitly relinquish focus and hit regions while the selection
confirmation owns the screen. The HUD drawing scope similarly relinquishes its
pointer, keyboard focus and newly created hit regions during confirmation. This
covers the top bar's manually calculated focus as well as ordinary card-grid
hover. Preview cards and confirmation controls keep their own interaction.

The rest-site focus outline now uses the authored outline-to-button size ratio,
instead of applying the old extra 55 × 39 enlargement to an already scaled
button.

## Enchantment tab and extra card text

Source owners: `NCard.cs`, `card.tscn`, enchantment models' `HasExtraCardText`,
`ExtraCardText`, `ShowAmount`, and the enchantment material.

The original `card_enchant_s.tres` tab, original enchantment icon, amount policy,
disabled modulation, and the source 45-unit no-star offset are rendered on the
card. The icon and tab do not become an extra paragraph in every description.
Only models that explicitly supply extra card text append their localized
`.extraCardText`; ordinary Sharp/Imbued-style explanations remain in hover help.
The same distinction applies to afflictions. Inky is represented by the same
live-card marker path.

Small-card and hand-tile caches include kind, amount and disabled state in their
enchantment key. Identical base cards with distinct enchantments cannot reuse
one another's rendered marker.

## Dedicated card transformation

Source owners: `NCardTransformVfx`, `card_transform_vfx.tscn`, its source particle
materials, anticipation/reveal curves and child textures. Scene properties
override constructor defaults and determine the implemented schedule.

| Source cue | Time from a card's own start |
| --- | ---: |
| Enter center | 250 ms |
| Mask/shine begins | 750 ms |
| Swap old model for transformed model | 1125 ms |
| Shine particles | 1225 ms |
| End particles | 1390 ms |
| Fly to deck | 2190 ms |

The before and after models retain their own card facts, enchantments, genetic
counter, scythe counter and effective costs. Formatting a detached snapshot
does not mutate a canonical deck entry. The normal acquired-card flight carries
that snapshot when the transform releases it to the top-bar deck.

Transformation is captured from real deck-removal and `CardTransformed`
observer notices, including batch transformations and legacy transformation
selectors. A dedicated presentation queue replaces the old removal visual.
The renderer uses the original card mask, border glow, nine-frame shine,
specks, flare and polar ring. The small bounded particle sampler preserves the
source cue times, emission shapes, colors and motion families. Its deterministic
CPU turbulence is a calculator adaptation; it is not a claim of GPU noise
equivalence. Up to three cards form a visible group, with 100 ms card staggering
and 2500 ms group spacing so large valid batches remain legible.

Auxiliary images 2204–2227 are emitted by `tools/rooms_ui13_assets.py`.
There are 24 source-owned images and no added animation-directory entries.
The queue is bounded by `MaxDeck`; three reusable description slots replace
per-card paragraph buffers. No additional framebuffer is retained.

## Architect reading, impacts and writing trail

Source owners: `TheArchitect.PlayCurrentLine`, `AnimPlayerAttackIfNecessary`,
`DivideWildly`, character `GetArchitectAttackVfx`, `NArchitectVfx`, `NBasicTrail`,
`architect.tscn`, and the original Architect Spine clips and track 1 animations.

The actor manifest includes the `_Ready` track 1 `_tracks/head_normal` layer.
Existing attack/hurt/idle clip IDs remain 49–51; resampled clips carry their own
pixel registration. The final-room renderer applies that registration in the
same way as combat actors.

The first conversation has three sampled body/head combinations: ordinary
reading, Silent Normal, and Silent Fast. Each retains the original simultaneous
body clip, four Silent hurt interruptions where applicable, the reading loop,
the actual stop-reading cue, and the later normal-head transition. They end at
2250, 3950 and 3750 ms respectively. Save reload does not replay an unrecorded
transient entrance.

Actual event attack notices capture the source score with `won: true`. The
source shuffled attack list and large/small/ordinary `DivideWildly` weights are
sampled from a presentation-local copy of the event stream. Score-derived
damage numbers and the existing original NHitSpark sampler are emitted at each
real source hit time with the room clock. Camera-shake settings do not determine
whether these effects exist. The presentation copy advances independently so
repeat rendering never advances a canonical random stream.

The writing trail begins on `trail_start` at 200 ms and ends on the following
idle clip's `trail_end` at 2167 ms. The sampler follows the actual `trail_slot`
at 60 Hz, keeps thirty points, and uses the source outer/inner widths, width
curves, gradient, textures, UV modes and outer noisy dissolve texture. Normal
and additive planes remain distinct. The line uses bounded CPU coverage with
two samples per LCD axis; the source engine's hardware antialiasing and its
variable display frame rate are not reproduced.

`tools/architect_ui13_assets.py` emits 199 reading frames and 80 trail-plane
frames as sparse 320 × 218 images, auxiliary 2228–2506. This stores only covered
pixels after packing and does not require another resident scene buffer.

## Verification status

Short `clang++-18 -fsyntax-only` checks cover the App implementation and changed
selection, keyword, deck-selection and Architect regressions. The four modified
selection tests cover actual commit/cancel operations, frozen transform facts,
separate removal/transform state, enchantment marker cache identity, sidebar
geometry, tooltip non-overlap, and unchanged save bytes during animation.

Matched-pack execution, source-image visual review, full native gates and
calculator execution are separate release gates coordinated in `WORK_STATE.md`.
Syntax checks alone do not establish pixel fidelity, a successful full run or
physical-calculator frame rate.
