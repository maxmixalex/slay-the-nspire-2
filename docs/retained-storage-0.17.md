# Bounded retained-image storage — NSP2/20

NSP2/20 keeps the NSP2/19 image, clip, card, auxiliary and localized-text
catalogs. It keeps card art, the stable 66-image base and auxiliary UI images
in their fast existing single-image encodings. Other retained pictures use
small compressed groups in addition to the existing tiled animation storage.
It does not reduce image
dimensions, change RGB565 pixels, discard alpha, simplify palettes, omit
frames or change timing. The title restoration may deliberately replace image
4 with an original-derived atlas; that replacement is recorded separately.

The measured hybrid candidate was regenerated after workspace pruning using
the saved v18 baseline, the original owner PCK and converter checkpoint
`b400ab88186ddb618167a8b7c2a8c141d41629e0`. All three split output identities match
the earlier measured candidate exactly. The [recovery receipt](validation-ui3-resource-recovery-0.17.json)
records the four completed conversions. This is a reproduction of those
resources, not a new performance measurement of the later runtime.

The final title-glow extension was generated separately at checkpoint
`3843e8e626317da0c1508fab02a2ed4a627f9fdd`. It keeps image ID 4 and expands that
atlas from 164×106 to 164×210 with one original 104×104 light mask. This adds
15,602 bytes to the shared artwork file and no images or clips. The final
common file is **74,756,230 bytes**, English is **1,780,887 bytes**, and Chinese
is **4,563,586 bytes**: **81,100,703 bytes** combined, saving **12,682,660 bytes**
against v19. Both full packs reconstruct exactly from this final split set.
All other retained pictures, animation payloads, glyphs, strings, catalogs and
timing passed the converter's direct byte comparisons. The [final resource
receipt](validation-ui3-glow-resources-0.17.json) records those completed runs.
The performance table below retains the earlier measured candidate's exact
identity and sizes; this title-only resource change does not relabel those
measurements as a new device or frame-rate test.

## Image format 7

The ordinary 16-byte image-directory row still stores offset, width, height,
format and byte count. Format 7 points to a 16-byte descriptor:

| Byte | Type | Meaning |
| ---: | --- | --- |
| 0 | signed 32-bit | Group offset relative to this descriptor's file offset |
| 4 | unsigned 32-bit | Complete group bytes, including the 32-byte header |
| 8 | unsigned 32-bit | Image byte offset inside the decoded group |
| 12 | unsigned 32-bit | Original format in high byte, decoded byte count in low 24 bits |

Original format is 0..4. These are the existing dense RGB565, dense alpha,
palette, palette/alpha and sparse-row representations. The relative group
offset is intentional: Chinese and English have different directory sizes,
but their complete artwork region remains identical and can be shared.

Each `RGR1` group has a 32-byte `<4s7I` header: tag, decoded bytes, encoded
bytes, LZMA2 property 14, informational image count and three unused words.
Raw LZMA2 uses the existing 512 KiB dictionary and 512 KiB maximum decoded
block. There is no checksum. Functional size/slice bounds and decoder results
are checked when a requested picture is loaded.

The default encoder target is 128 KiB decoded per group. An individual larger
picture may form a group up to 512 KiB. Pictures exceeding that bound retain
their previous encoding. Groups also retain the previous encoding when the
compressed group plus descriptors would take more space. The converter
deduplicates exact original-format/raw-byte pairs and can retain the shorter
existing representation of a duplicate.

The default `--fast-images ui` policy gives card and UI first-use latency
priority over maximum compression. `cards` retains only card artwork and
`none` groups every beneficial retained image; these are explicit comparison
options. The all-grouped candidate's actual eight-card library pages required
2.47 times their own raw picture bytes on average, with 498,072 decoded bytes
for a worst page containing 78,720 bytes of card art. The measured first-visible
CPU regression motivated the hybrid policy; it does not change any picture
pixels or add a runtime branch.

## Memory and I/O

Retained groups reuse the existing five MiB tile-group cache, its 40 pages and
its age/eviction behavior. No second group pool or full-image resident array
is added. Only the requested image slice is copied into the existing image
cache. Neighbors can then reuse the decoded group; unrelated groups remain
unread. Reusing this cache may change eviction patterns and requires actual
scene measurements before claiming calculator acceleration.

The last two formerly unused `TIL1` header words, at offsets 56 and 60, hold
the absolute decoded-cost-table offset and image count. One byte per image
follows the group lookup table, padded to an even byte count. Each byte stores
the number of 4 KiB decoded image pages. The resident directory reads these
costs directly, so preparation can budget sparse images without decoding or
scanning every animation index group during startup. Earlier resource versions
use geometry bounds until their first actual decode.

NSP2's historical checksum word and `TGR1` CRC words are unused and newly
written as zero. Split-container digest/checksum words are also unused. The
runtime still needs actual format, position and size checks to read a selected
resource. Offline exact-byte conversion comparisons are separate from runtime
startup work; no whole-pack integrity pass is introduced.

## Conversion and sharing

Use private directories outside the public source repository:

```sh
python tools/repack_retained.py \
  --eng /private/merchant-eng.nsp --zhs /private/merchant-zhs.nsp \
  --output /private/retained
python tools/split_content.py \
  --eng /private/retained/retained-eng.nsp \
  --zhs /private/retained/retained-zhs.nsp --output /private/split
```

`--logo-atlas /private/title/title-logo.json` replaces fixed image 4 without
adding images or clips. The manifest supplies width, height, original format
and a `raw_file` relative to the manifest. Its intended content is the title
agent's original Spine-derived static lettering, ten small flame frames and
one light-mask band for the original additive glow.

English is planned once. Chinese uses the same groups after direct comparison
of every retained input's original format/raw bytes. The converter compares
each distinct retained image, every glyph/string and all catalog/timing rows.
Animation group compressed payloads are copied and compared directly; their
CRC words alone are cleared. Existing tile descriptors remain byte-identical
and move only to make room for the small cost table. Split conversion requires
identical artwork and exact reconstruction of both full resource streams.

`retained-receipt.json` records actual input/output sizes, grouped entries,
largest group, comparison counts and any deliberate image replacements.
`--group-kib 512` is available for a measured storage/decode tradeoff, not the
default. A smaller file by itself is not evidence of faster cold scene access.

## Actual resource and host comparison

The hybrid converter and split execution at source checkpoint
`b4f7ba06164734028af2c673f38fa918120d1341` both completed. The rebuilt
Clang 18 Release `card_grid_benchmark` from runtime
`b4f7ba06164734028af2c673f38fa918120d1341` compared v19, all-grouped v20 and
hybrid v20 in three interleaved trials, with 180 advancing frames per scene.
The closed journals, actual binary/log identities, byte comparisons and full
measurements are in [validation-retained-storage-0.17.json](validation-retained-storage-0.17.json).

| Resource | Bytes |
| --- | ---: |
| v19 shared artwork + both locales | 93,783,363 |
| Hybrid shared artwork | 74,740,628 |
| Hybrid English metadata/text/glyphs | 1,780,887 |
| Hybrid Chinese metadata/text/glyphs | 4,563,586 |
| **Hybrid bilingual resources** | **81,085,101** |
| **Saved relative to v19** | **12,698,262** |

The hybrid uses 2,448,590 more bytes than the all-grouped candidate. That
tradeoff restores fast first-use access to 1,872 card/UI entries. The remaining
3,567 entries occupy 200 groups, with a largest decoded group of 131,063 bytes.
All 1,184 existing animation groups retain their compressed payloads. Every
distinct retained raw image, all glyphs and strings, and the unchanged catalogs
and timing were compared. The two full hybrid packs reconstruct exactly from
the common file and the corresponding locale. These resource totals exclude
the launcher executable; the 30–40 MB installation target remains unmet.

Median host CPU time for the first visible frame:

| Scene | v19 | All-grouped v20 | Hybrid v20 |
| --- | ---: | ---: | ---: |
| Eight-card library | 2.334 ms | 15.063 ms | 2.022 ms |
| Ten-card removal | 2.617 ms | 13.419 ms | 2.801 ms |
| Ten-card draw choice | 5.280 ms | 37.070 ms | 3.680 ms |

Hybrid first-visible callback reads were 239,361 / 250,121 / 394,785 bytes
respectively. Every hybrid warm animation window made zero resource callbacks.
Library/removal frame signatures match v19; all three hybrid scene signatures
match the all-grouped v20 candidate. Canonical saves remained unchanged and
no save callback was invoked.

Draw-choice preparation still costs more: hybrid's median preparation CPU is
6.281 ms versus v19's 1.240 ms. The v20 exact decoded-cost table prepares more
frames, taking 23 update steps instead of 14 in this fixture, so its animation
phase differs from v19. Preparation plus first-visible time therefore is not
uniformly improved. Warm CPU ranges overlap and other work shared the host;
these runs establish the large all-grouped cold regression and its removal,
not physical calculator FPS or a controlled storage-device cold-start speedup.

The default remains `--fast-images ui`. Device acceptance, executable delivery
size and broader scene first-use behavior are separate follow-up gates.
