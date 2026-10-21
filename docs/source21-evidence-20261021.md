# Source21 staged resource startup — 2026-10-21

The reference remains **v0.111.0 / 41cef1ea**. Source21 follows verified source20
`3f66da839e056fe291c27ca5e93260b3833bd7de` on `work/0.17-world-parity`.
The complete private resource file is reused unchanged; original inputs,
derived packs, screenshots and embedded-content HTML remain outside public git.

## Completed source20 gates

All **502 source files / 37 changed contents** matched the remote tree and
read-back. The source-only archive SHA-256 is
`304729acca3c12d9296344caf207281e0e50331726f58717200c5538ec2bcb5f`.
All nine formal journals have one unchanged runtime fingerprint:
`bbde75bed44b92e0fdef82cac6997536caaa3ce202b8b09c08c90348f9f3af35`.
See [validation-source20-0.17.json](validation-source20-0.17.json).

| Gate | Executed source20 result |
| --- | --- |
| Native ASan/UBSan build | All 145 steps passed |
| ARM packaging | 883,404-byte launcher; 14,710,272-byte allocation; 28,596 relocations |
| WASM linking | 4,875,740 bytes; existing 16 MiB memory limit retained |
| Linked ARM division/ABI | 5,249 vectors passed |
| Linked ARM LZMA2/ABI | 28 vectors, including maximum output and corruption cases |
| Registered sanitizer suites | Ten affected suites passed out of 56 registered |
| Final synthetic tiled content | 3,565,023 checks, including fixture construction assertions |
| Standalone bounded decoder | 6,400 checks |
| Python encoder | 24 tests passed in 6.008 seconds |
| Complete private conversion | 455.779 seconds; full inverse validation passed |

The complete production file is **80,846,058 bytes (77.100809 MiB)**, SHA-256
`407589a297a6e1bf935b6f2a5024ad532efd84423b0374db13c080e6671ba7dd`.
Its directory contains 45,724 images, 1,695 clips, 11,915 glyphs, 6,100 strings,
558 cards and 1,175 auxiliary entries. Validation compared all 28,910 canonical
frames / 443,359,210 original sparse bytes, 42,189 timing entries and 3,535
retained images. The 732 tile groups and 452 index groups were all decoded,
including all 1,184 reused cache entries. No frame was removed, resampled or
quantized by this storage conversion. The original v14 input remains unchanged.

The private manifest SHA-256 is
`f076004a4625cf2ce520beca14ab865f620179d697b668dacf6199b4e07f73cd`;
the complete conversion journal SHA-256 is
`a32796c0e8d89e95aa88d522c68ee12a704089ce441cbf1b42f7225cf252b858`.
Neither resource storage fit nor linked allocation establishes a hardware
frame rate or peak working-memory measurement.

## Why startup changes

Source20 eagerly reconstructed every tiled image during `Content::open`.
Its complete access model includes 42,189 entries, the existing 90 decoded
image slots, and the 4 MiB contiguous page/LRU pool. It still predicts
**30,793,111,501 decoded group bytes** and **3,635,936,324 logical file bytes**
before ordinary use. Those are modeled access costs, not measured startup time.
The 452 index groups contain only **60,089,163 decoded bytes** and
**6,478,949 encoded bytes**. This provides a concrete way to avoid decoding
every image before it is requested.

Source21 keeps full-file FNV checking, all resident directory validation,
every group header/range and every index stream's decompression/CRC. It checks
all descriptors, record boundaries, tile IDs and sparse geometry, including
each image's dimensions. Tile pixel streams are deferred until actual use.
The complete contract is in [content-tiles-0.17.md](content-tiles-0.17.md).

Before an image is first published, all its tiles are checked, including
inactive edge tiles, group padding and opaque-span alpha. A failed access
publishes no cache entry or validation bit and makes the Content unhealthy.
Only an exact descriptor-and-shape cache hit can transfer completed validation
to another timing alias. Later cache misses may omit entirely inactive tiles
of an already validated image; all IDs and loaded data remain checked.

Temporary descriptor lists and the per-image validation bitmap borrow the
unused tail of the existing 2 MiB directory buffer. The actual file uses
233,088 temporary bytes and a 5,716-byte bitmap. No new large resident array
or heap allocation is added. If lists do not fit, or descriptor aliases have
conflicting shapes, bounded sequential structural checks replace grouping.
If the bitmap does not fit, every image cache miss performs full tile checking.
Owners and every reopen reset validation state. Providers remain immutable
for the Content lifetime, as required by the existing cache contract.

## Short diagnostics and independent review

Two independent Content-only ASan/UBSan probes passed: **2,948,034 checks in
16.592 seconds** for structure, first touch, alias/owner/reopen and directory
fallbacks; **2,351,298 checks in 19.962 seconds** for view lifetime, real cache
pressure, partial-read failures and inactive reconstruction. These are separate
selected diagnostics, not their sum as a complete registered-suite result.
The pressure fixture now actually accesses its images after opening; an open
alone no longer fills the pixel-group pool. A maximal legal directory leaves
fewer than eight free bytes and proves the no-bitmap fallback.

A separate actual-file Content-only O0 sanitizer smoke passed in 8.984 seconds
including compilation. Its measured open was **4.292 seconds**, with 26,068
callbacks / 118,308,847 callback bytes; its first tiled image took 0.045 seconds
and produced 20,467 original format-4 bytes matching the independently stored
v14 image. This is one host diagnostic and one image, not the full-pack C++
proof, integrated App build or a calculator performance measurement.

`resource_open_benchmark` now records Content open separately from requested
image access, including real file-range callbacks and bytes. Construction,
filesystem opening and App initialization are outside its open interval.
The benchmark is deliberately not a registered synthetic test.

The complete resource oracle compares both new backends against the same old
v14 bytes in two full sequential image traversals. It still compares complete
decoded data, dimensions and format for every image, without replacing byte
equality with a hash. The change avoids deliberately thrashing the shared
tile pool by alternating owners on every image. All metadata checks remain,
and 40 actual UI frames still alternate all three owners. Progress flushes
every 1,000 images so a long proof remains observable.

## Post-checkpoint gates

1. Push/read back the complete source21 tree and changed contents, retain a
   source-only archive, and freeze the exact source before long execution.
2. Rebuild native, ARM and WASM; verify the linked memory bounds and execute
   final first-touch/cache/fallback sanitizer tests with the new App ABI.
3. Measure actual old/new Content open and representative first-image access.
   Run the complete three-way byte oracle in an optimized native build;
   state explicitly that this large-file pass has no sanitizer instrumentation.
4. Capture and replay all 23 native/WASM scenarios with the actual v15 file,
   compare 480 frames / 30 saves, and inspect the changed localized map titles.
5. Assign a concrete delivery status only after the matching pack/runtime
   gates pass. Keep hardware startup/frame-rate and original-PC visual
   acceptance separate from host results.

**95% original UI fidelity is not established.** Original label-clock phases,
remaining shader/particle layers and temporal animation sampling are explicit
implementation differences, outside the calculator-readability allowance.
No new Firebird or physical-device run exists. Main, disabled CI and the
verified 0.16 pair remain unchanged.
