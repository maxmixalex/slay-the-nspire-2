# Resource formats and ownership

This page records the contracts used by the runtime and converter. Resource
format versions are independent of the original game's version. The readable
maximum is shared by `MaximumContentVersion` in the native code and
`MAXIMUM_CONTENT_VERSION` in `tools/content_version.py` (currently NSP2/46).
Older conversion stages keep their own explicit input/output versions.

## Installed containers

| Layer | Format | Responsibility |
| --- | --- | --- |
| Resource index | NSM1 | Map the logical common artwork file to eight physical parts |
| Artwork parts | NSS1 | Store complete decoder groups by concern |
| Locale pages | NSL3 | Independent 4 KiB LZ4 or literal pages around an NSL1/NSL2 container |
| Split content | NSC1 + NSL1/NSL2 | Reconstruct one logical NSP2 stream from shared artwork and localized directory/text/glyph sections |
| Game content | NSP2 | Image, glyph, text, clip, card and auxiliary directories |

The filenames are defined by `ArtworkPartNames` in
[`storage_source.hpp`](../include/storage_source.hpp). The eight parts are
required for every language and animation profile. Locales must come from the
same conversion as their artwork because rewriting common storage can move
addresses even when image IDs and pixels stay unchanged.

`MultipartResourceSource` keeps at most eight parts and 128 ordered spans.
Spans cover the complete virtual range without overlap. Groups stay intact
across physical files. The file cache closes an evicted stream before opening
another and gives each handle its own seek cursor.

`BlockedResourceSource` permits at most 2,048 locale pages, with a 32 KiB encoded
buffer and one 4 KiB decoded-page cache. Literal ranges can be read directly;
mixed ranges batch only bytes belonging to the current request. Its buffers
are independent of the image transfer workspace, so glyph reads can happen
while an image decoder owns that workspace. Recursive use of the same adapter
is rejected and a failed decode never becomes a cached page.

NSL2 stores its directory in independent 64 KiB LZMA2 blocks. Its raw tail is
page-aligned to avoid decoding a directory block during a later image read.
The split adapter retains bounded addressing rows, and all underlying sources
must outlive the adapter. See [`resource_source.hpp`](../include/resource_source.hpp)
and [`storage_source.cpp`](../src/storage_source.cpp) for the actual layouts.

## Reads and cache lifetimes

`ResourceSource` is an immutable memory range or exact range-read callback.
Offsets are unsigned 32-bit values. A provider must fill the whole request or
return false, must not retain the destination, and must stay alive until its
readers are reset. Check ranges by subtraction before forming pointers.
A zero-length read at EOF is valid.

`ResourceReader` has sixty-four 4 KiB pages with second-chance replacement.
Direct memory and full-page requests bypass this metadata cache. A reset
invalidates every page. No cache pointer escapes; integer outputs change only
on success and bulk destinations are unspecified after failure.

Decoded image, tile, glyph and string views have bounded lifetimes. Do not hold
a view across an operation that can reuse its slot. Cache identity includes the
owning `Content`, and open/reset invalidates old ownership. String formatting
must consume a packed string before making another lookup that can replace it.
Directory pointer arithmetic must validate integer offsets before forming the
pointer; subtracting an adjustment after an out-of-range pointer is already UB.

The image transfer area, retained-group decoder and foreground state workspace
are shared resources with explicit owners. Incremental preparation pins its
source and destination reservations until completion or cancellation. Locale
page decoding must not borrow the image transfer area. Scene-specific picture
and tile partitions reuse the same arena; enlarging one without accounting for
the complete arena is not a memory saving.

Runtime startup checks format identification, functional addressing, required
parts and bounds. Payloads decode on demand. Legacy checksum fields remain in
several formats, but whole-file FNV scans, integrity-only CRC checks and eager
resource-wide validation are not runtime gates. Exact decoded comparisons in
developer tests are separate from startup work.

## Base tile format (introduced in NSP2/15)

### Outer pack and directory placement

The base NSP2/15 representation has a 40-byte header. Later storage versions
retain these fields and extend the codecs described below. All integers are little-endian. The reference
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
format's resident-directory limit. Current native readers use compact addressing
metadata within the allocation defined by `Content`. The writer places the descriptor array
immediately after the prefix. Retained image payloads follow it, then all tile
groups, all index groups, retained glyphs and retained strings. Payloads with
identical actual bytes may share storage. Images and groups are even-aligned;
the required zero or one padding byte is counted in the final file.

No descriptor, ordinary image payload, glyph or string may overlap the extended
resident prefix. Ordinary image payloads, glyphs and strings must additionally
start after the entire descriptor array. Glyph keys are strictly increasing by
`(pixel_size << 24) | codepoint`; each UTF-8 string length is below 16,384 bytes,
excluding its terminating NUL. Current readers compact resident addressing metadata; every resolved group range
must still stay inside its owning resource.

### Global tile header: 64 bytes

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

### Unified groups and lookup entries

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
trailing bytes. The CRC word remains in the file layout; current runtime decoding does not
perform integrity-only CRC checks.
The last tile group records its actual tile count, not the configured capacity.
Its raw length is exactly `item_count × 770`.

### Tile record: 770 bytes

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

### Format-6 image descriptor: 16 bytes

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

### Frame index record and original sparse geometry

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
No frame CRC is added. The runtime checks addressing and exact decoding for
requested data; legacy CRC/FNV fields do not impose integrity scans.


## Retained image descriptors

The ordinary 16-byte image-directory row still stores offset, width, height,
format and byte count. Format 7 points to a 16-byte descriptor:

| Byte | Type | Meaning |
| ---: | --- | --- |
| 0 | signed 32-bit | Group offset relative to this descriptor's file offset |
| 4 | unsigned 32-bit | Complete group bytes, including the 32-byte header |
| 8 | unsigned 32-bit | Image byte offset inside the decoded group |
| 12 | unsigned 32-bit | Original format in high byte, decoded byte count in low 24 bits |

Original format is 0..4. These are the existing dense RGB565, dense alpha,
palette, palette/alpha and sparse-row representations. The relative group
offset is intentional: Chinese and English have different directory sizes,
but their complete artwork region remains identical and can be shared.

Each `RGR1` group has a 32-byte `<4s7I` header: tag, decoded bytes, encoded
bytes, LZMA2 property 14, informational image count and three unused words.
Raw LZMA2 uses the existing 512 KiB dictionary and 512 KiB maximum decoded
block. There is no checksum. Functional size/slice bounds and decoder results
are checked when a requested picture is loaded.

The default encoder target is 128 KiB decoded per group. An individual larger
picture may form a group up to 512 KiB. Pictures exceeding that bound retain
their previous encoding. Groups also retain the previous encoding when the
compressed group plus descriptors would take more space. The converter
deduplicates exact original-format/raw-byte pairs and can retain the shorter
existing representation of a duplicate.

The retained-image converter's `--fast-images ui` policy prioritizes card/UI
first-use latency. `cards` retains only card artwork and `none` groups every
beneficial retained image. Grouping must be assessed against scene access
patterns as well as encoded size, since loading a small picture can otherwise
require decoding a much larger group.

## Grouped glyphs

The glyph directory remains 16 bytes (`<IIHBBHH>`). Codepoint, packed native
size/face, width, height and advance are unchanged. In the final encoded-length
field, `0x8000` identifies a grouped glyph. Its low 15 bits retain the previous
RLE byte count; zero still means a raw `width × height` alpha plane. No new
resident directory table or cache is needed.

A grouped directory entry's offset identifies a 16-byte descriptor:

| Byte | Field | Meaning |
| --- | --- | --- |
| 0 | signed 32-bit relative offset | RGR1 start relative to this descriptor |
| 4 | unsigned 32-bit length | Entire compressed group including its header |
| 8 | unsigned 32-bit offset | Glyph payload start in the decoded group |
| 12 | unsigned 32-bit length | Exactly old RLE bytes, or width × height |

Groups use the existing `<4s7I>` RGR1 header: tag, raw bytes, compressed bytes,
LZMA2 property 14, number of distinct payload records, and three zero words.
Decoded glyph groups are at most **65,536 bytes**. The converter offers 32 KiB
and 64 KiB targets. The encoded block uses the existing 512 KiB LZMA2 dictionary
and transfer bound. It ends exactly at the LZMA2 end marker, with no unused or
trailing compressed input. A descriptor's entire decoded slice must fit the
group. Raw and RLE slice lengths are exact, and RLE counts must be nonzero and
sum to width × height.

The runtime copies the requested slice into the existing glyph cache. Public
`Glyph.encoded_size` contains only the old raw/RLE value. Its pixel lifetime and
rendering contract do not change. A cold glyph can reuse the existing grouped
resource cache; a warm glyph does not reread or decompress its font group.

## Grouped strings

A string directory remains two little-endian unsigned 32-bit fields: offset and
length. Bit `0x80000000` of the length identifies grouped storage. The low bits
retain the original UTF-8 length excluding its terminal NUL. The string remains
strictly below 16,384 bytes; the longest accepted payload includes 16,383 text
bytes plus its NUL.

The offset of a grouped string points to the same 16-byte descriptor shape used
by grouped glyphs:

| Offset | Field | Required meaning |
| --- | --- | --- |
| 0 | signed relative group offset | RGR1 start relative to this descriptor |
| 4 | group byte length | Header and complete raw LZMA2 stream |
| 8 | decoded slice offset | First byte of the exact UTF-8 payload |
| 12 | decoded slice length | Original string length plus its NUL |

The descriptor must not overlap its group. Groups use the existing RGR1 header,
property 14 and 512 KiB codec dictionary, with at most **32 KiB decoded bytes**.
The item count is positive and no greater than 16,384; the final three header
words are zero. The runtime checks the small decoded bound and header before
loading a group, including when another record has already warmed that group.
A slice must fit wholly within the decoded group and end with a NUL.

The locale marker at string index 6110 always remains literal, including any
identical aliases. `Content::open`, split-resource probing and boot error text
therefore require no string decompression or extra preload pass. The latest
version gates accept 33, and future-version fixtures now test 34.

## Runtime ownership and lifetime

A cold request first checks the existing 102 KiB, 1,024-entry string cache. A
miss borrows the existing transfer workspace, loads through the existing decoded
resource-group cache and copies only the requested slice into the ordinary
string cache. It does not allocate another static buffer or cache. Cached text
continues to work during a Save/Codec lease. A cold request during an incompatible
lease returns an empty result without reading, evicting text, changing the save
workspace or poisoning Content health; a later request can retry.

Returned views retain their prior string-cache lifetime: unrelated glyph and
image loads, including eviction of the original compressed string group, do not
move or invalidate a surviving string entry. String-cache eviction or reopening
its Content owner can invalidate it, as before. Direct-memory packs use this same
stable cache for grouped values; their literal values still point directly into
the input. Read or decode failures publish no new valid text entry and mark the
Content unhealthy; reopening clears that owner's caches.


## Later codecs

| Format | Meaning | Implementation |
| --- | --- | --- |
| TGR2 | Reversible tile color filtering | `include/tile_filter.hpp`, `tools/tile_filter.py` |
| TGI2 | Predicted frame-index streams | `include/packed_content_index.hpp`, `tools/tile_prediction.py` |
| RGR2 | Reversible retained-image color transforms | `include/retained_filter.hpp`, `tools/retained_filter.py` |
| RGR3 | Separate retained geometry, color and alpha planes | `include/retained_planes.hpp`, `tools/retained_planes.py` |
| TGR3 | Sparse tile planes with occupancy masks | `include/sparse_tile_filter.hpp`, `tools/sparse_tile_filter.py` |
| NSP2/46 geometry | Implicit image geometry encoding | `tools/implicit_geometry.py` |

RGR3 preserves image lengths, slice positions, hidden RGB beneath zero alpha
and the 512 KiB retained-group bound. Plane restoration reuses existing scratch;
its complete slice table and largest scratch slice must fit before conversion
can select that representation.

TGR3 omits a pixel only when both RGB565 and alpha are zero. Hidden colors and
record padding survive. Sparse selection is limited to 60% occupancy and must
fit the existing scratch space. Mixed tile groups can use the versioned 1 MiB
bound introduced in NSP2/44; this does not enlarge the retained-image bound.

Compaction may remove unreachable tile records or retained slices, but must
rewrite every reference and preserve live data, aliases, action clocks and
locale address phases. Recompression must not change decoder input. The
converter keeps source sampling, color selection, storage compression and
runtime presentation as separate operations.

## Checks when changing a format

The Python reader in `tools/content_tiles.py` and native reader in
`src/content.cpp` must agree on versions, counts, flags, ranges and restored
bytes. Use the existing synthetic format tests for malformed lengths, aliases,
page boundaries, interrupted preparation and reset after failure. The linked
ARM decoder tests catch target-specific instruction and ABI mistakes.

For private resource work, compare every decoded image/glyph/string and clip
clock, reconstruct every requested locale, then exercise actual storage reads
and representative scene transitions. Smaller archives or equal native images
alone do not establish calculator I/O or performance.
