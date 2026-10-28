# UI11 combat secondary presentation

The source reference remains v0.111.0 / 41cef1ea. This stage closes several
explicit UI8 retained differences through bounded CPU effects. Its native
checks are correctness evidence; only the ordinary ARM/Firebird session may
establish calculator performance or playability.

## Actual triggers and visible behavior

- **Hand highlights:** the existing original scalar/color distance-field
  resource now follows the hand holder's affine transform. Playable cards use
  the source cyan; source `ShouldGlowGoldInternal` and `ShouldGlowRedInternal`
  conditions cover enemy poison/vulnerable/attack intent, exhausted/lost-HP/
  Doom/Osty counters, hand/pile conditions, FTL, Heavenly Drill, Fetch,
  Normality and Enthralled. The half-second width transition and selected
  card's 100 ms expansion / 350 ms return are bounded display state. The
  shader samples only exposed edge/corner pixels, not the opaque interior.
- **Exhaust:** actual transfers to Exhaust retain their last holder pose,
  use the source four-point 400 ms progress curve, dissolve the visible face,
  and keep short rising embers after the face disappears. They no longer
  vanish at the pile update or masquerade as a discard flight.
- **Power cards:** actual transfers to Power use all seven source swoosh
  nodes, sampled in fixed point. The final node targets the owner's actual
  VFX center, travel follows squared progress and source path length / 3000,
  scale reaches 0.1 over 300 ms and vanishes in the final tenth. The sampled
  curve is an explicit approximation to Godot's baked arc-length lookup.
- **Discard and power trails:** six recent actual card positions retain a
  short trail in the character's source color family. At arrival emission
  stops, and the retained positions fade over 500 ms. No framebuffer or
  historical card image is allocated.
- **Shuffle:** the real Shuffle log creates one aggregate pile-to-pile card
  back/trail, following the original random arc range. It is not one extra
  visual per canonical card. The card-back record uses a display-only
  sentinel and never enters a gameplay pile or saved card array.
- **Direct generated hand cards:** a new-instance transfer retains its own
  grow-in/brief particle feedback instead of being assigned a fabricated
  draw-pile origin. Existing enemy-generated preview batches stay intact.
- **Orb activation:** a new final `CoreObserver` callback records the exact
  activating kind, pre-removal slot, slot count, actual selected enemy mask
  and passive/evoke flag. Every real passive path uses it, including extra
  WorldStage activations. MultiCast/Dualcast generates one notice per evoke,
  even when only the final evoke dequeues the orb. All five kinds have local
  pulses; damaging evokes also use the actual target positions. The callback
  neither selects targets again nor consumes a random number.
- **Power-up particles:** only Inflame, Crimson Mantle, Corruption and
  Unmovable's real Play events create the corresponding one-second effect,
  matching their explicit `NPowerUpVfx.CreateNormal` calls. Ordinary power
  amount changes keep their existing title/icon feedback.

The previously implemented hit/block sparks, power title/icon echoes, target
reticles and damage-number triggers remain in place. Sharing the hit pool
does not make orb-only bursts load the five unused hit textures.

## Platform mappings and bounds

New effects reuse the installed `I7_GlowSpeck` and card-highlight resources.
They add no large texture, new resource format, runtime decoder or saved
state. Hit/orb/power metadata share the existing 48-burst limit; card metadata
remains below 6 KiB, including six small points per visible sprite, and the
hand highlight table has only ten entries. Expired/future effects do not run
an unbounded simulation loop.

The source's dense GPU turbulence, material noise, trails and full particle
counts are mapped to small deterministic particle groups. The exhaust noise
is a CPU edge approximation; its lifetime and progression are source-derived.
These mappings are not claims of exact desktop shader pixels. Original card
art, text, frame geometry, canonical effect queue, RNG and save format stay
unchanged. Original source/artwork remains private.

## Verification

`combat_secondary_ui11_regression` covers actual repeated-evoke and passive
commands with and without observers and compares the entire canonical State,
including all RNG streams. It also exercises the scoped UI callback, bounded
power/exhaust/shuffle/generated paths, representative highlighted-card conditions,
the larger flash shader arithmetic and optional original-resource raster
visibility. Final executed results and the ordinary Firebird result are
recorded with the integration checkpoint rather than inferred from this
test's presence.

At integration checkpoint `4e4ce0692a01132de613c01e3cb6a8a56f0d001c` (I),
the core/callback/card checks passed **325,076 checks**. Running the same target
against the completed 61,186,168-byte common file and new English side file
passed **325,101 checks**, including visible highlight, exhaust and all five
orb raster paths and healthy resource decoding. The independent existing
highlight target passed **12,592,419** exact scalar/clipping comparisons.
Measured metadata is **5,540 bytes** for card motion and **10,760 bytes** for
the shared spark pool. No new large image or font buffer is present.

The corresponding local logged receipts are
`ui11i-combat-secondary-correctness-73091beaf839.json`,
`ui11i-combat-secondary-assets-d83f8cea2e82.json` and
`ui11i-card-highlight-correctness-96fa966dcd8c.json`.
The final J checkpoint `f16573e1b9c6da390cd9944bd447acb4dc0482d3` adds the
Gold-Plated Cables/WorldStage callback case, cancels selection flashes when
the source hide animation takes ownership, clears old trail flags on return
to hand and skips unnecessary hit-texture lookups for orb-only bursts.
`ui11j-combat-secondary-assets-7178b28eabeb.json` passes **325,107 checks**
against the final installed common/English resources with
`source_changed=false`. J's test
includes and recompiles its actual `app.cpp` renderer, linking the unchanged
core library built at I; this is an explicit incremental validation, not a
claim that the whole library was rebuilt at J.
