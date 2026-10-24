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
The memory reduction and updated ARM package still require the next immutable
build and actual-device/emulator route. No device FPS claim follows from the
source-level buffer sharing or a successful linker result.
