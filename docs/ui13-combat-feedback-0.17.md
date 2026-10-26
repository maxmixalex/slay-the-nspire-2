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

## Resource integration

`tools/combat_ui13_assets.py::source_replacements` re-samples Draw, Discard and
Exhaust directly from their original textures at 24×24. It replaces image IDs
24, 25 and 26, keeping all auxiliary and clip IDs stable. This removes the
previous 18→24 pixel enlargement and adds no new asset slot. The returned PNG
mapping is compatible with `tools/ui12_resources.py::replace_images`.

## Validation status

The modified runtime and affected regression translation units passed the short
Clang 18 syntax gate. Focused executed gates and calculator acceptance are
recorded by the integrated UI13 checkpoint; syntax checks alone are not a
gameplay, visual or device-performance pass.

The regressions exercise actual 0–9 physical input in both banks, repeated-key
ownership, mechanical-arrow navigation, saved-bank migration, 16-slot potion
geometry, complete localized turn-counter glyphs, source orb geometry and the
actual-pack highlight's visible extent. Presentation checks preserve canonical
State/RNG. Merchant performance and Boss re-sampling are separate integration
work; these combat changes make no unmeasured frame-rate claim.
