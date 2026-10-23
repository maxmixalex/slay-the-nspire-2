# Power and status playback

Enemy-turn gameplay still resolves in the canonical core. The display already
retains health, block, energy and intent until their animation cues. Power
icons previously bypassed that projection: their visibility, amount and hover
description could reveal the completed turn while the first enemy was moving.

`CombatStatus` now retains only the fields used by the power strip, its hover
description and the status details. It observes the same settled-effect clock
as health. It includes signed amounts, Slow and Withering Presence counters,
the original applier's model, captured cards, per-instance powers, character
counters and orb values. Keyboard and pointer selection use that displayed
list. Enemy names and health in the status panel use the retained actor.
Applier lookup also honors the core's zero/player, slot-plus-one encoding;
the former tooltip lookup treated that saved value as a zero-based slot.

This is fixed-size App presentation storage, capped at 64 KiB, with 256 change
cues. It does not copy State, encode a save, invoke gameplay or draw randomness.
If an exceptional chain fills the intermediate queue, the final captured
values still become visible at the existing side boundary. Reset/load uses
the canonical state immediately; presentation is never saved.

This change does not establish original command delays for unaudited enemy
models. It follows the existing effect clock; the audited Nibbit/Louse delay
boundaries and Normal/Fast behavior remain the reference for regression.

Added regression cases exercise actual Nibbit strength and Louse Frail moves,
Osty's health icon, delayed auxiliary/instance values, input selection, bounded
queue convergence and unchanged canonical save bytes. Source is checkpointed
before full native/portable execution. Syntax checks alone are not acceptance.
