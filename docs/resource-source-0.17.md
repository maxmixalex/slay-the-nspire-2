# Bounded resource access — 0.17

`ResourceSource` represents one immutable resource file, using either a direct
memory pointer or an exact range-read callback. Offsets and file sizes are
unsigned 32-bit values, matching the existing NSP2 directory. A platform owns
the file handle or memory buffer and keeps it alive until the reader is reset.
The source does not allocate or open files itself.

`ResourceReader` copies data into the caller's output. Its fixed cache contains
eight 4096-byte pages and uses round-robin replacement. Requests of at least
4096 bytes bypass that cache, so a picture decode does not displace recently
read metadata. Direct memory sources also bypass it. The complete reader is
statically limited to 33 KiB; FNV checksum calculation uses one additional
4096-byte stack buffer. There is no resource-sized resident allocation.

Range validation subtracts from the declared length before comparing the
count, avoiding offset-plus-count overflow. A failed page fill is never marked
valid. Reset invalidates every page even if the new source has identical size
or offsets. The callback must report false after any short read. Integer and
checksum helpers leave their outputs unchanged on failure; bulk destinations
must be ignored after failure. Zero-length reads at the exact end are valid.

No cache pointer escapes the reader. Content parsing remains responsible for
the lifetimes of decoded pictures, glyphs and strings, and for validating their
format. Existing absolute offsets and payload bytes can remain in a single
NSP2 file. Its full FNV checksum is streamed once at open. The
256 MiB on-disk policy is separate from this API's unsigned 32-bit address
range and from the bounded runtime memory budget.

`Content` now reads and validates the complete directory into a fixed 2 MiB
buffer when a direct memory source is absent. Image payloads use four bounded
cache classes: 64 × 4096 bytes, 16 × 32768 bytes, 8 × 131072 bytes and two
buffers large enough for the maximum 320 × 240 sparse image. Glyphs and
strings have separate bounded caches. Views remain valid until their cache
slot is reused. Cache entries include the owning Content object and are
invalidated on every open, including failed opens. A failed payload read
marks that Content unhealthy, so a stale cache cannot hide a short read.
The memory-backed path remains available for fixtures and byte-for-byte
comparisons; production no longer reserves a resource-sized array.

Ndless and X11 retain the file handle while the app runs, and their provider
seeks to the requested offset and requires an exact read. The Ndless seek
and tell wrappers use the official SDK syscall numbers 33 and 89. The browser
retains its decoded pack in host memory and copies only requested ranges into
the fixed 16 MiB WASM memory. The standalone HTML still embeds one complete
pack and runs offline. These source changes passed the frozen target builds and validation below.

The dedicated `resource_source_regression` fixture covers truncated providers,
retry after a read failure, partial final pages, page crossings, cache capacity
and eviction, bulk-read cache retention, source reset, near-4-GiB range checks
without large allocation, little-endian values, memory parity, and segmented
FNV checksums. The prepared fixtures passed after publication and freezing of the resource stage.

The additional `resource_content_regression REAL_PACK` executable compares
every image, glyph, string and clip through memory and callback sources,
alternates both sources through identical rendered UI actions, and verifies
rejection/recovery after a provider failure. It is deliberately not a default
CTest entry because its input is the owner's private original-derived pack.

## Executed resource validation

Runtime commit `5b665a8b81d70df9215dff31e74a705d7cfeb121` was remotely verified before all seven logged executions. Native ASan/UBSan compilation, both default fixtures, the actual private pack comparison, ARM and WASM builds, and the production WASM range callback against existing native captures all passed with unchanged source. The actual comparison made 39,164 checks and rendered 40 exact memory/range frames. The backend check compared 384 frames and 23 byte-exact historical v14 saves. ARM allocation is 8,434,016 bytes compared with the prior 16,174,284, without increasing the 16 MiB ceiling. See `validation-resource-0.17.json` for the exact journals and hashes.

The tested content remains the completed NSP2/13 input. Expanded world models, map behavior, NSP2/14 assets and hardware performance have separate outstanding acceptance checks.
