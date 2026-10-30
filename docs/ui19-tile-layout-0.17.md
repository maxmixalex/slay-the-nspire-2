# UI19 lossless tile layout and cache recovery

Reference: StS II **v0.111.0 / 41cef1ea**. Application version:
`nSpire 0.1.0 (StS II v0.111.0)`.

The selected resource policy is
`171f30b97e03b0a18e2977bdd0459f31689d1ec6`; the production runtime remains
`090023d45ec6211da3b2e5a887cfcc2d1dd68004`. The earlier all-pair converter at
`83b08dd025c41fbdbf56821b8dca9fc6eaf6d660` established lossless conversion,
but its complete output was rejected after ordinary Firebird exposed cache
thrashing. Its old passing pixel/locale results are historical correctness
results, not final performance acceptance or the delivered resources.

Public source publication was blocked by automatic approval review. These
are local source identities preserved in the source/history archive; no
verified remote push or remote CI is implied. The [retained-group report](ui19-retained-groups.md)
records the other compression stages. The [ordinary device report](ui19-device-storage.md)
keeps the final raw measurements and the earlier rejected candidates.

## Final complete result

The selected NSP2/44 output has one NSM1 index, eight common concern files,
and all sixteen NSL3 locales. It includes RGR3, two retained-group passes,
selective large tile groups, and the measured Merchant cache correction.
All directory, index, locale, split-boundary and alignment costs are included.

| Installed resources | Original UI18 NSP2/42 | Selected UI19 NSP2/44 | Saved bytes |
| --- | ---: | ---: | ---: |
| Index and eight common parts | 72,727,684 | 67,585,022 | 5,142,662 |
| English | 778,844 | 776,922 | 1,922 |
| Simplified Chinese | 4,455,923 | 4,452,546 | 3,377 |
| Default bilingual data | 77,962,451 | **72,814,490** | **5,147,961** |
| All sixteen languages and common data | 96,506,908 | **91,312,346** | **5,194,562** |

These are installed data bytes, excluding executable, state, notices and
documentation; they are not ZIP sizes. The ordinary launcher is separately
1,724,027 bytes. Starting from the second retained pass's 94,474,866 bytes,
selective tile pairing produces 91,003,107 bytes. The final Merchant bounds
and RGR2 policy add 309,239 installed bytes, giving the selected 91,312,346.
The larger 5,888,965-byte saving previously reported for the rejected all-pair
candidate is not the delivered saving.

The final logical English file is **69,923,632 bytes**, SHA-256
`775cc5895dbc59fdf4c79b53d78854c3de9e488e659caeb0e530c2fe3dbc26b4`.
The complete 25-resource manifest has SHA-256
`463582860bf9a354771cbfd2f9ac5044433b943bcdcc019f55d8a7c1efd569ab`.
The tile-plan receipt is
`eab497484ac3e4bbf407d2f5b68c12660d1ffb996d6f9da32d64f088fd518b35`;
the final Merchant receipt is
`8c090f1edefccab14391415ab706ec239acecf071327ed517e661272bb0a97c7`.
The private frozen input for packaging is
`../private-ui19-storage/risk-refined/data`.

## Pair groups only where their working set permits it

The original common 680-record tile groups already approached 512 KiB.
The earlier assumption that they were generally 128 KiB was incorrect.
Sharing a 1 MiB LZMA dictionary between adjacent groups gives substantial
compression, but it also loads tiles that the current frame does not need.
The initial all-pair layout therefore failed a real multi-model scene.

On the same ordinary runtime and controlled Bowlbugs input, original42
measured **27.4491 FPS and 1,455,451 combat-read bytes**, while all-pair44 fell
to **17.3680 FPS and 12,343,303 bytes**. Native App replay confirmed cold
first-cycle group thrashing; later fully cached cycles had zero reads. A
58,060-image correctness pass had not discovered that performance failure.

The selected policy resolves semantic animation names into actual active
frame indices. It protects all four Bowlbug models, all CubexConstruct
skins/tracks, PunchConstruct, and all five playable actors plus Osty,
including actor death/particle tracks. It does not hardcode physical group
IDs or only tune the acceptance seed. All **154 selected tracks** resolve;
59 original adjacent pairs remain separate exact original streams.
Cubex contributes 90 tracks across nine skins, not ninety skins.

| Complete tile conversion | Original | Selected |
| --- | ---: | ---: |
| Direct tile groups | 1,155 | 637 |
| Global slots per group | 680 | 1,360 |
| Addressable tile slots | 784,936 | 865,176 |
| Small groups kept | Original layout | 119, including the final short group |
| Complete frame-index groups | 500 | 500, all remapped |
| Active frame/group occurrences | 79,279 | 71,562 |
| Largest raw direct group | At most 512 KiB | 1,047,200 bytes |
| Largest encoded group including header | — | 190,147 bytes |
| Added picture-arena bytes | — | **0** |

The global slot namespace contains holes after protected short groups.
Every active tile ID and TGI predictor is rebuilt for the new namespace;
per-group item bounds still reject IDs in those holes. A protected group
keeps its exact stream, record count and residency pages. Any union of the
selected tracks/phases therefore has its original tile-group residency.
This does not assert that the original working set always fits the cache.
Unprotected groups continue to use the 1 MiB codec.

The selected pixel streams save 3,449,220 bytes; the complete index streams
cost 742 extra bytes and lookup tables save 4,144. The final output includes
2,916 bytes of tile-stage alignment padding and the locale/split effects.
No frame, pixel, glyph, string, clip or clock is discarded. The converter
compares **516,960,290 original tile-record bytes**, including hidden RGB in
zero-alpha pixels and padding, and checks every descriptor/geometry/index.

### Follow up the observed action-phase risk

A single host followup of the already indicated Construct, Knights and
Turret compositions separated page-pressure estimates from actual reads.
All three had zero initial idle reads after ordinary preparation. Knights
and Turret first-turn reads decreased (256,584 to 216,306 bytes; 151,739 to
143,555), so no protection was added for those models. Decimillipede also
retained comparable ordinary device results; a static page threshold alone
was not treated as a performance regression.

Construct's first enemy turn did reveal another conflict: 621,030 source
bytes became 2,136,623, with four large tile groups read five to seven times
and two index groups nine to ten times. Exact full-group raw decoding grew
from **7,776,501 to 32,503,205 bytes**. Protecting every Cubex and Punch track
returns the final native replay to **613,077 source bytes / 28 calls**;
full-group raw decoding is again **7,776,501 bytes**, exactly the original
value. Preparation returns to 229 updates, and all three 720-frame idle
windows have zero reads. The final ordinary device also executes the real
turn correctly, at 28.8794 FPS with 626,642 combat-read bytes. There was no
original42 device-FPS control for this extra scene, so that absolute final
measurement is not presented as a device speedup.

The final Bowlbugs device sample measures **27.2301 FPS / 1,615,991 bytes**.
Earlier intermediate corrected samples were slightly better; the final
report uses the final sample, not the best run. The severe all-pair thrash is
removed, while no claim of identical timing for every scene or phase is made.

### Merchant is a separate retained-group issue

Modern Merchant clips use retained pictures. Tile actor protection did not
change its read trace. Two retained merges had enlarged several simultaneously
used groups to roughly 150–256 KiB; actual replay reread the same body groups
10–14 times. Exact complete-group raw reads in the closed-room window grew
from 4,391,636 to 14,081,781 bytes.

The final tool `bound_merchant_groups.py` resolves Bottom, Top, Focused and
all five Merchant character tracks. Top/Focused use their same-generation
small pre-plane groups. The remaining bodies keep first-pass boundaries
of at most **128,147 bytes**, and 97 planar groups use the already supported
RGR2 mode2 filter. This removes plane-copy/restore work while retaining the
same one-page allocation class. The conversion changes 65 current groups
into 135 bounded groups and compares all **11,417,876 affected raw bytes**,
every live slice and alias. Unrelated tile/index streams and image formats,
fonts, strings, clips, dimensions and preparation costs remain exact.
Reference groups must match the current generation's catalogs and complete
raw slices; a stale reference or a group crossing a replacement boundary
is rejected. Real NSM1 boundaries are relocated exactly, with a full split
round trip and suffix page-phase padding.

Final native closed-room reads are 884,348 bytes / 142 retained decodes,
versus original42's 1,272,008 / 173. The first Open input again reads exactly
10,155 bytes and reopening reads none. Exact full-group raw reads are
4,574,506 bytes, with only one 91,594-byte actor group read twice; the former
10–14 repeated large-group reads are removed. These are native logical-IO
measurements, not calculator FPS or an ARM CPU benchmark.

The ordinary Merchant result retains a measured tradeoff. Original42 was
28.0458 FPS with a 1,213.257-ms maximum. The final two samples are 27.3321
and 27.4347 FPS, with a pooled 27.3834 FPS and maxima 1,322.144 / 1,345.398 ms.
Both have zero read failures and fewer requested resource bytes. The added
resource-provider time cannot be attributed to a particular physical read
from these aggregate counters; filesystem layout or measurement variation
is a hypothesis, not a proved explanation. The final policy was accepted
with this residual disclosed, rather than claiming universal speedup.

The small alternative record-sort experiment saved only 444 net bytes after
index recompression and was rejected. No further compression candidate was
introduced after the selected policy and final device protocol were frozen.

## Runtime memory and ownership

The global picture arena stays **13,369,344 bytes**. Normal combat keeps its
existing 6.75 MiB decoded-image / 6 MiB tile-group split. For NSP2/44 rooms,
the same arena is divided into 11.25 MiB of images and 1.5 MiB of tile groups;
older resources retain the 11.75 MiB / 1 MiB division. This trades 512 KiB of
decoded room cache for larger direct groups within the existing allocation.

The new 12-page room layout accommodates a 1 MiB raw group, a 64 KiB decoder
workspace, and the small saved dimension/alpha planes without another full
raw buffer. The tile plan copies its geometry and at most 300 frame tile IDs
into the existing workspace before releasing the source index pin. Its
workspace footprint is 65,272 bytes, below 65,536. The in-place inverse
processes the high region backward and the low prefix forward; the boundary
includes the record header's starting byte. Tests cover all 1–1,360 record
counts, overlap boundaries, hidden color, both filters, and malformed input.

The 1 MiB dictionary/property is accepted only for NSP2/44 direct tiles.
Legacy tile layouts, retained groups, indices, glyphs and strings keep their
existing 512 KiB limits. The encoded-input transfer arena is unchanged; the
maximum permitted encoded block including its header remains 617,323 bytes.
Future version 45 is rejected. No resource-wide integrity scan is added to
ordinary startup; offline verification remains developer work.

The final ordinary ARM allocation gate reports **22,298,288 bytes
(21.2653 MiB)**, below both the 24 MiB ceiling and the existing approximately
22.45 MB guard. The ELF sections are text 2,520,080, rodata 665,492,
data 4,092,000, and bss 14,863,032 bytes; their sum is 22,140,604 bytes
(22,140,608 including alignment at the end). Allocation/section accounting
is not a measurement of total process peak memory or peak heap use.
The ordinary ARM journal is `ui19-final-release-arm-633f454a09d8`, passed
with unchanged source. The launcher SHA-256 is
`fd40e3badd58366a1b48f9525daa4ef92709510b0c7d89756b0b87349dfd1d52`.

### Foreground and background index handoff

Independent review identified a possible shared-pin defect. A private
reproduction confirmed the failure: a background tile preparation and a
foreground frame shared one index, the foreground destructor cleared their
single Boolean pin, and a later retained-image reservation evicted the index
while background preparation still used it. The next preparation step failed.

The selected fix cancels the old background preparation only when a
foreground frame actually acquires an already-pinned index, and then pins
that published index for the foreground owner. It does not cancel every
cold image request. The published data survive cancellation; workspace and
unpublished reservations do not. The positive regression combines the same
shared index, two distinct frame descriptors, a 512 KiB retained-image
eviction, early/late preparation, both tile filters, and exact recovery.

The separate suspicion that any full small-room cache would fail an unrelated
foreground retained image was disproved: the existing reservation fallback
already cancels background work when space is actually required. No blanket
cancellation change was made. The original failing reproduction and the
disproved probe remain in the private evidence, separately labeled.

## Final executed verification

The production native suite remains **157/157** at runtime `090023d`, unchanged
by these offline policy changes. The integrated mixed interior-group test
adds **2,607,962 checks**, including global 1,360-slot addressing holes, 512 KiB/1 MiB
interior groups, memory/range/incremental paths, 48/12-page caches, and late
cancel/failure/reopen. Its root checkpoint is
`b0ff44ae33d9af63957e0c09275a374c778d4c7e`, journal
`ui19-mixed-tile-integrated-regression-fc58a842396e`.
The final complete Python suite passes **369/369** at
`ec76d3bb3316ad7345a768ccb8d3b6d8705db0bb`, journal
`ui19-cache-aware-final-python-suite-e61e79f7635d`.
All cited final gates complete with unchanged source.

The final all-sixteen-language native gate passes **89,790 checks /
53,072 descriptions / zero failures**:
`ui19-corrected44-final-all16-locales-1194d77dee25`.

The selected final data pass the complete comparison against immutable
original42: **58,060 images / 1,167,930,140 decoded bytes**, each through
memory, ranged and incremental decoding. Twenty contiguous ranges cover
every image exactly once. The native checks also cover 13,299 English glyph
records, 8,704 strings, 1,783 clips and 40 identical memory/range UI frames.
The successful journal is `ui19-corrected44-final-all-images-a3d6e468f4d9`,
unchanged source, **547.541 seconds**. Maximum reference spool is
**67,108,736 bytes**, below the 67,108,864-byte (64 MiB) cap.

The final native receipt is
`../private-ui19-storage/risk-refined/final-native-images.json`, SHA-256
`e8f056ac65f3e655902b6249e75b3b2ebd93a0d4cb9ffe5e86fb63c453893ed9`.
Its oracle binary has SHA-256
`2d79d6d6dd7e0dacd1566089481ecbc39284262be84e2ed70e961827ba52d227`;
the complete log SHA-256 is
`146c2d3ebaa40430698645d7d69786aeea4308f71ede8fb1644cb9b44cabfab6`.
This is a fresh complete proof of the accepted layout; it does not reuse the
rejected all-pair candidate's earlier pixel result as final coverage.

The actual final player packages pass member-by-member readback
(`ui19-corrected44-player-package-5dc93d601506` and
`ui19-corrected44-player-zip-readback-5b9d1b655566`). The **107,211,281-byte**
HTML is decoded and compared with its frozen runtime, common payload,
English/Chinese locale data and exact reconstructed English file by
`ui19-corrected44-offline-html-readback-b59b99a08d0e`. These checks verify the
real produced files, not a second independently constructed resource blob.
The packaging script and dependent web template are identical to root
checkpoint ec76; the job's separate tool checkpoint is
`7ea65589eda387ebbc91febc2cecfa4414b5d72f`, while embedded runtime and resource
identities remain `090023d` and `171f30b` respectively.

The controlled native App cache replay, raw-read mapping, fixture hashes and
its ordinary-Continue protocol are recorded in
`../private-ui19-combat/native-cache-recovery-summary.json`, SHA-256
`5ca36d5ac89eb39cec54ba3e2478e156df15462da9d1a586fd2829c581668627`.
It deliberately omits host elapsed intervals from the summary. The final
ordinary device evidence, including first/warm flows, screenshots, natural
death, two Merchant samples and installation readback, is documented in
[the device report](ui19-device-storage.md). No physical-handheld FPS,
complete desktop-Godot equivalence or measured total process peak RAM is
implied by these host and emulator checks.

## Rebuilding and updating

Run the original extraction, retained/RGR3 and two retained-merge stages on
private same-generation inputs, retaining the unmerged and first-pass
references needed to bound measured hot groups. Then apply the selected
semantic tile policy and Merchant stage:

```sh
python3 tools/merge_tile_groups.py \
  --input /private/frozen-second-retained/data \
  --output /private/selective-tiles/data \
  --work /private/selective-tiles/work \
  --workers 2
python3 tools/bound_merchant_groups.py \
  --input /private/selective-tiles/data \
  --output /private/final/data \
  --work /private/final/work \
  --small-reference /private/original42/eng.nsp \
  --medium-reference /private/first-retained/eng.nsp
```

The defaults select semantic combat protection and same-page RGR2 Merchant
filtering. `--pairing-policy all` and `--medium-codec reference` are explicit
reproduction controls for rejected/intermediate candidates, not the selected
release policy. Omitting `--locales` requires all sixteen locales; a named
subset must include English. Cached streams are keyed by exact content and
encoder parameters, decoded and compared before reuse. Merchant cache hits
also require the intended mode and exact slice table, not merely equal pixels.

`tools/verify_resource_ranges.py` performs the complete original-to-final
native pixel proof in contiguous non-overlapping ranges with a 64 MiB spool
cap. Locale, ordinary device and actual package/HTML readback gates follow.
The [regeneration guide](content-regeneration.md) explains the complete
workflow. A later game update can reuse it after extraction/schema/model
changes are reconciled; successful regeneration alone cannot prove that new
cards, enemies, effects or save semantics have matching runtime behavior.
