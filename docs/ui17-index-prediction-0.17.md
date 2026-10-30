# UI17 exact frame-index prediction

The recovered UI16 installation is the immutable reference: 81,022,714 bytes
for its launcher and default English/Chinese resources. The new production
candidate changes the representation of animation tile indices, preserving
every image, RGB565/alpha bit, sparse byte, frame clock, string and glyph.
Conversion size, native correctness, ARM instruction behavior and ordinary
Firebird performance remain separate acceptance gates.

## Why this part of the pack

The complete UI16 artwork inventory contains 46,151,268 encoded tile-group
bytes, 7,433,477 frame-index-group bytes, 19,490,077 retained-image-group bytes
and 717,925 literal-image bytes, plus their descriptors and alignment. Its
784,936 tiles include 95,730 already-cleared unreachable records and 24,936
live exact duplicates. The cleared records are not new savings.

A deterministic sample weighted by stored index bytes showed that an index's
corresponding position in the prior record, or the prior position in the same
record, often predicts its tile ID with a smaller integer. The single mixed
predictor won all 24 sampled groups, saving 62,492 of 579,264 bytes. This is a
sample result, not the full conversion size. A separate same-group pixel
predictor made all 24 sampled tile groups larger and is excluded from the
production format and runtime. Existing rejected byte-plane/LZMA-parameter
experiments were not repeated as a claimed implementation.

## NSP2/41 and TGI2

The latest recognized logical resource version becomes 41. Older NSP2/40 and
their TGR1/TGR2 groups remain supported. An older program rejects a new NSP2/41
header, rather than accepting the file and failing later on its first new
index stream. The language boot probe and split-source version bounds agree
with Content's bound. NSM1, NSS1, NSL2 and NSL3 storage envelopes are unchanged.

TGI2 is an index-group alternative to TGR1. It keeps the same 32-byte group
header, unified group ID, type 2, original decoded length, item count and
independent raw LZMA2 stream. The final header word is exactly 1. Tile groups
cannot use TGI2. Each group contains one through 64 complete records within
the existing 512 KiB limit.

The 12-byte record header, sparse geometry and record boundaries are unchanged.
Only the tile-ID words differ. For each stored word E:

1. Bit zero chooses a predictor. Zero selects the preceding restored tile ID
   in this record, or zero at its first position. One selects the matching
   position in the preceding record if both grids contain the same number of
   IDs; otherwise its predictor is zero.
2. Z = E >> 1 is a zigzag difference. Its signed value is `(Z >> 1) ^ -(Z & 1)`.
3. Adding that difference to the predictor produces the original tile ID.
   Both the difference and final ID must remain within the global tile bound.

The encoder chooses the smaller difference, preferring the spatial predictor
on ties. Every record is restored forward in place, including unaligned ID
words. The prior record already occupies the same group cache allocation.
No cross-group dependency, extra provider call, new cache pool or resident
buffer is needed. A malformed stream discards its unpublished group through
the existing failure path.

## Converter and evidence contract

`tools/predict_index_resources.py` builds one English plan and requires every
other locale's old compressed artwork to agree before applying it. It selects
TGI2 only when the complete even-aligned physical span is smaller, preserving
the previous parity. All compressed pixel streams are copied unchanged. The
converter compares every decoded index group, every image/frame descriptor,
all retained-image groups, glyph payloads, metrics, strings and catalog clocks.
It then builds the eight concern files and all requested independent locales,
with an exact split/reassembly comparison for each edition.

The synthetic Python gate covers full conversion, idempotency, grouped text
and fonts, bilingual relocation, malformed metadata and corrupt encoder
caches. The independent C++ inverse gate includes 64-record groups,
300-ID grids, changing grid lengths, unaligned records, large IDs, guard
bytes and malformed lengths/counts/differences. Content's synthetic provider
gate exercises actual TGI2 loading, image equality, aliases, warm reuse,
version rejection, failed reads and reopen recovery. Full-resource and ARM
results are recorded after executing the frozen production checkpoint.

## Completed first candidate at c495273

The full first-stage conversion, independent of later retained-image work,
completed at `c495273b9a7d66aef5f9bce0879fefd02527f399`. Of 500 index groups,
496 become TGI2. Their complete artwork footprint decreases by **807,004
bytes**. All sixteen editions reconstruct and pass the converter's exact
proofs, including **68,020,408 decoded index bytes per edition**. The 1,155
compressed tile groups, every retained group and all 58,060 image identities
are unchanged. No previously cleared unused tile is counted in this saving.

| Component | First index candidate, bytes |
| --- | ---: |
| Common index plus eight concern files | 73,219,102 |
| English | 779,119 |
| Simplified Chinese | 4,456,897 |
| All sixteen locale files | 23,785,938 |
| Actual ARM launcher | 1,761,477 |
| Launcher plus default English/Chinese resources | **80,216,595** |
| Launcher plus all sixteen editions | **98,766,517** |

The default total saves **806,119 bytes** relative to UI16; all sixteen save
**809,764 bytes**. This remains above the prior installation-size target.
Launcher growth is counted rather than treating the 807,004 artwork saving
as the final installed saving.

The ordinary ARM build produces SHA256
`88ed66cfba79527128d95f76444e13377399ae6eb64e27eea1f3007d923cc738`,
with Zehn allocation **22,524,356 bytes**, 872 bytes above UI16. No static
buffer or image-cache allocation changes. The actual ARM inverse's stack
frame is 80 bytes; `load_tile_group` uses 184 bytes. These are compilation
and instruction-layout evidence, not calculator timing.

All completed journals below have `source_changed:false`:

| Gate | Result / journal |
| --- | --- |
| Complete sixteen-edition conversion | `ui17-index16-production-56d537b77f24`, passed in 230.95 seconds |
| New Python converter tests | 6 passed, `ui17-index-python-7b3c2cd80450` |
| Affected native suites | 5/5, `ui17-index-native-gates-d86e48aacce2` |
| Actual sixteen editions | 89,816 checks, 53,072 descriptions, zero failures; `ui17-index-actual16-1c32c603dbc3` |
| Changed Content/resource-source ASan and UBSan | 18,706,375 checks; `ui17-index-content-sanitize-fba074a50040` |
| Ordinary ARM build | `ui17-index-arm-d0056f0af0c7` |
| Complete actual English byte/UI oracle | `ui17-index-sequential-eng-90b337e29457`, test checkpoint `f5a31dd841ed528b22d4e65e97b34e8ff98e84fc` |

The last gate compares **all 58,060 images / 1,167,930,140 decoded bytes**
against the independent UI16 resource decoder, for both new memory and ranged
backends. It also compares 13,299 glyph records (7,124 kerning pairs and 56,019
exact-size samples), 8,704 strings, 1,783 clips and 40 complete UI frames;
**169,595,016 checks pass**. It writes the old decoder's complete bytes to an
automatic temporary stream once, then compares both new backends byte for
byte in ordinary sequential access order. It does not replace bytes with hash
comparison or reduce image coverage. Synthetic ownership/eviction cases and
the UI oracle retain their alternating-owner coverage.

The earlier `ui17-index-actual-eng-be9f7fc11c71` invocation used the old test's
per-image owner alternation, causing repeated eviction from the one fixed
group pool. It was stopped with the execution tool after the complete
sequential oracle passed. Its journal remains unfinished, not passed; that
superseded invocation is not used as acceptance evidence. The changed-path
sanitizer build instruments Content and the resource-source translation units;
the unrelated core archive is uninstrumented and leak detection is disabled
under the executor. No whole-game sanitizer or Firebird speed claim follows.

Private first-stage files remain at `private-ui17-index/data`, with complete
default logical files under `private-ui17-index/work`. Later retained-color
output must use a distinct directory and report its additional savings and
actual performance separately.
