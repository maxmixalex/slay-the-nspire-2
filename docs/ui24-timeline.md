# UI24 Timeline and unlock state

The target remains v0.111.0 / 41cef1ea. Numeric Epoch metadata and localization
keys are in `config/timeline-0.111.0.json` and `config/timeline-text-ui24.json`.
The latter appends 257 original strings at 8704..8960 for every original locale.
Original text and images are private converter inputs, never public sources.

## State and migration

The 57 Epochs use the original None / NoSlot / NotObtained / ObtainedNoSlot /
Obtained / Revealed states. Obtain stores the first actual UTC timestamp;
reveal expands slots and makes associated content available to the next run.
Only Revealed contributes to a fresh profile's frozen two-word run mask.
The score bar grants at most one agnostic Epoch at each Standard run end;
remaining score is clamped below the next threshold. Finished-run accounting
retains its existing outcome guard, including save/reload and retry.

NSHB appends bounded Timeline data to profile saves. Existing NSH1..A profiles
receive compatibility access to previously available content, with no invented
Epoch dates, reveals or accumulated score. Their observed character starts and
statistics remain actual facts. Canonical v18 appends the frozen run mask and
first-run flag; v17 and older retain the historical all-open run contract.

## Normal UI and generation

The title's Timeline entry opens a chronological LCD list. Inspect shows the
original title, story, obtain time, condition and revealed description/unlocks.
A separate Reveal action applies the original expansion graph. This 320x240
list/scroll presentation adapts the PC's large spatial Timeline and reveal
staging; it does not reproduce its portrait choreography. Pending reveals have
a visible title-menu marker. Legacy access is labelled explicitly.

Fresh profiles gate characters, Random, Custom/Daily and Epoch model pools.
Random appears only with all five characters available. Daily uses the original
all-unlocked policy. Custom portraits and randomization also obey character
locks. Standard first-run and unseen-boss ordering is applied after random room
generation, before the second A10 boss is selected. Without revealed Neow, the
first act opens on the existing map and its root children are reachable.

The ordinary profile records an act when that act is entered, including entry
before any combat or boss. This persisted fact controls original GetRandomList:
unlocked unseen Underdocks is forced without a random draw. Otherwise its
bucket is sampled; Hive and Glory each consume their original one-item draw.
Random character selection then uses the same independent act_selection stream.
Low-level explicit-region map APIs remain available for isolated seed tests.

## Evidence and current limits

The first production integration passed the 297-check foundation and 230 new
normal-flow/migration assertions. Its focused run exposed six old fixture
assumptions about menu counts, profile upgrades, v17 tails and discovery-sensitive
startup; these are corrected in the next source checkpoint and remain pending
rerun. No failed gate is counted as a pass.

`tools/timeline_reference_oracle` uses the owner's pinned assembly to execute
original GetRandomList, canonicalization/hash and Random selection. Its 40
numeric fixture rows cover eight ASCII seeds, fresh/all-open/Underdocks-only
masks, seen/unseen acts, both first-act results and all five Random outcomes.
Only the original telemetry module initializer is disabled for isolated loading;
original method bodies are retained and receipt-hashed. Four Acts and five
Characters are created through their original constructors and registered in
ModelDb; a real ProgressState is supplied by a SaveManager mock. Raw assembly,
IL, Godot resources and saves stay outside Git.

Example (private output directory, .NET 9):

```
dotnet run --project tools/timeline_reference_oracle -- /private/reference /private/timeline-output
```

Strict ARM acceptance and ordinary firmware UI/resource acceptance are separate
from native correctness. The initial integration exceeded the allocation cap;
root's cold-code optimization restored the original cap without larger pools or
smaller actor frames. Final exact runtime/resource identities and guest captures
will be recorded after the full and reduced installation checks.
