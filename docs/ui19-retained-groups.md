# UI19 exact retained-group consolidation

## Complete measured conversion

The final combined NSP2/44 data occupy **72,120,507 bytes** for English/Chinese
or **90,617,943 bytes** with all sixteen languages. Against the accepted complete
UI18/42 installation, the reductions are **5,841,944 bytes (7.49%)** and
**5,888,965 bytes (6.10%)**, respectively. The final complete conversion is
frozen; its paired-tile stage uses the two retained passes documented here.

Two complete retained-group passes reduce the sixteen-language installation
from **95,780,703 to 94,474,866 bytes**, a further **1,305,837 bytes** after the
RGR3 field-separation stage. The default English/Chinese data shrink by
**1,277,746 bytes**. These are installed file lengths, including the shared
index, all eight concern files and the selected locale files. They are not ZIP
compression ratios or estimates from the initial samples.

The conversion preserves every image, animation frame, RGB565 word, alpha byte,
glyph, string, identifier, clip clock and preparation-cost record. It adds no
runtime codec, resident pool, buffer capacity or resource version: both passes
remain NSP2/43 and use the existing **512 KiB retained-group limit**. The
separate paired-tile stage consumes this final directory and produces NSP2/44.

| Installed data | Accepted UI18 / 42 | RGR3 / 43 | First retained pass | Second retained pass | Final paired tiles / 44 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Shared index and eight concern files | 72,727,684 | 71,991,296 | 71,253,500 | 70,718,398 | 66,893,270 |
| English locale | 778,844 | 779,011 | 778,310 | 777,152 | 775,441 |
| Simplified Chinese locale | 4,455,923 | 4,457,604 | 4,456,388 | 4,454,615 | 4,451,796 |
| Default bilingual data | **77,962,451** | **77,227,911** | **76,488,198** | **75,950,165** | **72,120,507** |
| All sixteen locale files | 23,779,224 | 23,789,407 | 23,779,294 | 23,756,468 | 23,724,673 |
| All sixteen languages plus shared data | **96,506,908** | **95,780,703** | **95,032,794** | **94,474,866** | **90,617,943** |

The table excludes the launcher, saves and distribution text/licenses. Add the
actual final launcher's size to the selected data row for the program/resources
installation. A ZIP or offline HTML has a separate distribution size and must
not be substituted for these on-device bytes. The original all-language
baseline is **96,506,908 bytes**, including all twenty-five resource files.

The final ordinary ARM launcher is **1,724,027 bytes**, measured from the actual
file used by the release package. The default bilingual program/resources
installation is therefore **73,844,534 bytes (70.424 MiB)**; the single Simplified
Chinese installation is **73,069,093 bytes (69.684 MiB)**; the all-language
installation is **92,341,970 bytes (88.064 MiB)**. The default calculator archive's
instructions and licenses occupy another **70,161 bytes**; player state is
additional. The old
default program/resources installation was 79,681,674 bytes, so the complete
default reduction is **5,837,140 bytes** after paying for the new launcher's
4,804-byte growth. A single-language figure is not the bilingual size, and the
bilingual program/resources do not fall below 70 MiB.

| Retained pass | First | Second |
| --- | ---: | ---: |
| Source retained groups | 2,126 | 1,876 |
| Eligible disjoint adjacent pairs | 1,041 | 914 |
| Selected pairs | 250 | 112 |
| Output retained groups | 1,876 | 1,764 |
| Maximum selected combined raw group | 279,040 B | 521,894 B |
| Logical artwork saving | 737,796 B | 535,102 B |
| Actual all-language installation saving | **747,909 B** | **557,928 B** |

Recompressing relocated locale directories changes their size too; this is why
the physical savings differ from the logical artwork savings. The second pass
was the final compression candidate in this round. There is no unbounded
fixed-point search or further unmeasured saving included in the result.

The final paired-tile pass saves **3,856,923 bytes** on top of the second retained
pass's all-language installation. Its earlier standalone candidate had different
locale addressing and a different physical saving; those intermediate numbers
are not added to the retained savings. The last table column measures the
actual combined twenty-five files after all directory rewrites and alignment.
The new 1 MiB bound applies only to tile groups; retained groups remain limited
to 512 KiB, and the existing runtime pools are reused.
The [paired-tile implementation report](ui19-tile-layout-0.17.md) records the
fixed-arena layout, in-place restoration, cancellation/pin fixes and large-group
runtime gates without duplicating those details here.

## Selection, boundaries and exactness

`tools/merge_retained_resources.py` considers physically adjacent retained
groups only when they are wholly within the same continuous NSM1 concern span.
Two noncontiguous ranges owned by the same file are not treated as one span.
The combined decoded bytes must fit 524,288 bytes. Existing RGR1, RGR2 and RGR3
layouts and reversible color transforms compete using complete encoded lengths;
the default acceptance threshold requires at least **1,024 bytes and 3%** saved
for the pair. This avoids adding cold-group work for a few trivial bytes.

Replacing two groups requires more than changing their compressed streams.
The converter relocates image slice descriptors, the second group's raw slice
positions, glyph/string offsets, tile-table and frame-descriptor addresses,
and the shared installation's physical boundaries. New NSM1 boundaries follow
actual byte replacements, including their alignment padding. Every original
concern retains its complete byte range; every joined installation reconstructs
the complete rewritten logical pack exactly.

Each English pass compares all source retained groups and their covered image
slices against the replacement output, totaling **106,620,585 raw bytes**. Each
other locale must share the exact proven artwork and preserve its own complete
catalog, glyphs, strings and clocks. All **1,655 tile/index groups** remain
byte-identical during these retained passes. Content-addressed cached proposals
are still subject to the mandatory complete raw comparison before publication.
The public regressions include a structurally valid but wrong cached payload
that must fail that comparison.

The host converter also applies RGR3's reversible color operation to contiguous
color planes instead of repeatedly invoking it on small sparse spans. The
independent old per-span transformation remains the test oracle for exact
encoded-byte equality. This changes conversion cost, not calculator decoding
or image fidelity, and preserves existing compression-cache identities.

## Executed proof and source receipts

Initial implementation: `36fdb016e881d3c35a5c053386d39b5f59946a8f`.
Final boundary, cached-proof and host transformation checkpoint:
`82cc5d68f0037f01d0df1a8dee2132fb77725d10`.

| Gate | Actual result |
| --- | --- |
| First full conversion | `ui19-retained-merge-all16-b47869eaee65`; all sixteen locales completed, 1,345.07 s, source unchanged |
| Corrected first-pass cache rerun | `ui19-retained-merge-final-all16-646f090f211e`; all sixteen locales and complete raw proof, 68.02 s, source unchanged |
| Second and final retained conversion | `ui19-retained-second-final-all16-7bc2de76ccb6`; all sixteen locales and complete raw proof, 781.80 s, source unchanged |
| First-pass native image oracle | `ui19-merged-all-images-exact-238fbb6af03f`; 58,060 images, 1,167,930,140 decoded bytes, 247,885,340 checks; passed, source unchanged |
| First-pass native locale coverage | `ui19-merged-all16-locales-b6669d4a6681`; 53,072 descriptions, 89,806 checks, zero failures; passed, source unchanged |
| Final combined all-language conversion | `ui19-final-paired-all16-48f351cb20fe`; 149.23 s, all sixteen locales, passed with source unchanged; converter checkpoint `83b08dd025c41fbdbf56821b8dca9fc6eaf6d660` |
| Final combined native image oracle | `ui19-final44-all-images-4c094c092250`; all 58,060 images, 1,167,930,140 decoded bytes, 271,962,257 checks, 20 contiguous ranges, 594.89 s; passed, source unchanged |
| Final combined locale coverage | `ui19-final44-all16-locales-4aa31d0fa0a8`; 53,072 descriptions, 89,786 checks, zero failures; passed, source unchanged |
| Public retained tooling gates | 26 selected merge/plane/filter tests passed after the final boundary and cached-proof changes |

The first native oracle directly compares every byte against the complete old
UI18/42 runtime output through memory, ranged and incremental reads. Contiguous
nonoverlapping ranges cover the entire image catalog; the maximum temporary
spool is 131,090,340 bytes. Its 19,148,518 preparation microsteps are internal
bounded operations, not displayed frames or measured calculator latency.

The second retained output's decoding passed the final combined NSP2/44 complete
native oracle, directly against that same unchanged UI18/42 baseline through
memory, ranged and incremental paths. Its twenty contiguous ranges cover the
complete catalog; the maximum temporary spool is **67,108,736 bytes**, below
64 MiB. The receipt is
`private-ui19-combat/paired/final-native-images.json`, SHA-256
`970cd94d78287a15a1395fbf57bf6033789c87e2bf7b65a009ece3072bec2685`.
It records the native runtime checkpoint
`090023d45ec6211da3b2e5a887cfcc2d1dd68004` and executable hash. This final check
is deliberately not duplicated as another complete intermediate NSP2/43 run.
Paired ordinary Firebird scene/combat timing belongs to the integration record.
Host conversion duration, native byte proof and ARM instruction tests do not
substitute for device timing or establish that the new format is faster.

The last retained receipt is
`private-ui19-storage/retained-merged2/data/retained-merge-receipt.json`, SHA-256
`c18ce144b5ee4ae8e8e25a8389cf787e01d3b46c90b309f5eea5dffe38c98a6c`.
Its English logical hash is
`171a22be131d1893129a7b893edbd0a2418e3d3f960c3e1f25757a5e71ce31b0`;
Simplified Chinese is
`ba04d7236513bf119a21ab1930bc0d29a5172cfbbb29b2ac1ddf6a3a4115e88c`.
The frozen NSM1 index hash is
`869d22e14c08cc2a5734ce78cca4b054ad28bd9b56e9d07cd547981febb27501`.

The final combined resources are
`private-ui19-combat/paired/data`, with `paired-tiles-receipt.json` SHA-256
`082d00ec73f152f8bcf5dcdb838c92208537633e67d66dc0524ae629d8f4b531`.
Its complete twenty-five-file identity is recorded by
`private-ui19-combat/paired/final-resource-manifest.json`, SHA-256
`29764c3278ec36dbb5e23b93249027e0d2980f66497c161ed54ad06092525a53`.

## Reproduction and delivery

Run the same final converter twice, with distinct predecessor/output directories
and its recorded checkpoint. The private work cache can be reused between
passes because proposals are content-addressed and each output is fully proved.

```sh
python3 tools/run_logged.py --checkpoint 82cc5d68f0037f01d0df1a8dee2132fb77725d10 \
  --label retained-groups --timeout 1800 -- \
  python3 tools/merge_retained_resources.py \
    --input "$PREDECESSOR_DATA" --output "$NEXT_DATA" \
    --work "$PRIVATE_WORK" --workers 2
```

The first predecessor is the complete RGR3/43 directory. The second is the first
pass's complete output. The paired-tile converter then reads the second pass's
complete directory. Its compressed tile/index cache does not depend on directory
offsets, so it can reuse the already proved streams while rewriting all final
addresses and concern spans. Every final installed file must come from the same
final combined conversion; do not mix the intermediate directories.

Workspace space was limited during the full native build. After their complete
proofs finished, the RGR3-only and first-retained-pass data and logical pack
copies were removed; per-file cleanup manifests preserve their exact hashes and
sizes. Their receipts, logs and compression caches remain. The last retained
input directory stays frozen for the combined conversion. Its temporary English
and Chinese logical copies were also retired after their hashes and complete
proof were recorded; the converter and preserved split files can recreate them.
No original private input, final delivery, receipt or source checkpoint was
deleted to obtain these installation savings.

`tools/package_release.py` now accepts NSM1 plus its adjacent eight parts for
both calculator and offline-browser delivery. `build_web.py` joins the parts in
host memory, validates the selected NSL1/NSL2/NSL3 locales and embeds the common
bytes once, avoiding a duplicate common file on disk. Synthetic end-to-end
tests create the actual HTML and ZIP, decode their embedded bytes, read all
archive members and reject missing or damaged concern files. All twenty-four
selected package/split/storage/retained-pipeline tests passed for checkpoint
`1dd34eacef09357acbdefcefdda7b52b000dfd24`.

The actual release was then read independently by
`tools/verify_offline_release.py`, checkpoint
`b2d7fffcf9615ea1f387d7ae485f08aed173b0ab`. Gate
`ui19-offline-embedded-readback-daae8f384e87` passed with source unchanged against
the packaging readers at `c45d588f2c9145f04106cb9aeffb33dd7611190e`. Its executed
driver was a byte-identical private copy of the committed tool, SHA-256
`a1f7fc88885e59eb3baa8b93dc4637257eb7986d54a047e884ba473c860afcc8`.
The resulting `private-ui19-storage/offline-embedded-receipt.json` records:

- The actual **106,285,973-byte HTML** contains the exact **7,516,049-byte WASM**
  from runtime checkpoint `090023d45ec6211da3b2e5a887cfcc2d1dd68004`.
- Its embedded **66,891,542-byte NSC1** exactly equals the in-memory join of the
  final NSM1 index and eight parts. The installed shared set is slightly larger
  because its physical map and eight part headers also take space.
- Embedded English and Simplified Chinese locale bytes equal the final files,
  and both reconstruct valid NSP2/44 packs with all 58,060 images, 1,783 clips
  and 8,704 strings. The reconstructed English bytes also equal the exact
  logical candidate used by the final native image oracle.
- The HTML identifies the runtime checkpoint as local and explicitly
  unpublished. It contains no GitHub tree URL claiming that checkpoint was
  published. The calculator README uses the same local-source convention.

The HTML SHA-256 is
`1704d533aa0adbb7cd3e022e16e43eddfe5b71824322a1257d245c2b426c39e3`;
embedded WASM SHA-256 is
`9a8b10c694cff7eeface7742a34e2d30f8b638964f7b666e9b79c6daa273e811`.
This verifies the distributed bytes and their source notices, without claiming
browser interaction or browser performance. Reproduce with
`verify_offline_release.py --html ... --wasm ... --common ... --eng ... --zhs ...
--english-logical ... --runtime-checkpoint FULL_SHA --local-checkpoints
--output RECEIPT.json`; it creates only the small JSON receipt. Its two public
end-to-end tests also reject a mismatched WASM and an unpublished source notice
presented as a published reference.

This round's public source upload was rejected by automatic approval review.
The release tools' `--local-checkpoints` option therefore labels both archive
and HTML source receipts as local, without claiming a published GitHub tree.
The updated bilingual install instructions cover upgrades from both monolithic
and multipart versions: exit the game, remove old data resource files before
transferring the matching full set, replace the launcher and preserve `state`.
The [regeneration workflow](content-regeneration.md) explains which stages can
be reused after an original-game update and which behavior/import changes still
require manual implementation.
