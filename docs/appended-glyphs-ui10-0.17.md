# Compact newly appended scene fonts

`tools/compact_appended_glyphs.py` applies the existing NSP2/32 grouped-glyph
format to the new literal tail produced by `scene_ui9_fonts.py`. The resulting
pack retains its NSP2/35,36,37 or38 version. It adds no runtime decoder,
resource version, resident pool, startup scan or preload.

The immutable baseline identifies the old glyph keys and complete old payload.
The converter first proves that the scene-font append preserved that baseline.
Versions36–38 must match the baseline version; a35 append can follow34 or35.
Compaction never upgrades a format or downgrades an existing RGR2 resource.
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
The 36–38 cases include actual filtered RGR2 artwork, old grouped glyphs and
old grouped strings before adding and compacting a new literal suffix.

## Measured candidate and pipeline

The short comparison against the stage-C scene-font packs reports:

| Locale | New glyphs | New literal bytes | New groups | Net bytes saved |
|---|---:|---:|---:|---:|
| English |217 |7,108 |1 |1,046 |
| Chinese |3,331 |164,726 |4 |20,471 |

The combined saving is21,517 bytes. Small glyph payloads and per-glyph
descriptors limit this result; it is not a100KiB saving. The six synthetic
tests pass, and the real candidate preserves all old/new glyph payloads and
alpha. These are short pre-checkpoint results. The final font additions,
committed-source conversion, native rendering and calculator migration checks
must use the final resulting packs before delivery is accepted.

The formal stage-D conversion uses checkpoint
`78ac86e35651d3832e1b9638001aa0aa6be3f0a0` and its expanded exact-font list:

| Locale | New glyphs | New literal bytes | New groups | Net bytes saved |
|---|---:|---:|---:|---:|
| English | 219 | 7,128 | 1 | 1,046 |
| Chinese | 3,340 | 165,048 | 4 | 20,452 |
| Combined saving | | | | **21,498** |

These figures are read from `35-grouped/eng.json` and `35-grouped/zhs.json`
in the private D resource output, under the passed journal
`ui10d-resources-legacy-receipt-d763cd064f10.json`. Every old and new glyph's
stored payload, metrics and decoded alpha remains exact. The D output hashes
are `99484c9281c318e9d04ea9927920f689e19e060887aacf44503d66322033e1c6`
and `22dfc84799b1d26bcc5ae06985d62bf31ab5e46579d1a30cb258bbd4e65bd4fe`.
This 21,498-byte locale saving is independent of the 949,484-byte common
artwork saving and can be added once to it. It is an on-disk byte count,
not a runtime memory reduction or CPU timing result.

E appends only three Chinese glyphs to the already verified D37 output and
uses that D37 pack as its immutable baseline. The same-version append and
tail compaction preserve all existing RGR2 artwork, grouped fonts and strings.
The later original history lock image upgrades the resulting pack to38.
Final E font and split receipts, followed by ordinary calculator migration,
remain required; no extra compression benefit is assumed for its tiny tail.

Run the stages in this order, preserving all inputs:

1. Append exact scene fonts, retaining36–38 when those features are present.
2. Compact the new tail with its immutable pre-append baseline.
3. For an older35 pack, apply the cached retained-color conversion to36 and
   append the37 original room artwork.
4. Split the final locales. An existing37/38 baseline keeps its entire artwork
   byte-exact, so a later font-only append needs no artwork reconversion.

The compactor deliberately accepts the constrained font-append layout. It is
not a general payload garbage collector and does not reorder artwork or old
font/string groups to seek additional savings.
