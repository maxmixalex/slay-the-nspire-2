# Bounded retained-image storage — NSP2/20

NSP2/20 keeps the NSP2/19 image, clip, card, auxiliary and localized-text
catalogs. The change compresses ordinary retained images in small groups, in
addition to the existing tiled animation storage. It does not reduce image
dimensions, change RGB565 pixels, discard alpha, simplify palettes, omit
frames or change timing. The title restoration may deliberately replace image
4 with an original-derived atlas; that replacement is recorded separately.

## Image format 7

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

## Memory and I/O

Retained groups reuse the existing five MiB tile-group cache, its 40 pages and
its age/eviction behavior. No second group pool or full-image resident array
is added. Only the requested image slice is copied into the existing image
cache. Neighbors can then reuse the decoded group; unrelated groups remain
unread. Reusing this cache may change eviction patterns and requires actual
scene measurements before claiming calculator acceleration.

The last two formerly unused `TIL1` header words, at offsets 56 and 60, hold
the absolute decoded-cost-table offset and image count. One byte per image
follows the group lookup table, padded to an even byte count. Each byte stores
the number of 4 KiB decoded image pages. The resident directory reads these
costs directly, so preparation can budget sparse images without decoding or
scanning every animation index group during startup. Earlier resource versions
use geometry bounds until their first actual decode.

NSP2's historical checksum word and `TGR1` CRC words are unused and newly
written as zero. Split-container digest/checksum words are also unused. The
runtime still needs actual format, position and size checks to read a selected
resource. Offline exact-byte conversion comparisons are separate from runtime
startup work; no whole-pack integrity pass is introduced.

## Conversion and sharing

Use private directories outside the public source repository:

```sh
python tools/repack_retained.py \
  --eng /private/merchant-eng.nsp --zhs /private/merchant-zhs.nsp \
  --output /private/retained
python tools/split_content.py \
  --eng /private/retained/retained-eng.nsp \
  --zhs /private/retained/retained-zhs.nsp --output /private/split
```

`--logo-atlas /private/title/title-logo.json` replaces fixed image 4 without
adding images or clips. The manifest supplies width, height, original format
and a `raw_file` relative to the manifest. Its intended content is the title
agent's original Spine-derived static lettering plus ten small flame frames.

English is planned once. Chinese uses the same groups after direct comparison
of every retained input's original format/raw bytes. The converter compares
each distinct retained image, every glyph/string and all catalog/timing rows.
Animation group compressed payloads are copied and compared directly; their
CRC words alone are cleared. Existing tile descriptors remain byte-identical
and move only to make room for the small cost table. Split conversion requires
identical artwork and exact reconstruction of both full resource streams.

`retained-receipt.json` records actual input/output sizes, grouped entries,
largest group, comparison counts and any deliberate image replacements.
`--group-kib 512` is available for a measured storage/decode tradeoff, not the
default. A smaller file by itself is not evidence of faster cold scene access.

## Recovery checkpoint status

The source and synthetic tests were restored after temporary workspace
maintenance. The new resources and platform binaries must be rebuilt and
measured after this source checkpoint. Historical quick checks from the lost
workspace are not relabelled as executions of the restored source. No new
installation size, physical calculator timing or finished delivery is claimed
by this format specification.
