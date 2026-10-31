# UI23 color, storage and native-render evidence

This records the tested UI23 animation policy and its existing-content resource
pack. It does **not** certify a future Ancient scene representation, an eventual
complete install size, or calculator FPS. Private original artwork and generated
captures remain outside the repository.

## What the converter changes

`tools/animation_color_candidates_ui23.py` applies `bounded-a5` to eligible
animation pixels. It changes RGB565 codes by at most one red/blue code and two
green codes. Its 32 interior alpha values differ from the original alpha by at
most 4/255. Alpha zero stays zero, alpha 255 stays 255, and every partially
transparent pixel remains partially transparent. RGB and alpha mapping is
independent of frame number; no runtime quantizer is introduced.

Static backgrounds, UI/card artwork, typed parameter words, fonts and localized
text stay exact. Shared tiles used by a protected static image stay exact too.
Character selection uses whole-scene policies, including its fixed layer:

| Character | Policy | RGB565 maximum code error (R,G,B) | Alpha error |
|---|---|---:|---:|
| Ironclad | `555a8` | 0,1,0 | 0 |
| Silent | `dark-safe-a6` | 1,2,1 | 2/255 |
| Defect | `555a8` | 0,1,0 | 0 |
| Regent | `555a8` | 0,1,0 | 0 |
| Necrobinder | `dark-safe-a6` | 1,2,1 | 2/255 |

Dark-safe selection preserves red/blue codes 0–3 and green codes 0–7 exactly.
Tiles shared between conflicting selection policies are retained exactly.

No image dimensions, sample positions, frame counts, animation clocks, sparse
geometry or decoded image-cache page costs change. The process does not resize,
blur, interpolate, or remove animation frames. Exact tile coalescing only reuses
groups an individual image already references. Existing TGR3 sparse storage is
selected below its established occupancy and scratch bounds. Neither step adds
a runtime codec, cache capacity, or resident buffer.

## Measured existing-content packs

The reference is the private UI22 `eng-complete46.nsp` associated with source
baseline `1fde6dec2d19c7988cafb119eca37d6a306835d3`. Candidate color source was
checkpoint `477f1da599d53c4eb649754b6ec0462551ec004a`.

| Stage | English logical bytes | SHA-256 |
|---|---:|---|
| UI22 reference | 94,924,498 | `e2fc2beb4fe1e4d9ff5e54188f0c134bc9da2ee680ef2acc06ad5f7cd6784147` |
| Approved color candidate | 65,773,916 | `7b738d710e7216b6d95b03b7347795667db0c10eb1f328391359cfb7791d3ac7` |
| Exact coalescing + sparse | 63,713,306 | `dab6f474b6f649dd57c21edb5ce7462127dd118074ca1d10f5a86e5901bd7abc` |
| Exact context retuning | 63,287,322 | `52dadbcfe1f737bf84089e18526147797ccd2848734906181d7ffd00f8a457db` |

These are logical English resource sizes, **not** bilingual installed sizes.
The final launcher, common physical parts, both language files, and additional
content must be counted separately against the 70,000,000-byte install limit.

The coalescing/sparse proof compared 577,669,400 live tile bytes and 70,883,847
index bytes, preserving all 58,613 images, 13,299 glyph entries and 8,704 strings.
Context retuning preserves decoded bytes, dictionary capacity, and never raises
the original literal-context allocation requirement.

## Actual 320×240 visual evidence

The review used integer RGB565 output from the production native Canvas, not
resized source atlases or generated illustrations. Captures are presented at
320×240 in side-by-side sheets. Earlier global coarse-color candidates were
rejected after visible forest-background banding and selection-portrait grain.
The selected policy protects those static backgrounds and uses finer whole-scene
selection policies instead.

The selected color candidate was inspected in:

- 36 full-App selection captures: five characters plus Random, at frame ticks
  0, 5, 10, 20, 40 and 80. All 36 are byte-identical between the alpha6 and
  alpha5 hybrid candidates.
- 42 full-App combat captures covering Axebot idle/Strike, Necrobinder with
  Osty and Decimillipede idle/Unleash, and Ceremonial Beast idle/Strike/Plow.
- 32 existing production death-actor/timer regression captures. The actor
  fixture deliberately clears the Canvas to black to inspect silhouettes and
  room-specific position/scale; these are not complete room screenshots.
- Ten full-App Vantom captures from actual combat callbacks, including spray,
  tail dissolution, death sprays, explosion, and shrink-then-death.
- Ten additional full-App Necrobinder death-screen captures from controlled
  combat/shop-position fixtures, including backgrounds, transition and HUD.

The alpha5 candidate retains sharp scythe, robe and terminal-body geometry in
the sampled Necrobinder deaths. No new obvious blur or edge artifact was found
in the reviewed native-size scenes. This is a review of those captures, not a
claim that every possible difference is universally imperceptible or a report
of physical calculator viewing.

Across the original 110 captures, alpha5 versus the approved alpha6 hybrid
changed at most 831 of 76,800 pixels in a frame; maximum incremental channel
difference was 9/255. Comparison against the original reference had a sampled
maximum channel difference of 17/255. Numeric error alone was not the acceptance
criterion. After exact coalescing/sparse storage, **all 110 captures were byte
identical** to the approved color candidate: 25,344,000 RGB output bytes compared.

## Native whole-App performance scope and results

The data-only ABBA run used one c10b8e runtime archive and the same harness
binaries, Clang 18 `-O2 -DNDEBUG`, with unchanged cache capacities. Archive SHA:
`78b41f9002f56d65c77138da7703c6ec185ec081f566eae27db864b9951c4d52`.
The sequence was reference, candidate, candidate, reference. It measured process
CPU for **advance plus render**, with application-cold rendering, first animation
windows and later windows reported separately. Selection used three 450-frame
windows per character and twenty 45-frame switch visits; combat used three
300-frame windows per scene. State serialization and content-health checks passed.

Resource bytes/calls below are requests to a file-backed **logical resource
provider**. They are not physical calculator storage/USB measurements. A final
compressed language/common-parts wrapper can perform additional physical work.
The OS file cache was not flushed. CPU differences below 5% are not called gains.

| Scenario | Reference mean ms | Candidate mean ms | CPU change | Logical read-byte change |
|---|---:|---:|---:|---:|
| Selection switching | 0.993 | 0.886 | -10.8% | -18.9% |
| Five-character later loops | 0.500 | 0.547 | +9.4% | zero reads in both |
| Axebot Strike opening window | 0.508 | 0.593 | +16.7% | +21.9% |
| Ceremonial Strike opening window | 0.723 | 0.771 | +6.6% | +14.6% |
| Ceremonial Plow opening window | 0.745 | 0.733 | -1.6%; no gain claimed | -32.4% |
| Decimillipede Unleash opening window | 0.755 | 0.777 | +2.8%; no gain claimed | -34.2% |

The initial results were mixed. They support lower selection-switching work and
lower resource traffic in Plow/Unleash, **not** a general battle-FPS improvement.
Several battle CPU repetitions had broad, overlapping ranges. The initial warm
selection regression was investigated, not silently discarded: a narrowly
isolated Silent ABBA paused only verified project render/compression workers for
7.17 seconds and resumed them unconditionally. Its zero-read means were
0.40349/0.41680 ms reference and 0.42253/0.43310 ms candidate, a 4.31% difference
within the declared noise floor. Worst-window p95 remained 0.503/0.510 versus
0.548/0.572 ms, so this does not establish universally identical timing.

Strike I/O traces found identical bulk retained payloads (9,466, 5,139 and
29,198 bytes). Extra traffic comprised 4 KiB descriptor-page fills and a hot
tile-descriptor-page refetch after pack offsets changed. Removing coalescing
reduced Axebot/Ceremonial opening calls from 9/8 to 7/6, at a cost of 147,778
logical bytes. This confirms layout sensitivity; it does not prove a tile-alias
cause.

The reader experiment uses a second-chance reference flag in the existing
8-byte page tag's alignment padding; the 64 pages and their payload buffers do
not grow. On the pre-retune candidate it removes Axebot's late descriptor-page
refetch (9→8 calls); the two remaining extra descriptor fills are unchanged.
Both old and new readers use 6 calls/55,887 bytes for each Strike scenario on the
retuned data, so that specific reduction comes from the changed stream layout,
not the clock. Across all seven 100-frame diagnostic combat runs, clock policy
reduces retuned-data requests from 1,103 calls/17,671,380 bytes to
1,101 calls/17,663,188 bytes. These diagnostic runs include setup and cold
rendering and are not CPU benchmarks. The focused source-reader regression
passes 24,945 checks. Final assembled-pack traces and formal calculator memory
size remain separate gates before selecting this runtime change.

## Private evidence locations

- Original: `/tmp/nspire-preserved-ui22/release-final/logical/eng-complete46.nsp`.
- Color and exact-storage candidates:
  `/tmp/slay-ui23-color-bounded-a5-static-selectionhybrid.nsp` and
  `/tmp/slay-ui23-color-selectionhybrid-a5-coalesced-sparse.nsp`.
- Retuned candidate: `/tmp/slay-ui23-color-selectionhybrid-a5-sparse-retuned.nsp`.
- Capture hashes/provenance: `/tmp/slay-ui23-quality/alpha5-quality-receipt.json`.
- Exact Canvas comparison: `/tmp/slay-ui23-quality/alpha5-exact-passes-receipt.json`.
- Visual sheets under `/tmp/slay-ui23-quality/`: `selection-selectionhybrid-comparison.png`,
  `combat-selectionhybrid-a5-comparison.png`, `alpha5-additional-edges-comparison.png`,
  `vantom-selectionhybrid-a5-comparison.png`, `full-necro-death-alpha5-comparison.png`.
- ABBA raw rows, frame fingerprints, hashes and summary:
  `/tmp/slay-ui23-data-bench/results-abba/`; build provenance in
  `/tmp/slay-ui23-data-bench/build-provenance.json`.
- Isolated warm control: `/tmp/slay-ui23-data-bench/isolated-warm/receipt.json`.
- Descriptor read traces: `/tmp/slay-ui23-data-bench/*-read-trace.txt`.

No physical-device FPS or universal visual-equivalence claim follows from these
host measurements. Final newly appended Ancient animation quality and complete
bilingual install size require their own evidence.
