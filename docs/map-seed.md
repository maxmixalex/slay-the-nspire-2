# Map and seed scope — 0.5

Reference: **v0.111.0 / 41cef1ea**, verified against the supplied assembly/PCK hashes. Original assemblies and CIL dumps are not distributed.

## Implemented

`reference_rng.cpp` implements XXH64, SplitMix64 initialization, xoshiro256** state transitions and named streams. Bounded sampling reproduces the inspected binary64 multiply-and-truncate path using integer limbs, including nearest-even rounding. ASCII seed normalization uppercases, maps O to 0 and I to 1, and trims outer whitespace. Unicode and the old-seed compatibility path are explicitly unsupported.

`reference_map.cpp` ports the **explicit-quota StandardActMap constructor**. Its input is a configured RNG state and counts for rests, shops, elites and unknown rooms. It implements seven-column paths, crossover checks, fixed room rows, constrained room assignment, duplicate-segment pruning, edge breaking, quota repair, centering, spreading and straightening.

Enumeration order is part of the behavior: candidate points sort by column then row before shuffling; edge sets preserve slot order after removals; segment groups use ordinal keys. Repair runs in the original shop/elite/rest/unknown order. The API records RNG call counts after paths, types, pruning and layout, and retains the final four-word state.

The bounded workspace stores segment references instead of duplicate key strings. Capacity failures return an explicit error; they never fall back to a different map. The tested workspace is 1,114,120 bytes, separate from the 3,216-byte graph object.

## Preview versus original run

The title menu's reference preview uses `act_1_map`, canonical `NSP1RE-<number>` demonstration seeds and fixed quotas: seven rests, three shops, five elites and twelve unknown rooms, with a sixteen-row configuration. These are explicit inputs to the ported constructor, not a claim about the quotas the original game would choose for the same text seed.

**Original Gaussian quota generation is not connected.** Its RNG consumption must precede construction exactly as in the reference. The preview therefore remains separate from the existing 12×3 expedition, whose xorshift streams and version-3 saves are unchanged. It does not substitute Nibbits for original map encounters.

## Comparison evidence

Ten explicit-input cases matched restricted original-CIL replay: seeds 0, 1, 2, 3, 4, 5, 0xffffffffffffffff and 0x20261003 with the default explicit configuration; seeds 7 and 42 with six rests, ten unknowns, eight elites, treasure replacement and a second boss. Comparisons included node coordinates, types, modifiability, ordered child edges, all four stage call counts and final RNG state.

The reference runner executes the relevant original method bodies with bounded collection/LINQ/runtime adapters. It is not the full original executable and has its own possible adapter defects. A separate 1,000-map ASan/UBSan configuration-varying run checked graph structure, not original parity for all 1,000 maps.

Next acceptance: original quota generation and its floating-point boundaries; actual reference-run snapshots; exact default seeded topology; original room/encounter pools and full-run random call order. A matching explicit-quota graph alone is not PC seed compatibility.
