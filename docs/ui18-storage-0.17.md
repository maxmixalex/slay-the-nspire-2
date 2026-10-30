# UI18 exact tile storage work

Baseline: complete source `433445cf8218e7ac5d085cb4e4248d1c5393cb83`
and the immutable UI17 `44b239e` sixteen-language resources. Private outputs
are under `../private-ui18-storage`; the original packs are never overwritten.
No original texture, font, pack or calculator image is committed here.

## Selected complete output

The frozen output is `../private-ui18-storage/aligned/data`: one NSM1 index,
eight required NSS1 artwork parts and all sixteen aligned NSL3 languages.
Logical resources use NSP2/42. The final source tool checkpoint is
`3acb5b310d2d2cb42dbf98f21a79f0b45d8b42ed`; the runtime format implementation
is already present at `305c1b70cc52e8a39ac12abcef5c31be53ca53ec`.

The selected result is a **roughly 230 KB resource saving**, not a multi-MB
compression claim. Actual installed bytes include all split headers, the
89-span/1,520-byte index, locale directories, NSL3 tables and padding, and the
fourteen additional calendar glyphs. ZIP size is not used for these figures.

| Installed component | UI17 bytes | Final UI18 bytes | Reduction |
| --- | ---: | ---: | ---: |
| Shared index and eight artwork parts | 72,957,060 | 72,727,684 | 229,376 |
| English | 779,148 | 778,844 | 304 |
| Simplified Chinese | 4,454,226 | 4,455,923 | -1,697 |
| Default bilingual data | 78,190,434 | **77,962,451** | **227,983** |
| Fourteen optional languages | 18,542,228 | 18,544,457 | -2,229 |
| All sixteen languages and shared data | 96,732,662 | **96,506,908** | **225,754** |

The final integration launcher is 1,719,372 bytes, giving 79,681,823 bytes for
the default installation and 98,226,280 bytes for all sixteen languages.
Launcher optimization and final integrated runtime gates belong to the main
UI18 delivery record; the resource measurements here do not attribute its
size reduction to tile compression. The historical 65–70 MB installation
target has not been reached.

Final local byte identities:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `aligned/work/eng-aligned42.nsp` | 75,070,454 | `6f8fac39d828c49f063d4d4582980d8a766a95fcc11f1ee7d5a6ce22729ef5ab` |
| `aligned/work/zhs-aligned42.nsp` | 79,495,168 | `f20e374b4cd7a697a60d3c2dd7c72869ddec6ddc765adaf8c5996bb51c0181b5` |
| `nSpire2.eng.dat.tns` | 778,844 | `391d4600bdf8c65a91e19b17b65304b2406f2ed36511602373ae5d0463bb9286` |
| `nSpire2.zhs.dat.tns` | 4,455,923 | `c0a9c756718e25d9e8c4b745ced060437a29d96ffd0a29cac015c23e86b4f5c0` |
| `nSpire2.fra.dat.tns` | 830,004 | `af6874d9bf4429b00cfae7f7fca5ca493f45a2e72b1bfb2c9571d9914ce81cf2` |

The complete per-part hashes, per-language output hashes and physical sizes
are in `aligned/data/suffix-phase-receipt.json`. Final ENG/ZHS logical files
and all final data are retained. Completed raw spools and superseded sample
logical copies were removed to make room for integrated validation; their
source data and receipts remain available for exact reconstruction.

## First implemented candidate: exact aliases

`coalesce_tile_resources.py` compares complete 770-byte tile records, including
dimensions, all RGB565 words, alpha and padding. Hashes locate candidates;
record bytes prove equality. A destination group must already occur in every
active frame using that tile. Every sparse span is included in that constraint,
even when a span's pixels happen to have zero alpha and hidden nonzero RGB.

At `7252fc418615ee95b05584700b95aa9a4fe53423`, the default ENG/ZHS conversion
completed in 273.44 seconds (`ui18-coalesce-defaults-dfb1906fc1f5`, passed,
unchanged). The 31,210 independent live frames demand 80,328 groups before and
79,279 after; 918 frames demand fewer, none adds a group. This is a per-frame
group-set property, not a claim that arbitrary cache traces have fewer misses.

The full logical rewrite saves 71,524 bytes, including all changed TGI2 index
costs. Its recomputed concern index has one extra span. Actual default resource
storage saves **68,286 bytes**, with 72,885,552 artwork bytes, 779,412 English
bytes and 4,457,184 Chinese bytes. This is a real default-resource candidate,
not a multi-megabyte saving. It is a historical first candidate; the selected
sixteen-language pipeline and its executed gates are recorded below.

The raw mapped-slot count includes already-cleared unreachable records; it is
not new pixel removal or a storage saving. No pixel codec, buffer, runtime
lookup or group raw size changes in this first candidate.

The valid initial Python receipt is `ui18-coalesce-python-3346e493bd89`, eight
tests passed. The earlier `ui18-coalesce-python-e9dd79897c22` used a mistyped
checkpoint argument and is retained as an invalid identity receipt, not used
as acceptance evidence.

The selected full coalescing pass preserves each old stream's original LZMA2
`lc/lp/pb` context. Its receipt is
`coalesced-matched/data/coalesced-tiles-receipt.json`: 437 changed pixel groups,
465 changed index groups, and **122,866 logical bytes saved** after paying for
all index changes. `ui18-coalesce-all16-matched-726711dee1f0` passed unchanged
in 657.35 seconds. Every locale was rebuilt, not extrapolated from English.

## NSP2/42 group-local slot compaction

The next stage removes unreferenced local slots and rewrites references inside
the same existing group. Global TIL1 tile-count bound, group divisor, group IDs,
table entries, image identities, original sparse geometry and clocks stay fixed.

For version 42 only, a tile group's existing TGR1/TGR2 header accepts
`1 <= items <= min(per_group, tile_count - group_id * per_group)` and requires
`raw_bytes == items * 770`. Version 41 and older retain exact equality with the
configured group capacity. Every referenced local slot must be below actual
`items`. A wholly unreferenced group keeps one unused 1x1 all-zero record;
there is no zero-byte decoder allocation. TGI2 and its record format are
unchanged. Runtime cache allocation uses the actual smaller raw length.

The pixel decoder and filter are unchanged. Only the version-bound/header
contract changes; no resident table, picture-pool change, fifth artwork stream
or extra cross-group read is introduced. Original literal-context properties
`lc/lp/pb` are extracted and preserved; there is no encoder parameter sweep.
Complete candidate cost includes its rewritten indices and split metadata.

`compact_tile_slots.py` preserves the original proven concern ownership using
paired semantic object endpoints. Repeated aligned replacements can accumulate
padding; deriving one combined length delta would lose that padding. A
synthetic two-rewrite regression covers this case. Final splitting compares
every reconstructed byte with the complete logical pack.

At `45603668f8da78f18fbf67833b15ec9afb44d8dd`, the initial 26 Python tests
passed (`ui18-compact-python-21c481b0a322`). The first 24-group sample's encoded
pixel plus full-index cost saved 1,888 bytes, but its concern reconstruction
failed with missing ranges. That journal (`ui18-compact-sample-0b47279567c6`)
is a failed attempt, not a valid resource output. The corrected endpoint
mapping and source-context control require fresh source-identified gates.

The corrected 24-group, original-context sample at `e8ba88d` passed all byte
and split checks (`ui18-compact-sample-matched-6f213ee5ebc5`, 69.66 seconds).
Eleven groups were selected; tile streams saved 1,890 bytes and all affected
indices saved 366, a net 2,256 bytes. Decoded tile data decreased by 164,010
bytes. The 89 original concern spans reconstruct exactly. These are sample
figures, not extrapolated full-catalog savings. The 28 corrected Python tests
pass at `ui18-compact-python-corrected-a435ce1db181`; the corrected seven-target
native build is `ui18-storage-native-build-6fbfa8533739`. The earlier build
`ui18-storage-native-build-d19afb147cdc` named a nonexistent target and failed
before compiling; it is not an executed gate.

The complete receipt also records `ceil(raw_size/131072)` per tile group and
the before/after one-through-four-page distribution. Catalog-wide sums of
those page costs are not actual simultaneous cache residency or FPS results.

The complete compaction considered all 1,017 eligible groups and accepted 728.
Pixel streams saved 131,180 bytes; rewriting 483 index groups cost 22,046 bytes,
for **109,134 net logical bytes**. Together with coalescing, the pre-alignment
logical saving was 232,000 bytes. This is recorded by
`compact/data/compact-slots-receipt.json` and the passed, unchanged
`ui18-compact-all16-37b579c24090` journal (603.49 seconds).

Decoded tile data decreased from 604,400,720 to 516,960,290 bytes: 113,559
unreferenced records, or **87,440,430 raw bytes**, were removed. That number
is not installed storage saved. Of it, 72,673,370 bytes belonged to 139 wholly
unused groups, now represented by one unused dummy each. Nonempty groups
account for the remaining 14,767,060 raw bytes.

| Group's 128 KiB page cost | Before: number of groups | After: number of groups |
| --- | ---: | ---: |
| 1 page | 0 | 139 |
| 2 pages | 1 | 2 |
| 3 pages | 0 | 3 |
| 4 pages | 1,154 | 1,011 |
| Sum across the catalog | 4,618 pages | 4,196 pages |

Of the 422-page catalog reduction, 417 pages belong to the 139 unused groups.
Only four nonempty groups change allocation: group 762 goes from 680 to 376
items and 4 to 3 pages; group 821, 680 to 415 and 4 to 3; group 1064, 680 to
263 and 4 to 2; group 1146, 680 to 420 and 4 to 3. These total five pages.
They are not a claim that all four groups are present in one combat, or that
585 other compacted nonempty groups release a page. Their smaller raw streams
can reduce decoding work, but target timing requires an actual scene trace.

The global pixel pool remains 13,369,344 bytes. The storage format adds no
resident table, decoded buffer, transfer buffer or fifth artwork handle.
The isolated format-only ARM build at `305c1b7` passed with 22,269,780-byte
Zehn allocation and unchanged 157,220-byte relocation metadata. Its +88-byte
allocation delta against UI17 is code, not a new picture cache. The final
integrated launcher uses separate safe-ICF and compression changes and has
its own allocation evidence.

## Fixing an actual read-page regression

The first complete compacted package preserved image bytes but moved the
unchanged suffix earlier by 232,000 bytes, which is not a multiple of 4,096.
In the real bilingual Continue/End Turn native replay, English consequently
changed from 17 reads / 102,516 bytes / 17 seeks to 19 / 107,891 / 18. One new
4 KiB request crossed the interface/cards concern boundary and became two
physical reads; the locale page boundaries also changed. This was a genuine
layout regression, not counted as a finished performance optimization.

`align_resource_suffix.py` now locates and compares the **complete unchanged
suffix**, computes `(reference_start - new_start) % 4096`, and inserts only
that many zero bytes after the last tile/index group. For the actual resource
set it adds 2,624 bytes, making the retained suffix's relocation exactly
229,376 bytes, or 56 pages. Reference and final suffix starts both have phase
3,822. All 52,544,550 encoded group bytes remain unchanged; English's complete
20,679,944-byte suffix also remains unchanged. No runtime cache, input schedule,
picture fidelity, decoding dependency or warmup operation is added.

The operation is idempotent and rejects a changed reference suffix. Its
encoded-identity verification cannot accept a changed group stream. Tests
cover zero padding, the maximum even 4,094-byte padding, repeated execution,
exact hidden RGB/alpha, NSP42 compaction followed by alignment, split concern
round trips and mismatched-reference rejection.

After locale recompression, alignment costs the default installation 2,485
bytes compared with the unaligned compact candidate. The sixteen-language
total is actually 2,768 bytes smaller, because changed locale directory
compression more than offsets the shared padding. The final physical sizes
above are measured, not computed by adding 2,624 to an earlier total.

| Fixed 182-frame native action replay | UI17 | Final aligned UI18 |
| --- | --- | --- |
| ENG physical reads / bytes / seeks / opens | 17 / 102,516 / 17 / 0 | **17 / 102,516 / 17 / 0** |
| ZHS physical reads / bytes / seeks / opens | 37 / 201,499 / 36 / 0 | **37 / 201,483 / 36 / 0** |

Eager mode 2 and deferred mode 3 produce identical same-pack logical request
sequences. Every English and Chinese RGB565 frame and canonical State byte
matches the saved UI17 `d4303ba` oracle. These are host-provider I/O counts;
they do not establish calculator FPS or OS read latency.

## Executed verification and scope

Every journal below is passed with `source_changed: false`. Journals are under
`ui18-storage/build/journal`; private inputs and outputs stay outside Git.

| Gate | Journal | Evidence |
| --- | --- | --- |
| Format, converter and split Python tests at 305c | `ui18-storage-python-full-144390f5b994` | 28 tests |
| Native tile/source/split/boot/file-cache regressions | `ui18-storage-native-gates-824a2bcf2c3a` | 5 suites; 18,733,171 tile checks |
| Scoped ASan/UBSan tile and ResourceSource regression | `ui18-storage-sanitize-d103ab817548` | 18,733,171 checks; leak checker disabled for the host ptrace environment |
| Format-only ARM | `ui18-storage-arm-a7b82e2355a4` | Ordinary Ndless build; fixed pools retained |
| Complete compacted resource comparison | `ui18-compact-fullimage-159471ea7f3e` | 169,595,016 checks; all 58,060 images, 1,167,930,140 decoded bytes |
| Compacted sixteen-language fixture | `ui18-compact-actual16-c78d99d00f6f` | 89,814 checks; 53,072 descriptions; no failures |
| Suffix repair and prior Python regressions at 3acb | `ui18-suffix-python-df002132e3d9` | 33 tests |
| Final aligned sixteen-language reconstruction | `ui18-suffix-align-all16-59bf12b0d652` | All encoded groups, glyphs, strings, metadata, suffixes and split round trips byte exact; 87.94 seconds |
| Final aligned sixteen-language runtime fixture | `ui18-aligned-actual16-a37c714e44fe` | 89,814 checks; 53,072 descriptions; no failures |
| Final bilingual complete frame/State/I/O replay | `ui18-aligned-io-oracles-3acfe53465cc` | 728 complete rendered frames; 364 compared directly with UI17; exact same-pack logical sequences |

The complete image comparison separately traverses the UI17 reference pack,
then the new memory and ranged readers. It is not a sampled image range or
hash-only comparison. It also compares all 13,299 English glyph records,
7,124 kerning pairs at 56,019 exact-size samples, 8,704 strings, 1,783 clips and
40 UI frames. The final alignment preserves every compressed group and all
retained image bytes from that accepted candidate. The integrated UI18
runtime's later memory/ranged/**incremental preload** gate is separate and
recorded by the main delivery work; the storage-only replay uses the unchanged
C++ core built at `305c1b7`, with tools frozen at `3acb5b3`.

The calendar additions enter through the frozen
`../private-ui18-calendar/patch-nsl2` input: ZHS and JPN each add the original
8 px Regular `午`, and Thai adds twelve original shaped calendar glyphs.
Existing glyph masks, metrics, strings and shared artwork remain exact.
That input is explicitly an intermediate NSL2 wrapper; only the final NSL3
files are counted here. Native date/culture UI and original-font glyph
acceptance are owned by the separate calendar implementation.

The full replay output and resource identities are in
`../private-ui18-aligned-native-oracles/receipt.json`. For each language, both
providers compare 27,955,200 RGB565 bytes and 41,966,288 canonical State bytes
with the preserved UI17 baseline. Earlier unaligned receipts remain preserved
as diagnostic evidence and are not the selected delivery input.

## Reproduction

Use two encoder workers and immutable UI17 data. This sequence performs the
complete sixteen-language conversion and the mandatory page-phase repair:

```sh
PYTHONPATH=../.python-ui17 python3 tools/coalesce_tile_resources.py \
  --input ../private-ui17-retained/data \
  --extra-locales ../private-ui18-calendar/patch-nsl2 \
  --output ../private-ui18-storage/coalesced-matched/data \
  --work ../private-ui18-storage/coalesced-matched/work --workers 2
PYTHONPATH=../.python-ui17 python3 tools/compact_tile_slots.py \
  --input ../private-ui18-storage/coalesced-matched/data \
  --output ../private-ui18-storage/compact/data \
  --work ../private-ui18-storage/compact/work \
  --layout-reference ../private-ui17-retained/data --workers 2
PYTHONPATH=../.python-ui17 python3 tools/align_resource_suffix.py \
  --input ../private-ui18-storage/compact/data \
  --reference ../private-ui17-retained/data \
  --output ../private-ui18-storage/aligned/data \
  --work ../private-ui18-storage/aligned/work
```

Run these long commands through `tools/run_logged.py` after committing and
creating an all-reference local bundle. The source-only storage checkpoint
bundle is `../ui18-storage-alignment-checkpoint.bundle`. No remote push was
attempted to bypass the earlier automatic-approval rejection.

Final resource checks use:

```sh
/lib64/ld-linux-x86-64.so.2 build/native/locale_editions_regression \
  ../private-ui18-storage/aligned/data
python3 tools/verify_resource_io_oracles.py \
  --binary build/native/ui18-resource-io-oracle \
  --interpreter /lib64/ld-linux-x86-64.so.2 --candidate-mode 3 \
  --data ../private-ui18-storage/aligned/data \
  --save ../private-ui17-device/fixtures/ui14-enemy-generated-cards.sav.tns \
  --progress ../private-ui17-device/fixtures/ui14-enemy-generated-cards.progress.tns \
  --baseline ../private-ui17-final-native-oracles \
  --output ../private-ui18-aligned-native-oracles \
  --runtime-checkpoint 305c1b70cc52e8a39ac12abcef5c31be53ca53ec
```

The replay output directory must be new; never overwrite preserved oracles.
