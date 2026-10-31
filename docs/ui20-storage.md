# UI20 storage continuation — verified source and launcher checkpoint

Reference: StS II v0.111.0 / `41cef1ea`. This record replaces the missing
storage handoff linked by the UI20 animation notes. It does **not** claim a
complete new resource installation or a new ordinary-device acceptance.

## Measured production launcher

Both columns below are ordinary ARM release launchers rebuilt in the same
local toolchain. The baseline has the recovered UI20 runtime before packed
indices; it is not the historical UI19 binary. The final runtime checkpoint
is `f1d5fcdb702806fb3a0c447b46d3120466e89e8c`. Later changes concern tests,
documentation and recovery workflows, not the executable's production sources.

| Metric | Rebuilt baseline | Final grouped indices | Difference |
| --- | ---: | ---: | ---: |
| Installed launcher bytes | 1,724,397 | 1,675,061 | 49,336 fewer |
| Absolute relocation entries | 39,394 | 32,539 | 6,855 fewer |
| Zehn prefix bytes | 157,744 | 130,324 | 27,420 fewer |
| Execution image plus BSS | 22,131,860 | 22,143,772 | 11,912 more |
| Total Zehn allocation | 22,289,604 | 22,274,096 | 15,508 fewer |

Final launcher SHA-256:
`7946c9225cdf94b668f2709f91044c4f9f0e41492535cdac0f140be0b347c1cb`.
The separate execution/BSS and total-allocation guards still pass. The total
allocation decreases because the relocation prefix shrinks more than the
loaded image grows. This is not a claim that every memory component shrinks,
nor an assurance about arbitrary fragmented operating-system heaps.

The release compressor retains the old Zopfli 5-iteration/15-block profile
and tests one additional fixed 20-iteration/64-block profile. It keeps the
smallest exact zlib stream, including the existing zlib fallback. Every selected
candidate must independently decode exactly, reach EOF, and have no trailing
or unconsumed bytes. On the unchanged baseline ELF this alone saves 2,592 bytes;
that gain is already included in the final 49,336-byte total, not added twice.
No calculator decompressor or executable format changes.

## Implemented production index representation

`include/packed_content_index.hpp` and `src/world_content.inc` replace the
6,855 absolute pointers in the text/image/clip source indices with constexpr
integer offsets. First-dot domains are stored once per contiguous group.
Image/clip key offsets fit in 16 bits; the larger text pool uses 32 bits.
The unchanged original sorted `.inc` lists remain the authoritative inputs.

Lookup searches the domain and then its local range. It preserves every key,
ID, native override, null/missing behavior, 255-byte composite limit and the
uncommon spelling in which the domain argument itself contains dots. There
is no dynamic initialization, heap allocation, runtime catalog construction,
whole-file integrity scan, or extra resource read. The linked production
binary removes all 6,855 pointer relocations, not just an unused test table.
The complete old/new production lookup comparison passes 57,137 assertions.
These are behavior/storage measurements, not calculator frame-rate timings.

An intermediate pointer-free representation reduced stored bytes but increased
total allocation by 43,900 bytes. It was superseded by the grouped layout and
is not the delivered launcher.

## Implemented retained-image garbage collector

`tools/compact_retained_slices.py` is an offline converter, not an analysis-only
probe. It compacts unreachable retained image slices and exact duplicate
slices **inside their existing independent groups**. It supports the established
RGR1, RGR2 and RGR3 forms without a new runtime codec or workspace.

The converter preserves every live decoded RGB565/alpha/hidden-color byte,
image geometry, group membership, dictionary, frame clock, model binding,
font and string. Groups shared with glyph/string domains and overlapping live
slices are left intact. Equal pixels in separate groups are not regrouped.
Each accepted replacement pays for bounded zero padding so all subsequent
4 KiB address phases remain unchanged; gains below one page are declined.
Raw decoded working sets cannot increase for changed groups.

Every requested locale is reconstructed and verified before any output `.tns`
is published. A fresh output directory is required, inputs are never replaced,
and the converter adopts the candidate only when the sum of the actual output
installation files is smaller. Otherwise it writes the exact original files.
Its receipt records per-file bytes and SHA-256, with candidate and installed
savings distinguished. Corrupt caches, incomplete slice maps, invalid padding,
missing locales and a controlled late-locale verification failure are tested.

Nine public test cases cover all six supported filter modes, gap reclamation,
within-group aliases, cross-domain protection, separate-group identity,
bilingual NSL3/concern reconstruction and an idempotent second pass. These
synthetic fixtures prove the converter path. They do **not** establish the
number of reclaimable bytes in the unavailable private game installation.

## Private resource rebuild chain

First execute the original-art repair in [ui20-animation.md](ui20-animation.md):
all eight Ancient full-viewport images, 325 repaired enemy frames, bounded
overflow pieces, Osty dead-loop samples, registration tables and reward icons.
The resulting logical English pack must contain 58,099 images, 1,783 unchanged
clip descriptors and 5,422 auxiliary entries. An old UI19 pack merely converted
to wire format 45 is not a complete UI20 artwork release.

With the repaired logical pack and the matching old locale files, run each
stage through `tools/run_logged.py` from a published/read-back checkpoint:

1. `refresh_artwork_resources.py` grafts the repaired artwork into the locales.
2. `compact_tile_slots.py` reclaims unreachable tile slots.
3. `compact_sparse_tiles.py` chooses lossless TGR3 sparse groups.
4. `compact_retained_slices.py` reclaims unreachable retained slices while
   retaining the current read groups and page phases.
5. `retune_resource_streams.py` selects smaller exact bounded streams.
6. Run the complete native image oracle, bilingual UI tests and ordinary
   Firebird scenarios against this final matched candidate before packaging.

The compaction/retuning stages accept `--input`, `--output`, `--work` and
bounded workers. The artwork refresh takes `--artwork`, `--input`, `--output`
and `--work`, without a worker-count argument. Use `--locales eng zhs`
for a bilingual run or supply all sixteen verified originals. Never merge
files from different pipeline stages. `package_release.py --stage ui20`
checks the required content extension; it does not replace the pixel oracle
or ordinary-device acceptance. The final installed total must include the
launcher, common index, every concern file and selected locale files.

## Executed acceptance for this continuation

| Check | Result / boundary |
| --- | --- |
| All registered native suites | 158/158 passed on `9342f4f8086df7525fddec3c50a9c9e47aa25088`; public synthetic resources |
| Packed production lookup | 57,137 checks, all 6,855 source entries; included in the native suites |
| Converter/packaging tests | 399 passed on final production source; no skipped tests |
| Additional animation/logging/content-builder tests | 54 passed; those Python sources are unchanged |
| Ordinary ARM release build | Passed, final binary and allocation measured above |
| Linked ARM memory routines | 4,799 vectors passed under Unicorn; not Firebird FPS |
| WebAssembly compile-only | Passed; no manual browser/private-content validation |
| Final private resource package | Not rebuilt in this continuation |
| Ordinary Firebird / physical calculator | Not rerun in this continuation |

Final journals: `ui20-final-grouped-arm-b50e59146aec`,
`ui20-final-arm-memory-157fde4c527b`, `ui20-final-conversion-988155f88da2`,
`ui20-final-web-build-08855f3f1bd8`, `ui20-extra-converters-955c7e646b33`, and
`ui20-accepted-all-native-51a731c189cf`. Their recorded source inventories did
not change during execution. The full native driver replaces the rebuilt
Content object in the otherwise unchanged native core; unchanged test objects
are reused only when source bytes match. Changed tests are rebuilt. All 158
registered cases are actually rerun, not inferred from previous results.

The initial native sweep failed two tests. One contract check ran before a
saveable new run existed; it now runs after setup. The other compared trailing
`Card` padding in independently generated cards. The same failure was reproduced
against the old core with identical kinds, upgrades and RNG calls. Independent
outcomes now compare complete encoded saves or all Card fields; render-only
immutability remains bitwise. The changed comparison has a negative control
that rejects an altered real gold value. Earlier failed journals are retained.

## Delivery and remaining limits

This continuation provides source plus a smaller launcher, **not** a complete
new `data/` folder. No private original/derived assets are included. The last
historically recorded bilingual installation is 74,538,517 bytes from UI19;
its 65–70 MB goal was not yet met. It is not a newly measured UI20 total, and
neither synthetic compression ratios nor launcher savings should be relabeled
as a newly verified whole-game installation.

The old UI20 source fixes remain present, but animation/background/reward art
repairs require the corresponding private resource pack. Full Ancient scene
animation, LCD/material sampling and the other platform limits in
[ui20-retained-differences.md](ui20-retained-differences.md) remain disclosed.
No new calculator FPS, full original-game parity, all-feedback closure, or
successful ChatGPT Library persistence is claimed by this checkpoint.
