# Bounded LZMA2 decoder source

The seven C source/header files in this directory are unchanged copies of the
official [7-Zip 26.04 source](https://github.com/ip7z/7zip/tree/9128b80e3a471108678e2b3ec3c985861c8d7b0f/C),
pinned to commit `9128b80e3a471108678e2b3ec3c985861c8d7b0f`. `SOURCE.json`
records each original path and verified Git blob hash. Only `LzmaDec.c` and
`Lzma2Dec.c` are compiled. No archive parser, filesystem support, threads,
allocator implementation or encoder is included.

Igor Pavlov places the LZMA SDK in the public domain; see the original file
headers and the official [SDK licensing statement](https://www.7-zip.org/sdk.html).

The project wrapper in `src/lzma2_block.cpp` supplies one fixed 28,544-byte
probability arena and uses the caller's output as the dictionary. It accepts
only property 14, corresponding to a 512 KiB dictionary, and requires exact
input/output lengths and an end marker. Failed output is never
eligible for cache publication. This is a serial decoder for the existing
serial resource reader, not a reentrant general compression service.

Native builds use their normal system C headers. The two minimal headers under
`platform/decoder_headers` are applied only to the decoder C translation units
and their C++ wrapper when cross-compiling the freestanding ARM and WASM targets. `memcpy` is resolved
by the project's existing freestanding implementation. The unchanged generic
SDK types include `errno.h`, but these two decoder units use no errno services.
