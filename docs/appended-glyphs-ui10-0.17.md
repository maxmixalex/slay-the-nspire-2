# Compact newly appended scene fonts

`tools/compact_appended_glyphs.py` applies the existing NSP2/32 grouped-glyph
format to the new literal tail produced by `scene_ui9_fonts.py`. The resulting
pack remains NSP2/35. It adds no runtime decoder, resource version, resident
pool, startup scan or preload.

The immutable baseline identifies the old glyph keys and complete old payload.
The converter first proves that the scene-font append preserved that baseline.
New literal glyphs must occupy the entire contiguous suffix; overlapping ranges,
unexpected trailing data and a mismatched baseline are rejected. Existing glyph
groups are neither decompressed and recompressed for storage nor split. Their
compressed bytes, descriptors and directory entries keep the same locations.

Only new glyph payloads may move. Equal payloads can share an existing old
glyph slice or one new slice while retaining each glyph's own key, native size,
geometry, advance and raw/RLE interpretation. Small glyphs remain literal when
the 16-byte grouped descriptor would cost too much. Larger new glyphs are ordered
by their font band and stored in groups of at most32KiB. The converter tries
four bounded size cutoffs and three existing LZMA2 encoder parameter sets, then
uses the smallest complete tail, including headers, descriptors and alignment.
If no candidate is strictly smaller, the original pack is returned unchanged.
An already grouped appended tail is preserved on subsequent runs.

Verification compares every glyph's stored payload and decoded alpha, all old
glyph records, image/string/model/clip catalogs, and every old payload byte.
Only the new glyph offsets/group flags and the overall file length may differ.
The public tests cover old grouped glyphs and grouped strings, raw/RLE alpha,
aliases, incompressible tails, strict append bounds and exact bilingual split
reconstruction. They also decode every synthetic image on both sides.

## Measured candidate and pipeline

The short comparison against the stage-C scene-font packs reports:

| Locale | New glyphs | New literal bytes | New groups | Net bytes saved |
|---|---:|---:|---:|---:|
| English |217 |7,108 |1 |1,046 |
| Chinese |3,331 |164,726 |4 |20,471 |

The combined saving is21,517 bytes. Small glyph payloads and per-glyph
descriptors limit this result; it is not a100KiB saving. The five synthetic
tests pass, and the real candidate preserves all old/new glyph payloads and
alpha. These are short pre-checkpoint results. The final font additions,
committed-source conversion, native rendering and calculator migration checks
must use the final resulting packs before delivery is accepted.

Run the stages in this order, preserving all inputs:

1. Append exact scene fonts to NSP2/35.
2. Compact the new tail with its immutable pre-append baseline.
3. Apply the cached retained-color conversion to NSP2/36.
4. Append the NSP2/37 original room artwork, then split the final locales.

The compactor deliberately accepts the constrained font-append layout. It is
not a general payload garbage collector and does not reorder artwork or old
font/string groups to seek additional savings.
