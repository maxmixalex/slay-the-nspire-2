# UI21: complete resources, smaller installation, bounded decoding

## Delivered result

UI21 delivers a complete matched NSP2/45 resource set, a production ARM926
launcher, and the fourteen optional original locales. It includes the repaired
UI20 artwork that had previously been built privately but not delivered in a
complete installation. Original artwork and generated resource files remain
private; this repository contains the conversion and runtime code.

| Quantity | UI19 complete delivery | UI21 final delivery | Change |
| --- | ---: | ---: | ---: |
| Required bilingual `.tns` files, including launcher | 74,538,517 B | 73,295,445 B | -1,243,072 B |
| Complete bilingual archive after extraction, including notices | 74,608,678 B | 73,367,990 B | -1,240,688 B |
| Bilingual ZIP download | 73,726,639 B | 72,473,950 B | -1,252,689 B |
| Fourteen optional locale `.tns` files | 18,497,856 B | 18,507,815 B | +9,959 B |

The required installation is **73.295445 MB**, 1.668% smaller than UI19. Its
equivalent 69.899983 MiB is a unit conversion, not achievement of a 65–70 MB
decimal target. No resolution, animation frames, supported languages, or font
sizes were removed to obtain a smaller number.

The optional ZIP is 17,035,233 B and extracts to 18,508,650 B including its
instructions. All sixteen locale files plus shared data total 90,123,281 B;
installing all of them with the launcher requires 91,803,260 B before notices
and mutable saves. The original compressed font/text tails are preserved for
every language. The small optional-language increase is directory/storage
metadata for the changed common resource layout, not enlarged glyph payloads.

## Separate restored content from compression savings

The raw-source repairs add **1,105,032 B** to the bilingual data installation
before new compaction. The storage passes then remove **2,304,056 B** from that
complete artwork baseline. The launcher is 44,048 B smaller than the launcher
in the recovered UI19 delivery. Together these changes produce the net
1,243,072 B reduction above.

The table measures installed data files, not intermediate decompressed bytes.
Different logical addresses can slightly change outer locale-directory
compression, so a logical-stream reduction need not equal the final `.tns`
reduction.

| Actual stage | Bilingual data | Difference from preceding stage |
| --- | ---: | ---: |
| Recovered complete UI19 | 72,814,490 B | — |
| Complete repaired original artwork | 73,919,522 B | +1,105,032 B |
| Unreachable tile slots removed | 73,473,393 B | -446,129 B |
| Initial lossless sparse-tile candidate | 71,746,638 B | -1,726,755 B |
| Unreachable retained slices removed | 71,709,934 B | -36,704 B |
| Artwork-only stream retuning; original fonts/text | 71,418,581 B | -291,353 B |
| Restore dense streams above 60% occupancy | 71,615,450 B | +196,869 B |
| Final common-file partition for all sixteen locales | 71,615,466 B | +16 B |

An earlier size-first candidate would have required 73,057,024 B including
the launcher. It is **not** the delivered package: real ARM decoding showed
that some of its small savings made individual cold groups more expensive.
UI21 pays 238,421 B relative to that candidate to preserve the existing
font/text streams and to restore dense tile streams where appropriate.

## What actually changed

### Original resource completeness

The owner-supplied v0.111.0 / `41cef1ea` PCK was used to rebuild nine monster
actions (325 frames), eight full-screen Ancient scenes, three Osty death-loop
samples, and the distinct reward icons. Twenty-three overflow pieces preserve
large action frames without lowering their native image density. The final
catalog contains 58,099 images, 1,783 clips, and 5,422 auxiliaries. The repaired
English NSP2/44 artwork exactly reproduced the previously recorded source
artifact hash:

`d823e669731c75bcf7f62f307f699c0212a211af0f236c36383a4b6777cf66b5`

Source sampling, visible RGBA, final RGB565/alpha, overflow seams, frame clocks,
and the production Canvas were checked independently. See
[ui21-art-and-font-storage.md](ui21-art-and-font-storage.md) for the source
artwork and font decision, and [ui20-animation.md](ui20-animation.md) for the
underlying animation implementation.

### Lossless storage changes

`compact_tile_slots.py` removes 5,316 unreachable tile records, compacting 182
groups and updating the corresponding index references. This removes
4,093,320 decoded bytes; actual installed bilingual savings are 446,129 B.
Image geometry, surviving colors/alpha, and frame references are checked.

`compact_retained_slices.py` reclaims unreachable bytes only inside an existing
retained-image group. Seven groups become smaller, removing 256,598 raw bytes.
The logical saving is 36,864 B after preserving 4 KiB address phases, while
installed bilingual data saves 36,704 B. Group membership, codecs, dictionary
limits, all live image slices, fonts, and strings remain unchanged.

`retune_resource_streams.py --artwork-only` uses stronger offline compression
of image/index streams while preserving their exact decoder input. Its formal
receipt records **zero glyph groups and zero text groups retuned**. No runtime
codec, larger dictionary, global startup scan, or new resident buffer is
introduced by this step.

### Sparse tile selection and runtime restoration

TGR3 omits pixels only when both their stored RGB565 value and alpha are zero.
Invisible nonzero colors and record padding are preserved. A sparse stream
must fit the existing scratch reservation and save space. UI21 additionally
limits selection to **60% occupied pixels**; the converter permits tightening
that threshold but does not permit increasing it above 60%.

The final set has 338 sparse groups among 637 tile groups, plus 500 index
groups. Across the complete tile catalog, summed LZMA output falls from
512,866,970 to 391,803,490 B, a 23.605% reduction in the intermediate bytes
decoded. These are sums over independently loaded groups, not resident memory
or a claim about actual play-session I/O. Every restored tile record remains
exactly the same.

The runtime zeroes the tile body efficiently and visits only set occupancy
bits, instead of branching over all 256 pixels. The bit-scan is called only
with a nonzero mask. Color-mode dispatch is outside the per-pixel loop. Output
and scratch guards, partial rectangles, invisible RGB, both color modes,
maximum bounded groups, and AAPCS preserved registers were exercised in the
actual linked ARM926 launcher.

The 107 groups exceeding the final occupancy limit were restored from the
matching original dense streams. The converter compared all **95,118,870 B**
of their complete decoded records before accepting the replacement. It also
preserves suffix page phases and group/frame identities. No renderer or
runtime change was needed for this selection correction.

## Measured performance and its limits

Measurements below execute functions from the production ARM ELF under an
ARM926 emulator. They count executed instructions, not wall-clock cycles,
Flash latency, or calculator frames per second. The complete-group comparisons
include LZMA2 decoding and tile inverse work; they do not include OS reads or
the incremental scheduler.

| Actual retained group | Occupancy | Previous instructions | Final selected instructions | Change |
| --- | ---: | ---: | ---: | ---: |
| 296 | 38.5901% | 41,802,362 | 32,283,734 | -22.77% |
| 312 | 51.6664% | 49,602,225 | 43,524,527 | -12.25% |
| 33, highest selected occupancy | 59.9828% | 22,527,356 | 20,578,972 | -8.65% |
| 434, smallest absolute byte saving | 0% | 51,133 | 47,961 | -6.20% |

The final streams for these samples are identical to the measured candidate
streams. These samples support the chosen policy; they do not prove a speedup
for every group or a game-wide FPS improvement. The isolated sparse inverse
also passed 44 vectors for each launcher and twelve matched instruction
windows, with reductions from 10.39% to 66.92% in those inverse-only windows.

The following candidates were measured and rejected:

| Candidate | Observed tradeoff | Final decision |
| --- | --- | --- |
| Sparse group 86 at 79.6109% occupancy | Saves 715 stream bytes but complete decode instructions rise 8.44% | Restore its original dense stream |
| New font literal contexts | Largest changed English and Chinese samples rise 11.03% / 11.34% in decoder instructions | Preserve all original font/text streams; revert the two new default profiles |
| Split sparse color byte planes, 24 real groups | Per-tile layout adds 394,797 B; global layout adds 339,936 B | No new format or decoder |
| Three cold translation units compiled with `-Os` | Launcher saves 28,366 B, allocation saves 58,980 B; only 0.039% of installation | Keep production `-O2`; those functions can participate in gameplay |
| Same three units with `-Oz` | Introduces unavailable `__aeabi_llsl` / `__aeabi_llsr` helpers | Reject without adding runtime helpers |
| Cross-group retained duplicates | Only one useful duplicate, 9,240 raw bytes | No extra cross-group dependency mechanism |

The same-toolchain launcher comparison is separate from the UI19 delivery
comparison. LLVM 18 builds of UI20 source and UI21 runtime are respectively
1,679,626 and 1,679,979 B: the faster sparse inverse costs 353 stored bytes.
Historical UI20's 1,675,061 B was produced with a different compiler setup and
is not used as a controlled A/B measurement. The final Zehn allocation is
22,291,016 B, below the 24 MiB loader allocation guard. Static analysis records
a 20-byte increase in the inverse step's bounded stack frame; no additional
resident cache or decoding plane was allocated.

No new Firebird or physical-device timing run is claimed for UI21. Native
rendering and linked ARM instruction checks establish specific correctness
and algorithmic facts; they do not establish actual calculator smoothness.

## Final validation

The 7b production runtime passed all 158 existing native suites, the public
filesystem lifecycle regression added afterward, 4,799 linked ARM memory
vectors, and the sparse ARM vectors above. Executable permission metadata
initially prevented a few generated native tests from starting; those tests
were rerun after restoring their executable bits, and all actually executed
suites passed. There were no suppressed assertion failures.

The initial exact candidate passed all 58,099 images through production
memory, ranged, and preload readers, comparing 1,170,062,350 decoded bytes.
After the final performance selection, an independent map verified that the
1,030 unchanged tile/index groups retain their encoded bytes and that all 107
replaced groups restore exact records. Forty-two actual images covering those
107 groups were then checked again through all three production readers.
The remaining identical streams reuse the already completed full-catalog
evidence. Final 328-frame / 351-piece Canvas checks and ten captures are exact.

The final English and Chinese logical files separately passed all sixteen UI
suites (664 captured frames) and twenty filesystem/icon subprocess scenarios.
The latter include direct abandon, paused abandon, death, chapter-two/three
abandon, and independent process restarts. Chinese validation covers all
53,632 glyphs, 8,704 strings and 1,783 clips. All sixteen locale grafts preserve
their compressed font/text tails, verify their individual glyphs and strings,
and reconstruct exactly from the common files plus locale file. Across all
sixteen catalogs this covers 344,590 glyph entries and 139,264 string entries.

Both ZIPs were created and every member was read back and compared with its
source. The installation's outer layout is only `nSpire2.tns`, `data/`, and an
empty `state/`; notices and build information are inside `data/`. No test save,
profile, or diagnostic marker is included. Optional languages contain only
the fourteen matching locale files and their instructions under `data/`.

## Reproducibility and artifacts

- Runtime checkpoint: `7b0cd8b4f2fd824eea781e0e0a853011c3474b1c`.
- Resource policy checkpoint: `9d5f9966bb061cec48f7f6efeef131aa243540c6`.
- Package tooling checkpoint: `01176770c7bcb29c63e03f0bdf302fcd701669ac`.
- Final English logical SHA-256: `36e7c560558617053081bc4c731d838a3916801f6f9ce3cc8747e1c77dc94e40`.
- Final Chinese logical SHA-256: `6094d63ef015d691252e5bb788aed356f7b252d1cd77b5fc6d322646f59ce2ee`.
- Launcher SHA-256: `03e7b61abc9f6813f7b67d3c603848c1239d20fe0dc10b0673b71afcd4d66b93`.

| Delivered archive | Bytes | SHA-256 |
| --- | ---: | --- |
| `Slay-the-Nspire-2-0.1.0-ui21-bilingual-calculator.zip` | 72,473,950 | `172789594bfa5509b4f1567db5843abb90e6402e657ab1b4f4dda359d5f10cef` |
| `Slay-the-Nspire-2-0.1.0-ui21-optional-languages.zip` | 17,035,233 | `752f0b338622f0f1917cbbd947c26f83ddac9badd4bdf0743b53d6d9e777a7e7` |

Private conversion receipts record exact inputs, outputs, all locale hashes,
and per-group choices. Formal journal stages include
`ui21-final-artwork-only-09d7898e7960`,
`ui21-final-density-policy-3d174fd9264d`,
`ui21-final-all16-ad78b718d79c`,
`ui21-final-offline-policy-tests-cc56df503460`, and
`ui21-final-two-zips-22d452d54be4`; each passed with no source changes during
execution. The earlier unsupported `--workers 4` invocation of the two-worker
tile-slot converter and the rejected `-Oz` build remain recorded as failed
experiments, rather than being relabeled as successful release validation.

The final converter sequence uses the complete repaired artwork, tile-slot
compaction, exact sparse coding, retained-slice cleanup, artwork-only stream
retuning, and the dense-reference occupancy correction. It then grafts every
original locale tail once onto the same final artwork with
`refresh_artwork_resources.py`, followed by `package_release.py
--calculator-only --stage ui21`. Full commands, input hashes, and private
paths are retained in the local run journals. No hand-built WASM player or
manual browser acceptance is part of this release.
