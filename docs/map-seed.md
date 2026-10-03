# Map and seed compatibility

Status for **v0.111.0 / 41cef1ea**, inspected from the installation supplied by the project owner on 2026-10-03. These are read-only CIL inspection findings, not an assertion of runtime parity. Original assemblies and disassembly output are not distributed here.

## Current implementation

`src/run.cpp::create_map` still builds the original prototype's fixed twelve-row, three-column expedition. `random_below` still uses independent xorshift32 state. This is explicitly **not** a port of the original map or seed algorithms. Relic/UI/animation work in 0.3 does not change that claim.

## Verified original path

`SeedHelper.CanonicalizeSeed` uppercases invariantly, replaces O with 0 and I with 1, then trims whitespace. For the normal seed path, `RunRngSet` calls `StringHelper.GetDeterministicHashCode`; that method hashes UTF-8 bytes using XXHash64. A separate old-seed compatibility path also exists and must not be silently folded into the normal algorithm.

Named random streams use the run seed plus the deterministic hash of the stream name. `StandardActMap.CreateFor` constructs its stream with an `act_<one-based act index>_map` name. Drawing cards, generating a map and generating rewards must not consume a shared presentation RNG.

`MegaRandom.Reinitialise` fills four 64-bit words using SplitMix64. `NextULongInner` uses xoshiro256**: rotate-left(state[1] * 5, 7) * 9, followed by its state xor/shift/rotate update. `NextDouble` uses the upper 53 random bits. Bounded integers are derived from the floating-point sampling path, not an integer remainder. Merely replacing the prototype's xorshift with xoshiro would therefore not establish seed compatibility.

`StandardActMap` builds paths across seven columns, assigns room types, and invokes path pruning/repair and layout post-processing. Audited entry points include `PathGenerate`, `GenerateNextCoord`, `HasInvalidCrossover`, `AssignPointTypes`, `MapPathPruning.PruneAndRepair`, centering, spreading and straightening. Room-pool construction and call order are part of the result, not cosmetic details.

## Acceptance boundary for the next implementation

Port the canonical seed/hash/random stream layer with original-assembly reference vectors first. Then port room-pool construction, path generation, type constraints and pruning in the original order. Compare exact node coordinates, room types, edges and random-stream state on fixed seeds. Finally bind supported room content, while explicitly flagging substitutions for unsupported encounters.

Do not call a seeded random three-column graph an original map. Do not publish PC seed compatibility based only on deterministic repeats within this port. Full reward/shop/encounter parity is a separate milestone even after map topology matches.
