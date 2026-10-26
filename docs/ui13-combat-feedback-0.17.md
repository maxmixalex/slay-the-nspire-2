# UI13 combat and calculator feedback

Reference: the owner's pinned v0.111.0 / 41cef1ea assembly and scenes. Original
files and generated images remain private. This document supersedes the UI12
resource-rail, pile-placement, highlight-container and HUD-layout choices.

## Implemented behavior

| Area | Cause and correction |
| --- | --- |
| Playable-card blue glow | `card.tscn` puts a square SDF texture in a 759×951 `TextureRect` with `KEEP_ASPECT_CENTERED`. The old renderer stretched the texture to the rectangle height, giving the top a large false extension. Both the hand and selection renderer now use the actual 759×759 image, vertically centered with a 96-source-pixel inset. The source ripple clock, field and color remain intact; the existing LCD half-width remains. |
| Player-turn missing letters | The 7px font band is a sparse scene alphabet and omits the Chinese turn-count characters. The counter uses the complete 10px band and remains centered beneath the title. |
| Health and block numbers | Both labels again request the thin four-sided colored stroke. The glyph is resolved once and reused for all stroke passes. |
| Energy and Stars | The source Regent control puts Stars 36 source pixels left and 62 below the energy origin. The readable LCD controls now preserve that close diagonal relationship. |
| Exhaust pile | The right rail places Exhaust below End Turn and above Discard, wholly outside the normal 1–10-card fan. End Turn, its glow and hold bar move up together without changing art size. Exhaust's original purple icon carries the pink count in the center. The red Draw/Discard count badge no longer covers it; those badges regain their original 48:80 ratio (15px on a 24px pile) and fit wholly inside the 240px height. Card-flight destinations share the same constants. |
| Player thought bubble | The last smoke puff is anchored to the right side of the player's body instead of ending to its left. The body retains its original smoke source and bounded display lifetime. |
| Top bar | A single geometry owns potion cells, popup centers, pickup landings and paging. The fourth/fifth cell compacts slightly, leaving equal 4px gaps between the potion belt, Room, Floor, Boss and Timer. All five visible slots remain accessible, including inventories with 16 slots. |
| Timer | The existing original clock image returns. The elapsed label is drawn when it fits; its clock tooltip and keyboard focus always expose the complete value. The main Map phase joins overlay maps in the original always-visible map rule. |
| Character / Ascension hover | The source `PORTRAIT_DESCRIPTION` uses a ` +` prefix for each localized Ascension title in all 16 pinned locales. The hover now matches that list and adds the original achievement-lock text for Custom/Daily. A Standard Ascension-zero run has no portrait hover, matching `NTopBarPortraitTip.ShowTip`. |
| Physical digits | Defaults keep all ten digits as card-slot shortcuts and use the mechanical touchpad arrows for navigation. Each completely untouched historical binding bank upgrades independently. A customized bank is preserved byte-for-byte. The former unbound `5` acceptance fallback is removed; Enter or touchpad center accepts pointer-mode controls, and Catalog opens card details. |
| Card cache identity | The hand cache includes the enchantment marker state and accepts an explicit detached description for before/after transform snapshots, matching the room-flow implementation without another large bitmap cache. |

## Original source checks

- `scenes/cards/card.tscn` and `shaders/card_ripple.gdshader` establish the
  highlight's aspect fit and original shader inputs. A private pixel comparison
  of the same packed SDF shows the old tall extension and the corrected narrow
  edge at the full readable hand size.
- `scenes/combat/energy_counters/star_counter.tscn` and
  `NCombatUi.Initialize` establish the Regent resource offset.
- `scenes/combat/exhaust_pile.tscn` uses
  `images/packed/combat_ui/exhaust_pile.png`, a centered pink count and no
  separate red count texture. `NCombatCardPile` does not add such a texture.
  The red count art is the separate Draw/Discard scene element. This distinction
  was checked against the actual original assets, not inferred from screenshots.
- `AscensionHelper.GetHoverTip`, `NTopBarPortraitTip.Initialize` and
  `GameModeExtension.AreAchievementsAndEpochsLocked` establish the hover text
  and visibility. Every original `localization/*/ascension.json` has the same
  `PORTRAIT_DESCRIPTION` list punctuation.
- `NOrbManager.TweenLayout` uses maximum capacity, not occupied count, for its
  −150° to −25° arc and its 225→300 source-pixel radius. The current capacity
  formula already matches the pinned source. `NCreature.SetOrbManagerPosition`,
  `NCreatureVisuals.OrbPosition` and `scenes/orbs/orb_manager.tscn` establish the
  IntentPos fallback and the additional 195px child offset. The shared world
  projection already applies the scene lift; an extra subtraction would be
  incorrect. The regression now independently checks capacities 1–10, occupancy
  independence and continuity during capacity increases/decreases.

## Orb object lifetime and Doom completion

The static orb arc was already correct. Original-resource renders at capacities
1, 2, 3, 5, 7 and 10 confirmed that occupied count did not change it. The real
missing behavior was `NOrbManager.EvokeOrbAnim`: it removes one **object**, fades
it for 250 ms, shifts the remaining objects for 450 ms with Sine.InOut, and puts
a new empty holder at the origin. `AddOrbAnim` replaces that holder at its
current position and restarts the layout. This now runs from explicit
`Removed`, `Channeled` and `Capacity` observer events. A repeated activation
without dequeue cannot shift the queue. Future beam origins are predicted from
the same bounded presentation facts; no gameplay RNG is consumed. The 128-cue
object, ten holders and ten fading remnants occupy at most 2 KiB.

Doom now has its separate visual family, rather than only a death clip and
camera punch. The pinned references are `DoomPower.PlayVfx/StartDoomAnim`,
`NDoomVfx.PlayVfx/PlayVfxInternal`, `NDoomSubEmitterVfx.ShowOrHide` and
`NDoomOverlayVfx`, plus `scenes/vfx/vfx_doom.tscn` and its flame material.

- A disappearing creature triggers the source Hit transition. Only a clip
  named exactly `hurt` freezes at 100 ms, or 40 ms for Inklet. Other tracks
  continue with their original loop behavior. The detached body retains its
  registration and the source 1.5× hitbox viewport bounds. The original `hsv.gdshader`
  actually performs YIQ desaturation and its inverse-matrix reconstruction;
  it does not use maximum-channel HSV gray. After a 750 ms
  hold, an Expo.In tween sinks the viewport by its height over 750 ms.
- `ShouldDisappearFromDoom` exceptions remain visible for Ceremonial Beast,
  Crusher, all Decimillipede segments, Eye With Teeth, Parafright, Rocket and
  Vantom. Test Subject disappears only after its two respawns; Waterfall Giant
  depends on Steam Eruption. Player Doom emits particles without detaching or
  freezing the player's normal death animation. The last ordinary enemy waits
  1500 ms before the displayed kill; other disappearing enemies wait 250 ms.
- The original front/back hole, glow, floor fill, globs and eight moving spears
  use eight private source textures. Their emitters retain separate lifetime,
  gravity, scale-curve and width-tween behavior. The source flame is animated
  at runtime with its assigned noise, distortion, blue-channel mask and two
  step bands. Both distortion samplers read the assigned original noise at their separate
  scales and pans; Noise1 and the distortion mask retain the source white
  defaults. Shader time is global, so another Doom does not reset its phase.
  The LCD samples
  overlapping subpixel particles at a bounded density rather than allocating
  Godot particle buffers or a viewport.
- The YIQ overlay eases in for 1000 ms, holds for 1200 ms and fades over 500 ms.
  A later actual Doom cue restarts it from current alpha. Future synchronous
  commands do not prematurely suppress an earlier visible overlay. The weak
  300 ms camera punch now has independent cosmetic ±10-degree jitter.
- Detached bodies and effects render in the foreground after the shaken scene,
  matching `CombatVfxContainer`. Kaiser shared frames are clipped per affected
  body instead of copying the whole background actor into the Doom viewport.
  The ordinary game-over branch retains the player's particle tail.
- `NScreenShake.ClearTarget` now retains decaying trauma while clearing the
  target's punch/rumble and queued cues. Clock rebasing preserves the amount
  across combat and room clock domains. Loading resets transient Doom actors.

The Doom registration object is at most 384 bytes. This stage adds no canonical
State field, save-codec field, new framebuffer, heap allocation or gameplay
random draw. It reconstructs the named source behavior; desktop GPU bloom,
continuous particle density and exact noise pixels are not claimed.

## Resource integration

`tools/combat_ui13_assets.py::source_replacements` re-samples Draw, Discard and
Exhaust directly from their original textures at 24×24. It replaces image IDs
24, 25 and 26, keeping all auxiliary and clip IDs stable. This removes the
previous 18→24 pixel enlargement and adds no new asset slot. The returned PNG
mapping is compatible with `tools/ui12_resources.py::replace_images`.

`tools/doom_ui13_assets.py::source_assets` appends source textures 3331–3338,
ending at 3339. It checks every source path against the pinned Doom scene and
returns the ordinary image manifest rows. The shared UI13 resource assembler
owns all language-pack integration; the effect needs no localized texture.

## Validation status

The first combat stage was executed with synthetic fixtures and the actual
UI12 English/Chinese resources. Combat UI passed 983 synthetic / 12,688 real
checks; secondary effects passed 325,271 / 390,412; potion UI passed 126 /
1,581 per locale; end-turn hold passed 1,731 / 155,338 per locale. The health
renderer passed 82,236 independent checks. The corrected LCD-input fixture
passed real English/Chinese gates (31,901 / 31,898). Raw Q6 font metadata and
an old 5px Rainbow assumption were corrected in test oracles; the current card
body keeps its readable font floor and verifies full-description access.

The final runtime checkpoint is
`d5f9a3c9dfc9bb55e4742239dbb7be4ee5d8c8d1`. The final verification checkpoint
`78a2cef775ddb6943baf194f7bc9e0e8c568edf8` retains the same production files
and corrects the remaining old hover-layout test oracles. Its tests link the
matched runtime core. All six targets now pass with the final UI13 release
resources:

| Target | Synthetic checks | Final original-resource checks |
| --- | ---: | --- |
| Card / HUD UI | 385,861 | English 712,879; Simplified Chinese 790,241 |
| Combat UI | 1,260 | English 154,880; Simplified Chinese 154,880 |
| World UI | 7,744 | English 31,796; Simplified Chinese 31,796 |
| Card movement | 1,106 | Asset-independent movement and 5,540-byte metadata checks |
| Combat secondary / orb lifetime | 325,304 | Split English pack 390,445 |
| Screen effects / Doom / trauma | 1,993,453 | Split English and Japanese masks 2,122,313 |

The final static-hover test independently checks the existing readable
104px column plus its one-pixel shadow, resource/orb ownership, the Settings
tooltip's exact localized pixels including a scrolled General row, and all six
map-legend rows inside their padded paper. It does not read the production
geometry constants to manufacture expected bounds. The original d5 test run
retains its two failed Card/HUD cases as evidence of the obsolete 69px and
pre-scroll Settings assumptions; the complete final Card/HUD run supersedes
those cases without changing the runtime.

The private closed receipts are
`combat-ui13-acceptance-gates-9c1864ea6171` for the first five targets and the
initial Card/HUD failures, and `combat-cardhud-complete-gates-845c772a6e0a`
for the completed synthetic/English/Chinese Card/HUD runs. The latter and its
build receipt `combat-cardhud-complete-build-362b99bcc808` are closed with
exit code zero and unchanged source inventories. The final Card/HUD evidence
includes lossless 320×240 captures and hashes of their original PPM data,
converted PNGs, executable and input packs.

`combat-ui13-release-frames-71a887841aae` separately executes real
`DEvoke` with two activations before one dequeue, a two-slot `DSlots` increase,
and an `EndOfDays` card play in the
Nibbits encounter using the final English release pack. It captures the orb
queue transition and expansion through 462ms, and Doom from activation through
3531ms. Every command and captured time is compared with canonical save bytes
from the same commands without the UI observer. The closed run passes with
unchanged sources; the capacity-five layout and the Doom sink/flame frames
were also visually inspected. This is actual command execution and rendered
pixel evidence, not a geometry-only probe.

The regressions exercise actual 0–9 physical input in both banks, repeated-key
ownership, mechanical-arrow navigation, saved-bank migration, 16-slot potion
geometry, complete localized turn-counter glyphs, source orb geometry and the
actual-pack highlight's visible extent. Presentation checks preserve canonical
State/RNG. Merchant performance and Boss re-sampling are separate integration
work; these native checks do not claim Firebird or calculator frame rates.
