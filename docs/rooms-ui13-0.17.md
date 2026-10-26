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
When a card owns hover, its complete keyword set takes over this same column
and includes the pending enchantment exactly once. Associated cards and paged
text remain in the column, above the confirmation control. This removes the
duplicate fixed-pane/preview-tooltip overlap found in actual English/Chinese
capture review.

Grid painters explicitly relinquish focus and hit regions while the selection
confirmation owns the screen. The HUD drawing scope similarly relinquishes its
pointer, keyboard focus and newly created hit regions during confirmation. This
covers the top bar's manually calculated focus as well as ordinary card-grid
hover. Preview cards and confirmation controls keep their own interaction.

Combat hand selections also preserve the current frontmost holder while the
pointer remains inside it. Actual-resource testing caught a keyboard-to-pointer
transition where the press-triggered redraw changed overlapping cards' order:
the visible hit was card 9, but the press captured card 8. Keeping the current
holder before the ordinary overlap fallback fixes that real wrong-card click.
The original Toasty Mittens pointer-selection assertion is retained.

The rest-site focus outline now uses the authored outline-to-button size ratio,
instead of applying the old extra 55 × 39 enlargement to an already scaled
button.

The fake merchant's enlarged dialogue bubble uses the inventory's empty upper
left corner. Its previous centered placement covered the first relic and its
price: a real 53-to-54 price change was completely hidden by the bubble. The
original one-gold pixel-change assertion remains in the regression.
The later [custom Fake Merchant scene](fake-merchant-ui13-0.17.md) restores its
complete original room layers, five relaxed actors, particles, focus skin and
post-fight variants. Its final English/Chinese inventory gate retains that
same one-gold assertion over the live animated room.

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

Source owners: `NCardTransformVfx`, `scenes/vfx/ui/card/vfx_card_transform.tscn`, its source particle
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

`tools/architect_attack_assets.py` and `architect_attack_particles.py` sample
the thirteen character attack effects and `NFireBurstVfx` from their own scenes.
They preserve the two Spine effects, the four-frame Slash, the five-frame
Lightning, and the GPU effects' original textures, flipbooks, LUTs, curves,
subemitters and source lifetimes. The 824 auxiliary rows occupy 2507–3330.
Normal, additive and screen-displacement data remain separate where the source
requires them. Flying Slash has an independent horizontal pivot so its long
arrival trajectory is not prematurely clipped by a local image canvas.

The Heavy Blunt and Starry Impact distortion planes encode signed integer
pixel offsets in RGB565, with a maximum 15-row lookup distance. Their source
textures and shader masks determine those offsets; the runtime samples the
untouched framebuffer through a 32 × 320 row window (20 KiB). These data planes
must be packed losslessly, without the visual 454a6 precision reduction. Fire
Burst follows the player's original bottom-of-hitbox anchor and is covered by
the final screen's black backstop, while the pen trail remains with its retained
creature node above that backstop.

## Initial UI13 actual-resource acceptance

All eight room/selection gates have passed with the original English and
Simplified Chinese UI13 resources. The receipts distinguish the initial UI13
pack from the final pack whose unused tile-directory records were removed;
the pruning preserves image pixels, frame IDs and clocks. This initial acceptance
uses production source `d5f9a3c`; its last Architect test-only checkpoint is `b96bd0d`, with
the same production files and core library.

| Gate | Source checkpoint | English checks | Chinese checks |
| --- | --- | ---: | ---: |
| World selection, real confirmation/cancellation and transforms | `d8a1025` | 1,396,392 | 1,436,724 |
| Deck selection, real removal/upgrade and merchant preparation | `d5f9a3c` | 551,382 | 591,714 |
| Keyword and event-associated hover pages | `d8a1025` | 635,846 | 635,881 |
| Card keywords, enchantment facts and original tab pixels | `d8a1025` | 486,263 | 486,263 |
| Rewards, direct claims and animated Proceed | `d8a1025` | 4,076 | 4,076 |
| Events, fake merchant and ancient conversations | `d8a1025` | 5,841 | 5,818 |
| Rest, treasure, event and merchant room UI | `6e40566` | 14,689 | 14,689 |
| Architect, five characters × Normal/Fast | `b96bd0d` | 84,365 | 84,365 |

The final deck gate additionally passes its 453-check resource-independent
command/geometry fixture. It always waits for merchant preparation to finish;
I/O health is asserted when a resource source is actually open. The original
fake-merchant one-gold price-change pixel assertion passes. Actual screenshots
also confirm a single enchantment sidebar, an unobstructed confirmation button,
source marker pixels, separate transform masks, centered reward labels,
contained event text and the thinner authored rest outline.

The final actual `world_ui_regression` passes 31,796 checks per language,
including the unchanged Toasty Mittens click contract, Normal/Fast delayed
choice barriers and the original 57 × 102 merchant stock geometry.

The Architect gate covers all twenty character/speed/language combinations,
real final-room entry, authored dialogue and scrolling, separate source hit
effects, immutable canonical saves, and terminal two-actor composition. Large
Starry Impact coverage required a complete compositing oracle: independent
source anchors and literal attack/hurt frames are composed over the original
background before effects, and every opaque source-actor pixel must match.
The separate alpha probe is not used as the compositing surface, so a warp also
samples the correctly blended antialiased edges. Player death texels remain
checked above the lower FireBurst layer. None of these pixel comparisons has
an allowed mismatch count. All thirty final captures loaded as complete
320 × 240 images; closed journals and copied artifacts have matching hashes.

Actual startup testing also caught a capability-chain omission after the new
catalogs were appended. `Content::open` could read them, but `App::init` rejected
their profile. That stage's five explicit UI13 endpoints inherit the earlier
sleeping, intro-logo, history, selection and tutorial capabilities. Both direct
and ranged loader tests assert complete playable profiles while incomplete or
unknown suffixes still fail.

Private Architect and Vantom entry saves are supplied for separate calculator
display validation. Both use production world-room entry, validate, and
round-trip through the real save codec. Prior room outcomes are supplied by
the fixture and their history is explicitly partial; neither fixture is
evidence of a naturally played three-act victory. The Vantom fixture enters
Overgrowth's original boss on seed `UI13-VANTOM-4`, floor 17, with unmodified
Ironclad 80/80 HP and Vantom 173/173 HP. No enemy slot, HP or move-node editing
is used to construct that encounter.

Calculator execution and the complete release suite remain separate gates
coordinated in `WORK_STATE.md`. These native receipts do not claim a physical
calculator frame rate or GPU noise equivalence.

## Earlier diagnostic receipts

Frozen checkpoint `7bfc391` built the eight room/selection regressions. The
asset-independent selection, deck-selection and keyword gates passed, and the
Architect's independent geometry, immutable-row distortion and real event
control gate passed 79,351 checks. With the actual UI12 resources, English and
Chinese keyword gates passed 635,846/635,881 checks, reward gates passed 4,076
each, and card keyword/marker-fact gates passed 409,460 each. Architect's five
characters × Normal/Fast × both languages passed 84,251 checks per language.
These actual-resource gates prove the existing actor/body/death registration,
source text, selection facts and save invariance; they do not claim the newly
appended UI13 particle images have been accepted.

The first actual selection attempt exposed obsolete 4px/5px UI9 font assertions.
Checkpoint `8c8c900` updates their independent full-label pixel oracle to the
current 8px label, fractional advances, source kerning, negative bearings and
one-pixel shadow. It still rejects missing codepoints rather than accepting a
question-mark fallback. Actual English/Chinese world-selection passes then
reached 1,396,387/1,436,719 checks and supplied the screenshots that exposed the
duplicate enchantment pane above. The older deck/room merchant fixtures also
needed to advance the real yielded preparation frames before timing inventory
entry; waiting a fixed interval with no rendered loading frame never completed
that path. Failed and launch-error journals remain separate from passing runs.

The modified gates cover actual commit/cancel operations, frozen transform
facts, separate removal/transform state, enchantment marker cache identity,
sidebar geometry, tooltip non-overlap, and unchanged save bytes during
animation. Short `clang++-18 -fsyntax-only` checks pass for the follow-up
duplicate-pane and fake-merchant bubble fixes.

Those earlier syntax/old-pack receipts are retained as development evidence;
the actual-resource acceptance above supersedes their pending room, profile
and compositing checks.
