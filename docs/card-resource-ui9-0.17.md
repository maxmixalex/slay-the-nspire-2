# Original card resource overlays

Reference: v0.111.0 / 41cef1ea, recovered `NCard`, `CardModel.CanPlay`,
`UnplayableReasonExtensions.HasResourceCostReason`, and `scenes/cards/card.tscn`.

The hand resource crosses mean that a card rule prevents play while both
resource requirements are affordable. The source explicitly suppresses both
crosses when **either** energy or stars are insufficient, including a card
that is also blocked by another rule. Busy action queues, pending animation and
input locks are separate from `CardModel.CanPlay` reasons.

`card_has_nonresource_play_block` shares the existing keyword, relic, character,
enemy and event rule predicates with `can_play_card`. It does not change State,
play legality, costs, RNG, or save formats. The face renderer keeps a negative
energy parent hidden, so Wound does not acquire an invented energy badge. Hand
and small-card cache keys include the overlay independently of overall
playability, numeric costs and resource colors.

The original atlas `card/card_unplayable_icon.tres` is shared by both children.
Its 104px square includes the original atlas margin. The optional auxiliary
2050 stores a 16px projection for the largest ordinary inspect presentation;
the renderer applies each authored 48px-square child, at EnergyIcon+(8,8) and
StarIcon+(5,5), through the same 300×422 card geometry. Old packs omit the image
and continue rendering. The unpublished grouped formats accept either 2050
or 2051 auxiliaries without changing prior indices.

`patch_card_resource_assets.py` appends only this image to an existing v32/v33
pack. Its shared insertion helper locates the complete locale tail from actual
artwork boundaries. This is necessary because grouped glyph data can precede
its first directory-referenced descriptor. Moving the full tail together keeps
every signed relative group reference and every compressed glyph/string byte
unchanged. The private conversion still requires a saved source checkpoint.

Validation completed before the final integration checkpoint:

- New card/HUD cases compiled against immutable base runtime
  `da8c78b632653463b65787b5ae2d3b16e5292f66` and passed 385,090 synthetic
  checks. This is a bounded diagnostic of the saved runtime, not final-pack
  acceptance of later source.
- The cases cover Wound, Grand Finale, Smog, both resource shortages, temporary
  queue/input unavailability, exact canonical State preservation, and removal
  of a stale cached cross when overall playability remains false.
- Tangled/Entangled is tested as the sequel's actual attack-cost increase,
  following `TangledPower.TryModifyEnergyCostInCombat`; it is not incorrectly
  treated as an independent attack prohibition.
- Synthetic 32KiB and 64KiB original glyph groups survive image insertion with
  identical decoded alpha, payloads, encodings, text, old images, clips, model
  indices and group-relative descriptors.

Actual original-image append, final composed card captures and matching
integration execution are the next acceptance gate. No private art or captures
are stored in the public repository.

## Upgrade preview resource colors

Recovered `NCard.UpdateEnergyCostVisuals` and `UpdateStarCostVisuals` test
`WasJustUpgraded` before affordability. `CardEnergyCost.FinalizeUpgrade` clears
that transient flag for a permanently upgraded card. Explicit preview context
now follows the library and deck upgrade toggles, inspector checkbox/navigation
and camp upgrade comparison into the common face renderer. A changed numeric
cost receives the original green value and green outline; an unchanged or X
cost does not. Permanent upgraded cards retain normal hand cost rules.

The inspector retains each original model's upgrade state across navigation
and its close fade. The small-face cache already includes the resulting
resource style; hand tiles use ordinary finalized cards and retain their
existing style key. New regression cases alternate identical upgraded faces
between normal/preview cache entries, compare cached pixels with direct
composition, and check the unchanged/X cost, finalized-hand, shortage-priority
and inspector-origin branches. Short C++ syntax validation passed; these new
cases await the same complete integration checkpoint as the original cross.
