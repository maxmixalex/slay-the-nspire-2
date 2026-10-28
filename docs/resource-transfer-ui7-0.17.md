# Foreground transfer workspace

The immutable checkpoint-7 ordinary ARM build linked but did not package:
its image occupied 17,367,220 bytes, exceeding the existing 16 MiB Zehn image
bound. The failed journal is retained. Raising the limit would not validate
the format or recover calculator memory.

The resource decoder already owns a 617,323-byte temporary encoded-payload
buffer. Its returned image views refer to stable decoded image/group caches,
or to a direct immutable source. They do not retain this temporary buffer.
Ndless save read/write and UI-progress read/write callbacks complete their
file operations synchronously on the same foreground thread. They therefore
reuse that existing workspace instead of retaining a 262,148-byte SaveData
plus a 524,289-byte progress buffer on ARM.

This changes object lifetimes using trivial placement construction, without
zeroing a large array or allocating another stack aggregate. The image cache,
retained/tile group pool, font cache and all decode limits are preserved. The
ordinary legacy save codec and optional history codec are unchanged. The
browser's progress buffer remains separate because JavaScript acknowledges it
across WebAssembly calls; its lifetime is deliberately not treated as a
synchronous file callback.

The added regression overwrites the entire encoded workspace while holding
real retained/tile decoded views, checks those views and warm zero-I/O reuse,
then interleaves a complete historical save roundtrip, cold image decoding and
NSH2 history serialization. It keeps exact saved-state byte comparisons.
Checkpoint 8's actual ordinary ARM build packages successfully at 16,713,656
bytes of image allocation and 1,185,939 bytes on disk. The unchanged 16 MiB
bound leaves 63,560 bytes of static-image headroom. Its native interleaving
regression also passed within the 83/87 complete CTest run; four unrelated UI
assertions remain failed. Actual-device/emulator execution remains separate.
No device FPS claim follows from the buffer sharing or the package result.
