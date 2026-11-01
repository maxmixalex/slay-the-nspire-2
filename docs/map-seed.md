# Map and seed scope — current behavior and retained subsystem history

Reference: **v0.111.0 / 41cef1ea**, pinned in `config/reference-release.json`. The assembly/PCK hashes are checked before reference inspection or conversion. Assemblies, disassembly and private reference runners are not distributed.

## Current UI24 input and verification

See [ASCII seed compatibility and portable import](seed-ascii-ui24.md) for the current user-facing rules and the executed 927-input / 7,416-original-map comparison. The port's 64-byte text field is a representation choice, not an OS restriction; complete longer ASCII seeds can be encoded by the same desktop converter and imported through an explicit normal-run UI mode. The 2,048-point annotation buffer is unrelated to generated maps.

The original standard-map subsystem now drives the playable world. The historical 12×3 expedition is a separate legacy/practice path. Save files include the 64-bit seed, named RNG states and generated world maps. Claims below labeled 0.5/0.6/0.9 remain historical evidence for those stages.

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

UI17 supersedes the former ASCII-only input boundary: seed entry uses the original invariant Unicode uppercase, O/I replacement and whitespace trimming rules, within the existing 64-byte UTF-8 field. Raw/debug legacy `old` seed hashing is also implemented separately from ordinary canonicalized input. This operation does not perform NFC/NFKC normalization. See [the UI17 seed report](seed-unicode-ui17-0.17.md) for original-method comparisons, integration tests and the retained byte-capacity/globalization boundaries. The map/quota verification below remains historical evidence for its stated version.

## Historical verification executed in 0.6 (not rerun as new 0.9 evidence)

The private reference runner decodes and executes the relevant original `GetMapPointTypes`, standard-unknown-count and `NextGaussianInt` CIL method bodies, with adapters for the original random stream, constructors and host math calls. It evaluated 2,238,619 original instructions over **20,016 cases**: 16,016 region-quota cases and 4,000 sampler argument cases. Quotas, random call counts and final four-word RNG states matched the C++ implementation. Of those cases, 4,663 required rejection/redraw; the maximum sample case used 22 draws.

The math adapters use the Linux host's libm, not the original Windows executable. This is a real original-method replay comparison, but is not an independent full-game oracle. A separate **1,000-map** ASan/UBSan run varied all four regions and configuration flags, checked graph validity, and compared the composed pipeline to the same generator fed separately generated quotas. That is integration/structural coverage, **not 1,000 new original-topology comparisons**.

Version 0.5's ten explicit-quota original-CIL topology comparisons remain the earlier constructor evidence; they were not expanded into a full original-run suite here. In Firebird, six stable 0.6 preview frames matched native RGB565 output exactly, covering the default seed, a touchpad-entered seed, all four regions and the elevated elite quota.

## Remaining parity boundaries

The current playable world uses the regional map generator and original-content encounter/event pools. This supersedes the historical statement that the preview was disconnected from play. The preview remains a diagnostic UI with only region/elite controls, rather than a replacement for complete original run setup.

A matching seed and explicit map configuration do not establish an identical whole campaign. The original player's progression/unlocks, mode/modifiers, act choices, character, and action sequence also matter. UI24's separate Timeline implementation and whole-run encounter/reward ordering require their own verification. The direct original-method comparison ran on Linux .NET; it is not a proof of every Windows math-library rounding result. See the current report for exact scope and counts.
