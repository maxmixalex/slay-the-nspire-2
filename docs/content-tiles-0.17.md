# Lossless sparse tiles for the 0.17 resource pack

`tools/content_tiles.py` is a separate **NSP2/14 → NSP2/15** repack stage. It
reads an already converted private resource pack. It does not reopen a game
archive, render an animation, change sampling, remove frames, rescale pictures,
quantize colors, or change any clip timing or pivot. The existing version-14
reader remains supported.

The new representation stores globally deduplicated **16 × 16 RGB565 and
8-bit-alpha tiles**, plus the original format-4 scanline geometry. Reconstructing
an image produces the original format-4 bytes, including the original choice of
opaque/alpha spans and nonzero RGB values beneath zero alpha. The runtime's
existing image-cache size classes therefore use the same decoded byte counts.
Using a dense three-byte-per-pixel cache entry would not preserve those classes.

This document defines the file format and converter gates. Source availability,
synthetic tests, a complete converted pack, native/WASM comparisons, ARM memory
bounds and physical-device performance are separate evidence stages. A
successful size diagnostic alone is not a production pack or a device-speed
measurement.

## Converting an existing pack

Run the tool from a preserved source checkpoint. Long conversions use the
repository's `tools/run_logged.py` workflow. All input/output assets, caches and
receipts remain outside this public source repository.

```sh
python3 tools/content_tiles.py \
  --input /private/input-world.nsp.tns \
  --output /private/output-world-v15.nsp.tns \
  --work /private/tile-work \
  --receipt /private/tile-conversion.json \
  --tiles-per-group 680
```

The default group size is 680 tiles, at most 523,600 raw bytes. Supported values
are 1 through 680. A smaller group reduces each individual decode, while often
increasing compressed storage and the number of groups referenced by a frame.
Choose it using complete-pack sizes and access traces. The parameter changes
storage grouping only; it does not change pixels, tile IDs or animation timing.

The default output cap is **80 MiB**, below the format's 256 MiB maximum. The
tool fails before publishing if the complete output exceeds the requested cap.
`--max-output-bytes` can change that explicit diagnostic/delivery constraint.
It never overwrites its input or an existing output. The fully verified file is
published with an atomic, non-overwriting link from a temporary file on the same
filesystem. Failure retains verified encoded caches and removes only temporary
spools/candidate files created by that invocation.

Input, output and receipt paths must be distinct and must not resolve inside
the writable `work/encoded` cache namespace. This guard also applies to the
Python API and follows directory symlinks. Before publication, the input path
must still name the same file that was mapped at the start; an unchanged old
mapping alone is not proof that its pathname was preserved.

The CLI requires the current complete world catalog. It selects images belonging
to clips appended after the preserved 38-clip foundation. Images referenced by
older clips, cards or auxiliary entries remain in their old format. A new clip
image sharing an existing retained payload also remains unchanged. All other
selected images must decode to sparse format 4, directly or inside a format-5
LZ4 wrapper; an unexpected inner format is rejected explicitly.

The converter uses the pinned dependencies in `requirements.txt`. Its public
LZ4 block decoder is a fallback if the optional native LZ4 module is absent.
Compression uses Python's raw LZMA2 interface, preset 6 and a fixed 512 KiB
dictionary. At most four compression workers and eight pending groups are
allowed. Temporary raw tile/index spools have a combined 2 GiB limit; they are
not a whole-pack dense-frame expansion. Only one source frame's pixel planes
are constructed at a time.

### Checked cache reuse

`--reuse-cache DIRECTORY` can be repeated for directories of raw LZMA2 payloads
and adjacent JSON group receipts. A candidate must identify preset 6, a 512 KiB
dictionary, exact raw length/SHA-256, encoded length/SHA-256 and raw CRC32. The
payload must be beside its receipt, within the encoded bound. **The converter
also decompresses it and compares the actual raw bytes with the newly built
group.** Matching metadata alone never permits cache reuse. Corrupt matching
cache entries fail explicitly instead of being silently accepted or hidden by
recompression.

The output receipt records the input/output hashes, encoder source hash,
complete byte accounting, canonical/timing image coverage, group limits,
verified cache hits and readback results. It is private runtime output, not a
source fixture or a file to publish with the repository.

## Outer pack and directory placement

The existing 40-byte NSP2 header is unchanged except for version **15**, the
new file length and FNV checksum. All integers are little-endian. The reference
identifier remains `0x41cef1ea`. Header words retain the order:

| Byte offset | Value |
| --- | --- |
| 0 | Four bytes `NSP2` |
| 4 | Pack version, 15 |
| 8 | Reference identifier |
| 12 | Complete file byte length |
| 16 | Image count |
| 20 | Glyph count |
| 24 | String count |
| 28 | Clip count |
| 32 | Card count |
| 36 | FNV-1a of every byte from offset 40 to EOF |

The base directories retain their existing order and record sizes: images
(16 bytes), glyphs (16), strings (8), clips (16), cards (16), then auxiliary image
IDs (4). The auxiliary count comes from the versioned catalog, not the outer
header. Image IDs, glyph keys and order, string IDs, clip records, card records
and auxiliary references remain identical.

Immediately after that base directory are the 64-byte tile header and the
8-byte-per-group lookup table. This **extended resident prefix** must fit the
existing 2 MiB directory allocation. The writer places the descriptor array
immediately after the prefix. Retained image payloads follow it, then all tile
groups, all index groups, retained glyphs and retained strings. Payloads with
identical actual bytes may share storage. Images and groups are even-aligned;
the required zero or one padding byte is counted in the final file.

No descriptor, ordinary image payload, glyph or string may overlap the extended
resident prefix. Ordinary image payloads, glyphs and strings must additionally
start after the entire descriptor array. Glyph keys are strictly increasing by
`(pixel_size << 24) | codepoint`; each UTF-8 string length is below 16,384 bytes,
excluding its terminating NUL. The unified group table is already resident after opening;
each normal group miss needs one file range for the group header and compressed
payload, not a second file read for its lookup entry.

## Global tile header: 64 bytes

The header is encoded as `<4s15I>`. Its address is the exact end of the base
directory and is repeated in every format-6 descriptor.

| Offset | Field | Constraint |
| --- | --- | --- |
| 0 | Magic | `TIL1` |
| 4 | Tile format version | 1 |
| 8 | Tile side | 16 |
| 12 | Raw LZMA2 dictionary property | 14, meaning 512 KiB |
| 16 | Unique tile count | 1 through `65535 × 300` |
| 20 | Tile group count | `ceil(tile_count / tiles_per_group)` |
| 24 | Index group count | 1 through descriptor count |
| 28 | Canonical frame descriptor count | 1 through image count |
| 32 | Absolute descriptor-array offset | Even, at or after the extended prefix |
| 36 | Absolute group-table offset | Header offset + 64 |
| 40 | Uncompressed tile record bytes | 770 |
| 44 | Tiles per group | 1 through 680 |
| 48 | Maximum uncompressed group bytes | 524,288 |
| 52 | Maximum encoded group bytes, including header | 617,323 |
| 56 | Flags | 0 |
| 60 | Reserved | 0 |

Counts, additions and multiplications must be checked before addressing memory
or reading a file range. A mathematically valid count is still rejected if its
resident lookup table would exceed the directory limit.

## Unified groups and lookup entries

There is one 8-byte `<II>` lookup entry per group: absolute file offset and total
encoded byte length, including the 32-byte group header. Unified IDs put every
tile group first, followed by every index group. Physical ranges follow that
same order, are even-aligned and never overlap. This permits a bounded O(n)
validation pass without sorting or an extra interval array.

Each group header is `<4s7I>`:

| Offset | Field | Constraint |
| --- | --- | --- |
| 0 | Magic | `TGR1` |
| 4 | Type | 1 = tile array; 2 = frame index records |
| 8 | Unified group ID | Exact lookup-table index |
| 12 | Raw byte length | 1 through 524,288 |
| 16 | Compressed payload byte length | Lookup length minus 32 |
| 20 | Raw CRC32 | IEEE/zlib CRC32 of the complete raw bytes |
| 24 | Item count | Actual tiles, or 1 through 64 frame records |
| 28 | Reserved | 0 |

The payload is an independent **raw LZMA2 stream**, without an XZ container.
Its dictionary property is supplied by the global header. The end marker must
occur at the declared output length, consume all compressed input and leave no
trailing bytes. CRC must match before the result becomes a usable cache entry.
The last tile group records its actual tile count, not the configured capacity.
Its raw length is exactly `item_count × 770`.

## Tile record: 770 bytes

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 1 | Actual width, 1 through 16 |
| 1 | 1 | Actual height, 1 through 16 |
| 2 | 512 | Row-major 16 × 16 little-endian RGB565 values |
| 514 | 256 | Row-major 16 × 16 alpha values |

Every pixel outside the actual edge dimensions is zero in both planes. An image
reference checks the tile's exact expected width/height, including transparent
edge tiles. A tile key includes dimensions and all RGB565/alpha bytes; a hash
match is followed by an actual byte comparison. Tile IDs are first-use IDs and
are never reordered to obtain a more favorable size or access trace.

For tile ID `t`, its group is `t // tiles_per_group` and its raw byte offset is
`(t % tiles_per_group) × 770`. The ID must be strictly below the global tile
count and the complete record must fit the group's actual raw length.

## Format-6 image descriptor: 16 bytes

An ordinary image directory entry uses format **6**, retains its original width
and height, has payload length 16, and points to a member of the descriptor
array. Canonically identical frames share that descriptor while all authored
timing image IDs remain present.

The four little-endian words are:

1. Absolute global tile-header address.
2. Local index-group ID, before adding the tile-group count.
3. Byte offset of a complete frame record within that raw index group.
4. Byte length of that record.

The descriptor address must be an exact 16-byte array member. The index-group
decoder scans its at-most-64 explicit record lengths and validates complete
coverage of the group's bytes. A descriptor must identify a record start and
its exact length; pointing into a record is invalid. Extra valid records in a
group need not each have a descriptor, although this writer emits none.

## Frame index record and original sparse geometry

Each record starts with three u32 values, followed by tile IDs and geometry:

| Field | Meaning |
| --- | --- |
| `original_fmt4_bytes` | Exact reconstructed sparse image size, at most 614,880 |
| `tile_index_bytes` | `4 × ceil(width/16) × ceil(height/16)`, at most 1,200 |
| `geometry_bytes` | Exact size of the following original span headers |
| Tile indices | Every row-major canvas tile, including transparent tiles |
| Geometry | Every original row/span header in its original order |

`record_bytes` must equal `12 + tile_index_bytes + geometry_bytes`. There is no
padding inside a record or between records. Consequently record headers and
u32 tile IDs may be unaligned; readers must use checked little-endian byte
loads rather than cast an unaligned pointer.

Geometry consists of exactly `height` rows. Each row has a u16 span count and
that many five-byte `<HHB>` span headers: x position, positive pixel count and
mode. Spans are ordered, do not overlap and stay within the image width. Mode 0
stores two original RGB565 bytes per pixel and requires tile alpha 255. Mode 1
stores the original interleaved RGB565 low byte, high byte and alpha byte. Zero
alpha does not authorize discarding the RGB bytes. Empty rows remain present.

The complete geometry must be consumed, and the sum of row/span headers and
their two- or three-byte pixels must equal the declared original format-4 size.
No frame CRC is added: the group CRC, outer pack FNV, structure checks and the
conversion's complete original-byte comparison provide the required gates.

## Runtime memory and access phases

The intended runtime cache has 64 pages of 64 KiB, or **4 MiB of raw storage**,
and at most 64 entries. The decoder needs one continuous output/dictionary
range, so allocation uses contiguous first fit. If no run is available, it
evicts the oldest eligible unpinned entry until a continuous run exists. Total
free-page count alone is insufficient. There is no heap, cache compaction or
second full-group output buffer. The current frame's index group remains
pinned while tile groups are consumed; cached groups needed for the frame are
consumed before missing groups.

Opening performs full integrity and structure validation, including every
referenced tile ID, edge dimensions and every loaded tile's padding. After a
successful open of the immutable source, ordinary image reconstruction can
derive an active-tile mask from the validated span geometry and avoid loading
tiles outside all spans. All tile IDs still receive range checks. Failed open,
reset and reopen clear this validation state.

Startup validation and normal active-mask reconstruction must be reported
separately. Access models also need to state whether the existing decoded image
cache is included, which replacement policy is used, and whether reads count
the resident prefix once or incorrectly charge it on every miss. Cold-frame
group count, compressed bytes and raw bytes are useful costs; none is an ARM
frame-rate measurement.

## Verification gates

`tests/test_content_tiles.py` uses authored synthetic pixels and a tiny pack. It
checks direct/LZ4 sparse inputs, actual v15 output and independent readback,
shared timing descriptors, all four boundary group choices, partial edges,
colored alpha-zero pixels, old payload sharing, 64-record index boundaries,
deterministic checked cache reuse, forced tile-hash collisions, corrupt caches,
format/count/range/CRC failures, strict LZMA2 termination, disk/output bounds and
non-overwriting publication. It contains no original assets.

For a complete private conversion, the encoder additionally proves:

1. Every input image belongs to exactly one retained or tiled partition.
2. Every compressed group inverse is byte-identical to the constructed raw
   group, including cache hits.
3. Every real output group passes bounds, CRC, tile-padding or complete-record
   validation through the independent reference reader.
4. Every canonical output frame reconstructs the exact original format-4
   bytes; every authored timing alias has the same source bytes and output
   descriptor as its representative.
5. Retained image payloads, glyphs, strings, clip records, card records and
   auxiliary entries remain byte-identical in meaning and content.
6. The original input hash remains unchanged, the real output FNV validates,
   and complete byte accounting equals the actual file length.

The Python reader's bounded 128-group host cache and temporary dense pixel
planes are independent verification machinery. They are not the calculator
cache model. Matching production C++ reconstruction, cross-target UI/save
comparisons, actual ARM static allocation and emulator/device execution remain
integration gates after the source checkpoint and the actual pack exist.
