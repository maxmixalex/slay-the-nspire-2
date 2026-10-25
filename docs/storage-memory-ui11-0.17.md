# UI11 storage and calculator memory

The UI11 target is a readable, playable 320 × 240 port with an approximately
65–70 MB installation. The original UI10 English/Chinese installation used
91,604,965 bytes, including its 1,490,738-byte launcher. Bytes in this document
are decimal unless explicitly marked MiB. Private original-derived resources
are not included in this repository.

## Existing-format storage changes

`tools/retile_appended_resources.py` extends the existing global 16 × 16 tile
catalog to the animations appended after the original dicing conversion. It
does not introduce another runtime format or decoder. Original tile IDs remain
stable; appended frames reuse original tiles before allocating additional
records. The output remains NSP2/39 with TGR1/TGR2 and RGR1/RGR2 blocks.

The complete lossless conversion selected 6,104 appended image entries,
representing 5,843 distinct decoded frames. It added 220,854 tiles to the
497,344-tile catalog. The English logical pack decreased from 87,022,123 to
85,689,797 bytes; Chinese decreased from 90,331,788 to 88,999,435 bytes.
Its common artwork occupied 83,405,554 bytes, before the 64-byte common-file
header. This improvement alone does not meet the installation target.

The lossless stage checks the 5,843 distinct appended frames byte for byte,
preserves the existing 42,189 tiled image entries, and compares the remaining
2,923 retained/static entries. Its initial receipt called all 45,112 unselected
entries “retained”; the later source corrects that reporting label without
changing output bytes. All 51,216 image IDs, 1,783 clips, 48,293 animation frame
references, dimensions, clocks, pivots, strings and glyphs are retained.

## Bounded animation color precision

`tools/quantize_animation_resources.py` is an explicitly lossy platform
adaptation. It keeps every frame, the original frame dimensions and timing,
and the existing sparse geometry. It changes only animation color/opacity
precision and the five disjoint static planes that form the character-selection
backgrounds. Static UI, card artwork, icons, glyphs and text remain exact.

The mixed policy gives the first 38 base character/actor clips RGB555 with
6-bit alpha, character-selection backgrounds RGB454 with 6-bit alpha, and
the remaining tiled animations RGB444 with 6-bit alpha. The result is expanded
back to ordinary RGB565 and 8-bit alpha before offline compression. No runtime
quantization, new codec or additional buffer is needed.

| Policy | Maximum red / green / blue error in RGB565 channel codes | Maximum alpha error |
| --- | --- | ---: |
| RGB555 + alpha6 | 0 / 1 / 0 | 2 / 255 |
| RGB454 + alpha6 | 1 / 1 / 1 | 2 / 255 |
| RGB444 + alpha6 | 1 / 2 / 1 | 2 / 255 |

The color errors correspond to at most approximately 9 / 255 after expanding
the calculator's channel codes to eight-bit display values. Zero and full
opacity remain zero and full opacity. A single deterministic nearest-level
mapping applies to every pixel of a policy; there is no frame-dependent dither.
This can produce mild color banding in animation artwork, so the conversion
must not be described as lossless.

The actual retiled pack has no tile shared by an animation and a static UI,
card or auxiliary resource. The converter nevertheless follows those references
and preserves any such future static use. It also rejects shared visible tiles
whose requested precision policies disagree. There are 43 tiles shared across
the current mixed policies; their visible colors map identically. Character
selection's static and moving planes use the same mapping, preventing a color
step along their disjoint boundaries.

The build verifies every quantized tile group against its planned decoded
bytes, all compressed frame-index streams unchanged, all image descriptors and
cache costs unchanged, all 2,918 other static images exact, all five character
planes consistently transformed, and every original glyph/string exact.
English and Chinese artwork must still be byte-identical for the shared file.
The localization tool subsequently replaces glyph/text editions without
duplicating common artwork.

The full mixed conversion completed and its readback passed at local source
checkpoint `47a50b1b327b434aeec86d4210bc8801cbdc75d6`. It reduces each logical
edition by another **22,219,450 bytes** after lossless retiling:

| Production component | Bytes |
| --- | ---: |
| Common artwork payload | 61,186,104 |
| Common file, including its 64-byte header | 61,186,168 |
| English logical pack before UI11 text/font rebase | 63,470,347 |
| Chinese logical pack before UI11 text/font rebase | 66,779,985 |

The common file is 23,648,624 bytes smaller than UI10. This includes
1,429,174 bytes from lossless retiling/common split relocation and 22,219,450
bytes from bounded precision. The final rebased English and Chinese files
occupy 1,049,523 and 5,034,485 bytes, respectively. Together with F's
1,475,803-byte launcher, those three resource files would occupy 68,745,979
bytes. Final packaging records the actual final launcher size; the resource
files are already complete and shared by all sixteen language editions.

The passed conversion journal is
`ui11-animation-precision-resume-b9d1bdbceaaf.json`; its receipt verifies all
1,057 tile groups and 544 unchanged index groups, both complete language
editions, and the exact/preserved content described above. The first logged
attempt reached its 600-second command timeout and remained a failure;
the resumed run reused only checked, content-addressed compressed groups
and completed in 504.12 seconds under the longer explicit timeout.

## Memory budget follows the ordinary loader

The maximum permitted Zehn allocation is now 24 MiB. Zehn relocation *locations*
remain 24-bit fields and are checked independently; an ordinary 32-bit pointer
stored there may address a larger BSS. Header, relocation table, flags, loaded
image and BSS are all included in the build's allocation accounting.

The initial trial used a 1,772-page image cache (7,258,112 bytes), an 8 MiB
decoded-group pool, and 480 image keys. The D launcher occupied 23,874,780
bytes when loaded, below the numerical 24 MiB ceiling. **It did not start
through the ordinary calculator OS loader.** Firebird serial output reported
`[Zehn] Memory allocation failed!`; the OS then displayed its unsupported
document-format dialog. This is a failed compatibility trial, not a working
24 MiB runtime.

The official Ndless loader reads the 32-bit allocation field, subtracts the
prefix and calls `execmem_alloc` for one continuous image/BSS block. That
function normally calls `malloc`. The loader also temporarily owns relocation
metadata and a compressed input buffer. There is no general 16 MiB Zehn
allocation restriction in these sources, but successful allocation depends on
the calculator OS heap and its largest available continuous block:

- [Ndless Zehn loader](https://github.com/ndless-nspire/Ndless/blob/457a22f5cf286b134610e102da21fd0d646a1d1c/ndless/src/resources/zehn_loader.cpp)
- [Ndless executable allocator](https://github.com/ndless-nspire/Ndless/blob/457a22f5cf286b134610e102da21fd0d646a1d1c/ndless/src/resources/emu.c)
- [Zehn format fields](https://github.com/ndless-nspire/Ndless/blob/457a22f5cf286b134610e102da21fd0d646a1d1c/ndless-sdk/tools/genzehn/zehn.h)

F therefore keeps the 24 MiB upper bound but reduces the actual fixed pools:

| Cache | UI10 | UI11 F |
| --- | ---: | ---: |
| Decoded image pages, 4 KiB each | 748 | 1,280 |
| Decoded image bytes | 3,063,808 | 5,242,880 |
| Image keys | 320 | 480 |
| Decoded group pages, 128 KiB each | 40 | 48 |
| Decoded group bytes | 5,242,880 | 6,291,456 |

F's compiled launcher is 1,475,803 bytes and its measured Zehn allocation is
19,755,048 bytes. The memory increase is used by the real image and group caches,
including combat preparation's public capacity accounting. It is not merely a
larger limit. The buffers remain statically bounded; no opportunistic allocation
or debug-loader bypass was introduced. F subsequently started successfully from
the ordinary document list in the same normally installed Ndless Firebird
session and displayed the enlarged title/Continue UI. This verifies ordinary
startup; gameplay interaction and frame-time measurements remain separate
acceptance gates. The earlier D allocation failures remain failures.

## Executed correctness checks

The frozen F source checkpoint is
`cbdd5d412e9ca19b33407b8614d9c8659190f73d`. Only the affected native cache targets
were built and run with Clang 18:

- `content_tiles_regression`: 16,396,600 checks passed for directory-only open,
  selected decoding, aliases, owner changes, failed reservations, full capacity
  and fragmented reuse of the 6 MiB decoded-group pool.
- `content_cache_age_regression`: 5,966 checks passed for 480 colliding keys,
  owner/alias eviction, chain clearing and age wrap.
- `test_zehn.py`: both exact-limit and prefix-over-limit allocation cases passed.
- The retile round trip and three animation-precision tests passed, including
  the complete 65,536-color domain, opacity error bounds, protected/shared
  tiles and bilingual NSP relocation with unchanged sparse geometry.

These are correctness results, not performance measurements. No WASM test was
used for UI11. Actual performance evidence must come from the ordinary Firebird
calculator path. The first hand-linked cache build omitted core definitions;
the first standard GCC 13 build hit an internal compiler error in unchanged
`world_relics.cpp`. The standard Clang 18 targets above built and passed.

The source checkpoints and frozen snapshots were local when these commands ran:
automatic approval review blocked the remote source update. The logged-command
checkpoint field identifies the preserved source; it does not itself prove a
remote push or source publication.
