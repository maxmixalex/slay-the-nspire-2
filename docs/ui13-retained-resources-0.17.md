# UI13 retained presentation: resource integration and acceptance

Reference: **v0.111.0 / 41cef1ea**. Original images, fonts, animation samples,
logical packs and captures remain private. This document describes the
reproducible converters and their completed validation; it contains no original
asset payload.

## Source identities

| Purpose | Verified checkpoint |
| --- | --- |
| Final native / ARM production runtime | `792904cb5e7f5fcbbab7a5320ee0d54d247591c7` |
| Final resource conversion | `e606108221fd6f8202f61b3e5ae1372970de0e27` |
| Corrected source-image and kerning test oracles | `21c3cce61e5870f2018d2e3c231f2b5efa938c87` |

Production `src`, `include`, `platform` and CMake files are byte-identical
between these checkpoints. The last checkpoint also leaves conversion code
unchanged. The resource checkpoint corrects an offline Regent guard: the source
relaxed animation has 32 visible draw slots in 258 samples and an additional
Normal sweat slot in 62 samples. It preserves the unique Add eye slot and the
original order in all 320 samples.

## Complete catalog

`tools/ui13_resources.py` now supports two incremental stages after the earlier
UI13 catalog. Neither stage rebakes existing source images.

| Auxiliary range | Images | Content |
| --- | ---: | --- |
| 2201–3338 | 1,138 | Earlier UI13 fixes, including Architect and Doom; every row and payload unchanged |
| 3339–3342 | 4 | Original Random-selector gradient, noise, light and dot |
| 3343–5010 | 1,668 | Complete Fake Merchant room, actors, both merchant skins and source particle textures |
| 5011–5155 | 145 | Original room bottom with the rug bone hidden: one invariant layer and all 144 moving samples |

The complete catalog ends at **5156** and contains **2,955** appended images.
The Random light/dot textures retain every logical 256×256 source texel. Their
lossless linear storage is 320×205 so the existing native image bounds stay
valid; logical UV sampling still uses 256×256.

The Fake Merchant catalog retains nine complete sequences: the bottom and
foreground, merchant default and outline, and all five characters' normal-atlas
`relaxed_loop`. Its 1,656 dynamic samples use the existing 24 Hz target sampling
and a continuous 60 Hz source physics integration. The no-rug sequence adds all
144 samples, for 1,800 dynamic samples in total. The Necrobinder retains its
active attached head-fire shader. Regent's eye uses an original-order
Before/Add/After region in a 160×196 sparse atlas, keeping one cache image per
sample rather than adding hundreds of image-cache entries.

A failed early conversion completed seven sequences before reaching Regent.
The recovery input is explicit: a private manifest with checkpoint, source
recipe, sequence order, frame count, timing, skin, crop, payload length and SHA.
The converter accepts only those exact seven completed sequences, totaling
1,199 images; it does not relax the normal whole-converter cache identity.
Their final metadata and bytes were checked again, as were all 1,138 earlier
UI13 images.

The complete manifest SHA-256 is
`68e76d45268f2f9e09fae430727332a3c65dce0b47137c5d5346fc86147b147b`.
Completed sampling receipts are `ui13-retained-assets-source-order-e21f0c8e03aa`
and `ui13-retained-no-rug-final-2a974c7ebb21`, both exit zero with unchanged
source inventories. The earlier adapter-capacity, additive-coverage and
fixed-slot-count failures remain in the private evidence; no partial catalog
was released as complete.

## Lossless packing and installation size

The packer retains sparse images, aliases identical decoded frames and groups
neighboring payloads. For the full 2,955-image append, 1,013 exact aliases and
127 groups reduce **12,254,308 padded source bytes to 2,991,398 stored bytes**,
saving **9,262,910 bytes**. The group target is 131,072 bytes. A single complete
large texture occupies a 196,800-byte group, within the existing decoder bounds;
this does not enlarge a runtime buffer.

Relative to the preceding complete 1,138-image UI13 append, the 1,817 additional
images contain 10,306,552 padded source bytes and add only 2,249,148 stored common
bytes. The incremental lossless saving is therefore **8,057,404 bytes**. The
complete no-rug sequence adds just 26,246 common bytes relative to the 5011
catalog. No source samples, resolution, registration or animation clocks were
removed to achieve these figures.

The prior safe prune is retained: 3,006 unused descriptors and 38 unused index
groups were removed, saving 307,420 bytes in the default split installation.
That saving is already included in the baseline and is not counted again.

| Default installation file | Final bytes | Change from prior UI13 |
| --- | ---: | ---: |
| `nSpire2.tns` | 1,653,686 | +15,552 |
| `nSpire2.common.tns` | 70,659,306 | +2,249,148 |
| `nSpire2.eng.tns` | 1,140,675 | +7,576 |
| `nSpire2.zhs.tns` | 5,115,779 | −615 |
| **Total** | **78,569,446** | **+2,271,661** |

The total is **78.569446 MB / 74.929663 MiB**, excluding saves and the other
14 optional locales. It exceeds the rough 65–70 MB target; it is not represented
as meeting that target. The original scene content and complete target frame
sequences were preserved.

| File | SHA-256 |
| --- | --- |
| Runtime | `77a41a60cdd99c2a4d2011e567a69afdb6b61851947e9692624f99612e28be4b` |
| Shared artwork | `134c3bfa37ade7210585f300e55c0a30fba426d0544085f475a012d1fc42649e` |
| English locale | `427ccaafc371c907f70f0c048167c77e53f99b5e4a960089f18dd35198639b49` |
| Simplified Chinese locale | `ccabc40125426a8dd0e1bb49fa7af1fb23a20cddfdbc43b8f365cd94e7713880` |

## Completed native verification

`ui13-retained-final-pack16-61ffb1e8372c` closed successfully for all 16
languages. Every locale reconstructs its logical pack exactly from the common
and language files. Common artwork is byte-identical across languages. All
pre-existing glyph pixels, bearings, advances and localized strings remain
identical to both the previous UI12 and UI13 inputs, apart from the reserved
port-credit slot 8703. The default three resource files copied early for device
validation were rehashed after the complete run and remained unchanged.

`ui13-retained-final-all16-native-c8adc28428d3` passed **38,686 checks** and
**53,072 descriptions**, with no failures or unresolved interpolation. This
covers runtime localized descriptions and source sprite-font energy references.
It is not a claim of visual inspection of every localized screen.

`ui13-retained-resource-metadata-gates-12ba283c9284`, built by
`ui13-retained-resource-metadata-build-d3f5e9d05770`, passed:

| Scope | English | Simplified Chinese |
| --- | ---: | ---: |
| Checks | 8,387,948 | 8,549,992 |
| Appended images fully decoded by memory and range readers | 2,955 | 2,955 |
| Decoded image bytes compared | 18,225,705 | 18,225,705 |
| Complete glyph-directory records | 13,299 | 53,631 |
| Kerning metadata pairs | 7,124 | 7,889 |
| Exact raster-size kerning samples | 56,019 | 56,377 |
| Strings / clips / identical UI frames | 8,704 / 1,783 / 40 | 8,704 / 1,783 / 40 |

The image check is the complete **appended range**, 2,955 of 54,218 images;
it is not presented as a new all-image proof for the unchanged earlier catalog.
Every tested image satisfies the actual native dimension and sparse-decoder
bounds. Glyph records intentionally used for kerning have no drawable pixels:
the corrected oracle evaluates every such record through `glyph_kerning64` at
all of its actual raster sizes, independently deriving the expected adjustment
from the directory. Ordinary glyph payloads are still compared byte for byte.
The earlier mistaken drawable-glyph assumption and its failed receipt remain
separate from the final successful gate.

Final Card/HUD and rainbow lifecycle results are documented in
[ui13-combat-feedback-0.17.md](ui13-combat-feedback-0.17.md) and
[rich-rainbow-0.17.md](rich-rainbow-0.17.md). Fake Merchant's actual room actions,
ordered source pixels and warm-cache behavior have their own scene gates.
These native decoder, text and packing checks do not assert calculator FPS,
natural-play completion, or full desktop GPU equivalence.
