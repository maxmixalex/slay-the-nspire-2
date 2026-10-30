# UI19 cache-aware lossless resource layout

The accepted final policy uses 59 protected tile pairs and bounded merchant RGR2
groups. Its complete all-language installation is **91,312,346 data bytes**;
English/Chinese use **72,814,490 data bytes**. The current 1,724,027-byte launcher
brings the bilingual program/resources to **74,538,517 bytes (71.085469 MiB)**.
It saves **5,143,157 installed program/resource bytes** against the old bilingual
release. Instructions, licenses, saves and filesystem allocation overhead are
additional. Stage accounting is in [retained groups](ui19-retained-groups.md).

The smallest all-paired NSP2/44 candidate passed complete pixel readback but
regressed the ordinary three-Bowlbug scene. Its 17.368 FPS and 12,343,303 bytes of
room reads compared with 27.449 FPS and 1,455,451 bytes for NSP2/42 on the same
ordinary runtime and fixture. It was rejected. Pixel equality and a smaller
archive do not establish acceptable play performance.

## Reusable tile policy

`tile_group_policy.py` resolves original semantic model/animation names from the
reviewed profiles and checks their generated clip identifiers against the
headers. It protects every track of all four Bowlbug models, all playable
combat actors and Osty including their later death/particle tracks, and the
complete Cubex Construct and Punch Construct families. Cubex contributes 90
tracks across nine skins, not 90 skins. Required family/track mismatches fail
instead of silently producing an unprotected pack after an original update.

The current catalog resolves to 154 tracks and 59 adjacent source pairs. These
physical pair numbers are outputs of the source data, not constants in the
policy. A protected pair remains two original groups of at most 512 KiB.
Other adjacent pairs can use at most 1 MiB. There are 637 output tile groups,
including 119 small groups (118 protected groups and the odd tail), and 500
index groups. The maximum actual raw tile group is 1,047,200 bytes.

The global tile slot namespace still uses 1,360 slots per output group. Small
groups leave intentional holes, so the converter increases the slot limit to
865,176 and rewrites every affected TGI2 reference. It validates all valid tile
IDs, rejects references into holes, and compares all 516,960,290 bytes of the
decoded tile records. This consumes no additional runtime arena and uses the
same ordinary ARM build as the original 42/44 comparison.

Any selected-track working set therefore retains its old per-group decoded
residency. This removes the measured growth from roughly 48 to 72–80 pages for
the Bowlbug working sets; it is not a promise that every old working set fits
the 48-page pool. Some hero combinations already required 52 pages before this
round. Other encounter families are not protected merely because a static
upper bound exceeds 48 pages: Decimillipede, Knights and Turret did not exhibit
the corresponding observed regression.

An actual App replay exposed a separate Construct first-turn problem. Its
complete group decode work grew from 7,776,501 to 32,503,205 raw bytes, with some
tile groups read six or seven times and indices nine or ten times. The final
semantic Construct protection restores 7,776,501 raw bytes exactly, with only
the one duplicate read also present in the old pack. First-turn resource reads
are 613,077 bytes versus 621,030 bytes for the old pack. Three complete 720-frame
idle windows have zero reads after preparation. These are host resource-work
measurements, not ARM frame-rate estimates.

## Merchant layout and existing codec

The two retained merge passes enlarged the merchant's simultaneously used
Bottom, Top, Focused and actor groups. The initial inventory preparation draws
Focused while warming Top, so inspecting only the first open-card read misses
the recurring work. A native trace showed 14,081,781 complete raw group bytes
versus the old 4,391,636, with repeated Bottom, Focused and actor groups.

`bound_merchant_groups.py` selects all five merchant actors plus Bottom, Top and
Focused semantically. Top and Focused retain their matching-generation original
RGR2 groups (at most 38,070 raw bytes in this catalog). The other six tracks use
the first retained pass's group boundaries, at most 128,147 bytes, fitting one
cache page each. The 97 medium groups are encoded using existing RGR2 mode 2;
they do not require RGR3 plane restoration. Flames and unrelated retained
groups keep their selected compression.

The complete raw bytes of every replaced group, every image slice and every
alias are compared before publication. Replacements include non-hot slices
sharing the same group. A prior-stage group crossing a replacement boundary,
missing selected track, unexpected codec, mismatched catalog or stale reference
fails. All tile/index streams remain byte-identical, all offsets are relocated,
and padding preserves the unrelated retained suffix's 4 KiB phase. The final
merchant conversion compares 11,417,876 raw bytes and replaces 65 groups with
135 groups, yielding 1,834 retained groups overall.

The existing RGR2 codec adds 92,820 encoded bytes relative to the same bounded
RGR3 groups. Including the original small Top/Focused groups, the payload cost
is 310,200 bytes; alignment adds 1,096 bytes. The actual English installed cost
for this stage is 311,590 bytes after rewriting the locale directory. These
numbers distinguish stream cost, logical pack length and installed resources.

The final native merchant replay reads 884,348 bytes during the initial closed
window versus 1,272,008 for the old pack. Opening reads exactly the old 10,155
bytes; reopening reads zero bytes. The ordinary Firebird runs still have a measured residual, recorded below.
Resource timing wraps the actual file provider, seek and read; it is not a
decoder CPU measurement. Fewer bytes or lower host decode counts are not evidence
of equal ordinary cold-load time.

Mapping the same native trace to complete raw groups gives 4,574,506 decoded
bytes versus 4,391,636 in 42, an increase of 4.16%. One 91,594-byte Ironclad group
is read twice; the former 10–14 repeated reads of several much larger groups are
gone. This residual work is disclosed rather than described as identical cache
behavior. `merchant-final-mapped-trace.json` records the complete mapping.

## Actual ordinary results

All final rows use the same ordinary runtime as the 42 baseline, unchanged
Firebird configured as CX II CAS / OS 6.4.0.74 / 396 MHz / Ndless revision 2022,
and the same scenario inputs. Frames are counted over the recorded scene
interval; these are emulator observations, not physical-device measurements or
universal bounds. The [ordinary acceptance report](ui19-device-storage.md)
records the complete controls, capture identities, timing definitions and
excluded setup attempts.

| Scenario | Original 42 FPS | Accepted final FPS | Original maximum | Accepted final maximum | Accepted scene reads |
| --- | ---: | ---: | ---: | ---: | ---: |
| Bowlbugs and one enemy turn | 27.4491 | 27.2301 | 439.026 ms | 491.577 ms | 59 / 1,615,991 B |
| Decimillipede and one enemy turn | 29.4261 | 29.4265 | 406.433 ms | 405.212 ms | 16 / 189,026 B |
| Construct first charge/burrow | Not sampled | 28.8794 | Not sampled | 395.691 ms | 29 / 626,642 B |
| Merchant first final run | 28.0458 | 27.3321 | 1,213.257 ms | 1,322.144 ms | 161 / 1,245,945 B |
| Merchant same-protocol repeat | Same single baseline | 27.4347 | Same single baseline | 1,345.398 ms | 159 / 1,224,844 B |

Every final run has zero resource-read failures. Bowlbugs' catastrophic repeated
reads are removed, but its final average is 0.80% below the one baseline sample
and its maximum is 52.55 ms higher. The two Merchant samples contain 2,232 frames
over 81.509 seconds: **27.3834 pooled FPS**, **2.36% below** the single 42 sample.
Both samples are retained, including their 1.322/1.345-second cold maxima versus
1.213 seconds originally. This remains an explicit storage/performance tradeoff;
it is not presented as zero regression, statistical significance or timing noise.

Matched warm combat remains **29.9819 FPS** versus 29.9821, with zero reads in
both cases. Character selection averages **21.8398 FPS** versus 21.8565; its
maximum grows from 736.603 to 797.241 ms. Warm preparation total/max changes
from 3,039.246/250.610 to 3,029.968/189.789 ms. The measured startup scope changes
from 753.967 to 680.176 ms, but excludes OS boot and loader work before game main.
No all-scene 30-FPS, physical-handheld or fresh-cold-boot speedup is claimed.

The final device summary is `private-ui19-device/final-pair-summary.json`, SHA-256
`c5ecec5dc1353457c17e08b06e7b47378365dec3268df444ad2ca863bf866961`.
`all16-device-english-identity.json`, SHA-256
`e599e419b9dadf9437088212b5c13bf23031401654895245a98abfc224e29348`,
compares the completed all-language set with the actual device readback. All
nine shared files and English, plus the logical English pack, are unchanged.
The fifteen other language companions have host coverage; they are not claimed
as individually installed or visually reviewed on the emulator.

## Final byte proof and identities

The mixed tile stage and merchant stage ran at the same frozen checkpoint,
`171f30b97e03b0a18e2977bdd0459f31689d1ec6`, with `source_changed=false`.

| Gate | Result |
| --- | --- |
| Mixed tiles, all sixteen locales | `ui19-refined59-final-all16-1bf452858700`; 159.226 s; exact tile records, full index remapping and every locale preserved |
| Merchant groups, all sixteen locales | `ui19-refined-rgr2-final-all16-d51cf6d5e64e`; 100.743 s; exact complete replacement groups, aliases, unrelated compressed streams and locale catalogs |
| Final versus device-English identity | `ui19-refined-final-manifest-c43c8bacfb13`; ten data files and logical pack exactly unchanged |
| Final all-sixteen-locale coverage | `ui19-corrected44-final-all16-locales-1194d77dee25`; 89,790 checks, 53,072 descriptions, zero failures |
| Final native image proof against complete 42 | `ui19-corrected44-final-all-images-a3d6e468f4d9`; all 58,060 images / 1,167,930,140 decoded bytes, 20 contiguous ranges, memory/ranged/incremental equality, 547.541 s; passed, source unchanged |

The final twenty-five-file manifest is
`private-ui19-storage/risk-refined/final-resource-manifest.json`, SHA-256
`463582860bf9a354771cbfd2f9ac5044433b943bcdcc019f55d8a7c1efd569ab`.
The 69,923,632-byte English logical pack has SHA-256
`775cc5895dbc59fdf4c79b53d78854c3de9e488e659caeb0e530c2fe3dbc26b4`.
The mixed-stage receipt has SHA-256
`eab497484ac3e4bbf407d2f5b68c12660d1ffb996d6f9da32d64f088fd518b35`;
the merchant-stage receipt has SHA-256
`8c090f1edefccab14391415ab706ec239acecf071327ed517e661272bb0a97c7`.

The complete native receipt is
`private-ui19-storage/risk-refined/final-native-images.json`, SHA-256
`e8f056ac65f3e655902b6249e75b3b2ebd93a0d4cb9ffe5e86fb63c453893ed9`.
Its maximum reference spool is 67,108,736 bytes, below the 64 MiB budget. It
compares every image against the unchanged complete 42 decoder output through
memory, ranged reads and incremental preparation; it is not a sparse sample.
The final all-language locale gate is separate from this English-artwork oracle.

The actual 107,211,281-byte offline HTML contains the exact 7,516,049-byte WASM,
67,583,294-byte joined NSC1, English and Chinese locale streams. Both languages
reconstruct valid NSP2/44 packs; English equals the exact native/device input.
The physical shared set is 1,728 bytes larger than the joined NSC1 because it
includes multipart indexing and headers. The tool verifies local unpublished
source notices rather than asserting a GitHub publication.
`ui19-corrected44-offline-html-readback-b59b99a08d0e` and the independently
started `ui19-refined-final-offline-embedded-60b837e6d686` both passed and produced
the identical receipt SHA-256
`f9352f8da3d94d9bb8458c21cdee6dab28725fd2f6432f5e4607142950dfc842`.
The duplicate audit is recorded as such, not counted as additional image coverage.
No manual browser playthrough or browser performance claim follows from this
byte audit.

## Source and reproduction

The mixed tile converter was introduced at
`efa13598325294cb90bf415024e79e37e2b1d1a6`; semantic merchant restoration at
`ccf9f5ba0f9249ad6e63d8e3c43ff52ac595c5ab`; the Construct policy and same-boundary
merchant RGR2 choice at `171f30b97e03b0a18e2977bdd0459f31689d1ec6`.
All sixteen targeted public tests pass at the last checkpoint, including mixed
interior tile holes, malformed references, complete bilingual multipart
reconstruction, wrong cached bytes and an unexpected cached RGR3 stream.
The complete integrated Python suite at
`ec76d3bb3316ad7345a768ccb8d3b6d8705db0bb` passes 369 tests in 48.898 seconds,
with source unchanged. The ordinary runtime remains
`090023d45ec6211da3b2e5a887cfcc2d1dd68004`; these repairs change offline layout,
not the production C++ code or runtime allocation.

For a later original-game update, preserve the complete matching-generation
NSP2/42 English logical pack and first-merged NSP2/43 English pack until merchant
restoration finishes. They are references from the same build, not artwork from
a previous game release. The sequence is complete 42 → retained planes 43 → two
retained merge passes → mixed tile 44 → bounded merchant groups → final proof
and package. The CLI defaults select the measured policies; `--pairing-policy
all` and `--medium-codec reference` remain available to reproduce the rejected
or superseded comparisons. No one-command importer for a new original release
is implied by these storage transforms.

The two final conversion commands, after the retained passes, are:

```sh
python3 tools/run_logged.py --checkpoint 171f30b97e03b0a18e2977bdd0459f31689d1ec6 \
  --label mixed-tiles-all16 --timeout 900 -- \
  python3 tools/merge_tile_groups.py \
    --input "$SECOND_MERGED_43_DATA" --output "$MIXED_44_DATA" \
    --work "$TILE_WORK" --workers 2

python3 tools/run_logged.py --checkpoint 171f30b97e03b0a18e2977bdd0459f31689d1ec6 \
  --label merchant-bounds-all16 --timeout 900 -- \
  python3 tools/bound_merchant_groups.py \
    --input "$MIXED_44_DATA" --output "$FINAL_44_DATA" \
    --work "$MERCHANT_WORK" \
    --small-reference "$SAME_BUILD_42_ENG" \
    --medium-reference "$SAME_BUILD_FIRST_MERGED_43_ENG" \
    --medium-codec rgr2
```

Outputs and work caches belong outside the source checkout. The complete final
index, eight concern files and selected language companions must come from that
one final directory. Encoded caches are content-addressed and still validated;
they do not excuse retaining stale physical offsets or mixed locale generations.

The rejected all-pair candidate, intermediate merchant candidate, failed setup
attempts and their complete receipts remain identified in the evidence. Space
recovery removed only verified, reproducible intermediate resource containers;
cleanup manifests record every removed length and hash. Original inputs,
accepted resources, all source checkpoints, receipts and content-addressed
encoding caches remain separate from those temporary files.
