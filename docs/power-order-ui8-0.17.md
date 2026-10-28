# UI8 power ordering and Test Subject — source contract and implementation

Reference: the supplied v0.111.0 / 41cef1ea C# source and original PCK scenes.
This is a functional implementation record. The stage's frozen full build and
actual-pack acceptance remain necessary; these source changes alone do not
establish a whole-game fidelity percentage.

## Source behavior

| Original source | Contract implemented |
| --- | --- |
| `MegaCrit.Sts2.Core.Nodes.Combat/NPowerContainer.cs`, `Add`, `Remove`, `UpdatePositions` | Only visible models have nodes. Nodes retain acquisition order. Removing and reapplying a model puts it at the end. Fixed cell width, enough columns for at most two rows, and centered horizontal overflow. |
| PCK `scenes/combat/power.tscn` | 48 × 40 control; 40 × 40 icon; right-aligned amount within its original cell. Amount digit count does not change spacing. The source deliberately also uses the 48-pixel width as its vertical step. |
| `Models.Powers/{AutomationPower,PanachePower,RollingBoulderPower,TheBombPower,NightmarePower,MonologuePower,OrbitPower}.cs` | Independent powers remain distinct instances, with their own amount, progress, card snapshot and tooltip. Automation/Panache/Orbit display remaining progress; Monologue's label appears only after Strength was applied. |
| `Models/PowerModel.cs` and the pinned power definitions | `IsVisible`/amount flags are respected. Resources and temporary implementation wrappers do not invent duplicate power icons. Osty and orbs keep their dedicated controls. |
| `Models.Monsters/TestSubject.cs`, `TriggerDeadState`, `Title` | Each Adaptable knockout increments the current run before the death animation. The final, non-Adaptable death does not increment it. The localized title is prior profile total + 8, including in Bestiary. |
| `Saves.Managers/ProgressSaveManager.cs`, run-end update | Add the run's Test Subject count to profile progress before Standard/Custom/Daily win/ascension filtering. |

Paths above abbreviate the common `MegaCrit.Sts2.Core` namespace prefix.
No original assets, fonts or source excerpts are embedded in the public repo.

## Persistent model order

The simulation owns a sparse sequence of packed 16-bit identities: 4 actor bits,
4 existing instance-slot bits, and 8 stable status-model bits. Its 1,118-entry
capacity includes all existing player models and every one of the seven 16-slot
instance families, plus the existing eight-enemy model/debuff capacities.
No gameplay RNG, clock or pointer enters this structure.

Registration happens at the actual application/removal sites: legacy effects,
all five character gain-power functions, enemy powers and character debuffs,
instanced allocations, ordinary relics, world relics and potion mutations.
A settled effect removes powers that expired through direct model field updates.
Enemy-slot replacement clears the previous actor's sequence. A new combat clears
all previous combat ordering. Reapplying an amount to an existing live model
retains its position; removing and later acquiring it appends a fresh position.

The strip reads this sequence directly. Combat feedback snapshots its ordered
identities alongside existing amount facts; visual changes wait for the same
scheduled effect as the corresponding power amount. The sequence diff preserves
the longest retained prefix/subsequence and emits only required remove/append
operations. Deleting an early icon therefore costs one cue instead of rebuilding
the remaining list. It uses linear scans, the existing bounded cue queue, and
no additional per-effect allocation.

Instanced icons have individual hover targets and localized descriptions.
The optional full status browser still groups by model and lists each active
instance's description, keeping its separate resource-detail functionality.
A read-only fallback supports historical hand-authored/debug fixtures; normal
runtime mutations use the persistent model hooks. The regression explicitly
checks that reconciling actual gameplay produces no missing or reordered token,
so the fallback cannot silently substitute for unfinished runtime hooks.

## Save migration and host behavior

The v17 writer keeps the complete frozen v15 base payload at its old offsets,
then appends the 16-byte run options introduced by v16, followed by:

- 2-byte current-run Test Subject knockout count;
- 2-byte actual power-sequence length;
- 2 bytes per active ordered identity;
- zero alignment bytes to the existing four-byte boundary.

The suffix writes only the used sequence, not its full reserved capacity.
Readers retain versions 3–16. Old saves have no cross-family interleaving to
recover: migration retains each stored family's original sequence and appends
other live models in an explicitly deterministic directory order. It never
invents a claim that missing historical ordering was recovered. All subsequent
applications and v17 saves preserve actual cross-family order. Malformed count,
identity, duplicate token, padding, truncation or trailing data is rejected
before replacing the caller's live State.

Web now exports its codec version and reads the actual save header when choosing
a storage key. v17 is stored under `nSpire2-save-v17`, with the existing profile
suffix. Older supported keys remain readable and are removed only after a
successful atomic replacement. The run transaction rolls back failed writes or
removals. A page hosting an older codec cannot silently overwrite or clear a
newer unreadable run. X11 and Ndless already use the common versioned codec and
atomic profile files, and require no filename migration.

## Test Subject profile persistence and titles

The knockout counter is incremented at the original death transition, before
revival, and survives a mid-death v17 save. It is not inferred from encounter
wins or revival counts. Run completion, loss or abandonment adds it once through
the existing outcome guard; persistence retries do not double-count it. All
three run modes contribute, as in the original progress manager.

`UiProgress` adds a separate 32-bit cumulative counter. `NSH5` appends four bytes
after the unchanged `NSH4` map-drawing/history tail, and remains opt-in when the
counter is nonzero. `NSH1`–`NSH4` readers initialize the unavailable historical
counter to zero. Profile deletion clears it with the other profile facts.

The localized monster name, Bestiary title, combat status header and nested
owner/applier names all share the formatter. The current unfinished run keeps
the prior profile total in its name; the next run sees the completed total.
Normal monster names retain their existing string path. The fallback title is
used only when a source resource string is unavailable.

## Memory and verification

A native `sizeof` comparison against frozen stage 4 measured:

| Structure | Before | After | Change |
| --- | ---: | ---: | ---: |
| `State` | 198,128 B | 200,368 B | +2,240 B |
| `PowerOrderState` | absent | 2,238 B | +2,238 B |
| `CombatStatus` | 46,368 B | 51,356 B | +4,988 B |
| `UiProgress` | 273,976 B | 273,984 B | +8 B, including alignment |

The calculator has the live `App.game` and the codec State workspace, so the
State increment affects both. Save buffers retain their existing capacity and
shared resource-transfer storage. The full App size also includes independently
implemented auxiliary/room modules; it must not be attributed to this change.
Final ARM allocation and actual device performance remain separate build gates.

Executed before the stage checkpoint: Clang 18 syntax for core, app, pointer,
progress codec, new power regression and affected UI/codec regressions; browser
profile-storage behavioral tests, including v15→v16→v17 replacement, older-codec
protection and failed migration rollback. The added native regression is
`power_order_ui8_regression`: cross-family order, seven instance families,
per-instance progress, source flags/geometry, effect-time order, frozen v15/v16
prefix migration, real Test Subject transformation saves, run-mode accounting,
NSH5/NSH4 compatibility and localized title behavior. Its runtime result must
come from the parent's frozen checkpoint; it is not reported as passed here.
