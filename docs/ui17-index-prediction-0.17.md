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
