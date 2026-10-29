# UI16: bounded artwork file handles

## Device failure and implementation

The integration's ordinary-OS diagnostic reached `main` and identified
`nSpire2.effects.dat.tns`, the seventh artwork part, as the failed open. The
index and six preceding parts were already open: seven successful streams,
followed by a failed eighth simultaneous open. The requested file was present,
and the independently read index/part bytes and lengths were correct. This
observation applies to the validated CX environment; it is not a claim about
every calculator OS or the SDK's unrelated file-wrapper limits.

`ResourceFileCache<8,4>` now exposes all eight immutable part sources while
retaining at most four actual streams. Its startup length probes open and close
one file at a time. `MultipartResourceSource::open` reads the existing required
headers and span table through those lazy sources; no artwork payload scan or
copy is added. Once this succeeds, the index stream closes because the adapter
already owns its header and spans. The existing NSC1 common-file path retains
its stream as before. A failed multipart installation does not rebuild or fall
back to a monolithic artwork file.

The cache uses exact LRU ranks bounded by four slots, avoiding an overflow-prone
global timestamp. It evicts before opening a replacement, so even a cache miss
cannot temporarily exceed four streams. Each open owns a separate
`ResourceFileCursor`. Reopening a reused `FILE` address always invalidates that
cursor. Warm adjacent reads still omit the seek; failed opens are retryable,
and seek/short-read failures close the failed stream before a later retry.

Cold opens and closes call the existing foreground input collector both before
and after the OS operation. Reads and seeks keep their existing sampling. Input
collection does not dispatch or re-enter `App`. Read timing includes cold open
and eviction costs rather than hiding them from the resource counters.

## Handle budget and lifecycle

All run, history, preferences, selector, profile, and feedback callbacks were
checked: each closes its current file before opening another state file or
checking/renaming its backup. There is no callback retaining two state streams.

| Phase | Application-owned streams at peak |
| --- | ---: |
| Eight sequential length probes with index open | 2 |
| Multipart header validation with index open | 5 |
| Ordinary gameplay, four artwork slots and selected locale | 5 |
| Ordinary state operation | 6 |
| Language switch retaining old/new locale | 6 |
| Language switch plus one state operation | 7 |

The selected locale remains outside the artwork cache. Failed initial locale
loads and language rollback can reopen their own streams without clearing or
replacing artwork providers. The language-switch guard now uses the valid
`common_source` instead of the index's already-closed `FILE`.

Startup errors and a fatal language rollback close all resource streams before
the boot error's input-release gate. Normal exit explicitly closes locale,
common/index, and all cached part streams before the final release gate.
Repeated cleanup and scope destructors then perform no filesystem calls.

No dynamic allocation, additional image pool, or large stack object is added.
The ARM926 `main` translation unit compiles with a 1,224-byte main frame;
cache `read` uses 56 bytes and `open` uses 32 bytes. This is a compile/stack
result, not independent device acceptance or a final linked-allocation figure.

## Executed regression evidence

`resource_file_cache_regression` uses a host that refuses an eighth open stream,
reuses closed `FILE` addresses, and injects open/seek/tell/read/close failures.
Its real `MultipartResourceSource` traverses sixteen spans across all eight
part providers. The test includes:

- Sequential probes with a one-stream ceiling, missing/zero/oversized files,
  failed length operations, and cleanup after partial initialization.
- Four-slot LRU order, contiguous reads without new seeks, reopen invalidation,
  failed-open retries, short reads, seek failures, and replacement close errors.
- Null/zero-byte/boundary requests, overflow rejection, closed sources, and
  explicit close followed by a destructor with no further OS activity.
- Multipart reads crossing several parts, repeated and backward reads, and
  bytes matching the independent canonical common-file fixture.
- Old and new language streams plus one state stream held while artwork churns:
  peak **seven streams**, with no host-limit rejection. The only rejection in
  this scenario is the deliberately failed language open. Both locale cursors
  retain their own sequential position through the artwork activity.
- Input edges that occur inside a cold open/close are collected by the hooks.
- Artwork shutdown leaves the language stream untouched until its own close.

Native Clang 18 `-O2` and ASan + UBSan both pass **982 checks**. The sanitizer
build directly instruments the new provider, test, and `src/storage_source.cpp`.
LeakSanitizer alone is disabled under the executor's ptrace environment. The
tests do not substitute for the parent's final device startup, language-switch,
gameplay, and exit/restart checks with actual resource files.

Checkpoints are `9222a2a9c340ca9fdc2e23951002006736cebdcc` and the test-only
compile correction `f8f7e08b538931a60600cc438724e4591c44d454`. The first test
compile caught a mixed-type `auto` declaration in the fixture; it was corrected
without changing production behavior, and that failed journal remains visible.
Production ARM compilation already passed at the first checkpoint.

Successful `tools/run_logged.py` receipts under `build/journal`:

- `artwork-handles-native-9f6d8ddbe1ba`
- `artwork-handles-sanitized-54abc1d11764`
- `artwork-handles-arm-main-28cac4cc694c`

The source-only checkpoint bundle is `../ui16-artwork-handles-source.bundle`.
Original resource files are not included in Git.
