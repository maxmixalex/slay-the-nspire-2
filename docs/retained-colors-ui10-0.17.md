# Lossless retained color groups — NSP2/36

The ordinary calculator capacity checks exposed a concrete migration failure:
the packed three-profile case fitted, but retaining old raw backups or old raw
full histories exhausted the filesystem before the temporary update completed.
Those cases require a smaller installed resource set and another actual write
and independent readback. A rounded free-space label does not settle the gate.

Before adding a format, a private reachability audit followed every image,
retained descriptor, tile/index group, glyph group and string group in both v34
packs. English had only 1,384 unreferenced bytes in 1,203 holes; Chinese had
1,448 bytes in 1,267 holes. They were one/two-byte zero alignment gaps. The
retained image, glyph and text groups had no unused decoded slices. Removing
unreachable appended payload cannot recover a material amount from these inputs.

## Exact transformation and bounded reader

Version36 permits an RGR2 group wherever an existing format7 retained image
descriptor refers to a group. The outer 16-byte descriptor is unchanged:
signed relative group offset, group byte length, decoded slice offset, and
format in the top byte of the decoded slice length. Versions34 and35 remain
readable, and the version35 glyph limit remains65,535 entries inside the same
2MiB directory allocation. Glyph and string groups retain their RGR1 format.

An RGR2 group has this little-endian layout:

| Offset | Field | Required value or bound |
|---|---|---|
| 0 | Four-byte tag | `RGR2` |
| 4 | Original decoded bytes | 1 through524,288 |
| 8 | Compressed stream bytes | Exact remaining stream length |
| 12 | LZMA2 dictionary property | 14, matching the existing512KiB decoder |
| 16 | Slice count | Nonzero and bounded by both table and decoded bytes |
| 20 | Color mode | 1 or2 |
| 24 | Table bytes | Slice count multiplied by8 |
| 28 | Reserved | Zero |
| 32 | Slice table | Repeated decoded offset and `(format<<24)|length` |
| After table | Raw LZMA2 stream | Existing decoder and exact-output checks |

The table is ordered, gapless and nonoverlapping, covers the entire decoded
group, and permits only formats0 through4. The converter keeps RGR1 when an
older group has an unreferenced slice or cannot be described this way. Total
encoded group size stays within the existing617,323-byte transfer limit.

Both color modes are bijections of all65,536 RGB565 values. Mode1 stores green
with modulo32 red/blue differences from half green. Mode2 additionally zigzags
those differences. The decoder restores RGB565 once, in place, before publishing
the existing decoded cache group. Format0 transforms its RGB plane; format1
transforms its contiguous RGB plane and leaves the following alpha plane
unchanged; formats2/3 transform only the palette; format4 transforms the colors
inside its original sparse spans. Alpha, palette indices, row counts and span
geometry are unchanged. No image is resized and no animation frame is dropped.

The historical offline experiment treated dense format1 as interleaved RGB/A.
That transform was reversible but did not preserve its alpha bytes in the
filtered representation. The current encoder and inverse use the actual two
planes. Historical candidate bytes are reused only after decoding and matching
the correct filtered byte sequence; all dense-alpha groups are reconsidered.

RGR2 uses the existing transfer lease for encoded bytes and the table and the
existing decoded tile/retained pool for its output. It adds no resident pixel,
table, decoder or save buffer. Bounds and required format checks run only when
the requested group is cold. No file checksum or eager startup validation scan
is added. Any decode/filter failure discards the uncommitted group and releases
the lease, retaining the existing owner/error semantics.

## Converter and proofs

`tools/compact_retained_colors.py` accepts private v34/v35 inputs, writes v36
to new paths and never overwrites the input. Each complete candidate includes
its32-byte header,8-byte entries, LZMA2 stream and alignment; it replaces RGR1
only when that complete representation is strictly smaller. The converter
preserves the existing LZMA2 profile alternatives, and its resumable cache is
validated by decoding before reuse.

Replacements relocate all absolute image/font/text pointers, the tile table
and frame descriptors, and every relative retained/font/text group link.
Aliases keep their identities. An attempted live pointer into the interior of
a replaced group fails rather than guessing its new position. The second
locale must match each original compressed artwork group and slice table before
it can reuse the first locale's plan. The finished converter compares all image
pixels and alpha, all glyph metrics and bytes, every localized string and clip
clock, every compressed tile/index group and the reconstructed bilingual split.

Public synthetic tests cover both transforms, all RGB565 values, dense alpha,
palettes, sparse spans, old and new direct/streamed readers, aliased images,
malformed counts/offsets/table coverage, unsupported versions, failed I/O,
owner reopen, shared save/history workspace and reentrant callbacks. Converter
tests also exercise independent locale directories, grouped glyph/text pointer
relocation, corrupt cached streams, nonbeneficial fallback and idempotence.

Initial short checks passed3,403,371 retained-content checks and four synthetic
converter tests, together with the six exhaustive Python transform tests.
These are implementation checks. The source checkpoint, actual v35 input
conversion receipts, whole-frame cold-enter CPU/I/O comparison, final linked
ARM allocation and ordinary calculator capacity retry are separate gates.
No installed saving or device performance improvement is claimed here yet.
