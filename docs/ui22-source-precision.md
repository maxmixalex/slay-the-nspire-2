# UI22: original pixel precision and registered actor sampling

UI22 repairs actor resolution and restores the RGB565 color and eight-bit alpha that earlier space-saving passes removed. The fidelity target is the calculator's native RGB565 framebuffer with source alpha8; it is not the desktop game's RGB888/HDR framebuffer. Storage transforms after that target conversion must be reversible. They may not reduce palette precision, alpha precision, animation sample counts or clocks.

## What caused the Necrobinder regression

The inherited death frames had passed through the historical `444a6` color/alpha mapping. Basic character and Osty actions also included `555a6`, and selection backgrounds included `454a6`. UI21's byte-exact storage checks proved that its codecs did not further change their inputs; they did not undo those older color reductions.

There was a separate density problem. Merchant death actors were displayed using the room's `218/1080` projection even though their input pictures had been baked at combat size. Necrobinder's old merchant death canvas was only 65×88 pixels and was enlarged by about 1.757×. BigMushroom can grow playable characters by 1.5× and Osty can reach 2×. Generic monster checks must also resolve a zero clip `sprite_units` through the model's registration; treating zero as zero missed 176 actions in 20 monster models.

The repair samples the official original Spine data at the largest live display density for the relevant actor and room. It preserves all existing clip identities, sampled frame counts, sample schedules, terminal death samples and registration. This does not equate the port's existing sampling rate with every rendered desktop frame; UI22 performs no additional temporal decimation. It partitions a single solved raster into exact crops when it cannot fit in one bounded picture. No complete-animation union is fitted down and enlarged again at runtime.

The completed actor pass contains **427 actions, 11,051 frames and 11,134 pieces**, with at most five pieces in a frame. It replaces 11,048 existing image slots and appends 513 picture/parameter slots. The resulting catalog has 58,612 images, within the 16-bit image ID limit. The registered lookup adds 7,136 resident directory bytes inside the existing reservation, with no new persistent pixel buffer or image pool.

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

Release acceptance additionally binds the finished pack to those source-stage receipts. `export_source_image_oracle.py` streams independently decoded Python image bytes to disk, keeping at most 32 groups in each decoder LRU. `source_image_oracle_regression` compares every byte, dimension and decoded format through production memory, ranged and incremental-preload readers; it also requires zero source reads after preload reaches Ready. The release guard rejects an old 5,422-entry catalog or an unfinished 7,206-entry actor-only catalog under a UI22 label. It requires the full 427-entry registered coverage, all table/clock/piece bounds, and five selection markers with 540 opaque copy-frame contracts.

## Reproducible private build

Original artwork stays outside the repository. Use paths under a private working directory for the original PCK, recovered complete packs, source caches and generated artifacts. The public tools are:

1. `restore_animation_precision.py` inventories the complete original/current catalogs and produces the proven original-image spool.
2. `actor_animation_ui22.py` creates the registered native-density actor pack and per-action provenance.
3. `restore_derived_precision.py` restores the merchant and prior registered sources while preserving typed Vantom data.
4. `exact_artwork_ui22.py` applies all proven overlays once and writes the same-quality control pack.
5. `finish_artwork_ui22.py` compacts exact tile slots, applies selection geometry deltas, and recovers unused tile/retained storage.
6. `refresh_artwork_resources.py` propagates the complete artwork to all existing locales while preserving their text/font payloads.

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

The independent all-image native comparison is reproducible after the finished logical pack is written:

```sh
python3 tools/export_source_image_oracle.py \
  --input "$PRIVATE/final/eng-complete45.nsp" \
  --output "$PRIVATE/validation/final-images.bin" --cache-groups 32

"$NATIVE_BUILD/source_image_oracle_regression" \
  "$PRIVATE/final/eng-complete45.nsp" "$PRIVATE/validation/final-images.bin"
```

The exporter marks an explicitly selected subset as incomplete. The release check must use the default complete catalog, and its receipts must retain the exact input and oracle hashes. Selection's 540 stored images become copy patches; their source fidelity is established by full-frame recomposition against the same-quality control, rather than by comparing a patch with an older whole frame.

The finish step preserves a compact same-quality control before selection delta encoding. Compare storage and native CPU/I/O against that control to distinguish compression benefits from the deliberate restoration of more source information. Report the separate UI21-to-UI22 installed-size change as well; increased fidelity must not be presented as a compression saving.

Active exact image import and registered-source generation default to RGB565/alpha8. The legacy quantizer CLI requires an explicit migration flag and remains available for historical provenance comparisons. Exact restoration spools are written to temporary files, checked for full length and SHA-256 after flushing, and atomically published before their complete manifest can be written. The importer validates every row's bounds and hash before consuming it.

Native CPU measurements and emulated ARM instruction counts are not calculator frame-rate measurements. This iteration does not have an ordinary-OS calculator/Firebird capture, so it must not claim device FPS.

The existing 320×240 LCD presentation, sampled animation frequencies, and CPU scene/material substitutions remain platform differences. Pixel-oracle equality covers the stated target raster and its assembly; it does not claim identical desktop GPU shader output.
