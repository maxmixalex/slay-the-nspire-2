# Creature and intent hover restoration — UI10

Reference remains v0.111.0 / 41cef1ea. The reviewed source methods are
`NIntent.OnHovered`, `NIntent._Process`, `NCreature.ShowHoverTips`,
`NCreature.UpdateIntent`, `Creature.HoverTips`, `AbstractIntent.GetHoverTip`,
`AttackIntent`, `MultiAttackIntent`, `StatusIntent`, and the original
`combat/intent.tscn` plus `NHoverTipSet` alignment methods. Original method
bodies, scenes and localized payloads remain private.

## Actual corrections

An intent icon now owns its own hit region and displays only its own original
title, icon and formatted description. The former178px generic rectangle,
monster-name heading and concatenated descriptions are removed. Hovering the
creature body instead produces the original ordered intent set followed by its
actual power tips and associated cards. Both paths use the existing69px native
hover renderer and creature Bounds alignment. Its existing viewport overflow
paging remains the small-screen adaptation.

A merged creature hover keeps every power's own token, so its actual instance,
actor and retained effect-time amount cannot be taken from the neighboring
power. Pointer and keyboard card aiming suppress the hover consistently with
`NCombatUi.Hand.InCardPlay`. Card and potion targeting over individual intent
regions still identifies the same enemy slot.

Damage/status labels use5px Kreon Bold and repeat suffixes4px, replacing the
legacy10/8px regular labels. Source22/18px projects to roughly4/3px;5/4px is the
explicit minimum-readable calculator adaptation. Chinese repeats use the
source multiplication sign; English keeps its source `x`. The supplied exact
font update includes all digits and both suffix symbols. Earlier resources
retain their available-font fallback.

Intent bobbing now moves upward, matching `Vector2.Up`, rather than downward.
Adjacent intents retain the source0.3-radian phase separation. Their initial
process-object hash phase is deliberately deterministic on this port, not a
claim to reproduce .NET object identities. Original animated intent textures,
particle material output and exact subpixel rasterization are separate limits.

## Verification scope

Clang18 and G++ source syntax checks pass. `world_ui_regression` now exercises
real compound-intent pointer targeting, individual source titles and icons,
the body-owned combined power set, card-aim suppression and the direction of
the source animation. Its resource-aware hover fixture samples actual opaque
original skin pixels outside the text/icon area and requires a69px column.
`world_text_regression` retains canonical-save purity and the deferred-intent
and replacement ownership checks; its original title-region comparison now
uses the real source hover geometry.

Matching frozen native/actual bilingual execution, final resource acceptance
and integrated host builds are required before calling this stage accepted.
This source review and these tests do not establish a restoration percentage.
