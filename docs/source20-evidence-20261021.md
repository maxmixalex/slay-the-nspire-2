# Source20 evidence and execution gates — 2026-10-21

The reference remains **v0.111.0 / 41cef1ea**. Source20 follows source19
`00c6b9dda80333c3aa260d82e650b079f9fe76a3` on `work/0.17-world-parity`.
Original inputs, transformed packs, captures and embedded-content HTML remain
outside the public source repository.

## Completed source19 verification

All **479 source files / 35 changed contents** matched the remote tree and
read-back. The source-only archive SHA-256 is
`5b2d9e8ca13ce1114be840282f28adb3faf7dc713c79601b74c824d845e3c2a1`.
The one frozen runtime fingerprint is
`ecaa06183762b72a53399c902206437e607d3a090160bd2e6d353c3a52b64168`.
See [validation-source19-0.17.json](validation-source19-0.17.json) for all
individual journal and artifact hashes.

| Gate | Executed source19 result |
| --- | --- |
| Native build | All 138 steps passed with ASan/UBSan |
| ARM packaging | 870,008-byte launcher; 10,466,576-byte allocation; 28,574 relocations |
| WASM linking | 4,852,522 bytes |
| Linked ARM division/ABI | 5,249 vectors passed |
| Affected registered suites | 31 distinct suites passed out of 54 registered |
| Complete character acquisition suite | 38,001 checks / 860 variants |
| Complete enemy execution suite | 48,111 checks / 174 encounter-ascension variants |
| Actual content memory/range | 317,981 checks / 40 identical UI frames |
| Actual act-entry UI | 43,364 checks; captures inspected for viewport/readability |
| Expanded native/WASM comparison | 23 scenarios / 480 exact RGB565 frames / 30 exact saves |

The first native backend invocation could not open its frame-zero manifest
because its output directory did not yet exist. Its failed journal remains.
After creating the directory, a separately logged same-source native run and
the dependent WASM comparison passed. There are **16 passed journals and one
retained launch failure**. None reports changed source. This is not a complete
54-suite rerun: the complete 52-suite baseline remains source17. Backend
agreement is not an original-PC differential comparison, browser DOM test or
hardware performance result.

## Source20 implementation

The new resource-storage version preserves the complete catalog, all clip
timing/pivot records and original sparse format-4 output bytes. Exact tile and
frame deduplication use byte comparisons after hash matches. The writer reads
an already built private NSP2/14 pack, validates retained payloads, writes all
group headers/CRC and the whole-pack FNV, independently reconstructs the new
file, and publishes a new output only after verification. It does not resample
animations, quantize colors or rerender original assets.

The C++ reader uses one **4 MiB / 64-page contiguous LRU pool**. Index groups
stay pinned during a frame rebuild; tile output goes directly into the existing
90 image-cache slots. The resident directory remains bounded by 2 MiB. The
unmodified public-domain LZMA decoder is isolated behind a fixed **28,544-byte
probability arena**, strict 512 KiB output and encoded-input bounds, exact
stream consumption/end-marker checks and CRC. No SDK heap or filesystem
support is used. Native C, freestanding ARM and WASM build paths include the
same decoder sources. `run_logged.py` now fingerprints `third_party` too.

The first independent Content-only ASan/UBSan diagnostic passed **2,867,027
checks**. A subsequent separate probe passed **697,996 checks** for maximum
320×240 output and corrupt padding in an otherwise unreferenced tile within a
loaded group. Counts include synthetic-fixture construction assertions; they
are not added together to claim the final merged suite ran. The standalone
decoder diagnostic passed **6,400 checks**. The final merged tests and linked
App ABI remain post-checkpoint gates.

The independent converter review reproduced and repaired an input-path
collision with the encoder's reserved cache namespace, a descriptor-array
payload-boundary mismatch and incomplete glyph-order/string-length checks.
Resolved paths and final directory entries, including symlinks, are protected;
the input pathname's inode is rechecked before publication. All **24** final
asset-free converter tests passed in **2.588 seconds**, including protection
before CLI path normalization. These are short Python
diagnostics, not the complete private-pack conversion.

`resource_content_regression` now determines complete byte lengths from decoded
images. It cannot accidentally compare only a format-6 descriptor's 16 bytes.
An optional reference pack adds a three-way comparison: new memory-backed
content, new range-backed content and the independently stored original v14
content. All decoded images, glyphs, strings, clips, card/auxiliary references
and 40 UI frames are checked. A new linked ARM decoder test executes synthetic
streams and corruption cases as ARM926 instructions with output guards and
callee-saved register checks; it is not a device performance benchmark.

The actual map captures also exposed handwritten top-bar region names that
differed from the original act banner. The top bar now uses the same original
localized act title, preserving its existing small-screen width and fallback.
The calculator launcher metadata now identifies 0.17 development.

## Storage evidence and remaining gates

The complete sparse-tile diagnostic's 680-tile / 512 KiB candidate accounts for
**80,846,058 bytes (77.100809 MiB)**, including directories, headers, padding,
retained images, glyphs and strings. All **28,910 canonical frames**, totaling
**443,359,210 original sparse bytes**, matched; all **42,189 timing aliases**
were checked. A 170-tile candidate measured **88,550,010 bytes** and completed
its byte proof. The 85-tile candidate measured **93,055,786 bytes** and also
completed its byte proof. All three candidates verified the full 28,910-frame
set in one completed diagnostic run. These are diagnostic candidates, not
production resource-file receipts.

The default 680-tile choice meets the 80 MiB pack target. The access model also
shows substantial decompression work: its 4 MiB LRU active-frame traversal has
44,819 group misses and about 23.1 GB of decoded group data. Startup's eager
validation models about **31.3 GB** of decoding. Adding the original 90 image
slots to that model still leaves **30,793,111,501 decoded bytes**; image-cache
reuse alone does not resolve it. All 452 index groups total **60,089,163 raw
bytes**, providing a concrete path toward staged structural/first-use checking
in the next source checkpoint. Neither modeled byte counts nor native throughput
can establish calculator frame rate. The model originally counted an extra
8-byte lookup read per miss although that table is resident; its separate
correction receipt must retain the original diagnostic evidence.

Next execute, after source20 push/read-back/freeze:

1. Build native with C/C++ ASan/UBSan, ARM and WASM. Check the actual linked
   allocation against the unchanged 16 MiB limits and run linked ARM division
   plus decoder/ABI vectors.
2. Run the final decoder, tiled-content and existing resource regressions,
   the asset-free Python encoder tests and affected App/UI suites.
3. Repack the complete private v14 file with the frozen encoder, preserving
   verified caches; record actual bytes, hash, full inverse validation and
   source fingerprint.
4. Execute the three-way complete resource comparison, capture/replay all
   23 backend scenarios with the new pack, inspect changed actual map frames
   and measure startup/range-read costs before choosing a delivery status.

**95% original UI fidelity is not established.** Original label-clock phases,
remaining scene effects and temporal animation sampling are explicit separate
differences; performance sampling is not a calculator-readability exception.
There is no new Firebird or physical-device execution. Main, disabled CI and
the verified 0.16 pair remain unchanged.
