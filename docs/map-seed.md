# Map and seed scope — 0.9 (0.6 subsystem evidence retained)

Reference: **v0.111.0 / 41cef1ea**, pinned in `config/reference-release.json`. The assembly/PCK hashes are checked before reference inspection or conversion. Assemblies, disassembly and private reference runners are not distributed.

## Implemented pipeline

The 0.5 explicit-quota constructor is now preceded by `act_quotas`, consuming the same named random stream before path generation. `generate_act_map` composes seed hashing, `act_<one-based index>_map`, quota generation and the existing seven-column generator. It keeps quota call counts separately and preserves the generator's stage counts and final random state.

The inspected **integer** Gaussian branch uses two fresh random draws per attempt:

```
u1 = 1 - NextDouble()
u2 = 1 - NextDouble()
z = sqrt(-2 * log(u1)) * sin(2 * pi * u2)
value = RoundToEven(mean + deviation * z)
```

Values outside the inclusive bounds are rejected and both draws are repeated. This is not the cosine-based floating Gaussian overload, a clamped normal variable or a cached pair of normal samples.

| Region model | Stream | Map rows, solo | Rest quota | Unknown quota |
|---|---|---:|---|---|
| Overgrowth | `act_1_map` | 16 | Gaussian(7, 1, 6, 7) | Gaussian(12, 1, 10, 14) |
| Underdocks | `act_1_map` | 16 | Gaussian(7, 1, 6, 7) | Gaussian(12, 1, 10, 14) |
| Hive | `act_2_map` | 15 | Gaussian(6, 1, 6, 7) | Gaussian(12, 1, 10, 14) minus 1 |
| Glory | `act_3_map` | 14 | NextInt(5, 7), upper exclusive | Gaussian(12, 1, 10, 14) minus 1 |

Shops have quota 3; elites have quota 5 or 8 according to the increased-elite setting. The current API carries treasure replacement and second-boss flags. The former multiplayer row-reduction option was removed in 0.9 because multiplayer is explicitly excluded; solo dimensions are unchanged. These flags do not implement the corresponding encounters. The preview exposes region and elite-quota controls only.

Quota means requested room counts, not a guarantee that the constrained/repaired final graph contains exactly that many rooms of each kind. Fixed rows and generation constraints also contribute to the result.

## Portable arithmetic and boundaries

`reference_math.cpp` implements software binary64 arithmetic using fixed-width integer limbs, with no hardware FPU, target floating-point library, fused multiply/add or 128-bit integer requirement. Elementary functions are adapted from the Go math implementations; original notices and the BSD license are retained.

The arithmetic and bounded integer sampler round nearest-even. Logarithm and sine are approximations: **they are not claimed to reproduce every last bit of Windows' original math library**. Inputs to the quota sampler are bounded. If a value is within 2^-36 of a half-integer, generation returns `AmbiguousRounding` instead of silently selecting a side or consuming replacement random draws. The guard is conservative engineering protection, not a proof of universal cross-library equivalence. Other failures are explicit as well.

Seed entry accepts 1–64 ASCII characters after normalization. It uppercases, replaces O/I with 0/1, and trims outer whitespace. Unicode normalization and the raw lowercase `old` legacy seed path remain unsupported; the editor rejects that legacy prefix. These input limits are not advertised as the complete original seed-input specification.

## Historical verification executed in 0.6 (not rerun as new 0.9 evidence)

The private reference runner decodes and executes the relevant original `GetMapPointTypes`, standard-unknown-count and `NextGaussianInt` CIL method bodies, with adapters for the original random stream, constructors and host math calls. It evaluated 2,238,619 original instructions over **20,016 cases**: 16,016 region-quota cases and 4,000 sampler argument cases. Quotas, random call counts and final four-word RNG states matched the C++ implementation. Of those cases, 4,663 required rejection/redraw; the maximum sample case used 22 draws.

The math adapters use the Linux host's libm, not the original Windows executable. This is a real original-method replay comparison, but is not an independent full-game oracle. A separate **1,000-map** ASan/UBSan run varied all four regions and configuration flags, checked graph validity, and compared the composed pipeline to the same generator fed separately generated quotas. That is integration/structural coverage, **not 1,000 new original-topology comparisons**.

Version 0.5's ten explicit-quota original-CIL topology comparisons remain the earlier constructor evidence; they were not expanded into a full original-run suite here. In Firebird, six stable 0.6 preview frames matched native RGB565 output exactly, covering the default seed, a touchpad-entered seed, all four regions and the elevated elite quota.

## Still separate from the playable game

The preview now generates its quotas rather than accepting fixed demonstration counts. It still does not resolve the original run's region/modifier decisions, fill original encounter/event pools, or drive the playable expedition. Other region backgrounds and actual Boss artwork have not been ported into this diagnostic view. The playable 12×3 expedition still uses its own map. The current runtime writes version-6 project saves and imports versions 3–5; map preview state is not a playable campaign save.

Next acceptance is comparison against actual unmodified reference-game run/map captures across supported single-player region and difficulty configurations, then original room/encounter binding and full-run random call order. The current result is a seeded regional map subsystem, **not yet certified full PC-seed or full-game parity**.

Audio, multiplayer and Timeline progression are outside the current product scope. Reference-run setup must supply supported single-player configuration directly rather than recreating Timeline progression.
