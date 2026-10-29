# UI16 resource layout and compression

Reference: the owner's pinned v0.111.0 / `41cef1ea` and the delivered UI15
NSP2/40 resources. Source, conversion and runtime evidence are separate.

## Installed files

The former 74,024,394-byte `nSpire2.dat.tns` is now a 1,616-byte NSM1 index
plus eight required concern files. Every part belongs to the same resource
edition; these are not optional art packs. A missing or invalid required part
produces the built-in English resource error before the game starts.

| File | Bytes | Concern |
| --- | ---: | --- |
| nSpire2.dat.tns | 1,616 | Virtual artwork addressing and original NSC1 header |
| nSpire2.interface.dat.tns | 10,927,062 | UI and common visual controls |
| nSpire2.cards.dat.tns | 2,903,300 | Card illustrations |
| nSpire2.characters.dat.tns | 1,238,530 | Player character animations |
| nSpire2.enemies.dat.tns | 34,618,504 | Regular and elite creature animations |
| nSpire2.bosses.dat.tns | 8,834,136 | Boss animations |
| nSpire2.rooms.dat.tns | 4,742,228 | Room, event and merchant visuals |
| nSpire2.effects.dat.tns | 10,628,084 | Later effect and layered animation families |
| nSpire2.shared.dat.tns | 132,742 | Mixed groups and preserved padding |

Compressed decoder groups stay intact and are assigned using their actual
image references. Mixed groups follow their majority concern; tied groups
remain shared. This avoids splitting an indivisible decoder request between
files. The 95 virtual ranges preserve every old artwork byte and logical
address. Independent Python readback reconstructs the exact old common file.
The extra index/part headers cost 1,808 bytes in total.

The calculator exposes eight logical artwork sources through a four-stream
LRU cache and keeps an in-memory addressing table. The part lengths are probed
one at a time; the small index file closes after its validated header and spans
are resident. A request uses one cached range or a bounded binary search and
then reads the original requested bytes from the appropriate file. A miss
closes the evicted stream before opening its replacement. Each stream owns an
independent seek cursor, and failed operations invalidate that cursor. Images
are not preloaded at startup. The table capacity is 1,024 spans, rather than
reserving an unnecessarily large map for the current 95 spans. Native loading
uses the same storage adapters. Legacy NSC1 + NSL1/NSL2 installations remain
readable. See `ui16-artwork-file-handles-0.17.md` for the actual-device open
failure that motivated the bounded cache, lifecycle handling, and tests.

## Independent locale pages

NSL3 wraps an unchanged NSL1/NSL2 language container in independent 4,096-byte
LZ4 pages. It is distinct from the existing NSZ1 save/history envelope. The
converter chooses HC level 12 when available; encoding time is offline. Pages
saving at most 16 bytes remain literal. Contiguous literal pages coalesce into
a single provider read, and compressed pages share one 4 KB decoded cache.
The entire inner NSL2 stream is byte-identical after decompression.

This layer can safely compress the old raw frame/glyph descriptors. It owns
small addressing and encoded/decoded page buffers and never acquires
ResourceTransferLease. Image and glyph decoders can therefore read a locale
page while they own the shared foreground decoder workspace. Existing NSL2
large directory blocks retain their original resident-prefix rule.

| Edition | UI15 bytes | UI16 bytes | Reduction |
| --- | ---: | ---: | ---: |
| English | 1,130,938 | 779,181 | 351,757 |
| Simplified Chinese | 5,000,436 | 4,456,976 | 543,460 |
| Traditional Chinese | 5,174,475 | 4,622,935 | 551,540 |
| Japanese | 3,925,710 | 3,427,690 | 498,020 |
| Korean | 1,949,066 | 1,560,644 | 388,422 |

The default English/Chinese resource set decreases from 80,155,768 to
79,262,359 bytes, including all concern-file overhead: **893,409 bytes less**.
All 16 locales together save 6,211,620 bytes relative to the repaired locale
inputs. The optional Portuguese/Turkish exact-font repair costs 468 bytes
against the original UI15 inputs, so the complete 16-language reduction from
UI15 is **6,211,152 bytes**. Default English and Chinese incur no glyph repair
increase.

With the final integrated launcher, the default program/resource installation
is 81,021,659 bytes (77.2683 MiB), before saves, notices and explanatory text.
The UI15 equivalent was 81,910,800 bytes: **889,141 bytes less**. These are
installed-file counts, not ZIP download sizes. A smaller download alone is
not reported as a calculator storage improvement.

The final launcher is 4,268 bytes larger than UI15. Including it, installing all
16 languages saves **6,206,884 bytes** against the original UI15 installation.
This pass does not achieve a 30–40 MB or 65–70 MB default installation. It keeps
the existing artwork, animation frames, strings, and glyphs; the remaining
large artwork payload is still the existing compressed representation.

## Bounds and behavior

NSL3 permits at most 4,096 pages (16 MiB logical locale containers). Open reads
only functional headers and the bounded offset table. Compressed page reads
validate exact output lengths; missing/truncated data or invalid references
fail the current read. A failed selected language follows the normal English
recovery path. A failed page never becomes the cached successful page.

NSM1 checks its range map, contiguous virtual coverage and the eight NSS1 part
headers. It does not hash or scan full asset payloads at startup. The offline
converter and release packager perform full byte-identical roundtrips; these
checks are not runtime overhead. No animation frame, source texture pixel,
card, string or valid old glyph is removed.

Final runtime checkpoint `def1db3c68d3730edfddb4985feb403cc86b256b` builds a
1,759,300-byte launcher with SHA-256
`6698dc8e4fb02589ce810a9b1c5bf565601bcdc88b71982457648a07be0f5a3a`.
Its Zehn allocation is 22,568,868 bytes, including 157,048 bytes of metadata;
the contiguous executable/BSS request is 22,411,820 bytes. This is a net
311,028-byte allocation reduction from UI15 after all new code and adapters.
The build enforces a 22,450,000-byte executable/BSS budget in addition to the
existing 24 MiB total limit. Both small storage adapters live in static storage
rather than on the calculator stack. Two mutually exclusive state/raster
workspaces share memory without shrinking the 13,369,344-byte image pool.
Storage copying uses the independently checked ARM926 memory primitives.
Calculator execution, OS heap limits, and final native regression results are
recorded in the UI16 acceptance ledger.

## Investigated alternatives

Tile byte planes, channel predictors, alpha predictors and palette/bit-plane
variants failed to improve the existing encoding in the sampled groups; they
are not shipped. Seven LZMA parameter variants produced only about 0.09%
improvement in the sampled best case, insufficient to justify another codec
profile or a claimed major saving. The current pack's descriptor-pruning pass
reclaims zero bytes. Already cleared unused tiles are not falsely counted as
new savings. Complete Ancient actor animation remains the accepted storage
exception; omitting any additional content was not used to obtain these gains.

## Reproduce

Use `tools/storage_parts.py` with the matching private NSC1 and NSL2 input
folders. Repeated `--extra-locales` directories permit the two exact-font
repairs to override the original optional inputs without changing historical
archives. The generated `storage-receipt.json` records physical sizes and
logical/physical hashes for offline evidence. `tools/package_release.py
--calculator-only` includes all eight required parts, checks reassembly, and
packages only the fourteen named additional language files in the optional
language archive.
