# Compressed locale directories — NSL2

The matched UI10e runtime passed all119 native suites,298 Python tests,
94 bilingual actual-resource targets and18 ordinary browser flows. Its actual
calculator installation passed the packed three-profile capacity and raw
backup migration cases, but failed the raw-history and all-raw migration cases.
The original four save/history/language files were restored and independently
decoded before changing resources again. Those failed capacity receipts remain
failures; the successful UI gates do not establish sufficient device storage.

Further compression of representative existing image/tile/index streams gave
little benefit without changing their layout. Directory bytes are different:
they are still stored uncompressed in both locale files and are already retained
in Content's existing2MiB metadata array after opening. NSL2 compresses that
initial metadata region while preserving the entire logical NSP2/38 stream.
The common artwork file, image pixels, glyphs, strings, frame descriptors,
animation clocks and canonical game state do not change.

## Physical language container

NSL1 remains readable and the default output of the historical split API.
The new `--compress-directory` conversion writes NSL2. Its first96 bytes keep
the old semantic fields, except its magic is `NSL2`, container version is2,
header length is128 and physical file length reflects compressed storage.
Legacy digest fields are not runtime integrity gates.

| Offset | Little-endian32-bit field |
| --- | --- |
| 96 | Compressed directory's original byte count |
| 100 | Independent raw block size:65,536 |
| 104 | Block count, exactly the rounded-up quotient; at most32 |
| 108 | Block table offset:128 |
| 112 | Encoded data offset:128 plus8 times the block count |
| 116 | Raw tail's physical offset |
| 120 | Existing LZMA2 dictionary property:14 |
| 124 | Reserved zero |

Each8-byte table row stores an absolute physical offset and a stored length.
Bit31 of the length identifies a literal block. Literal length must equal the
corresponding original block size; compressed length must be strictly smaller.
The decoder requires exact output and input consumption with an end marker.
The raw tail concatenates the remaining prefix and the locale's unchanged
glyph/text suffix. The separate NSC1 artwork file is byte-identical to UI10e.

The compressed endpoint is `TIL1.descriptors` rounded **down** to4KiB. This
alignment is essential: the first animation descriptor can be unaligned, and
the existing range reader fetches a whole4KiB page while the image decoder
already owns the transfer workspace. Keeping the last partial metadata page
raw ensures those ordinary frame reads never reenter directory decoding.

## Runtime memory and ownership

Each requested compressed block leases the existing foreground Bytes workspace.
The first64KiB holds its decoded bytes and the following64KiB is bounded input
space. The lease ends before another block or a caller callback can use that
storage. There is no new resident byte/pixel pool, full-file checksum scan or
eager image validation. Literal blocks and all ordinary artwork/frame/glyph/text
reads require no directory lease.

`ResourceSource.required_cached_prefix` defaults to zero for old sources.
NSL2 publishes its compressed endpoint through that field; after finding its
ordinary directory boundary, Content rejects a source whose required prefix
would extend beyond the existing metadata cache. This constant-time functional
check avoids either duplicating the tile-header parser or allowing compressed
animation descriptors to fail only later under an image lease.

Independent synthetic tests exercise both locales, old containers, compressed
and literal blocks, partial final blocks, block/tail/artwork boundaries,
malformed tables and LZMA streams, nested workspace owners and callback reentry.
After96 unrelated reader pages evict the first descriptor page, its unaligned
read must still succeed inside an active image Bytes lease without altering
the owner's bytes. Content's separate test accepts the exact cached boundary
and rejects an overrun in both direct and ranged source modes.

## Measured candidate and remaining gates

The completed short actual conversion reconstructs both entire original
NSP2/38 byte streams exactly. It reduces the English locale from2,183,674 to
1,204,517 bytes and Chinese from5,482,564 to4,067,392 bytes: **2,394,329 bytes
saved**, including all new headers and block tables. Resource storage becomes
**90,014,771 bytes**. These are completed candidate measurements, not yet the
final frozen-converter, matched ARM/browser, four-case capacity or quiet timing
acceptance. Those receipts must follow the next verified source checkpoint.

The existing common file remains84,742,862 bytes, SHA-256
`4da593443170433c938fe6de5a323471989b0bb3cdf077b2faece99f9e61a499`.
No image or animation recompression is necessary for this change. The original
30–40MB installation goal is still unmet, and no source or test count is used
to infer a95% original-game fidelity result.
