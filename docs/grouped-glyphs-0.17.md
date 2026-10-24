# NSP2/32 lossless grouped glyphs

NSP2/32 adds a storage representation for the existing glyph payload. It does not
add or remove a glyph, resize or rerasterize a font, change the raw/RLE encoding,
or change an image, clip, timing, model registration or localized string.
Versions 3–31 remain reader inputs; `group_glyphs.py` accepts the tiled private
pack layouts from versions 20–32. Raw and original RLE glyphs remain valid in a
version-32 file.

## Directory and block contract

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
resource cache; a warm glyph does not reread or decompress its font group. The
runtime implementation and ARM cache/memory measurement are owned by the
performance stage, not inferred from this offline converter's result.

## Conversion and proof

The converter walks glyphs in their current physical payload order, preserving
the earlier frequency/locality arrangement. It deduplicates exact payload bytes,
including aliases across native sizes or faces. Directory ordering stays strictly
by the original packed key. Different geometry can safely point at the same
payload when the original payload bytes are equal; every entry retains its own
geometry and original encoding.

A group is selected only when compressed bytes, its RGR1 header, alignment and
all unique 16-byte descriptors together occupy less space than the original
unique aligned payloads. An incompressible batch keeps its previous encoding.
The writer copies the complete artwork section byte for byte and rebuilds only
the live glyph and string tail, removing obsolete unreferenced tail copies. It
never edits its input in place. Input/output/receipt paths must be distinct and
outside the public source checkout. Publication follows complete verification
and uses a temporary file with atomic replacement.

Readback compares every original directory key, native size, advance, dimensions,
raw/RLE payload, expanded alpha plane and localized string. It independently
checks artwork bounds, exact artwork bytes, image descriptors, clip timing,
model/auxiliary indices and the untouched TIL1/frame metadata. The receipt
includes source/output hashes, actual file sizes, selected groups, maximum
encoded/decoded sizes and the byte/pixel proof results. The ordinary split-pack
writer recognizes the new descriptor and keeps font groups strictly outside the
common artwork section; reconstruction remains byte-identical.

Example private conversion:

```sh
python3 tools/group_glyphs.py \
  --input /private/compact-art/ui9-zhs.nsp \
  --output /private/grouped-fonts/ui9-zhs.nsp \
  --receipt /private/grouped-fonts/glyph-zhs-receipt.json \
  --group-bytes 32768
```

`tests/test_group_glyphs.py` exercises both group bounds, mixed raw/RLE and native
faces, physical-order preservation, aliases, incompressible fallback,
idempotence, obsolete-tail reclamation, previous formats, bilingual exact split
reconstruction, altered keys/geometry/art/text, malformed descriptors and
headers, stream corruption and trailing bytes. Its independent parser directly
decompresses the RGR1 stream and reconstructs each alpha plane. Eight synthetic
tests pass, alongside the three preexisting split regressions. Production-pack
size reductions, native/ARM runtime behavior and installation fit require the
parent's frozen-tool conversion and runtime acceptance; no size estimate is
reported as a measured result here.
