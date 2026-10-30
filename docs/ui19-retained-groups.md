# UI19 exact retained-group consolidation

## Complete measured conversion

The accepted final NSP2/44 data occupy **72,814,490 bytes** for English/Chinese
or **91,312,346 bytes** with all sixteen languages. Against the accepted complete
UI18/42 installation, the reductions are **5,147,961 bytes (6.603129%)** and
**5,194,562 bytes (5.382580%)**, respectively. These are the actual final files,
after all directory rewrites, alignment and the measured cache-policy repair.

The smaller all-paired candidate was rejected by ordinary Firebird performance
checks. It caused recurring tile reads in the three-Bowlbug scene and expanded
merchant preparation work. The accepted layout keeps 59 semantically selected
tile pairs as their original small groups and bounds the merchant's active
groups. It costs **694,403 bytes** across all sixteen languages relative to the
rejected candidate. No pixel, alpha value, frame or game feature was removed.
See [cache recovery](ui19-cache-recovery.md) for the actual failures, repair and
remaining measured performance limits.

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

| Installed data | Accepted UI18 / 42 | RGR3 / 43 | First retained pass | Second retained pass | Rejected all-paired / 44 | Accepted final / 44 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Shared index and eight concern files | 72,727,684 | 71,991,296 | 71,253,500 | 70,718,398 | 66,893,270 | 67,585,022 |
| English locale | 778,844 | 779,011 | 778,310 | 777,152 | 775,441 | 776,922 |
| Simplified Chinese locale | 4,455,923 | 4,457,604 | 4,456,388 | 4,454,615 | 4,451,796 | 4,452,546 |
| Default bilingual data | **77,962,451** | **77,227,911** | **76,488,198** | **75,950,165** | **72,120,507** | **72,814,490** |
| All sixteen locale files | 23,779,224 | 23,789,407 | 23,779,294 | 23,756,468 | 23,724,673 | 23,727,324 |
| All sixteen languages plus shared data | **96,506,908** | **95,780,703** | **95,032,794** | **94,474,866** | **90,617,943** | **91,312,346** |

The table excludes the launcher, saves and distribution text/licenses. Add the
actual final launcher's size to the selected data row for the program/resources
installation. A ZIP or offline HTML has a separate distribution size and must
not be substituted for these on-device bytes. The original all-language
baseline is **96,506,908 bytes**, including all twenty-five resource files.

The final ordinary ARM launcher is **1,724,027 bytes**, measured from the actual
file used by the release package. The default bilingual program/resources
installation is therefore **74,538,517 bytes (71.085469 MiB)**; Simplified Chinese
alone is **73,761,595 bytes (70.344539 MiB)**; English alone is **70,085,971 bytes
(66.839190 MiB)**; all sixteen languages use **93,036,373 bytes (88.726399 MiB)**.
Instructions, licenses, save growth and filesystem allocation overhead are
additional. The old default program/resources installation was 79,681,674 bytes,
so the complete default reduction is **5,143,157 bytes (6.454630%)**, after paying
for the new launcher's 4,804-byte growth. Single-language figures are not the
bilingual size; the accepted bilingual program/resources do not fall below
70 MiB.

These single-language sizes describe supported optional file combinations.
The production launcher enumerates installed locales and falls back to the
first available locale if saved/system preferences and English are absent;
the existing boot regression covers every single-language mask. Final ordinary
device measurements in this round used English alone. The Simplified Chinese
combination has host/embedded-resource coverage, not a separate final ordinary
device installation sample.

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
was the last general retained-merge pass. The later cache repair changes only
the measured semantic hot sets; no unbounded fixed-point search or unmeasured
saving is included in the accepted result.

The accepted mixed-tile pass saves **3,471,759 bytes** on top of the second
retained pass's all-language installation, producing 91,003,107 bytes. The final
merchant bounds/RGR2 stage adds **309,239 installed bytes**, producing the
91,312,346-byte accepted set. The net saving after the second retained pass is
therefore **3,162,520 bytes**. The rejected all-paired stage's 3,856,923-byte
saving is retained only as a historical measurement, not added to this result.
The last table column measures all twenty-five actual files.
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
| Rejected all-paired conversion | `ui19-final-paired-all16-48f351cb20fe`; 149.23 s, all sixteen locales, byte proof passed with source unchanged; later rejected by ordinary performance gates |
| Rejected all-paired image oracle | `ui19-final44-all-images-4c094c092250`; 58,060 images, 1,167,930,140 decoded bytes, 271,962,257 checks, 20 ranges, 594.89 s; byte proof passed, source unchanged |
| Rejected all-paired locale coverage | `ui19-final44-all16-locales-4aa31d0fa0a8`; 53,072 descriptions, 89,786 checks, zero failures; source unchanged |
| Accepted final mixed tile conversion | `ui19-refined59-final-all16-1bf452858700`; 159.226 s, all sixteen locales, 516,960,290 raw tile bytes compared; passed, source unchanged |
| Accepted final merchant conversion | `ui19-refined-rgr2-final-all16-d51cf6d5e64e`; 100.743 s, all sixteen locales, 11,417,876 replacement raw bytes compared; passed, source unchanged |
| Device-to-final identity guard | `ui19-refined-final-manifest-c43c8bacfb13`; all nine shared files, English locale and logical English pack identical to the tested candidate; passed, source unchanged |
| Accepted final locale coverage | `ui19-corrected44-final-all16-locales-1194d77dee25`; 53,072 descriptions, 89,790 checks, zero failures; source unchanged |
| Accepted final full image oracle | `ui19-corrected44-final-all-images-a3d6e468f4d9`; all 58,060 images / 1,167,930,140 decoded bytes, 20 contiguous ranges, memory/ranged/incremental equality, 547.541 s; passed, source unchanged |
| Public retained tooling gates | 26 selected merge/plane/filter tests passed after the final boundary and cached-proof changes |

The first native oracle directly compares every byte against the complete old
UI18/42 runtime output through memory, ranged and incremental reads. Contiguous
nonoverlapping ranges cover the entire image catalog; the maximum temporary
spool is 131,090,340 bytes. Its 19,148,518 preparation microsteps are internal
bounded operations, not displayed frames or measured calculator latency.

The second retained output's decoding first passed the all-paired NSP2/44 complete
native oracle, directly against that same unchanged UI18/42 baseline through
memory, ranged and incremental paths. Its twenty contiguous ranges cover the
complete catalog; the maximum temporary spool is **67,108,736 bytes**, below
64 MiB. The receipt is
`private-ui19-combat/paired/final-native-images.json`, SHA-256
`970cd94d78287a15a1395fbf57bf6033789c87e2bf7b65a009ece3072bec2685`.
It records the native runtime checkpoint
`090023d45ec6211da3b2e5a887cfcc2d1dd68004` and executable hash. This is preserved
as a superseded candidate's pixel proof. It did not detect the later ordinary
cache regression and is not substituted for the accepted final layout's proof.
The accepted policy's complete final native receipt is recorded in
[cache recovery](ui19-cache-recovery.md).
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

The accepted final resources are `private-ui19-storage/risk-refined/data`.
`merchant-bounds-receipt.json` has SHA-256
`8c090f1edefccab14391415ab706ec239acecf071327ed517e661272bb0a97c7`.
The complete twenty-five-file identity is
`private-ui19-storage/risk-refined/final-resource-manifest.json`, SHA-256
`463582860bf9a354771cbfd2f9ac5044433b943bcdcc019f55d8a7c1efd569ab`.
The final English logical pack is 69,923,632 bytes, SHA-256
`775cc5895dbc59fdf4c79b53d78854c3de9e488e659caeb0e530c2fe3dbc26b4`.

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
pass's complete output. The mixed-tile converter then reads the second pass's
complete directory. Its compressed tile/index cache does not depend on directory
offsets, so it can reuse the already proved streams while rewriting all final
addresses and concern spans. Run `bound_merchant_groups.py` afterward, preserving
the same-generation 42 English pack and first-merged 43 English pack as its
checked references. This final stage reuses the original small Top/Focused
groups and the one-page medium boundaries, with existing RGR2 encoding.
Every installed file must come from the same final conversion; do not mix the
intermediate directories. The exact commands and semantic guards are in
[cache recovery](ui19-cache-recovery.md).

Workspace space was limited during the full native build. After their complete
proofs finished, redundant generated data and logical pack copies were removed;
per-file cleanup manifests preserve their exact hashes and sizes. Receipts, logs
and compression caches remain. The final intermediate mixed-tile files were
retired only after the all-language merchant transform and exact device-English
identity guard passed. The matching-generation 42 and first-merged 43 English
references were retained until the merchant conversion completed. These
temporary workspace deletions are not counted as game installation savings.
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

`tools/verify_offline_release.py` independently decodes the actual HTML's embedded
WASM, common resources and English/Chinese streams, compares each with the final
inputs, reconstructs both logical packs and checks the English pack against the
native oracle's exact input. The release report records the final package sizes,
readback receipt and their hashes. The earlier all-paired HTML receipt remains a
superseded artifact's successful byte audit, not evidence for the accepted pack.

Reproduce with `verify_offline_release.py --html ... --wasm ... --common ...
--eng ... --zhs ... --english-logical ... --runtime-checkpoint FULL_SHA
--local-checkpoints --output RECEIPT.json`. It writes only a small receipt. Its
two public end-to-end tests reject mismatched WASM and an unpublished source
notice presented as a published reference. The complete integrated Python suite
at `ec76d3bb3316ad7345a768ccb8d3b6d8705db0bb` passes **369 tests**, including the
mixed-tile, merchant-boundary and cache-validation regressions. This verifies
bytes and source notices without claiming browser interaction or performance.

This round's public source upload was rejected by automatic approval review.
The release tools' `--local-checkpoints` option therefore labels both archive
and HTML source receipts as local, without claiming a published GitHub tree.
The updated bilingual install instructions cover upgrades from both monolithic
and multipart versions: exit the game, remove old data resource files before
transferring the matching full set, replace the launcher and preserve `state`.
The [regeneration workflow](content-regeneration.md) explains which stages can
be reused after an original-game update and which behavior/import changes still
require manual implementation.
