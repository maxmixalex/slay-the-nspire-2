# UI14 reconciliation of earlier retained differences

Reference: the owner's pinned v0.111.0 /41cef1ea. This ledger separates
previously missing features, this turn's changes, bounded calculator
adaptations and verification limits. A historical document's pending sentence
does not override its later implemented source. The current user report
reopens the specifically reported UI13 defects and performance acceptance.

## Earlier omissions with implemented source

| Earlier difference | Current implementation and evidence boundary |
| --- | --- |
| Doom sinking, destruction and its particle family | UI13 added frozen Hurt, grayscale/sink, front/back holes, ground, droplets, spikes, flames and overlay; model exceptions are explicit. `combat_doom_presentation.hpp`, `combat_doom_ui.inc` and `ui13-combat-feedback-0.17.md` identify the complete triggers and original-resource gates. UI14 retains this implementation. |
| Architect reading, head overlays, pen and character-specific attack effects | UI13 added reading/stop-reading, pen trails, thirteen attack families and FireBurst with original layers, curves, timing and displacement data. `architect-vfx-ui13-0.17.md` records actual-resource comparisons. This is not newly completed by merely fixing UI14's room text. |
| Random water and three source particle emitters | UI13 implements the water/noise sampling, original textures and all1+20+20 emitters. UI14 investigates and optimizes the real device cost; named implementation and current FPS acceptance remain separate. `random-character-ui13-0.17.md` and `frontend-ui14-0.17.md` own these results. |
| Fake Merchant's room, actors and post-fight/no-rug state | UI13 added the exact static/moving scene layers, complete original cycles, five class actors, weighted separate additive planes and canonical event-state visibility. `fake-merchant-ui13-0.17.md` records the full resource/interaction gates. UI14's regular merchant split is a different room path. |
| Main-menu blur and per-label rainbow clocks | UI13 replaced the unimplemented blur and shared label origin with the source-derived CPU filter and label-local clocks. See `main-menu-blur-ui13-0.17.md` and `ui13-lcd-followup-0.17.md`. |
| Exhaust erosion, power/discard trails, shuffle backs, generated hand arrival and orb activation | UI11 connected actual command observers and bounded particle/trail effects; repeated orb evokes preserve exact targets and canonical RNG. `combat-secondary-ui11-0.17.md` supersedes the older `combat-card-motion-0.17.md` omission list. UI14 retains these effects and fixes shared-face instance sampling; its separate bounded current-intent preparation removes the measured generated-card resource bottleneck. |
| Duplicate-creature scale/hue, sleep effects and camera trauma lifecycle | `creature_instance_scale`, the eight source hue matrices, sleeping-creature presentation and source camera hooks exist. UI13 fixed trauma retention on target changes. The older UI6/UI7 missing-feature statements are superseded; continuous GPU color/noise equality is still not claimed. |
| Named Bestiary phase/multi-creature contexts | UI11's detached preview facts cover regeneration, charge, burrow, stun, hatch, coral growth, liquefaction, summons and Decimillipede revive states. UI14 fixes row pitch and selected-arrow geometry. This does not claim an exhaustive replay of every possible preview combination. |
| Full startup logo, skip preference, glyph advances and kerning | UI12 implemented the actual startup sequence and preference, natural1/64-pixel advances, pair kerning and negative bearings. Older UI10 "missing startup" and integer-advance overflow statements describe their historical runtime only. |

These are implementations carried into this turn; they are not added to the
UI14 new-feature total simply because their documentation was reread.

## Additional behavior restored in UI14

| Area | Concrete implementation |
| --- | --- |
| Source body scaling | ShrinkPower and Beetle Juice, Big Mushroom, Phantasmal Gardener growth and Test Subject respawn growth now drive their source transforms. Decimillipede rejects scaling. Vantom selects the correct separately baked normal/half-body endpoints, but its750ms root-bone transition remains missing; whole-sprite scaling would incorrectly move its independent tail. |
| Ancient/status/affliction card presentation | Ancient portrait/frame/banner/fire, Infection and Bound/Entangled/Galvanized/Hexed/Tainted/Ringing/Smog materials have explicit source resources and precedence. |
| Sovereign Blade and permanent enchantment | Real Forge/attack and successful enchantment commits drive their own original effects, queued exact card facts and cancellation rules. They are not ordinary discard flights. |
| Generated-card performance | Stage13 prepares the already displayed enemy intent's action clips within the existing640-page limit, after actor starts, without advancing RNG or creating a loading UI. Real five-Dazed generation measures29.5419FPS and its following flight/draw28.9576FPS in matched Firebird windows. The complete first6s measures25.3955FPS and still includes one initial interval above250ms; these scopes remain separate. |
| Continue transition | Resume uses the same character departure lifecycle as Embark without creating another run or resetting progress. |
| Reward entry and completion | The historical early-Loot shortcut is replaced by Normal death-track+1s, Fast ordinary500ms, and Fast boss half-track behavior. Actual command barriers are preserved, terminal dead loops cannot stall, and one deadline is scheduled. Empty terminal rewards separately fade250ms and still require Proceed to open the map. |
| Uniform readable overflow | Shared fitted labels, selected-hand text, source hover descriptions, enchantment text and FTUE pages scroll in pixels inside their actual clipped viewport. Content-defined summary truncation remains a separate original rule. |
| Closed regular merchant | All1,817 original frames are preserved through exact invariant/moving factoring and bounded retained groups. Final source pixels and alpha recompose exactly. First16s and matched warm60s are independently measured in Firebird; the warm result does not hide the cold entry peak. |

The relevant detailed ledgers are `combat-ui14-0.17.md`,
`frontend-ui14-0.17.md` and `rooms-ui14-0.17.md`. Their executed checkpoints
remain authoritative. The final death-to-reward change is later than stage7;
stage12 now passes its1,149 synthetic and58,696 actual-English UI12 checks,
including all four timing branches, held loops, command barriers and a real
winning-card State comparison. The rooms ledger records those receipts.

## Retained scope and platform boundaries

| Boundary | Meaning |
| --- | --- |
| Complete Ancient actor animation | Intentionally omitted under the user's already accepted storage exception. This is not an unowned TODO or a new permission request. Ancient gameplay, dialogue, options and the UI14 Ancient card presentation remain in scope. |
| Audio, multiplayer and Timeline | Excluded by the agreed offline single-player scope. No fidelity credit is assigned for those absent systems. |
| GPU/material rendering | RGB565/LDR, integer LCD rasterization, bounded noise and particle pools remain explicit CPU mappings. The UI14 procedural affliction materials use eight phases, whereas Infection keeps30 frames and Ancient fire keeps10. Eight phases are not continuous Godot GPU equivalence. |
| Sampled skeletons and authored clocks | Existing offline sampling is a temporal representation, not a text-readability exception. UI14's regular merchant work preserves every existing frame, duration, pivot and size; no additional frame removal is hidden in its compression result. |
| Calendar | The calculator supplies a raw RTC timestamp without a host timezone database. `ui_calendar` retains UTC interpretation; this differs from a desktop local-calendar conversion and is disclosed as a platform boundary. |
| Bounded history and drawing | History retains96 floors and32 runs; simultaneous presentation transitions and map notes have finite capacities. Missing/evicted facts remain partial. These limits are not unlimited-desktop-history parity. No obsolete-save compatibility work is a prerequisite for UI14. |
| Cosmetic randomness | Presentation-only hashes/noise preserve canonical RNG but do not reconstruct the original process-global chaotic stream's prior history. |
| Storage | The final UI14 common file is72,131,134 bytes,1,471,828 larger than the preceding common file after adding missing artwork. Grouping saves6,215,294 bytes against the intermediate UI14 candidate. Neither the complete package nor the earlier65–70MB target may be reported as achieved by quoting only the internal savings. |

## Actual remaining fidelity and evidence limits

Vantom still lacks its750ms skeletal shrink transition. Both shrink sources
select the correct normal and half-body endpoints, but the integrated bake
contexts do not carry the converter's `shrink_from`/`shrink_started` values.
Its independent tail control makes whole-sprite scaling incorrect. Restoring
the original skeleton/atlas and page textures is required to produce or verify
the missing intermediate bone poses. Standard creature shrink, the other
named growth effects and Sewer Clam's sampled coral growth are implemented;
this precise exception must not be described as completed.

Stage13 closes the specifically measured generated-card and subsequent-flight
performance verification, with zero resource failures, ordinary OS exit and
per-prefix saved-game identity against stage12. It does not remove the
initial0–2s cold stall or establish30FPS for every encounter. Its12s aggregate
includes idle and is not an animation-FPS substitute. These performance
measurements also do not supply any missing original await parameters.

The named old visual families above have implementations. The source still
does not claim every original per-monster command duration. The explicit
`enemy_command_timing` table covers Leaf Slimes and Damp Cultist, with
Nibbit/Louse-specific waits in `ui_enemy_move_wait`. Separately,
`combat_command_timing.hpp` already contains 196 original AttackCommand
profiles, and the effect observer uses their animation delay, callback wait,
repeat/once flag and Damage tail for each actual hit. Those implemented
attack waits must not be counted as absent features. A move's earlier or
later dialogue, cast, block, power and other awaited commands can still use
the registered clip-duration fallback; animation length is not evidence for
the original command's return time.

The 107 solo model entries contain 327 move nodes. Twelve nodes have the
complete named move wait and three explicitly return `Task.CompletedTask`.
Of the remainder, 189 have an existing AttackCommand profile and 123 are
outside that table. The latter group also includes raw-damage commands, so
it cannot simply be called 123 non-attack moves. These counts describe the
available timing evidence, not 312 missing rules. Examples still requiring
their original method bodies include Aeonglass `IncreasingIntensityMove`,
Axebot `BootUpMove`, Calcified Cultist `IncantationMove`, Corpse Slug
`GoopMove`, Crossbow Ruby Raider `ReloadMove`, and Crusher's raw-damage
move family. Existing explicit selection, generated-card, respawn and
Decimillipede sequencing remain separately implemented.

The retained UI13 evidence archive and release metadata contain no C#/IL
method bodies or additional await parameters. The existing model JSON
preserves method names/tokens and animator data, not those command bodies.
Completing this specific audit requires the pinned v0.111.0 `sts2.dll`
(`0861bfa1df347538d932f22d580e75420f08082792eb914e53b4882764acdbe9`)
or its matching private decompilation; the earlier private source directory
was removed during this session's environment cleanup. No guessed wait
table is substituted. The reward-entry rule has its own restored
four-branch timing and must not be confused with this broader boundary.

Every desktop GPU secondary material, every Bestiary move combination, and
an exhaustive comparison against an unmodified Godot application have not
been established by the port's regressions. Current native and Firebird
receipts prove their named correctness and device scenarios, not a measured
95% restoration score or physical-calculator FPS. A natural first battle,
controlled boss/animation fixtures and an entire natural three-act victory
must retain their distinct identities.
