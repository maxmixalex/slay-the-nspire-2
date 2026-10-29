# UI16: one raster allocation for Random and the merchant

## Implemented change

Character selection's exact Random gradient and light tables now share storage
with the settled merchant inventory's sampled rug. These renderers have mutually
exclusive lifetimes. `src/scene_raster_cache.inc` contains the three existing
cache types, a union, and an explicit `None` / `Random` / `Merchant` owner.
`App`'s public layout and the content image pool are unchanged.

Claiming a different owner starts that member's lifetime with the existing
freestanding placement-new helper. It initializes only metadata. Large arrays
intentionally have no default member initializer: gradient bind writes every
used table entry before publishing `valid`; light bind initializes all bounds;
merchant bind initializes its complete color/alpha raster. Retaining the old
array `{}` initializers would needlessly clear up to 230 KiB on every switch.

All existing `Content*`, resource generation, source picture, and dimensions
checks remain. Invalid or changed source pictures still use the same fallback
sampler. An ineligible merchant canvas returns before claiming the slot. Random
white particles do not claim a light table, so they cannot invalidate a live rug.
No cache pointer escapes the rendering call that uses it, and foreground input
collection does not re-enter `App` rendering.

This reduces permanent memory, not source image resolution or scene fidelity.
Returning to a different scene rebuilds that scene's tables once; subsequent
frames retain the original cache reuse. Existing coarse foreground polls remain
inside the cold table construction.

## Memory evidence

The ARM926 target's existing ELF had these three separate BSS objects:

| Object | ARM bytes |
| --- | ---: |
| Random gradient | 148,012 |
| Random light bounds | 16,424 |
| Merchant rug | 230,412 |
| Separate total | 394,848 |

An independent freestanding ARM926 translation unit includes the **actual** new
cache declarations and exports their sizes plus both claim functions. Its output
contains a 230,412-byte union and a one-byte owner. Raw cache storage decreases by
164,435 bytes; allowing for four-byte BSS alignment gives approximately 164,432
bytes. The final linked application allocation must be measured independently,
because text changes and section alignment also contribute to that allocation.

The ARM claim-function disassembly contains only metadata stores. It has no large
`memset`, heap allocation, aggregate stack temporary, or `.init_array` entry.
The merchant claim writes just its pointer, generation, and validity metadata.
The renderer comparison executables' native BSS decreases by 164,464 bytes;
their text increases by 1,048 bytes. Those native figures include the test's
conditional ownership assertions and are not calculator allocation figures.

## Independent before/after renderer oracle

The pre-change checkpoint is `6d1df4f65418329144ff1aaa888ac870a937ed64`, whose
only change adds an alternating-scene regression while retaining the three
original cache objects. The shared-cache checkpoint is
`3bb9a4591918442a80cd268b3532696aaf7a124c`.

Both renderer executables were compiled with Clang 18, C++17, and `-O2`, including
their respective `src/app.cpp` directly and linking the same already-built
native core archive. This holds the unrelated simulation implementation constant
while comparing the complete old and new renderers.

`scene_raster_cache_regression` first checks independent synthetic gradient,
noise, additive light, and alpha-particle textures. It compares cached pixels to
the general samplers after repeated owner changes. It also checks that an
ineligible faded canvas and a white particle cannot steal another scene's cache.

With actual source resources, the test reloads **English → Simplified Chinese →
English**. In each edition it renders at 1,400, 7,500, and 16,000 ms:

1. The complete Random background and particles.
2. The merchant inventory with its complete animated underlay.
3. The merchant inventory with its exact rug occlusion cache, compared to step 2.
4. Random again, compared to step 1 after the merchant owned the shared storage.
5. The merchant again, compared to step 2 after Random owned the storage.

Canonical save bytes are checked after every scene. The 18 recorded RGB565
frames from the independent old and new executables are **byte-identical**:
2,764,800 bytes compared, not only frame hashes. The shared-cache run passed
223 checks including capture-file writes. Captures remain private under
`../private-ui16-scene-cache/{before,after}`; source artwork is not committed.

## Other executed gates

| Gate | Actual result |
| --- | --- |
| `random_character_ui13_regression`, actual English | 4,899,904 checks passed |
| Same regression, actual Simplified Chinese | 4,899,904 checks passed |
| `foreground_input_regression` | 66,021 checks passed |
| ASan + UBSan alternating scenes, English → Chinese → English | 169 checks and all 18 frame hashes passed |
| ARM926 declarations and claim functions | Compile and metadata-only disassembly passed |

The existing Random suite covers the general sampler, changed resource
generations, complete particle emitters, and unchanged canonical state. The
foreground suite still observes 327 coarse polls in its real combat frame and
47 in three full-screen operations, with unchanged pixels and queued inputs
dispatched only after rendering. Its independently constructed cold shader
tables remain identical with and without polling.

The sanitizer executable instruments the directly included renderer and this
test. The unrelated static core archive is not sanitizer-instrumented; this is
not a claim of whole-game sanitizer coverage. LeakSanitizer is disabled because
the executor runs under ptrace; AddressSanitizer and UndefinedBehaviorSanitizer
remain enabled. No calculator FPS or physical-device acceptance is inferred from
these native results. The parent integration performs the final ARM build and
device startup/restart checks.

## Reproduction and durable checkpoints

Build the original test from the first checkpoint with
`-DSPIRE_TEST_ORIGINAL_RASTERS`; build the new test without that definition.
The final CMake target already selects the shared implementation.

```sh
build/scene_raster_cache_shared eng.nsp zhs.nsp capture-directory
build/random_character_shared eng.nsp
build/random_character_shared zhs.nsp
build/foreground_input_shared
ASAN_OPTIONS=detect_leaks=0 build/scene_raster_cache_sanitized eng.nsp zhs.nsp
```

The captures directory must exist. Every command was wrapped by
`tools/run_logged.py`, with logs and source fingerprints under `build/journal`.
Key successful receipts are:

- `scene-cache-original-frames-3ffdc857e5de`
- `scene-cache-shared-frames-8f1ff4069bee`
- `random-shared-eng-dfc338fa3b26`
- `random-shared-zhs-311aa05503a5`
- `foreground-shared-fcfdcfb1f14b`
- `scene-cache-sanitized-frames-aaa0fc35d0ce`
- `scene-cache-arm-layout-aa1621960485`

The local source checkpoint bundle is `../ui16-raster-cache-source.bundle`.
Remote publication remains the parent integration's responsibility.
