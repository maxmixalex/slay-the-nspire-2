# UI16: read artwork headers during their length probes

## Observed issue and production change

The ordinary Firebird CX II CAS / 396 MHz / OS 6.4.0.74 / Ndless r2022
measurement at `57cd512` reported 43,698 startup ticks (1.3336 s), compared
with UI15's 21,790 ticks (0.6650 s). The scope is the existing
`selected_resource_validation_and_app_init`. This difference is observed;
it is not wholly attributed to one mechanism or claimed fixed before the
next device measurement.

One definite unnecessary operation was duplicated artwork opening. The file
cache opened and closed all eight parts to query their lengths. Multipart
validation then opened all eight again to read their 32-byte format headers.
The four-slot LRU closed four of those handles while opening the other four.

The new production path obtains each length and 32-byte header in the same
open stream and closes that probe immediately. The eight saved headers use a
256-byte local array in the startup branch. `MultipartResourceSource::open`
validates those bytes with the original magic, version, reference, header
size, file length, part index, reserved-field and provider checks. It also
explicitly preserves the old minimum 32-byte source length requirement,
including unused parts. The supplied headers are never retained after open.
Callers without saved headers keep the original stream-reading interface.

There is no payload scan, additional persistent buffer, runtime header-cache
branch or additional file slot. The normal `read_artwork` and LRU paths are
unchanged. Part data still uses at most four live streams; locale and mutable
state streams remain independent. The tested language-switch combination is
four artwork streams, the old and new locales, and one state stream, for a
maximum of seven.

## Profiling scope and counters

The `startup_start` and `startup.add` positions are unchanged. Every new probe
is entirely inside that existing timer scope. Optional probe observation
starts before opening and ends after closing, including input sampling,
seek-to-end, tell, rewind, and the exact header read.

For the eight successful probes, `resource_total` now records eight real
32-byte reads and sixteen actual seeks. Validating the saved headers records
zero physical reads. Previously the second opening's eight header reads and
eight rewinds were included in `resource_total`; the first length probes'
eight seek-to-end operations and open/close time were included only in the
overall startup timer. This change makes resource totals more complete.
Do not treat that changed startup resource-accounting coverage as an I/O
regression or compare it as if the old total included the same operations.

The overall startup scope remains comparable. Runtime `read_artwork`
instrumentation is untouched, so control/action combat counter differences
also retain their meaning. All probe opens and closes still sample input on
both sides. Production uses the existing sampled seek/read callbacks; the
probe observer does not dispatch input or re-enter the application.

## Validation at 946ca63

The final production source is
`946ca639114f5fea39803f5fd569d72c193f74b4`, following the initial implementation
`9dcc5cc29e8aa7a7772b2608d4087a05314c81de`. The second commit explicitly
preserves stream bounds when headers are supplied independently. A source
bundle was updated before long validation; no external publication is claimed.

The complete file-cache regression passes **2,645 checks** in both optimized
native and ASan/UBSan builds. Both the test and changed storage source are
instrumented in the sanitizer build. Checks cover every open/seek/tell/read/
close failure, short and overlarge files, all header fields, unused short
parts, missing providers, stale cursors, reused FILE addresses, failure retry,
LRU eviction, 7-stream language switching, input capture, explicit shutdown,
and destructor idempotence. Probe observer counts are compared with actual
fake-host open/read/seek/close events, including failures before any read.

A separate actual-pack diagnostic executable was frozen and run twice using
the same 89-span multipart artwork, aligned NSL3 English pack, five-Dazed
fixture, and original game core archive from the completed `57cd512` build.
It toggles only the startup probe path. Counters below are actual artwork
host calls, including the length probes in both variants:

| Boundary | Earlier path | Single-probe path |
| --- | ---: | ---: |
| After multipart open: opens / closes | 16 / 12 | **8 / 8** |
| After App initialization: opens / closes | 16 / 12 | **8 / 8** |
| Through App initialization: reads / seeks / bytes | 8 / 16 / 256 | **8 / 16 / 256** |
| After title and combat warmup: opens / closes | 24 / 20 | **16 / 12** |
| After warmup: artwork reads / seeks / bytes | 80 / 87 / 1,094,724 | **80 / 87 / 1,094,724** |
| End Turn action: all resource reads / seeks / bytes | 19 / 17 / 102,505 | **19 / 17 / 102,505** |

All 182 action frame hashes match, and both replays verify five new Dazed
cards. The transcript SHA-256 is
`6aed40cf0ab6841dd856bae3b3828d1f891e5d6a87a3243df967a76aa07c7e22`.
This confirms the mechanism and unchanged rendering/I/O after warmup; it
does not establish calculator milliseconds. Actual assets, driver and frozen
input hashes remain private in `private-ui16-artwork-startup`.

The actual ARM main and storage translation units compile and link against
27 frozen unchanged objects from the passed `57cd512` build. Zehn validates
the result with allocation **22,494,068 bytes**, including a 157,096-byte
prefix and a 22,336,972-byte executable allocation. `.data` and `.bss` are
unchanged; `.text` increases 984 bytes. The main function's static ARM stack
frame increases from **1,224 to 1,400 bytes**. The cache open helper uses 64
bytes instead of 32; no stack frame exceeds the existing 8 KiB warning gate.
The isolated launcher is 1,759,882 bytes with SHA-256
`7a31e95a62b08899d497f8188cf16f37f8f1901545a9dc3ced6f0eff45bb66d5`.
The parent performs the final complete rebuild and ordinary device gate.

Final local journals in `ui16-artwork-startup/build/journal`:

- `artwork-startup-final-native-build-5c19f1bf25da` and
  `artwork-startup-final-native-f71292102028`: passed.
- `artwork-startup-final-sanitize-build-98f34bd8636a` and
  `artwork-startup-final-sanitize-89ae75b7899d`: passed.
- `artwork-startup-final-arm-build-63be90a52af8`: passed.
- `artwork-startup-actual-trace-build-6ef06f2d1331`,
  `artwork-startup-actual-baseline-dd72faef97d1` and
  `artwork-startup-actual-candidate-790457e1a0cd`: passed.

An initial separate storage-suite link omitted the unchanged split-resource
and workspace dependencies and failed to link. Adding the resource-source
translation unit and frozen core archive fixed the harness; the corresponding
ASan/UBSan storage suite passed at `9dcc5cc`. That earlier auxiliary result
is not relabelled as a final full-project sanitizer run. The final cache
suite above directly instruments the final changed code and bounds checks.
