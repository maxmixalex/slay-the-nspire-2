# UI23 runtime acceptance

The accepted d935 runtime measures **11.2930 FPS** in Neow and **23.0763 FPS**
in Nonupeipe, with zero resource reads during both warm paired intervals. It
passes the unchanged **22,291,016-byte linked allocation ceiling** at
**22,290,500 bytes**. These are ordinary 64 MiB Firebird measurements, not
physical-calculator results. Neow and the seven-family card stress remain
below 30 FPS.

## Source and artifact identity

The actual clean build is `d9350b94c234f443c29f12f7efb862f3595a6a57`. Its
[source-equivalent published checkpoint](https://github.com/maxmixalex/slay-the-nspire-2/commit/1298a1bbbbb9355e8c87ad02542ff14a6154730e)
has tree `402461fe462d4ab4c31322f9096ad28c76ef2e94`. All 641 runtime/build
inputs equal the clean candidate and linked ARM helper manifest; helper
verification passes. The artifact retains its actual build provenance rather
than being relabeled as a second build from the published commit.

| Item | Accepted value |
| --- | --- |
| Launcher bytes | 1,736,356 |
| Launcher SHA-256 | `9a00d1f6e3e795839914c0f65929b60eafe856c27c726157b055423e9e60dfae` |
| Compression / instrumentation | zopfli / no diagnostic instrumentation |
| Linked allocation / ceiling | 22,290,500 / 22,291,016 bytes |
| Allocation headroom | 516 bytes |
| Largest individual reported stack frame | 5,936 bytes |
| Runtime/build input inventory SHA-256 | `f00e08bf403f5228340999466954cfd266f8cae40b32b57b73df5801c1131f3a` |

The stack figure is not a maximum combined call-stack measurement. No heap,
persistent cache or arena capacity was enlarged. Ordinary release builds enforce
the strict allocation ceiling in `tools/zehn.py`. Event rules, progress
serialization, keyboard/pointer input and selected cold archive/profile/navigation
functions use size optimization; renderers and decoder kernels retain `-O2`.

The [machine-readable receipt](ui23-runtime-validation.json) records complete
source identities, artifact hashes, measured values and private evidence receipt
names/hashes. It contains no original asset payload or calculator image.

## Actual Ancient scene measurements

Each interval uses ordinary OS/Ndless startup, launcher installation and hash
readback, real scene navigation, warmup, and matched A/B counters with B advanced
by 30 seconds. Counters are read after normal UI exit. These scene controls use
the same `full7229-public789-candidate` resources. They are separate from the
newly produced final package's installation and smoke checks.

| Candidate | Neow FPS | Neow render ms | Neow frames >100 ms | Nonupeipe FPS | Nonupeipe render ms | Nonupeipe frames >100 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ea84be4 | 8.98302 | 110.833 | 270/270 | 11.11940 | 89.472 | 1/334 |
| a066e5c | 10.34134 | 96.229 | 14/311 | 20.02171 | 49.587 | 0/600 |
| f7601cc | 10.47910 | 94.959 | 5/315 | 19.61946 | 50.555 | 0/589 |
| d9350b9 | **11.29302** | **88.090** | **1/339** | **23.07631** | **42.976** | **0/692** |

All eight intervals have zero resource read calls and bytes. The final Nonupeipe
interval also has no frame over 50 ms. Relative to f760, d935 improves Neow FPS
by 7.77% and Nonupeipe FPS by 17.62%. The table retains the small f760 Nonupeipe
regression instead of selecting the higher earlier result. The historical
2825 Neow measurement was 5.595 FPS; it establishes the earlier problem but is
not substituted for a contemporaneous paired control.

The 025 diagnostic stage measured the main costs per Neow frame as fire
36.858 ms, lights 24.013 ms, fog 13.944 ms and post effects 18.058 ms, with about
22.315 ms outside the room renderer. That evidence guided the work. Zero I/O
alone was not treated as proof that CPU decoding or sampling was free.

## Changes responsible for the improvement

Exact changes remove redundant wide division and texture directory/page lookup,
skip proven zero-support regions and zero RGB565 contributions, and precompute
constant color/alpha contributions. A bounded two-pixel intermediate grid
approximates only smooth illumination/fog contributions. Source geometry,
full-resolution foreground masks, animation clocks and final destination pixels
remain intact. The exact Neow post coverage cache fits within the existing scene
union; static assertions preserve its size and alignment.

The last d935 change specializes only the two white Ancient option/outline
assets. Every draw verifies that visible source pixels are white. It retains
the original nearest projection and exact Normal/Add arithmetic, using small
local lookup tables for constant tint and separately rounded opacity. Nonwhite
or alternate formats and Canvas coverage/matrix/occlusion effects fall back to
the ordinary path. The proof is not cached; measured guest timing includes it.

Reduced/no-animation Ancient scenes already reuse a complete composed clock-zero
raster with content-generation and scene identity invalidation. They do not
recompute unchanged fire, lights, fog and post effects each frame. Cards, dialogue
and other UI continue using their own presentation clocks.

Continuous card effects share exact four-corner field addressing, borrow an
existing foreground scratch area for a depth owner map, and skip pixels hidden
by later opaque card faces. Exact reciprocal smoothstep division replaces costly
wide division without changing its quotient. Source effect clocks remain active.
The existing larger image-cache profile stays selected until leaving combat,
preventing repeated 3→2→3 family threshold churn.

## Image comparison and correctness scope

The accepted smooth-field Neow comparison covers **1,561 cases / 3,122 complete
320×240 frames / 239,769,600 pixels**: all 1,000 integer post phases, all 546
source body frames, and 15 distributed/critical clocks. Against the 2825 native
baseline, **24.2061% of pixels differ**, with maximum RGB565 channel differences
**R3/G5/B4** and mean channel-code error **0.101095**. These are finite coverage
statistics, not a bound for every combination of independent clocks or an
unmodified desktop GPU comparison. Native-size worst App/room pairs were
inspected; those witnesses showed no visible loss of actor contour detail.

The separate exhaustive **fire-only** proof evaluates **200,000 phases /
6,001,000,000 queries**, conservatively including all 546 source transmission
masks. Its bound is **R0/G4/B2**, with a witness at clock 2224. This must not be
described as the bound for the entire final composited scene. The earlier
26-frame R0/G2/B1 sample is not a global guarantee.

Later exact post and zero-border changes match the a066 soft-field output in
all 3,122 compared frames. The final d935 change likewise matches dfcf/f760
hot drawing output in all 3,122 frames, with **zero changed pixels**. The earlier
Nonupeipe soft-field prototype comparison covers 915 clocks / 1,830 App-plus-fog
frames and reports maximum R3/G5/B4; its native-size worst witnesses were also
inspected. It retains that prototype's identity and finite scope.

The final Canvas regression passes **401,774 cases / 3,094,644 checks**, covering
Normal/Add semantics, alternate-mode fallback, clipping, padded pitch, alpha
layering, coverage/matrix/occlusion, nonwhite/old-format fallback and unaligned
source access. Exact kernel gates pass **131,007 Neow fire**, **5,751,954 Neow
post**, and **9,286,886 Affliction** checks. The linked production ARM helper
executes the four required real decoder self-test paths under Unicorn. That
decoder binding does not replace Firebird execution or physical-device testing.

## Continuous-card stress and remaining release gates

The f760 runtime, whose combat renderer is unchanged by d935, measures
**18.3430 FPS / 54.0973 ms** with three continuous Affliction families and
**9.43565 FPS / 105.6172 ms** with seven. Both have zero warm resource reads.
These are the explicit **small3 TEST ONLY** fixtures on ordinary 64 MiB Firebird,
with 551 and 283 paired frames. Earlier same-fixture measurements were about
10.806 FPS for three and 4.988 FPS for seven. The seven-family case remains a
heavy CPU workload. A later owner-map row-span experiment showed only 0.18%
host gain and was not adopted.

The official bad0168 resource producer completed all 83 stages for both the
default bilingual and all-16-locale outputs. The 11 required data files occupy
68,155,584 bytes; 14 additional locale files occupy 17,773,035 bytes. These are
resource subtotals, excluding launcher, instructions, licenses and saves.
Continuous-card storage attribution is recorded in [its report](ui23-continuous-card-materials.md).

The final delivery report separately records full/reduced package installation,
current original-data guest smoke, complete extracted sizes and delivery hashes.
This runtime report does not assert unfinished package checks, universal 30 FPS,
physical-calculator performance or full desktop rendering parity.
