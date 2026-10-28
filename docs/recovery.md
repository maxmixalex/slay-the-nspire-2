# Verified recovery checkpoint

The retained 0.4 archive exactly matches upstream `d3fb8ac581dd` (tree
`ec2818d6a49f00ca5b8bdeefa3a24d43c1ff64c5`). The later `map-polish` archives,
patch and validation report previously named in the conversation were not
present in the retained files or remote branch. They are not evidence of a
recoverable implementation or completed map parity. Work resumes from 0.4.

This checkpoint adds independently rebuilt seed primitives in
`include/reference_rng.hpp` and `src/reference_rng.cpp`. The provided
`v0.111.0 / 41cef1ea` assembly was checked against the existing reference hashes.
Its SplitMix64 initialization, xoshiro256** transition, UTF-8 XXH64 hash,
named-stream addition and binary64 bounded sampling path were inspected.
Bounded sampling reproduces binary64 nearest-even multiplication using integer
limbs, so ARMv5 does not need a floating-point runtime for this operation.
ASCII normalization follows uppercase, O-to-0, I-to-1 and outer whitespace trim;
Unicode normalization and legacy `old` seed dispatch are not implemented here.
Existing expedition RNG and version-3 saves are deliberately unchanged.

Checks executed for this checkpoint:

- 8,192 sequential outputs compared with direct replay of the original
  initialization and next-output CIL methods across eight initial seeds.
- 1,352,673 comparisons against libxxhash and host binary64 multiplication,
  including generated integer-boundary neighborhoods, under ASan/UBSan.
- Pointer regression scenarios: ten-card wheel paging, cancelled release after
  scrolling, stable menu capture while held, and unchanged serialized game
  state after browsing.
- ARM, WebAssembly and CMake/X11 builds succeeded. Firebird itself compiled
  from the uploaded source and matching gif-h submodule. This checkpoint has
  not yet rerun the updated launcher in Firebird.

The pointer fixes prevent half-page movements being rounded away by the grid,
and prevent a held pointer from moving a menu's selection underneath its own
captured click. Reference RNG code is a foundation, not a claim that original
map topology, encounters or full-run seed compatibility are now implemented.

Algorithm references: https://prng.di.unimi.it/xoshiro256starstar.c and
https://xxhash.com/doc/v0.8.2/group___x_x_h64__family.html . The original game's
CIL and private test inputs are not part of the public source distribution.
