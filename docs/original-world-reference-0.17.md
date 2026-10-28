# Original world reference contracts — 0.17

This is an independent static audit of the owner's pinned v0.111.0 / 41cef1ea
assembly and resource directory. Model identifiers, numerical parameters and
method tokens below are an integration contract, not a runtime parity claim.
Original source, disassembly, scenes and artwork remain private.

## Reachability and capacity

There are three act positions: Overgrowth or Underdocks, then Hive, then Glory.
The four region templates contain 22, 20, 20 and 18 distinct encounter models,
respectively: 80 in their disjoint union. Each region has three elite and three
boss models. Their event pools contain 13, 10, 10 and seven entries, with 39
distinct models because Sunken Statue appears in both first-act regions.
`config/original-act-pools-0.17.json` preserves the declared pool order.

The shared event pool has 18 entries. Together with the 39 regional events and
The Architect's special final route, these account for the 58 EventModel
definitions. War Historian Repy's ordinary eligibility method returns false,
but Lantern Key can force it as the next event in the final act. The eight
ancient definitions are Neow; the three Hive and three Glory ancients; and
Darv, the sole shared ancient. Additional eligibility decisions belong in the
event implementation rather than silently changing declared pool order.

Raw namespace counts are 121 monster, 91 encounter and 68 event definitions.
They include abstract, deprecated, debug and special-route definitions, so
they are not counts of playable or implemented content. The event encounter
closure has nine definitions including the Battleworn Dummy base and its
three versions. Tunneler Normal is declared but is absent from the four act
pools; reachability must be established before placing it on a map.

All fixed encounter starting groups contain at most four enemies. The variable
generators also produce at most four. Living Fog and Ovicopter each have six
named slots; all other summon and replacement paths use fewer. The audit of
all enemy additions in monsters, powers, events and relics found no unbounded
spawn path. Eight enemy slots therefore cover the pinned single-player
encounter closure. Dead-actor continuation and replacement identity must still
be preserved independently of the physical slot number. The original
EncounterModel's spawned history records distinct canonical monster models;
its escaped-enemy gold adjustment must not substitute current live count.

## Map, room queues and random streams

| Contract | Original method token | Integration consequence |
| --- | --- | --- |
| Room count | `060017a0` | Single-player region counts are 15, 15, 14 and 13; the boolean reduction is specifically multiplayer |
| Floor count | `060017a1` | Total floors are room count plus two |
| Standard map constructor / factory | `06004a3c`, `06004a3d` | Seven columns; grid rows are room count plus one; use the named `act_<index+1>_map` stream |
| Act room generation | `060017bb` | Generate persistent encounter/event queues before traversal |
| Repeat-tag avoidance | `060017bf` | Consecutive encounter choice is constrained by tag overlap |
| Pull next event / encounter | `060017c1`, `060017c2` | Save queue cursors and recheck event eligibility when pulling |
| Run-wide room generation | `06000c44` | Shared ancient assignment precedes each act's room generation on the UpFront stream |
| Room roll / construction | `06000c4c`, `06000c4b` | Unknown points use saved odds; ancient points create ancient event rooms |
| Enter next act continuation | `06009784` | The last act leads to The Architect rather than immediate victory |
| Terminal reward continuation | `0600979e` | An event combat may resume its parent event instead of opening the map |

The normal encounter queues begin with three weak encounters in either first
region and two in each later region. A weak grab bag is refilled as required;
the regular pool then fills the remaining queue. The elite queue contains 15
entries. Boss and ancient selections are stored separately. The original
serializable room set stores these generated values, not only the graph.

Regional events concatenate with the shared pool before an unstable shuffle.
Eligibility is checked again when the next event is requested, including
visited-event constraints. Lantern Key overrides the final act's next unknown
point to an event and then substitutes War Historian Repy, so this forced
entry must be applied after the ordinary eligibility step. Shared Darv is allocated to at most one later act,
or neither, before those acts choose their ancient. At ascension ten the last
act receives a second boss different from its first.

The initial run also populates shared and character relic grab bags using the
UpFront random stream before generating room queues. Reproducing a map's
named stream alone does not reproduce a complete original run. The existing
reference geometry engine is suitable for graph construction, but the former
12-by-3 expedition does not supply these queue, odds, history or continuation
contracts. Content unlock handling must be explicit when Timeline is excluded.

## Resource and scene audit

The private PCK directory has 15,890 entries, including 3,555 compressed
textures, 20 font payloads, 170 Spine atlases, 164 Spine skeleton payloads,
1,037 Godot scenes, 126 creature visual scenes and 24 custom encounter scenes.
These are resource counts; visual-only variants and supporting scenes are
included. The exported `.cs` entries are one-byte stubs and do not provide
runtime source. The Chinese localization directory contains 47 tables.

The original map scene uses a vertically scrollable three-part background,
each part with a 1080-unit minimum height, and a separate legend. Ordinary map
points have a 56-by-56 control with an 84-by-84 selection reticle. Boss and
ancient map points have their own scenes. These distinguish node geometry
from the scroll viewport and the size of its input target.

The default event scene places a large centered portrait behind a right-side
800-unit text-and-option column. Its title, description and option list are
separate vertical elements. Ancient events instead have a full-scene ancient
background, a bottom-anchored content area and a separate dialogue hitbox.
Combat separates the scene container, effects and UI layers. Original
encounter camera scale and offset can differ: Ovicopter, for example, uses
0.8 scale and a left/down offset to fit its egg slots.

These authored dimensions are reference facts, not a directive to reduce all
text to one-sixth size. At 320 by 240, preserve the original artwork,
hierarchy, relative grouping and node semantics while explicitly adapting
font size, text pagination, input targets and camera framing. The visual
acceptance ledger must distinguish scene-derived comparisons from actual
captures of the original application. No fidelity percentage is assigned by
this static audit.
