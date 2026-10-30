# UI18 exact cached-coverage rendering

## Implemented operation

The cached-card canvas stores straight RGB565 color and a separate alpha
plane. Combining two partly covered pixels previously divided three channel
sums by the same variable coverage. The production path now reads one
512-byte, read-only Q16 reciprocal table and corrects each rounded-up quotient
by at most one. It preserves every integer result, including its original
rounding. The existing cases for an opaque canvas, opaque destination,
transparent pixel, modulation and additive drawing remain unchanged.

For coverage 2 through 255 the greatest numerator is `63*255+127=16192`.
The product fits in 32 bits; `ceil(65536/d)` can overestimate the quotient
by only one over this domain. Comparing `q*d` with the numerator removes
that one. Outside the bounded domain the established division remains.
There is no new writable arena, image pool, heap allocation or startup scan.

The final renderer code checkpoint is
`3fba5bb7a9753d1141a5ac4cbc7596f3a452ce5b`. It is a component checkpoint,
not the eventual integrated runtime or final ARM delivery identity.

## Executed correctness and measurements

- `ui18-coverage-final-exact-da9322c49bff`: **273,597,054** independent
  exact checks pass at the final component checkpoint. This includes all
  packed red/blue lane pairs and alpha values, all green channel pairs and
  alpha values, and all 4,113,022 bounded coverage quotient inputs.
- `ui18-coverage-exact-309fc13e2ba7`: native and ASan/UBSan passes at the
  preceding `e675f36` checkpoint. Its reciprocal implementation is identical;
  its separate opaque multiplication experiment was subsequently withdrawn.
- `ui18-coverage-canvas-276b0dcdeacf`: the actual Canvas implementation
  passes **330,242 cases / 2,951,469 checks**. This scoped executable includes
  that checkpoint's `app.cpp` and links unchanged UI17 core code. Final
  integrated validation must compile the complete current tree separately.
- `ui18-coverage-final-benchmark-48ef58076739`: native Clang 18 `-O3`,
  seven alternating runs of 240 synthetic frames, compares the existing
  division with the reciprocal operation. Median cached-coverage time is
  **158.602872 ms before / 69.635448 ms after**, with identical output hash
  `b0a028a3`. These are isolated native operation times, **not game FPS or
  calculator timings**. The opaque control is 42.408066 / 41.978142 ms,
  a noise-level difference between the same weighted-sum operation.

All listed completed journals report unchanged source. The complete final
integration, ordinary ARM build and ordinary-OS Firebird comparisons are
owned by the UI18 integration stage and are not implied by these results.

## Withdrawn multiplication experiment

`0d81aa7` factored opaque blending into a destination term plus color
difference. Although exact and emitted with two rather than four variable
multiplications on ARMv5, the native operation regressed from 56.002666 to
61.512989 ms (`ui18-blend-exact-1bc9f09a44eb`). Fewer multiplications did not
establish lower runtime cost. `3fba5bb` restores the established opaque
weighted sum; the candidate and its negative result remain in history.

The intermediate ordinary ARM build at `e675f36` passed, producing
1,767,185 bytes and a 22,269,992-byte Zehn allocation
(`ui18-coverage-arm-c8672bf76410`). Those figures include the withdrawn
factor and therefore are **not the final UI18 launcher or allocation**.
