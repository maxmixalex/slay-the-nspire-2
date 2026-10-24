# Lossless tile stream filtering — NSP2/33

The original `TGR1` stream remains supported. A `TGR2` color group uses the same
32-byte header, group ID, item count, raw-byte bound and LZMA2 property. The last
header word is a filter mode, either 1 or 2. Only a tile group (kind 1) may use
this form, and only in NSP2/33 or later. Index groups retain `TGR1` and mode zero.
Unknown modes, mismatched counts and dimensions outside 1–16 are rejected before
a decoded group is published.

For N tiles, the decoded filtered stream contains N two-byte width/height pairs,
N × 512 bytes of little-endian transformed color words, then N × 256 untouched
alpha bytes. Tile order and pixel order within each tile do not change. For
each original RGB565 value `(r, g, b)`, the word contains six-bit `g`, five-bit
`(r - (g >> 1)) mod 32`, and five-bit `(b - (g >> 1)) mod 32`, in that low-to-high
order. Mode 2 additionally maps the two signed modular differences to zigzag
codes. The inverse restores the complete original RGB565 value, including bits
under alpha zero. No image is sampled, palette-converted, resized or omitted.

After LZMA2 consumes its encoded input, the runtime copies its result to the
existing transfer scratch, then restores the original 770-byte records in the
already reserved tile-cache pages. There is no new resident pool, dictionary,
per-group field or cache page. The 512 KiB raw bound remains in force. A cache
hit returns before decoding or filtering, as it did previously. Cold decoding
adds a bounded copy and inverse transform; actual cost must be measured with
the final ordinary program and matching resources.

`tools/compact_tile_resources.py` tests two filter forms and two bounded LZMA2
literal-context configurations. It keeps the original whole group unless the
new physical span is smaller, including alignment. The content-addressed build
cache is validated by complete decoding before reuse. The tool adjusts every
absolute offset and every relative retained-image, grouped-glyph and grouped-
string descriptor. It verifies all decoded tile/index bytes, unchanged index
streams, frame slices, timing, retained streams, font payloads, strings and
prepared image-page costs. Input packs are never overwritten.

Short synthetic verification covers all 65,536 RGB565 values, alpha bytes,
1/256/680-tile groups, raw and ranged native readers, old streams, malformed
headers, cross-locale relocation and split reconstruction, grouped text and
forward retained pointers. These checks are codec evidence; they do not imply
original-game fidelity or physical-device frame rates. The final frozen gate
also compares real pack room-frame transcripts and reports cold/warm CPU and
provider I/O. `room_render_benchmark` can write every rendered frame hash after
the timed interval using its optional final transcript argument.
