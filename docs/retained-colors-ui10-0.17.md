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
These initial checks are superseded by the concrete resource proofs below;
installed capacity and device performance remain separate evidence.

## Actual resource and linked-reader evidence

The frozen `cb52b945fe9ba66f63ff97e4ab08fe5c4f9f7104` conversion completed
against both initial NSP2/35 scene-font packs. It replaces689 of840 groups
and saves949,484 bytes per full pack, or once in the shared bilingual artwork.
All51,123 images per locale compare exactly after decoding. The8704 strings,
1783 clip clocks, every glyph payload and1184 compressed tile/index streams
also remain exact. The initial bilingual split totals92,183,452 bytes; later
font and room-art additions change the final installation size.

The same stage-C native UI was then run against the original35 and filtered36
actual ranged-file packs. Each locale produced867 card/library/inspector/shop
frames and549 room frames. All2,832 paired RGB565/hit/canonical-state hash
transcripts and scene observations match, with unchanged canonical saves and
zero save callbacks. English card/room reads decrease113,251/57,470 bytes;
Chinese card reads increase21,917 bytes after page relocation, while its room
reads decrease59,834 bytes. This is a functional/I/O proof, not a timing claim.

The linked ARM926 RGR2 inverse passes 86 vectors, including 65 rejected
inputs: both modes, all five pixel formats, every RGB565 value, dense and
sparse alpha, palette indices,512KiB output, odd slice offsets and malformed
metadata. Every call preserves r4–r11, SP, output guards and the slice table.
`tests/arm_retained_filter_regression.py` runs the production ELF symbol rather
than recompiling the header for a host CPU. It also passes formally on stage D,
checkpoint `78ac86e35651d3832e1b9638001aa0aa6be3f0a0`, in journal
`ui10d-arm-fedaed52f2ad.json`. That ELF has SHA-256
`1ec409e704592fb587aee3ac69af46478122b3df4977cef29e6afcefef1c56a1`;
its linked inverse occupies 476 bytes. This is instruction/ABI correctness,
not physical calculator speed.

## Size and measurement boundaries

The successful stage-D resource journal is
`ui10d-resources-legacy-receipt-d763cd064f10.json`. After the complete D font
append and tail compaction, the artwork conversion still replaces 689 of 840
groups and saves exactly **949,484 bytes**. The full output verification again
decodes all 51,123 pre-append images in each locale. This artwork saving counts
once in a bilingual split; adding the two full-pack savings would double-count
the common file.

The D version-37 split, after six original room images are appended, contains:

| File | Bytes |
|---|---:|
| Common artwork | 84,742,754 |
| English directory, fonts and text | 2,183,654 |
| Chinese directory, fonts and text | 5,482,346 |
| Resource total | **92,408,754** |

The corresponding split receipt is `split/split-receipt.json` in the private D
resource output. It excludes the launcher, filesystem allocation overhead,
save/history files and temporary migration copies. Final E adds three Chinese
glyphs and a small original history icon; its final size and actual calculator
migration capacity still require their own receipts. The 30–40 MB goal has
not been reached.

The private bundles `private-performance-storage/retained36-initial/` and
`private-performance-storage/rgr2-actual-frames/` hold the initial conversion
and stage-C comparison receipts. The latter's passed journal is
`ui10c-rgr2-actual-frames-21ef9ffcc3c3.json`. Same-UI comparison uses the same
runtime on the initial matching 35/36 packs, with actual ranged file reads;
the final 38 artwork additions cannot be silently compared against an older
catalog and called a compression-only result. A focused cold-entry and warm
merchant comparison will report process CPU, elapsed time and I/O separately.
Its timing is pending a quiet host window. The exact frame/I/O evidence above
does not establish either a universal I/O reduction or calculator FPS.

## Receipt reuse correction

The historical compression probe records `chosen`/`stream`; this converter's
first receipts instead recorded `mode`/`saved_bytes`. An attempted reuse of the
second schema treated a missing `chosen` field as the first schema's explicit
nonbeneficial result. That incomplete candidate retained most original groups
and reported only79,542 bytes saved. The attempt was stopped and preserved as
unfinished, then resumed from the already completed font stage with the known
legacy receipt. Its files were not accepted as the final installation.

Reuse now recognizes both schemas explicitly. Current receipts include a
slice-table fingerprint. An earlier current receipt can recover its exact
compressed streams from its matching sibling output, after verifying that
output's receipt hash; each imported stream still must decode to the requested
filtered bytes. Successful imports populate the ordinary content-addressed
cache. Missing winner files are recompressed, unknown receipt shapes cannot
assert that a group is nonbeneficial, and already-filtered rows lacking a raw
fingerprint no longer cause a receipt-read error. Seven synthetic converter
tests cover these cases, including identical output from a self-receipt and an
empty cache with recompression explicitly forbidden.
