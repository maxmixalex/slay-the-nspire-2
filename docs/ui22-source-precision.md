# UI22: original pixel precision and registered actor sampling

UI22 repairs actor resolution and restores the RGB565 color and eight-bit alpha that earlier space-saving passes removed. The fidelity target is the calculator's native 16-bit RGB565 framebuffer with source alpha8; it is not the desktop game's RGB888/HDR framebuffer. Storage transforms after that target conversion must be reversible. They may not reduce palette precision, alpha precision, animation sample counts or clocks.

## What caused the Necrobinder regression

The inherited death frames had passed through the historical `444a6` color/alpha mapping. Basic character and Osty actions also included `555a6`, and selection backgrounds included `454a6`. UI21's byte-exact storage checks proved that its codecs did not further change their inputs; they did not undo those older color reductions.

There was a separate density problem. Merchant death actors were displayed using the room's `218/1080` projection even though their input pictures had been baked at combat size. Necrobinder's old merchant death canvas was only 65×88 pixels and was enlarged by about 1.757×. BigMushroom can grow playable characters by 1.5× and Osty can reach 2×. Generic monster checks must also resolve a zero clip `sprite_units` through the model's registration; treating zero as zero missed 176 actions in 20 monster models.

The repair samples the official original Spine data at the largest live display density for the relevant actor and room. It preserves all existing clip identities, sampled frame counts, sample schedules, terminal death samples and registration. This does not equate the port's existing sampling rate with every rendered desktop frame; UI22 performs no additional temporal decimation. It partitions a single solved raster into exact crops when it cannot fit in one bounded picture. No complete-animation union is fitted down and enlarged again at runtime.

The completed actor pass contains **427 actions, 11,051 frames and 11,134 pieces**, with at most five pieces in a frame. These are the actions requiring a density repair, not the total set of game actions. Existing actions with sufficient source density retain their geometry and recover their original color/alpha values. In particular, Crusher and Rocket use complete 320×218 scene rasters at 1:1 in combat and at a smaller projection in the bestiary; their source scenes are not downsampled.

The actor pass replaces 11,048 existing image slots and appends 513 picture/parameter slots. Its source catalog has 58,612 images, within the 16-bit image ID limit. The registered lookup adds 7,136 resident directory bytes inside the existing reservation, with no new persistent pixel buffer or image pool for the actor registration itself. The separate NSP2/46 geometry decoder described below adds one bounded 230,400-byte reconstruction buffer.

## Every inherited image has a source rule

The complete UI21 English input has 58,099 images and SHA-256 `36e7c560558617053081bc4c731d838a3916801f6f9ce3cc8747e1c77dc94e40`. Its recovered pre-quantization reference has 51,216 images and SHA-256 `670a863c34b472e8f7392deaa3cb66b12bca99f906ee4af55f35245b20441580`.

An old image may be restored only when its entire owning clip tuple and geometry are unchanged and applying the historical color/alpha map to the original source exactly reproduces every current pixel. A size match alone is insufficient. The complete audit found 41,036 eligible old images; some are superseded by the new actor sources. The final priority categories below are mutually exclusive.

| Source rule | Existing image slots |
| --- | ---: |
| Restore full pre-quantization precision with exact registration proof | 31,208 |
| Replace with native-density original actor raster | 11,048 |
| Preserve live original Vantom mesh/texture data | 7,669 |
| Preserve later original-source auxiliary assets | 3,258 |
| Already exact against pre-quantization input | 2,905 |
| Repartition complete original merchant frames | 1,827 |
| Fresh original Necrobinder selection scene | 97 |
| Fresh independently checked changed static source | 61 |
| Restore prior registered overflow/corpse source pieces | 26 |
| **Total classified; unclassified** | **58,099; 0** |

This table records the complete source-restoration stage. A later conservative reachability proof retires exactly 1,598 inherited images: the two obsolete merchant fallback clips and 72 Kaiser clips whose native action names have no runtime entry. All 1,598 came from the 31,208 restored category, leaving 29,610 active restored images from that category. Their dimensions and numeric IDs remain, with empty sparse placeholders. They are an explicit retirement exception, not an assertion that empty pixels equal the original artwork. The retirement leaves all 57,014 other images byte-exact and adds one checked profile-marker image, producing 58,613 final catalog entries. It preserves live Kaiser condition variants, `body/die`, the 18 bestiary supplements, current merchant planes, all actor registrations, and the dedicated Vantom path.

The merchant category contains 1,817 moving frames and ten fixed layers. Both layers are recomputed together from complete original frames, with disjoint alpha and exact recomposition. Restoring only the moving plane would leave historical quantization in the fixed plane or duplicate it.

The 61 changed static images are checked against their current source generators, including the newer title-scene composition and the full-viewport Ancient scenes. Later auxiliary resources use their independently added original-source generator lineage; they are not silently rolled back to an older image having the same numeric ID.

Vantom requires explicit typed handling. UI15 reused 4,080 slots formerly owned by 232 whole-frame clips for live rigid layers, texture pages and mesh words. Those numbers are no longer whole animation pictures. UI22 preserves the dedicated Vantom renderer and keeps all 232 generic registered entries empty. The typed live graph includes 116 parameters, 2,040 composite frames, 21,151 layers and 7,669 referenced images. No old Vantom whole-frame raster is written over it.

## One sampling grid for the complete actor

Independent scaling of each cropped piece can change its nearest-sample phase by a pixel at non-integer display sizes. Registered pieces now use one inverse sampling grid anchored at the actor origin. The same pixel-center rule determines both the piece's visible bounds and its source coordinates, including negative source offsets. A different crop or partition therefore cannot introduce a seam or alter the sampled pixel.

The renderer resolves dimensions from resident metadata and culls before decoding a picture. It computes quotient/remainder steps outside the pixel loop; the loop does not divide for every sample. Native 1:1 pieces retain the existing direct picture path. The Doom grayscale body renderer uses the same registered grid, and additive death particles and death-screen opacity share the normal rendering rules.

The Canvas oracle uses complete, unpartitioned source rasters. It tests normal combat, room `218/1080`, rest-site `0.760006 × 218/1080`, BigMushroom, Osty, duplicate-enemy scales and the exact death-room integer operation order. It also checks additive particles, partial opacity, grayscale bodies and zero source reads for fully off-screen pieces. The source oracle independently resamples the pinned official skeletons and requires zero RGB565 and zero alpha8 error.

## Source-stage validation completed

The complete native-density actor output, before global restoration and orphan reclamation, has SHA-256 `cc0156279967083ac3fc86cc143bae08aab4aa40e1938d3bd800cfb74a3d32be`. It is an intermediate source-stage pack; its retained obsolete tile payload is not an installed-size result.

- Every one of the 11,051 frames was freshly resampled from the official original skeletons and compared with the actual registered image bytes: **zero RGB565 error and zero alpha8 error**, across 11,134 pieces. The reference spool contains 335,168,036 bytes of complete unpartitioned source frames.
- The frozen production Canvas gate passed **540,415 checks**, with 61,816 frame/projection views. These include 50,682 non-native projection views and 2,326 grayscale views. It compared **5,106,124,800 LCD pixel values** exactly. Additive particles, partial opacity and the precise death-room integer operation order were included; invisible pieces caused no source reads.
- The 61 modified static source images matched their current original-source generation paths exactly. The complete inherited-image coverage receipt classifies all 58,099 baseline images with no unknown remainder.
- The atomic merchant source spool contains 1,827 records and 12,788,285 bytes, SHA-256 `0b165cadc915506577bee29842f79167e00017055b3f4e90eab9b0acc27528cd`. A truncated intermediate was rejected before import. Reconstruction and the published atomic-writer pass both reproduced every original record and the full expected hash.

These results are recorded by `ui22-full-actor-original-source-18323f29189a`, `ui22-full-actor-grid-original-source-0e95af0bdebf`, `ui22-changed-static-source-full-proof-6f7e62642824`, and `ui22-merchant-atomic-source-spool-0bde8d0505d9`. The Canvas source checkpoint is `1f6299fe91024679c4a9f13377ca92328300389d`.

Release acceptance additionally binds the finished pack to those source-stage receipts. `export_source_image_oracle.py` streams independently decoded Python image bytes to disk, keeping at most 32 groups in each decoder LRU. The complete pre-implicit reference was exported using the older `0b5e969dff43cee998ab081dfb7af889eaf2bf18` reader, which has no NSP2/46 implementation. Its source is the retired, retained-compacted NSP2/45 pack with SHA-256 `0fcbe7075a90e4a0cc230f8d1388ae1b5d6b5f98863d43e9175a28a1a833403a`. It contains 58,613 images and 1,065,442,263 decoded image bytes; the 1,066,614,551-byte oracle has SHA-256 `cc01a2b14741528bd4fc92fc7dd512bee900904eea48334f742e2211bb63224d`. Journal `ui22-preimplicit-full-independent-image-oracle-6473e90685f2` completed without source changes.

`bind_source_image_oracle.py` verifies the old pack, complete export receipt, spool hashes, row identities and unchanged target image shapes. It copies every row and image byte unchanged, adjusts only the native test's target header, and records the old and new complete pack and spool hashes. Thus a new decoder cannot redefine the reference pixels. `source_image_oracle_regression` compares original bytes and dimensions through production memory, ranged and incremental-preload readers; it also requires zero source reads after preload reaches Ready.

There is one explicit representation rule: the existing image cache removes an alpha plane only when every value is 255, changing format 1 to 0 or format 3 to 2. The old `0b5e969` core already does this. The oracle therefore permits only those two substitutions, compares every color/palette/index byte, and requires every original alpha byte to be 255. It logically reconstructs that implicit plane for the equality claim. Other format substitutions, non-255 alpha, changed prefix bytes and changed lengths fail. The original reference spool is never rewritten to accept a changed pixel.

The full production readback passed for the **final 94,924,498-byte NSP2/46 English pack**, SHA-256 `e2fc2beb4fe1e4d9ff5e54188f0c134bc9da2ee680ef2acc06ad5f7cd6784147`:

- All **58,613 images through all three readers**, with **1,065,442,263 original decoded bytes per reader**, matched the independent old-format reference.
- **200,421,907 checks** passed, including **28,655,100 incremental preload steps**, a maximum of 24,172 steps for one image, and **zero source reads after Ready**.
- Exactly **33 images per reader** used the strictly checked, entirely opaque alpha-plane normalization. Their original pixel and alpha values remain exact.
- The runtime source was `b9425530b24b0d851b57892872fc07865859cea1`, with native core SHA-256 `00be10d70ca66b344781dd4c28d440ad56b4c30f1899c5a606df106978a03dbd`. The test-only opacity correction was `dc175a9ca060478298e08abdaa9f35a22ca65a33`.
- The final bound oracle SHA-256 is `f50eefb2fbaf901bf0e39822277e3935ac48969ed72d706766b863e8f5937c59`; its unchanged row/pixel body SHA-256 is `1b6847d4c20b638f981ba12485d40e6870a3810c33db63e218fe1daea2c5c9b2`. Binding journal `ui22-final46-independent-source-binding-c8a01d3a79c9` and complete native readback journal `ui22-final46-complete-original-source-proof-f849e4193964` both passed without source changes.

This is the complete source-byte gate for the final artifact, after ordinary implicit geometry and removal of all 988 index records without image-descriptor consumers. The latter changes 21 groups and remaps only physical record offsets; every live record, image shape, pixel stream and clock stays exact. The final pack contains no unapproved selection-bitmap encoding. Byte fidelity alone did not approve that experiment for release.

The production-renderer source test passed before the last storage-only changes, on the `b05ecd5529fb0b05fec0e47026f48409f5b8892b8e903a40db43a0dcf6353af2` candidate and the same `b942553` core: **847,623 checks**, all **427 registered clips / 11,051 frames**, **61,816 views**, and **5,106,124,800 LCD pixel comparisons**. The views include 50,682 non-native projections and 2,326 grayscale projections, with 62,205 visible piece visits; off-screen pieces caused zero source reads. Journal `ui22-final-implicit-actor-source-canvas-a54fafbf5224` binds these results to the full original-source raster spool. That earlier candidate is not the release artifact. The unchanged renderer, exact live-record transformation receipts and final complete original-byte readback above establish the same actor pixels for the final `e2fc2beb` pack. The Canvas receipt remains bound to the earlier file; final-artifact acceptance combines it with these complete byte-preservation proofs.

The release guard rejects formats older than NSP2/46, an old 5,422-entry catalog, or an unfinished 7,206-entry actor-only catalog under a UI22 label. It requires the full 427-entry registered coverage, all table/clock/piece bounds, five selection markers with 540 opaque copy-frame contracts, and the precise guarded retirement marker. It also rejects the unapproved selection-bitmap marker while examining the existing selection records.

The final locale refresh reconstructs the shared artwork identically for all 16 locales and preserves their existing compressed locale tails byte for byte. Its completed receipt checks **344,590 glyph entries and 139,264 strings** across those locale editions; this is a sum of locale entries, not a count of distinct characters. English retains 13,299 glyphs and Simplified Chinese 53,632, each with 8,704 strings. The final Simplified Chinese logical pack has SHA-256 `e1add5f33fbbf8f2093fcf3676200518a0550808ad575bf6e9b0071b3ce7b3d6`; its shared artwork is the same as the final English pack. No charset reduction is used to obtain the storage result.

## Exact geometry storage

NSP2/46 can replace redundant sparse layout inside the original index-record area. The ordinary `0xFFFD` marker stores the exact active-tile bitmap and zero padding. The decoder reads the same pixel groups, reconstructs a bounded RGB565/alpha8 plane, and regenerates the original minimum-cost opaque/alpha segmentation. The encoder enables this representation only when every original geometry byte matches. Noncanonical spans, including explicit alpha-zero payloads that cannot be regenerated from coverage, stay literal.

The ordinary conversion preserves frame-record offsets and lengths, tile IDs, index-group raw sizes, decoded sparse lengths, cache-page costs, clocks and all pixel streams. The original compressed group remains the fallback when conversion does not reduce its stored size. The version boundary makes an older launcher reject the file before attempting to decode a new marker. Size eligibility alone is not a performance result: the final cold/warm and incremental-preload gates measure the reconstruction work too.

The separate `0xFFFC` selection-bitmap experiment also reconstructed the original copy spans exactly, but saved only **163,840 bytes** while bounded same-core host measurements reported **2.86–4.43% longer whole-frame time** in the affected Defect/Necrobinder first traversal and rapid switching. UI22 does not adopt it. `selection_bitmap_ui22.py` refuses conversion by default, requires `--experimental-candidate-only` for a private experiment, and marks every such receipt `not_release_approved`. The normal release chain below retains the original selection span representation.

## Reproducible private build

Original artwork stays outside the repository. Use paths under a private working directory for the original PCK, recovered complete packs, source caches and generated artifacts. The public tools are:

1. `restore_animation_precision.py` inventories the complete original/current catalogs and produces the proven original-image spool.
2. `actor_animation_ui22.py` creates the registered native-density actor pack and per-action provenance.
3. `restore_derived_precision.py` restores the merchant and prior registered sources while preserving typed Vantom data.
4. `exact_artwork_ui22.py` applies all proven overlays once and writes the same-quality control pack.
5. `finish_artwork_ui22.py` compacts exact tile slots, applies selection geometry deltas, and recovers unused tile/retained storage.
6. `select_index_streams_ui22.py` and `select_pixel_streams_ui22.py` select only smaller existing streams with their input-bound complete ARM gates; `retire_legacy_resources_ui22.py` applies the proved dead-resource closure.
7. `implicit_tile_geometry_ui22.py` writes the accepted ordinary NSP2/46 geometry; its runtime performance and independent byte gates are required. `prune_orphan_index_records_ui22.py` may then remove only records with no image-descriptor consumer, accepting each changed group only behind its complete linked ARM gate.
8. `refresh_artwork_resources.py` propagates the complete artwork to all existing locales while preserving their text/font payloads.

For example, after source restoration and actor sampling:

```sh
python3 tools/exact_artwork_ui22.py \
  --input "$PRIVATE/artwork/eng-native45.nsp" \
  --actor-receipt "$PRIVATE/artwork/actors/actor-animation-ui22-receipt.json" \
  --restoration "$PRIVATE/precision/precision-restoration.json" \
  --replacements "$PRIVATE/derived-precision-merchant-verified/merchant-original-color.json" \
  --replacements "$PRIVATE/derived-precision/previous-registered-original-color.json" \
  --necrobinder-selection "$PRIVATE/artwork/selection-necrobinder-exact/character-ui7-assets-necrobinder.json" \
  --work "$PRIVATE/work/exact-global" \
  --output "$PRIVATE/work/eng-quality-control7206.nsp" --workers 2

python3 tools/finish_artwork_ui22.py \
  --input "$PRIVATE/work/eng-quality-control7206.nsp" \
  --output "$PRIVATE/work/eng-final-artwork7211.nsp" \
  --work "$PRIVATE/work/finish" --workers 2
```

Export the independent old-format reference using an immutable `0b5e969dff43cee998ab081dfb7af889eaf2bf18` checkout (`$OLD_SOURCE`), before decoding NSP2/46. Once the final logical pack is written, bind that existing reference and run the new native reader:

```sh
python3 "$OLD_SOURCE/tools/export_source_image_oracle.py" \
  --input "$PRIVATE/work/eng-retired-retained7212.nsp" \
  --output "$PRIVATE/validation/source-images-preimplicit45.bin" --cache-groups 32

python3 tools/bind_source_image_oracle.py \
  --reference-pack "$PRIVATE/work/eng-retired-retained7212.nsp" \
  --target-pack "$PRIVATE/release-final/logical/eng-complete46.nsp" \
  --oracle "$PRIVATE/validation/source-images-preimplicit45.bin" \
  --output "$PRIVATE/validation/final-images.bin"

"$NATIVE_BUILD/source_image_oracle_regression" \
  "$PRIVATE/release-final/logical/eng-complete46.nsp" "$PRIVATE/validation/final-images.bin"
```

The exporter marks an explicitly selected subset as incomplete. The release check must use the default complete catalog, and its receipts must retain the exact input and oracle hashes. Selection's 540 stored images become copy patches; their source fidelity is established by full-frame recomposition against the same-quality control, rather than by comparing a patch with an older whole frame.

The finish step preserves a compact same-quality control before selection delta encoding. Compare storage and native CPU/I/O against that control to distinguish compression benefits from the deliberate restoration of more source information. Report the separate UI21-to-UI22 installed-size change as well; increased fidelity must not be presented as a compression saving.

Active exact image import and registered-source generation default to RGB565/alpha8. The legacy quantizer CLI requires an explicit migration flag and remains available for historical provenance comparisons. Exact restoration spools are written to temporary files, checked for full length and SHA-256 after flushing, and atomically published before their complete manifest can be written. The importer validates every row's bounds and hash before consuming it.

Native CPU measurements and emulated ARM instruction counts are not calculator frame-rate measurements. This iteration does not have an ordinary-OS calculator/Firebird capture, so it must not claim device FPS.

The existing 320×240 LCD presentation, sampled animation frequencies, and CPU scene/material substitutions remain platform differences. Pixel-oracle equality covers the stated target raster and its assembly; it does not claim identical desktop GPU shader output.
