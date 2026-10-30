# UI18 exact tile storage work

Baseline: complete source `433445cf8218e7ac5d085cb4e4248d1c5393cb83`
and the immutable UI17 `44b239e` sixteen-language resources. Private outputs
are under `../private-ui18-storage`; the original packs are never overwritten.
No original texture, font, pack or calculator image is committed here.

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
not a multi-megabyte saving. Complete native/ARM/device and all-language gates
are still required for a selected final release.

The raw mapped-slot count includes already-cleared unreachable records; it is
not new pixel removal or a storage saving. No pixel codec, buffer, runtime
lookup or group raw size changes in this first candidate.

The valid initial Python receipt is `ui18-coalesce-python-3346e493bd89`, eight
tests passed. The earlier `ui18-coalesce-python-e9dd79897c22` used a mistyped
checkpoint argument and is retained as an invalid identity receipt, not used
as acceptance evidence.

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

This report will record the selected complete sixteen-language output,
additional calendar glyph bytes, full image oracle, ARM allocation and actual
target results after those commands run. The historical 65–70 MB installation
target is not marked achieved by this candidate or any sample.
